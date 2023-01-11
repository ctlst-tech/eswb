// #include <catch2/catch_all.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../src/lib/include/public/eswb/api.h"
#include "../src/lib/services/eqrb/eqrb_priv.h"

extern const eqrb_media_driver_t eqrb_drv_file;
#define post_err(__fmt_str, __rv, ...)                                      \
    fprintf(stderr, "%s." __fmt_str ": %s (%d)\n", __func__, ##__VA_ARGS__, \
            eswb_strerror(__rv), __rv)

#define BUS_GENERATORS "generators"
#define EVENT_BUS "event_bus"
#define ITB_EVENT_BUS "itb:/" EVENT_BUS

class BasePublisher {
public:
    const char *name;
    eswb_topic_descr_t start_event_d;
    eswb_topic_descr_t out_d;

    virtual int init(void) = 0;
    virtual int update(void) = 0;
};

class SinGenerator final : public BasePublisher {
public:
    SinGenerator(const char *gen_name) {
        this->name = gen_name;
    }

    struct sin_out {
        double v;
    };

    virtual int init(void) final {
        TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
        struct sin_out so;
        topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, so, "sin");
        usr_topic_add_struct_child(cntx, rt, struct sin_out, v, "out",
                                   tt_double);
        eswb_rv_t rv = eswb_proclaim_tree_by_path("itb:/generators", rt,
                                                  cntx->t_num, &out_d);
        if (rv != eswb_e_ok) {
            post_err("eswb_proclaim_tree_by_path failed", rv);
            return 1;
        }
        return 0;
    }

    virtual int update(void) final {
        usleep(1000);
        struct sin_out so;
        static int i = 0;
        i++;
        so.v = sin(i * 0.05);
        eswb_update_topic(out_d, &so);
        return 0;
    }
};

void *publisherThread(void *p) {
    BasePublisher *publisher = static_cast<BasePublisher *>(p);
    int rv = publisher->init();

    char tn[16];
    snprintf(tn, sizeof(tn) - 1, "ec:%s", publisher->name);
    eswb_set_thread_name(tn);

    eswb_rv_t erv =
        eswb_connect("itb:/event_bus/start_event", &publisher->start_event_d);
    if (erv != eswb_e_ok) {
        post_err("eswb_fifo_subscribe for start event failed", erv);
    }

    if (rv) {
        fprintf(stderr, "Test thread %s failed\n", publisher->name);
        // return NULL;
    } else {
        printf("Test thread %s started\n", publisher->name);
    }

    uint32_t event_code;
    eswb_get_update(publisher->start_event_d, &event_code);

    while (1) {
        rv = publisher->update();
        sleep(1);
        if (rv) {
            return NULL;
        }
    }
}

class EqrbFileTest {
public:
    EqrbFileTest() = default;

    int StartPublisherService(BasePublisher *publisher) {
        eswb_rv_t rv;

        rv = eswb_create(BUS_GENERATORS, eswb_inter_thread, 20);
        if (rv != eswb_e_ok) {
            post_err("eswb_create \"" BUS_GENERATORS "\" failed", rv);
            return rv;
        }

        rv = eswb_create(EVENT_BUS, eswb_inter_thread, 5);
        if (rv != eswb_e_ok) {
            post_err("eswb_create \"" EVENT_BUS "\" failed", rv);
            return rv;
        }

        TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 2);
        topic_proclaiming_tree_t *rt =
            usr_topic_set_root(cntx, "start_event", tt_uint32, 4);

        rv = eswb_proclaim_tree_by_path(ITB_EVENT_BUS, rt, cntx->t_num,
                                        &publisher->start_event_d);
        if (rv != eswb_e_ok) {
            post_err("eswb_proclaim_tree_by_path failed", rv);
            return rv;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, publisherThread, (void *)publisher);

        return rv == eswb_e_ok ? 0 : -1;
    }

    int startEqrbService(const char *service_name, const char *bus2replicate,
                         const char *file_prefix, const char *dst_dir) {
        const char *err_msg;
        if (service_name == NULL || bus2replicate == NULL ||
            file_prefix == NULL || dst_dir == NULL) {
            return eqrb_invarg;
        }

        eqrb_rv_t rv = eqrb_file_server_start(service_name, file_prefix,
                                              dst_dir, bus2replicate, &err_msg);

        return rv == eqrb_rv_ok ? 0 : -1;
    }
};

// TEST_CASE("EQRB file test") {
//     EqrbFileTest eqrb_test;
//     BasePublisher *publisher = new SinGenerator();
//     SECTION("Run 1", "") {
//         int rv;
//         rv = eqrb_test.StartPublisherService(publisher);
//         REQUIRE(rv == 0);
//         rv = eqrb_test.startEqrbService("eqrb_file_write_service",
//         "generators", "generators", "./tmp"); REQUIRE(rv == 0);
//     }
// }

int main(int argc, char **argv) {
    EqrbFileTest eqrb_test;
    BasePublisher *publisher = new SinGenerator("sin");
    int rv;
    rv = eqrb_test.StartPublisherService(publisher);
    rv = eqrb_test.startEqrbService("eqrb_file_write_service", "generators",
                                    "generators", "./tmp");
    return 0;
}
