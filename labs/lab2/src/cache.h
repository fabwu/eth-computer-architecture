#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdbool.h>
#include <stdint.h>

enum Cache_Result { CACHE_MISS, CACHE_HIT };

typedef struct Cache_Block {
  /* addr saved in this cache block */
  uint32_t tag;
  /* valid bit */
  bool valid;
  /* timestamp for last access */
  int last_access;
} Cache_Block;

typedef struct Cache_State {
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
} Cache_State;

/* initialize a cache with the ususal values */
void cache_init(Cache_State *c, int total_size, int block_size, int num_ways);

/* free memory used by cache */
void cache_free(Cache_State *c);

/* simulates a cache access */
enum Cache_Result cache_access(Cache_State *c, uint32_t addr);

#endif
