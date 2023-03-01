#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../eqrb_priv.h"

#define EQRB_FILE_MAX_NAME_LEN 64
#define EQRB_FILE_SAME_PREFIX_MAX_FILES 100
#define EQRB_FILE_MAX_DESC 100

typedef struct {
    const char *file_prefix;
    const char *file_name;
    const char *dir;
    const char *bus;
    struct timespec tp;
} eqrb_drv_file_params_t;

eqrb_rv_t eqrb_drv_file_connect(void *param, device_descr_t *dh);
eqrb_rv_t eqrb_drv_file_send(device_descr_t dh, void *data, size_t bts,
                             size_t *bs);
eqrb_rv_t eqrb_drv_file_recv(device_descr_t dh, void *data, size_t bts,
                             size_t *bs, uint32_t timeout);
eqrb_rv_t eqrb_drv_file_command(device_descr_t dh, eqrb_cmd_t cmd);
eqrb_rv_t eqrb_drv_file_check_state(device_descr_t dh);
eqrb_rv_t eqrb_drv_file_disconnect(device_descr_t dh);
eqrb_rv_t eqrb_drv_file_update_timestamp_us(device_descr_t dh, uint32_t *ts,
                                            uint32_t *dts);

const eqrb_media_driver_t eqrb_drv_file = {
    .name = "eqrb_file",
    .connect = eqrb_drv_file_connect,
    .send = eqrb_drv_file_send,
    .recv = eqrb_drv_file_recv,
    .command = eqrb_drv_file_command,
    .check_state = eqrb_drv_file_check_state,
    .disconnect = eqrb_drv_file_disconnect,
};

static const char head[5] = "ebdf";
static eqrb_drv_file_params_t *file_params[EQRB_FILE_MAX_DESC] = {0};

eqrb_rv_t eqrb_drv_file_connect(void *param, device_descr_t *dh) {
    eqrb_rv_t rv;
    eqrb_drv_file_params_t *p = (eqrb_drv_file_params_t *)param;
    char file_name[EQRB_FILE_MAX_NAME_LEN];
    int file_num = 0;
    int fd;

    if (dh == NULL || p->dir == NULL) {
        return eqrb_media_invarg;
    }

    if (mkdir(p->dir, 0) && errno != EEXIST) {
        return eqrb_media_invarg;
    }

    if (p->file_prefix != NULL) {
        do {
            snprintf(file_name, EQRB_FILE_MAX_NAME_LEN, "%s/%s_%d.eqrb", p->dir,
                     p->file_prefix, file_num);
            fd = open(file_name, O_RDWR | O_CREAT | O_EXCL);
            file_num++;
        } while (fd < 0 && errno == EEXIST &&
                 file_num < EQRB_FILE_SAME_PREFIX_MAX_FILES);
    } else if (p->file_name != NULL) {
        snprintf(file_name, EQRB_FILE_MAX_NAME_LEN, "%s/%s", p->dir,
                 p->file_name);
        fd = open(file_name, O_RDONLY);
    }

    if (fd > 0 && fd < EQRB_FILE_MAX_DESC) {
        file_params[fd] = param;
        *(int *)dh = fd;
        const char bus[32];
        // chmod(file_name,  S_IRWXO | S_IRWXG | S_IRWXU);
        int bus_len = snprintf(bus, sizeof(bus), "%s", p->bus);
        size_t bw = write(fd, bus, bus_len);
        sync();
        if (bw > 0) {
            rv = eqrb_rv_ok;
        } else {
            rv = eqrb_media_err;
        }
    } else {
        rv = eqrb_media_invarg;
    }

    return rv;
}

eqrb_rv_t eqrb_drv_file_send(device_descr_t dh, void *data, size_t bts,
                             size_t *bs) {
    char head_time[32];
    int head_time_len;
    uint32_t t, dt;

    eqrb_drv_file_update_timestamp_us(dh, &t, &dt);
    head_time_len =
        snprintf(head_time, sizeof(head_time), "\n%s%f", head, t / 1000000.0);

    size_t bw = write((int)dh, head_time, head_time_len);
    sync();
    bw += write((int)dh, data, bts);
    sync();

    if (bw > 0) {
        if (bs != NULL) {
            *bs = bw;
        }
        return eqrb_rv_ok;
    } else if (bw == 0) {
        return eqrb_media_stop;
    }

    return eqrb_media_err;
}

eqrb_rv_t eqrb_drv_file_recv(device_descr_t dh, void *data, size_t bts,
                             size_t *bs, uint32_t timeout) {
    size_t bw = read((int)dh, data, bts);

    if (bw > 0) {
        if (bs != NULL) {
            *bs = bw;
        }
        return eqrb_rv_ok;
    } else if (bw == 0) {
        eqrb_interaction_header_t *rv = (eqrb_interaction_header_t *)data;
        rv->msg_code = EQRB_CMD_CLIENT_REQ_SYNC;
        return eqrb_rv_ok;
    }

    // TODO: Different behavior for server and client

    return eqrb_media_err;
}

eqrb_rv_t eqrb_drv_file_command(device_descr_t dh, eqrb_cmd_t cmd) {
    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_drv_file_check_state(device_descr_t dh) {
    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_drv_file_disconnect(device_descr_t dh) {
    return eqrb_rv_ok;
}

static eqrb_rv_t init_media_params(void **media_params, const char *file_prefix,
                                   const char *file_name, const char *dir,
                                   const char *bus) {
    eqrb_drv_file_params_t *params = calloc(1, sizeof(*params));

    if (params == NULL) {
        return eqrb_rv_nomem;
    }

    if (file_prefix != NULL) {
        params->file_prefix = strdup(file_prefix);
    } else {
        params->file_prefix = NULL;
    }

    if (file_name != NULL) {
        params->file_name = strdup(file_name);
    } else {
        params->file_name = NULL;
    }

    if (bus != NULL) {
        params->bus = strdup(bus);
    } else {
        params->bus = NULL;
    }

    if (dir != NULL) {
        params->dir = strdup(dir);
    } else {
        params->dir = NULL;
    }

    *media_params = params;

    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_file_server_start(const char *eqrb_service_name,
                                 const char *file_prefix, const char *dst_dir,
                                 const char *bus2replicate,
                                 const char **err_msg) {
    void *mp;
    eqrb_rv_t rv;
    eqrb_server_handle_t *sh;

    rv = init_media_params(&mp, file_prefix, NULL, dst_dir, bus2replicate);
    if (rv != eqrb_rv_ok) {
        return rv;
    }

    eqrb_server_instance_init(eqrb_service_name, &eqrb_drv_file, mp, NULL, &sh);

    uint32_t ch_mask_main = 0xFFFFFFFF;
    uint32_t ch_mask_sk = 0;

    return eqrb_server_start(sh, bus2replicate, ch_mask_main, ch_mask_sk,
                             err_msg);
}

static eqrb_rv_t instantiate_client(const char *service_name,
                                    const char *file_name, const char *src_dir,
                                    const char *mount_point,
                                    uint32_t repl_map_size) {
    eqrb_client_handle_t *ch = calloc(1, sizeof(*ch));

    if (ch == NULL) {
        return eqrb_rv_nomem;
    }

    eqrb_drv_file_params_t *params = calloc(1, sizeof(*params));

    if (params == NULL) {
        return eqrb_rv_nomem;
    }

    params->dir = strdup(src_dir);
    params->file_name = strdup(file_name);
    params->file_prefix = NULL;

    ch->h.driver = &eqrb_drv_file;
    ch->h.connectivity_params = params;

    return eqrb_client_start(ch, mount_point, repl_map_size);
}

eqrb_rv_t eqrb_file_client_connect(const char *service_name,
                                   const char *file_name, const char *src_dir,
                                   const char *mount_point,
                                   uint32_t repl_map_size,
                                   const char **err_msg) {
    eqrb_rv_t rv = instantiate_client(service_name, file_name, src_dir,
                                      mount_point, repl_map_size);
    if (rv != eqrb_rv_ok) {
        *err_msg = strdup("instantiate_client failed");
        return rv;
    }

    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_drv_file_update_timestamp_us(device_descr_t dh, uint32_t *ts,
                                            uint32_t *dts) {
    int fd = (int)dh;

    if (ts == NULL || file_params[fd] == NULL) {
        return eqrb_media_invarg;
    }

    struct timespec *tp = &file_params[fd]->tp;
    uint32_t prev = tp->tv_sec * 1000000 + tp->tv_nsec / 1000;
    clock_gettime(CLOCK_REALTIME, &file_params[fd]->tp);
    uint32_t current = tp->tv_sec * 1000000 + tp->tv_nsec / 1000;
    *ts = current;
    *dts = current - prev;

    return eqrb_rv_ok;
}
