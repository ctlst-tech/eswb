#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
// #include <malloc.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "sdtl_opaque.h"

#ifdef ESWB_NO_SOCKET

sdtl_rv_t sdtl_media_udp_open(const char *path, void *params, void **h_rv) {
    return SDTL_SYS_ERR;
}

sdtl_rv_t sdtl_media_udp_read(void *h, void *data, size_t l, size_t *lr) {
    return SDTL_SYS_ERR;
}

sdtl_rv_t sdtl_media_udp_write(void *h, void *data, size_t l) {
    return SDTL_SYS_ERR;
}

sdtl_rv_t sdtl_media_udp_close(void *h) {
    return SDTL_SYS_ERR;
}

#else

typedef struct {
    int rx_sock;
    struct sockaddr_in rx_addr;
    int tx_sock;
    struct sockaddr_in tx_addr;
} media_udp_inst_t;


static int enable_port_reuse(int sock)
{
    int opt = 1;
    /* Ignore the error handling on purpose – failure is not fatal,
       but try both options when they are available. */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
    return 0;
}


sdtl_rv_t sdtl_media_udp_open(const char *path, void *params, void **h_rv)
{
    sdtl_media_udp_params_t *udp_params = (sdtl_media_udp_params_t *)params;
    media_udp_inst_t        *inst       = calloc(1, sizeof(media_udp_inst_t));
    if (inst == NULL) {
        return SDTL_MEDIA_ERR;
    }

    // Convert ports / addresses first so we know whether ports match.
    uint16_t port_out = (uint16_t)atoi(udp_params->port_out);
    uint16_t port_in  = (uint16_t)atoi(udp_params->port_in);
    int same_port = (port_in == port_out);

    uint32_t ip_out, ip_in;
    if (!inet_pton(AF_INET, udp_params->ip_out, &ip_out)) {
        sdtl_dbg_msg("Failed to convert ip_out address");
        goto exit_fail;
    }
    if (!inet_pton(AF_INET, udp_params->ip_in, &ip_in)) {
        sdtl_dbg_msg("Failed to convert ip_in address");
        goto exit_fail;
    }

    // Create sockets.
    //    - Always create RX first (we must bind it).
    //    - If ports match, TX will just reuse RX.
    inst->rx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (inst->rx_sock < 0) {
        sdtl_dbg_msg("Failed to open rx socket: %s", strerror(errno));
        goto exit_fail;
    }
    enable_port_reuse(inst->rx_sock);

    if (!same_port) {
        inst->tx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (inst->tx_sock < 0) {
            sdtl_dbg_msg("Failed to open tx socket: %s", strerror(errno));
            goto exit_fail;
        }
        enable_port_reuse(inst->tx_sock);
    } else {
        /* Share the socket when ports coincide */
        inst->tx_sock = inst->rx_sock;
    }

    // Prepare socket addresses.
    inst->rx_addr.sin_family      = AF_INET;
    inst->rx_addr.sin_port        = htons(port_in);
    inst->rx_addr.sin_addr.s_addr = ip_in;

    inst->tx_addr.sin_family      = AF_INET;
    inst->tx_addr.sin_port        = htons(same_port ? port_in : port_out);
    inst->tx_addr.sin_addr.s_addr = ip_out;

    // Bind RX socket.
    if (bind(inst->rx_sock,
             (struct sockaddr *)&inst->rx_addr, sizeof(inst->rx_addr)) != 0) {
        sdtl_dbg_msg("Failed to bind rx socket: %s", strerror(errno));
        goto exit_fail;
    }

    // 5. Success path.
    char ip[INET_ADDRSTRLEN];
    sdtl_dbg_msg("Sockets successfully opened");

    inet_ntop(AF_INET, &ip_in, ip, sizeof(ip));
    sdtl_dbg_msg("Server: %s:%u", ip, port_in);

    inet_ntop(AF_INET, &ip_out, ip, sizeof(ip));
    sdtl_dbg_msg("Client: %s:%u", ip,
                 (unsigned)(same_port ? port_in : port_out));

    *h_rv = inst;
    return SDTL_OK;

exit_fail:
    if (inst) {
        if (inst->rx_sock > 0) {
            close(inst->rx_sock);
        }
        if (!same_port && inst->tx_sock > 0) {   /* avoid double-close */
            close(inst->tx_sock);
        }
        free(inst);
    }
    return SDTL_MEDIA_ERR;
}


sdtl_rv_t sdtl_media_udp_read(void *h, void *data, size_t l, size_t *lr) {
    media_udp_inst_t *inst = (media_udp_inst_t *)h;
    socklen_t addrlen = sizeof(inst->rx_addr);

    int rv = recvfrom(inst->rx_sock, data, l, 0,
                      (struct sockaddr *)&inst->rx_addr, &addrlen);
    sdtl_dbg_msg("Received %u bytes", (size_t)rv);
    *lr = rv;
    if (rv == -1) {
        return SDTL_MEDIA_ERR;
    }
    return rv == 0 ? SDTL_MEDIA_EOF : SDTL_OK;
}

sdtl_rv_t sdtl_media_udp_write(void *h, void *data, size_t l) {
    media_udp_inst_t *inst = (media_udp_inst_t *)h;
    int rv = sendto(inst->tx_sock, data, (size_t)l, 0,
                   (struct sockaddr *)&inst->tx_addr,
                   (socklen_t)sizeof(inst->tx_addr));
    sdtl_dbg_msg("Sent %u bytes", (size_t)rv);
    if (rv == -1) {
        return SDTL_MEDIA_ERR;
    }

    return SDTL_OK;
}

sdtl_rv_t sdtl_media_udp_close(void *h) {
    media_udp_inst_t *inst = (media_udp_inst_t *)h;
    int rv = close(inst->rx_sock);
    if (rv) {
        sdtl_dbg_msg("Failed to close rx socket: %s", strerror(errno));
    }
    rv = close(inst->tx_sock);
    if (rv) {
        sdtl_dbg_msg("Failed to close tx socket: %s", strerror(errno));
    }

    return rv == 0 ? SDTL_OK : SDTL_MEDIA_ERR;
}

#endif

const sdtl_service_media_t sdtl_media_udp = {.open = sdtl_media_udp_open,
                                             .read = sdtl_media_udp_read,
                                             .write = sdtl_media_udp_write,
                                             .close = sdtl_media_udp_close};

