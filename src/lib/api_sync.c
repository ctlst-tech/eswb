
#include "public/eswb/api_sync.h"

eswb_rv_t eswb_s_channel_create(const char *path, int req_size, int resp_size, eswb_sync_handle_t *rv) {

    int fifo_len = 2;

    // TODO allocate fifos for two directions

    // TODO how to work with non constant fifo structures? --> Avoid such for now

    // TODO how to handle multiple requests from different clients? --> assume that there is a only pair for now

    return eswb_e_ok;
}

eswb_rv_t eswb_s_channel_connect(const char *path, int req_size, int resp_size, eswb_sync_handle_t *rv) {

    int fifo_len = 2;

    // TODO check sizes of the fifo

    return eswb_e_ok;
}


eswb_rv_t eswb_s_request(eswb_sync_handle_t *ch, void *ds, void *dr) {
    eswb_rv_t rv;

    rv = eswb_fifo_push(ch->req_td, ds);

    if (rv != eswb_e_ok) {
        return rv;
    }

    rv = eswb_fifo_pop(ch->resp_td, dr);

    return rv;
}

eswb_rv_t eswb_s_receive(eswb_sync_handle_t *ch, void *ds) {
    return eswb_fifo_pop(ch->req_td, ds);
}

eswb_rv_t eswb_s_reply(eswb_sync_handle_t *ch, void *dr) {
    return eswb_fifo_push(ch->req_td, dr);
}
