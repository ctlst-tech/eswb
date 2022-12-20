#include <stdlib.h>
#include <string.h>

#include "../eqrb_priv.h"

typedef struct {
    const char *file_name;
} eqrb_drv_file_params_t;

eqrb_rv_t eqrb_drv_file_connect(void *param, device_descr_t *dh);
eqrb_rv_t eqrb_drv_file_send(device_descr_t dh, void *data, size_t bts,
                             size_t *bs);
eqrb_rv_t eqrb_drv_file_recv(device_descr_t dh, void *data, size_t bts,
                             size_t *bs, uint32_t timeout);
eqrb_rv_t eqrb_drv_file_command(device_descr_t dh, eqrb_cmd_t cmd);
eqrb_rv_t eqrb_drv_file_check_state(device_descr_t dh);
eqrb_rv_t eqrb_drv_file_disconnect(device_descr_t dh);

const eqrb_media_driver_t eqrb_drv_file = {
    .name = "eqrb_file",
    .connect = eqrb_drv_file_connect,
    .send = eqrb_drv_file_send,
    .recv = eqrb_drv_file_recv,
    .command = eqrb_drv_file_command,
    .check_state = eqrb_drv_file_check_state,
    .disconnect = eqrb_drv_file_disconnect,
};
