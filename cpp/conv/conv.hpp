#ifndef CONV_HPP
#define CONV_HPP

#include <time.h>

#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "eqrb.hpp"
#include "eswb.hpp"
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

namespace eswb {

struct TopicNode {
    uint16_t parent_id;
    topic_proclaiming_tree_t topic;
    std::vector<uint16_t> child;
    std::vector<uint16_t> primitive_topic;
};

struct __attribute__((packed)) Event {
    eqrb_interaction_header_t header;
    event_queue_transfer_t event;
    union {
        topic_proclaiming_tree_t topic;
        char data[0];
    } payload;
};

class TopicTree {
private:
    std::unordered_map<uint16_t, struct TopicNode> m_topic_tree;

public:
    TopicTree() {
        struct TopicNode root = {0};
        root.topic.topic_id = 0;
        m_topic_tree[0] = root;
    }

public:
    int addChildTopic(uint16_t parent_id, topic_proclaiming_tree_t *topic);
    bool isPrimitiveType(uint16_t id);
    int addPrimitiveTypeTopic(uint16_t id);
    std::string getTopicPath(uint16_t id);
    std::string getTopicName(uint16_t id);
    std::string convertRawToString(uint16_t id, void *raw);
    std::vector<uint16_t> getPrimitiveTypeTopics(uint16_t id);
    topic_data_type_s_t getTopicType(uint16_t id);
    uint16_t getTopicSize(uint16_t id);
    uint16_t getTopicOffset(uint16_t id);
};

class ConverterToCsv {
private:
    const std::string m_path_to_raw;
    const std::string m_path_to_csv;
    const std::string m_frame_sep;
    const char m_column_sep;
    std::string m_bus_from_file;
    std::string m_sep_from_file;

private:
    std::ifstream m_raw_log;
    std::ofstream m_csv_log;

private:
    char *m_file_start_ptr;
    char *m_file_end_ptr;
    char *m_file_cur_ptr;
    size_t m_file_size;

private:
    double m_time;
    double m_prev_time;

private:
    std::unordered_map<uint16_t, uint16_t> m_topic_column_num;
    unsigned int m_column_num;

private:
    std::string m_header;
    bool m_header_init;
    std::vector<std::string> m_values;
    TopicTree m_topic_tree;

public:
    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv,
                   const std::string &frame_separator,
                   const char &column_separator);
    ~ConverterToCsv();
    ConverterToCsv(const ConverterToCsv &c) = delete;
    ConverterToCsv &operator=(const ConverterToCsv &c) = delete;

public:
    bool convert(void);
    Event *getNextEvent(void);
    void newColumn(std::string &s, const char *data);
    void addColumnToHeader(const std::string &s);
};

}  // namespace eswb

#endif  // CONV_HPP
