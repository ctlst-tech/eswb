#include "conv.hpp"

#include <cstring>
#include <fstream>

char *eswb::ConverterToCsv::findSep(char *str, char *end_ptr, const char *sep) {
    char *rv;
    int length;
    while (1) {
        if (str >= end_ptr) {
            return nullptr;
        }
        rv = strstr(str, sep);
        length = strnlen(str, end_ptr - str);
        length = (length < 1 ? 1 : length);
        if (rv == nullptr) {
            str += length;
        } else {
            return rv;
        }
    }
}

int eswb::ConverterToCsv::convert(void) {
    int rv;
    std::ifstream raw_log;
    std::ofstream csv_log;
    std::string raw_string;

    raw_log.open(this->m_path_to_raw, std::ios::in);
    csv_log.open(this->m_path_to_csv, std::ios::out | std::ios::trunc);

    if (!raw_log.is_open()) {
        std::cout << "Failed to open raw log: " << this->m_path_to_raw
                  << std::endl;
        return -1;
    }

    if (!csv_log.is_open()) {
        std::cout << "Failed to create csv log: " << this->m_path_to_csv
                  << std::endl;
        return -1;
    }

    std::getline(raw_log, this->m_bus);
    std::cout << "Bus: " << this->m_bus << std::endl;

    raw_log.seekg(0, std::ios::end);
    size_t raw_length = raw_log.tellg();
    raw_log.seekg(0, std::ios::beg);
    char *raw_buf = new char[raw_length];

    if (raw_buf == nullptr) {
        std::cout << "Failed to create raw buf: " << std::endl;
    }

    raw_log.read(raw_buf, raw_length);
    char *raw_end_ptr = raw_buf + raw_length;

    bool loop = true;
    char *topic_start = raw_buf;
    char *topic_end = raw_buf;
    int topic_length;

    eswb::eqrb_interaction_header_t *h;
    eswb::event_queue_transfer_t *event;
    char timestamp[32];
    char *data;

    while (1) {
        topic_start = findSep(topic_start, raw_end_ptr, m_frame_sep.data());
        if (topic_start == nullptr) {
            break;
        }
        topic_end = topic_start + m_frame_sep.length();
        topic_end = findSep(topic_end, raw_end_ptr, m_frame_sep.data());
        if (topic_end == nullptr) {
            topic_end = raw_end_ptr;
        }
        topic_length = topic_end - topic_start;
        topic_start = topic_start + m_frame_sep.length();
        sscanf(topic_start, "%[0-9.]", timestamp);
        topic_start += strlen(timestamp);

        h = (eswb::eqrb_interaction_header_t *)topic_start;
        event = (eswb::event_queue_transfer_t
                     *)(topic_start + sizeof(eswb::eqrb_interaction_header_t));
        data = (char *)event + sizeof(eswb::event_queue_transfer_t);
        std::cout << "msg_code: " << (uint32_t)h->msg_code << std::endl; 
        std::cout << "size:     " << event->size << std::endl; 
        std::cout << "topic_id: " << event->topic_id << std::endl; 
        std::cout << "type:     " << (uint32_t)event->type << std::endl;
        std::cout << "data:     " << (char *)data << std::endl;
        std::cout << std::endl; 

        if (h->msg_code == eswb::EQRB_CMD_SERVER_TOPIC) {
        }
    }

    raw_log.close();
    csv_log.close();
    delete[] raw_buf;

    return rv;
}
