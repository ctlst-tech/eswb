#include <string.h>

#include "eswb/errors.h"
#include "registry.h"
#include "local_buses.h"

#include "eswb/event_queue.h"

#include "topic_io.h"

#define ITB_TD_MAX 512

#define LOCAL_INDEX_INIT 1

static topic_local_index_t local_td_index[ITB_TD_MAX];
static int local_index_num = LOCAL_INDEX_INIT; // omit first for having no zero td-s

#include <pthread.h>
static pthread_mutex_t local_topic_index_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t local_buses_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCAL_BUSSES_MAX 16
static eswb_bus_handle_t local_buses[LOCAL_BUSSES_MAX];

int local_bus_is_inited(const eswb_bus_handle_t *b){
    return b->registry != NULL;
}

eswb_rv_t local_buses_init(int do_reset) {
    pthread_mutex_lock(&local_buses_mutex);

    if (do_reset) {
        // TODO sync protection
        for (int i = 0; i < LOCAL_BUSSES_MAX; i++) {
            if (local_bus_is_inited(&local_buses[i]))
                reg_destroy(local_buses[i].registry);
        }

        memset(local_buses, 0, sizeof(local_buses));
        // drop connections
        memset(local_td_index, 0, sizeof(local_td_index));
        local_index_num = LOCAL_INDEX_INIT;
    }

    pthread_mutex_unlock(&local_buses_mutex);

    return eswb_e_ok;
}


eswb_rv_t local_bus_lookup(const char *bus_name, local_bus_type_t type, eswb_bus_handle_t **b) {
    // TODO local bus sync protection and platform independability

    eswb_rv_t rv = eswb_e_bus_not_exist;
    pthread_mutex_lock(&local_buses_mutex);

    for (int i = 0; i < LOCAL_BUSSES_MAX; i++) {
        if (local_bus_is_inited(&local_buses[i])) {
            if (((type == synced_or_nonsynced) || (local_buses[i].local_type == type)) &&
                (strncmp(local_buses[i].name, bus_name, ESWB_BUS_NAME_MAX_LEN) == 0)) {
                if (b != NULL) {
                    *b = &local_buses[i];
                }
                rv = eswb_e_ok;
                break;
            }
        }
    }

    pthread_mutex_unlock(&local_buses_mutex);

    return rv;
}


eswb_rv_t local_lookup_itb(const char *bus_name, eswb_bus_handle_t **b) {
    return local_bus_lookup(bus_name, synced, b);
}

eswb_rv_t local_lookup_nsb(const char *bus_name, eswb_bus_handle_t **b) {
    return local_bus_lookup(bus_name, nonsynced, b);
}

eswb_rv_t local_lookup_any(const char *bus_name, eswb_bus_handle_t **b) {
    return local_bus_lookup(bus_name, synced_or_nonsynced, b);
}

static int bus_is_synced(eswb_bus_handle_t *bh) {
    return bh->local_type==synced ? -1 : 0;
}

#define TOPIC_IS_FIFO(__t) (((__t)->type == tt_fifo) || ((__t)->type == tt_event_queue))

eswb_rv_t local_bus_connect(eswb_bus_handle_t *bh, const char *conn_pnt, eswb_topic_descr_t *td) {
    eswb_topic_descr_t new_td;
    topic_t *t = reg_find_topic(bh->registry, conn_pnt, bus_is_synced(bh));
            //bh->drv->find_topic((registry_t *)bh->registry, conn_pnt, &t);
    if (t == NULL) {
        return eswb_e_no_topic;
    }

    if ((t->parent != NULL) &&
            TOPIC_IS_FIFO(t->parent)) {
        t = t->parent;
    }

    eswb_rv_t rv = local_bus_alloc_topic_descr(bh, t, &new_td);
    if (rv != eswb_e_ok) {
        return rv;
    }

    if (TOPIC_IS_FIFO(t)) {
        rv = local_init_fifo_receiver(new_td);
        if (rv != eswb_e_ok) {
            return rv;
        }
    }

    *td = new_td;

    return eswb_e_ok;
}

//#include <stdio.h>

const topic_t *local_bus_topics_list(eswb_topic_descr_t td) {
    topic_local_index_t *li = &local_td_index[td];
//    printf("%s\n", li->bh->registry->topics[0].name);

    return &li->bh->registry->topics[0];
}

int local_buses_num() {
    int rv = 0;
    pthread_mutex_lock(&local_buses_mutex);

    for (int i = 0; i < LOCAL_BUSSES_MAX; i++) {
        rv += local_bus_is_inited(&local_buses[i]) ? 1 : 0;
    }

    pthread_mutex_unlock(&local_buses_mutex);

    return rv;
}

eswb_rv_t local_bus_create(const char *bus_name, local_bus_type_t type, eswb_size_t max_topics) {

    // TODO platform independability
    eswb_rv_t rv;

    if (strlen(bus_name) >= ESWB_BUS_NAME_MAX_LEN) {
        return eswb_e_invargs;
    }
    if (local_bus_lookup(bus_name, type, NULL) == eswb_e_ok) {
        return eswb_e_bus_exists;
    }

    pthread_mutex_lock(&local_buses_mutex);
    do {
        eswb_bus_handle_t *new = NULL;

        for (int i = 0; i < LOCAL_BUSSES_MAX; i++) {
            if (!local_bus_is_inited(&local_buses[i])) {
                new = &local_buses[i];
                break;
            }
        }

        if (new == NULL) {
            rv = eswb_e_max_busses_reached;
            break;
        }
        strncpy(new->name, bus_name, ESWB_BUS_NAME_MAX_LEN);
        new->local_type = type;
        rv = reg_create(bus_name, &new->registry, max_topics, type == synced ? -1 : 0);
    } while(0);
    pthread_mutex_unlock(&local_buses_mutex);

    return rv;
}

eswb_rv_t local_bus_delete(eswb_bus_handle_t *bh) {

    pthread_mutex_lock(&local_topic_index_mutex);

    // making related TDs invalid
    for (int i = LOCAL_INDEX_INIT; i < local_index_num; i++) {
        if (local_td_index[i].bh == bh) {
            local_td_index[i].t = NULL;
            local_td_index[i].bh = NULL;
        }
    }

    pthread_mutex_unlock(&local_topic_index_mutex);

    pthread_mutex_lock(&local_buses_mutex);
    reg_destroy(bh->registry);
    memset(bh, 0, sizeof(*bh));
    pthread_mutex_unlock(&local_buses_mutex);

    return eswb_e_ok;
}

eswb_rv_t local_bus_itb_create(const char *bus_name, eswb_size_t max_topics) {
    return local_bus_create(bus_name, synced, max_topics);
}

eswb_rv_t local_bus_nsb_create(const char *bus_name, eswb_size_t max_topics) {
    return local_bus_create(bus_name, nonsynced, max_topics);
}

eswb_rv_t  local_bus_alloc_topic_descr(eswb_bus_handle_t *bh, topic_t *t, eswb_topic_descr_t *td) {
    eswb_rv_t rv;

    pthread_mutex_lock(&local_topic_index_mutex); // TODO portable sync

    do {

        if (local_index_num >= ITB_TD_MAX) {
            rv = eswb_e_max_topic_desrcs;
            break;
        }

        topic_local_index_t *li = &local_td_index[local_index_num];

        li->t = t;
        li->bh = bh;

        *td = local_index_num;
        local_index_num++;
        rv = eswb_e_ok;
    } while (0);

    pthread_mutex_unlock(&local_topic_index_mutex);

    return rv;
}


#include <stdio.h>

static void print_event(event_queue_record_t *e) {
    printf("Event: event_type = %d, id = %u, data_size = %u\n", e->type, e->topic_id, e->size );
}

/**
 *
 * @param td
 * @param ut
 * @param data
 * @param elem_num - number of elements of array stored in data
 * @return
 */

static event_queue_record_type_t event_type (eswb_update_t ut) {
    switch (ut) {
        case upd_proclaim_topic:        return eqr_topic_proclaim;
        case upd_update_topic:          return eqr_topic_update;
        case upd_push_fifo:             return eqr_fifo_push;
        case upd_withdraw_topic:
        case upd_push_event_queue:
        default:
                                        return eqr_none;
    }
}

eswb_rv_t local_event_queue_update(eswb_bus_handle_t *bh, event_queue_record_t *record) {
    topic_local_index_t *eq_li = &local_td_index[bh->event_queue_publisher_td];

    return topic_io_do_update(eq_li->t, upd_push_event_queue, record, bus_is_synced(bh));
}

static eswb_rv_t local_event_queue_pack_and_update(topic_local_index_t *li, eswb_update_t ut, void *data, eswb_size_t elem_num) {

    if (li->bh->event_queue_publisher_td == 0) {
        return eswb_e_no_topic;
    }

    event_queue_record_type_t et = event_type(ut);
    if (et == eqr_none) {
        return eswb_e_ok;
    }

    event_queue_record_t v = {
            .size = ut == upd_proclaim_topic ? sizeof(topic_proclaiming_tree_t) * elem_num : li->t->data_size,
            .topic_id = li->t->id,
            .ch_mask = li->t->evq_mask,
            .type = et,
            .data = data
    };

    if (ut == upd_proclaim_topic) {
        v.ch_mask &= 0x0000FFFF;   // any proclaiming go stream 1
        v.ch_mask |= (1 << 0); // any proclaiming go to channel 0
    }

    return local_event_queue_update(li->bh, &v);
}

eswb_rv_t local_do_update(eswb_topic_descr_t td, eswb_update_t ut, void *data, eswb_size_t elem_num) {
    topic_local_index_t *li = &local_td_index[td];
    eswb_rv_t rv = topic_io_do_update(li->t, ut, data, bus_is_synced(li->bh));

    if (rv == eswb_e_ok) {
        if (li->t->evq_mask) {
            local_event_queue_pack_and_update(li, ut, data, elem_num);
            // TODO handle rv
        }
    }

    return rv;
}

eswb_rv_t local_do_read(eswb_topic_descr_t td, void *data) {
    topic_local_index_t *li = &local_td_index[td];

    return topic_io_read(li->t, data, bus_is_synced(li->bh));
}

eswb_rv_t local_get_update(eswb_topic_descr_t td, void *data) {
    topic_local_index_t *li = &local_td_index[td];

    eswb_rv_t rv = topic_io_get_update(li->t, data, bus_is_synced(li->bh), li->timeout_us);
    li->timeout_us = 0;

    return rv;
}

eswb_rv_t local_get_params(eswb_topic_descr_t td, topic_params_t *params) {
    topic_local_index_t *li = &local_td_index[td];
    // don't need to be sync proteced, data is read only for registered topics

    return topic_mem_get_params(li->t, params);
}

eswb_rv_t local_arm_timeout(topic_local_index_t *li, uint32_t timeout_us) {
    li->timeout_us = timeout_us;
    return eswb_e_ok;
}

eswb_rv_t local_get_mounting_point(eswb_topic_descr_t td, char *mp) {
    topic_local_index_t *li = &local_td_index[td];
    // don't need to be sync proteced, data is read only for registered topics

    strncpy(mp, li->bh->local_type == synced ? "itb:" : "nsb:", ESWB_TOPIC_MAX_PATH_LEN);

    int depth = 0;
    for (topic_t *n = li->t; n != NULL; n = n->parent) {
        depth++;
    }

    for (int i = 0; i < depth; i++) {
        topic_t *t = li->t;
        for(int j = 0; j < depth - 1 - i; j++) {
            t = t->parent;
        }
        strncat(mp, "/", ESWB_TOPIC_MAX_PATH_LEN - strlen(mp));
        strncat(mp, t->name, ESWB_TOPIC_MAX_PATH_LEN - strlen(mp));
    }

    return eswb_e_ok;
}

eswb_rv_t local_fifo_pop(eswb_topic_descr_t td, void *data, int do_wait) {
    topic_local_index_t *li = &local_td_index[td];

    eswb_rv_t rv;

    switch(li->t->type) {
        case tt_event_queue:
            rv = topic_io_event_queue_pop(li->t, li->event_queue_mask, &li->rcvr_state, data, bus_is_synced(li->bh),
                                            li->timeout_us);
            break;

        case tt_fifo:
            rv = topic_io_fifo_pop(li->t, &li->rcvr_state, data,
                                             bus_is_synced(li->bh), do_wait, li->timeout_us);;
            break;

        default:
            return eswb_e_not_fifo;
    }

    li->timeout_us = 0;
    return rv;
}

eswb_rv_t local_fifo_flush(topic_local_index_t *li) {
    return topic_io_fifo_flush(li->t, &li->rcvr_state);
}

eswb_rv_t local_init_fifo_receiver(eswb_topic_descr_t td) {
    topic_local_index_t *li = &local_td_index[td];

    topic_fifo_state_t s;

    eswb_rv_t rv = topic_io_get_state(li->t, &s, bus_is_synced(li->bh));
    if (rv == eswb_e_ok) {
        li->rcvr_state.tail = s.head; // TODO maybe we need already accumulated but not overwritten data?
        li->rcvr_state.lap = s.lap_num;
    }

    return rv;
}

eswb_rv_t local_bus_create_event_queue(eswb_bus_handle_t *bh, eswb_size_t events_num, eswb_size_t data_buf_size) {
    eswb_topic_descr_t td;

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 3);
    topic_proclaiming_tree_t *r = usr_topic_set_root(cntx, BUS_EVENT_QUEUE_NAME, tt_event_queue, events_num);
    // fifo convention: it's element must be first child
    usr_topic_add_child(cntx, r, "event", tt_struct, 0, sizeof(event_queue_record_t), TOPIC_FLAG_MAPPED_TO_PARENT);
    usr_topic_add_child(cntx, r, "data_buf", tt_byte_buffer, 0, data_buf_size, TOPIC_FLAG_USES_PARENT_SYNC);


    eswb_rv_t rv = topic_io_do_update(&bh->registry->topics[0], upd_proclaim_topic, r, bus_is_synced(bh));

    if (rv != eswb_e_ok) {
        return rv;
    }

    // TODO This is lame, must be removed in overall connectivity refactoring
    char full_path[ESWB_TOPIC_NAME_MAX_LEN+1];
    strcpy(full_path, bh->name );
    strcat(full_path, "/");
    strcat(full_path, BUS_EVENT_QUEUE_NAME);

    rv = local_bus_connect(bh, full_path, &td);
    if (rv != eswb_e_ok) {
        return rv;
    }

    // TODO fix this bad pattern:
    pthread_mutex_lock(&local_buses_mutex); // TODO portable sync, rethink where to sync it? or whould we at all?
    bh->event_queue_publisher_td = td;
    pthread_mutex_unlock(&local_buses_mutex); // TODO portable sync

    return eswb_e_ok;
}


static eswb_rv_t flags_attach_lambda(void *d, topic_t *t) {
    // TODO it is not thread safe, must lock on appropriate registry lock level
    eswb_index_t ch_id = *((eswb_index_t *) d);

//    if (t->type == tt_event_queue) {
//        // event queue itself cannot be marked for event queue, lol
//        return eswb_e_invargs;
//    }

    // TODO subscribe on whole struct if topic mapped to parent_ind?
    //printf ("%s. %s to event queue\n", __func__, t->name);
    t->evq_mask |= 1 << ch_id;

    return eswb_e_ok;
}

eswb_rv_t local_bus_mark_for_event_queue(eswb_topic_descr_t td, char *path_mask, eswb_index_t ch_id) {
    topic_local_index_t *li = &local_td_index[td];

    if (ch_id > 31) {
        return eswb_e_invargs;
    }

    return topic_mem_walk_through(li->t, path_mask, &flags_attach_lambda, &ch_id);
}

eswb_rv_t local_bus_get_next_topic_info(topic_local_index_t *li, eswb_topic_id_t tid, topic_extract_t *info) {
    if (tid == 0) {
        tid = li->t->id;
    }
    return reg_get_next_topic_info(li->bh->registry, li->t, tid, info, bus_is_synced(li->bh));
}

eswb_rv_t local_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size) {
    topic_local_index_t *li = &local_td_index[td]; // TODO make all consequent calls use this struct insted of creating own
    eswb_bus_handle_t *bh = li->bh;

    switch (ctl_type) {
        case eswb_ctl_enable_event_queue:
            ;
            eswb_size_t *params;
            params = ((eswb_size_t *) d);
            return local_bus_create_event_queue(bh, params[0], params[1]);

        case eswb_ctl_request_topics_to_evq:
            ;
            eswb_ctl_evq_order_t *ord = d;
            return local_bus_mark_for_event_queue(td, ord->path_mask_2order, ord->subch_ind);

        case eswb_ctl_evq_set_receive_mask:
            ;
            eswb_event_queue_mask_t mask = *((eswb_event_queue_mask_t *) d);
            if (li->t->type != tt_event_queue) {
                return eswb_e_not_evq;
            }
            // TODO lock registry?
            li->event_queue_mask = mask;
            return eswb_e_ok;

        case eswb_ctl_evq_get_params:
            return local_get_params(td, (topic_params_t *)d);

        case eswb_ctl_get_topic_path:
            return local_get_mounting_point(td, (char *)d);

        case eswb_ctl_get_next_proclaiming_info:
            return local_bus_get_next_topic_info(li, *((eswb_topic_id_t *) d), (topic_extract_t *) d);

        case eswb_ctl_fifo_flush:
            return local_fifo_flush(li);

        case eswb_ctl_arm_timeout:
            return local_arm_timeout(li, *((uint32_t *)d));

        default:
            return eswb_e_not_supported;
    }
}

#include <stdio.h>
/**
 * not thread safe!
 * @param busname
 */
void local_busses_print_registry(eswb_bus_handle_t *bh) {
    reg_print(bh->registry);
}

