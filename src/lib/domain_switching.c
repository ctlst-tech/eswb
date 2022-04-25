//
// Created by goofy on 12/25/21.
//

#include <string.h>
#include <stdio.h>

#include "domain_switching.h"
#include "local_buses.h"

#define BUS_TYPE_MAX_LEN 10

eswb_sys_type_t eswb_type(const char *bus_acr) {
    if (strcmp(bus_acr, "itb") == 0) return _inter_thread;
    else if (strcmp(bus_acr, "ipb") == 0) return _inter_process;
    else if (strcmp(bus_acr, "nsb") == 0) return _non_synced_bus;
    else return _invalid_type;
}


static eswb_rv_t parse_path(const char *connection_point, eswb_sys_type_t *t, char *bus_name, char **local_path) {
    char bus_type[BUS_TYPE_MAX_LEN + 1];

#   define SH(__s)  #__s
#   define S(__s)  SH(__s)

#   define NAME_REGEXP "[A-Za-z0-9_]"

#   define CP_SCANF_FMT_BASE "%" S(BUS_TYPE_MAX_LEN) "[itbnsp]:/%" S(ESWB_BUS_NAME_MAX_LEN) NAME_REGEXP
#   define CP_SCANF_FMT_FULL CP_SCANF_FMT_BASE "/%s"

    sscanf(connection_point, CP_SCANF_FMT_BASE, bus_type, bus_name);

    if (local_path != NULL) {
        char *c = strstr(connection_point, ":/");  // TODO not reliable?
        if (c != NULL) {
            *local_path = c + 2;
        }
    }

    if (t != NULL) {
        *t = eswb_type(bus_type);
    }

    return eswb_e_ok;
}


eswb_rv_t eswb_lookup_in_domain(const char *bus_name, eswb_sys_type_t type, eswb_bus_handle_t **b) {
    switch (type) {
        case _inter_thread:
            return local_itb_lookup(bus_name, b);

        case _inter_process:
            return eswb_e_not_supported;

        case _non_synced_bus:
            return local_nsb_lookup(bus_name, b);

        default:
            return eswb_e_invargs;
    }
}

static eswb_rv_t
bus_lookup(const char *connection_point, eswb_bus_handle_t **bh, eswb_sys_type_t *bus_type, char **local_path) {
    eswb_sys_type_t e_bus_type;
    char bus_name[ESWB_BUS_NAME_MAX_LEN + 1];

    parse_path(connection_point, &e_bus_type, bus_name, local_path);

    if (bus_type != NULL) {
        *bus_type = e_bus_type;
    }

    return eswb_lookup_in_domain(bus_name, e_bus_type, bh);
}

eswb_rv_t ds_create(const char *bus_name, eswb_type_t type, eswb_size_t max_topics) {
    switch (type) {
        case eswb_inter_thread:
            return local_bus_itb_create(bus_name, max_topics);

        case eswb_inter_process:
            return eswb_e_not_supported;

        case eswb_local_non_synced:
            return local_bus_nsb_create(bus_name, max_topics);

        default:
            return eswb_e_invargs;
    }
}

eswb_rv_t ds_delete(const char *bus_path) {

    char *cp;
    eswb_bus_handle_t *bh;
    eswb_sys_type_t bus_type;

    eswb_rv_t rv = bus_lookup(bus_path, &bh, &bus_type, NULL);
    if (rv != eswb_e_ok) {
        return rv;
    }

    switch (bus_type) {
        case _inter_process:
            return eswb_e_not_supported;

        case _inter_thread:
        case _non_synced_bus:
            return local_bus_delete(bh);

        default:
            return eswb_e_invargs;
    }
}

eswb_rv_t ds_connect(const char *connection_point, eswb_topic_descr_t *td) {

    char *cp;
    eswb_bus_handle_t *bh;
    eswb_sys_type_t bus_type;

    eswb_rv_t rv = bus_lookup(connection_point, &bh, &bus_type, &cp);

    if (rv != eswb_e_ok) {
        return rv;
    }

    switch(bus_type) {
        case _non_synced_bus:
        case _inter_thread:
            rv = local_bus_connect(bh, cp, td);
            if (rv != eswb_e_ok) {
                return rv;
            }

            *td = -(*td);
            return rv;

        case _inter_process:
            return eswb_e_not_supported;

        default:
            return eswb_e_invargs;
    }
}

#define SWITCH_FLOW_TO_DOMAIN(__td, __local_call, __interprocess_call, ...)      \
if ((__td) < 0) {                                                                \
    return __local_call(-(__td), __VA_ARGS__);                                   \
} else if ((__td > 0 )){                                                         \
    return eswb_e_not_supported;                                                 \
} else {                                                                         \
    return eswb_e_invargs;                                                       \
}

eswb_rv_t not_supported_stub(eswb_topic_descr_t td, ...) {
    return eswb_e_not_supported;
}

eswb_rv_t ds_disconnect(eswb_topic_descr_t td) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          not_supported_stub,
                          not_supported_stub,
                          1);
}


eswb_rv_t ds_update(eswb_topic_descr_t td, eswb_update_t ut, void *data, eswb_size_t elem_num) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_do_update,
                          not_supported_stub,

                          ut, data, elem_num);
}

eswb_rv_t ds_read (eswb_topic_descr_t td, void *data) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_do_read,
                          not_supported_stub,

                          data);
}

eswb_rv_t ds_get_update (eswb_topic_descr_t td, void *data) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_get_update,
                          not_supported_stub,

                          data);
}


eswb_rv_t ds_fifo_pop (eswb_topic_descr_t td, void *data) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_fifo_pop,
                          not_supported_stub,

                          data);
}


eswb_rv_t ds_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_ctl,
                          not_supported_stub,

                          ctl_type, d, size);
}
