#ifndef ESWB_LOCAL_BUSES_H
#define ESWB_LOCAL_BUSES_H

#include "eswb/types.h"
#include "topic_mem.h"
#include "registry.h"



typedef enum local_bus_type_e {
    local_bus_t_synced,
    local_bus_t_nonsynced,
    local_bus_t_synced_or_nonsynced,
} local_bus_type_t;

typedef struct eswb_bus_handle {
    char name[ESWB_BUS_NAME_MAX_LEN + 1];

    registry_t *registry;
    local_bus_type_t local_type;

    eswb_topic_descr_t event_queue_publisher_td;
} eswb_bus_handle_t;

typedef struct {
    eswb_fifo_index_t tail;
    eswb_fifo_index_t lap;
} fifo_rcvr_state_t;

typedef struct {
    topic_t *t;
    eswb_bus_handle_t *bh;

    fifo_rcvr_state_t rcvr_state;

    eswb_update_counter_t update_counter;

    eswb_event_queue_mask_t event_queue_mask; // this mask is used by EQ call for check desired topics

    uint32_t timeout_us;
} topic_local_index_t;

#ifdef __cplusplus
extern "C" {
#endif

eswb_rv_t local_buses_init(int do_reset);

eswb_rv_t local_bus_alloc_topic_descr(eswb_bus_handle_t *bh, topic_t *t, eswb_topic_descr_t *td);
eswb_rv_t local_do_update(eswb_topic_descr_t td, eswb_update_t ut, void *data, eswb_size_t elem_num);
eswb_rv_t local_do_read(eswb_topic_descr_t td, void *data);
eswb_rv_t local_get_update(eswb_topic_descr_t td, void *data);
eswb_rv_t local_try_get_update(eswb_topic_descr_t td, void *data);

eswb_rv_t local_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size);

eswb_rv_t local_init_fifo_receiver(eswb_topic_descr_t td);
eswb_rv_t local_fifo_pop(eswb_topic_descr_t td, void *data, int do_wait);

eswb_rv_t local_get_params(eswb_topic_descr_t td, topic_params_t *params);
void local_busses_print_registry(eswb_bus_handle_t *bh);

eswb_rv_t local_bus_itb_create(const char *bus_name, eswb_size_t max_topics);
eswb_rv_t local_lookup_itb(const char *bus_name, eswb_bus_handle_t **b);

eswb_rv_t local_bus_nsb_create(const char *bus_name, eswb_size_t max_topics);
eswb_rv_t local_lookup_nsb(const char *bus_name, eswb_bus_handle_t **b);
eswb_rv_t local_lookup_any(const char *bus_name, eswb_bus_handle_t **b);

eswb_rv_t local_bus_delete(eswb_bus_handle_t *bh);

eswb_rv_t local_bus_connect(eswb_bus_handle_t *bh, const char *conn_pnt, eswb_topic_descr_t *td);

#ifdef __cplusplus
}
#endif


#endif //ESWB_LOCAL_BUSES_H
