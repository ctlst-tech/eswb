#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eswb/api.h"
#include "eswb/bridge.h"
#include "eswb/event_queue.h"
#include "eswb/services/eqrb.h"

int test_event_chain (int verbose, int nonstop);

//int start_eqrb_server() {
//    return eqrb_tcp_server_start(0) == eqrb_rv_ok ? 0 : 1;
//}

int main(int argc, char *argv[]) {
    int non_stop = -1;
    int tcp_server = -1;
    int verbose = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "onepass") == 0) {
            non_stop = 0;
        } else if  (strcmp(argv[i], "no_tcp_server") == 0) {
            tcp_server = 0;
        } else if  (strcmp(argv[i], "verbose") == 0) {
            verbose = -1;
        }
    }

//    if (tcp_server) {
//        int trv = start_eqrb_server();
//        if (trv) {
//            fprintf(stderr, "start_eqrb_server failed\n");
//        }
//    }

    return test_event_chain(verbose, non_stop);;
}
