#include "conv.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>

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

void eswb::ConverterToCsv::append(std::ofstream &log, const char *data,
                                  std::streamsize size) {
    log.write(data, size);
    log.write(",", 1);
}

void eswb::ConverterToCsv::new_line(std::ofstream &log) {
    log.write("\n", 1);
}

int eswb::ConverterToCsv::convert(void) {
    int rv;
    std::ifstream raw_log;
    std::ofstream csv_log;
    std::string raw_string;

    raw_log.open(this->m_path_to_raw, std::ios::in);
    csv_log.open(this->m_path_to_csv,
                 std::ios::out | std::ios::trunc | std::ios::binary);

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
    eswb::topic_proclaiming_tree_t *topic;
    char timestamp[32];
    char *data;
    m_header.push_back("Time");
    m_raw.push_back("0.0");
    append(csv_log, m_header[0].data(), m_header[0].length());

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
        m_raw[0] = timestamp;
        topic_start += strlen(timestamp);

        h = (eswb::eqrb_interaction_header_t *)topic_start;
        event = (eswb::event_queue_transfer_t
                     *)(topic_start + sizeof(eswb::eqrb_interaction_header_t));
        data = (char *)event + sizeof(eswb::event_queue_transfer_t);
        topic = (eswb::topic_proclaiming_tree_t *)data;

        if (h->msg_code == eswb::EQRB_CMD_SERVER_TOPIC) {
            m_topic[topic->topic_id] = *topic;
            if (topic->type == tt_double) {
                m_topic_column_num[topic->topic_id] = m_column_num;
                size_t buf_size = 1024;
                auto buf = std::make_unique<char[]>(buf_size);
                std::snprintf(buf.get(), buf_size, "%s_%s",
                              m_topic[event->topic_id].name, topic->name);
                m_header.push_back(buf.get());
                // TODO: Add columns on the go
                append(csv_log, m_header[m_column_num].data(),
                       m_header[m_column_num].length());
                m_column_num++;
                m_raw.push_back("0.0");
            }
        } else {
            new_line(csv_log);
            std::cout << m_raw.size() << std::endl;
            std::cout << "topic_name: " << m_topic[event->topic_id].name
                      << std::endl;
            std::cout << "timestamp: " << timestamp << std::endl;
            for (int i = 1;
                 i <= m_topic[event->topic_id].data_size / sizeof(double);
                 i++) {
                std::cout << "column_index: "
                          << m_topic_column_num[event->topic_id + i]
                          << std::endl;
                std::cout << "column_name: "
                          << m_header[m_topic_column_num[event->topic_id + i]]
                          << std::endl;
                std::cout << "value: "
                          << (double)(*(double *)(data +
                                                  (i - 1) * sizeof(double)))
                          << std::endl;
            }
            std::cout << std::endl;
            m_raw[0] = timestamp;
            for (int i = 1;
                 i <= m_topic[event->topic_id].data_size / sizeof(double);
                 i++) {
                size_t buf_size = 1024;
                auto buf = std::make_unique<char[]>(buf_size);
                std::snprintf(
                    buf.get(), buf_size, "%lf",
                    (double)(*(double *)(data + (i - 1) * sizeof(double))));
                m_raw[m_topic_column_num[event->topic_id + i]] = buf.get();
            }
            for (auto i = 0; i < m_raw.size(); i++) {
                append(csv_log, m_raw[i].data(), m_raw[i].length());
            }
        }
    }

    raw_log.close();
    csv_log.close();
    delete[] raw_buf;

    return rv;
}
