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
    eqrb_eswb_err,

    eqrb_os_based_err,
    eqrb_media_err,
    eqrb_media_stop,
    eqrb_media_invarg,
    eqrb_media_reset_cmd,
    eqrb_media_remote_need_reset,

    eqrb_server_already_launched,

    eqrb_rv_rx_eswb_fatal_err
} eqrb_rv_t;

typedef enum {
    eqrb_cmd_reset_remote,
    eqrb_cmd_reset_local_state,
} eqrb_cmd_t;

#define EQRB_ERR_MSG_MAX_LEN 128


eqrb_rv_t
eqrb_sdtl_server_start(const char *eqrb_service_name,
                       const char *sdtl_service_name, const char *sdtl_ch1_name, const char *sdtl_ch2_name, uint32_t ch_mask,
                       const char *bus2replicate, const char **err_msg);

eqrb_rv_t eqrb_sdtl_client_connect(const char *service_name, const char *sdtl_ch1_name, const char *sdtl_ch2_name,
                                   const char *mount_point, uint32_t repl_map_size);

const char *eqrb_strerror(eqrb_rv_t ecode);

#ifdef __cplusplus
}
#endif

#endif //ESWB_PUBLIC_EQRB_H
