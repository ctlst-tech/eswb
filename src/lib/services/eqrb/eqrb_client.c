#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include "eqrb_priv.h"
#include "misc.h"


static eqrb_rv_t client_submit_repl_event (eqrb_client_handle_t *handle,
                                             event_queue_transfer_t *event) {

    eqrb_client_handle_t *h = (eqrb_client_handle_t *) handle;
    eqrb_rv_t rv = eqrb_rv_ok;
    eswb_rv_t erv;


//    eqrb_dbg_msg("topic_id = %d event_type = %d", event->topic_id, event->type);

    erv = eswb_event_queue_replicate(h->repl_dst_td, h->ids_map, event);

    switch (erv) {
        case eswb_e_ok:
//            eqrb_dbg_msg("eswb_e_ok");
            break;

        case eswb_e_repl_prtree_fraiming_error:
            eqrb_dbg_msg("eswb_e_repl_prtree_fraiming_error");
            break;

        case eswb_e_topic_exist:
            eqrb_dbg_msg("eswb_e_topic_exist (tid %d)", event->topic_id);
            break;

        case eswb_e_map_full:
            eqrb_dbg_msg("!!! eswb_e_map_full");
            eqrb_dbg_msg("!!! eswb_e_map_full");
            eqrb_dbg_msg("!!! eswb_e_map_full");
            eqrb_dbg_msg("!!! eswb_e_map_full");
            break;

        case eswb_e_map_no_match:
            eqrb_dbg_msg("eswb_e_map_no_match");
            break;

        default:
            eqrb_dbg_msg("unhandled error of eswb_event_queue_replicate: %s", eswb_strerror(erv));
            rv = eqrb_rv_rx_eswb_fatal_err;
            break;
    }

    return rv;
}

void *eqrb_client_thread(eqrb_client_handle_t *p) {
    eqrb_client_handle_t *h = p;
    eswb_rv_t erv;
    eqrb_rv_t rv = eqrb_rv_ok;
    const eqrb_media_driver_t *dev = h->h.driver;

    int stream_only = h->launch_sidekick;

    if (!stream_only) {
        eswb_set_thread_name(__func__);
    } else {
        eswb_set_thread_name("eqrb_client_stream");
    }

#define RX_BUF_SIZE 1048
    uint8_t *rx_buf = eqrb_alloc(RX_BUF_SIZE);
    eqrb_interaction_header_t *hdr = (eqrb_interaction_header_t *) rx_buf;

    device_descr_t dd;
    size_t br;
    size_t bs;

    int mode_wait_server = -1;
    int mode_wait_events = -1;

    int server_reset_requested;

    rv = dev->connect(h->h.connectivity_params, &dd);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("device connection error: %d", rv);
        return NULL;
    }

    if (!stream_only) {
        rv = dev->command(dd, eqrb_cmd_reset_remote);
        if (rv != eqrb_rv_ok) {
            eqrb_dbg_msg("device command 'eqrb_cmd_reset_remote' error: %d", rv);
        }
    } else {
        mode_wait_server = 0;
        mode_wait_events = -1;
    }

    do {
        server_reset_requested = 0;

        while(mode_wait_server) {
            eqrb_dbg_msg("Sending init command to server");

            hdr->msg_code = EQRB_CMD_CLIENT_REQ_SYNC;
            rv = dev->send(dd, hdr, sizeof(*hdr), &bs);
            switch (rv) {
                case eqrb_rv_ok:
                    mode_wait_server = 0;
                    mode_wait_events = -1;
                    break;

                case eqrb_media_remote_need_reset:
                    rv = dev->command(dd, eqrb_cmd_reset_remote);
                    if (rv != eqrb_rv_ok) {
                        eqrb_dbg_msg("device command 'eqrb_cmd_reset_remote' error: %d", rv);
                    }
                    if (server_reset_requested) {
                        usleep(100000);
                    } else {
                        server_reset_requested = -1;
                    }
                    break;

                case eqrb_media_reset_cmd:
                    eqrb_dbg_msg("Got eqrb_media_reset_cmd");
                    rv = dev->command(dd, eqrb_cmd_reset_local_state);
                    if (rv != eqrb_rv_ok) {
                        eqrb_dbg_msg("device command 'eqrb_cmd_reset_local_state' error: %d", rv);
                    }
                    continue;

                default:
                    eqrb_dbg_msg("device send error: %d", rv);
                    return NULL;
            }
        }

        int got_first_event = 0;

        while(mode_wait_events) {
            uint32_t timeout = got_first_event ? 0 : 500000;
            rv = dev->recv(dd, rx_buf, RX_BUF_SIZE, &br, timeout);

            switch (rv) {
                case eqrb_rv_ok:
                    got_first_event = -1;
                    break;

                case eqrb_media_timedout:
                    mode_wait_server = -1;
                    mode_wait_events = 0;
                    eqrb_dbg_msg("Got timeout on mode_wait_events state, return to mode_wait_server");
                    break;

                case eqrb_media_reset_cmd:
                    if (!stream_only) {
                        mode_wait_server = -1;
                        mode_wait_events = 0;
                        eqrb_dbg_msg("Client reset requested by server");
                    } else {
                        dev->command(dd, eqrb_cmd_reset_local_state);
                    }
                    break;

                default:
                    // TODO detect reset / disconnection
//                    mode_wait_events = 0;
//                    mode_wait_server = 0;
                    eqrb_dbg_msg("recv error: %d", rv);
                    break;
            }

            if (mode_wait_events) {
                switch (hdr->msg_code) {
                    case EQRB_CMD_SERVER_EVENT:
                    case EQRB_CMD_SERVER_TOPIC:;
                        event_queue_transfer_t *event = (event_queue_transfer_t *) (rx_buf + (sizeof(*hdr)));

                        if (br != sizeof(*hdr) + sizeof(*event) + event->size) {
                            eqrb_dbg_msg("Event size is different from accepted packet size");
//                        mode_wait_events = 0;
                            break;
                        }

                        if (hdr->msg_code == EQRB_CMD_SERVER_TOPIC) {
                            eqrb_dbg_msg(
                                    "---- got proclaim info for topic \"%s\" tid == %d parent_tid == %d topics_num == %d ----",
                                    ((topic_proclaiming_tree_t *) EVENT_QUEUE_TRANSFER_DATA(event))->name,
                                    ((topic_proclaiming_tree_t *) EVENT_QUEUE_TRANSFER_DATA(event))->topic_id,
                                    event->topic_id,
                                    event->size / sizeof(topic_proclaiming_tree_t));
                        }


                        rv = client_submit_repl_event(h, event);
                        if (rv != eqrb_rv_ok) {
                            eqrb_dbg_msg("Uclient_submit_repl_event failure: %d", rv);
//                        mode_wait_events = 0;
//                        mode_wait_server = 0;
                        }
                        break;

                    default:
                        eqrb_dbg_msg("Unknown command code: %d", hdr->msg_code);
                        break;
                }
            }
        }
    } while (mode_wait_server);

    return NULL;
}



eqrb_rv_t eqrb_client_start(eqrb_client_handle_t *h, const char *mount_point, size_t repl_map_size) {

    eswb_rv_t erv;

    if (h->ids_map == NULL) {
        erv = map_alloc(&h->ids_map, repl_map_size);
        if (erv != eswb_e_ok) {
            eqrb_dbg_msg("map_alloc failed %s", eswb_strerror(erv));
            return eqrb_nomem;
        }
    }

    erv = eswb_connect(mount_point, &h->repl_dst_td);
    if (erv != eswb_e_ok) {
        eqrb_dbg_msg("eswb_connect to %s failed %s", mount_point, eswb_strerror(erv));
        return eqrb_rv_rx_eswb_fatal_err;
    }

    int prv = pthread_create(&h->h.tid, NULL, (void *)(void*) eqrb_client_thread, h);
    if (prv != 0) {
        return eqrb_os_based_err;
    }

    return eqrb_rv_ok;
}


eqrb_rv_t eqrb_service_stop(eqrb_handle_common_t *h) {
    int *rx_rv;
    int *tx_rv;

    int rv;

    if (h->tid != 0) {
        rv = pthread_cancel(h->tid);
//        pthread_join(h->tid, (void **) &rx_rv);
    }

    return eqrb_rv_ok;
}


eqrb_rv_t eqrb_client_stop(eqrb_client_handle_t *h) {
    return eqrb_service_stop(&h->h);
}


// TODO to DELETE:


