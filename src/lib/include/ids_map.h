#ifndef ESWB_IDS_MAP_H
#define ESWB_IDS_MAP_H

#include "eswb/types.h"

typedef struct {
    uint32_t src_topic_id;
    eswb_topic_descr_t dst_td;
} map_record_t;

typedef struct topic_id_map {
    eswb_size_t size;
    eswb_size_t records_num;
    map_record_t *map;
} topic_id_map_t;


#ifdef __cplusplus
extern "C" {
#endif

eswb_rv_t map_alloc(topic_id_map_t **map_handle, eswb_size_t map_max_size);
eswb_rv_t map_find_index(topic_id_map_t *map_handle, uint32_t src_id_key, uint32_t *mid_rv);

eswb_rv_t map_add_pair(topic_id_map_t *map_handle, uint32_t src_id, eswb_topic_descr_t dst_td);
void map_dealloc(topic_id_map_t *map_handle);

#ifdef __cplusplus
}
#endif

#endif //ESWB_IDS_MAP_H
