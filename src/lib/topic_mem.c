#include <string.h>
#include "topic_mem.h"
#include "eswb/event_queue.h"


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

#include <stdio.h>

eswb_rv_t topic_mem_write_fifo(topic_t *t, void *data) {

    //printf ("%s: ")
    memcpy(t->data + t->fifo_ext->elem_step * t->fifo_ext->state.head, data, t->fifo_ext->elem_size);

    t->fifo_ext->state.head++;
    if (t->fifo_ext->state.head >= t->fifo_ext->fifo_size) {
        t->fifo_ext->state.head = 0;
        t->fifo_ext->state.lap_num++;
    }

    return eswb_e_ok;
}

void topic_mem_read_fifo(topic_t *t, eswb_index_t tail, void *data) {

    memcpy(data, t->data + t->fifo_ext->elem_step * tail, t->fifo_ext->elem_size);

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

    return t->data + t->fifo_ext->elem_step * (index % t->fifo_ext->fifo_size);

}

#   define MIN(a,b) (a) < (b) ? (a) : (b)

// TODO some non-alignment comes from reading from buffer using direct pointer instead of index
eswb_rv_t topic_read_byte_buffer(topic_t *t, void *src, void *dst, eswb_size_t size) {

    if ((src < t->data) || (src > (t->data + t->data_size))) {
        return eswb_e_invargs;
    }

    ssize_t tail = src - t->data;

    ssize_t btr = MIN(size, t->fifo_ext->fifo_size - tail);
    memcpy(dst, t->data + tail, btr);

    if (btr < size) {
        ssize_t offst = btr;
        memcpy(dst + offst, t->data /*from the beginning*/, size - btr);
    }

    return eswb_e_ok;
}

void *topic_write_byte_buffer(topic_t *t, void *data, eswb_size_t size) {

    void *data_origin = t->data + t->fifo_ext->state.head;

    size_t btw = MIN(size, t->fifo_ext->fifo_size - t->fifo_ext->state.head);
    memcpy(data_origin, data, btw);

    if (btw < size) {
        size_t offset = btw;
        btw = size - btw;
        memcpy(t->data, data + offset, btw);
        t->fifo_ext->state.head = btw; // circling back to the buffer beginning
        t->fifo_ext->state.lap_num++;
    } else {
        t->fifo_ext->state.head += btw;
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
    topic_mem_write_fifo(t, &rts);
    void *buffer_tail = data_buf_topic->data + data_buf_topic->fifo_ext->state.head;
    uint32_t i = 0;

    // TODO move this check to the receiver's site
    event_queue_record_t *checking_record;
//    while ( (checking_record = (event_queue_record_t *) topic_fifo_access_by_index(t, i + t->fifo_ext->state.head))
//        // check data overlap with next elem
//        //&& (checking_record->data != NULL)
//        && (ptr_crosses_record(checking_record, data_buf_topic->data, data_buf_topic->data_size, buffer_tail) == YES)
//        && (i <= t->data_size )) {
//        // erase events in a queue if case of overlap
//        checking_record->type = eqr_none;
//        i++;
//    }

    for ( i = 0; i < t->data_size; i++) {
        checking_record = (event_queue_record_t *) topic_fifo_access_by_index(t, i + t->fifo_ext->state.head);
        if (ptr_crosses_record(checking_record, data_buf_topic->data,
                               data_buf_topic->data_size, buffer_tail) == YES) {
            checking_record->type = eqr_none;
        }
    }

    return eswb_e_ok;
}
