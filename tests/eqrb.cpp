//
// Created by goofy on 1/6/22.
//


#include "tooling.h"

#include "../src/lib/utils/eqrb/framing.h"
#include "../src/lib/utils/eqrb/eqrb_core.h"

typedef std::function<bool (eqrb_rx_state_t*, eqrb_rv_t, bool last)> check_lambda_t;

bool rx(int parts, uint8_t *tx_frame_buf, size_t tx_frame_size, check_lambda_t &check_lambda) {
    eqrb_rx_state_t rx_state;
#       define RX_PAYLOAD_BUFF_SIZE 512
    uint8_t rx_payload_buf[RX_PAYLOAD_BUFF_SIZE];
    eqrb_init_state(&rx_state, rx_payload_buf, RX_PAYLOAD_BUFF_SIZE);
    size_t bytes_processed;
    eqrb_rv_t rv;

    size_t bytes_to_process = std::max<size_t>(1, tx_frame_size / parts);
    size_t offset = 0;

    bool loop;
    do {
        rv = eqrb_rx_frame_iteration(&rx_state, tx_frame_buf + offset,
                                     std::min<size_t>(bytes_to_process, tx_frame_size - offset), &bytes_processed);

        // terminate loop if necessary
        loop = check_lambda(&rx_state, rv, false);

        offset += bytes_processed;
    } while ((offset < tx_frame_size) && loop);

    check_lambda(&rx_state, rv, true);

    return loop;
};

TEST_CASE("EQBR framing", "[eqrb][unit]") {

#   define FRAME_BUF_SIZE 1024
#   define PAYLOAD_BUFF_SIZE 256

#   define COMMAND_CODE 0

    uint8_t tx_payload[PAYLOAD_BUFF_SIZE] = {0};
    uint8_t tx_frame_buf[FRAME_BUF_SIZE] = {0};

    SECTION("Bad command code") {
        size_t size;
        REQUIRE( eqrb_make_tx_frame(EQRB_FRAME_BEGIN_CHAR, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                &size) == eqrb_inv_code);
        REQUIRE( eqrb_make_tx_frame(EQRB_FRAME_END_CHAR, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                    &size) == eqrb_inv_code);
    }

    SECTION("Too small frame buffer") {
        size_t size;
        REQUIRE( eqrb_make_tx_frame(EQRB_FRAME_END_CHAR, tx_payload, sizeof(tx_payload), tx_frame_buf, sizeof(tx_payload),
                                    &size) == eqrb_small_buf);
    }

    for (unsigned char & i : tx_payload) {
        i = 123;
    }

    eqrb_rv_t rv;

    size_t tx_frame_size;

    eqrb_rx_state_t rx_state;
#   define RX_PAYLOAD_BUFF_SIZE 512
    uint8_t rx_payload_buf[RX_PAYLOAD_BUFF_SIZE];
    eqrb_init_state(&rx_state, rx_payload_buf, RX_PAYLOAD_BUFF_SIZE);

    uint8_t expected_code= COMMAND_CODE;
    uint8_t *buf_to_compare = tx_payload;

    check_lambda_t check_lambda_simple = [&] (eqrb_rx_state_t *rx_state, eqrb_rv_t rv, bool last) -> bool {
        if (last) {
            CHECK(rv == eqrb_rv_rx_got_frame);
            CHECK(rx_state->current_command_code == expected_code);

            CHECK(memcmp(rx_state->payload_buffer_origin,
                         buf_to_compare,
                         rx_state->current_payload_size) == 0);
        }
        return true;
    };

    for (auto &parts : {1, 3, 5, 10, 33}) {
        SECTION(std::string("Frame RX tests | by ") + std::to_string(parts) + std::string(" parts")) {
            SECTION("Simple") {
                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size);
                REQUIRE(rv == eqrb_rv_ok);

                buf_to_compare = tx_payload;
                expected_code = COMMAND_CODE;

                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With B-s and E-s") {
                for (int i: {10, 13, 43, 57}) {
                    tx_payload[i] = EQRB_FRAME_BEGIN_CHAR;
                }
                for (int i: {33, 22, 54, 81}) {
                    tx_payload[i] = EQRB_FRAME_END_CHAR;
                }
                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, PAYLOAD_BUFF_SIZE, tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With B-s and E-s") {
                for (int i: {10, 13, 43, 57}) {
                    tx_payload[i] = EQRB_FRAME_BEGIN_CHAR;
                }
                for (int i: {33, 22, 54, 81}) {
                    tx_payload[i] = EQRB_FRAME_END_CHAR;
                }
                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, PAYLOAD_BUFF_SIZE, tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With All B-s") {
                for (uint8_t &i: tx_payload) {
                    i = EQRB_FRAME_BEGIN_CHAR;
                }
                rv = eqrb_make_tx_frame(0, tx_payload, PAYLOAD_BUFF_SIZE, tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With B or/and E in crc") {
                uint32_t data;
                int have_seen_b = 0;
                int have_seen_e = 0;
                int have_seen_be = 0;

                buf_to_compare = (uint8_t *) &data;

                for (data = 0; data < UINT32_MAX >> 16; data++) {
                    rv = eqrb_make_tx_frame(COMMAND_CODE, &data, sizeof(data), tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                    REQUIRE(rv == eqrb_rv_ok);

                    if (tx_frame_buf[tx_frame_size - 4] == EQRB_FRAME_BEGIN_CHAR) {
                        have_seen_b = -1;
                    }
                    if (tx_frame_buf[tx_frame_size - 4] == EQRB_FRAME_END_CHAR) {
                        have_seen_e = -1;
                    }
                    if ((tx_frame_buf[tx_frame_size - 6] == EQRB_FRAME_BEGIN_CHAR) &&
                        (tx_frame_buf[tx_frame_size - 4] == EQRB_FRAME_END_CHAR)) {
                        have_seen_be = -1;
                    }
                    REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
                }
                REQUIRE(have_seen_b);
                REQUIRE(have_seen_e);
                REQUIRE(have_seen_be);
            }

            SECTION("With restarted frame") {
                size_t tx_frame_size_1;
                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, 50, tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size_1);

                REQUIRE(rv == eqrb_rv_ok);

                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, 50, tx_frame_buf + 25, FRAME_BUF_SIZE - 25,
                                        &tx_frame_size);

                tx_frame_size_1 = 25;

                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size + tx_frame_size_1, check_lambda_simple) == true);
            }

            SECTION("With several frames through") {
                uint8_t tx_payload2 [128];

                int j = 0;
                for (auto &i : tx_payload2) {
                    i = j++;
                }

                int received_frames = 0;

                check_lambda_t check_lambda_2frames = [&] (eqrb_rx_state_t *rx_state, eqrb_rv_t rv, bool last)  -> bool  {
                    if (!last) {
                        if (rv == eqrb_rv_rx_got_frame) {
                            received_frames++;
                            CHECK(rx_state->current_command_code == expected_code);

                            if (received_frames == 1) {
                                CHECK(memcmp(rx_state->payload_buffer_origin,
                                             tx_payload,
                                             rx_state->current_payload_size) == 0);
                            } else if (received_frames == 2) {
                                CHECK(memcmp(rx_state->payload_buffer_origin,
                                             tx_payload2,
                                             rx_state->current_payload_size) == 0);
                            }
                        }
                    } else {
                        CHECK(received_frames == 2);
                    }
                    return true;
                };

                size_t tx_frame_size2;
                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size);
                REQUIRE(rv == eqrb_rv_ok);

                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload2, sizeof(tx_payload2), tx_frame_buf  + tx_frame_size, FRAME_BUF_SIZE - 25,
                                        &tx_frame_size2);

                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size + tx_frame_size2, check_lambda_2frames) == true);
            }

            SECTION("With broken frame") {
                check_lambda_t check_lambda_bad_crc = [&] (eqrb_rx_state_t *rx_state, eqrb_rv_t rv, bool last)  -> bool  {
                    if (last) {
                        CHECK(rv == eqrb_rv_rx_inv_crc);
                    }
                    return true;
                };

                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size);

                tx_frame_buf[8] = 55;

                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_bad_crc) == true);
            }

            SECTION("With RX buffer overflow (frame with no end)") {
                uint8_t tx_payload_overflow [RX_PAYLOAD_BUFF_SIZE * 2];
                uint8_t frame_buf [RX_PAYLOAD_BUFF_SIZE * 5];

                uint8_t j = 0;
                for (auto &i : tx_payload_overflow) {
                    i = j++;
                }

                check_lambda_t check_lambda_overflow = [&] (eqrb_rx_state_t *rx_state, eqrb_rv_t rv, bool last)  -> bool  {
                    if (rv == eqrb_rv_rx_buf_overflow) {
                        CHECK(rx_state->payload_buffer_ptr - rx_state->payload_buffer_origin
                                        == rx_state->payload_buffer_max_size);
                        return false;
                    }
                    return true;
                };

                rv = eqrb_make_tx_frame(COMMAND_CODE, tx_payload_overflow, sizeof(tx_payload_overflow), frame_buf, sizeof(frame_buf),
                                        &tx_frame_size);

                REQUIRE(rv == eqrb_rv_ok);
                REQUIRE(rx(parts, frame_buf, tx_frame_size, check_lambda_overflow) == false);
            }

            SECTION("With empty frame") {
                uint8_t empty_frame [4] = {EQRB_FRAME_BEGIN_CHAR, EQRB_FRAME_BEGIN_CHAR, EQRB_FRAME_END_CHAR, EQRB_FRAME_END_CHAR};

                check_lambda_t check_lambda_empty = [&] (eqrb_rx_state_t *rx_state, eqrb_rv_t rv, bool last)  -> bool  {
                    if (rv == eqrb_rv_rx_got_empty_frame) {
                        return false;
                    }
                    return true;
                };

                REQUIRE(rx(parts, empty_frame, sizeof(empty_frame), check_lambda_empty) == false);
            }
        }
    }
}

#include "eswb/api.h"

void replication_test(std::function<void (std::string&, std::string&)> repl_factory_init,
                      std::function<void (void)> repl_factory_deinit){

    eswb_local_init(1);

    pthread_setname_np("main");

    std::string src_bus = "src";
    std::string src_bus_full_path = "itb:/" + src_bus;
    std::string dst_bus = "dst";
    std::string dst_bus_full_path = "itb:/" + dst_bus;
    eswb_rv_t erv;
    eqrb_rv_t hrv;

    erv = eswb_create(src_bus.c_str(), eswb_inter_thread, 20);
    REQUIRE(erv == eswb_e_ok);

    erv = eswb_create(dst_bus.c_str(), eswb_inter_thread, 20);
    REQUIRE(erv == eswb_e_ok);

    eswb_topic_descr_t src_bus_td;
    eswb_topic_descr_t dst_bus_td;

    erv = eswb_topic_connect(src_bus_full_path.c_str(), &src_bus_td);
    REQUIRE(erv == eswb_e_ok);

    erv = eswb_topic_connect(dst_bus_full_path.c_str(), &dst_bus_td);
    REQUIRE(erv == eswb_e_ok);

    erv = eswb_event_queue_enable(src_bus_td, 40, 1024);
    REQUIRE(erv == eswb_e_ok);


    repl_factory_init(src_bus_full_path, dst_bus_full_path);

    //usleep(1000000);
    //eqrb_demo_stop();

    eswb_topic_descr_t publisher_td;

    periodic_call_t proclaim = [&] () mutable {
        eswb_rv_t rv;

        TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 3);

        topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, "fifo", 10);
        usr_topic_add_child(cntx, fifo_root, "cnt", tt_uint32, 0, 4, TOPIC_FLAG_MAPPED_TO_PARENT);

        rv = eswb_event_queue_order_topic(src_bus_td, src_bus.c_str(), 1 );
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_proclaim_tree_by_path(src_bus_full_path.c_str(), fifo_root, cntx->t_num, &publisher_td);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_event_queue_order_topic(src_bus_td, (src_bus + "/fifo").c_str(), 1 );
        REQUIRE(rv == eswb_e_ok);
    };

    uint32_t counter = 0;

    periodic_call_t update = [&] () mutable {
        eswb_rv_t rv = eswb_fifo_push(publisher_td, &counter);
        REQUIRE(rv == eswb_e_ok);
        counter++;
    };

    timed_caller proclaimer(proclaim, 500);
    timed_caller updater(update, 250);

    proclaimer.start_once(false);
    proclaimer.wait();

    std::this_thread::sleep_for(std::chrono::milliseconds (500));
    updater.start_loop();

    //std::this_thread::sleep_for(std::chrono::seconds (2));
    eswb_topic_descr_t replicated_fifo_td;
    erv = eswb_wait_connect_nested(dst_bus_td, "fifo/cnt", &replicated_fifo_td, 2000);
    REQUIRE(erv == eswb_e_ok);

    uint32_t cnt;
    uint32_t expected_cnt = 0;

    do {
        erv = eswb_fifo_pop(replicated_fifo_td, &cnt);
        CHECK(cnt == expected_cnt);
        expected_cnt++;
    } while((erv == eswb_e_ok) && (expected_cnt < 10));

    repl_factory_deinit();
    updater.stop();
}

void repl_factory_mem_bypass_init(std::string &src, std::string &dst) {
    eqrb_rv_t hrv;

    hrv = eqrb_demo_start(src.c_str(), dst.c_str(), 0xFFFFFFFF);
    REQUIRE(hrv == eqrb_rv_ok);
}

void repl_factory_mem_bypass_deinit() {

}

static eqrb_client_handle_t *repl_factory_ch;

void repl_factory_tcp_init(std::string &src, std::string &dst) {
    eqrb_rv_t hrv;

    hrv = eqrb_tcp_server_start(0);
    REQUIRE(hrv == eqrb_rv_ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    hrv = eqrb_tcp_client_create(&repl_factory_ch);
    REQUIRE(hrv == eqrb_rv_ok);

    char err_msg[EQRB_ERR_MSG_MAX_LEN + 1];
    hrv = eqrb_tcp_client_connect(repl_factory_ch, "127.0.0.1", src.c_str(), dst.c_str(), 0, err_msg);
    REQUIRE(hrv == eqrb_rv_ok);
}

void repl_factory_tcp_deinit() {
    eqrb_rv_t rv = eqrb_tcp_client_close(repl_factory_ch);
    REQUIRE(rv == eqrb_rv_ok);

    rv = eqrb_tcp_server_stop();
    REQUIRE(rv == eqrb_rv_ok);
}

void repl_factory_serial_init(std::string &src, std::string &dst) {
    eqrb_rv_t hrv;

    int fd1;
    int fd2;
#define SERIAL1 "/tmp/vserial1"
#define SERIAL2 "/tmp/vserial2"

    fd1 = open(SERIAL1, O_RDWR);
    fd2 = open(SERIAL2, O_RDWR);
    if (fd1 < 0 || fd2 < 0) {
        FAIL("Run virtual serial ports by socat using comment below");
    } else {
        close(fd1);
        close(fd2);
    }

    /**
     * socat -d -d pty,link=/tmp/vserial1,raw,echo=0 pty,link=/tmp/vserial2,raw,echo=0
     */


    hrv = eqrb_serial_server_start(SERIAL1, 115200, src.c_str());
    REQUIRE(hrv == eqrb_rv_ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    hrv = eqrb_serial_client_connect(SERIAL2, 115200, dst.c_str(), 512);
    REQUIRE(hrv == eqrb_rv_ok);
}

void repl_factory_serial_deinit() {

}

TEST_CASE("EQBR - mem_bypass", "[eqrb]") {
    replication_test(repl_factory_mem_bypass_init, repl_factory_mem_bypass_deinit);
}

TEST_CASE("EQBR - tcp", "[eqrb]") {
    replication_test(repl_factory_tcp_init, repl_factory_tcp_deinit);
}

TEST_CASE("EQBR - serial", "[eqrb]") {
    replication_test(repl_factory_serial_init, repl_factory_serial_deinit);
}

PseudoTopic *create_bus_and_arbitrary_hierarchy(eswb_type_t bus_type, const std::string &bus_name);

TEST_CASE("EQRB bus state sync") {

    eswb_local_init(1);
    pthread_setname_np("main");

    eswb_rv_t erv;
    eqrb_rv_t rbrv;

    std::string src_bus_name = "src";
    auto src_bus = create_bus_and_arbitrary_hierarchy(eswb_inter_thread, src_bus_name);

    std::string dst_bus = "dst";
    std::string dst_bus_full_path = "itb:/" + dst_bus;
    erv = eswb_create(dst_bus.c_str(), eswb_inter_thread, 100);
    REQUIRE(erv == eswb_e_ok);

    eswb_topic_descr_t src_bus_td;

    erv = eswb_topic_connect(src_bus->get_full_path().c_str(), &src_bus_td);
    REQUIRE(erv == eswb_e_ok);

    erv = eswb_event_queue_enable(src_bus_td, 40, 1024);
    REQUIRE(erv == eswb_e_ok);

    // going to recreate full src hierarhy inside dst bus
    erv = eswb_mkdir(dst_bus_full_path.c_str(), src_bus_name.c_str());
    REQUIRE(erv == eswb_e_ok);
    auto mounting_point = dst_bus_full_path + "/" + src_bus_name;
    eswb_topic_descr_t dst_bus_mp_td;
    erv = eswb_topic_connect(mounting_point.c_str(), &dst_bus_mp_td);
    REQUIRE(erv == eswb_e_ok);


    // starting replication facilities
    rbrv = eqrb_tcp_server_start(0);
    REQUIRE(rbrv == eqrb_rv_ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    eqrb_client_handle_t *client_handle;
    rbrv = eqrb_tcp_client_create(&client_handle);
    REQUIRE(rbrv == eqrb_rv_ok);

    char err_msg[EQRB_ERR_MSG_MAX_LEN + 1];
    rbrv = eqrb_tcp_client_connect(client_handle, "127.0.0.1", src_bus->get_full_path().c_str(), mounting_point.c_str(), 100, err_msg);
    REQUIRE(rbrv == eqrb_rv_ok);
//    INFO(err_msg);

    eswb_topic_descr_t publisher_td;


    // wait for bus sync to complete
    eswb_topic_descr_t synced_folder_td;

    // path is chosen manually as the last in generated hierarchy
    erv = eswb_wait_connect_nested(dst_bus_mp_td, "ca/db", &synced_folder_td, 4000);
    CHECK(erv == eswb_e_ok);


    ExtractedTopicsRegistry reg;
    auto fakeRoot2compare = new PseudoTopic(src_bus_name);
    fakeRoot2compare->set_path_prefix(src_bus->get_path_prefix());
    reg.add2reg(fakeRoot2compare, 1);

    int extract_cnt = 0;
    eswb_topic_id_t next2tid = 0;
    eswb_topic_descr_t td;

    eswb_rv_t rv;
    rv = eswb_subscribe(mounting_point.c_str(), &td);

    do {
        topic_extract_t e;
        rv = eswb_get_next_topic_info(td, &next2tid, &e);
        if (rv == eswb_e_ok) {
            extract_cnt++;
            reg.add2reg(e.info.name, e.info.topic_id, e.parent_id);

//            std::cout << "Extracted topic \"" << e.info.name << "\" with id "
//            << e.info.topic_id << " with parent tid " << e.parent_id << std::endl;
        }
    } while (rv == eswb_e_ok);

    CHECK(extract_cnt == src_bus->subtopics_num() - 1);
    auto extracted_bus = reg.build_hierarhy();

    src_bus->print();
    extracted_bus->print();

    bool comparison = *src_bus == *extracted_bus;
    REQUIRE(comparison == true);
}
