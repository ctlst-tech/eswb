//
// Created by goofy on 1/6/22.
//

#ifndef ESWB_FRAMING_H
#define ESWB_FRAMING_H

#include <stdint.h>

#define BBEE_FRM_FRAME_BEGIN_CHAR 'B'
#define BBEE_FRM_FRAME_END_CHAR 'E'

typedef enum bbee_frm_rv {
    bbee_frm_ok = 0,
    bbee_frm_inv_crc,
    bbee_frm_got_frame,
    bbee_frm_buf_overflow,
    bbee_frm_got_empty_frame,
    bbee_frm_small_buf,
    bbee_frm_inv_code
} bbee_frm_rv_t;

#define CRC_LOG_LNG 4
typedef struct {
    uint8_t current_command_code;
    uint8_t *payload_buffer_origin;
    uint8_t *payload_buffer_ptr;
    size_t payload_buffer_max_size;
    size_t current_payload_size;

    uint16_t crc;
//    uint16_t crc_log[CRC_LOG_LNG];
//    int crc_log_ind;

    int got_begin_char;
    int got_end_char;
    int got_command_code;
    int payload_started;

    uint32_t stat_processings;
    uint32_t stat_good_frames;
    uint32_t stat_buf_overflow;
    uint32_t stat_inv_crc;
    uint32_t stat_got_empty_frame;
    uint32_t stat_frame_restart;
    uint32_t stat_non_framed_bytes;

} bbee_frm_rx_state_t;

typedef struct {
    void  *d;
    size_t l;
} io_v_t;


#ifdef __cplusplus
extern "C" {
#endif

bbee_frm_rv_t
bbee_frm_compose4tx(uint8_t command_code, void *payload, size_t payload_size, uint8_t *frame_buf, size_t frame_buf_size,
                    size_t *frame_size);
bbee_frm_rv_t
bbee_frm_compose4tx_v(uint8_t command_code,  io_v_t *iovec, uint8_t *frame_buf, size_t frame_buf_size,
                      size_t *frame_size);

void bbee_frm_reset_state(bbee_frm_rx_state_t *s);

void bbee_frm_init_state(bbee_frm_rx_state_t *s, uint8_t *buffer_origin, size_t buffer_size);

bbee_frm_rv_t bbee_frm_rx_iteration(bbee_frm_rx_state_t *s, const uint8_t *rx_buf, size_t rx_buf_l, size_t *byte_processed);

void iovec_set(io_v_t *v, size_t ind, void *d, size_t l, int terminate);

#ifdef __cplusplus
}
#endif


#endif //ESWB_FRAMING_H
