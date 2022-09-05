#ifndef SDTL_H
#define SDTL_H

#include <eswb/api.h>

typedef enum sdtl_rv {
    SDTL_OK = 0,
    SDTL_OK_LAST_PACKET,
    SDTL_RX_BUF_SMALL,
    SDTL_NON_CONSIST_FRM_LEN,
    SDTL_INVALID_FRAME_TYPE,
    SDTL_NO_CHANNEL_REMOTE,
    SDTL_NO_CHANNEL_LOCAL,
    SDTL_MEDIA_ERR,
    SDTL_ESWB_ERR,
    SDTL_NO_MEM,
    SDTL_CH_EXIST,
    SDTL_INVALID_MTU,
    SDTL_NAMES_TOO_LONG,
} sdtl_rv_t;

typedef enum sdtl_channel_type {
    SDTL_CHANNEL_NONRELIABLE = 0,
    SDTL_CHANNEL_RELIABLE = 1,
} sdtl_channel_type_t;

#define SDTL_MTU_DEFAULT 256
#define SDTL_MTU_MAX     1024

typedef sdtl_rv_t (*media_open_t)(const char *path, void *params, void *h_rv);
typedef sdtl_rv_t (*media_close_t)(void *h);
typedef sdtl_rv_t (*media_read_t)(void *h, void *data, size_t l, size_t *lr);
typedef sdtl_rv_t (*media_write_t)(void *h, void *data, size_t l, size_t *lw);

typedef struct sdtl_service {

    const char *eswb_root;
    uint32_t mtu;

    media_open_t media_open;
    media_read_t media_read;
    media_write_t media_write;
    void *media_handle;

    struct sdtl_channel *channels;
    size_t              channels_num;

} sdtl_service_t;

typedef struct sdtl_channel_cfg {
    char                    *name;

    uint8_t                 id;
    sdtl_channel_type_t     type;

    uint32_t                mtu_override;
} sdtl_channel_cfg_t;

typedef uint8_t sdtl_pkt_cnt_t;
typedef uint16_t sdtl_pkt_payload_size_t;

typedef enum sdtl_ack_code {
    SDTL_ACK_GOT_PKT = 0,
    SDTL_ACK_NO_RECEIVER = 1,
    SDTL_ACK_PAYLOAD_TOO_BIG = 2,
    SDTL_ACK_RESET = 3,
} sdtl_ack_code_t;

typedef struct sdtl_channel {
    sdtl_channel_cfg_t  *cfg;
    uint32_t            max_payload_size; // payload inside data_pkt
    sdtl_service_t      *service;

    struct {
        sdtl_pkt_cnt_t next_pkt_cnt;
    } tx_state;

    struct {
        sdtl_pkt_cnt_t expected_pkt_cnt;
    } rx_state;

} sdtl_channel_t;


typedef struct sdtl_channel_handle {
    eswb_topic_descr_t data_td;
    eswb_topic_descr_t ack_td;

    sdtl_channel_t     *channel;
} sdtl_channel_handle_t;



#define SDTL_PKT_ATTR_PKT_TYPE_DATA (0)
#define SDTL_PKT_ATTR_PKT_TYPE_ACK (1)

#define SDTL_PKT_ATTR_PKT_TYPE_MASK(t) ((t) & 0x02)
#define SDTL_PKT_ATTR_PKT_TYPE(t) (SDTL_PKT_ATTR_PKT_TYPE_MASK(t) << 0)

#define SDTL_PKT_ATTR_PKT_READ_TYPE(__attr) SDTL_PKT_ATTR_PKT_TYPE_MASK((__attr) >> 0)



// this structure goes over physical
typedef struct __attribute__((packed)) sdtl_base_header {
    uint8_t ch_id;
    uint8_t attr;
} sdtl_base_header_t;

#define SDTL_PKT_DATA_FLAG_LAST_PKT (1 << 0)
#define SDTL_PKT_DATA_FLAG_RELIABLE (1 << 1)

// these structures go over IPC
typedef struct __attribute__((packed)) sdtl_data_sub_header {
    sdtl_pkt_cnt_t cnt;
    uint8_t flags;
    sdtl_pkt_payload_size_t payload_size;

    // uint8_t payload [payload_size]
} sdtl_data_sub_header_t;

typedef struct __attribute__((packed)) sdtl_ack_sub_header {
    sdtl_pkt_cnt_t  cnt;
    uint32_t        code;

} sdtl_ack_sub_header_t;


// these structures go over physical
typedef struct __attribute__((packed)) sdtl_data_header {
    sdtl_base_header_t        base;
    sdtl_data_sub_header_t    sub;
} sdtl_data_header_t;

typedef struct __attribute__((packed)) sdtl_ack_header {
    sdtl_base_header_t        base;
    sdtl_ack_sub_header_t     sub;
} sdtl_ack_header_t;


#endif //SDTL_H
