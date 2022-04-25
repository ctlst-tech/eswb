//
// Created by goofy on 1/6/22.
//

#ifndef ESWB_FRAMING_H
#define ESWB_FRAMING_H

#include <stdint.h>
#include "eqrb_core.h"

#define EQRB_FRAME_BEGIN_CHAR 'B'
#define EQRB_FRAME_END_CHAR 'E'


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
} eqrb_rx_state_t;

#ifdef __cplusplus
extern "C" {
#endif

eqrb_rv_t
eqrb_make_tx_frame(uint8_t command_code, void *payload, size_t payload_size, uint8_t *frame_buf, size_t frame_buf_size,
                   size_t *frame_size);

void eqrb_reset_state(eqrb_rx_state_t *s);

void eqrb_init_state(eqrb_rx_state_t *s, uint8_t *buffer_origin, size_t buffer_size);

eqrb_rv_t eqrb_rx_frame_iteration(eqrb_rx_state_t *s, const uint8_t *rx_buf, size_t rx_buf_l, size_t *byte_processed);

#ifdef __cplusplus
}
#endif


#endif //ESWB_FRAMING_H
