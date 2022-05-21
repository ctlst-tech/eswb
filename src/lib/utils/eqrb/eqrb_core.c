

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

#include "eqrb_core.h"

#include "framing.h"
#include "misc.h"


eqrb_rv_t
eqrb_make_tx_frame_from_event(event_queue_transfer_t *event, uint8_t command_code, uint8_t *frame_buf, size_t frame_buf_size,
                              size_t *frame_size) {
    return eqrb_make_tx_frame(command_code, event, sizeof(*event) + event->size, frame_buf, frame_buf_size, frame_size);
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

static eqrb_rv_t send_event(event_queue_transfer_t *e, device_descr_t dd, const driver_t *dr, uint8_t *tx_buf, size_t tx_buff_size) {
    size_t tx_frame_size;
    eqrb_rv_t rv;

    rv = eqrb_make_tx_frame_from_event(e, EQRB_DOWNSTREAM_CODE_BUS_DATA, tx_buf, tx_buff_size,
                                       &tx_frame_size);
    if (rv != eqrb_rv_ok) {
        return rv;
    }

    return dr->send(dd, tx_buf, tx_frame_size, NULL);;
}

typedef enum {
    connect_to_device,
    wait_client,
    sync_bus_begin,
    sync_bus_process,
    sync_bus_wack,
    stream_bus_events,
    stream_bus_events_wack,
    suspend
} eqrb_server_state_t;

static const char *server_state_alias(eqrb_server_state_t s) {
    switch (s) {
        case connect_to_device:       return "CONNECT_TO_DEVICE";
        case wait_client:             return "WAIT_CLIENT";
        case sync_bus_begin:          return "SYNC_BUS_BEGIN";
        case sync_bus_process:        return "SYNC_BUS_PROCESS";
        case sync_bus_wack:           return "SYNC_BUS_WACK";
        case stream_bus_events:       return "STREAM_BUS_EVENTS";
        case stream_bus_events_wack:  return "STREAM_BUS_EVENTS_WACK";
        case suspend:                 return "SUSPEND";
        default:                      return "UNKNOWN";
    }
}


eqrb_rv_t eqrb_server_stop(eqrb_server_handle_t *h);

eqrb_rv_t eqrb_server_tx_thread(eqrb_server_handle_t *p) {
    eqrb_server_handle_t *h = p;
    eswb_rv_t erv;
    eqrb_rv_t rv;

    uint8_t event_buf[1024];
    uint8_t tx_buf[2048];
    event_queue_transfer_t *event = (event_queue_transfer_t*)event_buf;
    topic_extract_t *topic_info = (topic_extract_t *)(event->data - OFFSETOF(topic_extract_t, info)); // Let the info to transfer be mapped directly

    device_descr_t dd = 0;
    int have_initial_dd = 0;
    if (h->h.dd != 0) {
        dd = h->h.dd;
        have_initial_dd = -1;
    }

    int loop = 1;

    eqrb_server_state_t server_state = connect_to_device;
    eqrb_server_state_t prev_server_state = server_state;


    const driver_t *dr = h->h.driver;

#   define BREAK_LOOP_AND_RETURN(___rv) loop = 0; rv = ___rv
    server_bus_commands_t cmd;
    int wait_command_blocked = 0;

    eswb_topic_descr_t cmd_td;

    eqrb_bus_sync_state_t bus_sync_state;

    erv = eswb_wait_connect_nested(h->h.cmd_bus_root_td, EQRV_SERVER_COMMAND_FIFO "/" EQRB_COMMAND_TOPIC, &cmd_td, 200);
    if (erv != eswb_e_ok) {
        // TODO
        return eqrb_rv_rx_eswb_fatal_err;
    }

    eqrb_dbg_msg("started");

    do {
        if (prev_server_state != server_state) {
            eqrb_dbg_msg("Server state changed from %s to %s",
                         server_state_alias(prev_server_state),
                         server_state_alias(server_state)
                         );
            prev_server_state = server_state;
        }
        switch (server_state) {
            default:
                server_state = connect_to_device;
                break;

            case connect_to_device:
                if (have_initial_dd) {
                    server_state = wait_client;
                    have_initial_dd = 0;
                    break;
                }
                wait_command_blocked = 0;
                if (dd > 0) {
                    if (dr->disconnect(dd)) {
                        eqrb_server_stop(h);  // TODO deal with pthread cancellation (which not working in this case)
                        pthread_exit(NULL);
                    }
                }

                rv = dr->connect("some params", &dd);
                if (rv == eqrb_rv_ok) {
                    server_state = wait_client;
                } else {
                    usleep(250000); // FIXME platformic call
                }
                continue; // TODO do we need to accept commands from fifo during the device connection?
                break;

            case wait_client:
                if (dr->type == eqrb_duplex) {
                    wait_command_blocked = -1;
                } else {
                    server_state = sync_bus_begin;
                    wait_command_blocked = 0;
                }
                break;

            case sync_bus_begin:
                memset(&bus_sync_state, 0, sizeof(bus_sync_state));
                bus_sync_state.next_tid = 0;

                wait_command_blocked = 0;
                server_state = sync_bus_process;
                break;

            case sync_bus_process:
                wait_command_blocked = 0;

                if (!bus_sync_state.do_send_data) {
                    // send topic info
                    erv = eswb_get_next_topic_info(h->repl_root, &bus_sync_state.next_tid, topic_info);
                    if (erv == eswb_e_ok) {
                        eswb_topic_id_t parent_tid = topic_info->parent_id;
                        event->topic_id = parent_tid;
                        event->size = sizeof(topic_proclaiming_tree_t) * 1; // for now sending record by record
                        event->type = eqr_topic_proclaim;

                        bus_sync_state.current_tid = topic_info->info.topic_id;

                        eqrb_dbg_msg("---- send proclaim info for topic \"%s\" tid == %d parent_tid == %d ----",
                                     ((topic_proclaiming_tree_t *)event->data)->name,
                                     ((topic_proclaiming_tree_t *)event->data)->topic_id,
                                     event->topic_id);

                        rv = send_event(event, dd, dr, tx_buf, sizeof(tx_buf));
                        if (rv != eqrb_rv_ok) {
                            eqrb_dbg_msg("send_event regarding bus sync error: %d", rv);
                        }

                        // TODO add counter to control ack
                        server_state = sync_bus_wack;
                    } else {
                        eqrb_dbg_msg("Done sending bus state");
                        server_state = stream_bus_events;
                    }
                } else {
                    // send data
                    // TODO for now we dont send data
                }

                break;

            case stream_bus_events_wack:
            case sync_bus_wack:
                // TODO process timeout
                wait_command_blocked = -1;
//  server_state = sync_bus_state_process;
                break;

            case stream_bus_events:
                wait_command_blocked = 0;
                erv = eswb_event_queue_pop(h->evq_td, event);
                if (erv != eswb_e_ok) {
                    BREAK_LOOP_AND_RETURN(eqrb_rv_rx_eswb_fatal_err);
                    break;
                }

                rv = send_event(event, dd, dr, tx_buf, sizeof(tx_buf));
                switch (rv) {
                    case eqrb_rv_ok:
                        break;

                    case eqrb_media_err:
                    case eqrb_media_stop:
                        server_state = connect_to_device;
                        break;

                    default:
                        eqrb_dbg_msg("send_event unhandled error: %d", rv);
                        break;
                }

                if (event->type == eqr_topic_proclaim) {
                    server_state = stream_bus_events_wack; // TODO must have additiotnal rules in a future
                }

                break;

            case suspend:
                wait_command_blocked = -1;
                break;
        } // switch end

        // TODO process all available commands in a loop
        erv = (wait_command_blocked ?
                eswb_fifo_pop :
                eswb_fifo_try_pop) (cmd_td, &cmd);

        if (erv == eswb_e_ok) {
            switch (cmd.code) {
                default:
                    break;

                case start_stream:
                    if (server_state == wait_client) {
                        server_state = sync_bus_begin;
                    }
                    break;

                case send_proclaiming_info:
                    // server_state = sync_bus_begin;
                    // TODO for now, streaming all bus
                    break;

                case ack:
                    eqrb_dbg_msg("got ack");

                    switch (server_state) {
                        case sync_bus_wack:
                            server_state = sync_bus_process;
                            break;

                        default: // TODO handle invcalid state and restart?
                        case stream_bus_events_wack:
                            server_state = stream_bus_events;
                            break;
                    }
                    break;

                case stop_stream:
                    server_state = suspend;
                    break;

                case stop_service:
                    loop = 0;
                    break;
            }
        }
    } while(loop);

    // TODO free resources

    return rv;
}

eqrb_rv_t eqrb_generic_rx_thread(eqrb_handle_common_t *p,
                                 eqrb_rv_t (*rx_got_frame_handler)
                                        (void *handle, uint8_t cmd_code, uint8_t *data, size_t data_len));


void *eqrb_server_tx_thread_call(void *p) {
    static eqrb_rv_t rv;

    eswb_set_thread_name("eqbr_server_tx");

    rv = eqrb_server_tx_thread((eqrb_server_handle_t *)p);
    return &rv;
}



eqrb_rv_t eqrb_server_rx_handler_cmd (void *handle, uint8_t cmd_code, uint8_t *data, size_t data_len) {

    eqrb_server_handle_t *h = (eqrb_server_handle_t *) handle;
    eqrb_rv_t rv = eqrb_rv_ok;
    eswb_rv_t erv;

    server_bus_commands_t command;

    eqrb_dbg_msg("cmd = %02X", cmd_code);

    switch (cmd_code) {
        case EQRB_UPSTREAM_CODE_CLIENT_READY:
            command.code = start_stream;
            erv = eswb_fifo_push(h->cmd_bus_publisher_td, &command);
            if (erv != eswb_e_ok) {
                rv = eqrb_rv_rx_eswb_fatal_err;
            }
            break;

        case EQRB_UPSTREAM_CODE_RESEND_TID:
            command.code = send_proclaiming_info;
            command.param = *((uint32_t *) data);
            eqrb_dbg_msg("Requesting ID to proclaim tid = %d", command.param);
            erv = eswb_fifo_push(h->cmd_bus_publisher_td, &command);
            if (erv != eswb_e_ok) {
                rv = eqrb_rv_rx_eswb_fatal_err;
            }
            break;

        case EQRB_UPSTREAM_CODE_ACK:
            command.code = ack;
            command.param = *((uint32_t *) data);
            eqrb_dbg_msg("Ack with param = %d", command.param);
            erv = eswb_fifo_push(h->cmd_bus_publisher_td, &command);
            if (erv != eswb_e_ok) {
                rv = eqrb_rv_rx_eswb_fatal_err;
            }
            break;

        default:
            break;
    }

    return rv;
}

void *eqrb_server_rx_thread_call(void *p) {
    static eqrb_rv_t rv;

    eqrb_server_handle_t *h = (eqrb_server_handle_t *) p;

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 5);
    topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, EQRV_SERVER_COMMAND_FIFO, 5); // TODO confusing name, same as integrated service name
    topic_proclaiming_tree_t *elem_root = usr_topic_add_child(cntx, fifo_root, EQRB_COMMAND_TOPIC, tt_struct, 0, sizeof(server_bus_commands_t), TOPIC_FLAG_MAPPED_TO_PARENT);

    //usr_topic_add_struct_child(cntx, fifo_root, fifo_elem, "event", tt_struct);
    usr_topic_add_struct_child(cntx, elem_root, server_bus_commands_t, code, "code", tt_uint32);
    usr_topic_add_struct_child(cntx, elem_root, server_bus_commands_t, param, "param", tt_uint32);

    eswb_rv_t erv = eswb_proclaim_tree(h->h.cmd_bus_root_td, fifo_root, cntx->t_num, &h->cmd_bus_publisher_td);

    eswb_set_thread_name("eqbr_server_rx");

    if (erv != eswb_e_ok) {
        rv = eqrb_rv_rx_eswb_fatal_err;
    } else {
        rv = eqrb_generic_rx_thread((eqrb_handle_common_t *) p, eqrb_server_rx_handler_cmd);
    }

    return &rv;
}

static eqrb_rv_t send_command(const driver_t *dr, device_descr_t dd, uint8_t cmd_code, uint32_t param) {

    uint8_t frame_buf[20];
    size_t fr_size;

    eqrb_dbg_msg("cmd == 0x%02X param == 0x%08X", cmd_code, param);

    eqrb_rv_t rv = eqrb_make_tx_frame(cmd_code, &param, sizeof(param), frame_buf, sizeof(frame_buf), &fr_size);
    if (rv != eqrb_rv_ok) {
        return rv;
    }

    return dr->send(dd, frame_buf, fr_size, NULL);
}

static eqrb_rv_t send_ack(const driver_t *dr, device_descr_t dd, uint32_t param) {
    return send_command(dr, dd, EQRB_UPSTREAM_CODE_ACK, param);
}

eqrb_rv_t eqrb_client_rx_handler_replicator (void *handle, uint8_t cmd_code, uint8_t *data, size_t data_len) {

    eqrb_client_handle_t *h = (eqrb_client_handle_t *) handle;
    eqrb_rv_t rv = eqrb_rv_ok;
    eswb_rv_t erv;

//    eqrb_dbg_msg("cmd = 0x%02X datalen = %d", cmd_code, data_len);

    switch (cmd_code) {
        case EQRB_DOWNSTREAM_CODE_BUS_DATA:
            ;
            event_queue_transfer_t *event =  (event_queue_transfer_t *) data;
            if (event->type == eqr_topic_proclaim) {
                rv = send_ack(h->h.driver, h->h.dd, 0);
            }

            eqrb_dbg_msg("EQRB_DOWNSTREAM_CODE_BUS_DATA topic_id = %d event_type = %d", event->topic_id, event->type);

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
                    eqrb_dbg_msg("ESWB_E_MAP_FULL");
                    eqrb_dbg_msg("ESWB_E_MAP_FULL");
                    eqrb_dbg_msg("ESWB_E_MAP_FULL");
                    eqrb_dbg_msg("ESWB_E_MAP_FULL");
                    break;

                case eswb_e_map_no_match:
                    // TODO buffer this situation, don't send packet on every missing TID lookup
                    rv = send_command(h->h.driver, h->h.dd, EQRB_UPSTREAM_CODE_RESEND_TID, event->topic_id);
                    break;

                default:
                    eqrb_dbg_msg("unhandled error of eswb_event_queue_replicate: %s", eswb_strerror(erv));
                    rv = eqrb_rv_rx_eswb_fatal_err;
                    break;
            }
            // TODO post errors;
//            if (erv != eswb_e_ok) {
//                rv = eqrb_rv_rx_eswb_fatal_err; // FIXME, err code must be more specific
//            }
            break;

        case EQRB_UPSTREAM_CODE_CLIENT_READY:
            break;

        default:
            break;
    }

    return rv;
}

void *eqrb_client_rx_thread_call(void *p) {
    static eqrb_rv_t rv;
    eqrb_client_handle_t *h = (eqrb_client_handle_t *)p;

    eswb_set_thread_name("eqbr_client_rx");

//    rv = h->h.driver->connect(NULL, &h->dr_tx_handle);
//    if (rv != eqrb_rv_ok) {
//        return &rv;
//    }

    // FIXME seems like connection routine inside this threads is useless. As well for any form of reconnection?
    // TODO if have reconnection routine - reconnect, if not, terminate thread

    send_command(h->h.driver, h->h.dd, EQRB_UPSTREAM_CODE_CLIENT_READY, 0);

    if (rv == eqrb_rv_ok) {
        rv = eqrb_generic_rx_thread((eqrb_handle_common_t *) h, eqrb_client_rx_handler_replicator);
    }

    return &rv;
}

eqrb_rv_t eqrb_generic_rx_thread(eqrb_handle_common_t *p,
                                 eqrb_rv_t (*rx_got_frame_handler)
                                    (void *handle, uint8_t cmd_code, uint8_t *data, size_t data_len)) {
    eqrb_handle_common_t *h = p;
    eswb_rv_t erv;
    eqrb_rv_t rv = eqrb_rv_ok;

    eqrb_rx_state_t rx_state;

    uint8_t rx_buf[2048];

    uint8_t payload_buf[2048];

    int loop = 1;

    eqrb_init_state(&rx_state, payload_buf, sizeof(payload_buf));


    device_descr_t dd = 0;
    int have_initial_dd = 0;
    int device_is_fine = 0;

    if (h->dd != 0) {
        dd = h->dd;
        have_initial_dd = -1;
        device_is_fine = -1;
    }

    const driver_t *dr = h->driver;

    eqrb_dbg_msg("started (%s)", have_initial_dd ? "have_initial_dd" : "");

    do {
        if (!have_initial_dd) {
            do {
                if (dd != 0) {
                    if (dr->disconnect(dd)) {
                        // TODO stop service
                        // FIXME TX side does stop
                        pthread_exit(NULL);
                    }
                }
                rv = dr->connect("some params", &dd);
                if (rv != eqrb_rv_ok) {
                    usleep(250000); // TODO platform agnostic
                } else {
                    device_is_fine = -1;
                }
            } while (rv != eqrb_rv_ok);
        } else {
            have_initial_dd = 0;
        }

        do {
            size_t rx_buf_lng;
            rv = dr->recv(dd, rx_buf, sizeof(rx_buf), &rx_buf_lng);
            eqrb_dbg_msg("recv | rx_buf_lng == %d rv == %d", rx_buf_lng, rv);
            if (rv == eqrb_rv_ok) {
                size_t bp = 0;
                size_t total_br = 0;
                do {
                    rv = eqrb_rx_frame_iteration(&rx_state, rx_buf + total_br, rx_buf_lng - total_br, &bp);

                    switch (rv) {
                        default:
                        case eqrb_rv_ok:
                            break;

                        case eqrb_rv_rx_buf_overflow:
                        case eqrb_rv_rx_inv_crc:
                        case eqrb_rv_rx_got_empty_frame:
                            eqrb_reset_state(&rx_state);
                            eqrb_dbg_msg("eqrb_reset_state");
                            break;

                        case eqrb_rv_rx_got_frame:

                            rx_got_frame_handler(h, rx_state.current_command_code,
                                                            rx_state.payload_buffer_origin,
                                                            rx_state.current_payload_size);
                            eqrb_dbg_msg("rx_got_frame_handler");
                            eqrb_reset_state(&rx_state);
                            break;
                    }

                    total_br += bp;
                } while (total_br < rx_buf_lng);
            } else {
                // TODO tell disconnecting errors from fatal errors
                eqrb_dbg_msg("rv = %d", rv);
                device_is_fine = 0;
                if (rv < 0) { // FIXME useless statement just to break endless loop for lint
                    loop = 0;
                }
            }
        } while (device_is_fine);

    } while (loop);

    return rv;
}

//void *eqrb_generic_rx_thread_call(void *p) {
//    static eqrb_rv_t rv;
//    rv = eqrb_generic_rx_thread((eqrb_server_handle_t *)p);
//    return &rv;
//}


static eqrb_rv_t cmd_bus_init(const char *bus_name, eswb_topic_descr_t *root_td) {

    eswb_rv_t erv;

    erv = eswb_create(bus_name, eswb_inter_thread, 5);
    if (erv != eswb_e_ok) {
        return eqrb_rv_rx_eswb_fatal_err;
    }

    char root_path[ESWB_TOPIC_NAME_MAX_LEN + 1] = "itb:/";
    strncat(root_path, bus_name, ESWB_TOPIC_NAME_MAX_LEN - strlen(root_path));

    erv = eswb_topic_connect(root_path, root_td);
    if (erv != eswb_e_ok) {
        return eqrb_rv_rx_eswb_fatal_err;
    }

    return eqrb_rv_ok;
}

static eqrb_rv_t cmd_bus_deinit(eswb_topic_descr_t root_td) {
    return eswb_delete_by_td(root_td) == eswb_e_ok ? eqrb_rv_ok : eqrb_rv_rx_eswb_fatal_err;
}



eqrb_rv_t eqrb_service_stop(eqrb_handle_common_t *h) {
    int *rx_rv;
    int *tx_rv;

    int rv;

    if (h->tid_rx_thread != 0) {
        rv = pthread_cancel(h->tid_rx_thread);
//        pthread_join(h->tid_rx_thread, (void **) &rx_rv);

    }

    if (h->tid_tx_thread != 0) { // might be self in some case
        rv = pthread_cancel(h->tid_tx_thread);
//        pthread_join(h->tid_tx_thread, (void **) &tx_rv);
    }

    cmd_bus_deinit(h->cmd_bus_root_td);

    return eqrb_rv_ok;
}


#define EQRB_CMD_BUS_NAME "eqrb"

eqrb_rv_t eqrb_client_start(eqrb_client_handle_t *h, const char *mount_point, size_t repl_map_size) {

    eswb_rv_t erv = map_alloc(&h->ids_map, repl_map_size);
    if (erv != eswb_e_ok) {
        return eqrb_rv_rx_eswb_fatal_err;
    }

    erv = eswb_topic_connect(mount_point, &h->repl_dst_td);
    if (erv != eswb_e_ok) {
        return eqrb_rv_rx_eswb_fatal_err;
    }

    // client dont need a commanding bus
//    eqrb_rv_t rv = init_cmd_bus(eqrb_CMD_BUS_NAME, &h->h.cmd_bus_root_td);
//    if (rv != eqrb_rv_ok) {
//        return rv;
//    }

    int prv = pthread_create(&h->h.tid_rx_thread, NULL, eqrb_client_rx_thread_call, h);
    if (prv != 0) {
        return eqrb_os_based_err;
    }


    h->h.tid_tx_thread = 0;

    //pthread_create(&h->h.tid_tx_thread, NULL, __, &h);

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

    erv = eswb_topic_connect(bus_to_replicate, &h->repl_root);
    if (erv != eswb_e_ok) {
        if (err_msg != NULL) {
            *err_msg = eswb_strerror(erv);
        }
        eqrb_dbg_msg("eswb_topic_connect failed: %s", eswb_strerror(erv));
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

    eqrb_rv_t rv = cmd_bus_init(eqrb_bus, &h->h.cmd_bus_root_td);
    if (rv != eqrb_rv_ok) {
        if (err_msg != NULL) {
            *err_msg = "cmd_bus_init failed";
        }
        eqrb_dbg_msg("cmd_bus_init failed");
        return rv;
    }

    int prv;
    // TODO make it OS independent
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    eqrb_dbg_msg("bus2repl == %s", bus_to_replicate);

    rv = eqrb_rv_ok;

    do {
        prv = pthread_create(&h->h.tid_rx_thread, &attr, eqrb_server_rx_thread_call, h);
        if (prv == 0) {
            prv = pthread_create(&h->h.tid_tx_thread, &attr, eqrb_server_tx_thread_call, h);
            if (prv != 0) {
                rv = eqrb_os_based_err;
                break;
            }
        } else {
            rv = eqrb_os_based_err;
            break;
        }
        server_instance_num++;
    } while (0);

    if (rv != eqrb_rv_ok) {
        cmd_bus_deinit(h->h.cmd_bus_root_td);
    }

    return rv;
}

eqrb_rv_t eqrb_server_stop(eqrb_server_handle_t *h) {
    eqrb_dbg_msg("");
    return eqrb_service_stop(&h->h);
}
