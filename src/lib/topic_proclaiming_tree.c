#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "eswb/topic_proclaiming_tree.h"
#include "eswb/errors.h"

static eswb_rv_t check_topic_naming(const char* name) {
    char nn[ESWB_TOPIC_NAME_MAX_LEN];

    int rv = sscanf(name, "%[a-zA-Z0-9_-.]", nn);

    if (rv < 1) {
        return eswb_e_inv_naming;
    }

    return (strncmp (name, nn, ESWB_TOPIC_NAME_MAX_LEN) == 0) ? eswb_e_ok : eswb_e_inv_naming;
}


eswb_rv_t usr_topic_setup(topic_proclaiming_tree_t *topic, const char *name,
                          topic_data_type_t type, uint32_t data_offset, size_t data_size, uint32_t flags) {

    eswb_rv_t rv;
    if ((rv = check_topic_naming(name)) != eswb_e_ok) {
        return rv;
    }

    memset(topic, 0, sizeof(*topic));
    strncpy(topic->name, name, ESWB_TOPIC_NAME_MAX_LEN);
    topic->data_size = data_size;
    topic->data_offset = data_offset;
    topic->flags = flags;
    topic->type = type;

    topic->parent_ind = PR_TREE_NO_REF_IND;
    topic->first_child_ind = PR_TREE_NO_REF_IND;
    topic->next_sibling_ind = PR_TREE_NO_REF_IND;

    return eswb_e_ok;
}



eswb_rv_t usr_topic_in_context_setup(topic_tree_context_t *topic, const char *name, topic_proclaiming_tree_t *parent, topic_proclaiming_tree_t *prev_sibling) {

    return eswb_e_not_supported;
}


topic_proclaiming_tree_t *usr_topic_set_root(topic_tree_context_t *context, const char *name, topic_data_type_t type, size_t data_size) {

    eswb_rv_t rv = usr_topic_setup(&context->topics_pool[0], name, type, 0, data_size, 0);
    //context->last_parent = context->last_added = &context->root;
    if (rv != eswb_e_ok) {
        return NULL;
    }

    context->topics_pool[0].abs_ind = 0;

    context->t_num++; // TODO maybe 0 ?

    return &context->topics_pool[0];
}

topic_proclaiming_tree_t *usr_topic_add_child(topic_tree_context_t *context, topic_proclaiming_tree_t *root, const char *name,
                                              topic_data_type_t type, uint32_t data_offset, size_t data_size, uint32_t flags) {

    if (context->t_num >= context->t_max) {
        //TODO return eswb_e_mem_topic_na;
        return NULL;
    }

//    if (context->last_added == NULL) {
//        return eswb_e_noroot;
//    }
//
    topic_proclaiming_tree_t  *added_topic = &context->topics_pool[context->t_num];
    eswb_rv_t rv = usr_topic_setup(added_topic, name, type, data_offset, data_size, flags);
    if (rv != eswb_e_ok) {
        //TODO return rv;
        return NULL;
    }

    added_topic->abs_ind = (int32_t) context->t_num;

    //added_topic->parent_ind = context->last_parent;
    //context->last_added = added_topic;

    if (root->first_child_ind == PR_TREE_NO_REF_IND) {
        root->first_child_ind = added_topic->abs_ind - root->abs_ind;
    } else {
        topic_proclaiming_tree_t *iterator;
        for (iterator = &root[root->first_child_ind];
                iterator->next_sibling_ind != PR_TREE_NO_REF_IND;
                    iterator = &iterator[iterator->next_sibling_ind]);
        iterator->next_sibling_ind = added_topic->abs_ind - iterator->abs_ind;
    }
    added_topic->parent_ind = root->abs_ind - added_topic->abs_ind;

    context->t_num++;

    return added_topic;
}

topic_proclaiming_tree_t *usr_topic_set_vector(topic_tree_context_t *context, const char *name, size_t vector_max_len, topic_data_type_t elem_type, eswb_size_t elem_size) {
    topic_proclaiming_tree_t *vector_root = usr_topic_set_root(context, name, tt_vector, vector_max_len);
    if (vector_root == NULL) {
        return NULL;
    }

    topic_proclaiming_tree_t *ch = usr_topic_add_child(context, vector_root, "_", elem_type, 0, elem_size, TOPIC_PROCLAIMING_FLAG_MAPPED_TO_PARENT);
    if (ch == NULL) {
        return NULL;
    }

    return vector_root;
}


/*
eswb_rv_t usr_topic_add_child_to_last(topic_tree_context_t *context, const char *name) {
    //topic_structure_t *last_added;
    eswb_rv_t rv = att_topic_to_pool(context, name);
    if (rv != eswb_e_ok) {
        return rv;
    }

    context->last_added->first_child_ind = context->last_added;

    return rv;
}

eswb_rv_t usr_topic_add_sibling_to_last(topic_tree_context_t *context, const char *name) {
    topic_structure_t *last_added;
    eswb_rv_t rv = att_topic_to_pool(context, name, &last_added);
    if (rv != eswb_e_ok) {
        return rv;
    }

    if (context->last_sibling != NULL) {
        context->last_sibling->next_sibling_ind = last_added;
    }
    context->last_sibling = last_added;

    return rv;
}
*/



const char *eswb_type_name(topic_data_type_t t) {
    switch (t) {
        case tt_float: return "float";
        case tt_double: return "double";
        case tt_uint32: return "uint32";
        case tt_int32: return "int32";
        case tt_uint16: return "uint16";
        case tt_int16: return "int16";
        case tt_uint8: return "uint8";
        case tt_int8: return "int8";
        case tt_string: return "string";
        case tt_struct: return "struct";
        case tt_fifo: return "fifo";
        case tt_dir: return "dir";
        case tt_vector: return "vector";
        default: return "unknown";
    }
}

#include "misc.h"

void print_topic_params(topic_proclaiming_tree_t *t, int depth) {
    printf("%s", indent(depth)); printf (" Topic params: Type=%s data_size=%d\n", eswb_type_name(t->type), t->data_size);
}

static char *render_path (topic_proclaiming_tree_t *t) {
    static char _path[512];
    static char _rv[1024];
    _path[0] = 0;
    for ( ; t != NULL; t = &t[t->parent_ind]) {
        snprintf(_rv,sizeof(_rv), "%s%s%s", t->name, (_path[0] == 0) ? "" : "/", _path);
        strcpy(_path, _rv);
    }
    return _rv;
}


void print_topic_struct(topic_proclaiming_tree_t *t, int depth) {
    printf("%s\n", render_path(t));
    print_topic_params(t, depth);
}
//
//void print_topics_tree(topic_proclaiming_tree_t *current) {
//    topic_proclaiming_tree_t *t;
//    int depth = 0;
//
//    while (current) {
//        print_topic_struct(current, depth);
//        if (current->first_child_ind) {
//            current = current->first_child_ind;
//            depth++;
//        } else if (current->next_sibling_ind) {
//            current = current->next_sibling_ind;
//        } else if (current->parent_ind) {
//            // we have visited parent_ind already, so ...
//            if (current->parent_ind->next_sibling_ind) {
//                // ... someone next to him
//                current = current->parent_ind->next_sibling_ind;
//                depth--;
//            } else {
//                current = NULL;
//            }
//        } else {
//            current = NULL;
//        }
//    }
//}
