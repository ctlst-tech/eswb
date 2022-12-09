#include <string.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

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

eswb_rv_t topic_io_get_update(topic_t *t, void *data, int synced, uint32_t timeout_us) {
    eswb_rv_t rv;

    if (synced) {

        sync_take(t->sync);

        do {
            if (timeout_us) {
                rv = sync_wait_timed(t->sync, timeout_us);
            } else {
                rv = sync_wait(t->sync);
            }
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

static eswb_rv_t fifo_wait_and_read(topic_t *t, fifo_rcvr_state_t *rcvr_state, void *data, int synced, int do_wait,
                                    uint32_t timeout_us) {
    eswb_rv_t rv = eswb_e_ok;

    do {
        int32_t dlap = fifo_index_delta(t->fifo_ext->state.lap_num,  rcvr_state->lap, ESWB_FIFO_INDEX_OVERFLOW);
        int32_t dind = t->fifo_ext->state.head - rcvr_state->tail;

        if ( (dlap > 1) || ((dlap == 1) && (dind >= 0)) ) {
            rv = eswb_e_fifo_rcvr_underrun;
            rcvr_state->lap = t->fifo_ext->state.lap_num-1;
            rcvr_state->tail = t->fifo_ext->state.head;
            // printf("%s | %p Underrun (rcvr l = %d t = %d) (fifo l = %d t = %d)\n", __func__, rcvr_state,
            //        rcvr_state->lap, rcvr_state->tail,
            //        t->fifo_ext->state.lap_num, t->fifo_ext->state.head);

        } else if (t->fifo_ext->state.head == rcvr_state->tail) {
            if (do_wait && synced) {
                int wait_cnt = 0;
                do {
                    if (timeout_us > 0) {
                        rv = sync_wait_timed(t->sync, timeout_us);
                    } else {
                        rv = sync_wait(t->sync);
                    }
                    wait_cnt++;
                    // here we got an issue (under free rtos) when we've got a return from wait when there is no broadcast
                    // this cycle allowes fifos to be more robust
                    // printf("%s | %p %p sync_wait result = %s (cnt == %d)\n", __func__, rcvr_state, pthread_self(), eswb_strerror(rv), wait_cnt);
                } while((t->fifo_ext->state.head == rcvr_state->tail) && (rv == eswb_e_ok));

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
        // printf("%s | %p %p after pop result l = %d t = %d\n", __func__, rcvr_state, pthread_self(), rcvr_state->lap, rcvr_state->tail);

    } while (0);

    return rv;
}


static eswb_rv_t fifo_flush(topic_t *t, fifo_rcvr_state_t *rcvr_state) {

    if (t->fifo_ext == NULL) {
        return eswb_e_not_fifo;
    }

    rcvr_state->lap = t->fifo_ext->state.lap_num;
    rcvr_state->tail = t->fifo_ext->state.head;

    return eswb_e_ok;
}


eswb_rv_t
topic_io_fifo_pop(topic_t *t, fifo_rcvr_state_t *rcvr_state, void *data, int synced, int do_wait, uint32_t timeout_us) {
    if (synced) sync_take(t->sync);
    eswb_rv_t rv = fifo_wait_and_read(t, rcvr_state, data, synced, do_wait, timeout_us);
    if (synced) sync_give(t->sync);
    return rv;
}

eswb_rv_t topic_io_fifo_flush(topic_t *t, fifo_rcvr_state_t *rcvr_state, int synced) {
    if (synced) sync_take(t->sync);
    eswb_rv_t rv = fifo_flush(t, rcvr_state);
    if (synced) sync_give(t->sync);
    return rv;
}


eswb_rv_t topic_io_event_queue_pop(topic_t *t, eswb_event_queue_mask_t mask, fifo_rcvr_state_t *rcvr_state,
                                   event_queue_transfer_t *eqt, int synced, uint32_t timeout_us) {
    eswb_rv_t rv;

    event_queue_record_t event;

    struct timespec ts;
    struct timespec timeout_expiry_time;

    if (timeout_us) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        // TODO cover it by the test
        // TODO shift to micro sec call timeoftheday or something?

        timeout_expiry_time.tv_sec = ts.tv_sec + (timeout_us > 1000000 ? (timeout_us / 1000000) : 0);
        timeout_expiry_time.tv_nsec = ts.tv_nsec + ((timeout_us % 1000000) * 1000);
        if (timeout_expiry_time.tv_nsec > 1000000000) {
            timeout_expiry_time.tv_nsec %= 1000000000;
            timeout_expiry_time.tv_sec++;
        }
        // printf("Time now %d-%d time expire %d-%d\n",
               // (unsigned) ts.tv_sec, (unsigned) ts.tv_nsec / 1000000,
               // (unsigned) timeout_expiry_time.tv_sec, (unsigned) timeout_expiry_time.tv_nsec / 1000000);
    }

    eswb_rv_t arv;

    do {
        if (synced) sync_take(t->sync);
        rv = fifo_wait_and_read(t, rcvr_state, &event, synced, 1, timeout_us);
        if (synced) sync_give(t->sync);

        if (synced && timeout_us) {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            if ((ts.tv_sec >= timeout_expiry_time.tv_sec) && (ts.tv_nsec > timeout_expiry_time.tv_nsec)) {
                rv = eswb_e_timedout;
                // printf("Time now %d-%d time expire %d-%d\n",
                //        (unsigned) ts.tv_sec, (unsigned) ts.tv_nsec / 1000000,
                //        (unsigned) timeout_expiry_time.tv_sec, (unsigned) timeout_expiry_time.tv_nsec / 1000000);
            }
            // TODO recalc timeout in this iteration, optimize cycle (e.g. no need to calc timeout when we got a result)
        }
        arv = (rv == eswb_e_ok || rv == eswb_e_fifo_rcvr_underrun) ? eswb_e_ok : rv;
    } while(synced && (arv == eswb_e_ok &&
                    (((event.ch_mask & mask) == 0) || (event.type == eqr_none))));

    if (arv == eswb_e_ok) {
        eqt->size = event.size;
        eqt->topic_id = event.topic_id;
        eqt->type = event.type;
        rv = topic_mem_event_queue_get_data(t, &event, EVENT_QUEUE_TRANSFER_DATA(eqt));
    }

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
            // if (ut == upd_push_event_queue) {
            //     printf("%s upd_push_event_queue sync_broadcast\n", __func__);
            // }
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
