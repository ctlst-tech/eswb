//
// Created by goofy on 12/25/21.
//

#include <string.h>

#include "local_buses.h"
#include "topic_io.h"
#include "topic_mem.h"

#include "registry.h"


eswb_rv_t topic_io_read(topic_t *t, void *data, int synced) {

    if (synced) sync_take(t->sync);
    eswb_rv_t rv = topic_mem_simply_copy(t, data);
    if (synced) sync_give(t->sync);

    return rv;
}

eswb_rv_t topic_io_get_update (topic_t *t, void *data, int synced) {
    eswb_rv_t rv;

    if (synced) {

        sync_take(t->sync);

        do {
            rv = sync_wait(t->sync);
            if (rv != eswb_e_ok) {
                break;
            }
            rv = topic_mem_simply_copy(t, data);
        } while (0);

        sync_give(t->sync);
    } else {
        return topic_io_read(t, data, 0);
    };

    return rv;
}

static int32_t fifo_index_delta(eswb_fifo_index_t i1, eswb_fifo_index_t i2, eswb_fifo_index_t overflow_th) {
    int32_t rv = i1 - i2;
    if (rv < 0) {
        rv += overflow_th;
    }

    return rv;
}

static eswb_rv_t fifo_wait_and_read(topic_t *t, fifo_rcvr_state_t *rcvr_state, void *data, int synced) {
    eswb_rv_t rv = eswb_e_ok;

    do {
        int32_t dlap = fifo_index_delta(t->fifo_ext->state.lap_num,  rcvr_state->lap, ESWB_FIFO_INDEX_OVERFLOW);
        int32_t dind = t->fifo_ext->state.head - rcvr_state->tail;

        if ( (dlap > 1) || ((dlap == 1) && (dind >= 0)) ) {
            rv = eswb_e_fifo_rcvr_underrun;
            rcvr_state->lap = t->fifo_ext->state.lap_num-1;
            rcvr_state->tail = t->fifo_ext->state.head;
        } else if (t->fifo_ext->state.head == rcvr_state->tail) {
            if (synced) {
                rv = sync_wait(t->sync);
                if (rv != eswb_e_ok) {
                    break;
                }
            } else {
               rv = eswb_e_no_update;
               break;
            }
        }

        topic_mem_read_fifo(t, rcvr_state->tail, data);

        rcvr_state->tail++;
        if (rcvr_state->tail >= t->fifo_ext->fifo_size) {
            rcvr_state->tail = 0;
            rcvr_state->lap++;
        }


    } while (0);

    return rv;
}


eswb_rv_t topic_io_fifo_pop (topic_t *t, fifo_rcvr_state_t *rcvr_state, void *data, int synced) {

    if (synced) sync_take(t->sync);

    eswb_rv_t rv = fifo_wait_and_read(t, rcvr_state, data, synced);

    if (synced) sync_give(t->sync);

    return rv;
}


eswb_rv_t topic_io_event_queue_pop (topic_t *t, eswb_event_queue_mask_t mask, fifo_rcvr_state_t *rcvr_state, event_queue_transfer_t *eqt, int synced) {
    eswb_rv_t rv;

    event_queue_record_t event;

    if (synced) sync_take(t->sync);

    do {
        rv = fifo_wait_and_read(t, rcvr_state, &event, synced);
        if ((rv == eswb_e_ok) || (rv == eswb_e_fifo_rcvr_underrun) && (event.type == eqr_none)) {
            continue;
        }
    } while(synced && (!(event.ch_mask & mask))); // && (rv != eswb_e_no_update)

    if ((rv == eswb_e_ok) || (rv == eswb_e_fifo_rcvr_underrun)) {
        eqt->size = event.size;
        eqt->topic_id = event.topic_id;
        eqt->type = event.type;
        rv = topic_mem_event_queue_get_data(t, &event, eqt->data);
    }

    if (synced) sync_give(t->sync);

    return rv;
}

eswb_rv_t topic_io_do_update(topic_t *t, eswb_update_t ut, void *data, int synced) {

    if (synced) sync_take(t->sync);

    eswb_rv_t  rv;
    switch (ut) {
        case upd_proclaim_topic:
            rv = reg_tree_register(t->reg_ref, t, (topic_proclaiming_tree_t *) data, synced);
            break;

        case upd_update_topic:
            rv = topic_mem_write(t, data);
            break;

        case upd_push_fifo:
            rv = topic_mem_write_fifo(t, data);
            break;

        case upd_push_event_queue:
            rv = topic_mem_event_queue_write(t, (event_queue_record_t *) data);
            break;

        default:
        case upd_withdraw_topic:
            rv = eswb_e_not_supported;
            break;
    }

    if (synced) {
        if (rv == eswb_e_ok) {
            sync_broadcast(t->sync);
        }
        sync_give(t->sync);
    }

    return rv;
}


eswb_rv_t topic_io_get_state (topic_t *t, topic_fifo_state_t *state, int synced) {

    eswb_rv_t rv = eswb_e_ok;

    if (synced) sync_take(t->sync);

    if (t->fifo_ext == NULL) {
        rv = eswb_e_not_fifo;
    } else {
        memcpy(state, &t->fifo_ext->state, sizeof(topic_fifo_state_t));
    }

    if (synced) sync_give(t->sync);

    return rv;
}
