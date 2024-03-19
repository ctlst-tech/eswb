#ifndef ESWB_EVENT_QUEUE_H
#define ESWB_EVENT_QUEUE_H

/** @page event_queue
 * Check event_queue.h for calls descriptions
 */

/** @file */

#include "eswb/api.h"
#include "types.h"

typedef enum {
    eqr_none = 0,
    eqr_topic_proclaim = 1,
    eqr_topic_update = 2,
    eqr_fifo_push = 3
} event_queue_record_type_t;

typedef uint8_t event_queue_record_type_s_t;
typedef struct {
    uint32_t sec;
    uint32_t usec;
} event_queue_timestamp_t;

typedef struct {
    eswb_size_t size;
    eswb_topic_id_t topic_id;
    eswb_event_queue_mask_t ch_mask;
    event_queue_record_type_s_t type;
    event_queue_timestamp_t timestamp;
    void *data;
} event_queue_record_t;

typedef struct  __attribute__((packed)) event_queue_transfer {
    uint32_t size;
    uint32_t topic_id;
    uint8_t  type;
    event_queue_timestamp_t timestamp;
 /* uint8_t  data[size]; */
} event_queue_transfer_t;

#define EVENT_QUEUE_TRANSFER_DATA(__etp) ((uint8_t*) (((uint8_t*)(__etp)) + sizeof(event_queue_transfer_t)))

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
