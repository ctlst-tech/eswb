#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "domain_switching.h"
#include "local_buses.h"

#define BUS_TYPE_MAX_LEN 10


static eswb_type_t bus_type(const char *bus_acr) {
    if (strcmp(bus_acr, "nsb") == 0) return eswb_non_synced;
    else if (strcmp(bus_acr, "itb") == 0) return eswb_inter_thread;
    else if (strcmp(bus_acr, "ipb") == 0) return eswb_inter_process;
    else return eswb_not_defined;
}

static eswb_rv_t normalize_path(const char *p, char *n) {
    int got_slash = 0;
    int cnt = ESWB_TOPIC_MAX_PATH_LEN;

    while ((*p != 0) && (cnt > 0)) {
        if (*p == '/') {
            if (!got_slash) {
                got_slash = -1;
            } else {
                p++;
                continue;
            }
        } else {
            got_slash = 0;
        }

        *n++ = *p++;
        cnt--;
    }

    *n = 0;

    if (cnt <= 0) {
        return eswb_e_path_too_long;
    }

    return eswb_e_ok;
}

static int check_path (const char *p) {
    int has_colon = 0;
    int got_first_slash = 0;

    if (!isalnum(p[0]) && (p[0] != '/')) {
        return 1;
    }

    do {
        switch (*p) {
            case '.':
            case '_':
                break;

            case '/':
                got_first_slash = -1;
                break;

            case ':':
                if (got_first_slash) {
                    return 1;
                }
                if (has_colon) {
                    return 1;
                } else {
                    has_colon = 1;
                }
                break;

            default:
                if (!isalnum(*p)) {
                    return 1;
                }
        }
        p++;
    } while (*p != 0);

    return 0;
}

static eswb_rv_t parse_path(const char *connection_point, eswb_type_t *t, char *bus_name, char *local_path) {
    eswb_type_t bt;
    char norm_cp[ESWB_TOPIC_MAX_PATH_LEN + 1];

    eswb_rv_t rv = normalize_path(connection_point, norm_cp);
    if (rv != eswb_e_ok) {
        return rv;
    }

    if (check_path(norm_cp)) {
        return eswb_e_inv_naming;
    }

    char *c = strstr(norm_cp, ":/");
    char *bus_name_p;
    if (c != NULL) {
        *c = 0;
        bt = bus_type(norm_cp);
        if (bt == eswb_not_defined) {
            return eswb_e_inv_bus_spec;
        }
        bus_name_p = &c[2];
    } else {
        bt = eswb_not_defined;
        bus_name_p = norm_cp;
    }

    if (local_path != NULL) {
        strcpy(local_path, bus_name_p);
    }

    if (bus_name != NULL) {
        c = strchr(bus_name_p, '/');
        size_t l = c != NULL ? c - bus_name_p : strlen(bus_name_p);
        strncpy(bus_name, bus_name_p, l);
        bus_name[l] = 0;
    }

    if (t != NULL) {
        *t = bt;
    }

    return eswb_e_ok;
}

eswb_rv_t eswb_parse_path_test_handler(const char *connection_point, eswb_type_t *t, char *bus_name, char *local_path) {
    return parse_path(connection_point, t, bus_name, local_path);
}

eswb_rv_t eswb_lookup_in_domain(const char *bus_name, eswb_type_t type, eswb_bus_handle_t **b) {
    switch (type) {
        case eswb_non_synced:
            return local_lookup_nsb(bus_name, b);

        case eswb_inter_thread:
            return local_lookup_itb(bus_name, b);

        case eswb_inter_process:
            return eswb_e_not_supported;

        case eswb_not_defined:
            ;
            eswb_rv_t rv = local_lookup_any(bus_name, b);
            if (rv != eswb_e_ok) {
                // TODO here must be lookup for inter process bus
            }
            return rv;

        default:
            return eswb_e_invargs;
    }
}

static eswb_rv_t
bus_lookup(const char *connection_point, eswb_bus_handle_t **bh, eswb_type_t *bus_type, char *local_path) {
    eswb_type_t e_bus_type;
    char bus_name[ESWB_BUS_NAME_MAX_LEN + 1];

    eswb_rv_t rv = parse_path(connection_point, &e_bus_type, bus_name, local_path);
    if (rv != eswb_e_ok) {
        return rv;
    }

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

        case eswb_non_synced:
            return local_bus_nsb_create(bus_name, max_topics);

        default:
            return eswb_e_invargs;
    }
}

eswb_rv_t ds_delete(const char *bus_path) {

    char *cp;
    eswb_bus_handle_t *bh;
    eswb_type_t bus_type;

    eswb_rv_t rv = bus_lookup(bus_path, &bh, &bus_type, NULL);
    if (rv != eswb_e_ok) {
        return rv;
    }

    switch (bus_type) {
        case eswb_inter_process:
            return eswb_e_not_supported;

        case eswb_not_defined:
        case eswb_inter_thread:
        case eswb_non_synced:
            return local_bus_delete(bh);

        default:
            return eswb_e_invargs;
    }
}

eswb_rv_t ds_connect(const char *connection_point, eswb_topic_descr_t *td) {

    char cp[ESWB_TOPIC_MAX_PATH_LEN + 1];
    eswb_bus_handle_t *bh;
    eswb_type_t bus_type;

    eswb_rv_t rv = bus_lookup(connection_point, &bh, &bus_type, cp);

    if (rv != eswb_e_ok) {
        return rv;
    }

    switch(bus_type) {
        case eswb_not_defined: // FIXME
        case eswb_non_synced:
        case eswb_inter_thread:
            rv = local_bus_connect(bh, cp, td);
            if (rv != eswb_e_ok) {
                return rv;
            }
            *td = -(*td);
            return rv;

        case eswb_inter_process:
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


eswb_rv_t ds_fifo_pop(eswb_topic_descr_t td, void *data, int do_wait) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_fifo_pop,
                          not_supported_stub,
                          data,
                          do_wait);
}


eswb_rv_t ds_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size) {
    SWITCH_FLOW_TO_DOMAIN(td,
                          local_ctl,
                          not_supported_stub,

                          ctl_type, d, size);
}
