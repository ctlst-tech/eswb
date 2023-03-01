#ifndef CONV_HPP
#define CONV_HPP

#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "eswb.hpp"
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

namespace eswb {

// TODO: use headers with definitions
typedef struct __attribute__((packed)) eqrb_interaction_header {
    uint8_t msg_code;
    uint8_t reserved[3];
} eqrb_interaction_header_t;

typedef struct __attribute__((packed)) event_queue_transfer {
    uint32_t size;
    uint32_t topic_id;
    uint8_t type;
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

class ConverterToCsv {
private:
    const std::string m_path_to_raw;
    const std::string m_path_to_csv;
    const std::string m_frame_sep;

private:
    std::string m_bus;

private:
    std::vector<std::string> m_header;
    std::vector<std::string> m_raw;
    std::unordered_map<uint16_t, topic_proclaiming_tree_t> m_topic;
    std::unordered_map<uint16_t, uint16_t> m_topic_column_num;
    uint16_t m_column_num;

private:
    enum state {
        INIT = 0,
        UPDATE = 1,
    } m_state;

public:
    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv)
        : m_path_to_csv(path_to_csv),
          m_path_to_raw(path_to_raw),
          m_frame_sep("ebdf"),
          m_column_num(1),
          m_state(INIT) {}

    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv, const std::string &frame_sep)
        : m_path_to_csv(path_to_csv),
          m_path_to_raw(path_to_raw),
          m_frame_sep(frame_sep),
          m_column_num(1),
          m_state(INIT) {}

public:
    int convert(void);

private:
    char *findSep(char *str, char *end_ptr, const char *sep);

private:
    void append(std::ofstream &log, const char *data, std::streamsize size);
    void new_line(std::ofstream &log);
};

}  // namespace eswb

#endif  // CONV_HPP
