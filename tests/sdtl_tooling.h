#ifndef SDTL_TOOLING_H
#define SDTL_TOOLING_H

#include "tooling.h"
#include "../src/lib/services/sdtl/sdtl.h"


class SDTLtestPacket {
public:
    uint8_t *buf;
    size_t size;

    SDTLtestPacket(void *data, size_t s) {
        buf = new uint8_t[s];
        memcpy(buf, data, s);
        size = s;
    }

    ~SDTLtestPacket() {
        delete buf;
    }

    bool fill_in_reader(void *usr_buf, size_t buf_len, size_t *bytes_read) {
        int br = std::min(buf_len, size);
        memcpy(usr_buf, buf, br);
        bool pop_pkt;

        if (br < size) {
            size -= br;
            uint8_t tmp_buf[size];

            memcpy(tmp_buf, buf + br, size);
            memcpy(buf, tmp_buf, size);
            pop_pkt = false;
        } else {
            pop_pkt = true;
        }

        *bytes_read = br;

        return pop_pkt;
    }
};


class SDTLtestBridge {
    ThreadSafeQueue<SDTLtestPacket> upstream;
    ThreadSafeQueue<SDTLtestPacket> downstream;
    size_t do_loss_each_n_pkt;
    uint do_loss_first_n_pkt;
    size_t pkt_num_down;
    size_t pkt_num_up;

    static void write_call(ThreadSafeQueue<SDTLtestPacket> &stream, void *d, size_t s) {
        auto pkt = new SDTLtestPacket(d, s);
        stream.push(pkt);
    }

    static size_t read_call(ThreadSafeQueue<SDTLtestPacket> &stream, void *d, size_t s) {
        size_t br;
        auto pkt = stream.front();

        if (pkt == nullptr) {
            return 0;
        }

        if (pkt->fill_in_reader(d, s, &br)) {
            stream.pop();
            delete pkt;
        }

        return br;
    }

public:
    SDTLtestBridge() {
        do_loss_each_n_pkt = 0;
        do_loss_first_n_pkt = 0;
        pkt_num_down = 0;
        pkt_num_up = 0;
    }

    void drop_packet_n(size_t n) {
        do_loss_each_n_pkt = n;
    }

    void drop_first_n(uint n) {
        do_loss_first_n_pkt = n;
    }

    ThreadSafeQueue<SDTLtestPacket> &resolve_stream(std::string name) {
        if (name == "up") {
            return upstream;
        } else if (name == "down") {
            return downstream;
        }

        throw std::string ("invalid stream name");
    }

    void stop() {
        upstream.stop();
        downstream.stop();
    }

    void start() {
        upstream.start();
        downstream.start();
    }

    void write(bool up_agent, void *d, size_t s) {
        int do_write = -1;

        size_t *pkt_num = up_agent ? &pkt_num_up : &pkt_num_down;

        (*pkt_num)++;
        if ((do_loss_each_n_pkt > 0) && (*pkt_num % do_loss_each_n_pkt == 0)) {
            do_write = 0;
        } else if (do_loss_first_n_pkt > 0) {
            if (*pkt_num-1 < do_loss_first_n_pkt) {
                do_write = 0;
            }
        }

        if (do_write) {
            write_call(up_agent ? downstream : upstream, d, s);
        }

        usleep(1000);
    }

    size_t read(bool up_agent, void *d, size_t s) {
        return read_call(up_agent ? upstream : downstream, d, s);
    }
};

extern const sdtl_service_media_t sdtl_test_media;

#endif //SDTL_TOOLING_H
