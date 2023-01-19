
#include "eswb.hpp"

#include <iomanip>
#include <iostream>

#include "../src/lib/include/registry.h"

extern "C" const topic_t *local_bus_topics_list(eswb_topic_descr_t td);

namespace eswb {

Topic *new_Topic(const topic_t *t) {
    return new Topic(std::string(t->name), t->type, t->data);
}

static void process_children(Topic *rt, const topic_t *t) {
    topic_t *n = t->first_child;
    while (n != NULL) {
        Topic *tn = new_Topic(n);
        rt->add_child(tn);
        process_children(tn, n);
        n = n->next_sibling;
    }
}

void Bus::update_tree() {
    const topic_t *lrt = local_bus_topics_list(
        abs(root_td));  // FIXME dont read registry memory directly
    Topic *root = new_Topic(lrt);
    process_children(root, lrt);

    if (topic_tree != nullptr) {
        delete topic_tree;
        topic_tree = nullptr;
    }

    topic_tree = root;

    /*
    eswb_topic_id_t tid = 0;
    topic_extract_t te;
    Topic *t;

    eswb_rv_t rv = eswb_get_next_topic_info(root_td, &tid, &te);
    if (rv != eswb_e_ok) {
        throw Exception("eswb_get_next_topic_info", rv);
    }
    */
}

int pc(int w) {
    return w < 1 ? 1 : w;
}

void Topic::print_node(int nesting_level) {
    std::string spaces = std::string(nesting_level * 2, ' ');
    std::string value = value_str();

    std::string out = spaces + name;
    out += std::string(pc(30 - out.size()), ' ') + value;
    out += std::string(pc(60 - out.size()), ' ') + eswb_type_name(type);

    std::cout << out << std::endl;

    if (first_child != nullptr) {
        first_child->print_node(nesting_level + 1);
    }
    if (next_sibling != nullptr) {
        next_sibling->print_node(nesting_level);
    }
}

void Topic::print() {
    print_node();
}

std::string Topic::value_str() {
    std::string rv;
#define CAST_DREF2VAL(__t) (*((__t *)data_ref))
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
            rv = "\"" + std::string((const char *)data_ref) + "\"";
            break;
    }

    if (rv[0] != '-') {
        rv = " " + rv;
    }

    return rv;
}

}  // namespace eswb