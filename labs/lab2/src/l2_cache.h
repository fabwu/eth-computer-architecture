#ifndef _L2_CACHE_H_
#define _L2_CACHE_H_

#include "common.h"
#include "interconnect.h"

#define L2_MSHR_SIZE 16

typedef struct L2_Cache_Block {
  /* addr saved in this cache block */
  uint32_t tag;
  /* valid bit */
  bool valid;
  /* timestamp for last access */
  int last_access;
} L2_Cache_Block;

typedef struct L2_MSHR {
  /* cache block */
  Cache_Block *cache_block;
  /* indicates if MSHR is valid (i.e. L1 request was not cancelled) */
  bool valid;
  /* indicates if MSHR was served */
  bool done;
} L2_MSHR;

struct L2_Cache_State {
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
  L2_Cache_Block *blocks;
  /* miss status holding register */
  L2_MSHR mshrs[L2_MSHR_SIZE];
  /* num of allocated MSHRs */
  int mshr_count;
  /* ptr to interconnect */
  Interconnect_State *interconnect;
};

/* initialize a cache with the ususal values */
void l2_cache_init(L2_Cache_State *c, Interconnect_State *interconnect);

/* free memory used by cache */
void l2_cache_free(L2_Cache_State *c);

/* probe L2 cache */
void l2_cache_probe(L2_Cache_State *c, Cache_Block *b);

/* simulate one cycle for L2 cache */
void l2_cycle(L2_Cache_State *c);

/* insert block into L2 cache */
void l2_insert_block(L2_Cache_State *l2, Cache_Block *b);

/* cancel cache access from L1 cache */
void l2_cancel_cache_access(L2_Cache_State *l2, Cache_Block *b);

#endif
