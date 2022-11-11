//
// Created by goofy on 10/23/21.
//

#include <string.h>

#include "eswb/api.h"
#include "eswb_ctl.h"
#include "domain_switching.h"

eswb_rv_t local_buses_init(int do_reset);

eswb_rv_t eswb_local_init(int do_reset) {
    return local_buses_init(do_reset);
}

eswb_rv_t eswb_create(const char *bus_name, eswb_type_t type, eswb_size_t max_topics) {
    return ds_create(bus_name, type, max_topics);
}

eswb_rv_t eswb_delete(const char *bus_path) {
    return ds_delete(bus_path);
}

eswb_rv_t eswb_delete_by_td(eswb_topic_descr_t td) {
    char path[ESWB_TOPIC_MAX_PATH_LEN + 1];

    eswb_rv_t rv = eswb_get_topic_path(td, path);
    if (rv != eswb_e_ok) {
        return rv;
    }

    return eswb_delete(path);
}

eswb_rv_t eswb_disconnect(eswb_topic_descr_t td) {
    return ds_disconnect(td);
}

eswb_rv_t eswb_connect(const char *path2topic, eswb_topic_descr_t *new_td){
    return ds_connect(path2topic, new_td);
}

static eswb_rv_t restore_path_td_std(eswb_topic_descr_t mp_td, const char *topic_name, char *path) {
    eswb_rv_t rv = eswb_get_topic_path(mp_td, path);
    if (rv != eswb_e_ok) {
        return rv;
    }

    if (strlen(path) + strlen(topic_name) + 1 >= ESWB_TOPIC_MAX_PATH_LEN) {
        return eswb_e_path_too_long;
    }

    strcat(path, "/");
    strcat(path, topic_name);

    return eswb_e_ok;
}

eswb_rv_t eswb_connect_nested(eswb_topic_descr_t mp_td, const char *topic_name, eswb_topic_descr_t *td) {
    char full_path[ESWB_TOPIC_MAX_PATH_LEN + 1];

    eswb_rv_t rv = restore_path_td_std(mp_td, topic_name, full_path);
    if (rv != eswb_e_ok) {
        return rv;
    }

    return eswb_connect(full_path, td);
}

#include <unistd.h>

eswb_rv_t eswb_wait_connect_nested(eswb_topic_descr_t mp_td, const char *topic_name, eswb_topic_descr_t *td,
                                   uint32_t timeout_ms) {
    eswb_rv_t rv;

    char full_path[ESWB_TOPIC_MAX_PATH_LEN + 1];
    rv = restore_path_td_std(mp_td, topic_name, full_path);
    if (rv != eswb_e_ok) {
        return rv;
    }

    uint32_t timer_ms = 0;

    do {
        // TODO refactor this to wait for event
        rv = eswb_connect(full_path, td);
        if (rv == eswb_e_no_topic) {
#           define DELAY_MS 50
            usleep(DELAY_MS * 1000); // TODO platform agnostic
            timer_ms += DELAY_MS;
            if (timer_ms > timeout_ms) {
                rv = eswb_e_timedout;
                break;
            }
        }
    } while (rv == eswb_e_no_topic);

    return rv;
}


static inline eswb_rv_t do_update(eswb_topic_descr_t td, eswb_update_t ut, void *data, eswb_size_t elem_num) {
    return ds_update(td, ut, data, elem_num);
}

// TODO for now function dont reused in connection by path, what is sad.
eswb_rv_t eswb_proclaim_tree(eswb_topic_descr_t parent_td, topic_proclaiming_tree_t *bp, eswb_size_t tree_size,
                                     eswb_topic_descr_t *new_td) {
    eswb_rv_t rv;

    if (bp == NULL) {
        return eswb_e_invargs;
    }

    rv = do_update(parent_td, upd_proclaim_topic, bp, tree_size);
    if (rv != eswb_e_ok) {
        return rv;
    }

    return eswb_connect_nested(parent_td, bp->name, new_td);
}

eswb_rv_t eswb_proclaim_tree_by_path(const char *mount_point, topic_proclaiming_tree_t *bp, eswb_size_t tree_size,
                                     eswb_topic_descr_t *new_td) {

    eswb_topic_descr_t parent_td;
    eswb_rv_t rv = eswb_connect(mount_point, &parent_td);
    // TODO how to check, that we connected to the folder? Not the terminal topic?

    if (rv != eswb_e_ok) {
        return rv;
    }

    rv = do_update(parent_td, upd_proclaim_topic, bp, tree_size);
    if (rv != eswb_e_ok) {
        return rv;
    }

    eswb_disconnect(parent_td);

    if (new_td != NULL) {
        char new_topic_path[ESWB_TOPIC_MAX_PATH_LEN];
        strcpy(new_topic_path, mount_point);
        strcat(new_topic_path, "/");
        strcat(new_topic_path, bp->name);

        return eswb_connect(new_topic_path, new_td);
    } else {
        return rv;
    }
}


eswb_rv_t eswb_proclaim_plain(const char *mount_point, const char *topic_name, size_t data_size, eswb_topic_descr_t *new_td) {
    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 2);
    topic_proclaiming_tree_t *plain_root = usr_topic_set_root(cntx, topic_name, tt_plain_data, data_size);

    return eswb_proclaim_tree_by_path(mount_point, plain_root, cntx->t_num, new_td);
}


eswb_rv_t eswb_mkdir(const char *path, const char *dir_name) {
    char new_path[ESWB_TOPIC_MAX_PATH_LEN + 1];
    char new_dir_name[ESWB_TOPIC_NAME_MAX_LEN + 1];

    if (dir_name == NULL) {
        eswb_rv_t rv = eswb_path_split(path, new_path, new_dir_name);
        if (rv != eswb_e_ok) {
            return rv;
        }
        path = new_path;
        dir_name = new_dir_name;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 2);
    topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, dir_name, tt_dir, 0);

    return eswb_proclaim_tree_by_path(path, rt, cntx->t_num, NULL);
}

eswb_rv_t eswb_mkdir_nested(eswb_topic_descr_t parent_td, const char *dir_name, eswb_topic_descr_t *new_dir_td) {

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 2);
    topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, dir_name, tt_dir, 0);

    return eswb_proclaim_tree(parent_td, rt, cntx->t_num, new_dir_td);
}


eswb_rv_t eswb_update_topic (eswb_topic_descr_t td, void *data) {
    return do_update(td, upd_update_topic, data, 0);
}

eswb_rv_t eswb_read (eswb_topic_descr_t td, void *data) {
    return ds_read(td, data);
}

eswb_rv_t eswb_get_update (eswb_topic_descr_t td, void *data) {
    return ds_get_update(td, data);
}

eswb_rv_t eswb_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size) {
    return ds_ctl(td, ctl_type, d, size);
}

eswb_rv_t eswb_arm_timeout(eswb_topic_descr_t td, uint32_t timeout_us) {
    return eswb_ctl(td, eswb_ctl_arm_timeout, &timeout_us, sizeof(timeout_us));
}

eswb_rv_t eswb_get_topic_params (eswb_topic_descr_t td, topic_params_t *params) {
    return eswb_ctl(td, eswb_ctl_evq_get_params, params, sizeof(*params));
}

eswb_rv_t eswb_get_next_topic_info (eswb_topic_descr_t td, eswb_topic_id_t *next2tid, /*topic_extract_t*/ void *info) {
    union {
        eswb_topic_id_t             tid;
        topic_extract_t             xtract;
    } data = {
        .tid = *next2tid
    };
    eswb_rv_t rv;
    rv = eswb_ctl(td, eswb_ctl_get_next_proclaiming_info, &data, sizeof(data));
    if (rv == eswb_e_ok) {
        memcpy(info, &data.xtract, sizeof(data.xtract));
        *next2tid = data.xtract.info.topic_id;
    }

    return rv;
}


eswb_rv_t eswb_get_topic_path (eswb_topic_descr_t td, char *path) {
    return eswb_ctl(td, eswb_ctl_get_topic_path, path, 0);
}


eswb_rv_t eswb_fifo_subscribe(const char *path, eswb_topic_descr_t *new_td) {
    eswb_rv_t rv = eswb_connect(path, new_td);

    if (rv != eswb_e_ok) {
        return rv;
    }

    return rv;
}

eswb_rv_t eswb_fifo_push(eswb_topic_descr_t td, void *data) {
    return do_update(td, upd_push_fifo, data, 0);
}

eswb_rv_t eswb_fifo_pop(eswb_topic_descr_t td, void *data) {
    return ds_fifo_pop(td, data, 1);
}

eswb_rv_t eswb_fifo_flush(eswb_topic_descr_t td) {
    return ds_ctl(td, eswb_ctl_fifo_flush, NULL, 0);
}

eswb_rv_t eswb_fifo_try_pop(eswb_topic_descr_t td, void *data) {
    return ds_fifo_pop(td, data, 0);
}


void smartcat(char *dst, const char *tcat, int trailing_sl) {

    int sl = 0;

    char *initial_dst = dst;

    size_t len = strnlen(dst, ESWB_TOPIC_MAX_PATH_LEN);
    if (len > 0) {
        if (dst[len - 1] == '/') {
            sl = 1;
        }
    }

    while ( *dst != 0 ) {
        dst++;
    }

    do {
        if (*tcat == '/') {
            if (!sl) {
                *dst++ = *tcat;
                sl = 1;
            }
        } else {
            sl = 0;
            *dst++ = *tcat;
        }
    } while ( *tcat++ != 0 );

    if (( initial_dst < dst) && trailing_sl && !sl) {
        dst[-1] = '/';
        dst[0] = 0;
    }
}

const char *eswb_get_bus_prefix(eswb_type_t type) {
    switch (type) {
        case eswb_inter_thread:     return "itb:/";
        case eswb_inter_process:    return "ipb:/";
        case eswb_non_synced: return "nsb:/";
        default:
            return NULL;
    }
}

eswb_rv_t eswb_path_split(const char *full_path, char *path, char *topic_name) {
    if (full_path == NULL) {
        return eswb_e_invargs;
    }
    char *path_delim = strrchr(full_path,'/');
    if (path_delim == NULL) {
        return eswb_e_invargs;
    }
#define MIN(a,b) ((a) < (b)) ? (a) : (b)
    path_delim++;
    size_t strl = MIN(path_delim - full_path, ESWB_TOPIC_MAX_PATH_LEN);

    if (path != NULL) {
        strncpy(path, full_path, strl);
        path[strl] = 0;
    }
    if (topic_name != NULL) {
        strncpy(topic_name, path_delim, ESWB_TOPIC_NAME_MAX_LEN);
    }

    return eswb_e_ok;
}

eswb_rv_t eswb_path_trailing_topic(const char *path, char **result) {
    char *lst_slash;
    if (strnlen(path, ESWB_TOPIC_MAX_PATH_LEN) >= ESWB_TOPIC_MAX_PATH_LEN) {
        return eswb_e_path_too_long;
    }

    lst_slash = strrchr(path, '/');
    if (lst_slash == NULL) {
        return eswb_e_invargs;
    }

    *result = lst_slash;

    return eswb_e_ok;
}

eswb_rv_t eswb_path_compose(eswb_type_t type, const char *bus_name, const char *topic_subpath, char *result) {
    size_t l = 0;

    if (bus_name != NULL) {
        l = strnlen(bus_name, ESWB_TOPIC_MAX_PATH_LEN);
    }

    if (topic_subpath != NULL) {
        l += strnlen(topic_subpath, ESWB_TOPIC_MAX_PATH_LEN);
    }

    l += 4; // for type

    if (l > ESWB_TOPIC_MAX_PATH_LEN) {
        return eswb_e_inv_naming;
    }


    *result = 0;

    const char *prefix = eswb_get_bus_prefix(type);
    if (prefix == NULL) {
        return eswb_e_invargs;
    }

    smartcat(result, prefix, 0);
    if (bus_name != NULL) {
        smartcat(result, bus_name, 1);
    }

    if (topic_subpath != NULL) {
        smartcat(result, topic_subpath, 0);
    }

    return eswb_e_ok;
}

void eswb_print(const char *bus_name) {
//    eswb_bus_handle_t *bh;
//    eswb_rv_t rv = eswb_lookup(bus_name, &bh, NULL);
//    if (rv != eswb_e_ok) {
//        printf("No such bus: %s\n", bus_name);
//        return;
//    }
//
//    local_busses_print_registry(bh);
}
