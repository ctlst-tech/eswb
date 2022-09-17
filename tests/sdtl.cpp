#include "tooling.h"
#include "sdtl_tooling.h"



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

#define LOSE_PKT_ALL_BUT_LAST 111

std::string pkt_losses_note(uint32_t lose_every_n_paket) {
    switch (lose_every_n_paket) {
        case LOSE_PKT_ALL_BUT_LAST:
            return " with all lost but the last";

        case 0:
            return " no tx/rx losses";

        default:
            if (lose_every_n_paket > 0) {
                return " with each " + std::to_string(lose_every_n_paket) + " pkt lost ";
            }
            break;
    }

    return "invalid lose_every_n_paket";
}

void setup_bridge_losses(SDTLtestBridge &testing_bridge, size_t lose_every_n_paket, size_t pkts_needed) {
    if (lose_every_n_paket == LOSE_PKT_ALL_BUT_LAST) {
        testing_bridge.drop_first_n(pkts_needed - 1);
    } else if (lose_every_n_paket > 0) {
        testing_bridge.drop_packet_n(lose_every_n_paket);
    }
}

TEST_CASE("SDTL unreliable delivery") {
    eswb_local_init(1);

    eswb_rv_t erv;

    erv = eswb_create("bus", eswb_inter_thread, 256);
    REQUIRE(erv == eswb_e_ok);

//    auto lose_every_n_paket = GENERATE(0, 10, 5, 3, 2, 1);

    auto lose_every_n_paket = GENERATE(0, LOSE_PKT_ALL_BUT_LAST, 2, 3, 10);
    auto mtu = GENERATE(16, 32, 64, 128);
    auto data2send = GENERATE(15, 20, 57, 58, 64, 128, 400, 512, 600, 899);

    auto setup = SDTLtestSetup();
    SDTLtestBridge testing_bridge;
    sdtl_test_setup(setup, mtu, false, testing_bridge);

    SECTION("Send " + std::to_string(data2send) + " bytes" + pkt_losses_note(lose_every_n_paket) + " MTU " + std::to_string(mtu)) {
        size_t payload_per_pkt = setup.chh_up.channel->max_payload_size;

        uint pkts_needed = data2send / payload_per_pkt; // full packets
        if (data2send % payload_per_pkt != 0) { // data remnant
            pkts_needed++;
        }

        setup_bridge_losses(testing_bridge, lose_every_n_paket, pkts_needed);

        uint8_t send_buffer[data2send];
        uint8_t rcv_buffer[data2send];

        // gen data
        memset(rcv_buffer, 0, data2send);
        gen_data(send_buffer, data2send);

        // define transmitter
        periodic_call_t sender = [&]() {
            sdtl_rv_t rv;
            rv = sdtl_channel_send_data(&setup.chh_down, send_buffer, data2send);
//            CHECK(rv == SDTL_OK);
            thread_safe_failure_assert(rv == SDTL_OK, "rv != SDTL_OK");
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

TEST_CASE("SDTL reliable delivery") {
    eswb_local_init(1);

    eswb_rv_t erv;

    erv = eswb_create("bus", eswb_inter_thread, 256);
    REQUIRE(erv == eswb_e_ok);

    auto lose_every_n_paket = GENERATE(0, 3, 5);
    auto mtu = GENERATE(32, 64, 77);
    auto data2send = GENERATE(15, 20, 57, 58, 64, 128, 313, 512);


    auto setup = SDTLtestSetup();
    SDTLtestBridge testing_bridge;
    sdtl_test_setup(setup, mtu, true, testing_bridge);

    SECTION("Send " + std::to_string(data2send) + " bytes" + pkt_losses_note(lose_every_n_paket) + " MTU " + std::to_string(mtu)) {
        size_t payload_per_pkt = setup.chh_up.channel->max_payload_size;

        uint pkts_needed = data2send / payload_per_pkt; // full packets
        if (data2send % payload_per_pkt != 0) { // data remnant
            pkts_needed++;
        }

        setup_bridge_losses(testing_bridge, lose_every_n_paket, pkts_needed);

        uint8_t send_buffer[data2send];
        uint8_t rcv_buffer[data2send];

        // gen data
        memset(rcv_buffer, 0, data2send);
        gen_data(send_buffer, data2send);

        // define transmitter
        periodic_call_t sender = [&]() {
            sdtl_rv_t rv;
            rv = sdtl_channel_send_data(&setup.chh_down, send_buffer, data2send);
//            CHECK(rv == SDTL_OK);
            thread_safe_failure_assert(rv == SDTL_OK, "rv != SDTL_OK");
        };

        timed_caller sender_thread(sender, 50);

        sdtl_rv_t rv;

        sender_thread.start_once();

        size_t br;
//        sdtl_channel_recv_arm_timeout(&setup.chh_up, 100000);
        rv = sdtl_channel_recv_data(&setup.chh_up, rcv_buffer, data2send, &br);
        sdtl_rv_t expected_rv;

        expected_rv= SDTL_OK;

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


TEST_CASE("SDTL consecutive transfers") {
    eswb_local_init(1);

    eswb_rv_t erv;

    erv = eswb_create("bus", eswb_inter_thread, 256);
    REQUIRE(erv == eswb_e_ok);

    auto lose_every_n_paket = GENERATE(0, 3, 5);
    auto mtu = GENERATE(20, 37);
    auto data2send = 1024;
    auto seq_pieces = GENERATE(5, 10);

    auto setup = SDTLtestSetup();
    SDTLtestBridge testing_bridge;
    sdtl_test_setup(setup, mtu, true, testing_bridge);

    SECTION("Send " + std::to_string(data2send)
                + " bytes via " + std::to_string(seq_pieces) + " pieces"
                + pkt_losses_note(lose_every_n_paket)
                + " MTU " + std::to_string(mtu)
    ) {
        size_t payload_per_pkt = setup.chh_up.channel->max_payload_size;

        uint pkts_needed = data2send / payload_per_pkt; // full packets
        if (data2send % payload_per_pkt != 0) { // data remnant
            pkts_needed++;
        }

        setup_bridge_losses(testing_bridge, lose_every_n_paket, 0);

        uint8_t send_buffer[data2send];
        uint8_t rcv_buffer[data2send];

        // gen data
        memset(rcv_buffer, 0, data2send);
        gen_data(send_buffer, data2send);

        // define transmitter
        periodic_call_t sender = [&]() {
            sdtl_rv_t rv;
            uint32_t pkt_size_per_seq = data2send / seq_pieces;

            uint32_t l = data2send;
            uint32_t offset = 0;

            do {
                uint32_t bytes_per_seq = std::min(l, pkt_size_per_seq);
                do {
                    rv = sdtl_channel_send_data(&setup.chh_down, send_buffer + offset, bytes_per_seq);
                } while ((rv == SDTL_REMOTE_RX_NO_CLIENT) && (usleep(2000) | 1));

                thread_safe_failure_assert(rv == SDTL_OK, "rv != SDTL_OK");

                offset += bytes_per_seq;
                l -= bytes_per_seq;
            } while (l > 0);
        };

        timed_caller sender_thread(sender, 50, "sending client");
        pthread_setname_np("receiving client");

        sdtl_rv_t rv;
        sender_thread.start_once();
        size_t offset = 0;

        do {
            size_t br;
            rv = sdtl_channel_recv_data(&setup.chh_up, rcv_buffer + offset, data2send - offset, &br);
            REQUIRE(rv == SDTL_OK);
            offset += br;
        } while(offset < data2send);


        if (rv == SDTL_OK) {
            CHECK(offset == data2send);
            CHECK(memcmp(send_buffer, rcv_buffer, data2send) == 0);
        }

        sender_thread.stop();
    }

    testing_bridge.stop();
    sdtl_test_deinit(setup);
}

TEST_CASE("SDTL transfer reset") {
    eswb_local_init(1);

    eswb_rv_t erv;

    erv = eswb_create("bus", eswb_inter_thread, 256);
    REQUIRE(erv == eswb_e_ok);

    auto lose_every_n_paket = GENERATE(0, 3, 5);
    auto mtu = GENERATE(20, 37);
    auto data2send = 512;
    auto seq_pieces = 5;
    auto reset_on_piece = GENERATE(1, 2, 3);

    auto setup = SDTLtestSetup();
    SDTLtestBridge testing_bridge;
    sdtl_test_setup(setup, mtu, true, testing_bridge);

    SECTION("Send " + std::to_string(data2send)
                + " bytes via " + std::to_string(seq_pieces) + " pieces "
                + pkt_losses_note(lose_every_n_paket)
                + " MTU " + std::to_string(mtu)
                + " reset cond on piece " + std::to_string(reset_on_piece)
    ) {
        size_t payload_per_pkt = setup.chh_up.channel->max_payload_size;

        setup_bridge_losses(testing_bridge, lose_every_n_paket, 0);

        uint8_t send_buffer[data2send];
        uint8_t rcv_buffer[data2send];

        // gen data
        memset(rcv_buffer, 0, data2send);
        gen_data(send_buffer, data2send);

        // define transmitter
        periodic_call_t sender = [&]() {
            sdtl_rv_t rv;
            uint32_t pkt_size_per_seq = data2send / seq_pieces;

            uint32_t l = data2send;
            uint32_t offset = 0;
            size_t piece_num = 0;
            bool had_reset = false;
            do {
                uint32_t bytes_per_seq = std::min(l, pkt_size_per_seq);
                bool loop = true;
                do {
                    rv = sdtl_channel_send_data(&setup.chh_down, send_buffer + offset, bytes_per_seq);
                    switch (rv) {
                        case SDTL_OK:
                            loop = false;
                            break;

                        case SDTL_REMOTE_RX_CANCELED:
                        case SDTL_REMOTE_RX_NO_CLIENT:
                            usleep(2000);
                            break;

                        default:
                            usleep(2000);
                            break;
                    }
                } while (loop);

                thread_safe_failure_assert(rv == SDTL_OK, "rv != SDTL_OK");

                offset += bytes_per_seq;
                l -= bytes_per_seq;
                piece_num++;

                if (piece_num == reset_on_piece) {
                    rv = sdtl_channel_send_cmd(&setup.chh_down, SDTL_PKT_CMD_CODE_RESET);
                    CHECK(rv == SDTL_OK);
                    had_reset = true;
                }
            } while (l > 0);
        };

        timed_caller sender_thread(sender, 50, "sending client");
        pthread_setname_np("receiving client");

        sdtl_rv_t rv;
        sender_thread.start_once();
        size_t offset = 0;
        size_t piece_num = 0;
        size_t reset_nums = 0;

        do {
            size_t br;
            rv = sdtl_channel_recv_data(&setup.chh_up, rcv_buffer + offset, data2send - offset, &br);

            if (rv == SDTL_APP_RESET) {
                CHECK(piece_num == reset_on_piece);
                rv = sdtl_channel_reset_condition(&setup.chh_up);
                CHECK(rv == SDTL_OK);
                reset_nums++;
                continue;
            }
            piece_num++;

            CHECK(rv == SDTL_OK);
            offset += br;
        } while(offset < data2send);

        if (rv == SDTL_OK) {
            CHECK(offset == data2send);
            CHECK(memcmp(send_buffer, rcv_buffer, data2send) == 0);
        }

        sender_thread.stop();
    }

    testing_bridge.stop();
    sdtl_test_deinit(setup);
}



 /*
 *  TODO
 *   1. how to recover from sutiation, where we sender restarts in the middle of the transaction?
 *   2. how to recover from sutiation, where we receiver restarts in the middle of the transaction?
 *   3. sync MTU size? or at least check it. Different MTU will fail because of the fixed ESWB fifo size
 *   4. flush eswb fifo on starting accepting new frame
 *
 *  TODO new tests
 *   1. packets in sequence > 256
 *
 *
 */