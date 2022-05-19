#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "eswb/api.h"
#include "../eqrb_core.h"

#ifndef ESWB_NO_SERIAL

eqrb_rv_t serial_connect (const char *param, device_descr_t *dh) {
    eqrb_rv_t rv;
    eswb_rv_t erv;


    return eqrb_rv_ok;
}

eqrb_rv_t serial_send (device_descr_t dh, void *data, size_t bts, size_t *bs) {
    ssize_t bw;
    bw = write(dh, data, bts);

    eqrb_dbg_msg("write (sd %d) bw == %d. %s", dh, bw, bw == -1 ? strerror(errno) : "");

    if (bw > 0) {
        if (bs != NULL) {
            *bs = bw;
        }
        return eqrb_rv_ok;
    } else if (bw == 0) {
        return eqrb_media_stop;
    } else {
        if (errno == EPIPE) {
            return eqrb_media_stop;
        } else {
            return eqrb_media_err;
        }
    }
}

eqrb_rv_t serial_recv (device_descr_t dh, void *data, size_t btr, size_t *brr) {
    ssize_t br;

    br = read(dh, data, btr);

    eqrb_dbg_msg("read (sd %d) br == %d. %s", dh, br, br == -1 ? strerror(errno) : "");

    if (br > 0) {
        if (brr != NULL) {
            *brr = br;
        }
        return eqrb_rv_ok;
    } else if (br == 0) {
        return eqrb_media_stop;
    } else {
        if (errno == EPIPE) {
            return eqrb_media_stop;
        } else {
            return eqrb_media_err;
        }
    }
}

int serial_disconnect (device_descr_t dh) {
    eqrb_dbg_msg("");
//    close(dh);
    return 0;
}

const driver_t serial_driver = {
        .name = "serial",
        .connect = serial_connect,
        .send = serial_send,
        .recv = serial_recv,
        .disconnect = serial_disconnect,
        .type = eqrb_duplex
};

static eqrb_rv_t open_serial(const char *path, uint32_t baudrate, int *fd_rv) {
    int fd;
    struct termios 	termios_p;

    fd = open(path, O_RDWR);
    if (fd == -1) {
        eqrb_dbg_msg("Failed to connect to \"%s\": %s", path, strerror(errno));
        return eqrb_media_err;
    }

    tcgetattr(fd, &termios_p);
    cfsetispeed(&termios_p, baudrate);
    cfsetospeed(&termios_p, baudrate);
    tcsetattr(fd, TCSANOW, &termios_p);
    tcflush(fd, TCIOFLUSH);

    *fd_rv = fd;

    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_serial_server_start(const char *path, uint32_t baudrate, const char *bus2replicate) {

    eqrb_server_handle_t *sh;
    sh = calloc(1, sizeof(*sh)); // FIXME never freed
    int fd;

    eqrb_rv_t rv = open_serial(path, baudrate, &fd);
    if (rv != eqrb_rv_ok) {
        return rv;
    }

    sh->h.dd = fd;
    sh->h.driver = &serial_driver;

    return eqrb_server_start(sh, bus2replicate, 0xffffffff, NULL);
}



eqrb_rv_t
eqrb_serial_client_connect(const char *path, uint32_t baudrate,
                        const char *mount_point, uint32_t repl_map_size) {

    eqrb_client_handle_t *ch = calloc(1, sizeof(*ch));
    if (ch == NULL) {
        return eqrb_rv_nomem;
    }
    int fd;
    eqrb_rv_t rv = open_serial(path, baudrate, &fd);
    if (rv != eqrb_rv_ok){
        free(ch);
        return rv;
    }

    ch->h.driver = &serial_driver;
    ch->h.dd = fd;

    return eqrb_client_start(ch, mount_point, repl_map_size);
}

#else

eqrb_rv_t eqrb_serial_server_start(const char *path, uint32_t baudrate, const char *bus2replicate) {

    return eqrb_notsup;

}



eqrb_rv_t
eqrb_serial_client_connect(const char *path, uint32_t baudrate,
                        const char *mount_point, uint32_t repl_map_size) {
    return eqrb_notsup;
}

#endif

