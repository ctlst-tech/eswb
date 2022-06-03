//
// Created by goofy on 06.12.2020.
//

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


void eswb_set_thread_name(const char *n) {
#if __APPLE__
    pthread_setname_np(n);
#elif __linux__
    pthread_setname_np(pthread_self(), n);
#else
    #ifdef _FREERTOS_POSIX_PTHREAD_H_
        pthread_setname_np(n);
    #else
        warning "eswb_set_thread_name is not supported"
    #endif
#endif
}
