//
// Created by Ivan Makarov on 7/12/21.
//

#ifndef ESWB_BRIDGE_H
#define ESWB_BRIDGE_H

#include "types.h"
#include "errors.h"

#define BRIDGE_NAME_MAX 30

typedef struct {
    char name[BRIDGE_NAME_MAX + 1];
    eswb_size_t max_tds;
    eswb_index_t tds_num;
    void *buffer2post;
    eswb_size_t buffer2post_size;

    eswb_topic_descr_t dest_td;
    struct topics_subsriptions {
        eswb_topic_descr_t td;
        topic_data_type_t type;
        eswb_size_t size;
        char dest_name[ESWB_TOPIC_NAME_MAX_LEN + 1];
    } topics[0];

} eswb_bridge_t;

eswb_rv_t eswb_bridge_create(const char *name, eswb_size_t max_tds, eswb_bridge_t **rv);
/**
 *
 * @param b
 * @param src_path
 * @param dest_name - must be unique existing pointer to a name,
 * till the call of eswb_bridge_connect_vector (for a sake of bridge struct compactness). If it is a NULL, topic name will be used insted
 * TODO  ^^^ this is really lame ^^^
 * @return
 */
eswb_rv_t
eswb_bridge_add_topic(eswb_bridge_t *b, eswb_topic_descr_t mnt_td, const char *src_path, const char *dest_name);
eswb_rv_t eswb_bridge_connect_vector(eswb_bridge_t *b, const char *dest_mnt);
eswb_rv_t eswb_bridge_connect_scalar(eswb_bridge_t *b, eswb_topic_descr_t mtd_td, const char *dest_topic);
eswb_rv_t eswb_bridge_read(eswb_bridge_t *b, void *data);
eswb_rv_t eswb_bridge_update(eswb_bridge_t *b);

#endif //ESWB_BRIDGE_H
