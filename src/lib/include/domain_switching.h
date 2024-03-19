#ifndef ESWB_DOMAIN_SWITCHING_H
#define ESWB_DOMAIN_SWITCHING_H

#include "eswb/errors.h"
#include "eswb/types.h"

eswb_rv_t ds_create(const char *bus_name, eswb_type_t type, eswb_size_t max_topics);
eswb_rv_t ds_delete(const char *bus_path);

eswb_rv_t ds_connect(const char *connection_point, eswb_topic_descr_t *td);
eswb_rv_t ds_disconnect(eswb_topic_descr_t td);

eswb_rv_t ds_update(eswb_topic_descr_t td, eswb_update_t ut, void *data, array_alter_t *array_params);
eswb_rv_t ds_read (eswb_topic_descr_t td, void *data);
eswb_rv_t ds_get_update (eswb_topic_descr_t td, void *data);
eswb_rv_t ds_try_get_update(eswb_topic_descr_t td, void *data);

eswb_rv_t ds_vector_read(eswb_topic_descr_t td, void *data, eswb_index_t pos, eswb_index_t num, eswb_index_t *num_rv, int do_wait,
               int check_update);

eswb_rv_t ds_fifo_pop(eswb_topic_descr_t td, void *data, int do_wait);

eswb_rv_t ds_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size);


#endif //ESWB_DOMAIN_SWITCHING_H
