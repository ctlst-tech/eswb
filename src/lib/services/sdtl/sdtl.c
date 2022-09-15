#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


#include "sdtl.h"
#include "bbee_framing.h"
#include "eswb/api.h"
#include "eswb/topic_proclaiming_tree.h"

static sdtl_rv_t set_rx_state(sdtl_channel_handle_t *chh, sdtl_rx_state_t state, sdtl_seq_code_t code);
static sdtl_rv_t read_rx_state(sdtl_channel_handle_t *chh, sdtl_channel_rx_state_t *rx_state);
static sdtl_rv_t send_ack(sdtl_channel_handle_t *chh, uint8_t pkt_cnt, uint32_t ack_code);

void sdtl_debug_msg(const char *fn, const char *txt, ...) {
    va_list (args);
    char pn[16];
    pthread_getname_np(pthread_self(), pn, sizeof(pn));
    fprintf(stdout, "%s | %s | ", pn, fn);
    va_start (args,txt);
    vfprintf(stdout, txt, args);
    va_end (args);
    fprintf(stdout, "\n");
}


sdtl_channel_t *resolve_channel_by_id(sdtl_service_t *s, uint8_t ch_id) {

    for (int i = 0; i < s->channels_num; i++) {
        if (s->channels[i].cfg->id == ch_id) {
            return &s->channels[i];
        }
    }

    return NULL;
}

sdtl_channel_handle_t *resolve_channel_handle_by_id(sdtl_service_t *s, uint8_t ch_id) {

    for (int i = 0; i < s->channels_num; i++) {
        if (s->channel_handles[i].channel->cfg->id == ch_id) {
            return &s->channel_handles[i];
        }
    }

    return NULL;
}

sdtl_channel_t *resolve_channel_by_name(sdtl_service_t *s, const char *name) {

    for (int i = 0; i < s->channels_num; i++) {
        if (strcmp(s->channels[i].cfg->name, name) == 0) {
            return &s->channels[i];
        }
    }

    return NULL;
}

static int check_rel(sdtl_channel_handle_t *ch) {
    return (ch->channel->cfg->type == SDTL_CHANNEL_RELIABLE);
}


void *sdtl_alloc(size_t s) {
    return calloc(1, s);
}

static sdtl_rv_t process_data(sdtl_channel_handle_t *chh, sdtl_data_sub_header_t *data_header) {
    // TODO check state

    eswb_rv_t erv = eswb_fifo_push(chh->data_td, data_header);

    return erv == eswb_e_ok ? SDTL_OK : SDTL_ESWB_ERR;
}

static sdtl_rv_t process_ack(sdtl_channel_handle_t *chh, sdtl_ack_sub_header_t *ack_header) {
    // TODO check state

    eswb_rv_t erv = eswb_fifo_push(chh->ack_td, ack_header);

    return erv == eswb_e_ok ? SDTL_OK : SDTL_ESWB_ERR;
}

static sdtl_rv_t validate_base_header(sdtl_base_header_t *hdr, uint8_t pkt_type, size_t data_len) {
    switch (pkt_type) {
        case SDTL_PKT_ATTR_PKT_TYPE_DATA:
            ;
            sdtl_data_header_t *dhdr = (sdtl_data_header_t *) hdr;
            if (dhdr->sub.payload_size != data_len - sizeof(*dhdr)) {
                return SDTL_NON_CONSIST_FRM_LEN;
            }
            break;

        case SDTL_PKT_ATTR_PKT_TYPE_ACK:
            ;
            sdtl_ack_header_t *ahdr = (sdtl_ack_header_t *) hdr;
            if (data_len != sizeof(*ahdr)) {
                return SDTL_NON_CONSIST_FRM_LEN;
            }
            break;

        default:
            return SDTL_INVALID_FRAME_TYPE;
    }

    return SDTL_OK;
}

static sdtl_rv_t sdtl_got_frame_handler (sdtl_service_t *s, uint8_t cmd_code, uint8_t *data, size_t data_len) {
    sdtl_rv_t rv;
    sdtl_base_header_t *base_header = (sdtl_base_header_t *) data;

    uint8_t pkt_type = SDTL_PKT_ATTR_PKT_READ_TYPE(base_header->attr);

    sdtl_channel_rx_state_t rx_state;

    // validate packet
    rv = validate_base_header(base_header, pkt_type, data_len);
    if (rv != SDTL_OK) {
        // TODO send ack with error
        return rv;
    }

    // resolve channel
    sdtl_channel_handle_t *chh = resolve_channel_handle_by_id(s, base_header->ch_id);
    if (chh == NULL) {
        // TODO send ack with error
        return SDTL_NO_CHANNEL_LOCAL;
    }

    if (check_rel(chh)) {
        read_rx_state(chh, &rx_state);
    } else {
        rx_state.state = SDTL_RX_STATE_WAIT_DATA;
    }

#   define HAS_SAME_PARITY(__hdr,__curr_is_odd) (((__hdr)->base.attr & SDTL_PKT_ATTR_ODD_FLAG) && (__curr_is_odd))

    switch (pkt_type) {
        case SDTL_PKT_ATTR_PKT_TYPE_DATA:
            ;
            sdtl_data_header_t *dhdr = (sdtl_data_header_t *) base_header;

            if ((dhdr->sub.flags & (SDTL_PKT_DATA_FLAG_LAST_PKT)) &&
                (dhdr->sub.seq_code == rx_state.last_received_seq)) {
                sdtl_dbg_msg("Got rx state: ack trailing paket from prior seq 0x%04X", rx_state.last_received_seq);

//                rx_state.state = SDTL_RX_STATE_SEQ_DONE; // just a local copy of the state
                rv = send_ack(chh, dhdr->sub.cnt, SDTL_ACK_GOT_PKT);
            }

            switch (rx_state.state) {
                default:
                case SDTL_RX_STATE_RCV_CANCELED:
                    rv = send_ack(chh, dhdr->sub.cnt, SDTL_ACK_CANCELED);
                    sdtl_dbg_msg("Got rx state: SDTL_RX_STATE_RCV_CANCELED");
                    break;

                case SDTL_RX_STATE_SEQ_DONE:
                case SDTL_RX_STATE_IDLE:
                    rv = send_ack(chh, dhdr->sub.cnt, SDTL_ACK_NO_RECEIVER);
                    sdtl_dbg_msg("Got rx state: SDTL_RX_STATE_IDLE");
                    break;

//                case SDTL_RX_STATE_SEQ_DONE:
//                    rv = send_ack(chh, dhdr->sub.cnt, SDTL_ACK_GOT_PKT);
//                    sdtl_dbg_msg("Got rx state: SDTL_RX_STATE_SEQ_DONE");
//                    break;

                case SDTL_RX_STATE_WAIT_DATA:
                    rv = process_data(chh, &dhdr->sub);
                    sdtl_dbg_msg("Got pkt #%d with %d bytes", dhdr->sub.cnt, dhdr->sub.payload_size);
                    break;
            }
            break;

        case SDTL_PKT_ATTR_PKT_TYPE_ACK:
            ;
            sdtl_ack_header_t *ahdr = (sdtl_ack_header_t *) base_header;
            rv = process_ack(chh, &ahdr->sub);
            sdtl_dbg_msg("Got ack for pkt #%d", ahdr->sub.cnt);
            break;

        default:
            // TODO throw invalid packet error
            sdtl_dbg_msg("Invalid packet");
            break;
    }

    return rv;
}


sdtl_rv_t bbee_frm_process_rx_buf(sdtl_service_t *s,
                                  uint8_t *rx_buf,
                                  size_t rx_buf_lng,
                                  bbee_frm_rx_state_t *rx_state,
                                  sdtl_rv_t (*rx_got_frame_handler)
                                   (sdtl_service_t *s, uint8_t cmd_code, uint8_t *data, size_t data_len)
                           ) {
    bbee_frm_rv_t frv;

    size_t bp = 0;
    size_t total_br = 0;
    do {
        frv = bbee_frm_rx_iteration(rx_state, rx_buf + total_br,
                                    rx_buf_lng - total_br, &bp);

        switch (frv) {
            default:
            case bbee_frm_ok:
                break;

            case bbee_frm_buf_overflow:
            case bbee_frm_inv_crc:
            case bbee_frm_got_empty_frame:
                bbee_frm_reset_state(rx_state);
//                        eqrb_dbg_msg("bbee_frm_reset_state (%d)", rv);
                break;

            case bbee_frm_got_frame:
                rx_got_frame_handler(s, rx_state->current_command_code,
                                     rx_state->payload_buffer_origin,
                                     rx_state->current_payload_size);
//                        eqrb_dbg_msg("rx_got_frame_handler");
                bbee_frm_reset_state(rx_state);
                break;
        }

        total_br += bp;
    } while (total_br < rx_buf_lng);

    return SDTL_OK;
}

sdtl_rv_t bbee_frm_compose_frame(void *frame_buf, size_t frame_buf_size, void *hdr, size_t hdr_len, void *d, size_t d_len, size_t *frame_size_rv) {
    io_v_t iovec[3];

    iovec_set(iovec,0, hdr, hdr_len, 0);
    iovec_set(iovec,1, d, d_len, 1);

    bbee_frm_rv_t frv = bbee_frm_compose4tx_v(0, iovec, frame_buf, frame_buf_size, frame_size_rv);

    return frv == bbee_frm_ok ? SDTL_OK : SDTL_TX_BUF_SMALL;
}

sdtl_rv_t bbee_frm_allocate_tx_framebuf(size_t mtu, void **fb, size_t *l) {

//    if (available_buf_size < (payload_size * 2 /*for escape symbols*/ + 4 /*sync*/ + 2+2 /*crc and its escapes*/ + 1/*code*/)) {

    size_t fl = mtu * 2 + 10;
    *fb = sdtl_alloc(fl);
    if (*fb == NULL) {
        return SDTL_NO_MEM;
    }

    *l = fl;

    return SDTL_OK;
}

static void print_debug_data(uint8_t *rx_buf, size_t rx_buf_lng) {
//    char dbg_data[rx_buf_lng * 4];
//    char ss[4];
//    dbg_data[0] = 0;
//    for (int i = 0; i < rx_buf_lng; i++) {
//        sprintf(ss, "%02X ", rx_buf[i]);
//        strcat(dbg_data, ss);
//    }
//    eqrb_dbg_msg("recv | data | %s", dbg_data);
}

sdtl_rv_t sdtl_service_rx_thread(sdtl_service_t *s) {
    char thread_name[16];
#define SDTL_RX_THREAD_NAME_PREFIX "sdtlrx+"
    strcpy(thread_name, SDTL_RX_THREAD_NAME_PREFIX);
    strncat(thread_name, s->service_name, sizeof(thread_name) - 1);

    eswb_set_thread_name(thread_name);

    bbee_frm_rx_state_t rx_state;

    size_t payload_size = s->mtu + 10;
    size_t rx_buf_size = payload_size << 1; // * 2


    uint8_t *rx_buf = sdtl_alloc(rx_buf_size);
    uint8_t *payload_buf = sdtl_alloc(payload_size);

    bbee_frm_init_state(&rx_state, payload_buf, payload_size);

    sdtl_rv_t media_rv;

    int loop = -1;

    do {
        size_t rx_buf_lng;
        media_rv = s->media->read(s->media_handle, rx_buf, rx_buf_size, &rx_buf_lng);

        switch (media_rv) {
            case SDTL_OK:
                break;

            default:
            case SDTL_MEDIA_EOF:
                loop = 0;
                break;
        }

//      eqrb_dbg_msg("recv | rx_buf_lng == %d rv == %d", rx_buf_lng, rv);

#       ifdef EQRB_DEBUG
        print_debug_data(rx_buf, rx_buf_lng);
#       endif

        bbee_frm_process_rx_buf(s, rx_buf, rx_buf_lng, &rx_state, sdtl_got_frame_handler);
    } while (loop);

    return SDTL_OK;
}


static sdtl_rv_t media_tx(sdtl_channel_handle_t *chh, void *header, uint32_t hl, void *d, uint32_t l) {
    size_t composed_frame_len;
    sdtl_rv_t rv = bbee_frm_compose_frame(chh->tx_frame_buf, chh->tx_frame_buf_size, header, hl, d, l, &composed_frame_len);
    if (rv != SDTL_OK) {
        return rv;
    }

    return chh->channel->service->media->write(chh->channel->service->media_handle, chh->tx_frame_buf, composed_frame_len);
}



#define my_min(a,b) (((a) < (b)) ? (a) : (b))

static sdtl_rv_t
send_data(sdtl_channel_handle_t *chh, uint8_t pkt_num, uint8_t flags, sdtl_seq_code_t seq_code, void *d, uint32_t l) {
    sdtl_data_header_t hdr;


    hdr.base.attr = SDTL_PKT_ATTR_PKT_TYPE(SDTL_PKT_ATTR_PKT_TYPE_DATA);

    hdr.sub.seq_code = seq_code;

    hdr.base.ch_id = chh->channel->cfg->id;

    // TODO handle states properly
    hdr.sub.cnt = pkt_num;
    hdr.sub.payload_size = l;
    hdr.sub.flags = flags;

    return media_tx(chh, &hdr, sizeof(hdr), d, l);
}


static sdtl_rv_t send_ack(sdtl_channel_handle_t *chh, uint8_t pkt_cnt, sdtl_ack_code_t ack_code) {
    sdtl_ack_header_t hdr;

    hdr.base.attr = SDTL_PKT_ATTR_PKT_TYPE(SDTL_PKT_ATTR_PKT_TYPE_ACK);

    hdr.base.ch_id = chh->channel->cfg->id;

    hdr.sub.code = ack_code;
    hdr.sub.cnt = pkt_cnt;

    return media_tx(chh, &hdr, sizeof(hdr), NULL, 0);
}


static sdtl_rv_t wait_ack(sdtl_channel_handle_t *chh, uint32_t timeout_us, sdtl_ack_sub_header_t *ack_sh_rv) {

    sdtl_ack_sub_header_t ack_sh;

    eswb_rv_t erv;
    sdtl_rv_t rv;
    if (timeout_us > 0) {
        eswb_arm_timeout(chh->ack_td, timeout_us);
    }
    erv = eswb_fifo_pop(chh->ack_td, &ack_sh);
    chh->armed_timeout_us = 0;

    switch (erv) {
        case eswb_e_ok:
            rv = SDTL_OK;
            break;

        case eswb_e_timedout:
            rv = SDTL_TIMEDOUT;
            break;

        default:
            rv = SDTL_ESWB_ERR;
            break;
    }

    *ack_sh_rv = ack_sh;

    return rv;
}


static sdtl_rv_t
wait_data(sdtl_channel_handle_t *chh, void *d, uint32_t l, sdtl_data_sub_header_t **dsh_rv, int prev_pkt_num) {
    eswb_rv_t rv;
    sdtl_data_sub_header_t *dsh = chh->rx_dafa_fifo_buf;

    if (chh->armed_timeout_us > 0) {
        eswb_arm_timeout(chh->data_td, chh->armed_timeout_us);
    }

    rv = eswb_fifo_pop(chh->data_td, dsh);

    *dsh_rv = dsh;

    switch (rv) {
        case eswb_e_ok:
            if (prev_pkt_num != -1) {
                int dc = dsh->cnt - (prev_pkt_num % 256);
                if (dc < 0) {
                    dc += 256;
                }

                if (dc > 1) {
                    return SDTL_OK_MISSED_PKT_IN_SEQ;
                } else if (dc == 0) {
                    return SDTL_OK_REPEATED;
                } else {
                    if (l < dsh->payload_size) {
                        return SDTL_RX_BUF_SMALL;
                    }

                    // data is ok
                    return SDTL_OK;
                }
            } else {
                if (dsh->flags & SDTL_PKT_DATA_FLAG_FIRST_PKT) {
                    return SDTL_OK_FIRST_PACKET;
                } else {
                    return SDTL_OK_OMIT;
                }
            }

        case eswb_e_timedout:
            return SDTL_TIMEDOUT;

        case eswb_e_fifo_rcvr_underrun:
            chh->fifo_overflow++;
            return SDTL_RX_FIFO_OVERFLOW;

        default:
            return SDTL_ESWB_ERR;
    }



    return SDTL_OK;
}


//static sdtl_rv_t update_tx_state(sdtl_channel_handle_t *chh, int pkt_cnt) {
//    // TODO make enumeration for transition, make transitions accordingly
//
//    // TODO
//
//    if (pkt_cnt > 0) {
//
//    }
//
//    return SDTL_OK;
//}


static sdtl_rv_t channel_send_data(sdtl_channel_handle_t *chh, int rel, void *d, size_t l) {
    sdtl_pkt_payload_size_t dsize;
    uint32_t offset = 0;

    sdtl_rv_t rv = SDTL_OK;

    uint8_t pktn_in_seq = 0;
    uint8_t flags = SDTL_PKT_DATA_FLAG_FIRST_PKT |
                    (rel ? SDTL_PKT_DATA_FLAG_RELIABLE : 0);

    sdtl_seq_code_t seq_code;
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC, &timespec);

    // pseudorandom seq num
    seq_code = chh->tx_seq_num + (timespec.tv_nsec >> 10);
    if (seq_code == 0) {
        seq_code--;
    }

    sdtl_dbg_msg("================= call ================= (size == %d)", l);

    do {
        dsize = my_min(chh->channel->max_payload_size, l);

        flags|= dsize == l ? SDTL_PKT_DATA_FLAG_LAST_PKT : 0;

        do {
            rv = send_data(chh, pktn_in_seq, flags, seq_code, d + offset, dsize);
            sdtl_dbg_msg("Send %d bytes in pkt #%d in seq 0x%04X", dsize, pktn_in_seq, seq_code);

            // TODO get timeout from somewhere
            if (rel) {
                sdtl_ack_sub_header_t ack_sh;
                rv = wait_ack(chh, 100000, &ack_sh);

                switch (rv) {
                    case SDTL_OK:
                        switch (ack_sh.code) {
                            case SDTL_ACK_GOT_PKT:
                                // TODO check pkt num
                                sdtl_dbg_msg("Got ack for pkt #%d", ack_sh.cnt);
                                break;

                            case SDTL_ACK_CANCELED:
                                sdtl_dbg_msg("Got ack with SDTL_ACK_CANCELED");
                                return SDTL_REMOTE_RX_CANCELED;

                            case SDTL_ACK_NO_RECEIVER:
                                sdtl_dbg_msg("Got ack with SDTL_ACK_NO_RECEIVER");
                                return SDTL_REMOTE_RX_NO_CLIENT;

                            default:
                                break;
                        }
                        break;

                    case SDTL_TIMEDOUT:
                        sdtl_dbg_msg("Ack timeout");
                        break;

                    default:
                        // error
                        return rv;
                }
            }
        } while (rv == SDTL_TIMEDOUT);

        flags&= ~SDTL_PKT_DATA_FLAG_FIRST_PKT;

        pktn_in_seq++;

        l -= dsize;
        offset += dsize;
    } while (l > 0);

    chh->tx_seq_num++;

    return rv;
}

static sdtl_rv_t set_rx_state(sdtl_channel_handle_t *chh, sdtl_rx_state_t state, sdtl_seq_code_t code) {
    eswb_rv_t erv;

    sdtl_channel_rx_state_t rx_state = {
            .state = state,
            .last_received_seq = code,
    };

    erv = eswb_update_topic(chh->rx_state_td, &rx_state);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    return SDTL_OK;
}

static sdtl_rv_t read_rx_state(sdtl_channel_handle_t *chh, sdtl_channel_rx_state_t *rx_state) {
    eswb_rv_t erv;
    erv = eswb_read(chh->rx_state_td, rx_state);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    return SDTL_OK;
}


static sdtl_rv_t channel_recv_data(sdtl_channel_handle_t *chh, int rel, void *d, size_t s, size_t *br) {

    sdtl_rv_t rv_rcv;
    sdtl_rv_t rv_ack;
    sdtl_rv_t rv_state;
    sdtl_rv_t rv = SDTL_OK;

    sdtl_data_sub_header_t *dsh;

    // state variables:
    int sequence_started = 0;
    uint32_t offset = 0;
    size_t l = s;
    int prev_pkt_num = -1;

#   define RESET_SEQ()  l = s; \
                        offset = 0; \
                        prev_pkt_num = -1; \
                        sequence_started = 0

    int loop = -1;

    if (rel) {
        sdtl_channel_rx_state_t rx_state;
        rv_state = read_rx_state(chh, &rx_state);
        if (rv_state != SDTL_OK) {
            return rv_state;
        }

        rv_state = set_rx_state(chh, SDTL_RX_STATE_WAIT_DATA, rx_state.last_received_seq);
        if (rv_state != SDTL_OK) {
            return rv_state;
        }
        // TODO check state before, check that channal is not busy
    }

    sdtl_dbg_msg("================= call ================= (size == %d)", s);


    do {
        rv_rcv = wait_data(chh, d + offset, l, &dsh, prev_pkt_num);

        switch (rv_rcv) {
            default:
            case SDTL_RX_BUF_SMALL:
                rv = rv_rcv;
                loop = 0;
                sdtl_dbg_msg("Buffer too small (l==%d against payload_size == %d)", l, dsh->payload_size);
                break;

            case SDTL_OK_REPEATED:
                if (rel) {
                    rv_ack = send_ack(chh, dsh->cnt, SDTL_ACK_GOT_PKT);
                }
                sdtl_dbg_msg("Got repeated pkt #%d", dsh->cnt);
                break;

            case SDTL_OK_OMIT:
                break;

            case SDTL_OK_FIRST_PACKET:
                sdtl_dbg_msg("Got SDTL_OK_FIRST_PACKET state");
                sequence_started = -1;
                prev_pkt_num = 0;
                // no break here


            case SDTL_OK:
                if (sequence_started) {
                    if (rel) {
                        rv_ack = send_ack(chh, dsh->cnt, SDTL_ACK_GOT_PKT);
                    } else {
                        // no action required
                    }

                    void *payload_data = ((void *) dsh) + sizeof(*dsh);
                    memcpy(d + offset, payload_data, dsh->payload_size);

                    offset += dsh->payload_size;
                    l -= dsh->payload_size;

                    if (rv_rcv == SDTL_OK) {
                        prev_pkt_num++;
                    }

                    sdtl_dbg_msg("Send %d bytes in pkt #%d in seq 0x%04X", dsh->payload_size, dsh->cnt, dsh->seq_code);

                    if (dsh->flags & SDTL_PKT_DATA_FLAG_LAST_PKT) {
                        sdtl_dbg_msg("Got SDTL_PKT_DATA_FLAG_LAST_PKT flag");

                        rv = SDTL_OK;
                        loop = 0;
                    }
                }
                break;

            case SDTL_OK_MISSED_PKT_IN_SEQ:
                if (rel) {

                } else {
                    RESET_SEQ();
                }
                sdtl_dbg_msg("Got missed pkt in seq (pkt #%d, prev #%d)", dsh->cnt, prev_pkt_num);
                break;


        }

    } while (loop);

    // we want to keep timeout for sequentially arriving packets
    chh->armed_timeout_us = 0;

    if (rel) {
        if (rv == SDTL_OK) {
            set_rx_state(chh, SDTL_RX_STATE_SEQ_DONE, dsh->seq_code);
        } else {
            set_rx_state(chh, SDTL_RX_STATE_RCV_CANCELED, 0);
        }
    }

    if (rv == SDTL_OK) {
        *br = offset;
    }

    return rv;
}


#define CHANNEL_DATA_FIFO_NAME "data_fifo"
#define CHANNEL_ACK_FIFO_NAME "ack_fifo"

#define CHANNEL_DATA_BUF_NAME "data_buf"
#define CHANNEL_ACK_BUF_NAME "ack_buf"
#define CHANNEL_RX_STATE_STRUCT_NAME "rx_state"
//#define CHANNEL_RX_STATE_NAME "state"

#define CHANNEL_DATA_SUBPATH CHANNEL_DATA_FIFO_NAME "/" CHANNEL_DATA_BUF_NAME
#define CHANNEL_ACK_SUBPATH CHANNEL_ACK_FIFO_NAME "/" CHANNEL_ACK_BUF_NAME

static sdtl_rv_t check_channel_path(const char *root_path, const char *ch_name, const char *sub_path) {
    if (strlen(root_path) +
            strlen(ch_name) +
                strlen(sub_path) + 1 > ESWB_TOPIC_MAX_PATH_LEN) {
        return SDTL_NAMES_TOO_LONG;
    }

    return SDTL_OK;
}

static sdtl_rv_t check_channel_both_paths(const char *root_path, const char *ch_name) {

    if ((check_channel_path(root_path, ch_name, CHANNEL_DATA_SUBPATH) != SDTL_OK) ||
        (check_channel_path(root_path, ch_name, CHANNEL_ACK_SUBPATH) != SDTL_OK)) {
        return SDTL_NAMES_TOO_LONG;
    }

    return SDTL_OK;
}


static eswb_rv_t open_channel_resource(const char *base_path, const char* ch_name, const char *subpath, eswb_topic_descr_t *rv_td) {
    eswb_rv_t erv;

    char path[ESWB_TOPIC_MAX_PATH_LEN];

    strcpy(path, base_path);
    strcat(path, "/");
    strcat(path, ch_name);
    strcat(path, "/");
    strcat(path, subpath);

    return eswb_connect(path, rv_td);
}




sdtl_rv_t sdtl_service_init(sdtl_service_t *s, const char *service_name, const char *mount_point, size_t mtu,
                            size_t max_channels_num, const sdtl_service_media_t *media) {
    memset(s, 0, sizeof(*s));


    s->service_name = service_name;
    s->mtu = mtu;
    s->max_channels_num = max_channels_num;
    s->media = media;
    s->channels = 0;

    s->service_eswb_root = sdtl_alloc(strlen(mount_point) + 1 + strlen(service_name) + 1);

    strcpy(s->service_eswb_root, mount_point);
    strcat(s->service_eswb_root, "/");
    strcat(s->service_eswb_root, s->service_name);

    s->channels = sdtl_alloc(max_channels_num * sizeof(*s->channels));
    if (s->channels == NULL) {
        return SDTL_NO_MEM;
    }

    s->channel_handles = sdtl_alloc(max_channels_num * sizeof(*s->channel_handles));
    if (s->channel_handles == NULL) {
        return SDTL_NO_MEM;
    }

    eswb_rv_t erv;
    erv = eswb_mkdir(mount_point, s->service_name);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    return SDTL_OK;
}


sdtl_rv_t sdtl_service_start(sdtl_service_t *s, const char *media_path, void *media_params) {
    sdtl_rv_t rv;

    rv = s->media->open(media_path, media_params, &s->media_handle);

    if (rv != SDTL_OK) {
        return rv;
    }

    for (int i = 0; i < s->channels_num; i++) {
        rv = sdtl_channel_open(s, s->channels[i].cfg->name, &s->channel_handles[i]);
        if (rv != SDTL_OK) {
            return rv;
        }
        if (check_rel(&s->channel_handles[i])) {
            set_rx_state(&s->channel_handles[i], SDTL_RX_STATE_IDLE, 0);
        }
    }

    int prv = pthread_create(&s->rx_thread_tid, NULL, (void*)(void*) sdtl_service_rx_thread, s);
    if (prv) {
        return SDTL_SYS_ERR;
    }

    return SDTL_OK;
}

sdtl_rv_t sdtl_service_stop(sdtl_service_t *s) {
    int rv;

    rv = pthread_cancel(s->rx_thread_tid);
    if (rv != 0) {
        return SDTL_SYS_ERR;
    }

    rv = pthread_join(s->rx_thread_tid, NULL);
    if (rv != 0) {
        return SDTL_SYS_ERR;
    }

    for (int i = 0; i < s->channels_num; i++) {
        sdtl_channel_close(&s->channel_handles[i]);
    }

    return SDTL_OK;
}


sdtl_rv_t sdtl_channel_create(sdtl_service_t *s, sdtl_channel_cfg_t *cfg) {

    if (resolve_channel_by_id(s, cfg->id)) {
        return SDTL_CH_EXIST;
    }

    if (s->channels_num >= s->max_channels_num) {
        return SDTL_NO_MEM;
    }

    sdtl_channel_t *ch = &s->channels[s->channels_num];

    memset(ch, 0, sizeof(*ch));

    size_t mtu = cfg->mtu_override > 0 ? my_min(s->mtu, cfg->mtu_override) : s->mtu;

    int max_payload_size = mtu - sizeof(sdtl_data_header_t);
    if (max_payload_size < 0) {
        return SDTL_INVALID_MTU;
    }

    ch->cfg = cfg;
    ch->service = s;
    ch->max_payload_size = max_payload_size;

    eswb_rv_t erv;

    erv = eswb_mkdir(s->service_eswb_root, cfg->name);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    if (check_channel_both_paths(s->service_eswb_root, cfg->name) != SDTL_OK) {
        return SDTL_NAMES_TOO_LONG;
    }

    char path[ESWB_TOPIC_MAX_PATH_LEN + 1];

    strcpy(path, s->service_eswb_root);
    strcat(path, "/");
    strcat(path, cfg->name);

    // TODO fifo size supposed to be tuned according the speed of interface and overall service latency
#   define FIFO_SIZE 8

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 4);
    topic_proclaiming_tree_t *data_fifo_root = usr_topic_set_fifo(cntx, CHANNEL_DATA_FIFO_NAME, FIFO_SIZE);
    usr_topic_add_child(cntx, data_fifo_root, CHANNEL_DATA_BUF_NAME, tt_plain_data, 0, mtu, 0);
    erv = eswb_proclaim_tree_by_path(path, data_fifo_root, cntx->t_num, NULL);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    if (ch->cfg->type == SDTL_CHANNEL_RELIABLE) {
        TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);
        topic_proclaiming_tree_t *ack_fifo_root = usr_topic_set_fifo(cntx, CHANNEL_ACK_FIFO_NAME, FIFO_SIZE);
        usr_topic_add_child(cntx, ack_fifo_root, CHANNEL_ACK_BUF_NAME, tt_plain_data, 0, sizeof(sdtl_ack_sub_header_t),
                            0);
        erv = eswb_proclaim_tree_by_path(path, ack_fifo_root, cntx->t_num, NULL);
        if (erv != eswb_e_ok) {
            return SDTL_ESWB_ERR;
        }

        sdtl_channel_rx_state_t rx_state;

        TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);
        topic_proclaiming_tree_t *rx_state_root = usr_topic_set_struct(cntx, rx_state, CHANNEL_RX_STATE_STRUCT_NAME);
        usr_topic_add_struct_child(cntx, rx_state_root, sdtl_channel_rx_state_t, state, "state", tt_uint32);
        erv = eswb_proclaim_tree_by_path(path, rx_state_root, cntx->t_num, NULL);
        if (erv != eswb_e_ok) {
            return SDTL_ESWB_ERR;
        }
    }

    s->channels_num++;

    return SDTL_OK;
}


sdtl_rv_t sdtl_channel_open(sdtl_service_t *s, const char *channel_name, sdtl_channel_handle_t *chh_rv) {

    sdtl_channel_t *ch = resolve_channel_by_name(s, channel_name);
    if (ch == NULL) {
        return SDTL_NO_CHANNEL_LOCAL;
    }

    if (check_channel_both_paths(s->service_eswb_root, channel_name) != SDTL_OK) {
        return SDTL_NAMES_TOO_LONG;
    }

    memset(chh_rv, 0, sizeof(*chh_rv));
    chh_rv->channel = ch;

    size_t mtu = s->mtu;

    sdtl_rv_t rv;
    rv = bbee_frm_allocate_tx_framebuf(mtu, &chh_rv->tx_frame_buf, &chh_rv->tx_frame_buf_size);
    if (rv != SDTL_OK) {
        return rv;
    }

    chh_rv->rx_dafa_fifo_buf = sdtl_alloc(ch->max_payload_size + sizeof(sdtl_data_sub_header_t));
    if (chh_rv->rx_dafa_fifo_buf == NULL) {
        return SDTL_NO_MEM;
    }

    eswb_rv_t erv;
    erv = open_channel_resource(s->service_eswb_root, channel_name, CHANNEL_DATA_FIFO_NAME, &chh_rv->data_td);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    if (ch->cfg->type == SDTL_CHANNEL_RELIABLE) {
        erv = open_channel_resource(s->service_eswb_root, channel_name, CHANNEL_ACK_FIFO_NAME, &chh_rv->ack_td);
        if (erv != eswb_e_ok) {
            return SDTL_ESWB_ERR;
        }

        erv = open_channel_resource(s->service_eswb_root, channel_name, CHANNEL_RX_STATE_STRUCT_NAME, &chh_rv->rx_state_td);
        if (erv != eswb_e_ok) {
            return SDTL_ESWB_ERR;
        }
    }

    return SDTL_OK;
}

sdtl_rv_t sdtl_channel_close(sdtl_channel_handle_t *chh) {
    eswb_rv_t erv;

    // TODO free buffers
    // FIXME eswb_disconnect currently is not supported

    erv = eswb_disconnect(chh->data_td);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    if (chh->channel->cfg->type == SDTL_CHANNEL_RELIABLE) {
        erv = eswb_disconnect(chh->ack_td);
        if (erv != eswb_e_ok) {
            return SDTL_ESWB_ERR;
        }
        erv = eswb_disconnect(chh->rx_state_td);
        if (erv != eswb_e_ok) {
            return SDTL_ESWB_ERR;
        }
    }

    return SDTL_OK;
}


sdtl_rv_t sdtl_channel_recv_arm_timeout(sdtl_channel_handle_t *chh, uint32_t timeout_us) {
    // TODO limit timeout abs value
    chh->armed_timeout_us = timeout_us;
    return SDTL_OK;
}

sdtl_rv_t sdtl_channel_recv_data(sdtl_channel_handle_t *chh, void *d, uint32_t l, size_t *br) {
    int rel = check_rel(chh);
    return channel_recv_data(chh, rel, d, l, br);
}

sdtl_rv_t sdtl_channel_send_data(sdtl_channel_handle_t *chh, void *d, uint32_t l) {
    int rel = check_rel(chh);
    return channel_send_data(chh, rel, d, l);
}
