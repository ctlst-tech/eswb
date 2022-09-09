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
    int do_loss_each_n_pkt;
    uint do_loss_first_n_pkt;
    int pkt_num;

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
        pkt_num = 0;
    }

    void drop_packet_n(int n) {
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

    void write(bool up_agent, void *d, size_t s) {
        int do_write = -1;

        pkt_num++;
        // && (pkt_num > 0)
        if ((do_loss_each_n_pkt > 0) && (pkt_num % do_loss_each_n_pkt == 0)) {
            do_write = 0;
        } else if (do_loss_first_n_pkt > 0) {
            if (pkt_num-1 < do_loss_first_n_pkt) {
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

struct test_media_store_handle {
    SDTLtestBridge *bridge_ref;
//    ThreadSafeQueue<SDTLtestPacket> &stream;
    bool up_agent;

    test_media_store_handle(SDTLtestBridge *br, const std::string &path) : bridge_ref(br) {
        up_agent = path == "up";
    }
};

sdtl_rv_t sdtl_test_media_open(const char *path, void *params, void **h_rv) {
    auto *br_ref = (SDTLtestBridge *) params;
    auto *h = new test_media_store_handle(br_ref, path);

    *h_rv = h;

    return SDTL_OK;
}

sdtl_rv_t sdtl_test_media_close(void *h) {
    // TODO
    return SDTL_MEDIA_ERR;
}

sdtl_rv_t sdtl_test_media_read(void *h, void *data, size_t l, size_t *lr) {
    auto *bh = (test_media_store_handle *)h;
    *lr = bh->bridge_ref->read(bh->up_agent, data, l);
    return *lr == 0 ? SDTL_MEDIA_EOF : SDTL_OK;
}

sdtl_rv_t sdtl_test_media_write(void *h, void *data, size_t l) {
    auto *bh = (test_media_store_handle *)h;
    bh->bridge_ref->write(bh->up_agent, data, l);
    return SDTL_OK;
}

const sdtl_service_media_t sdtl_test_media = {
    .open = sdtl_test_media_open,
    .read = sdtl_test_media_read,
    .write = sdtl_test_media_write,
    .close = sdtl_test_media_close
};

TEST_CASE("SDPL testing bridge standalone test") {
    auto bridge = new SDTLtestBridge();

    auto m = &sdtl_test_media;

    void *writer_handle;
    void *reader_handle;

    sdtl_rv_t rv;
    rv = m->open("up", bridge, &writer_handle);
    REQUIRE(rv == SDTL_OK);
    rv = m->open("down", bridge, &reader_handle);
    REQUIRE(rv == SDTL_OK);

    uint8_t buf[256];
    uint8_t readback_buf[256];
    memset(readback_buf, 0, sizeof (readback_buf));
    gen_data(buf, sizeof(buf));

    rv = m->write(writer_handle, buf, 128);
    REQUIRE(rv == SDTL_OK);
    rv = m->write(writer_handle, buf + 128, 128);
    REQUIRE(rv == SDTL_OK);

    size_t br;

    int j = 0;
    for ( const int &i : {100, 28, 100, 28}) { // ^^^ 28 - is remnant of the packet of 128 ^^^
        rv = m->read(reader_handle, readback_buf + j, 100, &br);
        REQUIRE(rv == SDTL_OK);
        REQUIRE(br == i);
        j += i;
    }

    REQUIRE(memcmp(buf, readback_buf, sizeof(buf)) == 0);
}

#define SDTL_TEST_CHANNEL_NAME "test_channel"

sdtl_service_t *
create_and_start_service_w_channel(const char *service_name, const char *media_channel, size_t mtu, bool rel_ch,
                                   SDTLtestBridge &bridge) {

    sdtl_service_t *sdtl_service;

    sdtl_service = new sdtl_service_t;

    sdtl_rv_t rv;
    rv = sdtl_service_init(sdtl_service, service_name, "bus", mtu, 4, &sdtl_test_media);
    REQUIRE(rv == SDTL_OK);

    sdtl_channel_cfg_t ch_cfg_template = {
            .name = SDTL_TEST_CHANNEL_NAME,
            .id = 1,
            .type = rel_ch ? SDTL_CHANNEL_RELIABLE : SDTL_CHANNEL_NONRELIABLE,
            .mtu_override = 0,
    };

    auto *ch_cfg = new sdtl_channel_cfg_t;

    memcpy(ch_cfg, &ch_cfg_template, sizeof (*ch_cfg));

    rv = sdtl_channel_create(sdtl_service, ch_cfg);
    REQUIRE(rv == SDTL_OK);

    rv = sdtl_service_start(sdtl_service, media_channel, &bridge);
    REQUIRE(rv == SDTL_OK);

    return sdtl_service;
}



void sender_thread(sdtl_service_t *service, uint8_t *data2send, size_t data_size) {

}

struct SDTLtestSetup {
    sdtl_service_t *service_up;
    sdtl_service_t *service_down;

    sdtl_channel_handle_t chh_up;
    sdtl_channel_handle_t chh_down;
};

void sdtl_test_setup(SDTLtestSetup &setup, size_t mtu, bool reliable, SDTLtestBridge &bridge) {
    // init both sides terminals
    setup.service_up = create_and_start_service_w_channel("test_up", "up", mtu, reliable, bridge);
    setup.service_down = create_and_start_service_w_channel("test_down", "down", mtu, reliable, bridge);

    sdtl_rv_t rv;
    rv = sdtl_channel_open(setup.service_up, SDTL_TEST_CHANNEL_NAME, &setup.chh_up);
    REQUIRE(rv == SDTL_OK);

    rv = sdtl_channel_open(setup.service_down, SDTL_TEST_CHANNEL_NAME, &setup.chh_down);
    REQUIRE(rv == SDTL_OK);

}

void sdtl_test_deinit(SDTLtestSetup &setup) {
    sdtl_rv_t rv;
    rv = sdtl_service_stop(setup.service_down);
    CHECK(rv == SDTL_OK);

    rv = sdtl_service_stop(setup.service_up);
    CHECK(rv == SDTL_OK);
}

TEST_CASE("SDTL non-guaranteed delivery") {
    eswb_local_init(1);

    eswb_rv_t erv;

    erv = eswb_create("bus", eswb_inter_thread, 256);
    REQUIRE(erv == eswb_e_ok);

//    auto lose_every_n_paket = GENERATE(0, 10, 5, 3, 2, 1);
#define LOSE_PKT_ALL_BUT_LAST 111
    auto lose_every_n_paket = GENERATE(0, LOSE_PKT_ALL_BUT_LAST, 2, 3, 10);
    auto mtu = GENERATE(16, 32, 64, 128);
    auto data2send = GENERATE(10, 20, 57, 58, 64, 128, 256, 512, 1024);

    std::string losses_str;

    switch (lose_every_n_paket) {
        case LOSE_PKT_ALL_BUT_LAST:
            losses_str = " with all lost but the last";
            break;

        case 0:
            losses_str = " no tx/rx losses";
            break;

        default:
            if (lose_every_n_paket > 0) {
                losses_str = " with each " + std::to_string(lose_every_n_paket) + " pkt lost ";
            }
            break;
    }


    auto setup = SDTLtestSetup();
    SDTLtestBridge testing_bridge;
    sdtl_test_setup(setup, mtu, false, testing_bridge);



    SECTION("Send " + std::to_string(data2send) + " bytes" + losses_str + " MTU " + std::to_string(mtu)) {
        size_t payload_per_pkt = setup.chh_up.channel->max_payload_size;

        uint pkts_needed = data2send / payload_per_pkt; // full packets
        if (data2send % payload_per_pkt != 0) { // data remnant
            pkts_needed++;
        }

        if (lose_every_n_paket == LOSE_PKT_ALL_BUT_LAST) {
            testing_bridge.drop_first_n(pkts_needed - 1);
        } else if (lose_every_n_paket > 0) {
            testing_bridge.drop_packet_n(lose_every_n_paket);
        }


        uint8_t send_buffer[data2send];
        uint8_t rcv_buffer[data2send];

        // gen data
        memset(rcv_buffer, 0, data2send);
        gen_data(send_buffer, data2send);

        // define transmitter
        periodic_call_t sender = [&]() {
            sdtl_rv_t rv;
            rv = sdtl_channel_send_data(&setup.chh_down, send_buffer, data2send);
            CHECK(rv == SDTL_OK);
        };

        timed_caller sender_thread(sender, 50);

        sdtl_rv_t rv;

        sender_thread.start_once();

        size_t br;
        sdtl_channel_recv_arm_timeout(&setup.chh_up, 100000);
        rv = sdtl_channel_recv_data(&setup.chh_up, rcv_buffer, data2send, &br);
        sdtl_rv_t expected_rv;

        // check either transaction supposed to be successful
        switch (lose_every_n_paket) {
            case 0:
                expected_rv = SDTL_OK;
                break;

            case LOSE_PKT_ALL_BUT_LAST:
                if (pkts_needed > 1) {
                    expected_rv = SDTL_TIMEDOUT;
                } else {
                    expected_rv = SDTL_OK;
                }
                break;

            default:
                expected_rv = pkts_needed >= lose_every_n_paket ? SDTL_TIMEDOUT : SDTL_OK;
                break;
        }

        CHECK(rv == expected_rv);

        if (expected_rv == SDTL_OK) {
            CHECK(br == data2send);
            CHECK(memcmp(send_buffer, rcv_buffer, data2send) == 0);
        }

        sender_thread.stop();
    }

    testing_bridge.stop();
    sdtl_test_deinit(setup);
}


//
//
//TEST_CASE("SDTL guaranteed delivery") {
//
//    SECTION("Simple TX") {
//
//    }
//
//    SECTION("Fragmentation") {
//
//    }
//
//    SECTION("Duplex interaction") {
//
//    }
//}
