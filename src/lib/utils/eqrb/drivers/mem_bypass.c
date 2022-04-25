//
// Created by goofy on 1/9/22.
//

#include <pthread.h>
#include <string.h>

#include "eswb/api.h"
#include "../eqrb_core.h"


typedef struct {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int cnt;
} barrier_t;

typedef struct {
    uint8_t buf[512];
    uint32_t cnt;
    barrier_t barrier;
} buffer_t;

static void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->mutex);
    b->cnt--;

    pthread_cond_broadcast(&b->cond);

    if (b->cnt < 2) {
        pthread_cond_wait(&b->cond, &b->mutex);
    }

    b->cnt--;
    pthread_mutex_unlock(&b->mutex);
}

static void barrier_init(barrier_t *b) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->cnt = 0;
}

static int driver_inited;
pthread_mutex_t driver_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static eswb_topic_descr_t root_td;
static eswb_topic_descr_t writer_td; // FIXME handles convention problem, regarding the publication / subscription topic for fifo

typedef struct {
    pthread_t sender_tid;
    uint32_t bytes_num;
    uint8_t buf[256];

} bypass_queue_pkt_t;

static int init_driver() {
    pthread_mutex_lock(&driver_init_mutex);
    eswb_rv_t erv;
    int rv = 0;

    do {
        if (driver_inited) {
            break;
        }

        erv = eswb_create("eqbr_bypass", eswb_inter_thread, 4);
        if (erv != eswb_e_ok) {
            rv = 1;
            break;
        }

        erv = eswb_topic_connect("itb:/eqbr_bypass", &root_td);
        if (erv != eswb_e_ok) {
            rv = 1;
            break;
        }

        TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 4);
        topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, "packets", 50); // TODO confusing name, same as integrated service name
        topic_proclaiming_tree_t *elem_root = usr_topic_add_child(cntx, fifo_root, "packet", tt_struct, 0, sizeof(bypass_queue_pkt_t), TOPIC_FLAG_MAPPED_TO_PARENT);

        erv = eswb_proclaim_tree(root_td, fifo_root, cntx->t_num, &writer_td);

        driver_inited = -1;
    } while(0);

    pthread_mutex_unlock(&driver_init_mutex);

    return rv;
}

eqrb_rv_t mem_bypass_connect (const char *param, device_descr_t *dh) {
    eqrb_rv_t rv;
    eswb_rv_t erv;
    eswb_topic_descr_t td;

    init_driver(); // will be inited once

    erv = eswb_connect_nested(writer_td, "packet", &td);
    if (erv != eswb_e_ok) {
        return eqrb_rv_rx_eswb_fatal_err;
    }

    *dh = td;
    return eqrb_rv_ok;
}
eqrb_rv_t mem_bypass_send (device_descr_t dh, void *data, size_t bts, size_t *bs) {
    eqrb_rv_t rv = eqrb_rv_ok;
    eswb_rv_t erv;
    int bw;
    int bw_total;
    bypass_queue_pkt_t pkt;

#define MIN(a,b) ((a) < (b) ? (a) : (b))

    bw = 0;
    bw_total = 0;
    do {
        bw = MIN(bts, sizeof(pkt.buf));
        memcpy(pkt.buf, data, bw);
        pkt.bytes_num = bw;
        pkt.sender_tid = pthread_self();
        bw_total += bw;
        erv = eswb_fifo_push(writer_td, &pkt);
        if (erv != eswb_e_ok) {
            rv = eqrb_rv_rx_eswb_fatal_err;
        }
    } while(bw_total < bts);

    if (bs != NULL) {
        *bs = bw_total;
    }

    return rv;
}
eqrb_rv_t mem_bypass_recv (device_descr_t dh, void *data, size_t btr, size_t *br) {
    eswb_rv_t erv;

    bypass_queue_pkt_t pkt;

    do {
        erv = eswb_fifo_pop(dh, &pkt);
    } while ((erv == eswb_e_ok) && (pkt.sender_tid == pthread_self()));

    if (erv != eswb_e_ok) {
        return eqrb_rv_rx_eswb_fatal_err;
    }

    // don't care if we lost bytes in case buffer is smaller. Reader must have bigger buffer
    int r = MIN(btr, pkt.bytes_num);
    memcpy(data, pkt.buf, r);

    if (br != NULL) {
        *br = r;
    }

    return eqrb_rv_ok;
}
int mem_bypass_disconnect (device_descr_t dh) {
    return 1;
}

const driver_t mem_bypass_driver = {
        .name = "mem_bypass",
        .connect = mem_bypass_connect,
        .send = mem_bypass_send,
        .recv = mem_bypass_recv,
        .disconnect = mem_bypass_disconnect,
        .type = eqrb_duplex
};

static eqrb_server_handle_t sh;
static eqrb_client_handle_t ch;

eqrb_rv_t eqrb_demo_start(const char *bus_to_replicate, const char *replication_point, uint32_t mask_to_replicate) {

    sh.h.driver = &mem_bypass_driver;
    ch.h.driver = &mem_bypass_driver;

    eqrb_rv_t s_rv = eqrb_server_start(&sh, bus_to_replicate, mask_to_replicate, NULL);
    if (s_rv != eqrb_rv_ok) {
        return s_rv;
    }
    eqrb_rv_t c_rv = eqrb_client_start(&ch, replication_point, 100);
    if (c_rv != eqrb_rv_ok) {
        return c_rv;
    }

    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_demo_stop() {
    eqrb_service_stop(&sh.h);
    return eqrb_service_stop(&ch.h);
}
