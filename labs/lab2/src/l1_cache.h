#ifndef _L1_CACHE_H_
#define _L1_CACHE_H_

#include <stdbool.h>
#include <stdint.h>

#include "cache.h"

/* initialize a cache with the ususal values */
void l1_cache_init(L1_Cache_State *c, char *label, int total_size, int num_ways,
                   L2_Cache_State *l2);

/* free memory used by cache */
void l1_cache_free(L1_Cache_State *c);

/* simulates a cache access */
enum Cache_Result l1_cache_access(L1_Cache_State *c, uint32_t addr);

/* insert block into cache */
void l1_insert_block(L1_Cache_State *c, uint32_t addr);

#endif
