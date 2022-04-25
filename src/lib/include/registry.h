//
// Created by Ivan Makarov on 28/10/21.
//

#ifndef ESWB_REGISTRY_H
#define ESWB_REGISTRY_H

#include "sync.h"
#include "topic_mem.h"
#include "eswb/topic_proclaiming_tree.h"

typedef struct registry {

    struct sync_handle* sync;
    eswb_size_t         max_topics;
    eswb_index_t        topics_num;
    topic_t             topics[0];

} registry_t;



eswb_rv_t reg_create(const char *root_name, registry_t **new_reg, eswb_size_t max_topics, int synced);
eswb_rv_t reg_destroy(registry_t *reg);

eswb_rv_t reg_tree_register(registry_t *reg, topic_t *mounting_topic, topic_proclaiming_tree_t *new_topic_struct, int synced);
topic_t *reg_find_topic(registry_t *reg, const char *path, int synced);
eswb_rv_t reg_get_next_topic_info(registry_t *reg, eswb_topic_id_t id, topic_extract_t *extract, int synced);

void reg_print(registry_t *reg);

#endif //ESWB_REGISTRY_H
