#include <string.h>
#include <time.h>
#include "topic_mem.h"
#include "eswb/event_queue.h"

#include <stdio.h>


eswb_rv_t topic_mem_simply_copy(topic_t *t, void *data) {
    if (data != NULL) {
        memcpy(data, t->data, t->data_size);
    }
    return eswb_e_ok;
}


eswb_rv_t topic_mem_write(topic_t *t, void *data) {
    memcpy(t->data, data, t->data_size);
    return eswb_e_ok;
}


eswb_rv_t topic_mem_write_vector(topic_t *t, void *data, array_alter_t *params) {

    if (params->elem_index >= t->array_ext->len) {
        return eswb_e_vector_inv_index;
    }

    eswb_index_t end_index = params->elem_index + params->elems_num;

    if (end_index > t->array_ext->len) {
        return eswb_e_vector_len_exceeded;
    }

    memcpy(t->data + t->array_ext->elem_step * params->elem_index,
           data,
           t->array_ext->elem_step * params->elems_num);

    if (params->flags & ESWB_VECTOR_WRITE_OPT_FLAG_DEFINE_END) {
        t->array_ext->curr_len = end_index;
    }

    return eswb_e_ok;
}

eswb_rv_t topic_mem_read_vector(topic_t *t, void *data, eswb_index_t pos, eswb_index_t num, eswb_index_t *num_rv) {

    if (pos >= t->array_ext->curr_len) {
        return eswb_e_vector_inv_index;
    }

    eswb_index_t elems_to_read = ((pos + num) > t->array_ext->curr_len) ?
                                 (t->array_ext->curr_len - pos) :
                                 num;

    memcpy(data,
           t->data + t->array_ext->elem_step * pos,
           t->array_ext->elem_step * elems_to_read);

    *num_rv = elems_to_read;

    return eswb_e_ok;
}

eswb_rv_t topic_mem_write_fifo(topic_t *t, void *data) {

    memcpy(t->data + t->array_ext->elem_step * t->array_ext->state.head, data, t->array_ext->elem_size);

    t->array_ext->state.head++;
    if (t->array_ext->state.head >= t->array_ext->len) {
        t->array_ext->state.head = 0;
        t->array_ext->state.lap_num++;
    }

    // printf("%s | fifo after update l = %d h = %d)\n", __func__,
    //        t->array_ext->state.lap_num, t->array_ext->state.head);

    return eswb_e_ok;
}

void topic_mem_read_fifo(topic_t *t, eswb_index_t tail, void *data) {

    memcpy(data, t->data + t->array_ext->elem_step * tail, t->array_ext->elem_size);

}

eswb_rv_t topic_mem_get_params(topic_t *t, topic_params_t *params) {
    strncpy(params->name, t->name, ESWB_TOPIC_NAME_MAX_LEN);
    if (t->parent != NULL) {
        strncpy(params->parent_name, t->parent->name, ESWB_TOPIC_NAME_MAX_LEN);
    }

    params->type = t->type;
    params->size = t->data_size;

    return eswb_e_ok;
}

void *topic_fifo_access_by_index(topic_t *t, eswb_index_t index) {

    return t->data + t->array_ext->elem_step * (index % t->array_ext->len);

}

#   define MIN(a,b) (a) < (b) ? (a) : (b)

// TODO some non-alignment comes from reading from buffer using direct pointer instead of index
eswb_rv_t topic_read_byte_buffer(topic_t *t, void *src, void *dst, eswb_size_t size) {

    if ((src < t->data) || (src > (t->data + t->data_size))) {
        return eswb_e_invargs;
    }

    ssize_t tail = src - t->data;

    ssize_t btr = MIN(size, t->array_ext->len - tail);
    memcpy(dst, t->data + tail, btr);

    if (btr < size) {
        ssize_t offst = btr;
        memcpy(dst + offst, t->data /*from the beginning*/, size - btr);
    }

    return eswb_e_ok;
}

void *topic_write_byte_buffer(topic_t *t, void *data, eswb_size_t size) {

    void *data_origin = t->data + t->array_ext->state.head;

    size_t btw = MIN(size, t->array_ext->len - t->array_ext->state.head);
    memcpy(data_origin, data, btw);

    if (btw < size) {
        size_t offset = btw;
        btw = size - btw;
        memcpy(t->data, data + offset, btw);
        t->array_ext->state.head = btw; // circling back to the buffer beginning
        t->array_ext->state.lap_num++;
    } else {
        t->array_ext->state.head += btw;
    }

    return data_origin;
}

static topic_t *event_queue_get_buffer(topic_t *evq) {
    return evq->first_child->next_sibling; // hangs on slippery convention that buffer is the second member ...
}

eswb_rv_t topic_mem_event_queue_get_data(topic_t *evq, event_queue_record_t *event, void *data) {
    topic_t *buff = event_queue_get_buffer(evq);

    return topic_read_byte_buffer(buff, event->data, data, event->size);
}

#define YES (-1)

static inline int ptr_crosses_record(event_queue_record_t *r, void *buffer_origin, eswb_size_t buffer_size, void *ptr) {

    ssize_t left_border = r->data - buffer_origin;
    ssize_t right_border = left_border + r->size;
    ssize_t p = ptr - buffer_origin;

    if (right_border > buffer_size) {
        left_border -= buffer_size;
        right_border -= buffer_size;
    }

    return (p > left_border) && (p < right_border) ? YES : 0;
}

eswb_rv_t topic_mem_event_queue_write(topic_t *t, const event_queue_record_t *r) {

    topic_t *data_buf_topic = event_queue_get_buffer(t);

    if (r->size > data_buf_topic->data_size) {
        return eswb_e_ev_queue_payload_too_large;
    }

    event_queue_record_t rts = *r;
    // push data
    // reposition data associated with event record
    rts.data = topic_write_byte_buffer(data_buf_topic, r->data, r->size);

#   define CALC_INDEX(__i,__s) ((__i) > (__s) ? (__i) - (__s) : (__i))

    // push event
    static struct timespec prev_timestamp;
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    rts.timestamp.sec = (uint32_t)timestamp.tv_sec;
    rts.timestamp.usec = (uint32_t)(timestamp.tv_nsec / 1000);

    double tnow = timestamp.tv_sec + timestamp.tv_nsec / 1000000000.0;
    double tprev = prev_timestamp.tv_sec + prev_timestamp.tv_nsec / 1000000000.0;
    double dt = tnow - tprev;

    if (dt > 0.004) {
        printf("topic_mem_event_queue_write: t = %lf, dt = %lf\n", tnow, dt);
    }
    prev_timestamp = timestamp;

    topic_mem_write_fifo(t, &rts);
    void *buffer_tail = data_buf_topic->data + data_buf_topic->array_ext->state.head;
    uint32_t i = 0;

    // TODO move this check to the receiver's site
    event_queue_record_t *checking_record;
//    while ( (checking_record = (event_queue_record_t *) topic_fifo_access_by_index(t, i + t->array_ext->state.head))
//        // check data overlap with next elem
//        //&& (checking_record->data != NULL)
//        && (ptr_crosses_record(checking_record, data_buf_topic->data, data_buf_topic->data_size, buffer_tail) == YES)
//        && (i <= t->data_size )) {
//        // erase events in a queue if case of overlap
//        checking_record->type = eqr_none;
//        i++;
//    }

    for ( i = 0; i < t->data_size; i++) {
        checking_record = (event_queue_record_t *) topic_fifo_access_by_index(t, i + t->array_ext->state.head);
        if (ptr_crosses_record(checking_record, data_buf_topic->data,
                               data_buf_topic->data_size, buffer_tail) == YES) {
            checking_record->type = eqr_none;
            checking_record->ch_mask = 0;
        }
    }

    return eswb_e_ok;
}
