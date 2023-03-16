#ifndef EQRB_HPP
#define EQRB_HPP

#include <time.h>

#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "eswb.hpp"
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

namespace eswb {

// TODO: use headers with definitions of eqrb structures
typedef struct __attribute__((packed)) eqrb_interaction_header {
    uint8_t msg_code;
    uint8_t reserved[3];
} eqrb_interaction_header_t;

typedef struct {
    uint32_t sec;
    uint32_t usec;
} event_queue_timestamp_t;

typedef struct __attribute__((packed)) event_queue_transfer {
    uint32_t size;
    uint32_t topic_id;
    uint8_t type;
    event_queue_timestamp_t timestamp;
    /* uint8_t  data[size]; */
} event_queue_transfer_t;

typedef struct __attribute__((packed)) {
    char name[PR_TREE_NAME_ALIGNED];
    int32_t abs_ind;
    topic_data_type_s_t type;
    uint16_t data_offset;
    uint16_t data_size;
    uint16_t flags;
    uint16_t topic_id;
    int16_t parent_ind;
    int16_t first_child_ind;
    int16_t next_sibling_ind;
} topic_proclaiming_tree_t;

typedef enum {
    EQRB_CMD_CLIENT_REQ_SYNC = 0,
    EQRB_CMD_CLIENT_REQ_STREAM = 1,
    EQRB_CMD_SERVER_EVENT = 2,
    EQRB_CMD_SERVER_TOPIC = 3,
} eqrb_cmd_code_t;

}  // namespace eswb

#endif  // CONV_HPP
