#include <string.h>

#include <pthread.h>

#include "misc.h"

char *indent(int depth) {
    static char rv[60];
    rv[0] = 0;

    for (int i=0; i < depth; i++) {
        strcat(rv, "    ");
    }

    return rv;
}

void eswb_set_delta_priority(int dp) {
    struct sched_param shp;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &shp);
    shp.sched_priority += dp;
    pthread_setschedparam(pthread_self(), policy, &shp);
}

void eswb_set_thread_name(const char *n) {
#ifdef __APPLE__
    pthread_setname_np(n);
#else
    #ifdef __linux__
        pthread_setname_np(pthread_self(), n);
    #else
        #ifdef _FREERTOS_POSIX_PTHREAD_H_
            pthread_setname_np(n);
        #else
            #ifdef __EXT_QNX
                pthread_setname_np(pthread_self(), n);
            #else
                #warning "eswb_set_thread_name is not supported"
            #endif
        #endif
    #endif
#endif
}
