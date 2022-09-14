

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#include "eqrb_core.h"

#include "misc.h"

#include "../../services/sdtl/sdtl.h" // FIXME divide header to public-opaque and private parts

void *eqrb_alloc(size_t s) {
    return malloc(s);
}


void eqrb_debug_msg(const char *fn, const char *txt, ...) {
    va_list (args);
    char pn[16];
    pthread_getname_np(pthread_self(), pn, sizeof(pn));
    fprintf(stdout, "%s | %s | ", pn, fn);
    va_start (args,txt);
    vfprintf(stdout, txt, args);
    va_end (args);
    fprintf(stdout, "\n");
}

typedef struct {
    eswb_topic_id_t current_tid;
    eswb_topic_id_t next_tid;
    int             do_send_data;

} eqrb_bus_sync_state_t;


typedef enum {
    EQRB_CMD_CLIENT_REQ_SYNC = 0,
    EQRB_CMD_CLIENT_REQ_STREAM = 1,
    EQRB_CMD_SERVER_EVENT = 2,
    EQRB_CMD_SERVER_TOPIC = 3,
    EQRB_CMD_SERVER_RESET = 4,
} eqrb_cmd_code_t;

typedef struct  __attribute__((packed)) eqrb_interaction_header {
    uint8_t msg_code;
    uint8_t reserved[3];
} eqrb_interaction_header_t;

static eqrb_rv_t
send_msg(device_descr_t dd, const driver_t *dr, eqrb_cmd_code_t msg_code, eqrb_interaction_header_t *hdr,
         event_queue_transfer_t *e) {
    hdr->msg_code = msg_code;
    size_t br;

    return dr->send(dd, hdr, sizeof(*hdr) + sizeof(*e) + e->size, &br);;
}

static eqrb_rv_t send_topic(device_descr_t dd, const driver_t *dr, eqrb_bus_sync_state_t *bus_sync_state, event_queue_transfer_t *event, topic_extract_t *topic_info) {
    eswb_topic_id_t parent_tid = topic_info->parent_id;
    event->topic_id = parent_tid;
    event->size = sizeof(topic_proclaiming_tree_t) * 1; // for now sending record by record
    event->type = eqr_topic_proclaim;

    bus_sync_state->current_tid = topic_info->info.topic_id;

    eqrb_dbg_msg("---- send proclaim info for topic \"%s\" tid == %d parent_tid == %d ----",
                 ((topic_proclaiming_tree_t *) EVENT_QUEUE_TRANSFER_DATA(event))->name,
                 ((topic_proclaiming_tree_t *) EVENT_QUEUE_TRANSFER_DATA(event))->topic_id,
                 event->topic_id);
    eqrb_rv_t rv;

    rv = send_msg(dd, dr, EQRB_CMD_SERVER_TOPIC, NULL, event);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("send_msg for EQRB_CMD_SERVER_TOPIC error: %d", rv);
    }

    return rv;
}

void *eqrb_server_thread(void *p) {
    eqrb_rv_t rv;

    eqrb_server_handle_t *h = (eqrb_server_handle_t *) p;
    const driver_t *dev = h->h.driver;

    eswb_rv_t erv;

#   define EVENT_BUF_SIZE 1024
    uint8_t *event_buf = eqrb_alloc(EVENT_BUF_SIZE);
    eqrb_interaction_header_t *hdr = (eqrb_interaction_header_t *) event_buf;

    event_queue_transfer_t *event = (event_queue_transfer_t*)(event_buf + sizeof(*hdr));

    // Let the info for transfer be mapped directly
    topic_extract_t *topic_info = (topic_extract_t *)(EVENT_QUEUE_TRANSFER_DATA(event) - OFFSETOF(topic_extract_t, info));

#define RESET_BUS_SYNC_STATE()  {memset(&bus_sync_state, 0, sizeof(bus_sync_state)); \
                                bus_sync_state.next_tid = 0;}

    eqrb_bus_sync_state_t bus_sync_state;


    device_descr_t dd;
    size_t br;

    int mode_wait_cmd = -1;
    int mode_do_initial_sync = 0;
    int mode_do_stream = 0;

    rv = dev->connect(h->h.connectivity_params, &dd);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("device connection error: %d", rv);
        return NULL;
    }

    do {
        while(mode_wait_cmd) {
            rv = dev->recv(dd, hdr, EVENT_BUF_SIZE, &br);
            if (rv != eqrb_rv_ok) {
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
                    break;
            }
        }

        RESET_BUS_SYNC_STATE();

        while (mode_do_initial_sync) {
            erv = eswb_get_next_topic_info(h->repl_root, &bus_sync_state.next_tid, topic_info);
            if (erv == eswb_e_ok) {
                send_topic(dd, dev, &bus_sync_state, event, topic_info);
            } else {
                eqrb_dbg_msg("Done sending bus state");
                mode_do_initial_sync = 0;
                mode_do_stream = -1; // FIXME programmed somewhere?
                break;
            }
        }

        while (mode_do_stream) {
            erv = eswb_event_queue_pop(h->evq_td, event);
            if (erv != eswb_e_ok) {
                // TODO BREAK_LOOP_AND_RETURN(eqrb_rv_rx_eswb_fatal_err);
                break;
            }

            // TODO detect client events (timeout, stop, cancel, and command to restart)
            rv = send_msg(dd, dev, EQRB_CMD_SERVER_EVENT, hdr, event);
            switch (rv) {
                case eqrb_rv_ok:
                    break;
                // TODO handle restart situations: transfer cancel, reset
                default:
                    eqrb_dbg_msg("send_msg unhandled error: %d", rv);
                    break;
            }
        }
    } while (mode_wait_cmd);

    return NULL;
}


static eqrb_rv_t client_submit_repl_event (eqrb_client_handle_t *handle,
                                             event_queue_transfer_t *event) {

    eqrb_client_handle_t *h = (eqrb_client_handle_t *) handle;
    eqrb_rv_t rv = eqrb_rv_ok;
    eswb_rv_t erv;


    eqrb_dbg_msg("topic_id = %d event_type = %d", event->topic_id, event->type);

    erv = eswb_event_queue_replicate(h->repl_dst_td, &h->ids_map, event);

    switch (erv) {
        case eswb_e_ok:
            eqrb_dbg_msg("eswb_e_ok");
            break;

        case eswb_e_repl_prtree_fraiming_error:
            eqrb_dbg_msg("eswb_e_repl_prtree_fraiming_error");
            break;

        case eswb_e_topic_exist:
            eqrb_dbg_msg("eswb_e_topic_exist");
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
    const driver_t *dev = h->h.driver;

#define RX_BUF_SIZE 1048
    uint8_t *rx_buf = eqrb_alloc(RX_BUF_SIZE);
    eqrb_interaction_header_t *hdr = (eqrb_interaction_header_t *) rx_buf;

    device_descr_t dd;
    size_t br;
    size_t bs;

    int mode_wait_server = -1;
    int mode_wait_events = -1;

    rv = dev->connect(h->h.connectivity_params, &dd);
    if (rv != eqrb_rv_ok) {
        eqrb_dbg_msg("device connection error: %d", rv);
        return NULL;
    }

    do {
        while(mode_wait_server) {
            rv = dev->send(dd, hdr, sizeof(*hdr), &bs);
            if (rv == eqrb_rv_ok) {
                mode_wait_server = 0;
                mode_wait_events = -1;
                break;
            } else {
                eqrb_dbg_msg("device send error: %d", rv);
                return NULL;
            }
        }

        while(mode_wait_events) {
            rv = dev->recv(dd, rx_buf, RX_BUF_SIZE, &br);

            if (rv != eqrb_rv_ok) {
                // TODO detect reset / disconnection
                mode_wait_events = 0;
                mode_wait_server = 0;
                break;
            }

            switch (hdr->msg_code) {
                case EQRB_CMD_SERVER_RESET:
                    mode_wait_events = 0;
                    mode_wait_server = -1;
                    break;

                case EQRB_CMD_SERVER_EVENT:
                case EQRB_CMD_SERVER_TOPIC:
                    ;
                    event_queue_transfer_t *event = (event_queue_transfer_t *) (rx_buf + (sizeof(*hdr)));

                    if (br != sizeof(*hdr) + sizeof(*event) + event->size) {
                        eqrb_dbg_msg("Event size is different from accepted packet size");
                        mode_wait_events = 0;
                        break;
                    }

                    rv = client_submit_repl_event (h, event);
                    if (rv != eqrb_rv_ok) {
                        mode_wait_events = 0;
                        mode_wait_server = 0;
                    }
                    break;

                default:
                    eqrb_dbg_msg("Unknown command code: %d", hdr->msg_code);
                    break;
            }
        }
    } while (mode_wait_server);

    return NULL;
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


#define EQRB_CMD_BUS_NAME "eqrb"

eqrb_rv_t eqrb_client_start(eqrb_client_handle_t *h, const char *mount_point, size_t repl_map_size) {

    eswb_rv_t erv = map_alloc(&h->ids_map, repl_map_size);
    if (erv != eswb_e_ok) {
        eqrb_dbg_msg("map_alloc failed %s", eswb_strerror(erv));
        return eqrb_nomem;
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

eqrb_rv_t eqrb_client_stop(eqrb_client_handle_t *h) {
    return eqrb_service_stop(&h->h);
}

static int server_instance_num;

eqrb_rv_t
eqrb_server_start(eqrb_server_handle_t *h, const char *bus_to_replicate, uint32_t ch_mask, const char **err_msg) {
    eswb_topic_descr_t evq_td;
    eswb_rv_t erv;

    erv = eswb_event_queue_subscribe(bus_to_replicate, &h->evq_td);
    if (erv != eswb_e_ok) {
        if (err_msg != NULL) {
            *err_msg = eswb_strerror(erv);
        }
        eqrb_dbg_msg("eswb_event_queue_subscribe failed: %s", eswb_strerror(erv));
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

    erv = eswb_event_queue_set_receive_mask(h->evq_td, ch_mask);
    if (erv != eswb_e_ok) {
        if (err_msg != NULL) {
            *err_msg = eswb_strerror(erv);
        }
        eqrb_dbg_msg("eswb_event_queue_set_receive_mask failed: %s", eswb_strerror(erv));
        return eqrb_rv_rx_eswb_fatal_err;
    }

    char eqrb_bus[ESWB_BUS_NAME_MAX_LEN + 1];
    snprintf(eqrb_bus, ESWB_BUS_NAME_MAX_LEN, "%s%d", EQRB_CMD_BUS_NAME, server_instance_num);

    int prv;
    // TODO make it OS independent
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    eqrb_dbg_msg("bus2repl == %s", bus_to_replicate);

    eqrb_rv_t rv = eqrb_rv_ok;

    do {
        prv = pthread_create(&h->h.tid, &attr, eqrb_server_thread, h);
        if (prv != 0) {
            rv = eqrb_os_based_err;
            break;
        }
        server_instance_num++;
    } while (0);


    return rv;
}

eqrb_rv_t eqrb_server_stop(eqrb_server_handle_t *h) {
    eqrb_dbg_msg("");
    return eqrb_service_stop(&h->h);
}
