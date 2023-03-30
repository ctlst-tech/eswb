#ifndef ESWB_EQRB_CORE_H
#define ESWB_EQRB_CORE_H

// FIXME make it platform independent
#include <pthread.h>
#include <unistd.h>

#include "eswb/event_queue.h"
#include "eswb/errors.h"
#include "eswb/types.h"
#include "eswb/services/eqrb.h"

#include "ids_map.h"


/**
 * Event queue replication bridge = EQRB
 */

typedef void* device_descr_t;

typedef struct eqrb_media_driver {
    char name[30];
    eqrb_rv_t (*connect)(void *param, device_descr_t *dh);
    eqrb_rv_t (*send)(device_descr_t dh, void *data, size_t bts, size_t *bs);
    eqrb_rv_t (*recv)(device_descr_t dh, void *data, size_t btr, size_t *br, uint32_t timeout);
    eqrb_rv_t (*command)(device_descr_t dh, eqrb_cmd_t cmd);
    eqrb_rv_t (*check_state)(device_descr_t dh);
    eqrb_rv_t (*disconnect)(device_descr_t dh);
} eqrb_media_driver_t;

typedef struct {
    pthread_t       tid;
    const eqrb_media_driver_t  *dev;
    char            *cmd_topic_path;
    void            *connectivity_params;
    eswb_topic_descr_t eq_td;

} eqrb_streaming_sideckick_t;

typedef struct {
    enum {
        SK_PAUSE = 0,
        SK_RUN,
        SK_QUIT
    } code;
} eqrb_streaming_sideckick_cmd_t;

typedef struct {
    void *connectivity_params;
    const eqrb_media_driver_t *driver;
    pthread_t tid;

} eqrb_handle_common_t;

typedef struct eqrb_server_handle {
    eqrb_handle_common_t h;

    const char *instance_name;

    void *connectivity_params_sk;

    eswb_topic_descr_t repl_root;
    eswb_topic_descr_t evq_td;
    eswb_topic_descr_t evq_sk_td;

    const char *cmd_bus_name;

} eqrb_server_handle_t;


typedef struct eqrb_client_handle {
    eqrb_handle_common_t h;

    eswb_topic_descr_t repl_dst_td;
    topic_id_map_t *ids_map;

    int launch_sidekick;
    int verbosity;

} eqrb_client_handle_t;

typedef enum {
    EQRB_CMD_CLIENT_REQ_SYNC = 0,
    EQRB_CMD_CLIENT_REQ_STREAM = 1,
    EQRB_CMD_SERVER_EVENT = 2,
    EQRB_CMD_SERVER_TOPIC = 3,
} eqrb_cmd_code_t;


typedef struct  __attribute__((packed)) eqrb_interaction_header {
    uint8_t msg_code;
    uint8_t reserved[3];
} eqrb_interaction_header_t;


typedef struct {
    eswb_topic_id_t current_tid;
    eswb_topic_id_t next_tid;
    int             do_send_data;

} eqrb_bus_sync_state_t;




#ifdef __cplusplus
extern "C" {
#endif

void eqrb_debug_msg(const char *fn, const char *txt, ...);

//#define EQRB_DEBUG

#ifdef EQRB_DEBUG
#define eqrb_dbg_msg(txt,...) eqrb_debug_msg(__func__, txt, ##__VA_ARGS__)
#else
#define eqrb_dbg_msg(txt,...) {}
#endif

eqrb_rv_t eqrb_server_instance_init(const char *eqrb_instance_name,
                                    const eqrb_media_driver_t *drv,
                                    void *conn_params,
                                    void *conn_params_sk,
                                    eqrb_server_handle_t **h);

eqrb_rv_t eqrb_client_start(eqrb_client_handle_t *h, const char *mount_point, size_t repl_map_size);
eqrb_rv_t eqrb_client_stop(eqrb_client_handle_t *h);

eqrb_rv_t
eqrb_server_start(eqrb_server_handle_t *h, const char *bus_to_replicate,
                  uint32_t ch_mask, uint32_t ch_mask_sk, const char **err_msg);

eqrb_rv_t eqrb_service_stop(eqrb_handle_common_t *h);




void *eqrb_alloc(size_t s);

#ifdef __cplusplus
}
#endif


#endif //ESWB_EQRB_CORE_H
