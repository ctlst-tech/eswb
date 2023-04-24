#ifndef SDTL_H
#define SDTL_H

#include <eswb/api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum sdtl_rv {
    SDTL_OK = 0,
    SDTL_TIMEDOUT,
    SDTL_OK_FIRST_PACKET,
    SDTL_OK_OMIT,
    SDTL_OK_REPEATED,
    SDTL_OK_MISSED_PKT_IN_SEQ,
    SDTL_REMOTE_RX_CANCELED,
    SDTL_REMOTE_RX_NO_CLIENT,
    SDTL_RX_BUF_SMALL,
    SDTL_TX_BUF_SMALL,
    SDTL_NON_CONSIST_FRM_LEN,
    SDTL_INVALID_FRAME_TYPE,
    SDTL_NO_CHANNEL_REMOTE,
    SDTL_NO_CHANNEL_LOCAL,
    SDTL_ESWB_ERR,
    SDTL_RX_FIFO_OVERFLOW,
    SDTL_NO_MEM,
    SDTL_CH_EXIST,
    SDTL_SERVICE_EXIST,
    SDTL_NO_SERVICE,
    SDTL_INVALID_MTU,
    SDTL_INVALID_MEDIA,
    SDTL_NAMES_TOO_LONG,
    SDTL_SYS_ERR,
    SDTL_INVALID_CH_TYPE,

    SDTL_MEDIA_NO_ENTITY,
    SDTL_MEDIA_NOT_SUPPORTED, // not supported call
    SDTL_MEDIA_ERR,
    SDTL_MEDIA_EOF,

    SDTL_APP_CANCEL,  // cancel current operation (out-of-band notification)
    SDTL_APP_RESET,   // reset application state  (out-of-band notification)
} sdtl_rv_t;

typedef enum sdtl_channel_type {
    SDTL_CHANNEL_UNRELIABLE = 0,
    SDTL_CHANNEL_RELIABLE = 1,
} sdtl_channel_type_t;


typedef sdtl_rv_t (*media_open_t)(const char *path, void *params, void **h_rv);
typedef sdtl_rv_t (*media_close_t)(void *h);
typedef sdtl_rv_t (*media_read_t)(void *h, void *data, size_t l, size_t *lr);
typedef sdtl_rv_t (*media_write_t)(void *h, void *data, size_t l);

typedef struct sdtl_service_media {
    media_open_t open;
    media_read_t read;
    media_write_t write;
    media_close_t close;
} sdtl_service_media_t;

typedef struct sdtl_service sdtl_service_t;
typedef struct sdtl_channel_cfg sdtl_channel_cfg_t;

typedef struct sdtl_channel_handle sdtl_channel_handle_t;

struct sdtl_channel_cfg {
    const char *name;

    uint8_t id;
    sdtl_channel_type_t type;

    uint32_t mtu_override;
};

/**
 * Media drivers and theirs parameter structures:
 */
typedef struct sdtl_media_serial_params {
    uint32_t baudrate;
} sdtl_media_serial_params_t;

extern const sdtl_service_media_t sdtl_media_serial;
/**
 *
 * @param s
 * @param service_name
 * @param mount_point
 * @param mtu maximum transmission unit on transport level, frame lavel may add some extra
 * @param max_channels_num
 * @param media
 * @return
 */
sdtl_rv_t sdtl_service_init(sdtl_service_t **s, const char *service_name, const char *mount_point, size_t mtu,
                            size_t max_channels_num, const sdtl_service_media_t *media);
sdtl_rv_t sdtl_service_init_w(sdtl_service_t **s_rv, const char *service_name, const char *mount_point, size_t mtu,
                              size_t max_channels_num, const char *media_name);

sdtl_service_t *sdtl_service_lookup(const char *service_name);
sdtl_rv_t sdtl_service_start(sdtl_service_t *s, const char *media_path, void *media_params);
sdtl_rv_t sdtl_service_stop(sdtl_service_t *s);

sdtl_rv_t sdtl_channel_create(sdtl_service_t *s, sdtl_channel_cfg_t *cfg);

sdtl_rv_t sdtl_channel_open(sdtl_service_t *s, const char *channel_name, sdtl_channel_handle_t **chh);
sdtl_rv_t sdtl_channel_close(sdtl_channel_handle_t *chh);

sdtl_rv_t sdtl_channel_recv_arm_timeout(sdtl_channel_handle_t *chh, uint32_t timeout_us);

sdtl_rv_t sdtl_channel_recv_data(sdtl_channel_handle_t *chh, void *d, uint32_t l, size_t *br);
sdtl_rv_t sdtl_channel_send_data(sdtl_channel_handle_t *chh, void *d, uint32_t l);

#define SDTL_PKT_CMD_CODE_CANCEL 0x10
#define SDTL_PKT_CMD_CODE_RESET  0x11

sdtl_rv_t sdtl_channel_send_cmd(sdtl_channel_handle_t *chh, uint8_t code);
sdtl_rv_t sdtl_channel_reset_condition(sdtl_channel_handle_t *chh);
sdtl_rv_t sdtl_channel_check_reset_condition(sdtl_channel_handle_t *chh);

uint32_t sdtl_channel_get_max_payload_size(sdtl_channel_handle_t *chh);
const sdtl_service_media_t *sdtl_lookup_media(const char *mtype);

const char *sdtl_strerror(sdtl_rv_t ecode);

#ifdef __cplusplus
}
#endif

#endif //SDTL_H
