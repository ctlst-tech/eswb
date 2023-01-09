#include <catch2/catch_all.hpp>
#include <iostream>
#include <string.h>
#include <regex>
#include "tooling.h"

#include "eswb/api.h"
#include "eswb/bridge.h"
#include "tests.h"

#include "ids_map.h"

extern "C" eswb_rv_t eswb_parse_path_test_handler(const char *connection_point, eswb_type_t *t, char *bus_name, char *local_path);

static eswb_rv_t parse_path_test_wrapper(const char *cp, eswb_type_t &t, std::string &bn, std::string &lp) {
    eswb_type_t bus_type = eswb_not_defined;
    char bus_name[ESWB_BUS_NAME_MAX_LEN + 1] = "";
    char local_path[ESWB_BUS_NAME_MAX_LEN + 1] = "";

    eswb_rv_t rv = eswb_parse_path_test_handler(cp, &bus_type, bus_name, local_path);

    t = bus_type;

    lp = local_path;
    bn = bus_name;

    return rv;
}


TEST_CASE("Path parsing", "[unit]") {
    eswb_rv_t rv;

    std::string local_path;
    std::string bus_name;
    eswb_type_t bus_type;

    SECTION("No path") {
        rv = parse_path_test_wrapper("", bus_type, bus_name, local_path);
        REQUIRE(rv == eswb_e_inv_naming);
    }

    SECTION("Too long path") {
        rv = parse_path_test_wrapper("nsb:/qweqweqwe/qweqweqwe/qweqweqwe/qweqweqwe/qweqweqwe/qweqweqwee/"
                                     "qweqweqwe/qweqweqwe/qweqweqwe/qweqweqwe/qweqweqwee", bus_type, bus_name, local_path);
        REQUIRE(rv == eswb_e_path_too_long);
    }

    SECTION("Bus prefix check") {
#   define BN "bus"
#   define LP BN "/topic"

#   define CHECK_OUTPUT()              \
            CHECK(rv == eswb_e_ok);  \
            CHECK(bus_name == BN);   \
            CHECK(local_path == LP);

        SECTION("NSB") {
            rv = parse_path_test_wrapper("nsb:/" LP, bus_type, bus_name, local_path);
            CHECK_OUTPUT();
            CHECK(bus_type == eswb_non_synced);
        }
        SECTION("ITB") {
            rv = parse_path_test_wrapper("itb:/" LP, bus_type, bus_name, local_path);
            CHECK_OUTPUT();
            CHECK(bus_type == eswb_inter_thread);
        }
        SECTION("ITP") {
            rv = parse_path_test_wrapper("ipb:/" LP, bus_type, bus_name, local_path);
            CHECK_OUTPUT();
            CHECK(bus_type == eswb_inter_process);
        }
        SECTION("Not valid") {
            rv = parse_path_test_wrapper("iii:/" LP, bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_inv_bus_spec);
        }
        SECTION("No prefix") {
            rv = parse_path_test_wrapper(LP, bus_type, bus_name, local_path);
            CHECK_OUTPUT();
            CHECK(bus_type == eswb_not_defined);
        }
    }

    SECTION("Variety symbols in paths") {
        SECTION("Double colon") {
            rv = parse_path_test_wrapper("ipb::/" LP, bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_inv_naming);
        }
        SECTION("Start with colon") {
            rv = parse_path_test_wrapper(":/" LP, bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_inv_naming);
        }
        SECTION("Start with slash") {
            rv = parse_path_test_wrapper("/" LP, bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_ok);
            REQUIRE(local_path == local_path);
        }
        SECTION("Colon after slash") {
            rv = parse_path_test_wrapper("ipb/ipb:/" LP, bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_inv_naming);
        }

        SECTION("Non alphanum") {
            rv = parse_path_test_wrapper("ipb:/bus/abc?def/", bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_inv_naming);
            rv = parse_path_test_wrapper("ipb:/bus/*)(?/", bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_inv_naming);
        }

        SECTION("Alphanum") {
            rv = parse_path_test_wrapper("ipb:/bus/123123/", bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_ok);
            rv = parse_path_test_wrapper("ipb:/bus/asdasdasd/", bus_type, bus_name, local_path);
            REQUIRE(rv == eswb_e_ok);
        }
    }
}

TEST_CASE("Basic Operations") {

    eswb_local_init(1);

    std::string bus_name("bus");
    std::string bus_path = "itb:/" + bus_name;

    eswb_rv_t rv = eswb_create(bus_name.c_str(), eswb_inter_thread, 20);
    REQUIRE(rv == eswb_e_ok);

    SECTION("Topic connection") {
        eswb_topic_descr_t td;

        SECTION("By full path") {
            rv = eswb_connect(bus_path.c_str(), &td);
            REQUIRE(rv == eswb_e_ok);
        }

        SECTION("By path without prefix") {
            rv = eswb_connect(bus_name.c_str(), &td);
            REQUIRE(rv == eswb_e_ok);
        }

        SECTION("By path with duplicated slashes") {
            std::string slashy_bus_path = std::regex_replace( bus_path, std::regex("/"), "//" );
            rv = eswb_connect(bus_path.c_str(), &td);
            REQUIRE(rv == eswb_e_ok);
        }

        SECTION("By path with trailing slash") {
            bus_path += "/";
            rv = eswb_connect(bus_path.c_str(), &td);
            REQUIRE(rv == eswb_e_ok);
        }
    }

    SECTION("Bus deletion") {
        SECTION("Delete bus by path") {
            rv = eswb_delete(bus_path.c_str());
            REQUIRE(rv == eswb_e_ok);
        }

        eswb_topic_descr_t td;

        SECTION("Delete bus by TD") {

            rv = eswb_connect(bus_path.c_str(), &td);
            REQUIRE(rv == eswb_e_ok);

            rv = eswb_delete_by_td(td);
            REQUIRE(rv == eswb_e_ok);
        }

        rv = eswb_connect(bus_path.c_str(), &td);
        REQUIRE(rv == eswb_e_bus_not_exist);
    }

    SECTION("Topic try get update") {
        TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 2);
        std::string var_name = "var";
        topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, var_name.c_str(), tt_double, sizeof(double));
        REQUIRE(rt != NULL);

        eswb_topic_descr_t publish_td;
        rv = eswb_proclaim_tree_by_path(bus_path.c_str(), rt, cntx->t_num, &publish_td);
        REQUIRE(rv == eswb_e_ok);

        eswb_topic_descr_t subscr_td;
        auto subs_path = bus_path + "/" + var_name;
        rv = eswb_connect(subs_path.c_str(), &subscr_td);
        REQUIRE(rv == eswb_e_ok);


        SECTION("Non updated topic") {
            double dummy_data;
            rv = eswb_try_get_update(subscr_td, &dummy_data);
            REQUIRE(rv == eswb_e_no_update);
        }

        uint32_t update_num = GENERATE(1, 2, 10, 128);

        SECTION("Once try get update for " + std::to_string(update_num) + " times updated topic") {
            double dummy_data_write = 123.456;
            double dummy_data_read = 0.321;

            for (int i = 0; i < update_num; i++) {
                rv = eswb_update_topic(publish_td, &dummy_data_write);
                REQUIRE(rv == eswb_e_ok);
            }

            rv = eswb_try_get_update(subscr_td, &dummy_data_read);
            REQUIRE(rv == eswb_e_ok);
            REQUIRE(dummy_data_write == dummy_data_read);

            rv = eswb_try_get_update(subscr_td, &dummy_data_read);
            REQUIRE(rv == eswb_e_no_update);
            REQUIRE(dummy_data_write == dummy_data_read);
        }

        SECTION("Connect and once try get update after " + std::to_string(update_num) + " times updated topic") {
            double dummy_data_write = 123.456;
            double dummy_data_read = 0.321;

            for (int i = 0; i < update_num; i++) {
                rv = eswb_update_topic(publish_td, &dummy_data_write);
                REQUIRE(rv == eswb_e_ok);
            }

            eswb_topic_descr_t subscr_td2;
            rv = eswb_connect(subs_path.c_str(), &subscr_td2);
            REQUIRE(rv == eswb_e_ok);

            rv = eswb_try_get_update(subscr_td2, &dummy_data_read);
            REQUIRE(rv == eswb_e_ok);
            REQUIRE(dummy_data_write == dummy_data_read);

            rv = eswb_try_get_update(subscr_td2, &dummy_data_read);
            REQUIRE(rv == eswb_e_no_update);
            REQUIRE(dummy_data_write == dummy_data_read);
        }
    }
}


TEST_CASE("Timed out eswb_get_update wait") {

    eswb_local_init(1);

    std::string bus_name("bus");
    std::string bus_path = "itb:/" + bus_name;

    eswb_rv_t rv = eswb_create(bus_name.c_str(), eswb_inter_thread, 20);
    REQUIRE(rv == eswb_e_ok);

    // proclaim topic
    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
    struct structure {
        double a;
        double b;
    } st = {0};
    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, st, "st");
    usr_topic_add_struct_child(cntx, rt, struct structure, a, "a", tt_double);
    usr_topic_add_struct_child(cntx, rt, struct structure, b, "b", tt_double);

    eswb_topic_descr_t publish_td;
    rv = eswb_proclaim_tree_by_path(bus_path.c_str(), rt, cntx->t_num, &publish_td);
    REQUIRE(rv == eswb_e_ok);

    st.a = 0.1;
    st.b = 0.2;
    rv = eswb_update_topic(publish_td, &st);
    REQUIRE(rv == eswb_e_ok);

    eswb_topic_descr_t subs_td;
    rv = eswb_connect((bus_path + "/st/a").c_str(), &subs_td);
    REQUIRE(rv == eswb_e_ok);

    uint32_t timeout = GENERATE(1, 5, 10, 20, 200, 500, 1000, 100000, 500000, 1000000, 1500000, 250);
    structure st_rcv = {0};

    SECTION("Simple " + std::to_string(timeout) + " timeout") {
        rv = eswb_arm_timeout(subs_td, timeout);
        REQUIRE(rv == eswb_e_ok);
        struct timespec t0, t1;
        clock_gettime(CLOCK_REALTIME, &t0);
        rv = eswb_get_update(subs_td, &st_rcv);
        clock_gettime(CLOCK_REALTIME, &t1);
        REQUIRE(rv == eswb_e_timedout);
#       define DELTA_T(a,b) ( ( (a)->tv_sec - (b)->tv_sec ) + ( (double) ( (a)->tv_nsec - (b)->tv_nsec ) / 1000000000 ) )
        double dt = DELTA_T(&t1, &t0);

        std::cout << "Timed out call with " << std::to_string(timeout)
                    << " parameter gives " << std::to_string(dt * 1000000 - timeout) << " uSec measured error" << std::endl;
        // 300 micro sec allowed margin
        // double to_usec = timeout / 1000000.0;
        // CHECK(fabs ((dt - to_usec) / to_usec) < 0.02);
    }
}

TEST_CASE("Timed out eswb_fifo_pop wait") {
    eswb_rv_t rv;

    eswb_local_init(1);

    std::string bus_path = "bus";

    rv = eswb_create(bus_path.c_str(), eswb_inter_thread, 20);
    REQUIRE(rv == eswb_e_ok);

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
#   define FIFO_SIZE 10
    topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, "fifo", FIFO_SIZE);
    usr_topic_add_child(cntx, fifo_root, "elem", tt_uint32,
                        0, 4, TOPIC_PROCLAIMING_FLAG_MAPPED_TO_PARENT);

    eswb_topic_descr_t snd_td;
    rv = eswb_proclaim_tree_by_path(bus_path.c_str(), fifo_root, cntx->t_num, &snd_td);
    REQUIRE(rv == eswb_e_ok);

    eswb_topic_descr_t rcv_td;
    rv = eswb_fifo_subscribe((bus_path + "/fifo/elem").c_str(), &rcv_td);
    REQUIRE(rv == eswb_e_ok);

    uint32_t timeout = GENERATE(1, 5, 10, 20, 200, 500, 1000, 100000, 500000);

    SECTION("Simple " + std::to_string(timeout) + " timeout") {
        rv = eswb_arm_timeout(rcv_td, timeout);
        REQUIRE(rv == eswb_e_ok);
        uint32_t elem;
        rv = eswb_fifo_pop(rcv_td, &elem);
        REQUIRE(rv == eswb_e_timedout);
    }
}

TEST_CASE("FIFO | nsb", "[unit]") {

    eswb_local_init(1);

    std::string bus_name("bus");
    std::string bus_path = "nsb:/" + bus_name;

    eswb_rv_t rv = eswb_create(bus_name.c_str(), eswb_non_synced, 20);
    REQUIRE(rv == eswb_e_ok);

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
#   define FIFO_SIZE 10
    topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, "fifo", FIFO_SIZE);
    usr_topic_add_child(cntx, fifo_root, "elem", tt_uint32,
                        0, 4, TOPIC_PROCLAIMING_FLAG_MAPPED_TO_PARENT);

    eswb_topic_descr_t snd_td;
    rv = eswb_proclaim_tree_by_path(bus_path.c_str(), fifo_root, cntx->t_num, &snd_td);
    REQUIRE(rv == eswb_e_ok);

    eswb_topic_descr_t rcv_td;
    rv = eswb_fifo_subscribe( (bus_path + "/fifo/elem").c_str(), &rcv_td);
    REQUIRE(rv == eswb_e_ok);

    SECTION("Single push and pop") {
        uint32_t data_snd = 123;
        uint32_t data_rcv;
        rv = eswb_fifo_push(snd_td, &data_snd);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_fifo_pop(rcv_td, &data_rcv);
        REQUIRE(rv == eswb_e_ok);
        REQUIRE(data_snd == data_rcv);
    }

    SECTION(std::string("push and pop N times; fifo size = ") + std::to_string(FIFO_SIZE) ) {
        for (int intrctn_num = 6; intrctn_num < FIFO_SIZE * 2 + 4; intrctn_num += intrctn_num < FIFO_SIZE + 1 ? 1 : 2 ) {
            SECTION(std::string("Push and pop ") + std::to_string(intrctn_num)) {
                for (int i = 0; i < intrctn_num; i++) {
                    uint32_t data_snd = intrctn_num + i;
                    uint32_t data_rcv;
                    rv = eswb_fifo_push(snd_td, &data_snd);
                    REQUIRE(rv == eswb_e_ok);

                    rv = eswb_fifo_pop(rcv_td, &data_rcv);
                    REQUIRE(rv == eswb_e_ok);
                    REQUIRE(data_snd == data_rcv);
                }
            }
        }
    }

    SECTION("Underrun detection") {

#       define OVERRUN_MARGIN 4
        for (int i = 0; i < FIFO_SIZE + OVERRUN_MARGIN; i++) {
            uint32_t data_snd = i;
            rv = eswb_fifo_push(snd_td, &data_snd);
            REQUIRE(rv == eswb_e_ok);
        }

        uint32_t data_rcv;
        rv = eswb_fifo_pop(rcv_td, &data_rcv);
        CHECK(rv == eswb_e_fifo_rcvr_underrun);
        CHECK(data_rcv == OVERRUN_MARGIN);
        rv = eswb_fifo_pop(rcv_td, &data_rcv);
        CHECK(rv == eswb_e_ok);
        CHECK(data_rcv == OVERRUN_MARGIN + 1);
    }

    SECTION("Push N times, Pop M; N > M") {

        for (int pushes = 11; pushes < FIFO_SIZE * 2 + 4; pushes += pushes < FIFO_SIZE + 4 ? 1 : 2) {
            SECTION(std::string("Push and pop ") + std::to_string(pushes)) {
                for (int i = 0; i < pushes; i++) {
                    uint32_t data_snd = i;
                    rv = eswb_fifo_push(snd_td, &data_snd);
                    REQUIRE(rv == eswb_e_ok);
                }

                int pops = 0;
                int underruns = 0;

                do {
                    uint32_t data_rcv;
                    rv = eswb_fifo_pop(rcv_td, &data_rcv);
                    switch (rv) {
                        case eswb_e_fifo_rcvr_underrun:
                            underruns++;

                        case eswb_e_ok:
                            CHECK(data_rcv == pops + pushes - FIFO_SIZE);
                            pops++;
                            break;

                        case eswb_e_no_update:
                            break;

                        default:
                            FAIL(rv);
                            break;
                    }

                    REQUIRE(pops <= FIFO_SIZE);
                    REQUIRE(underruns <= 1);
                } while (rv != eswb_e_no_update);
                CHECK(pops == FIFO_SIZE);
            }
        }
    }

    SECTION("Fifo state's lap counter overflow") {
#define INITIAL_LAG 4

        int j;
        for (int i = 0; i < INITIAL_LAG; i++) {
            uint32_t data_snd = j++;
            rv = eswb_fifo_push(snd_td, &data_snd);
            REQUIRE(rv == eswb_e_ok);
        }

        for (int k = 0; k < ESWB_FIFO_INDEX_OVERFLOW * 1.5; k++) {
            for (int i = 0; i < FIFO_SIZE; i++) {
                uint32_t data_snd = j++;
                rv = eswb_fifo_push(snd_td, &data_snd);
                REQUIRE(rv == eswb_e_ok);

                uint32_t data_rcv;
                rv = eswb_fifo_pop(rcv_td, &data_rcv);
                REQUIRE(rv == eswb_e_ok);
                REQUIRE(data_rcv == data_snd - INITIAL_LAG);
            }
        }
    }

    SECTION("FIFO flush" ) {
        int num_op_pushs = GENERATE(1, 4, 8, 12);

        SECTION(std::to_string(num_op_pushs) + std::string(" pushes")) {
            for (int push = 0; push < num_op_pushs; push++) {
                uint32_t data_snd = push;
                uint32_t data_rcv;
                rv = eswb_fifo_push(snd_td, &data_snd);
                REQUIRE(rv == eswb_e_ok);

                rv = eswb_fifo_flush(rcv_td);
                REQUIRE(rv == eswb_e_ok);

                rv = eswb_fifo_pop(rcv_td, &data_rcv);
                REQUIRE(rv == eswb_e_no_update);

                // push one more time  and check that it is still working
                rv = eswb_fifo_push(snd_td, &data_snd);
                REQUIRE(rv == eswb_e_ok);

                rv = eswb_fifo_pop(rcv_td, &data_rcv);
                REQUIRE(rv == eswb_e_ok);
                REQUIRE(data_rcv == data_snd);
            }
        }
    }
}


#include "topic_mem.h"
#include "registry.h"
#include "local_buses.h"

extern "C" eswb_rv_t local_bus_create_event_queue(eswb_bus_handle_t *bh, eswb_size_t events_num, eswb_size_t data_buf_size);
extern "C" eswb_rv_t local_bus_create(const char *bus_name, local_bus_type_t type, eswb_size_t max_topics);
extern "C" eswb_rv_t local_event_queue_update(eswb_bus_handle_t *bh, event_queue_record_t *record);

void setup_queue_record(event_queue_record_t *r, event_queue_record_type_t t, void *d, ssize_t s) {
    r->size = s;
    r->topic_id = 777;
    r->ch_mask = 0xFFFFFFFF;
    r->type = t;
    r->data = d;
}

int compare_event_queue_transfer(event_queue_record_t *evqr, event_queue_transfer_t *evqt) {

    if (evqt->type == evqr->type) {
        if (evqt->topic_id == evqr->topic_id) {
            if (evqt->size == evqr->size) {
                int rv = memcmp(evqr->data, EVENT_QUEUE_TRANSFER_DATA(evqt), evqt->size);
                if (rv != 0) {
                    uint8_t bufR[200];
                    uint8_t bufT[200];
                    for(int i = 0; i < evqt->size; i++) {
                        bufR[i] = ((uint8_t*)evqr->data)[i];
                        bufT[i] = EVENT_QUEUE_TRANSFER_DATA(evqt)[i];
                    }
                    return rv;
                }
                return 0;
            }
        }
    }

    return -1;
}

TEST_CASE("Event queue io | local bus level", "[unit]") {
#   define EVQ_TEST_BUS_NAME "evqtb"
#   define EVQ_TEST_BUS_MAX_TOPICS 10
#   define EVQ_QUEUE_SIZE 10
#   define EVQ_DATA_BUF_SIZE 1000

    eswb_rv_t rv;
    event_queue_transfer_t *eqt = NULL;

    eswb_local_init(1);

    eqt = (event_queue_transfer_t *) calloc(1, EVQ_DATA_BUF_SIZE + sizeof(event_queue_transfer_t));

    rv = local_bus_create(EVQ_TEST_BUS_NAME, local_bus_t_nonsynced, EVQ_TEST_BUS_MAX_TOPICS);
    REQUIRE(rv == eswb_e_ok);

    eswb_bus_handle_t *bh;
    rv = local_lookup_nsb(EVQ_TEST_BUS_NAME, &bh);
    REQUIRE(rv == eswb_e_ok);

    rv = local_bus_create_event_queue(bh, EVQ_QUEUE_SIZE, EVQ_DATA_BUF_SIZE);
    REQUIRE(rv == eswb_e_ok);

    eswb_topic_descr_t event_queue_td;
    std::string evq_path = std::string(EVQ_TEST_BUS_NAME) + std::string("/") + std::string(BUS_EVENT_QUEUE_NAME) + std::string("/event"); // DRY is CRYing
    rv = local_bus_connect(bh, evq_path.c_str(), &event_queue_td);
    REQUIRE(rv == eswb_e_ok);


    char bulk_data[255];
    for (int i = 0; i < sizeof(bulk_data); i++) {
        bulk_data[i] = i;
    }

    event_queue_record_t evqr;


    SECTION("Pushing and checking events") {

        SECTION("Single usual") {
            double test_val = 123.0;
            setup_queue_record(&evqr, eqr_topic_update, &test_val, sizeof(test_val));

            rv = local_event_queue_update(bh, &evqr);
            REQUIRE(rv == eswb_e_ok);
            rv = local_fifo_pop(event_queue_td, eqt, 1);
            REQUIRE(rv == eswb_e_ok);

            REQUIRE(compare_event_queue_transfer(&evqr, eqt) == 0);
        }

        SECTION("Single bulk") {
            setup_queue_record(&evqr, eqr_topic_proclaim, bulk_data, sizeof(bulk_data));

            rv = local_event_queue_update(bh, &evqr);
            REQUIRE(rv == eswb_e_ok);
            rv = local_fifo_pop(event_queue_td, eqt, 1);
            REQUIRE(rv == eswb_e_ok);

            REQUIRE(compare_event_queue_transfer(&evqr, eqt) == 0);
        }

        SECTION("Greater than event queue buffer size") {
            setup_queue_record(&evqr, eqr_topic_proclaim, bulk_data, EVQ_DATA_BUF_SIZE + 1);
            rv = local_event_queue_update(bh, &evqr);
            REQUIRE(rv == eswb_e_ev_queue_payload_too_large);
        }

#       define OVERWRITING_MARGIN 4
        for (int pushes_num = 8; pushes_num <= EVQ_QUEUE_SIZE + OVERWRITING_MARGIN; pushes_num++) {
            int pops = 0;
            SECTION(std::string("Event FIFO overwriting ") + std::to_string(pushes_num) + "/" + std::to_string(EVQ_QUEUE_SIZE)) {
#           define NON_OVERWRITING_PAYLOAD_SIZE (( EVQ_DATA_BUF_SIZE / EVQ_QUEUE_SIZE ) - 1)
                setup_queue_record(&evqr, eqr_topic_update, bulk_data, NON_OVERWRITING_PAYLOAD_SIZE);

                for (int i = 0; i < pushes_num; i++) { //EVQ_QUEUE_SIZE + OVERWRITING_MARGIN
                    bulk_data[0] = i; // contains packet counter
                    rv = local_event_queue_update(bh, &evqr);
                    REQUIRE(rv == eswb_e_ok);
                }

                do {
                    rv = local_fifo_pop(event_queue_td, eqt, 1);
                    if ((rv == eswb_e_ok) || (rv == eswb_e_fifo_rcvr_underrun)) {
                        if (pushes_num > EVQ_QUEUE_SIZE) {
                            CHECK(EVENT_QUEUE_TRANSFER_DATA(eqt)[0] == pops + pushes_num - EVQ_QUEUE_SIZE);
                        } else {
                            CHECK(EVENT_QUEUE_TRANSFER_DATA(eqt)[0] == pops);
                        }
                        EVENT_QUEUE_TRANSFER_DATA(eqt)[0] = ((uint8_t *) evqr.data)[0] = 0;// resetting expected difference
                        CHECK(compare_event_queue_transfer(&evqr, eqt) == 0);
                        pops++;
                        rv = eswb_e_ok;
                    }
                    //REQUIRE(rv == eswb_e_ok);
                } while (rv != eswb_e_no_update);
                int expected_pops = pushes_num > EVQ_QUEUE_SIZE ? EVQ_QUEUE_SIZE : pushes_num;
                CHECK(pops == expected_pops);
            }
        }

        SECTION("Data buffer overwriting") {
#           define OVERWRITING_DATA_SIZE sizeof(bulk_data)
            setup_queue_record(&evqr, eqr_topic_update, bulk_data, OVERWRITING_DATA_SIZE);

            for (int i = 0; i < 6; i++) {
                bulk_data[0]++;
                rv = local_event_queue_update(bh, &evqr);
                REQUIRE(rv == eswb_e_ok);
            }

            int pops = 0;
            do {
                rv = local_fifo_pop(event_queue_td, eqt, 1);
                if ((rv == eswb_e_ok) || (rv == eswb_e_fifo_rcvr_underrun)) {
                    EVENT_QUEUE_TRANSFER_DATA(eqt)[0] = ((uint8_t *) evqr.data)[0] = 0;// resetting expected difference
                    CHECK(compare_event_queue_transfer(&evqr, eqt) == 0);
                    pops++;
                    rv = eswb_e_ok;
                }
                //REQUIRE(rv == eswb_e_ok);
            } while (rv != eswb_e_no_update);

            CHECK(pops == EVQ_DATA_BUF_SIZE / OVERWRITING_DATA_SIZE);
        }

        SECTION("Event queue stream demux") {
            REQUIRE(1 == eswb_e_ok);
        }
    }


    if (eqt != NULL) {
        free(eqt);
    }
}

#include "eswb/event_queue.h"
#include "eswb/topic_proclaiming_tree.h"

TEST_CASE("Basic event queue and replication") {

    // create source bus
    std::string src_bus_name("src_bus");
    eswb_rv_t rv = eswb_create(src_bus_name.c_str(), eswb_non_synced, 20);
    REQUIRE(rv == eswb_e_ok);

    std::string src_bus_path = "nsb:/" + src_bus_name;
    eswb_topic_descr_t src_td;
    rv = eswb_connect(src_bus_path.c_str(), &src_td);
    REQUIRE(rv == eswb_e_ok);

    // create dst bus
    std::string dst_bus_name("dst_bus");
    rv = eswb_create(dst_bus_name.c_str(), eswb_non_synced, 20);
    REQUIRE(rv == eswb_e_ok);

    std::string dst_bus_path = "nsb:/" + dst_bus_name;
    eswb_topic_descr_t dst_td;
    rv = eswb_connect(dst_bus_path.c_str(), &dst_td);
    REQUIRE(rv == eswb_e_ok);

    // enable event queue
    rv = eswb_event_queue_enable(src_td, 20, 2000);
    REQUIRE(rv == eswb_e_ok);

    // order topics root
#   define TELEMETRY_CHANNEL 1
    rv = eswb_event_queue_order_topic(src_td, src_bus_name.c_str(), TELEMETRY_CHANNEL);
    REQUIRE(rv == eswb_e_ok);

    // connect to event queue
    eswb_topic_descr_t ev_q_td;
    rv = eswb_event_queue_subscribe(src_bus_path.c_str(), &ev_q_td);
    REQUIRE(rv == eswb_e_ok);

    // proclaim topic
    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
    struct structure {
        double a;
        double b;
        double c;
    } st = {0};
    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, st, "st");
    usr_topic_add_struct_child(cntx, rt, struct structure, a, "a", tt_double);
    usr_topic_add_struct_child(cntx, rt, struct structure, b, "b", tt_double);
    usr_topic_add_struct_child(cntx, rt, struct structure, c, "c", tt_double);

    eswb_topic_descr_t src_struct_td;
    rv = eswb_proclaim_tree_by_path(src_bus_path.c_str(), rt, cntx->t_num, &src_struct_td);
    REQUIRE(rv == eswb_e_ok);

    // TODO: create a convention for subtopics to inherit subscription mask (e.g. bits 16-32 are inheritable)
    rv = eswb_event_queue_order_topic(src_td, (src_bus_name + "/st").c_str(), TELEMETRY_CHANNEL);
    REQUIRE(rv == eswb_e_ok);


#define A_VAL 1.0
#define B_VAL 1.33
#define C_VAL 2.0
    st.a = A_VAL;
    st.b = B_VAL;
    st.c = C_VAL;

    rv = eswb_update_topic(src_struct_td, &st);
    REQUIRE(rv == eswb_e_ok);

    st.a *= 2;
    st.b *= 2;
    st.c *= 2;

    rv = eswb_update_topic(src_struct_td, &st);
    REQUIRE(rv == eswb_e_ok);

    struct structure receiver_st = {0};

    topic_id_map_t *tid_map;
    rv = map_alloc(&tid_map, 10);
    REQUIRE(rv == eswb_e_ok);

    // pop queue, post to replication
    uint8_t event_buf [512];
    event_queue_transfer_t *event = (event_queue_transfer_t*)event_buf;

    rv = eswb_event_queue_pop(ev_q_td, event);
    REQUIRE(rv == eswb_e_ok);
    rv = eswb_event_queue_replicate(dst_td, tid_map, event);
    REQUIRE(rv == eswb_e_ok);

    eswb_topic_descr_t td_a;
    eswb_topic_descr_t td_b;
    eswb_topic_descr_t td_c;
    rv = eswb_connect((dst_bus_path + "/st/a").c_str(), &td_a);
    REQUIRE(rv == eswb_e_ok);
    rv = eswb_connect((dst_bus_path + "/st/b").c_str(), &td_b);
    REQUIRE(rv == eswb_e_ok);
    rv = eswb_connect((dst_bus_path + "/st/c").c_str(), &td_c);
    REQUIRE(rv == eswb_e_ok);


    rv = eswb_event_queue_pop(ev_q_td, event);
    REQUIRE(rv == eswb_e_ok);
    rv = eswb_event_queue_replicate(dst_td, tid_map, event);
    REQUIRE(rv == eswb_e_ok);

    double value;
    rv = eswb_get_update(td_a, &value);
    REQUIRE(rv == eswb_e_ok);
    REQUIRE(value == A_VAL);
    rv = eswb_get_update(td_b, &value);
    REQUIRE(rv == eswb_e_ok);
    REQUIRE(value == B_VAL);
    rv = eswb_get_update(td_c, &value);
    REQUIRE(rv == eswb_e_ok);
    REQUIRE(value == C_VAL);

    rv = eswb_event_queue_pop(ev_q_td, event);
    REQUIRE(rv == eswb_e_ok);
    rv = eswb_event_queue_replicate(dst_td, tid_map, event);
    REQUIRE(rv == eswb_e_ok);

    rv = eswb_get_update(td_a, &value);
    REQUIRE(rv == eswb_e_ok);
    REQUIRE(value == A_VAL * 2);
    rv = eswb_get_update(td_b, &value);
    REQUIRE(rv == eswb_e_ok);
    REQUIRE(value == B_VAL * 2);
    rv = eswb_get_update(td_c, &value);
    REQUIRE(rv == eswb_e_ok);
    REQUIRE(value == C_VAL * 2);

    // compare topics values
    // reset busses context
    eswb_local_init(1);
}

TEST_CASE("Retrieve tree struct") {
    eswb_local_init(1);

    std::string bus_name("bus");

    PseudoTopic *bus = create_bus_and_arbitrary_hierarchy(eswb_non_synced, bus_name);

    ExtractedTopicsRegistry reg;
    auto regRoot2compare = new PseudoTopic(bus_name);
    regRoot2compare->set_path_prefix(bus->get_path_prefix());
    reg.add2reg(regRoot2compare);

    int extract_cnt = 0;
    eswb_topic_id_t next2tid = 0;
    eswb_topic_descr_t td;

    eswb_rv_t rv;
    rv = eswb_connect(bus->get_full_path().c_str(), &td);

    do {
        topic_extract_t e;

        rv = eswb_get_next_topic_info(td, &next2tid, &e);
        if (rv == eswb_e_ok) {
            extract_cnt++;
            reg.add2reg(e.info.name, e.info.topic_id, e.parent_id);
        }
    } while (rv == eswb_e_ok);

    REQUIRE(extract_cnt == bus->subtopics_num() - 1);

    auto extracted_bus = reg.build_hierarhy();

    bus->print();
//    extracted_bus->print();

    bool comparison = *bus == *extracted_bus;
    REQUIRE(comparison == true);

//    namegen test
//    for (int i = 0; i < 200; i++) {
//        std::cout << gen_name() << std::endl;
//    }

}

std::string add_path(std::string s1, std::string s2) {
    return s1 + "/" + s2;
}

typedef struct test_structure {
    double a;
    double b;
    double c;
} test_structure_t;

TEST_CASE("ESWB bridge") {
    eswb_local_init(1);

    eswb_rv_t rv;

    std::string src_bus_name = "src_bus";
    rv = eswb_create(src_bus_name.c_str(), eswb_inter_thread, 256);
    REQUIRE(rv == eswb_e_ok);

    test_structure_t st = {};

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);

    std::string struct1_name = "struct1";
    eswb_topic_descr_t struct1_td;
    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, st, struct1_name.c_str());
    usr_topic_add_struct_child(cntx, rt, test_structure_t, a, "a", tt_double);
    usr_topic_add_struct_child(cntx, rt, test_structure_t, b, "b", tt_double);
    usr_topic_add_struct_child(cntx, rt, test_structure_t, c, "c", tt_double);

    rv = eswb_proclaim_tree_by_path(src_bus_name.c_str(), rt, cntx->t_num, &struct1_td);
    REQUIRE(rv == eswb_e_ok);


    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);
    std::string struct2_name = "struct2";
    eswb_topic_descr_t struct2_td;
    topic_proclaiming_tree_t *rt2 = usr_topic_set_struct(cntx, st, struct2_name.c_str());
    usr_topic_add_struct_child(cntx, rt, test_structure_t, a, "e", tt_double);
    usr_topic_add_struct_child(cntx, rt, test_structure_t, b, "f", tt_double);
    usr_topic_add_struct_child(cntx, rt, test_structure_t, c, "g", tt_double);

    rv = eswb_proclaim_tree_by_path(src_bus_name.c_str(), rt2, cntx->t_num, &struct2_td);
    REQUIRE(rv == eswb_e_ok);

    std::string scalar1_name = "scalar1";
    eswb_topic_descr_t scalar1_td;
    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);
    topic_proclaiming_tree_t *rt3 = usr_topic_set_root(cntx, scalar1_name.c_str(), tt_double, sizeof(double));
    rv = eswb_proclaim_tree_by_path(src_bus_name.c_str(), rt3, cntx->t_num, &scalar1_td);
    REQUIRE(rv == eswb_e_ok);

    std::string scalar2_name = "scalar2";
    eswb_topic_descr_t scalar2_td;
    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);
    topic_proclaiming_tree_t *rt4 = usr_topic_set_root(cntx, scalar2_name.c_str(), tt_double, sizeof(double));
    rv = eswb_proclaim_tree_by_path(src_bus_name.c_str(), rt4, cntx->t_num, &scalar2_td);
    REQUIRE(rv == eswb_e_ok);


    // destination bus
    std::string dst_bus_name = "dst_bus";
    rv = eswb_create(dst_bus_name.c_str(), eswb_inter_thread, 256);
    REQUIRE(rv == eswb_e_ok);

    eswb_bridge_t *br;
    eswb_topic_descr_t dst_td;
    eswb_topic_descr_t dst2_td;

    SECTION("Scalar via path") {
        rv = eswb_bridge_create("scalar", 1, &br);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, scalar1_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);
        std::string scalar1_at_dst_path = add_path(dst_bus_name, scalar1_name);
        rv = eswb_bridge_connect(br, 0, scalar1_at_dst_path.c_str());
        REQUIRE(rv == eswb_e_ok);
        double value_src = 123.456;
        rv = eswb_update_topic(scalar1_td, &value_src);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_update(br);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_connect(add_path(dst_bus_name, scalar1_name).c_str(), &dst_td);
        REQUIRE(rv == eswb_e_ok);
        double value_dst = 0;
        rv = eswb_read(dst_td, &value_dst);
        REQUIRE(value_dst == value_src);
    }


    SECTION("Scalar via mouting topic descriptor") {
        int i = 0;
        // iterate through combination of explicitly specifying bridging destination name
        for (const char *p : {"", "scalar1_explicit_name"}) {
            const char *dst_name = strlen(p) == 0 ? NULL : p;
            std::string section_name = "dst_name == " + (dst_name == NULL ? "NULL" : std::string(dst_name));
            SECTION(section_name) {
                rv = eswb_bridge_create("scalar", 1, &br);
                REQUIRE(rv == eswb_e_ok);

                eswb_topic_descr_t src_mtd;
                rv = eswb_connect(src_bus_name.c_str(), &src_mtd);

                eswb_topic_descr_t dst_mtd;
                rv = eswb_connect(dst_bus_name.c_str(), &dst_mtd);
                REQUIRE(rv == eswb_e_ok);

                rv = eswb_bridge_add_topic(br, src_mtd, scalar1_name.c_str(), dst_name);
                REQUIRE(rv == eswb_e_ok);

                std::string scalar1_at_dst_path = add_path(dst_bus_name, scalar1_name);
                rv = eswb_bridge_connect(br, dst_mtd, scalar1_name.c_str());
                REQUIRE(rv == eswb_e_ok);

                double value_src = dst_name == NULL ? 123.456 : 654.321;

                rv = eswb_update_topic(scalar1_td, &value_src);
                REQUIRE(rv == eswb_e_ok);
                rv = eswb_bridge_update(br);
                REQUIRE(rv == eswb_e_ok);

                rv = eswb_connect(add_path(dst_bus_name, dst_name == NULL ? scalar1_name : dst_name).c_str(), &dst_td);
                REQUIRE(rv == eswb_e_ok);
                double value_dst = 0;
                rv = eswb_read(dst_td, &value_dst);
                REQUIRE(value_dst == value_src);
            }
        }
    }


    SECTION("Two scalars") {
        std::string bridge_name = "two_scalars";
        rv = eswb_bridge_create(bridge_name.c_str(), 2, &br);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, scalar1_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, scalar2_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_bridge_connect(br, 0, dst_bus_name.c_str());
        REQUIRE(rv == eswb_e_ok);

        double value1_src = 123.456;
        double value2_src = 777.777;

        rv = eswb_update_topic(scalar1_td, &value1_src);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_update_topic(scalar2_td, &value2_src);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_bridge_update(br);
        REQUIRE(rv == eswb_e_ok);

        std::string scalar1_at_dst_path = add_path(add_path(dst_bus_name, bridge_name), scalar1_name);
        std::string scalar2_at_dst_path = add_path(add_path(dst_bus_name, bridge_name), scalar2_name);

        rv = eswb_connect(scalar1_at_dst_path.c_str(), &dst_td);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_connect(scalar2_at_dst_path.c_str(), &dst2_td);
        REQUIRE(rv == eswb_e_ok);

        double value1_dst = 0;
        rv = eswb_read(dst_td, &value1_dst);
        REQUIRE(value1_dst == value1_src);

        double value2_dst = 0;
        rv = eswb_read(dst2_td, &value2_dst);
        REQUIRE(value2_dst == value2_src);
    }



    SECTION("Scalars and structure") {
        std::string bridge_name = "composite";
        rv = eswb_bridge_create(bridge_name.c_str(), 4, &br);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, scalar1_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, scalar2_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, struct1_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_bridge_add_topic(br, 0, add_path(src_bus_name, struct2_name).c_str(), NULL);
        REQUIRE(rv == eswb_e_ok);

        rv = eswb_bridge_connect(br, 0, dst_bus_name.c_str());
        REQUIRE(rv == eswb_e_ok);

        double value1_src = 123.456;
        double value2_src = 777.777;
        rv = eswb_update_topic(scalar1_td, &value1_src);
        REQUIRE(rv == eswb_e_ok);
        rv = eswb_update_topic(scalar2_td, &value2_src);
        REQUIRE(rv == eswb_e_ok);

        SECTION("Values match" ) {
            test_structure_t src_struct1 = {
                    .a = 1.0,
                    .b = 2.0,
                    .c = 3.0,
            };

            test_structure_t src_struct2 = {
                    .a = 10.0,
                    .b = 20.0,
                    .c = 30.0,
            };

            rv = eswb_update_topic(struct1_td, &src_struct1);
            REQUIRE(rv == eswb_e_ok);
            rv = eswb_update_topic(struct2_td, &src_struct2);
            REQUIRE(rv == eswb_e_ok);

            rv = eswb_bridge_update(br);
            REQUIRE(rv == eswb_e_ok);

            std::string scalar1_at_dst_path = add_path(add_path(dst_bus_name, bridge_name), scalar1_name);
            std::string scalar2_at_dst_path = add_path(add_path(dst_bus_name, bridge_name), scalar2_name);

            rv = eswb_connect(scalar1_at_dst_path.c_str(), &dst_td);
            REQUIRE(rv == eswb_e_ok);
            rv = eswb_connect(scalar2_at_dst_path.c_str(), &dst2_td);
            REQUIRE(rv == eswb_e_ok);

            double value1_dst = 0;
            rv = eswb_read(dst_td, &value1_dst);
            REQUIRE(value1_dst == value1_src);

            double value2_dst = 0;
            rv = eswb_read(dst2_td, &value2_dst);
            REQUIRE(value2_dst == value2_src);

            test_structure_t dst_struct1;
            test_structure_t dst_struct2;

            std::string struct1_at_dst_path = add_path(add_path(dst_bus_name, bridge_name), struct1_name);
            std::string struct2_at_dst_path = add_path(add_path(dst_bus_name, bridge_name), struct2_name);


            eswb_topic_descr_t test_struct_dst1_td;
            eswb_topic_descr_t test_struct_dst2_td;
            rv = eswb_connect(struct1_at_dst_path.c_str(), &test_struct_dst1_td);
            REQUIRE(rv == eswb_e_ok);
            rv = eswb_connect(struct2_at_dst_path.c_str(), &test_struct_dst2_td);
            REQUIRE(rv == eswb_e_ok);

            rv = eswb_read(test_struct_dst1_td, &dst_struct1);
            REQUIRE(rv == eswb_e_ok);
            REQUIRE(memcmp(&src_struct1, &dst_struct1, sizeof(dst_struct1)) == 0);
            rv = eswb_read(test_struct_dst2_td, &dst_struct2);
            REQUIRE(rv == eswb_e_ok);
            REQUIRE(memcmp(&src_struct2, &dst_struct2, sizeof(dst_struct2)) == 0);
        }

        SECTION("Structure match" ) {
            FAIL("Not implemented");
        }
    }
}


extern "C" int test_event_chain (int verbose, int nonstop);

TEST_CASE("Event chain test") {

    eswb_local_init(1);

    SECTION("Run 1", "") {
        int rv = test_event_chain(0, 0);
        REQUIRE(rv == 0);
    }

    SECTION("Run 2", "") {
        int rv = test_event_chain(0, 0);
        REQUIRE(rv == 0);
    }
}
