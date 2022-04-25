//
// Created by goofy on 10/23/21.
//

#ifndef ESWB_API_H
#define ESWB_API_H

// TODO: make name shorter
#include "eswb/topic_proclaiming_tree.h"

#include "eswb/types.h"
#include "eswb/errors.h"

#ifdef __cplusplus
extern "C" {
#endif

eswb_rv_t eswb_local_init(int do_reset);

eswb_rv_t eswb_create(const char *bus_name, eswb_type_t type, eswb_size_t max_topics);

eswb_rv_t eswb_delete(const char *bus_path);
eswb_rv_t eswb_delete_by_td(eswb_topic_descr_t td);

/**
 *
 * @param path
 * @param dir_name - dir name or NULL if full path is presented in 'path' argument
 * @return
 */
eswb_rv_t eswb_mkdir(const char *path, const char *dir_name);

eswb_rv_t eswb_proclaim_tree(eswb_topic_descr_t parent_td, topic_proclaiming_tree_t *bp, eswb_size_t tree_size,
                             eswb_topic_descr_t *new_td);

eswb_rv_t eswb_proclaim_tree_by_path(const char *mount_point, topic_proclaiming_tree_t *bp, eswb_size_t tree_size,
                                     eswb_topic_descr_t *new_td);
eswb_rv_t eswb_update_topic (eswb_topic_descr_t td, void *data);

eswb_rv_t eswb_subscribe(const char *path2topic, eswb_topic_descr_t *new_td);

eswb_rv_t eswb_read (eswb_topic_descr_t td, void *data);
eswb_rv_t eswb_get_update (eswb_topic_descr_t td, void *data);

eswb_rv_t eswb_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size);

eswb_rv_t eswb_get_topic_params (eswb_topic_descr_t td, topic_params_t *params);

eswb_rv_t eswb_get_next_topic_info (eswb_topic_descr_t td, eswb_topic_id_t *next2tid, topic_extract_t *info);

eswb_rv_t eswb_get_topic_path (eswb_topic_descr_t td, char *path);

eswb_rv_t eswb_fifo_subscribe(const char *mount_point, eswb_topic_descr_t *new_td);

eswb_rv_t eswb_fifo_push(eswb_topic_descr_t td, void *data);

eswb_rv_t eswb_fifo_pop(eswb_topic_descr_t td, void *data);

eswb_rv_t eswb_fifo_try_pop(eswb_topic_descr_t td, void *data);

eswb_rv_t eswb_topic_connect(const char *connection_point, eswb_topic_descr_t *td);

/// experimental:
eswb_rv_t eswb_connect_nested(eswb_topic_descr_t mp_td, const char *topic_name, eswb_topic_descr_t *td);

/// experimental:
eswb_rv_t
eswb_wait_connect_nested(eswb_topic_descr_t mp_td, const char *topic_name, eswb_topic_descr_t *td, uint32_t timeout_ms);


eswb_rv_t eswb_disconnect(eswb_topic_descr_t td);

const char *eswb_get_bus_prefix(eswb_type_t type);

eswb_rv_t eswb_path_compose(eswb_type_t type, const char *bus_name, const char *topic_subpath, char *result);
eswb_rv_t eswb_path_trailing_topic(const char *path, char **result);

/**
 *
 * @param full_path
 * @param path string of size ESWB_TOPIC_MAX_PATH_LEN
 * @param topic_name  string of size ESWB_TOPIC_NAME_MAX_LEN
 * @return
 */
eswb_rv_t eswb_path_split(const char *full_path, char *path, char *topic_name);

void eswb_print(const char *bus_name);

#ifdef __cplusplus
}
#endif

#endif //ESWB_API_H
