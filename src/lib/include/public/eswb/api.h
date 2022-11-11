#ifndef ESWB_API_H
#define ESWB_API_H

/** @page api
 * Check api.h for calls descriptions
 */

/** @file */

// TODO: make name shorter
#include "eswb/topic_proclaiming_tree.h"

#include "eswb/types.h"
#include "eswb/errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reset local busses by flushing them and theirs topics registries. Important to to use this call when you have to
 * restart initialization within one runtime cycle.
 * @param do_reset must be non zero to take effect
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_local_init(int do_reset);

/**
 * Create a bus
 * @param bus_name
 * @param type
 *   eswb_non_synced    - no synchronization and mutual exclusion are provided for topics access
 *   eswb_inter_thread  - with synchronized access to topics and with blocked updated calls
 *   eswb_inter_process - NOT SUPPORTED YET; will create a broker process
 * @param max_topics number of topics to allocate inside a bus registry
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_create(const char *bus_name, eswb_type_t type, eswb_size_t max_topics);

/**
 * Delete bus by its name
 * @param bus_path path / name of the bus
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_delete(const char *bus_path);

/**
 * Delete bus by any of its topic descriptor
 * @param td topic descriptor of any bus's topic
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_delete_by_td(eswb_topic_descr_t td);

/**
 * Create a directory within a path
 * @param path root of the directory
 * @param dir_name dir name or NULL if full path is presented in 'path' argument
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_mkdir(const char *path, const char *dir_name);

/**
 * Create directory as a child to a specified descriptor
 * @param parent_td
 * @param dir_name
 * @return
 */
eswb_rv_t eswb_mkdir_nested(eswb_topic_descr_t parent_td, const char *dir_name, eswb_topic_descr_t *new_dir_td);

/**
 * Publish tree of the topics to the topic with a specified descriptor
 * @param parent_td topic descriptor of the root, must have type tt_dir
 * @param bp tree initialized by usr_topic_* calls
 * @param tree_size number of elements inside bp
 * @param new_td pointer to topic descriptor variable assigned to the just published topic in case of success
 * and if it non NULL.
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_proclaim_tree(eswb_topic_descr_t parent_td, topic_proclaiming_tree_t *bp, eswb_size_t tree_size,
                             eswb_topic_descr_t *new_td);

/**
 * Publish tree of the topics to the specified path
 * @param mount_point path to the topic the root to publish, must have type tt_dir
 * @param bp tree initialized by usr_topic_* calls
 * @param tree_size number of elements inside bp
 * @param new_td pointer to topic descriptor variable assigned to the just published topic in case of success, might NULL
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_proclaim_tree_by_path(const char *mount_point, topic_proclaiming_tree_t *bp, eswb_size_t tree_size,
                                     eswb_topic_descr_t *new_td);

/**
 * Proclaim plain data
 * @param mount_point path to the topic the root to publish, must have type tt_dir
 * @param topic_name name of the plain topic
 * @param data_size size of the plain data to allocate
 * @param new_td pointer to topic descriptor variable assigned to the just published topic in case of success, might NULL
 * @return
 */
eswb_rv_t eswb_proclaim_plain(const char *mount_point, const char *topic_name, size_t data_size, eswb_topic_descr_t *new_td);

/**
 * Update topic value and notify blocked subscribers
 * @param td topic descriptor
 * @param data data to publish; must have a size according to the topic size
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_update_topic (eswb_topic_descr_t td, void *data);

/**
 * Subscribe on topic
 * @param path2topic path to the topic to subscribe
 * @param new_td pointer to save new topic descriptor value in case of success
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_connect(const char *path2topic, eswb_topic_descr_t *new_td);

/**
 * Non blocking read of topic's current value
 * @param td topic descriptor
 * @param data data to read; must have a size according to the topic size
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_read (eswb_topic_descr_t td, void *data);

/**
 * Blocking read of topic's value, returned right after its value publication
 * @param td topic descriptor
 * @param data data to read; must have a size according to the topic size
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_get_update (eswb_topic_descr_t td, void *data);

/**
 * Get topic's information by its descriptor
 * @param td topic descriptor
 * @param params pointer to an allocated parameters strucutre to store parameters on success
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_get_topic_params (eswb_topic_descr_t td, topic_params_t *params);


/**
 * Retrieve topics
 * @param td topic descriptor of the root to retrieve its children topics
 * @param next2tid pointer to the id value for the topic, preceeding the desired. At the first call value at this ref must be zero.
 *              After successfull return it contains the id of just retrieved topic info
 * @param info retrieved information on success must be pointer to topic_extract_t

* @return eswb_e_ok on success
 */

eswb_rv_t eswb_get_next_topic_info (eswb_topic_descr_t td, eswb_topic_id_t *next2tid, void *info);

/**
 * Retrieve full path of the topic by its descriptor
 * @param td topic descriptor
 * @param path string with a size of ESWB_TOPIC_MAX_PATH_LEN + 1
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_get_topic_path (eswb_topic_descr_t td, char *path);

/**
 * Subscribe on fifo topic
 * @param path path to fifo
 * @param new_td pointer to save new topic descriptor value in case of success
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_fifo_subscribe(const char *path, eswb_topic_descr_t *new_td);

/**
 * Push new value to fifo
 * @param td fifo's topic descriptor
 * @param data pointer to data to push; must have a size according to the fifo element size
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_fifo_push(eswb_topic_descr_t td, void *data);

/**
 * Pop value from fifo
 * @param td fifo's topic descriptor
 * @param data pointer to data to save popped element; must point to the data with a size equal to the proclaimed
 * fifo's element size
 * @return eswb_e_ok on success
 * eswb_e_no_update if FIFO is created on NSB bus type and there are no elements in queue
 */
eswb_rv_t eswb_fifo_pop(eswb_topic_descr_t td, void *data);

/**
 * Flush fifo for specified td
 * @param td fifo's topic descriptor
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_fifo_flush(eswb_topic_descr_t td);


/**
 * Check if there was an update since the last call and pop it if so
 * @param td fifo's topic descriptor
 * @param data pointer to data to save popped element; must point to the data with a size equal to the proclaimed
 * fifo's element size
 * @return
 *  eswb_e_ok on success
 *  eswb_e_no_update if there were no updates since the last call
 */
eswb_rv_t eswb_fifo_try_pop(eswb_topic_descr_t td, void *data);

/**
 * Arm timeout for getting updates from synchronized calls
 * @param td topic descriptor
 * @param timeout_us timeout value in microseconds
 * @return
 *  eswb_e_ok on success
 */
eswb_rv_t eswb_arm_timeout(eswb_topic_descr_t td, uint32_t timeout_us);

/**
 *
 * @param mp_td
 * @param topic_name
 * @param td
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_connect_nested(eswb_topic_descr_t mp_td, const char *topic_name, eswb_topic_descr_t *td);

/**
 *
 * @param mp_td
 * @param topic_name
 * @param td
 * @param timeout_ms
 * @return
 *  eswb_e_ok on success
 *  eswb_e_timedout was not able to connect within specified timeframe
 */
eswb_rv_t
eswb_wait_connect_nested(eswb_topic_descr_t mp_td, const char *topic_name, eswb_topic_descr_t *td, uint32_t timeout_ms);

/**
 * Disconnect from topic
 * @param td topic descriptor
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_disconnect(eswb_topic_descr_t td);

/**
 * Get a bus prefix associated with specific bus type
 * @param type bus type
 * @return string constant of bus prefix or NULL if type is invalid
 */
const char *eswb_get_bus_prefix(eswb_type_t type);

/**
 * Compose full path to a topic
 * @param type bus type
 * @param bus_name name of the bus
 * @param topic_subpath subpath wihtin a bus
 * @param result pointer to resulting string with a size ESWB_TOPIC_MAX_PATH_LEN
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_path_compose(eswb_type_t type, const char *bus_name, const char *topic_subpath, char *result);

/**
 * Split a full path to the specific topic
 * @param full_path pointer to the full path to the topic
 * @param path pointer to the basepath of the topic (string of size ESWB_TOPIC_MAX_PATH_LEN)
 * @param topic_name pointer to the topic's name (string of size ESWB_TOPIC_NAME_MAX_LEN)
 * @return eswb_e_ok on success
 */
eswb_rv_t eswb_path_split(const char *full_path, char *path, char *topic_name);

/**
 * Wrapper for a system's specific calls to name calling a thread
 * @param n name to set for calling thread
 */
void eswb_set_thread_name(const char *n);

#ifdef __cplusplus
}
#endif

#endif //ESWB_API_H
