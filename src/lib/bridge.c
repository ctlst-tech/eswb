//
// Created by Ivan Makarov on 5/12/21.
//

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
    /*
     * 1. crete and init eswb_bridge_t instance
     * 2. define bridge behaviour
     */
    
    *rv = br;
    
    return eswb_e_ok;
}


eswb_rv_t
eswb_bridge_add_topic(eswb_bridge_t *b, eswb_topic_descr_t mnt_td, const char *src_path, const char *dest_name) {
    /*
     * 1. find_topic to a topic, get descr
     */

    if (b->buffer2post != NULL) {
        // TODO acceptable behaviour if we post topics as structure
        return eswb_e_topic_exist; // TODO change error code;
    }

    if (b->tds_num >= b->max_tds) {
        return eswb_e_mem_topic_na; // TODO change it?
    }

    eswb_rv_t rv;
    if (mnt_td == 0) {
        rv = eswb_subscribe(src_path, &b->topics[b->tds_num].td);
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

eswb_rv_t eswb_bridge_connect_vector(eswb_bridge_t *b, const char *dest_mnt) {

    // TODO chose policy: replicate topics as separete topics, or post them as signle structure

    // TODO if there is no structure at the dst - then post w/o structure?

    int i;

    b->buffer2post = alloc_buffer(b->buffer2post_size);

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, b->tds_num + 1);

    topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, b->name, tt_struct, b->buffer2post_size);
            //usr_topic_set_struct(cntx, b->buffer2post, b->name);

    eswb_size_t offset = 0;

    // prepare topic struct
    for (i = 0; i < b->tds_num; i++) {
        struct topics_subsriptions *ts = &b->topics[i];

        usr_topic_add_child(cntx, rt,
                            ts->dest_name,
                            ts->type, offset, ts->size, TOPIC_FLAG_MAPPED_TO_PARENT);

        offset += ts->size;
    }

    eswb_rv_t rv = eswb_proclaim_tree_by_path(dest_mnt, rt, i + 1, &b->dest_td);

    if (rv != eswb_e_ok) {
        // TODO free b->buffer2post
    }

    return rv;
}

eswb_rv_t eswb_bridge_connect_scalar(eswb_bridge_t *b, eswb_topic_descr_t mtd_td, const char *dest_topic) {

    if ((b->tds_num > 1) || (b->tds_num < 1)) {
        return eswb_e_invargs;
    }

    eswb_rv_t rv;


    b->buffer2post = alloc_buffer(b->topics[0].size);
    if (b->buffer2post == NULL) {
        return eswb_e_map_no_mem;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, b->tds_num + 1);

    char topic_name[ESWB_TOPIC_NAME_MAX_LEN + 1];
    char topic_path[ESWB_TOPIC_MAX_PATH_LEN + 1];


    if (mtd_td == 0) {
        rv = eswb_path_split(dest_topic, topic_path, topic_name);
        if (rv != eswb_e_ok) {
            return rv;
        }

        topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, topic_name, b->topics[0].type, b->topics[0].size);
        rv = eswb_proclaim_tree_by_path(topic_path, rt, cntx->t_num, &b->dest_td);
    } else {
        topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, dest_topic, b->topics[0].type, b->topics[0].size);
        rv = eswb_proclaim_tree(mtd_td, rt, cntx->t_num, &b->dest_td);
    }

    return rv;
}

eswb_rv_t eswb_bridge_read(eswb_bridge_t *b, void *data) {
    eswb_size_t offset = 0;
    for (int i = 0; i < b->tds_num; i++) {
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



/* TODO
 * 1. static bridges - called inside a designated thread on thread clocking rule
 *    dymanic bridges - are services with a separete thread likely attached to a relication services
 *
 * 2. mark topic's fifo ID inside a subscription
 * 3. push bulk data in that fifo
 * 4. receiver has routing ability to read from a single bridge
 * 5. receiver checks correpsondance with expected table and then unblocks if it got a desired packet
 */
