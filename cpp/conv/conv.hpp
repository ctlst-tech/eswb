#ifndef CONV_HPP
#define CONV_HPP

#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>

#include "eswb.hpp"
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

namespace eswb {

// TODO: use headers with definitions
typedef struct  __attribute__((packed)) eqrb_interaction_header {
    uint8_t msg_code;
    uint8_t reserved[3];
} eqrb_interaction_header_t;

typedef struct  __attribute__((packed)) event_queue_transfer {
    uint32_t size;
    uint32_t topic_id;
    uint8_t  type;
 /* uint8_t  data[size]; */
} event_queue_transfer_t;

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
    std::vector<std::string> header;
    std::vector<std::string> raw;
    std::unordered_map<int, std::string> hash;

public:
    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv)
        : m_path_to_csv(path_to_csv), m_path_to_raw(path_to_raw), m_frame_sep("ebdf") {}

    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv,
                   const std::string &frame_sep)
        : m_path_to_csv(path_to_csv), m_path_to_raw(path_to_raw), m_frame_sep(frame_sep) {}

public:
    int convert(void);

private:
    char *findSep(char *str, char *end_ptr, const char *sep);
};

}  // namespace eswb

#endif  // CONV_HPP
