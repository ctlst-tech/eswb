#ifndef ESWB_TOPIC_PROCLAIMING_TREE_H
#define ESWB_TOPIC_PROCLAIMING_TREE_H

/** @page proclaiming
 * Check topic_proclaiming_tree.h for calls descriptions
 */

/** @file */


#include <stdint.h>
#include <stddef.h>

#include "eswb/errors.h"
#include "eswb/types.h"


#define TOPIC_FLAG_MAPPED_TO_PARENT (1UL << 0UL)
#define TOPIC_FLAG_USES_PARENT_SYNC    (1UL << 1UL)
//#define TOPIC_USER_PARENT_IS_FIFO (1UL << 0UL)

#define PR_TREE_NAME (ESWB_TOPIC_NAME_MAX_LEN+1)

#define PR_TREE_NO_REF_IND      (-1024)

#define PR_TREE_NAME_ALIGNED (PR_TREE_NAME + (PR_TREE_NAME % 4))

typedef struct __attribute__((packed)) {
    char name[PR_TREE_NAME_ALIGNED]; // should give nice padding

    int32_t            abs_ind;

    topic_data_type_s_t  type;
    uint16_t           data_offset;
    uint16_t           data_size;

    uint16_t           flags;

    uint16_t           topic_id; // assigned at first proclaiming

    int16_t            parent_ind; // also used as parent Topic ID in
    int16_t            first_child_ind;
    int16_t            next_sibling_ind;
} topic_proclaiming_tree_t;

typedef struct {
    topic_proclaiming_tree_t info;
    eswb_topic_id_t          parent_id;
} topic_extract_t;


typedef struct {
    eswb_index_t t_num;
    eswb_index_t t_max;

    topic_proclaiming_tree_t topics_pool[0];
} topic_tree_context_t;

#ifdef __cplusplus
extern "C" {
#endif

#define TOPIC_TREE_CONTEXT_LOCAL_MEMSET(__name) memset(&__name##data_pool, 0, sizeof(__name##data_pool));

#define TOPIC_TREE_CONTEXT_LOCAL_DEFINE(_name,_topics_num)   \
        uint8_t _name##data_pool [sizeof(topic_tree_context_t) + (_topics_num) * sizeof(topic_proclaiming_tree_t)]; \
        topic_tree_context_t *(_name) = (topic_tree_context_t *)&_name##data_pool; \
        TOPIC_TREE_CONTEXT_LOCAL_MEMSET(_name);                                \
        (_name)->t_max = _topics_num;                             \

#define TOPIC_TREE_CONTEXT_LOCAL_RESET(__name) __name->t_num = 0; memset(&(__name)->topics_pool[0], 0, sizeof(topic_proclaiming_tree_t));

topic_proclaiming_tree_t *usr_topic_set_root(topic_tree_context_t *context, const char *name,
                                             topic_data_type_t type, size_t data_size);

topic_proclaiming_tree_t *usr_topic_add_child(topic_tree_context_t *context, topic_proclaiming_tree_t *root, const char *name,
                                              topic_data_type_t type, uint32_t data_offset, size_t data_size, uint32_t flags);

#define usr_topic_set_struct(__cntx,__struct_val,__topic_name) \
                            usr_topic_set_root(__cntx,        \
                            __topic_name, \
                            tt_struct,                        \
                            sizeof(__struct_val))

#define usr_topic_set_fifo(__cntx,__topic_name,__fifo_size) \
                            usr_topic_set_root(__cntx,        \
                            __topic_name, \
                            tt_fifo,                        \
                            __fifo_size)

#define OFFSETOF(__type, __member) ((size_t)&(((__type *)(void*)0)->__member) )
#define SIZEOFMEMBER(__type, __member) (sizeof(((__type *)(void*)0)->__member))

#define usr_topic_add_struct_child(__cntx,__root,__struct_type,__struct_var,__topic_name,__type) \
                            usr_topic_add_child(__cntx, __root,       \
                            __topic_name, \
                            __type,                                   \
                            OFFSETOF(__struct_type, __struct_var),                                    \
                            SIZEOFMEMBER(__struct_type, __struct_var),                                \
                            TOPIC_FLAG_MAPPED_TO_PARENT)

void print_topics_tree(topic_proclaiming_tree_t *current);

#ifdef __cplusplus
}
#endif

#endif //ESWB_TOPIC_PROCLAIMING_TREE_H
