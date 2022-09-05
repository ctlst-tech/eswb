#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sdtl.h"
#include "bbee_framing.h"
#include "eswb/api.h"
#include "eswb/topic_proclaiming_tree.h"

sdtl_channel_t *resolve_channel_by_id(sdtl_service_t *s, uint8_t ch_id) {

    for (int i = 0; i < s->channels_num; i++) {
        if (s->channels[i].cfg->id == ch_id) {
            return &s->channels[i];
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

sdtl_rv_t sdtl_service_start(sdtl_service_t *s, const char *ser_path, uint32_t baudrate, uint32_t mtu) {
    // TODO open port

    // TODO setup baudrate

    // TODO create channels

    // TODO start rx thread

    /* TODO define parameters
     *  1. MTU ?
     *  2. Delay before resend
     */
}

sdtl_rv_t sdtl_service_stop(sdtl_service_t *s) {

    // TODO stop thread

    // TODO close port

}

void *sdtl_alloc(size_t s) {
    return calloc(1, s);
}

static sdtl_rv_t process_data(sdtl_channel_t *ch, sdtl_data_sub_header_t *data_header) {
    // TODO check state

    eswb_rv_t erv = eswb_fifo_push(ch->data_td, data_header);

    return erv == eswb_e_ok ? SDTL_OK : SDTL_ESWB_ERR;
}

static sdtl_rv_t process_ack(sdtl_channel_t *ch, sdtl_ack_sub_header_t *ack_header) {
    // TODO check state

    eswb_rv_t erv = eswb_fifo_push(ch->ack_td, ack_header);

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

    // validate packet
    rv = validate_base_header(base_header, pkt_type, data_len);
    if (rv != SDTL_OK) {
        // TODO send ack with error
        return rv;
    }

    // resolve channel

    sdtl_channel_t *ch = resolve_channel_by_id(s, base_header->ch_id);
    if (ch == NULL) {
        // TODO send ack with error
        return SDTL_NO_CHANNEL_LOCAL;
    }

    switch (pkt_type) {
        case SDTL_PKT_ATTR_PKT_TYPE_DATA:
            ;
            sdtl_data_header_t *dhdr = (sdtl_data_header_t *) base_header;
            rv = process_data(ch, &dhdr->sub);
            break;

        case SDTL_PKT_ATTR_PKT_TYPE_ACK:
            ;
            sdtl_ack_header_t *ahdr = (sdtl_ack_header_t *) base_header;
            rv = process_ack(ch, &ahdr->sub);
            break;

        default:
            // TODO throw invalid packet error
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
    sdtl_rv_t media_rv;

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

_Noreturn sdtl_rv_t sdtl_service_rx_thread(sdtl_service_t *s) {

    eswb_rv_t erv;
    bbee_frm_rx_state_t rx_state;

    size_t payload_size = s->mtu;
    size_t rx_buf_size = payload_size << 1; // * 2


    uint8_t *rx_buf = sdtl_alloc(rx_buf_size);
    uint8_t *payload_buf = sdtl_alloc(payload_size);

    bbee_frm_init_state(&rx_state, payload_buf, payload_size);

    sdtl_rv_t media_rv;

    do {
        size_t rx_buf_lng;
        media_rv = s->media_read(s->media_handle, rx_buf, rx_buf_size, &rx_buf_lng);

        if (media_rv != SDTL_OK) {

        }
//      eqrb_dbg_msg("recv | rx_buf_lng == %d rv == %d", rx_buf_lng, rv);

#       ifdef EQRB_DEBUG
        print_debug_data(rx_buf, rx_buf_lng)
#       endif

        bbee_frm_process_rx_buf(s, rx_buf, rx_buf_lng, &rx_state, sdtl_got_frame_handler);
    } while (1);

    return SDTL_OK;
}


static sdtl_rv_t media_tx(sdtl_service_t *s, void *header, uint32_t hl, void *d, uint32_t l) {
    // TODO frame buffer and vectorization

    s->media_write(s->media_handle, header, hl, NULL);
    return s->media_write(s->media_handle, d, l, NULL);
}



#define my_min(a,b) ((a) < (b)) ? (a) : (b)

static sdtl_rv_t send_data(sdtl_channel_t *ch, int rel, int last_pkt, void *d, uint32_t l) {
    sdtl_data_header_t hdr;

    hdr.base.attr = SDTL_PKT_ATTR_PKT_TYPE(SDTL_PKT_ATTR_PKT_TYPE_DATA);

    hdr.base.ch_id = ch->cfg->id;

    // TODO handle states properly
    hdr.sub.cnt = ch->tx_state.next_pkt_cnt;
    hdr.sub.payload_size = l;
    hdr.sub.flags = (last_pkt ? SDTL_PKT_DATA_FLAG_LAST_PKT : 0) |
                    (rel ? SDTL_PKT_DATA_FLAG_RELIABLE : 0);

    return media_tx(ch->service, &hdr, sizeof(hdr), d, l);
}


static sdtl_rv_t send_ack(sdtl_channel_t *ch, uint32_t ack_code) {
    sdtl_data_header_t hdr;

    // TODO
    return media_tx(ch->service, &hdr, sizeof(hdr), NULL, 0);
}


static sdtl_rv_t wait_ack(sdtl_channel_handle_t *chh, uint32_t timeout_us) {
    // TODO arm timeout
    // TODO wait on fifo
    // TODO process return code
    return SDTL_OK;
}


static sdtl_rv_t wait_data(sdtl_channel_handle_t *chh, void *d, uint32_t l, uint32_t *br) {
    // TODO wait data
    // TODO process rv
    return SDTL_OK;
}


static sdtl_rv_t update_tx_state(sdtl_channel_handle_t *chh, int pkt_cnt) {
    // TODO make enumeration for transition, make transitions accordingly

    // TODO

    if (pkt_cnt > 0) {

    }

    return SDTL_OK;
}


static sdtl_rv_t channel_send_data(sdtl_channel_handle_t *chh, int rel, void *d, uint32_t l) {
    sdtl_pkt_payload_size_t dsize;
    uint32_t offset = 0;

    sdtl_rv_t rv = SDTL_OK;

    do {
        dsize = my_min(chh->channel->max_payload_size, l);

        do {
            int last_pkt = dsize <= l;

            rv = send_data(chh->channel, rel, last_pkt, d + offset, dsize);
            // TODO get timeout from somewhere
            if (rel) {
                rv = wait_ack(chh, 4000);
            }
            // TODO handle state with sync
            update_tx_state(chh, chh->channel->tx_state.next_pkt_cnt + 1);
        } while (rv != SDTL_OK);

        l -= dsize;
        offset += dsize;
    } while (l > 0);

    return rv;
}

static sdtl_rv_t update_rx_state(sdtl_channel_t *ch, int pkt_cnt) {

    // TODO

    if (pkt_cnt > 0) {

    }

    return SDTL_OK;
}

static sdtl_rv_t channel_recv_data(sdtl_channel_t *ch, int rel, void *d, uint32_t l, uint32_t *br) {
    uint32_t offset = 0;
    uint32_t br_pkt = 0;
    uint32_t br_total = 0;

    sdtl_rv_t rv_rcv;
    sdtl_rv_t rv_ack;
    sdtl_rv_t rv = SDTL_OK;

    int loop = -1;

    do {
        rv_rcv = wait_data(ch, d + offset, l, &br_pkt);
        switch (rv_rcv) {
            default:
            case SDTL_RX_BUF_SMALL:
                rv = rv_rcv;
                loop = 0;
                break;

            case SDTL_OK:
            case SDTL_OK_LAST_PACKET:
                rv_ack = send_ack(ch, SDTL_ACK_GOT_PKT);
                if (rv_ack == SDTL_OK) {
                    offset += br_pkt;
                    l -= br_pkt;

                    update_rx_state(ch, ch->rx_state.expected_pkt_cnt + 1);
                    if (rv_rcv == SDTL_OK_LAST_PACKET) {
                        rv = SDTL_OK;
                    }
                }
                break;
        }

    } while ((rv_rcv != SDTL_OK_LAST_PACKET) && loop);

    if (rv == SDTL_OK) {
        *br = offset;
    }

    return rv;
}


#define CHANNEL_DATA_FIFO_NAME "data_fifo"
#define CHANNEL_ACK_FIFO_NAME "ack_fifo"

#define CHANNEL_DATA_BUF_NAME "data_buf"
#define CHANNEL_ACK_BUF_NAME "ack_buf"

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

sdtl_rv_t sdtl_channel_create(sdtl_service_t *s, sdtl_channel_cfg_t *cfg, sdtl_channel_t **rv) {

    if (resolve_channel_by_id(s, cfg->id)) {
        return SDTL_CH_EXIST;
    }

    sdtl_channel_t *ch = sdtl_alloc(sizeof(*ch));
    if (ch == NULL) {
        return SDTL_NO_MEM;
    }


    size_t mtu = my_min(s->mtu, cfg->mtu_override);
    int max_payload_size = mtu - sizeof(sdtl_data_header_t);
    if (max_payload_size < 0) {
        return SDTL_INVALID_MTU;
    }

    ch->cfg = cfg;
    ch->service = s;
    ch->max_payload_size = max_payload_size;

    eswb_rv_t erv;

    erv = eswb_mkdir(s->eswb_root, cfg->name);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    if (check_channel_both_paths(s->eswb_root, cfg->name) != SDTL_OK) {
        return SDTL_NAMES_TOO_LONG;
    }

    char path[ESWB_TOPIC_MAX_PATH_LEN + 1];

    strcpy(path, s->eswb_root);
    strcat(path, "/");
    strcat(path, cfg->name);

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 4);
    topic_proclaiming_tree_t *data_fifo_root = usr_topic_set_fifo(cntx, CHANNEL_DATA_FIFO_NAME, 4);
    usr_topic_add_child(cntx, data_fifo_root, CHANNEL_DATA_BUF_NAME, tt_byte_buffer, 0, mtu, 0);
    erv = eswb_proclaim_tree_by_path(path, data_fifo_root, cntx->t_num, NULL);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    TOPIC_TREE_CONTEXT_LOCAL_RESET(cntx);
    topic_proclaiming_tree_t *ack_fifo_root = usr_topic_set_fifo(cntx, CHANNEL_ACK_FIFO_NAME, 4);
    usr_topic_add_child(cntx, ack_fifo_root, CHANNEL_ACK_BUF_NAME, tt_byte_buffer, 0, sizeof(sdtl_ack_sub_header_t), 0);
    erv = eswb_proclaim_tree_by_path(path, ack_fifo_root, cntx->t_num, NULL);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    return SDTL_OK;
}

static eswb_rv_t open_fifo(const char *base_path, const char* ch_name, const char *subpath, eswb_topic_descr_t *rv_td) {
    eswb_rv_t erv;

    char path[ESWB_TOPIC_MAX_PATH_LEN];

    strcpy(path, base_path);
    strcat(path, "/");
    strcat(path, ch_name);
    strcat(path, "/");
    strcat(path, subpath);

    return eswb_connect(path, rv_td);
}

sdtl_rv_t sdtl_channel_open(sdtl_service_t *s, const char *channel_name, sdtl_channel_handle_t *rv) {

    sdtl_channel_t *ch = resolve_channel_by_name(s, channel_name);
    if (ch == NULL) {
        return SDTL_NO_CHANNEL_LOCAL;
    }

    if (check_channel_both_paths(s->eswb_root, channel_name) != SDTL_OK) {
        return SDTL_NAMES_TOO_LONG;
    }

    rv->channel = ch;

    eswb_rv_t erv;
    erv = open_fifo(s->eswb_root, channel_name, CHANNEL_DATA_SUBPATH, &rv->data_td);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }

    erv = open_fifo(s->eswb_root, channel_name, CHANNEL_ACK_SUBPATH, &rv->ack_td);
    if (erv != eswb_e_ok) {
        return SDTL_ESWB_ERR;
    }


    return SDTL_OK;
}

static int check_rel(sdtl_channel_handle_t *ch) {
    return (ch->channel->cfg->type == SDTL_CHANNEL_RELIABLE);
}

sdtl_rv_t sdtl_channel_recv_data(sdtl_channel_handle_t *chh, void *d, uint32_t l, uint32_t *br) {
    int rel = check_rel(chh);
    return channel_recv_data(chh, rel, d, l, br);
}

sdtl_rv_t sdtl_channel_send_data(sdtl_channel_handle_t *chh, void *d, uint32_t l) {
    int rel = check_rel(chh);
    return channel_send_data(chh, rel, d, l);
}


sdtl_rv_t sdtl_channel_wait_cmd(sdtl_channel_t *ch) {

}

sdtl_rv_t sdtl_channel_send_cmd(sdtl_channel_t *ch) {

}