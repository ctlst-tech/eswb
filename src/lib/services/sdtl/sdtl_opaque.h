#ifndef SDTL_OPAQUE_H
#define SDTL_OPAQUE_H

#include <eswb/services/sdtl.h>
#include <pthread.h>

#define SDTL_MTU_DEFAULT 256
#define SDTL_MTU_MAX     1024

typedef uint8_t sdtl_pkt_cnt_t;
typedef uint16_t sdtl_pkt_payload_size_t;
typedef uint16_t sdtl_seq_code_t;

typedef struct sdtl_rx_stat {
    uint32_t    frames_received;
    uint32_t    bytes_received;
    uint32_t    bad_crc_frames;
    uint32_t    non_framed_bytes;
} sdtl_rx_stat_t;

typedef struct sdtl_tx_stat {
    uint32_t    bytes_sent;
    uint32_t    frames_sent;
} sdtl_tx_stat_t;

typedef struct sdtl_service {
    const char *service_name;
    char *service_eswb_root;
    uint32_t mtu;

    const sdtl_service_media_t *media;
    void *media_handle;

    size_t max_channels_num;

    struct sdtl_channel *channels;
    struct sdtl_channel_handle **channel_handles; // used on rx side

    size_t channels_num;

    pthread_t rx_thread_tid;

    sdtl_rx_stat_t rx_stat;
    eswb_topic_descr_t rx_stat_td;

} sdtl_service_t;



typedef enum sdtl_ack_code {
    SDTL_ACK_GOT_PKT = 0,
    SDTL_ACK_GOT_CMD = 1,
    SDTL_ACK_NO_RECEIVER = 2,
    SDTL_ACK_CANCELED = 3,
    SDTL_ACK_PAYLOAD_TOO_BIG = 4,
    SDTL_ACK_OUT_BAND_EVENT = 5,
} sdtl_ack_code_t;

typedef enum sdtl_rx_state {
    SDTL_RX_STATE_IDLE = 0,
    SDTL_RX_STATE_WAIT_DATA = 1,
    SDTL_RX_STATE_SEQ_DONE = 2,
    SDTL_RX_STATE_RCV_CANCELED = 3,
} sdtl_rx_state_t;

typedef struct sdtl_channel_rx_stat {
    uint32_t sequences;
    uint32_t packets;
    uint32_t bytes;
    uint32_t acks;
} sdtl_channel_rx_stat_t;

typedef struct sdtl_channel_tx_stat {
    uint32_t sequences;
    uint32_t packets;
    uint32_t bytes;
    uint32_t retries;
} sdtl_channel_tx_stat_t;

typedef struct sdtl_channel {
    sdtl_channel_cfg_t cfg;
    uint32_t max_payload_size; // payload inside data_pkt
    sdtl_service_t *service;
} sdtl_channel_t;

#define SDTL_CHANNEL_STATE_COND_FLAG_APP_RESET (1 << 0)
#define SDTL_CHANNEL_STATE_COND_FLAG_APP_CANCEL (1 << 1)

typedef struct sdtl_channel_state {

    union {
        uint32_t rx_state;
        sdtl_rx_state_t rx_state_enum;
    };

    sdtl_seq_code_t last_received_seq;

    uint8_t condition_flags;

} sdtl_channel_state_t;


typedef struct sdtl_channel_handle {
    eswb_topic_descr_t data_td;
    eswb_topic_descr_t ack_td;
    eswb_topic_descr_t rx_state_td;

    eswb_topic_descr_t rx_stat_td;
    eswb_topic_descr_t tx_stat_td;

    sdtl_channel_t *channel;

    void *rx_dafa_fifo_buf; // mtu size

    void *tx_frame_buf;
    size_t tx_frame_buf_size;

    uint32_t fifo_overflow;

    uint32_t armed_timeout_us; // microseconds timeout

    sdtl_channel_rx_stat_t rx_stat;
    sdtl_channel_tx_stat_t tx_stat;

    int tx_seq_num;

} sdtl_channel_handle_t;


#define SDTL_PKT_ATTR_PKT_TYPE_DATA (0)
#define SDTL_PKT_ATTR_PKT_TYPE_ACK (1)
#define SDTL_PKT_ATTR_PKT_TYPE_CMD (2)

#define SDTL_PKT_ATTR_PKT_TYPE_MASK(t)  ((t) & 0x03)
#define SDTL_PKT_ATTR_PKT_TYPE(t)       (SDTL_PKT_ATTR_PKT_TYPE_MASK(t))

#define SDTL_PKT_ATTR_PKT_GET_TYPE(__attr) SDTL_PKT_ATTR_PKT_TYPE_MASK((__attr))


// this structure goes over physical
typedef struct __attribute__((packed)) sdtl_base_header {
    uint8_t ch_id;
    uint8_t attr;
} sdtl_base_header_t;

#define SDTL_PKT_DATA_FLAG_FIRST_PKT (1 << 0)
#define SDTL_PKT_DATA_FLAG_LAST_PKT (1 << 1)
#define SDTL_PKT_DATA_FLAG_RELIABLE (1 << 2)
#define SDTL_PKT_DATA_FLAG_REPEAT   (1 << 3)

// these structures go over IPC
typedef struct __attribute__((packed)) sdtl_data_sub_header {
    sdtl_pkt_cnt_t cnt;
    uint8_t flags;
    sdtl_seq_code_t seq_code;
    sdtl_pkt_payload_size_t payload_size;

    // uint8_t payload [payload_size]
} sdtl_data_sub_header_t;

typedef struct __attribute__((packed)) sdtl_ack_sub_header {
    sdtl_pkt_cnt_t cnt;
    uint32_t code;
} sdtl_ack_sub_header_t;


// these structures go over physical
typedef struct __attribute__((packed)) sdtl_data_header {
    sdtl_base_header_t base;
    sdtl_data_sub_header_t sub;
} sdtl_data_header_t;

typedef struct __attribute__((packed)) sdtl_ack_header {
    sdtl_base_header_t base;
    sdtl_ack_sub_header_t sub;
} sdtl_ack_header_t;



typedef struct __attribute__((packed)) sdtl_cmd_header {
    sdtl_base_header_t base;
    uint8_t cmd_code;
} sdtl_cmd_header_t;


void sdtl_debug_msg(const char *fn, const char *txt, ...);

//#define SDTL_DEBUG

#ifdef SDTL_DEBUG
#define sdtl_dbg_msg(txt,...) sdtl_debug_msg(__func__, txt, ##__VA_ARGS__)
#else
#define sdtl_dbg_msg(txt,...) {}
#endif


#endif //SDTL_OPAQUE_H
