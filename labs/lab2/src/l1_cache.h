#ifndef _L1_CACHE_H_
#define _L1_CACHE_H_

#include <stdbool.h>
#include <stdint.h>

#include "cache.h"

typedef struct L1_Cache_State {
  /* total size of cache in bytes */
  int total_size;
  /* block size in bytes */
  int block_size;
  /* number of ways */
  int num_ways;
  /* number of sets */
  int num_sets;
  /* bit number of set idx start */
  int set_idx_from;
  /* bit number of set idx end */
  int set_idx_to;
  /* timestamp gets increased on every cache access */
  int timestamp;
  /* ptr to cache blocks */
  Cache_Block *blocks;
} L1_Cache_State;

/* initialize a cache with the ususal values */
void l1_cache_init(L1_Cache_State *c, int total_size, int block_size, int num_ways);

/* free memory used by cache */
void l1_cache_free(L1_Cache_State *c);

/* simulates a cache access */
enum Cache_Result l1_cache_access(L1_Cache_State *c, uint32_t addr);

#endif
