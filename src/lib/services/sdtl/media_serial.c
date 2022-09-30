#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

#include "sdtl_opaque.h"


#define CHEAT_CAST(h__) ((size_t) (h__))

sdtl_rv_t sdtl_media_serial_open(const char *path, void *params, void **h_rv) {
    sdtl_media_serial_params_t *ser_params = (sdtl_media_serial_params_t *) params;
    struct termios 	termios_p;

    sdtl_dbg_msg("entered");

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        sdtl_dbg_msg("Failed to connect to \"%s\": %s", path, strerror(errno));

        if (errno == ENOENT) {
            return SDTL_MEDIA_NO_ENTITY;
        } else {
            return SDTL_MEDIA_ERR;
        }
    }

    int rv = tcgetattr(fd, &termios_p);
    if (rv) {
        sdtl_dbg_msg("tcgetattr failed: %s", path, strerror(errno));
        return SDTL_MEDIA_ERR;
    }

    cfsetispeed(&termios_p, ser_params->baudrate);
    cfsetospeed(&termios_p, ser_params->baudrate);
    rv = tcsetattr(fd, TCSANOW, &termios_p);
    if (rv) {
        sdtl_dbg_msg("tcsetattr failed: %s", path, strerror(errno));
        return SDTL_MEDIA_ERR;
    }

    tcflush(fd, TCIOFLUSH);

    *h_rv = (void *) ((size_t) fd);  // cheating for not allocating 4 bytes

    return SDTL_OK;
}

sdtl_rv_t sdtl_media_serial_read(void *h, void *data, size_t l, size_t *lr) {
    int fd = CHEAT_CAST(h);

    int rv = read(fd, data, (int)l);
    if (rv == -1) {
        return SDTL_MEDIA_ERR;
    }
    *lr = rv;
    return rv == 0 ? SDTL_MEDIA_EOF : SDTL_OK;
}

sdtl_rv_t sdtl_media_serial_write(void *h, void *data, size_t l) {
    int fd = CHEAT_CAST(h);

    int rv = write(fd, data, (int)l);
    if (rv == -1) {
        return SDTL_MEDIA_ERR;
    }

    return SDTL_OK;
}

sdtl_rv_t sdtl_media_serial_close(void *h) {
    int fd = CHEAT_CAST(h);
    int rv = close(fd);

    return rv == 0 ? SDTL_OK : SDTL_MEDIA_ERR;
}

const sdtl_service_media_t sdtl_media_serial = {
    .open = sdtl_media_serial_open,
    .read = sdtl_media_serial_read,
    .write = sdtl_media_serial_write,
    .close = sdtl_media_serial_close
};
