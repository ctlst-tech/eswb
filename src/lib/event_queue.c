//
// Created by goofy on 12/25/21.
//

#include <string.h>
#include <stdlib.h>
#include "eswb/event_queue.h"

eswb_rv_t eswb_event_queue_order_topic(eswb_topic_descr_t td, const char *topics_path_mask, eswb_index_t subch_ind) {

    // TODO add topic by it's TD

    eswb_ctl_evq_order_t order;
    order.subch_ind = subch_ind;
    strncpy(order.path_mask_2order, topics_path_mask, ESWB_TOPIC_MAX_PATH_LEN);

    return eswb_ctl(td, eswb_ctl_request_topics_to_evq, &order, sizeof(order));
}

eswb_rv_t eswb_event_queue_enable(eswb_topic_descr_t td, eswb_size_t queue_size, eswb_size_t buffer_size) {
    eswb_size_t params[2] = {queue_size, buffer_size};
    return eswb_ctl(td, eswb_ctl_enable_event_queue, &params, sizeof(params));
}

eswb_rv_t eswb_event_queue_subscribe(const char *bus_path, eswb_topic_descr_t *td) {
    char path[ESWB_TOPIC_MAX_PATH_LEN + 1];

#define EVQ_PATH (BUS_EVENT_QUEUE_NAME "/event")

    strncpy(path, bus_path, ESWB_TOPIC_MAX_PATH_LEN - 1);
    strcat(path, "/");
    strncat(path, EVQ_PATH, ESWB_TOPIC_MAX_PATH_LEN - strlen(path));

    eswb_rv_t rv = eswb_fifo_subscribe(path, td);
    if (rv == eswb_e_no_topic) {
        rv = eswb_e_ev_queue_not_enabled;
    }
    if (rv == eswb_e_not_fifo) {
        rv = eswb_e_not_evq;
    }
    return rv;
}

eswb_rv_t eswb_event_queue_set_receive_mask(eswb_topic_descr_t td, eswb_event_queue_mask_t mask) {
    return eswb_ctl(td, eswb_ctl_evq_set_receive_mask, &mask, 0);
}


eswb_rv_t eswb_event_queue_pop(eswb_topic_descr_t td, event_queue_transfer_t *event) {
    eswb_rv_t rv = eswb_fifo_pop(td, event);

    return rv == eswb_e_not_fifo ? eswb_e_not_evq : rv;
}

/*
 * Problems:
 * 1. hash table to map topic_id --> replicated_topic_id
 * 2. post by topic ID or open topic, get _td_ and then do mapping topic_id --> replicated_td
 *    -
 */

/*
 * current assumptions:
 * - all the proclaiming goes to root node
 */

#include "ids_map.h"

eswb_rv_t map_alloc(topic_id_map_t *map_handle, eswb_size_t map_max_size) {

    memset(map_handle, 0, sizeof(*map_handle));

    map_handle->size = map_max_size;
    map_handle->map = calloc(map_max_size, sizeof(*map_handle->map));

    if (map_handle->map == NULL) {
        return eswb_e_map_no_mem;
    }

    return eswb_e_ok;
}

void map_dealloc(topic_id_map_t *map_handle) {
    free(map_handle->map);
}


eswb_rv_t map_find_index(topic_id_map_t *map_handle, uint32_t src_id_key, uint32_t *mid_rv) {

    int32_t l = 0;
    int32_t r = map_handle->records_num - 1;
    int32_t mid = r;
    eswb_rv_t rv = eswb_e_map_no_match;

    while (r >= l) {
        mid = l + ((r - l) >> 1);

        map_record_t *mr = &map_handle->map[mid];
        if (mr->src_topic_id == src_id_key) {
            rv = eswb_e_ok;
            break;
        } else if (mr->src_topic_id > src_id_key) {
            r = mid - 1;
        } else {
            l = mid + 1;
        }
    }

    if (mid_rv != NULL) {
        *mid_rv = mid;
    }

    return rv;
}

static void init_record(map_record_t *r, uint32_t src_id, eswb_topic_descr_t dst_td){
    r->src_topic_id = src_id;
    r->dst_td = dst_td;
}

static void add_pair(topic_id_map_t *map_handle, uint32_t src_id, eswb_topic_descr_t dst_td) {
    int32_t i = map_handle->records_num;
    while ((i > 0) && (map_handle->map[i-1].src_topic_id > src_id)) {
        map_handle->map[i] = map_handle->map[i-1];
        i--;
    }

    init_record(&map_handle->map[i], src_id, dst_td);

    map_handle->records_num++;
}


eswb_rv_t map_add_pair(topic_id_map_t *map_handle, uint32_t src_id, eswb_topic_descr_t dst_td) {
    if (map_handle->records_num >= map_handle->size) {
        return eswb_e_map_full;
    }

    eswb_rv_t rv = map_find_index(map_handle, src_id, NULL);
    if (rv == eswb_e_ok) {
        return eswb_e_map_key_exists;
    }

    add_pair(map_handle, src_id, dst_td);

    return eswb_e_ok;
}

eswb_rv_t map_find(topic_id_map_t *map_handle, uint32_t src_id_key, eswb_topic_descr_t *dst_td) {

    uint32_t i;
    eswb_rv_t rv = map_find_index(map_handle, src_id_key, &i);
    if (rv != eswb_e_ok) {
        return rv;
    }

    if (dst_td != NULL) {
        *dst_td = map_handle->map[i].dst_td;
    }

    return eswb_e_ok;
}

/*
 * 1. proclaim on low level by parent td
 * 2. get new tds back to register in map (how?)
 * !!! or associate with just TDs not topic_ids?
 */


//#include "eswb/topic_proclaiming_tree.h"

/*
 * conventions:
 *  - proclaiming data is streamed by separated as-initially-proclaimed tree
 *  - source topic id is identified by root topic, e.i. first one (as proclaimed)
 */

eswb_rv_t eswb_event_queue_replicate(eswb_topic_descr_t mount_point_td, struct topic_id_map *map_handle, event_queue_transfer_t *event) {

    eswb_rv_t rv = eswb_e_ok; // TODO no_effect?

    eswb_rv_t lookup_rv;

    switch (event->type) {
        case eqr_topic_proclaim:
            if ((event->size % sizeof(topic_proclaiming_tree_t))) {
                rv = eswb_e_repl_prtree_fraiming_error;
                break;
            }

            topic_proclaiming_tree_t *root = (topic_proclaiming_tree_t *)event->data;

            lookup_rv = map_find(map_handle, root->topic_id, NULL);
            if (lookup_rv == eswb_e_ok) {
                rv = eswb_e_topic_exist;
                break;
            }

            eswb_topic_descr_t proclaiming_td;
            if (event->topic_id != 0) {
                lookup_rv = map_find(map_handle, event->topic_id, &proclaiming_td);
                if (lookup_rv != eswb_e_ok) {
                    // TODO what you gonna do?
                    rv = lookup_rv;
                    break;
                }
            } else {
                proclaiming_td = mount_point_td;
            }

            // root->topic_id will be overwritten in a new local proclaim.
            // TODO make proclaim struct readonly? How to readback id then?
            uint32_t src_topic_id = root->topic_id;

            eswb_topic_descr_t new_td;

            // TODO Security issue: must check indexes before proclaiming it the whole thing.
            rv = eswb_proclaim_tree(proclaiming_td, root,
                                            event->size / sizeof(topic_proclaiming_tree_t), &new_td);

            if (rv == eswb_e_ok) {
                // TODO repetative lookup here?
                rv = map_add_pair(map_handle, src_topic_id, new_td);
            }

            break;

        case eqr_topic_update:
            ;
            eswb_topic_descr_t td;
            lookup_rv = map_find(map_handle, event->topic_id, &td);
            if (lookup_rv == eswb_e_ok) {
                rv = eswb_update_topic(td, event->data);
            } else {
                rv = lookup_rv;
            }

            break;

        case eqr_fifo_push: // TODO deprecate at all?
            ;
            lookup_rv = map_find(map_handle, event->topic_id, &td);
            if (lookup_rv == eswb_e_ok) {
                // TODO lame convention regarding the writing td (fifo or struct inside) likely will raise here:
                rv = eswb_fifo_push(td, event->data);
            } else {
                rv = lookup_rv;
            }

            break;

        default:
            rv = eswb_e_invargs;
    }


    return rv;
}
