#ifndef _L2_CACHE_H_
#define _L2_CACHE_H_

#include <stdbool.h>
#include <stdint.h>

#include "cache.h"

/* initialize a cache with the ususal values */
void l2_cache_init(L2_Cache_State *c);

/* free memory used by cache */
void l2_cache_free(L2_Cache_State *c);

/* probe L2 cache */
void l2_cache_probe(L2_Cache_State *c, uint32_t addr, L1_Cache_State *l1);

/* reduce remaining cycles or insert data into L1 cache */
void l2_process_l1_notifications(L2_Cache_State *c);

#endif
