#ifndef ESWB_ERRORS_H
#define ESWB_ERRORS_H

/** @page errors
 *
 */

/** @file */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    eswb_e_ok = 0,
    eswb_e_invargs,
    eswb_e_notdir,
    eswb_e_path_too_long,
    eswb_e_timedout,
    eswb_e_sync_init,
    eswb_e_sync_take,
    eswb_e_sync_inconsistent,
    eswb_e_sync_give,
    eswb_e_sync_wait,
    eswb_e_sync_broadcast,
    eswb_e_no_update,
    eswb_e_mem_reg_na,
    eswb_e_mem_topic_na,
    eswb_e_mem_sync_na,
    eswb_e_mem_data_na,
    eswb_e_mem_static_exceeded,
    eswb_e_vector_inv_index,
    eswb_e_vector_len_exceeded,
    eswb_e_inv_naming,
    eswb_e_inv_bus_spec,
    eswb_e_no_topic,
    eswb_e_topic_exist,
    eswb_e_topic_is_not_dir,
    eswb_e_not_fifo,
    eswb_e_not_vector,
    eswb_e_not_evq,
    eswb_e_fifo_rcvr_underrun,
    eswb_e_not_supported,
    eswb_e_bus_not_exist,
    eswb_e_bus_exists,
    eswb_e_max_busses_reached,
    eswb_e_max_topic_desrcs,
    eswb_e_ev_queue_payload_too_large,
    eswb_e_ev_queue_not_enabled,
    eswb_e_repl_prtree_fraiming_error,
    eswb_e_map_full,
    eswb_e_map_key_exists,
    eswb_e_map_no_mem,
    eswb_e_map_no_match,
} eswb_rv_t;

const char *eswb_strerror(eswb_rv_t e);

#ifdef __cplusplus
}
#endif

#endif //ESWB_ERRORS_H
