#ifndef ESWB_ESWBCPP_H
#define ESWB_ESWBCPP_H

#include <string>
#include <eswb/types.h>
#include <eswb/api.h>
#include <eswb/event_queue.h>
#include "eswb/services/eqrb.h"

namespace eswb {

class Exception : public std::exception{
    eswb_rv_t ec;
    std::string msg;

    std::string what_msg;
public:
    Exception(const std::string &m, eswb_rv_t rv) : ec(rv), msg(m) {
        what_msg = msg + std::string(eswb_strerror(rv));
    }

    const char* what() {
        return what_msg.c_str();
    }
};

class Topic {
    std::string name;
    topic_data_type_t type;
    void *data_ref;
    Topic *parent;
    Topic *first_child;
    Topic *next_sibling;

    void print_node(int nesting_level = 0);

public:
    Topic(const std::string &n, topic_data_type_t t, void *dref) : name(n), type(t), data_ref(dref) {
        first_child = nullptr;
        next_sibling = nullptr;
        parent = nullptr;
    }

    ~Topic() {
        if (first_child != nullptr) {
            delete first_child;
        }
        if (next_sibling != nullptr) {
            delete next_sibling;
        }
    }

    void add_child(Topic *t) {
        if (first_child == nullptr) {
            first_child = t;
        } else {
            first_child->add_sibling(t);
        }

        t->parent = this;
    }

    void add_sibling(Topic *t) {
        if (next_sibling == nullptr) {
            next_sibling = t;
        } else {
            Topic *n;
            for (n = next_sibling; n->next_sibling != nullptr; n = n->next_sibling);
            n->next_sibling = t;
        }

        t->parent = parent;
    }

    std::string value_str();

    void print();
};


enum BusType {
    non_synced,
    inter_thread,
    inter_process
};

class Bus {
    std::string bus_path;
    enum BusType type;

    Topic *topic_tree;

    eswb_topic_descr_t root_td;

    eswb_type_t eswb_type(enum BusType bt) {
        switch (bt) {
            default:
            case non_synced:    return eswb_non_synced;
            case inter_thread:  return eswb_inter_thread;
            case inter_process: return eswb_inter_process;
        }
    }

public:
    Bus(const std::string &name, enum BusType bt, int max_topics): type(bt){
        bus_path = std::string(eswb_get_bus_prefix(eswb_type(bt))) + name;

        eswb_rv_t rv = eswb_create(name.c_str(), eswb_type(bt), max_topics);
        if (rv != eswb_e_ok) {
            throw Exception("eswb_create", rv);
        }

        rv = eswb_connect(bus_path.c_str(), &root_td);
        if (rv != eswb_e_ok) {
            throw Exception("eswb_connect", rv);
        }

        topic_tree = nullptr;
    }

    std::string mkdir(const std::string &dirname, const std::string &path = "") {
        std::string dpath = bus_path + path + '/';
        eswb_rv_t rv = eswb_mkdir(dpath.c_str(), dirname.c_str());
        if (rv != eswb_e_ok) {
            throw Exception("eswb_mkdir", rv);
        }

        return dpath + dirname;
    }

    void eq_enable(int queue_size, int buffer_size) {
        eswb_rv_t rv = eswb_event_queue_enable(root_td, queue_size, buffer_size);

        if (rv != eswb_e_ok) {
            throw Exception("eswb_event_queue_enable", rv);
        }
    }

    void eq_order(const std::string &topic_mask, int channel) {
        eswb_rv_t rv = eswb_event_queue_order_topic(root_td, topic_mask.c_str(), channel);

        if (rv != eswb_e_ok) {
            throw Exception("eswb_event_queue_order_topic", rv);
        }
    }

    void update_tree();
    void print_tree() {
        topic_tree->print();
    }
};


class Bridge {
    std::string bus2replicate;
    std::string replicate_to;
    struct eqrb_client_handle *ch;

public:
    Bridge(const std::string &bus2replicate_, const std::string &replicate_to_) :
            bus2replicate(bus2replicate_),
            replicate_to(replicate_to_) {

        eqrb_rv_t rv = eqrb_tcp_client_create(&ch);

        if (rv != eqrb_rv_ok) {
            throw Exception("eqrb_tcp_client_create (fix code)", eswb_e_invargs);
        }
    }

    void connect(const std::string &addr, int repl_map_size = 1024) {
        char err_msg[256];

        eqrb_rv_t rv = eqrb_tcp_client_connect(ch, addr.c_str(),
                                               bus2replicate.c_str(),
                                               replicate_to.c_str(),
                                               repl_map_size, err_msg);

        if (rv != eqrb_rv_ok) {
            std::string msg = "eqrb_tcp_client_connect (fix code)" + std::string(err_msg);
            throw Exception(msg, eswb_e_invargs);
        }
    }

    void close() {
        eqrb_tcp_client_close(ch);
    }
};


class BridgeSerial {
    std::string replicate_to;

public:
    BridgeSerial(const std::string &replicate_to_) :
            replicate_to(replicate_to_) {

    }

    void connect(const std::string &path, int baudrate, int repl_map_size = 1024) {
        char err_msg[256];

        eqrb_rv_t rv = eqrb_serial_client_connect(path.c_str(), baudrate,
                                               replicate_to.c_str(),
                                               repl_map_size);

        if (rv != eqrb_rv_ok) {
            std::string msg = "eqrb_serial_client_connect (fix code)" + std::string(err_msg);
            throw Exception(msg, eswb_e_invargs);
        }
    }
};

}

#endif //ESWB_ESWBCPP_H
