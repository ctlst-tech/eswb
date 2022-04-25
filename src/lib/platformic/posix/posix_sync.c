//
// Created by goofy on 12/25/21.
//

#include <pthread.h>
#include <stdlib.h> // for malloc
#include <errno.h>
#include <string.h>

#include "eswb/errors.h"
#include "eswb/types.h"
#include "sync.h"



typedef struct sync_handle {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int last_err;
} posix_sync_t;

eswb_rv_t sync_create(posix_sync_t **s){

    posix_sync_t *ps;

    // TODO remove calloc
    ps = calloc(1, sizeof(*ps));
    if (ps == NULL) {
        return eswb_e_mem_sync_na;
    }

    pthread_condattr_t cattr;
    pthread_mutexattr_t mattr;

    pthread_mutexattr_init(&mattr);
//    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
//    pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
//
//    pthread_condattr_init(&cattr);
//    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    if ((ps->last_err = pthread_mutex_init(&ps->mutex, &mattr)) != 0) {
        // TODO dealloc ps
        return eswb_e_sync_init;
    }

    if ((ps->last_err = pthread_cond_init(&ps->cond, &cattr)) != 0) {
        // TODO deinit mutex
        // TODO dealloc ps
        return eswb_e_sync_init;
    }

    *s = ps;

    return eswb_e_ok;
}



eswb_rv_t sync_take(posix_sync_t *ps){

    ps->last_err = pthread_mutex_lock(&ps->mutex);
    if (ps->last_err == EOWNERDEAD) {
//        fprintf(stderr, "pthread_mutex_lock -> EOWNERDEAD\n");
//        ps->last_err = pthread_mutex_consistent(&ps->mutex);
//        if (ps->last_err == EINVAL) {
//            fprintf(stderr, "pthread_mutex_consistent failed\n");
//            return eswb_e_sync_inconsistent;
//        }
    } else if (ps->last_err == ENOTRECOVERABLE) {
        return eswb_e_sync_inconsistent;
    }
    return (ps->last_err == 0) ? eswb_e_ok : eswb_e_sync_take;
}

eswb_rv_t sync_give(posix_sync_t *ps){
    //posix_sync_t *ps = posix_sync_cast(s);
    return ((ps->last_err = pthread_mutex_unlock(&ps->mutex)) == 0) ? eswb_e_ok : eswb_e_sync_give;
}

eswb_rv_t sync_wait(posix_sync_t *ps){
    int rv;
    //posix_sync_t *ps = posix_sync_cast(s);
    struct timespec ts;

    do {
//        clock_gettime(CLOCK_REALTIME, &ts);
//        ts.tv_sec += 1;
//        ps->last_err = rv = pthread_cond_timedwait(&ps->cond, &ps->mutex, &ts);
//        // TODO proper exit
        ps->last_err = rv = pthread_cond_wait(&ps->cond, &ps->mutex);
    } while (rv != 0);


    return (ps->last_err == 0) ? eswb_e_ok : eswb_e_sync_wait;
}

eswb_rv_t sync_broadcast(posix_sync_t *ps){
    //posix_sync_t *ps = posix_sync_cast(s);
    return ((ps->last_err = pthread_cond_broadcast(&ps->cond)) == 0) ? eswb_e_ok : eswb_e_sync_broadcast;
}

eswb_rv_t sync_destroy(posix_sync_t *ps){
    //posix_sync_t *ps = posix_sync_cast(s);

    pthread_mutex_destroy(&ps->mutex);
    pthread_cond_destroy(&ps->cond);

    // TODO check errors
    free(ps);

    return eswb_e_ok;
}

const char *sync_last_strerror(posix_sync_t *ps){
    //posix_sync_t *ps = posix_sync_cast(s);
    return strerror(ps->last_err);
}
