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
    eqrb_cmd_code_t getCmdCode(char *event_raw);
    uint32_t getEventTopicId(char *event_raw);
    topic_proclaiming_tree_t *getTopicPtr(char *event_raw);
    double getTimestampSec(char *event_raw);
    void *getData(char *event_raw);
    int addChildTopic(uint16_t parent_id, topic_proclaiming_tree_t *topic);
    bool isPrimitiveType(uint16_t id);
    int addPrimitiveTypeTopic(uint16_t id);
    std::string getTopicPath(uint16_t id);
    std::string getTopicName(uint16_t id);
    std::string convertRawToString(uint16_t id, void *raw);
    std::vector<uint16_t> getPrimitiveTypeTopics(uint16_t id);
    uint16_t getTopicSize(uint16_t id);
    uint16_t getTopicOffset(uint16_t id);
};

class ConverterToCsv {
private:
    const std::string m_path_to_raw;
    const std::string m_path_to_csv;

private:
    std::ifstream m_raw_log;
    std::ofstream m_csv_log;

private:
    std::string m_bus;
    std::string m_frame_sep;

private:
    char *m_file_ptr;
    size_t m_file_size;

private:
    std::unordered_map<uint16_t, uint16_t> m_topic_column_num;
    unsigned int m_column_num;

private:
    TopicTree m_topic_tree;

public:
    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv);
    ~ConverterToCsv();

private:
    ConverterToCsv(const ConverterToCsv &c);
    ConverterToCsv &operator=(const ConverterToCsv &c);

public:
    bool convert(void);

private:
    char *findSep(char *str, const char *end_ptr, const char *sep);

private:
    void newColumn(std::ofstream &log, const char *data, std::streamsize size);
    void newRow(std::ofstream &log);
};

}  // namespace eswb

#endif  // CONV_HPP
