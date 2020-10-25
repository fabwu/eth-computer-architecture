#ifndef _L1_CACHE_H_
#define _L1_CACHE_H_

#include "common.h"
#include "interconnect.h"

typedef enum Cache_Result { CACHE_MISS, CACHE_HIT } Cache_Result;

typedef struct L1_Cache_Block {
  /* addr saved in this cache block */
  uint32_t tag;
  /* valid bit */
  bool valid;
  /* timestamp for last access */
  int last_access;
} L1_Cache_Block;

struct L1_Cache_State {
  /* name of cache */
  char *label;
  /* total size of cache in bytes */
  int total_size;
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
  L1_Cache_Block *blocks;
  /* ptr to interconnect */
  Interconnect_State *interconnect;
};

/* init L1 cache */
void l1_cache_init(L1_Cache_State *c, char *label, int total_size, int num_ways,
                   Interconnect_State *i);

/* free memory used by cache */
void l1_cache_free(L1_Cache_State *c);

/* simulates a cache access */
Cache_Result l1_cache_access(L1_Cache_State *c, uint32_t addr);

/* insert block into cache */
void l1_insert_block(Cache_Block *b);

/* cancel request to memory hierarchy e.g. on branch recovery */
void l1_cancel_cache_access(L1_Cache_State *c, uint32_t addr);

#endif
