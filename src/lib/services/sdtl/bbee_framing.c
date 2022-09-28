//
// Created by goofy on 1/6/22.
//


#include <string.h>
#include "bbee_framing.h"

#include "crc16-ccitt.h"

void iovec_set(io_v_t *v, size_t ind, void *d, size_t l, int terminate) {
    v[ind].d = d;
    v[ind].l = l;
    if (terminate) {
        v[ind + 1].d = NULL;
    }
}

bbee_frm_rv_t
bbee_frm_compose4tx_v(uint8_t command_code,  io_v_t *iovec, uint8_t *frame_buf, size_t frame_buf_size,
                    size_t *frame_size) {

    size_t available_buf_size = frame_buf_size;

    size_t payload_size = 0;
    for (int i = 0; iovec[i].d != NULL; i++) {
        payload_size += iovec[i].l;
    }

    // worst case scenario sizes check
    if (available_buf_size < (payload_size * 2 /*for escape symbols*/ + 4 /*sync*/ + 2+2 /*crc and its escapes*/ + 1/*code*/)) {
        return bbee_frm_small_buf;
    }

    if ((command_code == BBEE_FRM_FRAME_BEGIN_CHAR) || (command_code == BBEE_FRM_FRAME_END_CHAR)) {
        return bbee_frm_inv_code;
    }

    uint8_t *fb = frame_buf;
    *fb++ = BBEE_FRM_FRAME_BEGIN_CHAR;
    *fb++ = BBEE_FRM_FRAME_BEGIN_CHAR;
    *fb++ = command_code;

    uint16_t crc;
    crc16_ccitt_init(&crc);
    crc16_ccitt_update(&crc, command_code);



#   define IS_FRAME_SYNC_SYMB(__s) (((__s) == BBEE_FRM_FRAME_BEGIN_CHAR) || ((__s) == BBEE_FRM_FRAME_END_CHAR))

    for (size_t i = 0; iovec[i].d != NULL; i++) {
        uint8_t *eb = (uint8_t *) iovec[i].d;
        for (size_t j = 0; j < iovec[i].l; j++) {
            *fb = *eb;
            crc16_ccitt_update(&crc, *fb);
            if (IS_FRAME_SYNC_SYMB(*fb)) {
                fb++;
                *fb = 0; // escape symbol;
            }
            fb++;
            eb++;
        }
    }

    crc16_ccitt_finalize(&crc);

    uint8_t *pcrc = (uint8_t *) &crc;
    for (uint32_t i = 0; i < sizeof(crc); i++) {
        *fb = pcrc[i];
        if (IS_FRAME_SYNC_SYMB(*fb)) {
            fb++;
            *fb = 0; // escape symbol;
        }
        fb++;
    }

    *fb++ = BBEE_FRM_FRAME_END_CHAR;
    *fb++ = BBEE_FRM_FRAME_END_CHAR;

    *frame_size = fb - frame_buf;

    return bbee_frm_ok;
}


bbee_frm_rv_t
bbee_frm_compose4tx(uint8_t command_code, void *payload, size_t payload_size, uint8_t *frame_buf, size_t frame_buf_size,
                    size_t *frame_size) {
    io_v_t iovec[2];
    iovec_set(iovec, 0, payload, payload_size, -1);

    return bbee_frm_compose4tx_v(command_code, iovec, frame_buf, frame_buf_size, frame_size);
}


void bbee_frm_reset_state(bbee_frm_rx_state_t *s) {
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

void bbee_frm_init_state(bbee_frm_rx_state_t *s, uint8_t *buffer_origin, size_t buffer_size) {
    s->payload_buffer_origin = buffer_origin;
    s->payload_buffer_max_size = buffer_size;
    bbee_frm_reset_state(s);
}

#ifdef BBEE_FRM_DEBUG
#include <stdio.h>
#endif

bbee_frm_rv_t bbee_frm_rx_iteration(bbee_frm_rx_state_t *s, const uint8_t *rx_buf, size_t rx_buf_l, size_t *byte_processed) {
    bbee_frm_rv_t rv = bbee_frm_ok; // nothing happened, need next frame

    uint32_t i = 0;
    do {
        uint8_t b = rx_buf[i];

        if (b == BBEE_FRM_FRAME_BEGIN_CHAR) {
            if (s->got_begin_char) {
                if (s->payload_started) {
                    s->stat_frame_restart++;
                }
                bbee_frm_reset_state(s);
                s->payload_started = -1;
            } else {
                s->got_begin_char = -1;
            }
        } else if (b == BBEE_FRM_FRAME_END_CHAR) {
            if (s->got_end_char) {
                if (s->payload_started) {
                    if (s->payload_buffer_ptr - s->payload_buffer_origin >= 3) { /*crc, command code*/
                        uint16_t frame_crc;
                        s->payload_buffer_ptr -= 2;
                        crc16_ccitt_finalize(&s->crc);
                        memcpy(&frame_crc, s->payload_buffer_ptr, 2);
                        s->payload_started = 0; // for frame restart detection wo full state reset
                        if (s->crc == frame_crc) {
                            rv = bbee_frm_got_frame;
                        } else {
//                            eqrb_dbg_msg("invalid crc 0x%04X != 0x%04X", s->crc, frame_crc);
                            rv = bbee_frm_inv_crc;
                        }
                    } else {
                        rv = bbee_frm_got_empty_frame;
                    }
                    s->current_payload_size = s->payload_buffer_ptr - s->payload_buffer_origin;
                }
            } else {
                s->got_end_char = -1;
            }
        } else {
            if (s->payload_started) {
                size_t bsize = s->payload_buffer_ptr - s->payload_buffer_origin;
                if (bsize < s->payload_buffer_max_size) {
                    if (!s->got_command_code) {
                        s->current_command_code = b;
                        s->got_command_code = -1;
                        crc16_ccitt_update(&s->crc, b);
                    } else if (s->got_end_char) {
                        *s->payload_buffer_ptr++ = b = BBEE_FRM_FRAME_END_CHAR;
                        s->got_end_char = 0;
                        s->got_begin_char = 0;
                    } else if (s->got_begin_char) {
                        *s->payload_buffer_ptr++ = b = BBEE_FRM_FRAME_BEGIN_CHAR;
                        s->got_begin_char = 0;
                    } else {
                        *s->payload_buffer_ptr++ = b;
                    }

                    if (s->payload_buffer_ptr - s->payload_buffer_origin > 2) {
                        crc16_ccitt_update(&s->crc, s->payload_buffer_ptr[-3]); // don't want to calc crc over crc
                    }
                } else {
                    //eqrb_reset_state(s);
                    rv = bbee_frm_buf_overflow;
                }
            }
        }
        i++;
    } while ((i < rx_buf_l) && (rv == bbee_frm_ok));

    *byte_processed = i;

    return rv;
}
