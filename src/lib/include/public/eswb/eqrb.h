#ifndef ESWB_PUBLIC_EQRB_H
#define ESWB_PUBLIC_EQRB_H

/** @page eqrb
 * Check eqrb.h for calls descriptions
 */

/** @file */

#include "eswb/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    eqrb_rv_ok = 0,
    eqrb_small_buf,
    eqrb_inv_code,
    eqrb_rv_nomem,
    eqrb_notsup,
    eqrb_invarg,
    eqrb_nomem,
    eqrb_inv_size,

    eqrb_os_based_err,
    eqrb_media_err,
    eqrb_media_stop,
    eqrb_media_invarg,

    eqrb_server_already_launched,

    eqrb_rv_rx_eswb_fatal_err
} eqrb_rv_t;

struct eqrb_client_handle;

#define EQRB_ERR_MSG_MAX_LEN 128

eqrb_rv_t eqrb_tcp_server_start(uint16_t port);
eqrb_rv_t eqrb_tcp_server_stop();

eqrb_rv_t eqrb_tcp_client_create(struct eqrb_client_handle **rv_ch);
eqrb_rv_t eqrb_tcp_client_connect(struct eqrb_client_handle *ch, const char *addr_str, const char *bus2replicate,
                                  const char *mount_point, uint32_t repl_map_size, char *err_msg);
eqrb_rv_t eqrb_tcp_client_close(struct eqrb_client_handle *ch);

eqrb_rv_t eqrb_serial_server_start(const char *path, uint32_t baudrate, const char *bus2replicate);
eqrb_rv_t eqrb_serial_client_connect(const char *path, uint32_t baudrate, const char *mount_point, uint32_t repl_map_size);


#ifdef __cplusplus
}
#endif

#endif //ESWB_PUBLIC_EQRB_H
