//
// Created by goofy on 12/25/21.
//

#ifndef ESWB_SYNC_H
#define ESWB_SYNC_H

#include <stdint.h>
#include "eswb/errors.h"

struct sync_handle;

eswb_rv_t sync_create(struct sync_handle **s);
eswb_rv_t sync_take(struct sync_handle *ps);
eswb_rv_t sync_give(struct sync_handle *ps);
eswb_rv_t sync_wait(struct sync_handle *ps);
eswb_rv_t sync_wait_timed(struct sync_handle *ps, uint32_t timeout_us);
eswb_rv_t sync_broadcast(struct sync_handle *ps);
eswb_rv_t sync_destroy(struct sync_handle *ps);
const char *sync_last_strerror(struct sync_handle *ps);

#endif //ESWB_SYNC_H
