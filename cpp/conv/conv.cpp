#include "conv.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>

using namespace std;
using namespace eswb;

ConverterToCsv::ConverterToCsv(const string &path_to_raw,
                               const string &path_to_csv,
                               const string &frame_separator,
                               const char &column_separator)
    : m_path_to_csv(path_to_csv),
      m_path_to_raw(path_to_raw),
      m_frame_sep(frame_separator),
      m_column_sep(column_separator),
      m_column_num(0) {
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
        getline(m_raw_log, m_bus_from_file);
        getline(m_raw_log, m_sep_from_file);
        if (m_sep_from_file != m_frame_sep) {
            cout << "Invalid format" << endl;
        }

        m_raw_log.seekg(0, ios::end);
        m_file_size = m_raw_log.tellg();
        m_raw_log.seekg(0, ios::beg);
        m_file_start_ptr = new char[m_file_size];
        m_file_cur_ptr = m_file_start_ptr;

        if (m_file_start_ptr == nullptr) {
            cout << strerror(errno) << endl;
            cout << "Failed to create raw buf" << endl;
        } else {
            m_file_end_ptr = m_file_start_ptr + m_file_size;
            m_raw_log.read(m_file_start_ptr, m_file_size);
        }
    }
}

ConverterToCsv::~ConverterToCsv() {
    if (m_file_start_ptr != nullptr) {
        delete[] m_file_start_ptr;
    }
    if (m_raw_log.is_open()) {
        m_raw_log.close();
    }
    if (m_csv_log.is_open()) {
        m_csv_log.close();
    }
}

Event *ConverterToCsv::getNextEvent() {
    while (1) {
        if (m_file_cur_ptr >= m_file_end_ptr) {
            return nullptr;
        }
        char *rv = strstr(m_file_cur_ptr, m_frame_sep.data());
        int length = strnlen(m_file_cur_ptr, m_file_end_ptr - m_file_cur_ptr);
        length = (length < 1 ? 1 : length);
        if (rv == nullptr) {
            m_file_cur_ptr += length;
        } else {
            m_file_cur_ptr = rv + m_frame_sep.length();
            return reinterpret_cast<Event *>(rv + m_frame_sep.length());
        }
    }
}

void ConverterToCsv::newColumn(std::string &s, const char *data) {
    s.append(data);
    s.push_back(m_column_sep);
}

void ConverterToCsv::addColumnToHeader(const std::string &s) {
    newColumn(m_header, s.data());
    m_values.push_back("0.0");
    m_column_num++;
}

bool ConverterToCsv::convert(void) {
    if (m_file_start_ptr == nullptr || !m_raw_log.is_open() ||
        !m_csv_log.is_open() || m_sep_from_file != m_frame_sep) {
        return false;
    }

    Event *event = getNextEvent();
    if (event == nullptr) {
        return false;
    }

    addColumnToHeader("Time");
    addColumnToHeader("Period");

    while (1) {
        event = getNextEvent();
        if (event == nullptr) {
            break;
        }

        if (event->event.type == eqr_topic_proclaim) {
            int topic_num = 0;
            int num_of_topics =
                event->event.size / sizeof(topic_proclaiming_tree_t);
            uint16_t parent_id = event->event.topic_id;
            while (topic_num < num_of_topics) {
                topic_proclaiming_tree_t *topic =
                    &event->payload.topic + topic_num;
                if (topic->parent_ind != -1024) {
                    parent_id = topic->topic_id + topic->parent_ind;
                }
                int id = m_topic_tree.addChildTopic(parent_id, topic);
                if (id < 0) {
                    cout << "No such parent topic id: " << parent_id << endl;
                }

                if (m_topic_tree.isPrimitiveType(id)) {
                    m_topic_tree.addPrimitiveTypeTopic(id);
                    if (m_topic_column_num.count(id)) {
                        cout << "This id already exists: " << id << endl;
                    }
                    m_topic_column_num[id] = m_column_num;
                    string name = m_topic_tree.getTopicPath(id);
                    addColumnToHeader(name);
                }
                topic_num++;
            }
        } else if (event->header.msg_code == EQRB_CMD_SERVER_EVENT &&
                   event->event.type == eqr_topic_update) {
            if (m_header_init == false) {
                m_header.pop_back();
                m_csv_log.write(m_header.data(), m_header.size());
                m_header_init = true;
            }
            m_csv_log.write("\n", 1);

            char *data = event->payload.data;
            m_time = event->event.timestamp.sec * 1.0 +
                     event->event.timestamp.usec / 1000000.0;
            m_values[0] = to_string(m_time);
            m_values[1] = to_string(m_time - m_prev_time);

            vector<uint16_t> topics =
                m_topic_tree.getPrimitiveTypeTopics(event->event.topic_id);

            for (int i = 0; i < topics.size(); i++) {
                uint16_t id = topics[i];
                uint16_t offset = m_topic_tree.getTopicOffset(id);
                uint16_t size = m_topic_tree.getTopicSize(id);
                string string_value = m_topic_tree.convertRawToString(id, data);
                int sd = m_topic_column_num[id];
                m_values[sd] = string_value;
                data += size;
            }

            string out;
            for (auto i = 0; i < m_values.size(); i++) {
                newColumn(out, m_values[i].data());
            }
            out.pop_back();

            m_csv_log.write(out.data(), out.size());
            m_prev_time = m_time;
        } else {
            cout << "Offset: " << m_file_cur_ptr - m_file_start_ptr << endl;
            cout << "Unhandled cmd code: " << to_string(event->header.msg_code) << endl;
            cout << "Unhandled event type: " << to_string(event->event.type) << endl
                 << endl;
        }
    }
    return true;
}

int TopicTree::addChildTopic(uint16_t parent_id,
                             topic_proclaiming_tree_t *topic) {
    int rv = -1;
    struct TopicNode topic_node;
    if (m_topic_tree.count(parent_id)) {
        topic_node.topic = *topic;
        topic_node.parent_id = parent_id;
        m_topic_tree[topic->topic_id] = topic_node;
        m_topic_tree[parent_id].child.push_back(topic->topic_id);
        rv = topic->topic_id;
        cout << parent_id << " " << topic->topic_id << " " << topic->name
             << endl;
    }
    return rv;
}

bool TopicTree::isPrimitiveType(uint16_t id) {
    bool rv = false;
    if (m_topic_tree[id].topic.type >= tt_uint8 &&
        m_topic_tree[id].topic.type <= tt_string) {
        rv = true;
    }
    return rv;
}

int TopicTree::addPrimitiveTypeTopic(uint16_t id) {
    uint16_t current_id = id;
    while (current_id != 0) {
        m_topic_tree[current_id].primitive_topic.push_back(id);
        current_id = m_topic_tree[current_id].parent_id;
    }
    return id;
}

vector<uint16_t> TopicTree::getPrimitiveTypeTopics(uint16_t id) {
    return m_topic_tree[id].primitive_topic;
}

string TopicTree::getTopicPath(uint16_t id) {
    string path(m_topic_tree[id].topic.name);
    uint16_t parent_id = m_topic_tree[id].parent_id;
    while (parent_id != 0) {
        path.insert(0, "_");
        path.insert(0, m_topic_tree[parent_id].topic.name);
        parent_id = m_topic_tree[parent_id].parent_id;
    }
    return path;
}

string TopicTree::getTopicName(uint16_t id) {
    return string(m_topic_tree[id].topic.name);
}

topic_data_type_s_t TopicTree::getTopicType(uint16_t id) {
    return m_topic_tree[id].topic.type;
}

uint16_t TopicTree::getTopicSize(uint16_t id) {
    return m_topic_tree[id].topic.data_size;
}

uint16_t TopicTree::getTopicOffset(uint16_t id) {
    return m_topic_tree[id].topic.data_offset;
}

string TopicTree::convertRawToString(uint16_t id, void *raw) {
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
