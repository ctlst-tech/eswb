#include "conv.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>

using namespace std;

eswb::ConverterToCsv::ConverterToCsv(const std::string &path_to_raw,
                                     const std::string &path_to_csv)
    : m_path_to_csv(path_to_csv), m_path_to_raw(path_to_raw), m_column_num(0) {
    m_raw_log.open(m_path_to_raw, ios::in);
    m_csv_log.open(m_path_to_csv, ios::out | ios::trunc | ios::binary);

    if (!m_raw_log.is_open()) {
        cout << strerror(errno) << endl;
        cout << "Failed to open raw log: " << m_path_to_raw << endl;
    }

    if (!m_csv_log.is_open()) {
        cout << strerror(errno) << endl;
        cout << "Failed to create csv log: " << m_path_to_csv << endl;
    }

    if (m_csv_log.is_open() && m_raw_log.is_open()) {
        getline(m_raw_log, m_bus);
        getline(m_raw_log, m_frame_sep);

        m_raw_log.seekg(0, ios::end);
        m_file_size = m_raw_log.tellg();
        m_raw_log.seekg(0, ios::beg);
        m_file_ptr = new char[m_file_size];

        if (m_file_ptr == nullptr) {
            cout << strerror(errno) << endl;
            cout << "Failed to create raw buf: " << endl;
        } else {
            m_raw_log.read(m_file_ptr, m_file_size);
        }
    }
}

eswb::ConverterToCsv::~ConverterToCsv() {
    if (m_file_ptr != nullptr) {
        delete[] m_file_ptr;
    }
    if (m_raw_log.is_open()) {
        m_raw_log.close();
    }
    if (m_csv_log.is_open()) {
        m_csv_log.close();
    }
}

char *eswb::ConverterToCsv::findSep(char *str, const char *end_ptr,
                                    const char *sep) {
    while (1) {
        if (str >= end_ptr) {
            return nullptr;
        }
        char *rv = strstr(str, sep);
        int length = strnlen(str, end_ptr - str);
        length = (length < 1 ? 1 : length);
        if (rv == nullptr) {
            str += length;
        } else {
            return rv;
        }
    }
}

void eswb::ConverterToCsv::newColumn(ofstream &log, const char *data,
                                     streamsize size) {
    log.write(data, size);
    log.write(",", 1);
}

void eswb::ConverterToCsv::newRow(ofstream &log) {
    log.write("\n", 1);
}

bool eswb::ConverterToCsv::convert(void) {
    if (m_file_ptr == nullptr || !m_raw_log.is_open() || !m_csv_log.is_open()) {
        cout << "Initialization failed" << endl;
        return false;
    }

    std::vector<std::string> m_header;
    std::vector<std::string> m_row;

    char *topic_start = m_file_ptr;
    char *file_end_ptr = m_file_ptr + m_file_size;
    char timestamp[32];
    char dt[32];

    m_header.push_back("Time");
    m_row.push_back("0.0");
    m_column_num++;
    m_header.push_back("Period");
    m_row.push_back("0.0");
    m_column_num++;

    newColumn(m_csv_log, m_header[0].data(), m_header[0].length());
    newColumn(m_csv_log, m_header[1].data(), m_header[1].length());

    while (1) {
        topic_start = findSep(topic_start, file_end_ptr, m_frame_sep.data());
        if (topic_start == nullptr) {
            break;
        }

        char *topic_end = topic_start + m_frame_sep.length();
        topic_end = findSep(topic_end, file_end_ptr, m_frame_sep.data());

        topic_start = topic_start + m_frame_sep.length();

        eswb::eqrb_cmd_code_t cmd_code = m_topic_tree.getCmdCode(topic_start);
        uint32_t event_id = m_topic_tree.getEventTopicId(topic_start);
        char *data = static_cast<char *>(m_topic_tree.getData(topic_start));

        if (cmd_code == eswb::EQRB_CMD_SERVER_TOPIC) {
            int id = m_topic_tree.addChildTopic(
                event_id, m_topic_tree.getTopicPtr(topic_start));

            if (id < 0) {
                cout << "No such parent topic id: " << event_id << endl;
            }

            if (m_topic_tree.isPrimitiveType(id)) {
                m_topic_tree.addPrimitiveTypeTopic(id);
                if (m_topic_column_num.count(id)) {
                    cout << "This id already exists: " << id << endl;
                }
                m_topic_column_num[id] = m_column_num;
                string name = m_topic_tree.getTopicPath(id);
                m_header.push_back(name);
                newColumn(m_csv_log, m_header[m_column_num].data(),
                          m_header[m_column_num].length());
                m_column_num++;
                m_row.push_back("0.0");
            }
        } else if (cmd_code == eswb::EQRB_CMD_SERVER_EVENT) {
            double time = m_topic_tree.getTimestampSec(topic_start);
            sprintf(timestamp, "%lf", time);
            m_row[0] = timestamp;

            static double prev_time = 0.0;
            sprintf(dt, "%lf", time - prev_time);
            m_row[1] = dt;

            newRow(m_csv_log);

            vector<uint16_t> topics =
                m_topic_tree.getPrimitiveTypeTopics(event_id);

            for (int i = 0; i < topics.size(); i++) {
                uint16_t id = topics[i];
                uint16_t offset = m_topic_tree.getTopicOffset(id);
                uint16_t size = m_topic_tree.getTopicSize(id);
                string string_value = m_topic_tree.convertRawToString(id, data);
                int sd = m_topic_column_num[id];
                m_row[sd] = string_value;
                data += size;
            }
            for (auto i = 0; i < m_row.size(); i++) {
                newColumn(m_csv_log, m_row[i].data(), m_row[i].length());
            }
            prev_time = time;
        } else {
            cout << "Unhandled cmd code: " << cmd_code << endl;
        }
    }
    return true;
}

eswb::eqrb_cmd_code_t eswb::TopicTree::getCmdCode(char *event_raw) {
    eswb::eqrb_interaction_header_t *h =
        (eswb::eqrb_interaction_header_t *)event_raw;
    return static_cast<eswb::eqrb_cmd_code_t>(h->msg_code);
}

uint32_t eswb::TopicTree::getEventTopicId(char *event_raw) {
    eswb::event_queue_transfer_t *event =
        (eswb::event_queue_transfer_t *)(event_raw +
                                         sizeof(
                                             eswb::eqrb_interaction_header_t));
    return static_cast<uint32_t>(event->topic_id);
}

eswb::topic_proclaiming_tree_t *eswb::TopicTree::getTopicPtr(char *event_raw) {
    eswb::event_queue_transfer_t *event =
        (eswb::event_queue_transfer_t *)(event_raw +
                                         sizeof(
                                             eswb::eqrb_interaction_header_t));
    char *data = (char *)event + sizeof(eswb::event_queue_transfer_t);
    eswb::topic_proclaiming_tree_t *topic =
        (eswb::topic_proclaiming_tree_t *)data;
    return topic;
}

double eswb::TopicTree::getTimestampSec(char *event_raw) {
    eswb::event_queue_transfer_t *event =
        (eswb::event_queue_transfer_t *)(event_raw +
                                         sizeof(
                                             eswb::eqrb_interaction_header_t));
    return event->timestamp.sec + event->timestamp.usec / 1000000.0;
}

void *eswb::TopicTree::getData(char *event_raw) {
    eswb::event_queue_transfer_t *event =
        (eswb::event_queue_transfer_t *)(event_raw +
                                         sizeof(
                                             eswb::eqrb_interaction_header_t));
    char *data = (char *)event + sizeof(eswb::event_queue_transfer_t);
    return (void *)data;
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

vector<uint16_t> eswb::TopicTree::getPrimitiveTypeTopics(uint16_t id) {
    return m_topic_tree[id].primitive_topic;
}

string eswb::TopicTree::getTopicPath(uint16_t id) {
    string path(m_topic_tree[id].topic.name);
    uint16_t parent_id = m_topic_tree[id].parent_id;
    while (parent_id != 0) {
        path.insert(0, "_");
        path.insert(0, m_topic_tree[parent_id].topic.name);
        parent_id = m_topic_tree[parent_id].parent_id;
    }
    return path;
}

string eswb::TopicTree::getTopicName(uint16_t id) {
    return string(m_topic_tree[id].topic.name);
}

uint16_t eswb::TopicTree::getTopicSize(uint16_t id) {
    return m_topic_tree[id].topic.data_size;
}

uint16_t eswb::TopicTree::getTopicOffset(uint16_t id) {
    return m_topic_tree[id].topic.data_offset;
}

string eswb::TopicTree::convertRawToString(uint16_t id, void *raw) {
    string rv;
    topic_data_type_s_t type = m_topic_tree[id].topic.type;
    switch (type) {
        default:
            rv = "";
            break;
        case tt_float:
            rv = to_string(*static_cast<float *>(raw));
            break;
        case tt_double:
            rv = to_string(*static_cast<double *>(raw));
            break;
        case tt_uint8:
            rv = to_string(*static_cast<uint8_t *>(raw));
            break;
        case tt_int8:
            rv = to_string(*static_cast<int8_t *>(raw));
            break;
        case tt_uint16:
            rv = to_string(*static_cast<uint16_t *>(raw));
            break;
        case tt_int16:
            rv = to_string(*static_cast<int16_t *>(raw));
            break;
        case tt_uint32:
            rv = to_string(*static_cast<uint32_t *>(raw));
            break;
        case tt_int32:
            rv = to_string(*static_cast<int32_t *>(raw));
            break;
        case tt_uint64:
            rv = to_string(*static_cast<uint64_t *>(raw));
            break;
        case tt_int64:
            rv = to_string(*static_cast<int64_t *>(raw));
            break;
        case tt_string:
            rv = "\"" + string(static_cast<const char *>(raw)) + "\"";
            break;
    }
    return rv;
}
