#include <pthread.h>

// #include <catch2/catch_all.hpp>
#include <cmath>
#include <cstring>
#include <string>

#include "../src/lib/include/public/eswb/api.h"
#include "../src/lib/services/eqrb/eqrb_priv.h"

extern const eqrb_media_driver_t eqrb_drv_file;

class BasePublisher {
public:
    std::string pub;
    std::string bus;
    eswb_topic_descr_t topic_desc;
    virtual int init(void) = 0;
    virtual int update(void) = 0;
};

class SinGenerator final : public BasePublisher {
private:
    struct SinGen {
        double in;
        double out;
    } m_sin;
    double m_amp;
    double m_period_scaler;
    uint64_t m_counter;

public:
    SinGenerator(const std::string &pub_name, const std::string &bus_name,
                 int amp, double period_scaler)
        : m_amp(amp), m_period_scaler(period_scaler), m_counter(0) {
        pub = std::move(pub_name);
        bus = std::move(bus_name);
    }

    virtual int init(void) final {
        TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);

        topic_proclaiming_tree_t *rt =
            usr_topic_set_struct(cntx, m_sin, pub.c_str());
        usr_topic_add_struct_child(cntx, rt, struct SinGen, in, "in",
                                   tt_double);
        usr_topic_add_struct_child(cntx, rt, struct SinGen, out, "out",
                                   tt_double);
        eswb_rv_t rv = eswb_proclaim_tree_by_path(
            this->bus.c_str(), rt, cntx->t_num, &this->topic_desc);
        // REQUIRE(rv == eswb_e_ok);

        return 0;
    }

    virtual int update(void) final {
        struct SinGen sin_data;
        sin_data.in = m_counter * m_period_scaler;
        sin_data.out = sin(sin_data.in) * m_amp;
        eswb_update_topic(topic_desc, &sin_data);
        m_counter++;
        usleep(1000);
        return 0;
    }
};

void *publisherThread(void *p) {
    BasePublisher *publisher = static_cast<BasePublisher *>(p);
    publisher->init();
    while (1) {
        publisher->update();
    }
}

class EqrbFileTest {
public:
    EqrbFileTest() = default;

    int createBus(const std::string &bus) {
        eswb_rv_t rv;
        eswb_topic_descr_t queue_desc;
        eswb_topic_descr_t bus_desc;

        rv = eswb_create(bus.c_str(), eswb_inter_thread, 20);
        // REQUIRE(rv == eswb_e_ok);

        rv = eswb_connect(bus.c_str(), &bus_desc);
        // REQUIRE(rv == eswb_e_ok);

        rv = eswb_event_queue_enable(bus_desc, 40, 1024);
        // REQUIRE(erv == eswb_e_ok);

        // rv = eswb_event_queue_subscribe(publisher->bus.c_str(),
        // &publisher->queue_desc); REQUIRE(erv == eswb_e_ok);

        // rv = eswb_event_queue_set_receive_mask(publisher->queue_desc,
        // 0xFFFFFFFF); REQUIRE(erv == eswb_e_ok);
        rv = eswb_event_queue_order_topic(bus_desc, bus.c_str(), 1);
        // REQUIRE(erv == eswb_e_ok);

        return 0;
    }

    int startPublisherService(BasePublisher *publisher) {
        pthread_t tid;
        pthread_create(&tid, NULL, publisherThread, (void *)publisher);
        // REQUIRE(tid != NULL);

        return 0;
    }

    int startEqrbService(const char *service_name, const char *bus2replicate,
                         const char *file_prefix, const char *dst_dir) {
        const char *err_msg;
        // REQUIRE(service_name != NULL && bus2replicate != NULL &&
        //         file_prefix != NULL && dst_dir != NULL);

        eqrb_rv_t rv = eqrb_file_server_start(service_name, file_prefix,
                                              dst_dir, bus2replicate, &err_msg);
        // REQUIRE(rv == eqrb_rv_ok);

        return 0;
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
    BasePublisher *sin1 = new SinGenerator(std::string("sin1"),
                                           std::string("generators"), 1.0, 0.001);
    BasePublisher *sin2 = new SinGenerator(std::string("sin2"),
                                           std::string("generators"), 2.0, 0.002);

    eqrb_test.createBus(std::string("generators"));
    eqrb_test.startPublisherService(sin1);
    eqrb_test.startPublisherService(sin2);
    eqrb_test.startEqrbService("eqrb_file_write_service", "generators", "sin",
                               "/tmp");
    while (1) {
        sleep(1000);
    }
    return 0;
}
