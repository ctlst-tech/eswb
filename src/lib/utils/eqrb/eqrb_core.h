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




typedef enum {
    eqrb_duplex,
    eqrb_simplex
} eqrb_type_t;

typedef int device_descr_t;

typedef struct {
    char name[30];
    eqrb_rv_t (*connect)(const char *param, device_descr_t *dh);
    eqrb_rv_t (*send)(device_descr_t dh, void *data, size_t bts, size_t *bs);
    eqrb_rv_t (*recv)(device_descr_t dh, void *data, size_t btr, size_t *br);
    int (*disconnect)(device_descr_t dh); // should return non-zero to stop service instance
    eqrb_type_t type;
} driver_t;


#define EQRB_DOWNSTREAM_CODE_BUS_DATA    0x10

#define EQRB_UPSTREAM_CODE_CLIENT_READY  0x20
#define EQRB_UPSTREAM_CODE_RESEND_TID    0x21

#define EQRB_UPSTREAM_CODE_ACK           0x30


typedef enum {
    start_stream,
    stop_stream,
    send_proclaiming_info,
    ack,
    stop_service,
} server_command_code_t;

typedef struct {
    uint32_t code;
    uint32_t param;
} server_bus_commands_t;

struct eqrb_handle;

#define EQRB_COMMAND_TOPIC "cmd"
#define EQRV_SERVER_COMMAND_FIFO "srv_cmd_queue"
#define EQRB_CLIENT_COMMAND_FIFO "clnt_cmd_queue"

typedef struct {
    const driver_t *driver;

    device_descr_t dd;

    //eqrb_rv_t (*rx_got_frame_handler)(void *handle, uint8_t cmd_code, uint8_t *data, size_t data_len);

    pthread_t tid_rx_thread;
    pthread_t tid_tx_thread;

    eswb_topic_descr_t cmd_bus_root_td;

} eqrb_handle_common_t;

typedef struct eqrb_server_handle {
    eqrb_handle_common_t h;

    eswb_topic_descr_t repl_root;
    eswb_topic_descr_t evq_td;
    uint32_t eswb_reading_mask;

    eswb_topic_descr_t cmd_bus_publisher_td; // tx <--> rx part of services td (tx side)

} eqrb_server_handle_t;


typedef struct eqrb_client_handle {
    eqrb_handle_common_t h;

    eswb_topic_descr_t repl_dst_td;
    topic_id_map_t ids_map;

//    device_descr_t dr_tx_handle;

} eqrb_client_handle_t;



#ifdef __cplusplus
extern "C" {
#endif

void eqrb_debug_msg(const char *fn, const char *txt, ...);

//#define EQRB_DEBUG

#ifdef EQRB_DEBUG
//#error EQRB_DEBUG
#define eqrb_dbg_msg(txt,...) eqrb_debug_msg(__func__, txt, ##__VA_ARGS__)
#else
#define eqrb_dbg_msg(txt,...) {}
#endif

eqrb_rv_t eqrb_client_start(eqrb_client_handle_t *h, const char *mount_point, size_t repl_map_size);
eqrb_rv_t eqrb_client_stop(eqrb_client_handle_t *h);

eqrb_rv_t
eqrb_server_start(eqrb_server_handle_t *h, const char *bus_to_replicate, uint32_t ch_mask, const char **err_msg);
eqrb_rv_t eqrb_service_stop(eqrb_handle_common_t *h);

eqrb_rv_t eqrb_demo_start(const char *bus_to_replicate, const char *replication_point, uint32_t mask_to_replicate);
eqrb_rv_t eqrb_demo_stop();

#ifdef __cplusplus
}
#endif


#endif //ESWB_EQRB_CORE_H
