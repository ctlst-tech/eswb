//
// Created by goofy on 1/4/22.
//

#ifndef ESWB_EQRB_CORE_H
#define ESWB_EQRB_CORE_H

// FIXME make it platform independent
#include <pthread.h>
#include <unistd.h>

#include "eswb/event_queue.h"
#include "eswb/errors.h"
#include "eswb/types.h"
#include "eswb/eqrb.h"

#include "ids_map.h"


/**
 * Event queue replication bridge = EQRB
 */

typedef void* device_descr_t;

typedef struct {
    char name[30];
    eqrb_rv_t (*connect)(void *param, device_descr_t *dh);
    eqrb_rv_t (*send)(device_descr_t dh, void *data, size_t bts, size_t *bs);
    eqrb_rv_t (*recv)(device_descr_t dh, void *data, size_t btr, size_t *br);
    eqrb_rv_t (*command)(device_descr_t dh, eqrb_cmd_t cmd);
    int (*disconnect)(device_descr_t dh);
} driver_t;

typedef struct {
    void *connectivity_params;
    const driver_t *driver;
    device_descr_t dd;
    pthread_t tid;
} eqrb_handle_common_t;

typedef struct eqrb_server_handle {
    eqrb_handle_common_t h;

    eswb_topic_descr_t repl_root;
    eswb_topic_descr_t evq_td;

} eqrb_server_handle_t;


typedef struct eqrb_client_handle {
    eqrb_handle_common_t h;

    eswb_topic_descr_t repl_dst_td;
    topic_id_map_t ids_map;

} eqrb_client_handle_t;



#ifdef __cplusplus
extern "C" {
#endif

void eqrb_debug_msg(const char *fn, const char *txt, ...);

#define EQRB_DEBUG

#ifdef EQRB_DEBUG
#define eqrb_dbg_msg(txt,...) eqrb_debug_msg(__func__, txt, ##__VA_ARGS__)
#else
#define eqrb_dbg_msg(txt,...) {}
#endif

eqrb_rv_t eqrb_client_start(eqrb_client_handle_t *h, const char *mount_point, size_t repl_map_size);
eqrb_rv_t eqrb_client_stop(eqrb_client_handle_t *h);

eqrb_rv_t
eqrb_server_start(eqrb_server_handle_t *h, const char *bus_to_replicate, uint32_t ch_mask, const char **err_msg);
eqrb_rv_t eqrb_service_stop(eqrb_handle_common_t *h);


#ifdef __cplusplus
}
#endif


#endif //ESWB_EQRB_CORE_H
