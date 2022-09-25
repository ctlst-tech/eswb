#include <eswb/services/sdtl.h>
#include <stdlib.h>
#include <string.h>
#include "../eqrb_priv.h"

typedef struct {
    const char *ch_name;
    const char *service_name;
} eqrb_drv_sdtl_params_t;

eqrb_rv_t eqrb_drv_sdtl_connect (void *param, device_descr_t *dh) {
    eqrb_drv_sdtl_params_t *p = (eqrb_drv_sdtl_params_t *)param;
    sdtl_channel_handle_t *chh;

    sdtl_service_t *service = sdtl_service_lookup(p->service_name);

    sdtl_rv_t rv = sdtl_channel_open(service, p->ch_name, &chh);

    *dh = chh;

    return rv == SDTL_OK ? eqrb_rv_ok : eqrb_media_err;
}

eqrb_rv_t eqrb_drv_sdtl_send (device_descr_t dh, void *data, size_t bts, size_t *bs) {

    sdtl_channel_handle_t *chh = (sdtl_channel_handle_t *) dh;

    sdtl_rv_t rv = sdtl_channel_send_data(chh, data, bts);

    switch (rv) {
        case SDTL_OK:
            *bs = bts;
            return eqrb_rv_ok;

        case SDTL_REMOTE_RX_NO_CLIENT:
        case SDTL_APP_RESET:
            return eqrb_media_reset_cmd;

        default:
            return eqrb_media_err;
    }
}

eqrb_rv_t eqrb_drv_sdtl_recv (device_descr_t dh, void *data, size_t btr, size_t *br) {
    sdtl_channel_handle_t *chh = (sdtl_channel_handle_t *) dh;

    sdtl_rv_t rv = sdtl_channel_recv_data(chh, data, btr, br);
    switch (rv) {
        case SDTL_OK:
            return eqrb_rv_ok;

        case SDTL_APP_RESET:
            return eqrb_media_reset_cmd;

        default:
            return eqrb_media_err;
    }
}

eqrb_rv_t eqrb_drv_sdtl_command (device_descr_t dh, eqrb_cmd_t cmd) {
    sdtl_channel_handle_t *chh = (sdtl_channel_handle_t *) dh;
    sdtl_rv_t rv;

    switch (cmd) {
        case eqrb_cmd_reset_remote:
            rv = sdtl_channel_send_cmd(chh, SDTL_PKT_CMD_CODE_RESET);
            break;

        case eqrb_cmd_reset_local_state:
            rv = sdtl_channel_reset_condition(chh);
            break;

        default:
            return eqrb_media_invarg;
    }

    return rv == SDTL_OK ? eqrb_rv_ok : eqrb_media_err;
}

int eqrb_drv_sdtl_disconnect (device_descr_t dh) {
//    return rv == SDTL_OK ? eqrb_rv_ok : eqrb_media_err;
    return SDTL_OK;
}

const driver_t eqrb_drv_sdtl = {
        .name = "eqrb_sdtl",
        .connect = eqrb_drv_sdtl_connect,
        .send = eqrb_drv_sdtl_send,
        .recv = eqrb_drv_sdtl_recv,
        .command = eqrb_drv_sdtl_command,
        .disconnect = eqrb_drv_sdtl_disconnect,
};

static eqrb_rv_t init_handle(eqrb_handle_common_t *h, const char *service_name, const char *sdtl_ch_name) {

    eqrb_drv_sdtl_params_t *params = calloc(1, sizeof(*params));

    if (params == NULL) {
        return eqrb_rv_nomem;
    }

    params->service_name = strdup(service_name);
    params->ch_name = strdup(sdtl_ch_name);

    h->driver = &eqrb_drv_sdtl;
    h->connectivity_params = params;

    return eqrb_rv_ok;
}


eqrb_rv_t
eqrb_sdtl_server_start(const char *service_name, const char *sdtl_ch_name, uint32_t ch_mask, const char *bus2replicate,
                       const char **err_msg) {

    eqrb_server_handle_t *sh = calloc(1, sizeof(*sh));

    if (sh == NULL) {
        return eqrb_rv_nomem;
    }

    eqrb_rv_t rv = init_handle(&sh->h, service_name, sdtl_ch_name);
    if (rv != eqrb_rv_ok) {
        return rv;
    }

    return eqrb_server_start(sh, bus2replicate, ch_mask, err_msg);
}


eqrb_rv_t eqrb_sdtl_client_connect(const char *service_name, const char *sdtl_ch_name, const char *mount_point,
                                   uint32_t repl_map_size) {

    eqrb_client_handle_t *ch = calloc(1, sizeof(*ch));

    if (ch == NULL) {
        return eqrb_rv_nomem;
    }

    eqrb_rv_t rv = init_handle(&ch->h, service_name, sdtl_ch_name);
    if (rv != eqrb_rv_ok) {
        return rv;
    }

    return eqrb_client_start(ch, mount_point, repl_map_size);
}

