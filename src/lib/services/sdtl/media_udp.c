#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "sdtl_opaque.h"

typedef struct {
    int rx_sock;
    struct sockaddr_in rx_addr;
    int tx_sock;
    struct sockaddr_in tx_addr;
} media_udp_inst_t;

sdtl_rv_t sdtl_media_udp_open(const char *path, void *params, void **h_rv) {
    sdtl_media_udp_params_t *udp_params = (sdtl_media_udp_params_t *)params;
    media_udp_inst_t *inst = calloc(1, sizeof(media_udp_inst_t));

    if (inst == NULL) {
        return SDTL_MEDIA_ERR;
    }

    inst->tx_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (inst->tx_sock < 0) {
        sdtl_dbg_msg("Failed to open tx socket: %s", strerror(errno));
        goto exit_fail;
    }

    inst->rx_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (inst->rx_sock < 0) {
        sdtl_dbg_msg("Failed to open rx socket: %s", strerror(errno));
        goto exit_fail;
    }

    uint16_t port_out = (uint16_t)atoi(udp_params->port_out);
    uint32_t ip_out;
    if (!inet_pton(AF_INET, udp_params->ip_out, &ip_out)) {
        sdtl_dbg_msg("Failed to convert ip_out address");
        goto exit_fail;
    }

    uint16_t port_in = (uint16_t)atoi(udp_params->port_in);
    uint32_t ip_in;
    if (!inet_pton(AF_INET, udp_params->ip_in, &ip_in)) {
        sdtl_dbg_msg("Failed to convert ip_in address");
        goto exit_fail;
    }

    inst->tx_addr.sin_family = AF_INET;
    inst->tx_addr.sin_port = htons(port_out);
    inst->tx_addr.sin_addr.s_addr = ip_out;

    inst->rx_addr.sin_family = AF_INET;
    inst->rx_addr.sin_port = htons(port_in);
    inst->rx_addr.sin_addr.s_addr = ip_in;

    if (bind(inst->rx_sock, (struct sockaddr *)&inst->rx_addr,
             sizeof(inst->rx_addr))) {
        sdtl_dbg_msg("Failed to bind rx socket: %s", strerror(errno));
        goto exit_fail;
    }

    sdtl_dbg_msg("Sockets successfully opened");
    char ip[16];
    inet_ntop(AF_INET, &ip_in, ip, INET_ADDRSTRLEN);
    sdtl_dbg_msg("Server: %s:%u", ip, port_in);
    inet_ntop(AF_INET, &ip_out, ip, INET_ADDRSTRLEN);
    sdtl_dbg_msg("Client: %s:%u", ip, port_out);

    *h_rv = inst;

    return SDTL_OK;

exit_fail:
    if (inst != NULL) {
        if (inst->rx_sock > 0) {
            close(inst->rx_sock);
        }
        if (inst->tx_sock > 0) {
            close(inst->tx_sock);
        }
        free(inst);
    }
    return SDTL_MEDIA_ERR;
}

// #define SDTL_MEDIA_UDP_LOOPBACK

sdtl_rv_t sdtl_media_udp_read(void *h, void *data, size_t l, size_t *lr) {
    media_udp_inst_t *inst = (media_udp_inst_t *)h;
    socklen_t addrlen = sizeof(inst->rx_addr);

#ifndef SDTL_MEDIA_UDP_LOOPBACK
    int rv = recvfrom(inst->rx_sock, data, l, 0,
                      (struct sockaddr *)&inst->rx_addr, &addrlen);
#else
    uint8_t buf[1024];
    int rv;
    while (1) {
        rv = recvfrom(inst->rx_sock, buf, sizeof(buf), 0,
                          (struct sockaddr *)&inst->rx_addr, &addrlen);
        sdtl_dbg_msg("Server: received %u bytes", (size_t)rv);
        rv = sendto(inst->tx_sock, buf, (size_t)rv, 0,
                    (struct sockaddr *)&inst->tx_addr,
                    (socklen_t)sizeof(inst->tx_addr));
        sdtl_dbg_msg("Server: sent %u bytes", (size_t)rv);
    }
#endif

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

const sdtl_service_media_t sdtl_media_udp = {.open = sdtl_media_udp_open,
                                             .read = sdtl_media_udp_read,
                                             .write = sdtl_media_udp_write,
                                             .close = sdtl_media_udp_close};
