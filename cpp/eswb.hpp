#ifndef ESWB_ESWBCPP_H
#define ESWB_ESWBCPP_H

#include <string>
#include <eswb/types.h>
#include <eswb/api.h>
#include <eswb/event_queue.h>
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

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


class sdtl {

    sdtl_service_t *service;
    const std::string service_name;
    size_t mtu;
    std::string media_name;

public:
    sdtl(const std::string &sname, size_t mtu_) :
        service_name(sname), mtu(mtu_){

        media_name = "serial";
    }

    sdtl_rv_t init(const std::string &mount_point) {
        return sdtl_service_init_w(&service, service_name.c_str(), mount_point.c_str(), mtu, 16, media_name.c_str());
    }


    sdtl_rv_t add_channel(const std::string &ch_name, uint8_t id, sdtl_channel_type_t type) {
        sdtl_channel_cfg_t cfg;

        cfg.name = ch_name.c_str();
        cfg.id = id;
        cfg.type = type;
        cfg.mtu_override = 0;

        return sdtl_channel_create(service, &cfg);
    }

    sdtl_rv_t start(const std::string &dev_path, uint32_t baudrate) {
        sdtl_media_serial_params_t params;
        params.baudrate = baudrate;
        return sdtl_service_start(service, dev_path.c_str(), &params);
    }
};


class BridgeSDTL {
    std::string replicate_to;
    std::string device_path;
    uint32_t baudrate;

    const std::string bus_name;
    const std::string service_name;

    sdtl sdtl_service;
    Bus sdtl_service_bus;


    const struct {
        std::string ch_name;
        uint8_t id;
        sdtl_channel_type_t type;
    } ch_cfgs [2] = {
        {
            .ch_name = "bus_sync",
            .id = 1,
            .type = SDTL_CHANNEL_RELIABLE
        },
        {
            .ch_name = "bus_sync_sk",
            .id = 2,
            .type = SDTL_CHANNEL_UNRELIABLE
        }
    };

public:
    BridgeSDTL(const std::string &dp, uint32_t br, const std::string &replicate_to_):
            replicate_to(replicate_to_),
            device_path(dp),
            baudrate(br),
            bus_name("sdtl_bus"),
            service_name("sdtl"),
            sdtl_service(service_name, 0),
            sdtl_service_bus(bus_name, inter_thread, 256) {

        sdtl_service.init(bus_name);

        sdtl_service.add_channel(ch_cfgs[0].ch_name, ch_cfgs[0].id, ch_cfgs[0].type);
        sdtl_service.add_channel(ch_cfgs[1].ch_name, ch_cfgs[1].id, ch_cfgs[1].type);
    }

    eqrb_rv_t start() {
        sdtl_rv_t srv = sdtl_service.start(device_path, baudrate);
        if (srv != SDTL_OK) {
            return eqrb_media_err;
        }

        return eqrb_sdtl_client_connect(service_name.c_str(),
                                 ch_cfgs[0].ch_name.c_str(), ch_cfgs[1].ch_name.c_str(),
                                 replicate_to.c_str(),
                                 1024,
                                 0);

    }
};

}

#endif //ESWB_ESWBCPP_H
