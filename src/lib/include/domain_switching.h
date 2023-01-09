#ifndef ESWB_DOMAIN_SWITCHING_H
#define ESWB_DOMAIN_SWITCHING_H

#include "eswb/errors.h"
#include "eswb/types.h"

eswb_rv_t ds_create(const char *bus_name, eswb_type_t type, eswb_size_t max_topics);
eswb_rv_t ds_delete(const char *bus_path);

eswb_rv_t ds_connect(const char *connection_point, eswb_topic_descr_t *td);
eswb_rv_t ds_disconnect(eswb_topic_descr_t td);

eswb_rv_t ds_update(eswb_topic_descr_t td, eswb_update_t ut, void *data, eswb_size_t elem_num);
eswb_rv_t ds_read (eswb_topic_descr_t td, void *data);
eswb_rv_t ds_get_update (eswb_topic_descr_t td, void *data);
eswb_rv_t ds_try_get_update(eswb_topic_descr_t td, void *data);

eswb_rv_t ds_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size);

//TODO reuse ds_read instead?
eswb_rv_t ds_fifo_pop(eswb_topic_descr_t td, void *data, int do_wait);

#endif //ESWB_DOMAIN_SWITCHING_H
