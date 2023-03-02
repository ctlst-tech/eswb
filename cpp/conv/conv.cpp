#include "conv.hpp"

#include <cstdio>
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

        // TODO: replace this to TopicTree class
        h = (eswb::eqrb_interaction_header_t *)topic_start;
        event = (eswb::event_queue_transfer_t
                     *)(topic_start + sizeof(eswb::eqrb_interaction_header_t));
        data = (char *)event + sizeof(eswb::event_queue_transfer_t);
        topic = (eswb::topic_proclaiming_tree_t *)data;

        if (h->msg_code == eswb::EQRB_CMD_SERVER_TOPIC) {
            int id = m_topic_tree.addChildTopic(event->topic_id, topic);
            if (id < 0) {
                std::cout << "No such parent topic id: " << event->topic_id;
            }
            if (m_topic_tree.isPrimitiveType(id)) {
                m_topic_tree.addPrimitiveTypeTopic(id);
                if (m_topic_column_num.count(id)) {
                    std::cout << "This id already exists: " << id;
                }
                m_topic_column_num[id] = m_column_num;
                std::string name = m_topic_tree.getTopicPath(id);
                m_header.push_back(name);
                // TODO: Add columns on the go
                append(csv_log, m_header[m_column_num].data(),
                       m_header[m_column_num].length());
                m_column_num++;
                m_raw.push_back("0.0");
            }
        } else if (h->msg_code == eswb::EQRB_CMD_SERVER_EVENT) {
            new_line(csv_log);
            std::vector<uint16_t> topics =
                m_topic_tree.getPrimitiveTypeTopics(event->topic_id);
            for (int i = 0; i < topics.size(); i++) {
                uint16_t id = topics[i];
                uint16_t offset = m_topic_tree.getTopicOffset(id);
                uint16_t size = m_topic_tree.getTopicSize(id);
                std::string string_value = m_topic_tree.convertRawToString(id, data);
                std::cout << "name: " << m_topic_tree.getTopicPath(id) << std::endl;
                std::cout << "value: " << string_value << std::endl;
                std::cout << "size: " << size << std::endl;
                std::cout << "offset: " << offset << std::endl;
                int sd = m_topic_column_num[id];
                m_raw[sd] = string_value;
                data += size;
            }
            std::cout << std::endl;
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

int eswb::TopicTree::addChildTopic(uint16_t parent_id,
                                   topic_proclaiming_tree_t *topic) {
    int rv = -1;
    struct TopicNode topic_node;
    if (m_topic_tree.count(parent_id)) {
        topic_node.topic = *topic;
        topic_node.parent_id = parent_id;
        m_topic_tree[topic->topic_id] = topic_node;
        m_topic_tree[parent_id].child.push_back(topic->topic_id);
        rv = topic->topic_id;
    }
    return rv;
}

bool eswb::TopicTree::isPrimitiveType(uint16_t id) {
    bool rv = false;
    if (m_topic_tree[id].topic.type >= tt_uint8 &&
        m_topic_tree[id].topic.type <= tt_string) {
        rv = true;
    }
    return rv;
}

int eswb::TopicTree::addPrimitiveTypeTopic(uint16_t id) {
    uint16_t current_id = id;
    while (current_id != 0) {
        m_topic_tree[current_id].primitive_topic.push_back(id);
        current_id = m_topic_tree[current_id].parent_id;
    }
    return id;
}

std::vector<uint16_t> eswb::TopicTree::getPrimitiveTypeTopics(uint16_t id) {
    return m_topic_tree[id].primitive_topic;
}

std::string eswb::TopicTree::getTopicPath(uint16_t id) {
    std::string path(m_topic_tree[id].topic.name);
    uint16_t parent_id = m_topic_tree[id].parent_id;
    while (parent_id != 0) {
        path.insert(0, "_");
        path.insert(0, m_topic_tree[parent_id].topic.name);
        parent_id = m_topic_tree[parent_id].parent_id;
    }
    return path;
}

std::string eswb::TopicTree::getTopicName(uint16_t id) {
    return std::string(m_topic_tree[id].topic.name);
}

uint16_t eswb::TopicTree::getTopicSize(uint16_t id) {
    return m_topic_tree[id].topic.data_size;
}

uint16_t eswb::TopicTree::getTopicOffset(uint16_t id) {
    return m_topic_tree[id].topic.data_offset;
}

std::string eswb::TopicTree::convertRawToString(uint16_t id, void *raw) {
    std::string rv;
    topic_data_type_s_t type = m_topic_tree[id].topic.type;
#define CAST_DREF2VAL(__t) (*((__t *)raw))
    switch (type) {
        default:
            rv = "";
            break;
        case tt_float:
            rv = std::to_string(CAST_DREF2VAL(float));
            break;
        case tt_double:
            rv = std::to_string(CAST_DREF2VAL(double));
            break;
        case tt_uint8:
            rv = std::to_string(CAST_DREF2VAL(uint8_t));
            break;
        case tt_int8:
            rv = std::to_string(CAST_DREF2VAL(int8_t));
            break;
        case tt_uint16:
            rv = std::to_string(CAST_DREF2VAL(uint16_t));
            break;
        case tt_int16:
            rv = std::to_string(CAST_DREF2VAL(int16_t));
            break;
        case tt_uint32:
            rv = std::to_string(CAST_DREF2VAL(uint32_t));
            break;
        case tt_int32:
            rv = std::to_string(CAST_DREF2VAL(int32_t));
            break;
        case tt_uint64:
            rv = std::to_string(CAST_DREF2VAL(uint64_t));
            break;
        case tt_int64:
            rv = std::to_string(CAST_DREF2VAL(int64_t));
            break;
        case tt_string:
            rv = "\"" + std::string((const char *)raw) + "\"";
            break;
    }
    return rv;
}
