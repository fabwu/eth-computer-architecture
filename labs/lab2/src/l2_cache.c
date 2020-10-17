#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "l2_cache.h"

void l2_cache_init(L2_Cache_State *c) {
  c->total_size = 256 * 1024;
  c->block_size = 32;
  c->num_ways = 16;
  c->num_sets = 512;

  c->set_idx_from = 5;
  c->set_idx_to = 13;

  c->blocks =
      (Cache_Block *)calloc(c->num_sets * c->num_ways, sizeof(Cache_Block));

  c->timestamp = 0;
}

void l2_cache_free(L2_Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(L2_Cache_State *c, uint32_t addr) {
    uint32_t mask = ~0;
    mask >>= 32 - (c->set_idx_to + 1);

    uint32_t set_idx = (addr & mask);
    set_idx >>= c->set_idx_from;

    return set_idx;
}

static uint32_t get_tag(L2_Cache_State *c, uint32_t addr) {
  return addr >> (c->set_idx_to + 1);
}

static void write_block(Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

enum Cache_Result l2_cache_access(L2_Cache_State *c, uint32_t addr) {
  /* increase timestamp for recency */
  c->timestamp++;

  /* calculate set idx */
  uint32_t set_idx = get_set_idx(c, addr);
  uint32_t tag = get_tag(c, addr);
  Cache_Block *set = c->blocks + set_idx * c->num_ways;
  Cache_Block *block;

  /* check if addr is in cache */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag)) {
#ifdef DEBUG
      printf("0x%X: HIT in set %d way %d tag 0x%X\n", addr, set_idx, way, tag);
#endif
      write_block(block, tag, c->timestamp);
      return CACHE_HIT;
    }
  }

  /* addr not in cache -> find invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
#ifdef DEBUG
      printf("0x%X: MISS (invalid) in set %d way %d tag 0x%X\n", addr, set_idx,
             way, tag);
#endif
      write_block(block, tag, c->timestamp);
      return CACHE_MISS;
    }
  }

  /* no invalid block -> evict LRU block */
  block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    if (block->last_access > temp_block->last_access) {
      block = temp_block;
    }
  }

#ifdef DEBUG
  printf("0x%X: MISS (lru) in set %d way %d tag 0x%X\n", addr, set_idx,
         (int)(block - set), tag);
#endif

  write_block(block, tag, c->timestamp);
  return CACHE_MISS;
}
