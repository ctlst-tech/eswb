//
// Created by Ivan Makarov on 28/10/21.
//




#include <unistd.h>
#include <math.h>
#include <string.h>


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "eswb/api.h"
#include "eswb/bridge.h"
#include "eswb/event_queue.h"


/**
 *
 *      Fa      Fb      Fc      Fd
 *      |       |       |       |
 *  ------------------------------------------------  nsb  functions
 *                                      |
 *   sin_gen    saw_gen               bridge
 *      |          |                    |
 *  ----------------------------------  |  ---------  itb  generators
 *    |        |                        |      |
 *    |      lin_freq       fifo_src    |   funcs_sum_fifo
 *    |        |               |        |     |
 *    |   ------------------------------------------ itb  conversions
 *    |        |               |             |
 *    |       print        event_print   sum_print
 *   evq
 */

#define TM_EVENT_BRIDGE_DATA_UPDATE         (1 << 0)
#define TM_BRIDGE_DATA_NON_NULL             (1 << 1)
#define TM_EVENT_LIN_FREQ_SIN_UPDATE        (1 << 2)
#define TM_LIN_FREQ_SAW_NON_NULL            (1 << 3)
#define TM_EVENT_FIFO_SRC_DATA_POP          (1 << 4)
#define TM_EVENT_EVENT_PRINT_GOT_C_CHAR     (1 << 5)
#define TM_EVENT_SUM_PRINT_GOT_STR          (1 << 6)
#define TM_EVENT_GOT_LIN_FREQ_DATA          (1 << 7)
#define TM_EVENT_EV_QUEUE_GOT_TOPIC_ONE     (1 << 8)
#define TM_EVENT_EV_QUEUE_GOT_TOPIC_TWO     (1 << 9)

#define SUCCESS_MASK  ( \
TM_EVENT_BRIDGE_DATA_UPDATE | \
TM_BRIDGE_DATA_NON_NULL | \
TM_EVENT_LIN_FREQ_SIN_UPDATE | \
TM_LIN_FREQ_SAW_NON_NULL | \
TM_EVENT_FIFO_SRC_DATA_POP | \
TM_EVENT_EVENT_PRINT_GOT_C_CHAR | \
TM_EVENT_SUM_PRINT_GOT_STR | \
TM_EVENT_GOT_LIN_FREQ_DATA | \
TM_EVENT_EV_QUEUE_GOT_TOPIC_ONE | \
TM_EVENT_EV_QUEUE_GOT_TOPIC_TWO )

static uint32_t test_mask = 0;
pthread_mutex_t test_mask_mutex = PTHREAD_MUTEX_INITIALIZER;

static void test_mask_set_flag(uint32_t flag) {
    pthread_mutex_lock(&test_mask_mutex);
    test_mask |= flag;
    pthread_mutex_unlock(&test_mask_mutex);
}

static uint32_t test_mask_get() {
    uint32_t rv;
    pthread_mutex_lock(&test_mask_mutex);
    rv = test_mask;
    pthread_mutex_unlock(&test_mask_mutex);

    return rv;
}

static void test_mask_reset() {
    pthread_mutex_lock(&test_mask_mutex);
    test_mask = 0;
    pthread_mutex_unlock(&test_mask_mutex);

}

static int verbosity = 0;


#define BUS_FUNCTIONS "functions"
#define BUS_GENERATORS "generators"
#define BUS_CONVERSIONS "conversions"

struct test_thread_param;

#define post_err(__fmt_str,__rv, ...) fprintf(stderr, "%s." __fmt_str ": %s (%d)\n", __func__, ## __VA_ARGS__, eswb_strerror(__rv), __rv)


eswb_topic_descr_t start_fifo_event_td;

#define EVENT_BUS "event_bus"
#define ITB_EVENT_BUS "itb:/" EVENT_BUS

void init_start_event() {

    eswb_rv_t rv = eswb_create(EVENT_BUS, eswb_inter_thread, 5);
    if (rv != eswb_e_ok) {
        post_err("eswb_create \"" EVENT_BUS "\" failed", rv);
        return;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx,2);
    topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, "start_event", tt_uint32, 4);

    rv = eswb_proclaim_tree_by_path(ITB_EVENT_BUS, rt, cntx->t_num, &start_fifo_event_td);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return;
    }
}


void fire_start_event_fire() {
    uint32_t data = 123;
    eswb_update_topic(start_fifo_event_td, &data);
}


typedef struct test_thread_param{
    eswb_topic_descr_t start_event_d;

    const char *name;
    int (*init_handler)(struct test_thread_param *p);
    int (*cycle_handler)(struct test_thread_param *p);
    eswb_topic_descr_t in1_d;
    eswb_topic_descr_t in2_d;
    eswb_topic_descr_t out_d;
    eswb_topic_descr_t out2_d;
    void *usr_data;
    int cascade_num;

    pthread_t tid;
} test_thread_param_t;


static pthread_cond_t test_thread_barrier_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t test_thread_barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static int test_thread_barrier_cnt = 0;

static void test_barrier_wait() {
    pthread_mutex_lock(&test_thread_barrier_mutex);
    test_thread_barrier_cnt++;

    pthread_cond_broadcast(&test_thread_barrier_cond);

    if (test_thread_barrier_cnt < 2) {
        pthread_cond_wait(&test_thread_barrier_cond, &test_thread_barrier_mutex);
    }

    test_thread_barrier_cnt--;
    pthread_mutex_unlock(&test_thread_barrier_mutex);
}

void eswb_set_thread_name(const char *n);

void *test_thread(void *p) {
    test_thread_param_t *tp = p;
    // printf ("Test thread %s started\n", tp->name);
    int rv = tp->init_handler(tp);

    char tn[16];
    snprintf(tn, sizeof(tn) - 1, "ec:%s", tp->name);
    eswb_set_thread_name(tn);

    eswb_rv_t erv = eswb_subscribe("itb:/event_bus/start_event", &tp->start_event_d);
    if (erv != eswb_e_ok) {
        post_err("eswb_fifo_subscribe for start event failed", erv);
    }

    if (rv) {
        fprintf (stderr, "Test thread %s failed\n", tp->name);
        //return NULL;
    } else {
        if (verbosity)
            printf ("Test thread %s started\n", tp->name);
    }

    test_barrier_wait();

    uint32_t event_code;
    eswb_get_update(tp->start_event_d, &event_code);

    while(1) {
        rv = tp->cycle_handler(tp);
        if (rv) {
            return NULL;
        }
    }
}

int start_thread(test_thread_param_t *p) {
    pthread_t t;
    eswb_rv_t rv = pthread_create(&p->tid, NULL, test_thread, p);
    test_barrier_wait();
    return rv;
}

int stop_thread(test_thread_param_t *p) {
    void *rv_ptr;

    pthread_cancel(p->tid);
    pthread_join(p->tid, &rv_ptr);

    return 0;
}

typedef struct fX {
    double out;
} fXout_t;


typedef struct {
    uint32_t cnt;

    eswb_topic_descr_t d_fa;
    eswb_topic_descr_t d_fb;
    eswb_topic_descr_t d_fc;
    eswb_topic_descr_t d_fd;

    eswb_bridge_t *bridge;
} nsb_test_cnt_struct_t;

int functions_init_handler(struct test_thread_param *p) {

    eswb_rv_t rv;

#define NSB_FUNCTIONS "nsb:/" BUS_FUNCTIONS

    nsb_test_cnt_struct_t *cs = p->usr_data = calloc (1, sizeof(nsb_test_cnt_struct_t) );

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);

    fXout_t fa;
    topic_proclaiming_tree_t *Fa = usr_topic_set_struct(cntx, fa, "Fa");
    usr_topic_add_struct_child(cntx, Fa, fXout_t, out, "out", tt_double);

    rv = eswb_proclaim_tree_by_path(NSB_FUNCTIONS, Fa, cntx->t_num, &cs->d_fa);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return rv;
    }

    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);

    fXout_t fb;
    topic_proclaiming_tree_t *Fb = usr_topic_set_struct(cntx, fb, "Fb");
    usr_topic_add_struct_child(cntx, Fb, fXout_t, out, "out", tt_double);

    rv = eswb_proclaim_tree_by_path(NSB_FUNCTIONS, Fb, cntx->t_num, &cs->d_fb);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return rv;
    }

    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);

    fXout_t fc;
    topic_proclaiming_tree_t *Fc = usr_topic_set_struct(cntx, fc, "Fc");
    usr_topic_add_struct_child(cntx, Fc, fXout_t, out, "out", tt_double);

    rv = eswb_proclaim_tree_by_path(NSB_FUNCTIONS, Fc, cntx->t_num, &cs->d_fc);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return rv;
    }

    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);

    fXout_t fd;
    topic_proclaiming_tree_t *Fd = usr_topic_set_struct(cntx, fd, "Fd");
    usr_topic_add_struct_child(cntx, Fd, fXout_t, out, "out", tt_double);


    rv = eswb_proclaim_tree_by_path(NSB_FUNCTIONS, Fd, cntx->t_num, &cs->d_fd);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return rv;
    }

    rv = eswb_bridge_create("funcs2world", 10, &cs->bridge);
    if (rv != eswb_e_ok) {
        post_err("eswb_bridge_create failed", rv);
        return rv;
    }

    rv = eswb_bridge_add_topic(cs->bridge, 0, NSB_FUNCTIONS "/Fa/out", "Fa");
    if (rv != eswb_e_ok) {
        post_err("eswb_bridge_add_topic failed", rv);
        return rv;
    }
    rv = eswb_bridge_add_topic(cs->bridge, 0, NSB_FUNCTIONS "/Fb/out", "Fb");
    if (rv != eswb_e_ok) {
        post_err("eswb_bridge_add_topic failed", rv);
        return rv;
    }
    rv = eswb_bridge_add_topic(cs->bridge, 0, NSB_FUNCTIONS "/Fc/out", "Fc");
    if (rv != eswb_e_ok) {
        post_err("eswb_bridge_add_topic failed", rv);
        return rv;
    }
    rv = eswb_bridge_add_topic(cs->bridge, 0, NSB_FUNCTIONS "/Fd/out", "Fd");
    if (rv != eswb_e_ok) {
        post_err("eswb_bridge_add_topic failed", rv);
        return rv;
    }

#define ITB_CONVERSIONS "itb:/" BUS_CONVERSIONS

    rv = eswb_bridge_connect_vector(cs->bridge, ITB_CONVERSIONS);

    return rv;
}

int test_nsb_update_func(uint32_t val, eswb_topic_descr_t d) {
    fXout_t f;

    f.out = val;

    return eswb_update_topic(d, &f);
}

int functions_cycle_handler(struct test_thread_param *p) {

    usleep(1000000);

    nsb_test_cnt_struct_t *cs = p->usr_data;

    cs->cnt++;

    test_nsb_update_func(cs->cnt >> 1, cs->d_fa);
    test_nsb_update_func(cs->cnt >> 2, cs->d_fb);
    test_nsb_update_func(cs->cnt << 1, cs->d_fc);
    test_nsb_update_func(cs->cnt % 8, cs->d_fd);

    eswb_bridge_update(cs->bridge);

    return 0;
}

struct sin_out {
    double v;
};

int gen_sin_init_handler(struct test_thread_param *p) {
    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
    struct sin_out so;
    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, so, "sin");
    usr_topic_add_struct_child(cntx, rt, struct sin_out, v, "out", tt_double);
    eswb_rv_t rv = eswb_proclaim_tree_by_path("itb:/generators", rt, cntx->t_num, &p->out_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return 1;
    }
    return 0;
}

int gen_sin_cycle_handler(struct test_thread_param *p) {
    usleep(10000);
    struct sin_out so;
    static int i = 0;
    i++;
    so.v = sin(i / 100.0);
    eswb_update_topic(p->out_d, &so);
    return 0;
}

struct saw_out {
    double v;
    double test_var;
};

int gen_saw_init_handler(struct test_thread_param *p) {
    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
    struct saw_out so;
    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, so, "saw");
    usr_topic_add_struct_child(cntx, rt, struct saw_out, v, "out", tt_double);
    usr_topic_add_struct_child(cntx, rt, struct saw_out, test_var, "test_var", tt_double);

    eswb_rv_t rv = eswb_proclaim_tree_by_path("itb:/generators", rt, cntx->t_num, &p->out_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return 1;
    }

    return 0;
}

int gen_saw_cycle_handler(struct test_thread_param *p) {
    usleep(1000);
    struct saw_out so;
    static int i = 0;
    i++;
    so.v = i * 0.1;
    if (i > 1000) {
        i = 0;
    }
    eswb_update_topic(p->out_d, &so);
    return 0;
}

struct lin_freq_out {
    double v;
};

int lin_freq_init_handler(struct test_thread_param *p) {

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
    struct lin_freq_out so;
    topic_proclaiming_tree_t *rt = usr_topic_set_struct(cntx, so, "lin_freq");
    usr_topic_add_struct_child(cntx, rt, struct lin_freq_out, v, "out", tt_double);

    eswb_rv_t rv = eswb_proclaim_tree_by_path("itb:/conversions", rt, cntx->t_num, &p->out_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return 1;
    }

    rv = eswb_subscribe("itb:/generators/sin/out", &p->in1_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_subscribe failed", rv);
        return 1;
    }

    rv = eswb_subscribe("itb:/generators/saw/out", &p->in2_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_subscribe failed", rv);
        return 1;
    }

    return 0;
}

int lin_freq_cycle_handler(struct test_thread_param *p) {
    struct lin_freq_out o;
    struct sin_out i1;
    struct saw_out i2;

    eswb_rv_t rv = eswb_get_update(p->in1_d, &i1.v);
    if (rv != eswb_e_ok) {
        return 1;
    }
    test_mask_set_flag(TM_EVENT_LIN_FREQ_SIN_UPDATE);

    rv = eswb_read(p->in2_d, &i2.v);
    if (rv != eswb_e_ok) {
        return 1;
    }
    if (i2.v > 0.5) {
        test_mask_set_flag(TM_LIN_FREQ_SAW_NON_NULL);
    }

    o.v = i1.v * i2.v;

    //printf("%s out == %.2f\n", __func__, o.v);

    eswb_update_topic(p->out_d, &o);
    return 0;
}


int lin_freq_print_init_handler(struct test_thread_param *p) {

    eswb_rv_t rv;

    rv = eswb_subscribe("itb:/conversions/lin_freq/out", &p->in1_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_subscribe failed", rv);
        return 1;
    }

    return 0;
}

int lin_freq_print_cycle_handler(struct test_thread_param *p) {
    struct lin_freq_out i;

    eswb_rv_t rv = eswb_get_update(p->in1_d, &i.v);
    if (rv != eswb_e_ok) {
        post_err("eswb_get_update failed", rv);
        return 1;
    }

    if (i.v > 0.2) {
        test_mask_set_flag(TM_EVENT_GOT_LIN_FREQ_DATA);
    }
    //printf("%s out == %.2f\n", __func__, i.v);

    return 0;
}

struct fifo_src {
    uint32_t id;
    uint32_t num;
    uint32_t data;
};

int fifo_src_init_handler(struct test_thread_param *p) {

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 10);
    struct fifo_src fifo_elem;
    topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, "event_queue", 10); // TODO confusing name, same as integrated service name
    topic_proclaiming_tree_t *elem_root = usr_topic_add_child(cntx, fifo_root, "event", tt_struct, 0, sizeof(fifo_elem), TOPIC_FLAG_MAPPED_TO_PARENT);

            //usr_topic_add_struct_child(cntx, fifo_root, fifo_elem, "event", tt_struct);
    usr_topic_add_struct_child(cntx, elem_root, struct fifo_src, id, "id", tt_uint32);
    usr_topic_add_struct_child(cntx, elem_root, struct fifo_src, num, "num", tt_uint32);
    usr_topic_add_struct_child(cntx, elem_root, struct fifo_src, data, "data", tt_uint32);

    eswb_rv_t rv = eswb_proclaim_tree_by_path("itb:/conversions", fifo_root, cntx->t_num, &p->out_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_proclaim failed", rv);
        return 1;
    }

    return 0;
}

int fifo_src_cycle_handler(struct test_thread_param *p) {
    struct fifo_src e;
    usleep(200000);

    static char *message = "abcdefgjk";
    static int cnt = 0;
    static int msg_shift = 0;

    e.id = 1;
    e.num = cnt;
    e.data = (uint32_t) message[msg_shift];
    cnt++;
    msg_shift++;
    if (msg_shift >= 9) {
        msg_shift = 0;
    }

    eswb_rv_t rv = eswb_fifo_push(p->out_d, &e);

    if (verbosity) {
        printf("%s event push \'%c\'\n", __func__, e.data);
    }

    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_push failed", rv);
        return 1;
    }

    return 0;
}


int event_print_init_handler(struct test_thread_param *p) {

    eswb_rv_t rv = eswb_fifo_subscribe("itb:/conversions/event_queue/event", &p->in1_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_subscribe failed", rv);
        return 1;
    }

    return 0;
}

int event_print_cycle_handler(struct test_thread_param *p) {

    struct fifo_src i;

    eswb_rv_t rv = eswb_fifo_pop(p->in1_d, &i);
    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_pop failed", rv);
        return 1;
    }

    test_mask_set_flag(TM_EVENT_FIFO_SRC_DATA_POP);

    if (i.data == 'c') {
        test_mask_set_flag(TM_EVENT_EVENT_PRINT_GOT_C_CHAR);
    }

    if (verbosity) {
        printf("%s event id=%d num=%d data=\'%c\'\n", __func__, i.id, i.num, i.data);
    }

    return 0;
}

int funcs_sum_init_handler(struct test_thread_param *p) {

    eswb_rv_t rv = eswb_subscribe("itb:/conversions/funcs2world", &p->in1_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_subscribe failed", rv);
        return 1;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 3);

    topic_proclaiming_tree_t *fifo_root = usr_topic_set_fifo(cntx, "funcs_sum_msg", 10);
    usr_topic_add_child(cntx, fifo_root, "msg", tt_string, 0, 100, TOPIC_FLAG_MAPPED_TO_PARENT);

    rv = eswb_proclaim_tree_by_path("itb:/conversions", fifo_root, cntx->t_num, &p->out_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return 1;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx2, 3);

    topic_proclaiming_tree_t *msg_root = usr_topic_set_root(cntx2, "plain_msg", tt_string, 10);

    rv = eswb_proclaim_tree_by_path("itb:/conversions", msg_root, cntx2->t_num, &p->out2_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_proclaim_tree_by_path failed", rv);
        return 1;
    }

    return 0;
}

int funcs_sum_cycle_handler(struct test_thread_param *p) {


    double a[4];
    memset(a, 0, sizeof(a));

    eswb_rv_t rv = eswb_get_update(p->in1_d, a);
    if (rv != eswb_e_ok) {
        return 1;
    }

    test_mask_set_flag(TM_EVENT_BRIDGE_DATA_UPDATE);

    int sum = (int) (a[0] + a[1] + a[2] + a[3]);
    char out_msg[100];
    sprintf(out_msg, "%s info: %d %d %d %d sum = %d", __func__, (int) a[0], (int) a[1], (int) a[2], (int) a[3], sum);

    if (sum > 0) {
        test_mask_set_flag(TM_BRIDGE_DATA_NON_NULL);
    }

    rv = eswb_fifo_push(p->out_d, out_msg);
    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_push failed", rv);
        return 1;
    }


    sprintf(out_msg, "%d %d",(int) a[3], sum);
    out_msg[10] = 0;

    rv = eswb_update_topic(p->out2_d, out_msg);
    if (rv != eswb_e_ok) {
        post_err("eswb_update_topic failed", rv);
        return 1;
    }

    return 0;
}


int sum_print_init_handler(struct test_thread_param *p) {

    eswb_rv_t rv = eswb_fifo_subscribe("itb:/conversions/funcs_sum_msg/msg", &p->in1_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_subscribe failed", rv);
        return 1;
    }

    return 0;
}

int sum_print_cycle_handler(struct test_thread_param *p) {

    char fifo_out [100];

    eswb_rv_t rv = eswb_fifo_pop(p->in1_d, fifo_out);
    if (rv != eswb_e_ok) {
        post_err("eswb_fifo_pop failed", rv);
        return 1;
    }

    test_mask_set_flag(TM_EVENT_SUM_PRINT_GOT_STR);

    if (verbosity) {
        printf("%s info: %s\n", __func__, fifo_out);
    }

    return 0;
}

#define EVENT_QUEUE_CHAN_ID 20

int evq_init_handler(struct test_thread_param *p) {

    eswb_rv_t rv = eswb_event_queue_subscribe("itb:/" BUS_CONVERSIONS, &p->in1_d);
    if (rv != eswb_e_ok) {
        post_err("eswb_event_queue_subscribe failed", rv);
        return 1;
    }

    rv = eswb_event_queue_set_receive_mask(p->in1_d, (1 << EVENT_QUEUE_CHAN_ID));
    if (rv != eswb_e_ok) {
        post_err("eswb_event_queue_set_receive_mask failed", rv);
        return 1;
    }

    return 0;
}


static int topic_unique_ids_registry(uint32_t tid) {

#   define REG_SIZE 4
    static uint32_t topic_ids[REG_SIZE] = {-1, -1, -1, -1};
    static int topics_ids_num = 0;

    for (int i = 0; i < REG_SIZE; i++) {
        if ( topic_ids[i] == tid ) {
            return topics_ids_num;
        }
    }

    topic_ids[topics_ids_num] = tid;
    topics_ids_num++;

    return topics_ids_num;
}

int evq_cycle_handler(struct test_thread_param *p) {

    uint8_t fifo_out [100];

    event_queue_transfer_t *event = (event_queue_transfer_t *) fifo_out;

    eswb_rv_t rv = eswb_event_queue_pop(p->in1_d, event);
    if (rv != eswb_e_ok) {
        post_err("eswb_event_queue_pop failed", rv);
        return 1;
    }

    int tid_cnt = topic_unique_ids_registry(event->topic_id);

    if (tid_cnt >= 1) {
        test_mask_set_flag(TM_EVENT_EV_QUEUE_GOT_TOPIC_ONE);
    }
    if (tid_cnt >= 2) {
        test_mask_set_flag(TM_EVENT_EV_QUEUE_GOT_TOPIC_TWO);
    }

    if (verbosity) {
        printf("%s. Event type=%d size=%d topic_id=%d\n", __func__, event->type, event->size, event->topic_id);
    }

    return 0;
}


static void test1 ( int para )
{
    para = 1;
}
static void test2 ( char *p,  char *q )
{
    p = q;
}

int main2() {
    char *p = NULL;
    test1(0);
    test2(p, p);
    return 0;
}

int test_event_chain (int verbose, int nonstop) {

    verbosity = verbose;

    test_mask_reset();

    eswb_rv_t rv = eswb_create(BUS_GENERATORS, eswb_inter_thread, 20);
    if (rv != eswb_e_ok) {
        post_err("eswb_create \"" BUS_GENERATORS "\" failed", rv);
    }

//    eswb_topic_descr_t eq_td = 0;
//    rv = eswb_topic_connect("itb:/" BUS_GENERATORS, &eq_td);
//    if (rv != eswb_e_ok) {
//        post_err("eswb_topic_connect \"" BUS_GENERATORS "\" failed", rv);
//    }
//
//    rv = eswb_event_queue_enable(eq_td, 40, 800);
//    if (rv != eswb_e_ok) {
//        post_err("eswb_event_queue_enable \"" BUS_CONVERSIONS "\" failed", rv);
//    }

    rv = eswb_create(BUS_CONVERSIONS, eswb_inter_thread, 20);
    if (rv != eswb_e_ok) {
        post_err("eswb_create \"" BUS_CONVERSIONS "\" failed", rv);
    }

    rv = eswb_create(BUS_FUNCTIONS, eswb_non_synced, 20);
    if (rv != eswb_e_ok) {
        post_err("eswb_create \"" BUS_FUNCTIONS "\" failed", rv);
    }

    static test_thread_param_t sin = {
            .name = "sin_gen",
            .init_handler = gen_sin_init_handler,
            .cycle_handler = gen_sin_cycle_handler,
            .cascade_num = 1,
    };

    static test_thread_param_t saw = {
            .name = "saw_gen",
            .init_handler = gen_saw_init_handler,
            .cycle_handler = gen_saw_cycle_handler,
            .cascade_num = 1,
    };

    static test_thread_param_t lin_freq = {
            .name = "lin_freq",
            .init_handler = lin_freq_init_handler,
            .cycle_handler = lin_freq_cycle_handler,
            .cascade_num = 2,
    };

    static test_thread_param_t lin_freq_print = {
            .name = "lin_freq_print",
            .init_handler = lin_freq_print_init_handler,
            .cycle_handler = lin_freq_print_cycle_handler,
            .cascade_num = 3,
    };

    static test_thread_param_t fifo_src = {
            .name = "fifo_src",
            .init_handler = fifo_src_init_handler,
            .cycle_handler = fifo_src_cycle_handler,
            .cascade_num = 2,
    };

    static test_thread_param_t event_print = {
            .name = "event_print",
            .init_handler = event_print_init_handler,
            .cycle_handler = event_print_cycle_handler,
            .cascade_num = 3,
    };

    static test_thread_param_t nsb_functions = {
            .name = "nsb_functions",
            .init_handler = functions_init_handler,
            .cycle_handler = functions_cycle_handler,
            .cascade_num = 0,
    };

    static test_thread_param_t funcs_sum = {
            .name = "funcs_sum",
            .init_handler = funcs_sum_init_handler,
            .cycle_handler = funcs_sum_cycle_handler,
            .cascade_num = 0,
    };

    static test_thread_param_t sum_print = {
            .name = "sum_print",
            .init_handler = sum_print_init_handler,
            .cycle_handler = sum_print_cycle_handler,
            .cascade_num = 0,
    };

    static test_thread_param_t evq = {
            .name = "evq",
            .init_handler = evq_init_handler,
            .cycle_handler = evq_cycle_handler,
            .cascade_num = 0,
    };

    init_start_event();

    start_thread(&sin);
    start_thread(&saw);
    start_thread(&lin_freq);
    start_thread(&lin_freq_print);
    start_thread(&fifo_src);
    start_thread(&event_print);
    start_thread(&nsb_functions);
    start_thread(&funcs_sum);
    start_thread(&sum_print);


    eswb_print(ITB_EVENT_BUS);
    eswb_print("itb:/" BUS_GENERATORS);
    eswb_print("itb:/" BUS_CONVERSIONS);
    eswb_print("nsb:/" BUS_FUNCTIONS);

    fire_start_event_fire();

    usleep(200000);

    eswb_topic_descr_t ordering_td = 0;
    rv = eswb_topic_connect("itb:/" BUS_CONVERSIONS, &ordering_td);
    if (rv != eswb_e_ok) {
        post_err("eswb_topic_connect \"" BUS_CONVERSIONS "\" failed", rv);
    }

    if (ordering_td != 0) {
        rv = eswb_event_queue_enable(ordering_td, 20, 4000);
        if (rv != eswb_e_ok) {
            post_err("eswb_enable_event_queue \"" BUS_CONVERSIONS "\" failed", rv);
        }
    }

    start_thread(&evq);

    usleep(200000);

    fire_start_event_fire();


    if (ordering_td != 0) {
        rv = eswb_event_queue_order_topic(ordering_td, BUS_CONVERSIONS "/funcs_sum_msg", EVENT_QUEUE_CHAN_ID);
        if (rv != eswb_e_ok) {
            post_err("eswb_event_queue_order_topic \"" BUS_CONVERSIONS "\" failed", rv);
        }
        usleep(200000);

        rv = eswb_event_queue_order_topic(ordering_td, BUS_CONVERSIONS "/lin_freq", EVENT_QUEUE_CHAN_ID);
        if (rv != eswb_e_ok) {
            post_err("eswb_event_queue_order_topic \"" BUS_CONVERSIONS "\" failed", rv);
        }

        rv = eswb_event_queue_order_topic(ordering_td, BUS_CONVERSIONS "/funcs2world", EVENT_QUEUE_CHAN_ID);
        if (rv != eswb_e_ok) {
            post_err("eswb_event_queue_order_topic \"" BUS_CONVERSIONS "\" failed", rv);
        }

        rv = eswb_event_queue_order_topic(ordering_td, BUS_CONVERSIONS "/plain_msg", EVENT_QUEUE_CHAN_ID);
        if (rv != eswb_e_ok) {
            post_err("eswb_event_queue_order_topic \"" BUS_CONVERSIONS "\" failed", rv);
        }

        eswb_disconnect(ordering_td);
    }

    if (nonstop) {
        while(1) {
            sleep(10000);
        }
    } else {
        printf("Init is fine, waiting results ...\n");

        uint32_t tm;
        int i = 0;
        do {
            tm = test_mask_get();
            sleep(1);
            i++;
        } while ((tm != SUCCESS_MASK) && (i < 5));

        stop_thread(&sin);
        stop_thread(&saw);
        stop_thread(&lin_freq);
        stop_thread(&lin_freq_print);
        stop_thread(&fifo_src);
        stop_thread(&event_print);
        stop_thread(&nsb_functions);
        stop_thread(&funcs_sum);
        stop_thread(&sum_print);
        stop_thread(&evq);

        int test_rv = tm == SUCCESS_MASK ? 0 : 1;

        if (test_rv) {
            for (i = 0; i <= 9; i++) {
                printf("Flag %d; Result = %s\n", i, (1 << i) & tm ? "Success" : "Failure");
            }
        }
        return test_rv;
    }

    return 1;
}



/* TODO defensive programming:
 * TODO DP-1. create NSB ownership check. Don't allow to interact with topics from other threads
 *
 */

