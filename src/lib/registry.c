#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "registry.h"
#include "eswb/topic_proclaiming_tree.h"


registry_t *alloc_registry(eswb_size_t topics_num) {
    eswb_size_t alloc_size = sizeof(topic_t) * topics_num + sizeof(registry_t);

    registry_t *nr = calloc(1, alloc_size);
    // TODO issues with struct allocation in array?
    if (nr != NULL) {
        nr->max_topics = topics_num;
    }

    return nr;
}

topic_t *alloc_topic(registry_t *reg) {
    if (reg->topics_num >= reg->max_topics) {
        return NULL;
    }

    topic_t *t = &reg->topics[reg->topics_num];

    t->reg_ref = reg;
    t->id = reg->topics_num;

    reg->topics_num++;

    return t;
}

eswb_rv_t alloc_topic_data(topic_t *t) {
    if (t->data_size > 0) {
        t->data = calloc(1, t->data_size);
        if (t->data == NULL) {
            return eswb_e_mem_data_na;
        }
    }
    return eswb_e_ok;
}

eswb_rv_t topic_dealloc_resources(topic_t *t) {
    if (!(t->flags & TOPIC_FLAGS_MAPPED_TO_PARENT)) {
        if (t->array_ext != NULL) {
            free(t->array_ext);
        }
        if (t->data != NULL) {
            free(t->data);
        }
        if (!(t->flags & TOPIC_FLAGS_USES_PARENT_SYNC)) {
            if (t->sync != NULL) {
                sync_destroy(t->sync);
            }
        }
    }

    return eswb_e_ok;
}

eswb_rv_t reg_destroy(registry_t *reg) {
    for (uint32_t i = 0; i < reg->topics_num; i++) {
        topic_dealloc_resources(&reg->topics[i]);
    }
    free(reg);
    return eswb_e_ok;
}

static eswb_rv_t alloc_array_topic_data_generalized(topic_t *t, eswb_size_t elem_size, int do_align) {

    t->array_ext = calloc(1, sizeof(*t->array_ext));
    if (t->array_ext == NULL) {
        return eswb_e_mem_data_na;
    }

    if (do_align) {
        eswb_size_t dword_size = (elem_size >> 2) + ((elem_size & 0x03) ? 1 : 0);
        t->array_ext->elem_step = dword_size << 2; // align to dword;
    } else {
        t->array_ext->elem_step = elem_size;
    }

    t->array_ext->elem_size = elem_size;
    t->array_ext->len = t->data_size;

    t->array_ext->state.head = 0;
    t->array_ext->state.lap_num = 0;

    t->data = calloc(t->array_ext->len, t->array_ext->elem_step);
    if (t->data == NULL) {
        free(t->array_ext);
        return eswb_e_mem_data_na;
    }

    return eswb_e_ok;
}

eswb_rv_t alloc_fifo_topic_data(topic_t *t, eswb_size_t fifo_elem_data_size) {
    return alloc_array_topic_data_generalized(t, fifo_elem_data_size, -1);
}

eswb_rv_t alloc_buffer_data(topic_t *t) {
    return alloc_array_topic_data_generalized(t, 1, 0);
}

eswb_rv_t alloc_vector_data(topic_t *t, eswb_size_t elem_size) {
    eswb_rv_t rv = alloc_array_topic_data_generalized(t, elem_size, 0);
    if (rv == eswb_e_ok) {
        t->array_ext->curr_len = 0;
    }

    return rv;
}

void dealloc_topic(topic_t *t) {
    // TODO dealloc data if it is no parent_ind? or not? because the whole process is atomic.
    //  TODO If something fails, parent_ind is not registered too
    // if ((t->data != NULL) && (t->))
    //return free(t);
}



topic_t *reg_find_topic_among_siblings(topic_t *first_child, const char *topic_name) {

    topic_t *n;
    for (n = first_child; n != NULL; n = n->next_sibling) {
        if (strncmp(n->name, topic_name, ESWB_TOPIC_NAME_MAX_LEN) == 0) {
            return n;
        }
    }

    return NULL;
}


static topic_t *find_topic(topic_t *root, const char *find_path) {

    char *rest;
    char *topic_name;
    //int first_run = -1;
    topic_t *t;
    topic_t *child_t;

#define DELIM "/"

    char path[ESWB_TOPIC_MAX_PATH_LEN+1];

    strncpy(path, find_path, ESWB_TOPIC_MAX_PATH_LEN);
    topic_name = strtok_r(path, DELIM, &rest);
    if (strcmp(topic_name, root->name) != 0) {
        return NULL;
    }

    t = root;

    while((topic_name = strtok_r(NULL, DELIM, &rest)) != NULL) {
        child_t = t->first_child;
        t = reg_find_topic_among_siblings(child_t, topic_name);
        if (t == NULL) {
            return NULL;
        }
    }

    return t;
}


static eswb_rv_t parse_path(char *path, eswb_size_t max_lev, char *ptrs[]) {

    char *rest;
    uint32_t i = 0;

#define DELIM "/"
    ptrs[i++] = strtok_r(path, DELIM, &rest);
    while((ptrs[i++] = strtok_r(NULL, DELIM, &rest)) != NULL) {
        if (i >= max_lev) {
            return eswb_e_mem_static_exceeded;
        }
    }
    ptrs[i] = NULL;
    return eswb_e_ok;
}



int match_str(const char *name, const char *mask) {
    char *wc = strchr(mask, '*');
    int has_wildcard = wc != NULL;
    if (has_wildcard) {
        // TODO what if several wildcards?

        // TODO what if we have expression like this: abc*xyz ?
        if (strlen(mask) == 1) {
            if (name[0] == '.') {
                return 0; // TODO do not wildcarding hidden names?
            } else {
                return 1; // we have just wildcard
            }
        }
        char mask_str[ESWB_TOPIC_NAME_MAX_LEN+1];
        strncpy(mask_str, mask, ESWB_TOPIC_NAME_MAX_LEN);
        mask_str[wc - mask] = 0; // char '*' must me exactly here

        char *r = strstr(name, mask);
        if (r == name) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (strcmp(name, mask) == 0) {
            return 1;
        } else {
            return 0;
        }
    }
}

#define WALK_RV_TERMINAL (-1)
#define WALK_RV_NO_NESTED_TOPIC (-2)

static int walk_through_tree(topic_t *t, char *dir_ptrs[], crawling_lambda_t lambda, void *usr_l_data) {
    int i = 0;

    if (t == NULL) {
        return WALK_RV_NO_NESTED_TOPIC;
    }

    if (dir_ptrs[0] == NULL) {
        return WALK_RV_TERMINAL;
    }

    for (topic_t *n = t; n != NULL; n = n->next_sibling) {
        if (match_str(n->name, dir_ptrs[0])) {
            int wt_rv = walk_through_tree(n->first_child, &dir_ptrs[1], lambda, usr_l_data);
            switch (wt_rv) {
                case WALK_RV_NO_NESTED_TOPIC:
                case WALK_RV_TERMINAL:
                    (*lambda)(usr_l_data, n);
                    i++;
                    break;

                default:
                    i+= wt_rv;
            }
        }
    }

    return i;
}

eswb_rv_t topic_mem_walk_through (topic_t *root, const char *find_path, crawling_lambda_t lambda, void *usr_data) {
#define MAX_LEVELS 10
    char *dir_ptrs[MAX_LEVELS + 1];

    memset(dir_ptrs, 0, sizeof(dir_ptrs));

    const char *find_path_start = strstr(find_path, ":/");
    if (find_path_start == NULL) {
        find_path_start = find_path;
    } else {
        find_path_start += 2;
    }

    char path[ESWB_TOPIC_MAX_PATH_LEN+1];
    strncpy(path, find_path_start, ESWB_TOPIC_MAX_PATH_LEN);

    eswb_rv_t rv = parse_path(path, MAX_LEVELS, dir_ptrs);
    if (rv != eswb_e_ok) {
        return rv;
    }


//    if (strcmp(dir_ptrs[0], root->name) != 0) {
//        return eswb_e_no_topic;
//    }

    int wt_rv = walk_through_tree(root, &dir_ptrs[0], lambda, usr_data);

    return wt_rv > 0 ? eswb_e_ok : eswb_e_no_topic;
}

static eswb_rv_t fill_in_topic(topic_t *t, topic_proclaiming_tree_t *tsrc) {
    if ((strlen(tsrc->name) == 0) || strlen(tsrc->name) > ESWB_TOPIC_NAME_MAX_LEN) {
        return eswb_e_invargs;
    }

    strncpy(t->name, tsrc->name, ESWB_TOPIC_NAME_MAX_LEN);
    t->type = tsrc->type;
    t->data_size = tsrc->data_size;

    return eswb_e_ok;
}


static eswb_rv_t topic_add_child(topic_t *parent, topic_proclaiming_tree_t *topic_struct, topic_t **rv_tpc,
                          int synced) {

    topic_t *new = alloc_topic(parent->reg_ref);

    if (new == NULL) {
        return eswb_e_mem_topic_na;
    }

    topic_struct->topic_id = new->id;

    // establish topic's resources
    eswb_rv_t rv = fill_in_topic(new, topic_struct);
    if (rv != eswb_e_ok) {
        // TODO Dealloc topic
        return rv;
    }

    if (topic_struct->flags & TOPIC_PROCLAIMING_FLAG_MAPPED_TO_PARENT) {
        new->sync = parent->sync;
        if ((parent->type == tt_fifo) || (parent->type == tt_event_queue)) {
            new->data = parent->data;
        } else {
            new->data = parent->data + topic_struct->data_offset;
        }
        new->array_ext = parent->array_ext; // don't care if it is null
        new->flags |= TOPIC_FLAGS_MAPPED_TO_PARENT;
    } else {
        do {
            // topics might be added created only inside directories
            switch (parent->type) {
                case tt_dir:
                case tt_fifo:
                case tt_event_queue:
                    break;

                default:
                    rv = eswb_e_notdir;
                    break;
            }

            if (rv != eswb_e_ok) {
                break;
            }

            switch (new->type) {
                default:
                    rv = alloc_topic_data(new);
                    break;

                case tt_fifo:
                    if ((topic_struct->first_child_ind == PR_TREE_NO_REF_IND) || // fifo must have a child
                        (topic_struct[topic_struct->first_child_ind].next_sibling_ind != PR_TREE_NO_REF_IND)) { // and the only child
                        // TODO children cannot be fifo-s
                        rv = eswb_e_invargs;
                        break;
                    }

                    // fall through ...
                case tt_event_queue:
                    rv = alloc_fifo_topic_data(new, topic_struct[topic_struct->first_child_ind].data_size);
                    break;

                case tt_byte_buffer:
                    rv = alloc_buffer_data(new);
                    break;

                case tt_vector:
                    rv = alloc_vector_data(new, topic_struct[topic_struct->first_child_ind].data_size);
                    break;
            }

            if (rv != eswb_e_ok) {
                break;
            }

            if (topic_struct->flags & TOPIC_PROCLAIMING_FLAG_USES_PARENT_SYNC) {
                new->sync = parent->sync;
                new->flags |= TOPIC_FLAGS_USES_PARENT_SYNC;
            } else {
                if (synced) {
                    rv = sync_create(&new->sync);
                    if (rv != eswb_e_ok) {
                        break;
                    }
                } else {
                    new->sync = NULL;
                }
            }
        } while(0);

        if (rv != eswb_e_ok) {
            dealloc_topic(new);
            return rv;
        }
    }

    // link inside registry
    new->parent = parent;
    // inheriting event queue mask
    new->evq_mask = parent->evq_mask;


    if (parent->first_child == NULL) {
        parent->first_child = new;
    } else {
        topic_t *n;
        for (n = parent->first_child; n->next_sibling != NULL; n = n->next_sibling);
        n->next_sibling = new;
    }

    *rv_tpc = new;

    return eswb_e_ok;
}

static eswb_rv_t topics_tree_register(topic_t *mount_point, topic_proclaiming_tree_t *new_topic_struct,
                               int synced) {
    // TODO make NO recursion;

    // go over new_topic structure
    // create child
    // add accordingly
    topic_proclaiming_tree_t *n;
    topic_t *new;

    //printf("%s. %s\n", __func__, new_topic_struct->name);

    eswb_rv_t  rv;
    rv = topic_add_child(mount_point, new_topic_struct, &new, synced);

    if (rv != eswb_e_ok) {
        return rv;
    }

    #   define INDEX2PTR(__array,__ind) \
    (((__ind) != PR_TREE_NO_REF_IND) ? &(__array)[__ind] : NULL)

    for (n = INDEX2PTR(new_topic_struct, new_topic_struct->first_child_ind);
            n != NULL;
                n = INDEX2PTR(n, n->next_sibling_ind)) {
        rv = topics_tree_register(new, n, synced);
        if (rv != eswb_e_ok) {
            return rv;
        }
    }

    return eswb_e_ok;
}

eswb_rv_t reg_tree_register(registry_t *reg, topic_t *mounting_topic, topic_proclaiming_tree_t *new_topic_struct) {
    int synced = REG_SYNCED(reg);
    if (synced) sync_take(reg->sync);
    eswb_rv_t rv = topics_tree_register(mounting_topic, new_topic_struct, synced);
    if (synced) sync_give(reg->sync);

    return rv;
}


topic_t *reg_find_topic(registry_t *reg, const char *path) {

    if (REG_SYNCED(reg)) sync_take(reg->sync);
    topic_t *rv = find_topic(&reg->topics[0], path);
    if (REG_SYNCED(reg)) sync_give(reg->sync);

    return rv;
}


eswb_rv_t reg_create(const char *root_name, registry_t **new_reg_rv, eswb_size_t max_topics, int synced) {
    registry_t *new_reg = alloc_registry(max_topics);

    if (new_reg == NULL) {
        return eswb_e_mem_reg_na;
    }

    if (strlen(root_name) > ESWB_TOPIC_NAME_MAX_LEN ) {
        return eswb_e_invargs;
    }

    eswb_rv_t rv;


    topic_t *root = alloc_topic(new_reg);
    if (root == NULL) {
        return eswb_e_mem_topic_na;
    }

    strncpy(root->name, root_name, ESWB_TOPIC_NAME_MAX_LEN);
    root->type = tt_dir;

    if (synced) {
        rv = sync_create(&root->sync);
        if (rv != eswb_e_ok) {
            return rv;
        }

        rv = sync_create(&new_reg->sync);
        if (rv != eswb_e_ok) {
            return rv;
        }
    } else {
        root->sync = NULL;
        new_reg->sync = NULL;
    }

    *new_reg_rv = new_reg;
    return eswb_e_ok;
}

void print_topic_oneline(topic_t *t, int level, int last) {
    int i, n = level * 4;


    for (i = 0; i < n; i++) {
        printf(" ");
    }

    printf("%s\n", t->name);
    //printf("%s id == %d\n", t->name, t->id);
}


topic_t *topic_tree_next(topic_t *r) {

    if (r->first_child != NULL) {
        return r->first_child;
    } else if (r->next_sibling != NULL) {
        return r->next_sibling;
    } else {
        topic_t *n;
        for(n = r->parent; n != NULL; n = n->parent) {
            if (n->next_sibling != NULL) {
                return n->next_sibling;
            }
        }
    }

    return NULL;
}

topic_t *topic_tree_next_filtered(topic_t *r) {
    int loop;
    do {
        r = topic_tree_next(r);
        if (r != NULL) {
            loop = (r->type == tt_event_queue)
                        || ((r->parent != NULL) && (r->parent->type == tt_event_queue));
        } else {
            loop = 0;
        }
    } while (loop);

    return r;
}

int parent_has_child(topic_t *parent, topic_t *child) {
    for (topic_t *n = child; n != NULL; n = n->parent) {
        if (parent == n->parent) {
            return -1;
        }
    }
    return 0;
}

eswb_rv_t
reg_get_next_topic_info(registry_t *reg, topic_t *parent, eswb_topic_id_t id, topic_extract_t *extract) {
    eswb_rv_t rv = eswb_e_ok;

    int synced = REG_SYNCED(reg);

    if (synced) sync_take(reg->sync);

    do {
        topic_t *t = &reg->topics[id];
        if (id != t->id) {
            rv = eswb_e_invargs;
            break;
        }

        t = topic_tree_next_filtered(t);
        if (t == NULL || parent_has_child(parent, t) == 0) {
            rv = eswb_e_no_topic;
            break;
        }

        strncpy(extract->info.name, t->name, PR_TREE_NAME_ALIGNED - 1);
        extract->parent_id = t->parent != NULL ? t->parent->id : 0;
        extract->info.type = t->type;
        extract->info.data_size = t->data_size;
        extract->info.data_offset = t->flags & TOPIC_FLAGS_MAPPED_TO_PARENT ? t->data - t->parent->data : 0;
        extract->info.flags = t->flags;
        extract->info.topic_id = t->id;
        extract->info.abs_ind = 0;
        extract->info.parent_ind = PR_TREE_NO_REF_IND;
        extract->info.next_sibling_ind = PR_TREE_NO_REF_IND;
        extract->info.first_child_ind = PR_TREE_NO_REF_IND;
    } while (0);

    if (synced) sync_give(reg->sync);

    return rv;
}

void topic_print_tree(topic_t *t, int level, int process_siblings) {
    if (t == NULL) {
        return;
    }

    print_topic_oneline(t, level, t->next_sibling == NULL ? -1 : 0 );

    topic_print_tree(t->first_child, level + 1, 1);

    if (process_siblings) {
        for (topic_t *n = t->next_sibling; n != NULL; n = n->next_sibling) {
            topic_print_tree(n, level, 0);
        }
    }
}

void reg_print(registry_t *reg) {
    //printf("%c\n", 14);

    topic_print_tree(&reg->topics[0], 0, 1);

    //printf("%c\n", 15);
}
