//
// Created by goofy on 1/6/22.
//


#include <string.h>
#include "framing.h"

#include "crc16-ccitt.h"

eqrb_rv_t
eqrb_make_tx_frame(uint8_t command_code, void *payload, size_t payload_size, uint8_t *frame_buf, size_t frame_buf_size,
                              size_t *frame_size) {

    size_t available_buf_size = frame_buf_size;

    // worst case scenario sizes check
    if (available_buf_size < (payload_size * 2 /*for escape symbols*/ + 4 /*sync*/ + 2+2 /*crc and its escapes*/ + 1/*code*/)) {
        return eqrb_small_buf;
    }

    if ((command_code == EQRB_FRAME_BEGIN_CHAR) || (command_code == EQRB_FRAME_END_CHAR)) {
        return eqrb_inv_code;
    }

    uint8_t *fb = frame_buf;
    *fb++ = EQRB_FRAME_BEGIN_CHAR;
    *fb++ = EQRB_FRAME_BEGIN_CHAR;
    *fb++ = command_code;

    uint16_t crc;
    crc16_ccitt_init(&crc);
    crc16_ccitt_update(&crc, command_code);

    uint8_t *eb = (uint8_t *) payload;

#   define IS_FRAME_SYNC_SYMB(__s) (((__s) == EQRB_FRAME_BEGIN_CHAR) || ((__s) == EQRB_FRAME_END_CHAR))

    for (int i = 0; i < payload_size; i++) {
        *fb = *eb;
        crc16_ccitt_update(&crc, *fb);
        if( IS_FRAME_SYNC_SYMB(*fb) ) {
            fb++;
            *fb = 0; // escape symbol;
        }
        fb++;
        eb++;
    }

    crc16_ccitt_finalize(&crc);

    uint8_t *pcrc = (uint8_t *) &crc;
    for (int i = 0; i < sizeof(crc); i++) {
        *fb = pcrc[i];
        if (IS_FRAME_SYNC_SYMB(*fb)) {
            fb++;
            *fb = 0; // escape symbol;
        }
        fb++;
    }

    *fb++ = EQRB_FRAME_END_CHAR;
    *fb++ = EQRB_FRAME_END_CHAR;

    *frame_size = fb - frame_buf;

    return eqrb_rv_ok;
}


void eqrb_reset_state(eqrb_rx_state_t *s) {
    s->payload_buffer_ptr = s->payload_buffer_origin;
    crc16_ccitt_init(&s->crc);

    s->got_begin_char = 0;
    s->got_end_char = 0;
    s->payload_started = 0;
    s->current_payload_size = 0;

    s->current_command_code = 0;
    s->got_command_code = 0;
    //s->crc_log_ind = 0;
}

void eqrb_init_state(eqrb_rx_state_t *s, uint8_t *buffer_origin, size_t buffer_size) {
    s->payload_buffer_origin = buffer_origin;
    s->payload_buffer_max_size = buffer_size;
    eqrb_reset_state(s);
}


eqrb_rv_t eqrb_rx_frame_iteration(eqrb_rx_state_t *s, const uint8_t *rx_buf, size_t rx_buf_l, size_t *byte_processed) {
    eqrb_rv_t rv = eqrb_rv_ok; // nothing happened, need next frame

    int i = 0;
    do {
        uint8_t b = rx_buf[i];

        if (b == EQRB_FRAME_BEGIN_CHAR) {
            if (s->got_begin_char) {
                eqrb_reset_state(s);
                s->payload_started = -1;
            } else {
                s->got_begin_char = -1;
            }
        } else if (b == EQRB_FRAME_END_CHAR) {
            if (s->got_end_char) {
                if (s->payload_started) {
                    if (s->payload_buffer_ptr - s->payload_buffer_origin >= 3) { /*crc, command code*/
                        uint16_t frame_crc;
                        s->payload_buffer_ptr -= 2;
                        crc16_ccitt_finalize(&s->crc);
                        memcpy(&frame_crc, s->payload_buffer_ptr, 2);
                        if (s->crc == frame_crc) {
                            rv = eqrb_rv_rx_got_frame;
                        } else {
                            rv = eqrb_rv_rx_inv_crc;
                        }
                    } else {
                        rv = eqrb_rv_rx_got_empty_frame;
                    }
                    s->current_payload_size = s->payload_buffer_ptr - s->payload_buffer_origin;
                }
            } else {
                s->got_end_char = -1;
            }
        } else {
            if (s->payload_started) {
                if (!s->got_command_code) {
                    s->current_command_code = b;
                    s->got_command_code = -1;
                    crc16_ccitt_update(&s->crc, b);
                } else if (s->got_end_char) {
                    *s->payload_buffer_ptr++ = b = EQRB_FRAME_END_CHAR;
                    s->got_end_char = 0;
                    s->got_begin_char = 0;
                } else if (s->got_begin_char) {
                    *s->payload_buffer_ptr++ = b = EQRB_FRAME_BEGIN_CHAR;
                    s->got_begin_char = 0;
                } else {
                    *s->payload_buffer_ptr++ = b;
                }

                size_t bsize = s->payload_buffer_ptr - s->payload_buffer_origin;
                if (bsize > 2) {
                    crc16_ccitt_update(&s->crc, s->payload_buffer_ptr[-3]); // don't want to calc crc over crc
                }
                if (bsize >= s->payload_buffer_max_size) {
                    //eqrb_reset_state(s);
                    rv = eqrb_rv_rx_buf_overflow;
                }
            }
        }
    } while ((i++ < rx_buf_l) && (rv == eqrb_rv_ok));

    *byte_processed = i;

    return rv;
}
