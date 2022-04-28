#ifndef ESWB_TOPIC_MEM_H
#define ESWB_TOPIC_MEM_H

#include "eswb/types.h"
#include "eswb/errors.h"
#include "eswb/event_queue.h"

#include "sync.h"

struct registry;


typedef struct topic_state {

    eswb_fifo_index_t    head; // TODO actually points to a place where next push will be stored, rename it? or change convention
    eswb_fifo_index_t    lap_num;

} topic_fifo_state_t;


typedef struct fifo_ext {
    eswb_size_t         fifo_size;
    eswb_size_t         elem_step;
    eswb_size_t         elem_size;
    topic_fifo_state_t  state;

} fifo_ext_t;

#define TOPIC_FLAGS_TO_EVENT_QUEUE (1 << 0)

typedef struct topic {
    // credentials
    char name[ESWB_TOPIC_NAME_MAX_LEN + 1];
    topic_data_type_t type;
    char* annotation;
    eswb_size_t data_size; // field size for regular topic; length of fifo for fifo; overall size for event_queue

    void* data;
    fifo_ext_t *fifo_ext;
    // sync and stat
    struct sync_handle* sync;

    //state : state
    //last_update_time : time
    //eswb_index_t me;

    uint32_t flags;

    // navigating
    struct topic *parent;
    struct topic *first_child;
    struct topic *next_sibling;

    struct registry *reg_ref;
    eswb_topic_id_t    id;

    eswb_event_queue_mask_t evq_mask; // TODO this thing should be inherited by nested topics
} topic_t;

#ifdef __cplusplus
extern "C" {
#endif


eswb_rv_t topic_mem_write(topic_t *t, void *data);
eswb_rv_t topic_mem_simply_copy(topic_t *t, void *data);
eswb_rv_t topic_mem_write_fifo(topic_t *t, void *data);
void topic_mem_read_fifo(topic_t *t, eswb_index_t tail, void *data);
eswb_rv_t topic_mem_get_params(topic_t *t, topic_params_t *params);

typedef eswb_rv_t (*crawling_lambda_t)(void *d, topic_t *t);

eswb_rv_t topic_mem_walk_through(topic_t *root, const char *find_path, crawling_lambda_t lambda, void *usr_data);

eswb_rv_t topic_mem_event_queue_write(topic_t *t, const event_queue_record_t *r);

eswb_rv_t topic_mem_event_queue_get_data(topic_t *evq, event_queue_record_t *event, void *data);
void topic_event_queue_read(topic_t *t, eswb_index_t tail, event_queue_record_t *r);

#ifdef __cplusplus
}
#endif


#endif //ESWB_TOPIC_MEM_H
