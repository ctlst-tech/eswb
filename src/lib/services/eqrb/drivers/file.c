#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "../eqrb_priv.h"

#define EQRB_FILE_MAX_NAME_LEN 64
#define EQRB_FILE_MAX_SEP_LEN 8
#define EQRB_FILE_SYNC_TIMEOUT 100
#define EQRB_FILE_BUF_SIZE 512
#define EQRB_FILE_MAX_FILES 128

typedef struct {
    char *file_prefix;
    char *file_name;
    char *dir;
    char *bus;
    char *sep;
    uint16_t offset;
    uint8_t *buf;
    struct timespec ts;
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

static eqrb_drv_file_params_t *file_params[EQRB_FILE_MAX_FILES] = {0};

eqrb_rv_t eqrb_drv_file_connect(void *param, device_descr_t *dh) {
    eqrb_rv_t rv;
    eqrb_drv_file_params_t *p = (eqrb_drv_file_params_t *)param;
    char file_name[EQRB_FILE_MAX_NAME_LEN];
    int file_num = 0;
    int fd = -1;

    if (dh == NULL || p->dir == NULL) {
        return eqrb_media_invarg;
    }

    if (mkdir(p->dir, 0) && errno != EEXIST) {
        return eqrb_media_invarg;
    }

    if (strlen(p->sep) > EQRB_FILE_MAX_SEP_LEN) {
        return eqrb_media_invarg;
    }

    if (p->file_prefix != NULL) {
        do {
            snprintf(file_name, EQRB_FILE_MAX_NAME_LEN, "%s/%s_%d.eqrb", p->dir,
                     p->file_prefix, file_num);
            fd = open(file_name, O_RDWR | O_CREAT | O_EXCL);
            file_num++;
        } while (fd < 0 && errno == EEXIST && file_num < EQRB_FILE_MAX_FILES);
    } else if (p->file_name != NULL) {
        snprintf(file_name, EQRB_FILE_MAX_NAME_LEN, "%s/%s", p->dir,
                 p->file_name);
        fd = open(file_name, O_RDONLY);
    }

    if (fd > 0) {
        file_params[fd] = param;
        *(int *)dh = fd;
        // chmod(file_name,  S_IRWXO | S_IRWXG | S_IRWXU);
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%s\n", p->bus);
        len += snprintf(buf + len, sizeof(buf), "%s\n", p->sep);
        size_t bw = write(fd, buf, len);
        fsync(fd);
        if (bw > 0) {
            rv = eqrb_rv_ok;
        } else {
            rv = eqrb_media_err;
        }
    } else {
        rv = eqrb_media_invarg;
    }

    if (rv == eqrb_rv_ok) {
        file_params[fd]->offset = 0;
        clock_gettime(CLOCK_MONOTONIC, &file_params[fd]->ts);
        file_params[fd]->buf = calloc(2 * EQRB_FILE_BUF_SIZE, 1);
    }

    return rv;
}

size_t eqrb_drv_file_write(device_descr_t dh, void *data, size_t size) {
    int fd = (int)dh;
    eqrb_drv_file_params_t *fp = file_params[fd];
    ssize_t rv = 0;

    if (fp->offset >= EQRB_FILE_BUF_SIZE) {
        rv = write(fd, fp->buf, EQRB_FILE_BUF_SIZE);
        if (rv < 0) {
            eqrb_dbg_msg("write failed");
            return -1;
        }
        rv = fsync(fd);
        if (rv < 0) {
            eqrb_dbg_msg("sync failed");
            return -1;
        }
        fp->offset -= EQRB_FILE_BUF_SIZE;
        memcpy(fp->buf, fp->buf + EQRB_FILE_BUF_SIZE, fp->offset);
    }
    memcpy(fp->buf + fp->offset, data, size);
    fp->offset += size;

    // struct timespec ts;
    // clock_gettime(CLOCK_MONOTONIC, &ts);
    // long unsigned diff = (ts.tv_sec - fp->ts.tv_sec) * 1000 + (ts.tv_nsec - fp->ts.tv_nsec) / 1000000;
    // if (diff > EQRB_FILE_SYNC_TIMEOUT) {
    //     fsync(fd);
    //     fp->ts = ts;
    // }

    return size;
}

eqrb_rv_t eqrb_drv_file_send(device_descr_t dh, void *data, size_t bts,
                             size_t *bs) {
    int fd = (int)dh;
    char sep[32];
    int sep_len;

    sep_len = snprintf(sep, sizeof(sep) - 1, "\n%s", file_params[fd]->sep);

    size_t bw = eqrb_drv_file_write(dh, sep, sep_len);
    bw += eqrb_drv_file_write(dh, data, bts);

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
                                   const char *bus,
                                   const char *frame_separator) {
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

    if (frame_separator != NULL) {
        params->sep = strdup(frame_separator);
    } else {
        params->sep = NULL;
    }

    *media_params = params;

    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_file_server_start(const char *eqrb_service_name,
                                 const char *file_prefix, const char *dst_dir,
                                 const char *bus2replicate,
                                 const char *frame_separator,
                                 const char **err_msg) {
    void *mp;
    eqrb_rv_t rv;
    eqrb_server_handle_t *sh;

    rv = init_media_params(&mp, file_prefix, NULL, dst_dir, bus2replicate,
                           frame_separator);
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
