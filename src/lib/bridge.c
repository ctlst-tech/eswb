#include <stdlib.h>
#include <string.h>

#include "eswb/bridge.h"
#include "eswb/api.h"


static eswb_bridge_t* alloc_bridge(eswb_size_t max_tds) {
    eswb_bridge_t *rv;
    eswb_size_t alloc_size = sizeof(eswb_bridge_t) + max_tds * sizeof(struct topics_subsriptions);
    
    rv = calloc(1, alloc_size);
    if (rv != NULL) {
        rv->max_tds = max_tds;
    }
    return rv;
}

void* alloc_buffer(eswb_size_t s) {
    return malloc(s);
}


eswb_rv_t eswb_bridge_create(const char *name, eswb_size_t max_tds, eswb_bridge_t **rv) { //eswb_bridge_type_t t,
    
    eswb_bridge_t *br;

    br = alloc_bridge(max_tds);
    if (br == NULL) {
        return eswb_e_mem_data_na;
    }

    strncpy(br->name, name, BRIDGE_NAME_MAX);

    *rv = br;
    
    return eswb_e_ok;
}


eswb_rv_t
eswb_bridge_add_topic(eswb_bridge_t *b, eswb_topic_descr_t mnt_td, const char *src_path, const char *dest_name) {
    if (b->buffer2post != NULL) {
        return eswb_e_topic_exist; // TODO change error code;
    }

    if (b->tds_num >= b->max_tds) {
        return eswb_e_mem_topic_na; // TODO change it?
    }

    eswb_rv_t rv;
    if (mnt_td == 0) {
        rv = eswb_connect(src_path, &b->topics[b->tds_num].td);
    } else {
        rv = eswb_connect_nested(mnt_td, src_path, &b->topics[b->tds_num].td);
    }
    if (rv == eswb_e_ok) {
        topic_params_t tp;
        eswb_get_topic_params(b->topics[b->tds_num].td, &tp);
        b->topics[b->tds_num].size = tp.size;
        b->topics[b->tds_num].type = tp.type;

        b->buffer2post_size += tp.size;

        if (dest_name == NULL) {
            dest_name = tp.name;
        }

        strncpy(b->topics[b->tds_num].dest_name, dest_name, ESWB_TOPIC_NAME_MAX_LEN);

        b->tds_num++;
    }

    return rv;
}

static eswb_rv_t calc_nested_topics(eswb_topic_descr_t td, uint32_t *num_rv) {
    eswb_topic_id_t next2tid = 0;
    eswb_rv_t rv;
    uint32_t t_num = 0;

    do {
        rv = eswb_get_next_topic_info(td, &next2tid, NULL);
        if (rv == eswb_e_ok) {
            t_num++;
        }
    } while(rv == eswb_e_ok);

    if (rv == eswb_e_no_topic) {
        *num_rv = t_num;
        return eswb_e_ok;
    } else {
        return rv;
    }
}

static eswb_rv_t calc_bridge_topics(eswb_bridge_t *b, uint32_t *num_rv) {
    uint32_t n = 0;
    uint32_t num_nested;
    eswb_rv_t rv;

    for (int i = 0; i < b->tds_num; i++) {
        struct topics_subsriptions *ts = &b->topics[i];
        n++;
        if (ts->type == tt_struct) {
            rv = calc_nested_topics(ts->td, &num_nested);
            if (rv == eswb_e_ok) {
                n += num_nested;
            } else {
                return rv;
            }
        }
    }

    *num_rv = n;

    return eswb_e_ok;
}

static eswb_rv_t add_nested_children_to_proclaiming_array(eswb_topic_descr_t from_td, topic_tree_context_t *to_cntx, topic_proclaiming_tree_t *to_root) {
    eswb_topic_id_t next2tid = 0;
    eswb_rv_t rv;
    struct topic_extract topic_info;

    do {
        rv = eswb_get_next_topic_info(from_td, &next2tid, &topic_info);
        if (rv == eswb_e_ok) {
            topic_proclaiming_tree_t *topic = usr_topic_add_child(to_cntx, to_root,
                                            topic_info.info.name,
                                            topic_info.info.type,
                                            topic_info.info.data_offset,
                                            topic_info.info.data_size, TOPIC_FLAG_MAPPED_TO_PARENT);
            if (topic == NULL) {
                return eswb_e_invargs;
            }
        }
    } while(rv == eswb_e_ok);

    return rv == eswb_e_no_topic ? eswb_e_ok : rv;
}

eswb_rv_t eswb_bridge_connect(eswb_bridge_t *b, eswb_topic_descr_t mtd_td, const char *dest_mnt) {
    uint32_t i;

    b->buffer2post = alloc_buffer(b->buffer2post_size);

    topic_proclaiming_tree_t *root;

    char topic_name[ESWB_TOPIC_NAME_MAX_LEN + 1];
    char topic_path[ESWB_TOPIC_MAX_PATH_LEN + 1];
    eswb_rv_t rv;

    uint32_t topics_num = 0;
    rv = calc_bridge_topics(b, &topics_num);
    if (rv != eswb_e_ok) {
        return rv;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, topics_num + 1);

    if (b->tds_num > 1) {
        // several topics in bridge will deliver structure
        root = usr_topic_set_root(cntx, b->name, tt_struct, b->buffer2post_size);

        eswb_size_t offset = 0;

        // prepare topic struct
        for (i = 0; i < b->tds_num; i++) {
            struct topics_subsriptions *ts = &b->topics[i];

            topic_proclaiming_tree_t *sub_topic;

            sub_topic = usr_topic_add_child(cntx, root,
                                ts->dest_name,
                                ts->type, offset, ts->size, TOPIC_FLAG_MAPPED_TO_PARENT);
            if (ts->type == tt_struct) {
                rv = add_nested_children_to_proclaiming_array(ts->td, cntx, sub_topic);
                if (rv != eswb_e_ok) {
                    return rv;
                }
            }

            offset += ts->size;
        }
    } else {

        if (mtd_td == 0) {
            // for vector path targets to mounting point, for scalar to exact path and name of the topic
            rv = eswb_path_split(dest_mnt, topic_path, topic_name);
            if (rv != eswb_e_ok) {
                return rv;
            }
            dest_mnt = topic_path;
        }

        root = usr_topic_set_root(cntx, mtd_td == 0 ? topic_name : b->topics[0].dest_name,
                                                          b->topics[0].type, b->topics[0].size);
        if (b->topics[0].type == tt_struct) {
            rv = add_nested_children_to_proclaiming_array(b->topics[0].td, cntx, root);
            if (rv != eswb_e_ok) {
                return rv;
            }
        }
    }

    if (mtd_td == 0) {
        rv = eswb_proclaim_tree_by_path(dest_mnt, root, cntx->t_num, &b->dest_td);
    } else {
        rv = eswb_proclaim_tree(mtd_td, root, cntx->t_num, &b->dest_td);
    }

    return rv;
}


eswb_rv_t eswb_bridge_read(eswb_bridge_t *b, void *data) {
    eswb_size_t offset = 0;
    for (uint32_t i = 0; i < b->tds_num; i++) {
        eswb_read(b->topics[i].td, data + offset);
        offset += b->topics[i].size;
    }

    return eswb_e_ok;
}

eswb_rv_t eswb_bridge_update(eswb_bridge_t *b) {

    if (b == NULL) {
        return eswb_e_invargs;
    }

    eswb_bridge_read(b, b->buffer2post);

    return eswb_update_topic(b->dest_td, b->buffer2post);
}
