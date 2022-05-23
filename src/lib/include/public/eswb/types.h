#ifndef ESWB_TYPES_H
#define ESWB_TYPES_H

#include <stdint.h>

#define ESWB_FIFO_INDEX_OVERFLOW UINT16_MAX

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t eswb_index_t;
typedef uint16_t eswb_fifo_index_t;
typedef uint32_t eswb_size_t;
typedef uint32_t eswb_topic_id_t;
typedef uint32_t eswb_event_queue_mask_t;

typedef enum {
    tt_none = 0,
    tt_float,
    tt_double,
    tt_uint8,
    tt_int8,
    tt_uint16,
    tt_int16,
    tt_uint32,
    tt_int32,
    tt_uint64,
    tt_int64,
    tt_string,
    tt_struct,
    tt_fifo,
    tt_byte_buffer,
    tt_dir,
    tt_event_queue
} topic_data_type_t;

typedef enum {
    upd_proclaim_topic,
    upd_update_topic,
    upd_push_fifo,
    upd_push_event_queue,
    upd_withdraw_topic
} eswb_update_t;


typedef enum {
    eswb_inter_thread,
    eswb_inter_process,
    eswb_local_non_synced,
} eswb_type_t;

typedef enum {
    eswb_ctl_enable_event_queue,
    eswb_ctl_request_topics_to_evq,
    eswb_ctl_evq_set_receive_mask,
    eswb_ctl_evq_get_params,
    eswb_ctl_get_topic_path,
    eswb_ctl_get_next_proclaiming_info
} eswb_ctl_t;


#define ESWB_BUS_NAME_MAX_LEN 32
#define ESWB_TOPIC_NAME_MAX_LEN 30
#define ESWB_TOPIC_MAX_PATH_LEN 300


typedef int eswb_topic_descr_t;

typedef struct {
    char name[ESWB_TOPIC_NAME_MAX_LEN + 1];
    char parent_name[ESWB_TOPIC_NAME_MAX_LEN + 1];
    topic_data_type_t type;
    eswb_size_t size;
} topic_params_t;

const char *eswb_type_name(topic_data_type_t t);

#ifdef __cplusplus
}
#endif

#endif //ESWB_TYPES_H
