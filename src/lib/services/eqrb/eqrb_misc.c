#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include "eqrb_priv.h"

void eqrb_debug_msg(const char *fn, const char *txt, ...) {
    va_list (args);
    char pn[16];
    pthread_getname_np(pthread_self(), pn, sizeof(pn));
    fprintf(stdout, "%s | %s | ", pn, fn);
    va_start (args,txt);
    vfprintf(stdout, txt, args);
    va_end (args);
    fprintf(stdout, "\n");
}


void *eqrb_alloc(size_t s) {
    return malloc(s);
}


const char *eqrb_strerror(eqrb_rv_t ecode) {
    switch (ecode) {
        case eqrb_rv_ok: return "eqrb_rv_ok";
        case eqrb_small_buf: return "eqrb_small_buf";
        case eqrb_inv_code: return "eqrb_inv_code";
        case eqrb_rv_nomem: return "eqrb_rv_nomem";
        case eqrb_notsup: return "eqrb_notsup";
        case eqrb_invarg: return "eqrb_invarg";
        case eqrb_nomem: return "eqrb_nomem";
        case eqrb_inv_size: return "eqrb_inv_size";
        case eqrb_eswb_err: return "eqrb_eswb_err";
        case eqrb_os_based_err: return "eqrb_os_based_err";
        case eqrb_media_err: return "eqrb_media_err";
        case eqrb_media_stop: return "eqrb_media_stop";
        case eqrb_media_invarg: return "eqrb_media_invarg";
        case eqrb_media_reset_cmd: return "eqrb_media_reset_cmd";
        case eqrb_media_remote_need_reset: return "eqrb_media_remote_need_reset";
        case eqrb_server_already_launched: return "eqrb_server_already_launched";
        case eqrb_rv_rx_eswb_fatal_err: return "eqrb_rv_rx_eswb_fatal_err";
        default: return "Unhandled error code";
    }
}
