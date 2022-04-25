//
// Created by goofy on 1/9/22.
//

#include <pthread.h>

#include "eswb/api.h"
#include "../eqrb_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include <signal.h>

void ignore_sigpipe() {
    signal(SIGPIPE, SIG_IGN);
}

#define EQRB_TCP_PORT_DEFAULT 2333

#define skt_err(txt,...) eqrb_dbg_msg(txt ": %s", ##__VA_ARGS__, strerror(errno))


eqrb_rv_t tcp_connect (const char *param, device_descr_t *dh) {
    eqrb_rv_t rv;
    eswb_rv_t erv;

    // FIXME deprecated, delete this
    int server = 1;
    
    if (server) {
        
    } else {
        
    }

    eqrb_dbg_msg("");
    
    return eqrb_rv_ok;
}
eqrb_rv_t tcp_send (device_descr_t dh, void *data, size_t bts, size_t *bs) {
    ssize_t bw;
    bw = send(dh, data, bts, 0);

    eqrb_dbg_msg("send (sd %d) bw == %d. %s", dh, bw, bw == -1 ? strerror(errno) : "");

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

eqrb_rv_t tcp_recv (device_descr_t dh, void *data, size_t btr, size_t *brr) {
    ssize_t br;

    br = recv(dh, data, btr, 0);

    eqrb_dbg_msg("recv (sd %d) br == %d. %s", dh, br, br == -1 ? strerror(errno) : "");

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

int tcp_disconnect (device_descr_t dh) {
    skt_err("thread for client_sd == %d quits", dh);
    eqrb_dbg_msg("");
    close(dh);
//    pthread_exit(NULL);
    return 1;
}

const driver_t tcp_driver = {
        .name = "tcp",
        .connect = tcp_connect,
        .send = tcp_send,
        .recv = tcp_recv,
        .disconnect = tcp_disconnect,
        .type = eqrb_duplex
};

int receive_msg_line(int sktd, char *line, int max_len) {
    char *pp = line;

    // TODO rely it on calls above
    int br;
    do {
        char b;
        br = recv(sktd, &b, 1, 0);
        if (b == '\n') {
            *pp++ = 0;
            break;
        } else {
            *pp++ = b;
        }
        if ((pp - line) >= max_len) {
            return -1;
        }
    } while(br == 1);

    return br;
}

eqrb_rv_t eqrb_start_serving_thread(int client_sd) {
    eqrb_server_handle_t *sh;

    char bus2replicate[ESWB_TOPIC_MAX_PATH_LEN + 1];

    int br = receive_msg_line(client_sd, bus2replicate, sizeof(bus2replicate));

    if (br == -1) {
        return eqrb_media_err;
    }

    sh = calloc(1, sizeof(*sh)); // FIXME it is never freed then (must be freed at threads termination)

    sh->h.dd = client_sd;
    sh->h.driver = &tcp_driver;

    eqrb_dbg_msg("Connecting to \"%s\" ...", bus2replicate);
    const char *err_msg;
    eqrb_rv_t rv = eqrb_server_start(sh, bus2replicate, 0xffffffff, &err_msg);

    if (rv == eqrb_rv_ok) {
        printf("Sucessfull connection to \"%s\"\n", bus2replicate);
#define OK_MSG "OK\n"
        send(client_sd, OK_MSG, strlen(OK_MSG), 0);
    } else {
        char msg[256];
        sprintf(msg, "ERR: %s\n", err_msg);
        send(client_sd, msg, strlen(msg), 0);
        close(client_sd);
    }

    return rv;
}

eqrb_rv_t eqrb_tcp_server(uint16_t port) {
    struct sockaddr_in addr;
    int clientsd;
    socklen_t addrlen;
    struct timeval to;
    int rc;

    int sktd;

    if (port == 0) {
        port = EQRB_TCP_PORT_DEFAULT;
    }

    do {
        // TODO cancelation handler for sktd

        sktd = socket(AF_INET, SOCK_STREAM, 0);
        if (sktd == -1) {
            skt_err("socket failed");
            break;
        }

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        addrlen = sizeof(addr);

        to.tv_sec = 4;
        to.tv_usec = 0;

        setsockopt(sktd, SOL_SOCKET, SO_REUSEPORT, &rc, sizeof(rc));
        setsockopt(sktd, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc));
        setsockopt(sktd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
        setsockopt(sktd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
        int set = 1;
        // setsockopt(sktd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
        ignore_sigpipe();

        if (bind(sktd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
            skt_err("bind failed");
            break;
        }

        if (listen(sktd, 20) != 0) {
            skt_err("listen failed");
            break;
        }

        // TODO cancelation handler for connected descriptors

        while (1) {
            clientsd = accept(sktd, (struct sockaddr *) &addr, &addrlen);
            if (clientsd > 0) {
                eqrb_rv_t rv = eqrb_start_serving_thread(clientsd);
                if (rv != eqrb_rv_ok) {
                    skt_err("eqrb_start_serving_thread failed: %d", rv);
                }
            } else {
                skt_err("accept failed");
                break;
            }
        }
    } while (0);

    return eqrb_media_err;
}

static void* eqrb_tcp_server_thread(void *p) {
    uint16_t port = *((uint16_t *) p);

    pthread_setname_np("tcp_server");

    static eqrb_rv_t rv;
    rv = eqrb_tcp_server(port);

    return &rv;
}

static pthread_t eqrb_tcp_server_tid = 0;

eqrb_rv_t eqrb_tcp_server_start(uint16_t p) {
    if (eqrb_tcp_server_tid > 0) {
        return eqrb_server_already_launched;
    }

    static uint16_t port;
    port = p;
    int rv = pthread_create(&eqrb_tcp_server_tid, NULL, eqrb_tcp_server_thread, &port);

    return rv == 0 ? eqrb_rv_ok : eqrb_os_based_err;
}

eqrb_rv_t eqrb_tcp_server_stop() {
    if (eqrb_tcp_server_tid == 0) {
        return eqrb_server_already_launched;
    }

    // TODO cancel all client threads

    pthread_cancel(eqrb_tcp_server_tid);
    pthread_join(eqrb_tcp_server_tid, NULL);
//    eqrb_tcp_server_tid = 0;

    return eqrb_rv_ok;
}

eqrb_rv_t eqrb_tcp_client_create(eqrb_client_handle_t **rv_ch) {
    eqrb_client_handle_t *ch;

    ch = calloc(1, sizeof(*ch)); // FIXME it is never freed then (must be freed at threads termination)
    if (ch == NULL) {
        return eqrb_rv_nomem;
    }
    ch->h.driver = &tcp_driver;

    *rv_ch = ch;

    return eqrb_rv_ok;
}


eqrb_rv_t
eqrb_tcp_client_connect(eqrb_client_handle_t *ch, const char *addr_str, const char *bus2replicate,
                        const char *mount_point, uint32_t repl_map_size, char *err_msg) {

    char host [255];
    int port = 0;
    int sktd;

#define PASS_SKT_ERR(__err) {if (err_msg != NULL) {strncpy(err_msg, strerror(__err), EQRB_ERR_MSG_MAX_LEN);}}

    if ( sscanf(addr_str, "%[^:]:%d", host, &port) < 1 ) {
        PASS_SKT_ERR(EINVAL);
        return eqrb_media_invarg;
    }
    if (port == 0) {
        port = EQRB_TCP_PORT_DEFAULT;
    }

    struct sockaddr_in addr;
    socklen_t addrlen;

    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(host, &addr.sin_addr);
    addrlen = sizeof(addr);

    sktd = socket( AF_INET, SOCK_STREAM, 0 );
    if (sktd == -1) {
        PASS_SKT_ERR(errno);
        return eqrb_media_err;
    }

    if (connect(sktd, (struct sockaddr *)&addr, addrlen) != 0 ) {
        skt_err("connect failed");
        PASS_SKT_ERR(errno);
        return eqrb_media_err;
    }

    send(sktd, bus2replicate, strlen(bus2replicate), 0);
    send(sktd, "\n", 1, 0);

    char reply_msg[256];
    int br = receive_msg_line(sktd, reply_msg, sizeof(reply_msg));
    if (br == -1) {
        skt_err("reply receive failed");
        return eqrb_media_err;
    }

    if (strcmp(reply_msg, "OK") != 0) {
//        fprintf(stderr, "Client connection error: %s\n", reply_msg);
        strncpy(err_msg, reply_msg, EQRB_ERR_MSG_MAX_LEN);
        return eqrb_media_err;
    }

    ch->h.dd = sktd;

    return eqrb_client_start(ch, mount_point, repl_map_size);
}

eqrb_rv_t eqrb_tcp_client_close(eqrb_client_handle_t *ch) {
    eqrb_rv_t rv = eqrb_client_stop(ch);
    close(ch->h.dd);
    return rv;
}
