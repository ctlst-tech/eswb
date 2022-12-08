#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#include "eqrb_priv.h"

#include "misc.h"


static eqrb_rv_t
send_msg(device_descr_t dd, const eqrb_media_driver_t *dr, eqrb_cmd_code_t msg_code, eqrb_interaction_header_t *hdr,
         event_queue_transfer_t *e) {
    hdr->msg_code = msg_code;
    size_t br;

    return dr->send(dd, hdr, sizeof(*hdr) + sizeof(*e) + e->size, &br);;
}

static eqrb_rv_t
send_topic(device_descr_t dd, const eqrb_media_driver_t *dr, eqrb_interaction_header_t *hdr, eswb_topic_id_t parent_tid, event_queue_transfer_t *event,
           size_t topics_num) {
    event->topic_id = parent_tid;
    event->size = sizeof(topic_proclaiming_tree_t) * topics_num; // for now sending record by record
    event->type = eqr_topic_proclaim;

    eqrb_dbg_msg("---- send proclaim info for topic \"%s\" tid == %d parent_tid == %d topics_num == %d ----",
                 ((topic_proclaiming_tree_t *) EVENT_QUEUE_TRANSFER_DATA(event))->name,
                 ((topic_proclaiming_tree_t *) EVENT_QUEUE_TRANSFER_DATA(event))->topic_id,
                 event->topic_id,
                 topics_num);
    eqrb_rv_t rv;

    rv = send_msg(dd, dr, EQRB_CMD_SERVER_TOPIC, hdr, event);
    switch (rv) {
        case eqrb_rv_ok:
        case eqrb_media_reset_cmd:
            break;

        case eqrb_media_remote_need_reset:
            // TODO send restart request?
            break;

        default:
            eqrb_dbg_msg("send_msg for EQRB_CMD_SERVER_TOPIC error: %d", rv);
    }

    return rv;
}


static eqrb_rv_t check_state(device_descr_t dd, const eqrb_media_driver_t *dr) {
    return dr->check_state(dd);
}

static void *eqrb_server_sidekick_thread(void *p);

static eqrb_rv_t sidekick_thread_start(eqrb_streaming_sideckick_t *sk) {
    int prv;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    eqrb_rv_t rv = eqrb_rv_ok;

    prv = pthread_create(&sk->tid, &attr, eqrb_server_sidekick_thread, sk);
    if (prv != 0) {
        rv = eqrb_os_based_err;
    }

    return rv;
}

static eqrb_rv_t sidekick_pause(eswb_topic_descr_t td) {
    eqrb_streaming_sideckick_cmd_t cmd;
    cmd.code = SK_PAUSE;
    eswb_rv_t erv = eswb_update_topic(td, &cmd);

    return erv == eswb_e_ok ? eqrb_rv_ok : eqrb_eswb_err;
}

static eqrb_rv_t sidekick_run(eswb_topic_descr_t td) {
    eqrb_streaming_sideckick_cmd_t cmd;
    cmd.code = SK_RUN;
    eswb_rv_t erv = eswb_update_topic(td, &cmd);

    return erv == eswb_e_ok ? eqrb_rv_ok : eqrb_eswb_err;
}

static void *eqrb_server_sidekick_thread(void *p) {
    eqrb_rv_t rv;

    eqrb_streaming_sideckick_t *sk = (eqrb_streaming_sideckick_t *) p;
    const eqrb_media_driver_t *dev = sk->dev;

    eswb_topic_descr_t cmd_td;
    eswb_topic_descr_t eq_td = sk->eq_td;

    eswb_rv_t erv;

    eswb_set_thread_name(__func__);

#   define EVENT_BUF_SIZE_SIDEKICK 2048
    uint8_t *event_buf = eqrb_alloc(EVENT_BUF_SIZE_SIDEKICK);
    if (event_buf == NULL) {
        eqrb_dbg_msg("Buffer allocation error");
        return NULL;
    }
    eqrb_interaction_header_t *hdr = (eqrb_interaction_header_t *) event_buf;

    event_queue_transfer_t *event = (event_queue_transfer_t*)(event_buf + sizeof(*hdr));

    device_descr_t dd;

    rv = dev->connect(sk->connectivity_params, &dd);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("device connection error: %d", rv);
        return NULL;
    }

    erv = eswb_connect(sk->cmd_topic_path, &cmd_td);
    if (erv != eswb_e_ok) {
        eqrb_dbg_msg("eswb_connect error: %s", eswb_strerror(erv));
        return NULL;
    }

    eqrb_streaming_sideckick_cmd_t cmd;
    int loop = -1;

    do {
        while (eswb_get_update(cmd_td, &cmd) == eswb_e_ok && cmd.code == SK_PAUSE);

        erv = eswb_fifo_flush(eq_td);
        if (erv != eswb_e_ok) {
            eqrb_dbg_msg("eswb_fifo_flush error: %s", eswb_strerror(erv));
        }

        while (eswb_read(cmd_td, &cmd) == eswb_e_ok && cmd.code == SK_RUN) {
            erv = eswb_event_queue_pop(eq_td, event);
            switch (erv) {
                case eswb_e_ok:
                    rv = send_msg(dd, dev, EQRB_CMD_SERVER_EVENT, hdr, event);
                    switch (rv) {
                        case eqrb_rv_ok:
                            break;

                        case eqrb_media_reset_cmd:
                            // TODO how to unblock when command is there? push fake event?
                            break;

                        default:
                            eqrb_dbg_msg("send_msg unhandled error: %d", rv);
                            break;
                    }
                    break;

                default:
                    eqrb_dbg_msg("eswb_event_queue_pop error: %d", eswb_strerror(erv));
                    break;
            }
        }

        if (cmd.code == SK_QUIT) {
            loop = 0;
            break;
        }
    } while(loop);

    eqrb_dbg_msg("Thread quits");

    return NULL;
}




static int server_instance_num;

eqrb_rv_t eqrb_server_instance_init(const char *eqrb_instance_name,
                                    const eqrb_media_driver_t *drv,
                                    void *conn_params,
                                    void *conn_params_sk,
                                    eqrb_server_handle_t **h) {
    eqrb_server_handle_t *sh = calloc(1, sizeof(*sh));
    if (sh == NULL) {
        return eqrb_nomem;
    }

    sh->instance_name = strdup(eqrb_instance_name);
    if (sh->instance_name == NULL) {
        return eqrb_nomem;
    }

    sh->h.driver = drv;
    sh->h.connectivity_params = conn_params;
    sh->connectivity_params_sk = conn_params_sk;

    *h = sh;

    return eqrb_rv_ok;
}


static void *eqrb_server_thread(void *p);

eqrb_rv_t
eqrb_server_start(eqrb_server_handle_t *h, const char *bus_to_replicate, uint32_t ch_mask, uint32_t ch_mask_sk, const char **err_msg) {
    eswb_topic_descr_t evq_td;
    eswb_rv_t erv;

    do {
        erv = eswb_event_queue_subscribe(bus_to_replicate, &h->evq_td);
        if (erv != eswb_e_ok) {eqrb_dbg_msg("eswb_event_queue_subscribe failed: %s", eswb_strerror(erv)); break;}

        erv = eswb_event_queue_set_receive_mask(h->evq_td, ch_mask);
        if (erv != eswb_e_ok) {eqrb_dbg_msg("eswb_event_queue_set_receive_mask failed: %s", eswb_strerror(erv)); break;}

        if (ch_mask_sk) {
            erv = eswb_event_queue_subscribe(bus_to_replicate, &h->evq_sk_td);
            if (erv != eswb_e_ok) {eqrb_dbg_msg("eswb_event_queue_subscribe failed: %s", eswb_strerror(erv)); break;}

            erv = eswb_event_queue_set_receive_mask(h->evq_sk_td, ch_mask_sk);
            if (erv != eswb_e_ok) {eqrb_dbg_msg("eswb_event_queue_set_receive_mask failed: %s", eswb_strerror(erv)); break;}
        }
    } while(0);

    if ((erv != eswb_e_ok) && (err_msg != NULL)) {
        *err_msg = eswb_strerror(erv);
        return eqrb_rv_rx_eswb_fatal_err;
    }

    erv = eswb_connect(bus_to_replicate, &h->repl_root);
    if (erv != eswb_e_ok) {
        if (err_msg != NULL) {
            *err_msg = eswb_strerror(erv);
        }
        eqrb_dbg_msg("eswb_connect failed: %s", eswb_strerror(erv));
        return eqrb_rv_rx_eswb_fatal_err;
    }

#   define EQRB_CMD_BUS_NAME "eqrb"

    char eqrb_bus[ESWB_BUS_NAME_MAX_LEN + 1];
    snprintf(eqrb_bus, ESWB_BUS_NAME_MAX_LEN, "%s_%s", EQRB_CMD_BUS_NAME, h->instance_name);
    h->cmd_bus_name = strdup(eqrb_bus);
    if (h->cmd_bus_name == NULL) {
        return eqrb_nomem;
    }

    int prv;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    eqrb_dbg_msg("bus2repl == %s", bus_to_replicate);

    eqrb_rv_t rv = eqrb_rv_ok;

    prv = pthread_create(&h->h.tid, &attr, eqrb_server_thread, h);
    if (prv != 0) {
        rv = eqrb_os_based_err;
    } else {
        server_instance_num++;
    }

    return rv;
}

eqrb_rv_t eqrb_server_stop(eqrb_server_handle_t *h) {
    eqrb_dbg_msg("");
    return eqrb_service_stop(&h->h);
}



static void *eqrb_server_thread(void *p) {
    eqrb_rv_t rv;

    eqrb_server_handle_t *h = (eqrb_server_handle_t *) p;
    const eqrb_media_driver_t *dev = h->h.driver;

    eswb_rv_t erv;

    eswb_set_thread_name(__func__);

#   define EVENT_BUF_SIZE 1024
    uint8_t *event_buf = eqrb_alloc(EVENT_BUF_SIZE);
    eqrb_interaction_header_t *hdr = (eqrb_interaction_header_t *) event_buf;

    event_queue_transfer_t *event = (event_queue_transfer_t*)(event_buf + sizeof(*hdr));

    // Let the info for transfer be mapped directly
    topic_extract_t *topic_info = (topic_extract_t *)(EVENT_QUEUE_TRANSFER_DATA(event));
    topic_proclaiming_tree_t *topic_tree_elem = (topic_proclaiming_tree_t *)(EVENT_QUEUE_TRANSFER_DATA(event));

#define RESET_BUS_SYNC_STATE()  {memset(&bus_sync_state, 0, sizeof(bus_sync_state)); \
                                bus_sync_state.next_tid = 0;}

    eqrb_bus_sync_state_t bus_sync_state;

    device_descr_t dd;
    size_t br;

    int mode_wait_cmd = -1;
    int mode_do_initial_sync = 0;
    int mode_do_stream = 0;
    int keep_sending = 0;

    eqrb_streaming_sideckick_t sk;

#define TRANSITION_TO_WAIT_CMD() mode_wait_cmd = -1; mode_do_initial_sync = 0; mode_do_stream = 0

    rv = dev->connect(h->h.connectivity_params, &dd);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("device connection error: %d", rv);
        return NULL;
    }

    rv = dev->command(dd, eqrb_cmd_reset_remote);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("device command eqrb_cmd_reset_remote error: %d", rv);
        return NULL;
    }

    eswb_topic_descr_t sk_cmd_td = 0;

    if (h->evq_sk_td != 0) {
#define EQRB_CMD_SK_TOPIC_NAME "sk_cmd"
#define CMD_SK_TOPIC_TRAIL "/" EQRB_CMD_SK_TOPIC_NAME
        memset(&sk, 0, (sizeof(sk)));
        sk.dev = h->h.driver;

        sk.cmd_topic_path = malloc(strlen(h->cmd_bus_name) +  strlen(CMD_SK_TOPIC_TRAIL) + 1);
        if (sk.cmd_topic_path == NULL) {
            return NULL;
        }
        strcpy(sk.cmd_topic_path, h->cmd_bus_name);
        strcat(sk.cmd_topic_path, CMD_SK_TOPIC_TRAIL);

        sk.connectivity_params = h->connectivity_params_sk;
        sk.eq_td = h->evq_sk_td;

        erv = eswb_create(h->cmd_bus_name, eswb_inter_thread, 16);
        if (erv != eswb_e_ok) {
            eqrb_dbg_msg("eswb_create for sk failed: %s", eswb_strerror(erv));
            return NULL;
        }

        erv = eswb_proclaim_plain(h->cmd_bus_name, EQRB_CMD_SK_TOPIC_NAME, sizeof(eqrb_streaming_sideckick_cmd_t), &sk_cmd_td);
        if (erv != eswb_e_ok) {
            eqrb_dbg_msg("eswb_proclaim_plain failed: %s", eswb_strerror(erv));
            return NULL;
        }

        rv = sidekick_thread_start(&sk);
        if (rv != eqrb_rv_ok) {
            eqrb_dbg_msg("sidekick_thread_start failed: %s", eswb_strerror(erv));
            return NULL;
        }
    }

    do {
        if (sk_cmd_td != 0) {
            sidekick_pause(sk_cmd_td);
        }

        while(mode_wait_cmd) {
            eqrb_dbg_msg("Waiting client command");
            rv = dev->recv(dd, hdr, EVENT_BUF_SIZE, &br);
            switch (rv) {
                case eqrb_rv_ok:
                    break;

                case eqrb_media_reset_cmd:
                    eqrb_dbg_msg("Got eqrb_media_reset_cmd");
                    dev->command(dd, eqrb_cmd_reset_local_state);
                    continue;

                default:
                    eqrb_dbg_msg("device recv error: %d", rv);
                    return NULL;
            }

            switch (hdr->msg_code) {
                case EQRB_CMD_CLIENT_REQ_SYNC:
                    mode_do_initial_sync = -1;
                    mode_wait_cmd = 0;
                    break;

                case EQRB_CMD_CLIENT_REQ_STREAM:
                    mode_do_stream = -1;
                    mode_wait_cmd = 0;
                    break;

                default:
                    // TODO send: invalid cmd back
                    eqrb_dbg_msg("Invalid cmd, waiting next");
                    break;
            }
        }


        if (mode_do_initial_sync) {
            RESET_BUS_SYNC_STATE();
            eqrb_dbg_msg("Do initial topics sync data");

            while (mode_do_initial_sync) {
                erv = eswb_get_next_topic_info(h->repl_root, &bus_sync_state.next_tid,
                                               /*This is nasty but efficient >> */(topic_extract_t *) &topic_tree_elem[0]);
                if (erv == eswb_e_ok) {
                    size_t topics_num = 1;
                    if (topic_info->info.type == tt_fifo) {
                        erv = eswb_get_next_topic_info(h->repl_root, &bus_sync_state.next_tid,
                                                       /*This is nasty but efficient >> */(topic_extract_t *) &topic_tree_elem[1]);
                        if (erv == eswb_e_ok) {
                            topics_num++;
                            topic_tree_elem[0].first_child_ind = 1;
                        }
                    }
                    // insted of topic_tree_elem structure topic_extract_t is extracted with trailing additional info
                    bus_sync_state.current_tid = ((topic_extract_t *) &topic_tree_elem[topics_num - 1])->info.topic_id;

                    keep_sending = -1;
                    do {
                        rv = send_topic(dd, dev, hdr, topic_info->parent_id, event, topics_num);
                        switch (rv) {
                            case eqrb_rv_ok:
                                keep_sending = 0;
                                break;

                            case eqrb_media_reset_cmd:
                                TRANSITION_TO_WAIT_CMD();
                                keep_sending = 0;
                                eqrb_dbg_msg("Server reset requested by client");
                                break;

                            default:
                                eqrb_dbg_msg("send topic error: %d", rv);
                                break;
                        }
                    } while (keep_sending);
                } else {
                    eqrb_dbg_msg("Done sending bus state");
                    mode_do_initial_sync = 0;
                    mode_do_stream = -1; // FIXME programmed somewhere?
                    break;
                }
            }
        }

        if (mode_do_stream) {
            eqrb_dbg_msg("Streaming data");
            if (sk_cmd_td != 0) {
                sidekick_run(sk_cmd_td);
            }
            do {
                eswb_arm_timeout(h->evq_td, 500000);
                erv = eswb_event_queue_pop(h->evq_td, event);
                switch (erv) {
                    case eswb_e_timedout:
                        rv = check_state(dd, dev);
                        if (rv == eqrb_media_reset_cmd) {
                            TRANSITION_TO_WAIT_CMD();
                        }
                        continue;

                    case eswb_e_ok:
                        break;

                    default:
                        // TODO BREAK_LOOP_AND_RETURN(eqrb_rv_rx_eswb_fatal_err);
                        eqrb_dbg_msg("eswb_event_queue_pop error: %s", eswb_strerror(erv));
                        break;
                }

                rv = send_msg(dd, dev, EQRB_CMD_SERVER_EVENT, hdr, event);
                switch (rv) {
                    case eqrb_rv_ok:
                        break;

                    case eqrb_media_reset_cmd:
                    TRANSITION_TO_WAIT_CMD();
                        break;

                    default:
                        eqrb_dbg_msg("send_msg unhandled error: %d", rv);
                        break;
                }
            } while (mode_do_stream);
        }
    } while (mode_wait_cmd);

    return NULL;
}

