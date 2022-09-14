#ifndef API_SYNC_H
#define API_SYNC_H

#include "eswb/api.h"

typedef struct eswb_sync_handle {
    eswb_topic_descr_t req_td; // request
    eswb_topic_descr_t resp_td; // response
} eswb_sync_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

eswb_rv_t eswb_s_channel_create(const char *path, int req_size, int resp_size, eswb_sync_handle_t *rv);
eswb_rv_t eswb_s_channel_connect(const char *path, int req_size, int resp_size, eswb_sync_handle_t *rv);
eswb_rv_t eswb_s_request(eswb_sync_handle_t *ch, void *ds, void *dr);
eswb_rv_t eswb_s_receive(eswb_sync_handle_t *ch, void *ds);
eswb_rv_t eswb_s_reply(eswb_sync_handle_t *ch, void *dr);

#ifdef __cplusplus
}
#endif

#endif //API_SYNC_H
