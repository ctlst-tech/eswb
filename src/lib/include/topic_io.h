#ifndef ESWB_TOPIC_IO_H
#define ESWB_TOPIC_IO_H

#include "eswb/types.h"
#include "eswb/errors.h"
#include "topic_mem.h"


eswb_rv_t topic_io_read(topic_t *t, void *data);
eswb_rv_t topic_io_read_w_counter(topic_t *t, void *data, eswb_update_counter_t *update_counter);
eswb_rv_t topic_get_update_counter(topic_t *t, eswb_update_counter_t *update_counter);

eswb_rv_t topic_io_read_vector(topic_t *t, void *data, eswb_index_t pos, eswb_index_t num, eswb_index_t *num_rv, int do_wait,
                     eswb_update_counter_t *update_counter, uint32_t timeout_us);

eswb_rv_t topic_io_get_update(topic_t *t, void *data, uint32_t timeout_us);
eswb_rv_t topic_io_fifo_pop(topic_t *t, fifo_rcvr_state_t *rcvr_state, void *data, int do_wait, uint32_t timeout_us);
eswb_rv_t topic_io_fifo_flush(topic_t *t, fifo_rcvr_state_t *rcvr_state);
eswb_rv_t topic_io_event_queue_pop(topic_t *t, eswb_event_queue_mask_t mask, fifo_rcvr_state_t *rcvr_state,
                                   event_queue_transfer_t *eqt, uint32_t timeout_us);
eswb_rv_t topic_io_do_update(topic_t *t, eswb_update_t ut, void *data, array_alter_t *params);
eswb_rv_t topic_io_get_state(topic_t *t, topic_array_state_t *state);

#endif //ESWB_TOPIC_IO_H
