#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

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

