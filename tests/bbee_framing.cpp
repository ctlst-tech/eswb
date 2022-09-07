#include "tooling.h"

#include "../src/lib/services/sdtl/bbee_framing.h"


typedef std::function<bool (bbee_frm_rx_state_t*, bbee_frm_rv_t, bool last)> check_lambda_t;

bool rx(int parts, uint8_t *tx_frame_buf, size_t tx_frame_size, check_lambda_t &check_lambda) {
    bbee_frm_rx_state_t rx_state;
#       define RX_PAYLOAD_BUFF_SIZE 512
    uint8_t rx_payload_buf[RX_PAYLOAD_BUFF_SIZE];
    uint8_t pkt_buf[tx_frame_size];
    bbee_frm_init_state(&rx_state, rx_payload_buf, RX_PAYLOAD_BUFF_SIZE);
    size_t bytes_processed;
    bbee_frm_rv_t rv;

    size_t bytes_to_process = std::max<size_t>(1, tx_frame_size / parts);
    size_t offset = 0;

    bool loop;
    do {
        memset(pkt_buf, 0, sizeof(pkt_buf));
        auto pkt_size = std::min<size_t>(bytes_to_process, tx_frame_size - offset);
        memcpy(pkt_buf, tx_frame_buf + offset, pkt_size);

        rv = bbee_frm_rx_iteration(&rx_state, pkt_buf, pkt_size, &bytes_processed);

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
        REQUIRE( bbee_frm_compose4tx(BBEE_FRM_FRAME_BEGIN_CHAR, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                    &size) == bbee_frm_inv_code);
        REQUIRE( bbee_frm_compose4tx(BBEE_FRM_FRAME_END_CHAR, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                    &size) == bbee_frm_inv_code);
    }

    SECTION("Too small frame buffer") {
        size_t size;
        REQUIRE( bbee_frm_compose4tx(BBEE_FRM_FRAME_END_CHAR, tx_payload, sizeof(tx_payload), tx_frame_buf, sizeof(tx_payload),
                                    &size) == bbee_frm_small_buf);
    }
    
    for (unsigned char & i : tx_payload) {
        i = rand(); 
    }
    
    bbee_frm_rv_t rv;
    
    size_t tx_frame_size;
    
    bbee_frm_rx_state_t rx_state;
    #   define RX_PAYLOAD_BUFF_SIZE 512
    uint8_t rx_payload_buf[RX_PAYLOAD_BUFF_SIZE];
    bbee_frm_init_state(&rx_state, rx_payload_buf, RX_PAYLOAD_BUFF_SIZE);
    
    uint8_t expected_code= COMMAND_CODE;
    uint8_t *buf_to_compare = tx_payload;
    
    check_lambda_t check_lambda_simple = [&] (bbee_frm_rx_state_t *rx_state, bbee_frm_rv_t rv, bool last) -> bool {
        if (last) {
            //eqrb_rv_rx_got_frame
            CHECK(rv == bbee_frm_got_frame);
            CHECK(rx_state->current_command_code == expected_code);
    
            CHECK(memcmp(rx_state->payload_buffer_origin,
                         buf_to_compare,
                         rx_state->current_payload_size) == 0);
        }
        return true;
    };
    
    SECTION("Check processed bytes num") {
        uint8_t pkt[99];
        for (uint8_t i : pkt) {
            i = rand();
        }

        rv = bbee_frm_compose4tx(0x10, pkt, sizeof(pkt), tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
        REQUIRE(rv == bbee_frm_ok);

        for (auto &bts : {64, 51, 33, 13, 45}) {
            SECTION(std::string("First part size ") + std::to_string(bts) + std::string(" bytes")) {
                size_t bytes_processed;
                rv = bbee_frm_rx_iteration(&rx_state, tx_frame_buf, bts, &bytes_processed);
                REQUIRE(rv == bbee_frm_ok);
                CHECK(bytes_processed == bts);

                rv = bbee_frm_rx_iteration(&rx_state, &tx_frame_buf[bytes_processed], tx_frame_size - bts, &bytes_processed);
                REQUIRE(rv == bbee_frm_got_frame);
                CHECK(bytes_processed == tx_frame_size - bts);
            }
        }
    }

    for (auto &parts : {1, 3, 5, 10, 33}) {
        SECTION(std::string("Frame RX tests | by ") + std::to_string(parts) + std::string(" parts")) {
            SECTION("Simple") {
                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size);
                REQUIRE(rv == bbee_frm_ok);

                buf_to_compare = tx_payload;
                expected_code = COMMAND_CODE;

                auto trv = rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple);
                REQUIRE(trv == true);
            }

            SECTION("With B-s and E-s") {
                for (int i: {10, 13, 43, 57}) {
                tx_payload[i] = BBEE_FRM_FRAME_BEGIN_CHAR;
                }
                for (int i: {33, 22, 54, 81}) {
                tx_payload[i] = BBEE_FRM_FRAME_END_CHAR;
                }
                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, PAYLOAD_BUFF_SIZE, tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With B-s and E-s") {
                for (int i: {10, 13, 43, 57}) {
                    tx_payload[i] = BBEE_FRM_FRAME_BEGIN_CHAR;
                }
                for (int i: {33, 22, 54, 81}) {
                    tx_payload[i] = BBEE_FRM_FRAME_END_CHAR;
                }
                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, PAYLOAD_BUFF_SIZE, tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With All B-s") {
                for (uint8_t &i: tx_payload) {
                    i = BBEE_FRM_FRAME_BEGIN_CHAR;
                }
                rv = bbee_frm_compose4tx(0, tx_payload, PAYLOAD_BUFF_SIZE, tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_simple) == true);
            }

            SECTION("With B or/and E in crc") {
                uint32_t data;
                int have_seen_b = 0;
                int have_seen_e = 0;
                int have_seen_be = 0;

                buf_to_compare = (uint8_t *) &data;

                for (data = 0; data < UINT32_MAX >> 16; data++) {
                    rv = bbee_frm_compose4tx(COMMAND_CODE, &data, sizeof(data), tx_frame_buf, FRAME_BUF_SIZE, &tx_frame_size);
                    REQUIRE(rv == bbee_frm_ok);

                    if (tx_frame_buf[tx_frame_size - 4] == BBEE_FRM_FRAME_BEGIN_CHAR) {
                        have_seen_b = -1;
                    }
                    if (tx_frame_buf[tx_frame_size - 4] == BBEE_FRM_FRAME_END_CHAR) {
                        have_seen_e = -1;
                    }
                    if ((tx_frame_buf[tx_frame_size - 6] == BBEE_FRM_FRAME_BEGIN_CHAR) &&
                        (tx_frame_buf[tx_frame_size - 4] == BBEE_FRM_FRAME_END_CHAR)) {
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
                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, 50, tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size_1);

                REQUIRE(rv == bbee_frm_ok);

                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, 50, tx_frame_buf + 25, FRAME_BUF_SIZE - 25,
                                        &tx_frame_size);

                tx_frame_size_1 = 25;

                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size + tx_frame_size_1, check_lambda_simple) == true);
            }

            SECTION("With several frames through") {
                uint8_t tx_payload2 [128];

                int j = 0;
                for (auto &i : tx_payload2) {
                    i = j++;
                }

                int received_frames = 0;

                check_lambda_t check_lambda_2frames = [&] (bbee_frm_rx_state_t *rx_state, bbee_frm_rv_t rv, bool last)  -> bool  {
                    if (!last) {
                        if (rv == bbee_frm_got_frame) {
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
                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                    &tx_frame_size);
                REQUIRE(rv == bbee_frm_ok);

                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload2, sizeof(tx_payload2), tx_frame_buf  + tx_frame_size, FRAME_BUF_SIZE - 25,
                                    &tx_frame_size2);

                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size + tx_frame_size2, check_lambda_2frames) == true);
            }

            SECTION("With broken frame") {
                check_lambda_t check_lambda_bad_crc = [&] (bbee_frm_rx_state_t *rx_state, bbee_frm_rv_t rv, bool last)  -> bool  {
                    if (last) {
                        CHECK(rv == bbee_frm_inv_crc);
                    }
                    return true;
                };

                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload, sizeof(tx_payload), tx_frame_buf, FRAME_BUF_SIZE,
                                        &tx_frame_size);

                tx_frame_buf[8] = 55;

                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, tx_frame_buf, tx_frame_size, check_lambda_bad_crc) == true);
            }

            SECTION("With RX buffer overflow (frame with no end)") {
                uint8_t tx_payload_overflow [RX_PAYLOAD_BUFF_SIZE * 2];
                uint8_t frame_buf [RX_PAYLOAD_BUFF_SIZE * 5];

                uint8_t j = 0;
                for (auto &i : tx_payload_overflow) {
                    i = j++;
                }

                check_lambda_t check_lambda_overflow = [&] (bbee_frm_rx_state_t *rx_state, bbee_frm_rv_t rv, bool last)  -> bool  {
                    if (rv == bbee_frm_buf_overflow) {
                        CHECK(rx_state->payload_buffer_ptr - rx_state->payload_buffer_origin
                              == rx_state->payload_buffer_max_size);
                        return false;
                    }
                    return true;
                };

                rv = bbee_frm_compose4tx(COMMAND_CODE, tx_payload_overflow, sizeof(tx_payload_overflow), frame_buf, sizeof(frame_buf),
                                        &tx_frame_size);

                REQUIRE(rv == bbee_frm_ok);
                REQUIRE(rx(parts, frame_buf, tx_frame_size, check_lambda_overflow) == false);
            }

            SECTION("With empty frame") {
                uint8_t empty_frame [4] = {BBEE_FRM_FRAME_BEGIN_CHAR, BBEE_FRM_FRAME_BEGIN_CHAR, BBEE_FRM_FRAME_END_CHAR, BBEE_FRM_FRAME_END_CHAR};

                check_lambda_t check_lambda_empty = [&] (bbee_frm_rx_state_t *rx_state, bbee_frm_rv_t rv, bool last)  -> bool  {
                    if (rv == bbee_frm_got_empty_frame) {
                        return false;
                    }
                    return true;
                };

                REQUIRE(rx(parts, empty_frame, sizeof(empty_frame), check_lambda_empty) == false);
            }
        }
    }
}
