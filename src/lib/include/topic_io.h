//
// Created by goofy on 12/25/21.
//

#ifndef ESWB_TOPIC_IO_H
#define ESWB_TOPIC_IO_H

#include "eswb/types.h"
#include "eswb/errors.h"
#include "topic_mem.h"


eswb_rv_t topic_io_read(topic_t *t, void *data, int synced);
eswb_rv_t topic_io_get_update(topic_t *t, void *data, int synced, uint32_t timeout_us);
eswb_rv_t topic_io_fifo_pop(topic_t *t, fifo_rcvr_state_t *rcvr_state, void *data, int synced, int do_wait, uint32_t timeout_us);
eswb_rv_t topic_io_event_queue_pop (topic_t *t, eswb_event_queue_mask_t mask, fifo_rcvr_state_t *rcvr_state, event_queue_transfer_t *eqt, int synced);
eswb_rv_t topic_io_do_update(topic_t *t, eswb_update_t ut, void *data, int synced);
eswb_rv_t topic_io_get_state (topic_t *t, topic_fifo_state_t *state, int synced);

#endif //ESWB_TOPIC_IO_H
