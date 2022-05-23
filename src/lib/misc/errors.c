//
// Created by goofy on 27.11.2020.
//

#include "eswb/errors.h"


const char *eswb_strerror(eswb_rv_t e) {

    switch (e) {
        case eswb_e_ok:                     return "OK";
        case eswb_e_invargs:                return "Invalid arguments";
        case eswb_e_invargs_path_too_long:  return "Path is too long";
        case eswb_e_timedout:               return "Call is timed out";
        case eswb_e_sync_init:              return "Sync init failed";
        case eswb_e_sync_take:              return "Sync take failed";
        case eswb_e_sync_inconsistent:      return "Sync inconsistent failed";
        case eswb_e_sync_give:              return "Sync give failed";
        case eswb_e_sync_wait:              return "Sync wait failed";
        case eswb_e_sync_broadcast:         return "Sync broadcast failed";
        case eswb_e_no_update:              return "No update";
        case eswb_e_mem_reg_na:             return "Registry allocation failed";
        case eswb_e_mem_topic_na:           return "Topic allocation failed";
        case eswb_e_mem_sync_na:            return "Sync allocation failed";

        case eswb_e_mem_data_na:            return "Topic's data allocation failed";
        case eswb_e_mem_static_exceeded:    return "eswb_e_mem_static_exceeded";
        case eswb_e_naming:                 return "Naming error";
        case eswb_e_no_topic:               return "No such topic";
        case eswb_e_topic_exist:            return "Topic exists";
        case eswb_e_topic_is_not_dir:       return "Topic is not a directory";
        case eswb_e_not_fifo:               return "Topic is not a FIFO";
        case eswb_e_not_evq:                return "Topic is not an event queue";
        case eswb_e_fifo_rcvr_underrun:     return "Fifo under run detected";
        case eswb_e_not_supported:          return "Call is not supported";
        case eswb_e_bus_not_exist:          return "Bus does not exists";
        case eswb_e_bus_exists:             return "Bus already exists";
        case eswb_e_max_busses_reached:     return "Maximum busses";
        case eswb_e_max_topic_desrcs:       return "Maximum topics descriptors";
        case eswb_e_ev_queue_payload_too_large: return "Event queue payload is too large";
        case eswb_e_ev_queue_not_enabled:   return "Event queue is not enabled";
        case eswb_e_repl_prtree_fraiming_error: return "Proclaiming tree has framing errors";
        case eswb_e_map_full:               return "Replication map is full";
        case eswb_e_map_key_exists:         return "Replication map key is alredy exists";
        case eswb_e_map_no_mem:             return "Replication map allocation failed";
        case eswb_e_map_no_match:           return "Replication map have no match";
        default:                            return "Unknown!";
    }
}
