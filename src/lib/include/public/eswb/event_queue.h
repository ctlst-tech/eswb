#ifndef ESWB_EVENT_QUEUE_H
#define ESWB_EVENT_QUEUE_H

#include "eswb/api.h"
#include "types.h"

typedef enum {
    eqr_none = 0,
    eqr_topic_proclaim,
    eqr_topic_update,
    eqr_fifo_push
} event_queue_record_type_t;

typedef struct {
    eswb_size_t size;
    eswb_topic_id_t topic_id;
    eswb_event_queue_mask_t ch_mask;
    event_queue_record_type_t type;
    void *data;
} event_queue_record_t;

typedef struct  __attribute__((packed)) event_queue_transfer {
    uint32_t size;
    uint32_t topic_id;
    event_queue_record_type_t type;
    uint8_t  data[0];
} event_queue_transfer_t;

typedef struct {
    eswb_index_t subch_ind;
    char path_mask_2order[ESWB_TOPIC_MAX_PATH_LEN + 1];
} eswb_ctl_evq_order_t;

#define BUS_EVENT_QUEUE_NAME ".bus_events_queue"

#ifdef __cplusplus
extern "C" {
#endif

eswb_rv_t eswb_event_queue_enable(eswb_topic_descr_t td, eswb_size_t queue_size, eswb_size_t buffer_size);
eswb_rv_t eswb_event_queue_order_topic(eswb_topic_descr_t td, const char *topics_path_mask, eswb_index_t subch_ind);

eswb_rv_t eswb_event_queue_set_receive_mask(eswb_topic_descr_t td, eswb_event_queue_mask_t mask);
eswb_rv_t eswb_event_queue_subscribe(const char *bus_path, eswb_topic_descr_t *td);


eswb_rv_t eswb_event_queue_pop(eswb_topic_descr_t td, event_queue_transfer_t *event);

struct topic_id_map;

eswb_rv_t eswb_event_queue_replicate(eswb_topic_descr_t mount_point_td, struct topic_id_map *map_handle, event_queue_transfer_t *event);

#ifdef __cplusplus
}
#endif

#endif //ESWB_EVENT_QUEUE_H
