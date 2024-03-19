#ifndef SDTL_HPP
#define SDTL_HPP

#include <iostream>
#include <thread>

#include "eswb.hpp"
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

namespace eswb {

class sdtl {
    sdtl_service_t *service;
    const std::string service_name;
    size_t mtu;
    std::string media_name;

public:
    sdtl(const std::string &sname, size_t mtu_)
        : service_name(sname), mtu(mtu_) {
        media_name = "serial";
    }

    sdtl_rv_t init(const std::string &mount_point) {
        return sdtl_service_init_w(&service, service_name.c_str(),
                                   mount_point.c_str(), mtu, 16,
                                   media_name.c_str());
    }

    sdtl_rv_t add_channel(const std::string &ch_name, uint8_t id,
                          sdtl_channel_type_t type) {
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
    } ch_cfgs[2] = {
        {.ch_name = "bus_sync", .id = 1, .type = SDTL_CHANNEL_RELIABLE},
        {.ch_name = "bus_sync_sk", .id = 2, .type = SDTL_CHANNEL_UNRELIABLE}};

public:
    BridgeSDTL(const std::string &dp, uint32_t br,
               const std::string &replicate_to_)
        : replicate_to(replicate_to_),
          device_path(dp),
          baudrate(br),
          bus_name("sdtl_bus"),
          service_name("sdtl"),
          sdtl_service(service_name, 0),
          sdtl_service_bus(bus_name, inter_thread, 256) {
        sdtl_service.init(bus_name);

        sdtl_service.add_channel(ch_cfgs[0].ch_name, ch_cfgs[0].id,
                                 ch_cfgs[0].type);
        sdtl_service.add_channel(ch_cfgs[1].ch_name, ch_cfgs[1].id,
                                 ch_cfgs[1].type);
    }

    eqrb_rv_t start() {
        sdtl_rv_t srv = sdtl_service.start(device_path, baudrate);
        if (srv != SDTL_OK) {
            return eqrb_media_err;
        }

        return eqrb_sdtl_client_connect(
            service_name.c_str(), ch_cfgs[0].ch_name.c_str(),
            ch_cfgs[1].ch_name.c_str(), replicate_to.c_str(), 1024, 0);
    }
};

void clrsrc();
void read_sdtl_bridge_serial(const std::string &path, unsigned baudrate);

}  // namespace eswb

#endif  // SDTL_HPP
