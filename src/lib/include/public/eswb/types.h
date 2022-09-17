#ifndef ESWB_TYPES_H
#define ESWB_TYPES_H

/** @page proclaiming
 * Check types.h for calls descriptions
 */


/** @file */

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
    tt_none         = 0x00,

    tt_dir          = 0x01,

    tt_struct       = 0x02,
    tt_fifo         = 0x03,

    tt_uint8        = 0x10,
    tt_int8         = 0x11,
    tt_uint16       = 0x12,
    tt_int16        = 0x13,
    tt_uint32       = 0x14,
    tt_int32        = 0x15,
    tt_uint64       = 0x16,
    tt_int64        = 0x17,

    tt_float        = 0x20,
    tt_double       = 0x21,

    tt_string       = 0x30,
    tt_plain_data   = 0x31,

    tt_byte_buffer  = 0x40,

    tt_event_queue  = 0xF0
} topic_data_type_t;

typedef uint8_t topic_data_type_s_t;

typedef enum {
    upd_proclaim_topic,
    upd_update_topic,
    upd_push_fifo,
    upd_push_event_queue,
    upd_withdraw_topic
} eswb_update_t;


typedef enum {
    eswb_not_defined,
    eswb_non_synced,
    eswb_inter_thread,
    eswb_inter_process,
} eswb_type_t;

typedef enum {
    eswb_ctl_enable_event_queue,
    eswb_ctl_request_topics_to_evq,
    eswb_ctl_evq_set_receive_mask,
    eswb_ctl_evq_get_params,
    eswb_ctl_get_topic_path,
    eswb_ctl_get_next_proclaiming_info,
    eswb_ctl_fifo_flush,
    eswb_ctl_arm_timeout
} eswb_ctl_t;


#define ESWB_BUS_NAME_MAX_LEN 32
#define ESWB_TOPIC_NAME_MAX_LEN 30
#define ESWB_TOPIC_MAX_PATH_LEN 100


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
