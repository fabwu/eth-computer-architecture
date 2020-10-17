#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "l1_cache.h"

static int bit_length(uint32_t n) {
  uint32_t l = 0;

  while (n >>= 1)
    ++l;

  return l;
}

void l1_cache_init(L1_Cache_State *c, int total_size, int block_size, int num_ways) {
  c->total_size = total_size;
  c->block_size = block_size;
  c->num_ways = num_ways;

  c->num_sets = (c->total_size / c->num_ways) / c->block_size;
  c->set_idx_from = bit_length(c->block_size);

  if(c->num_sets == 1) {
    c->set_idx_to = c->set_idx_from;
  } else {
    c->set_idx_to = c->set_idx_from + bit_length(c->num_sets) - 1;
  }
  
  /* init sets*ways cache blocks */
  c->blocks =
      (Cache_Block *)calloc(c->num_sets * c->num_ways, sizeof(Cache_Block));

  c->timestamp = 0;
}

void l1_cache_free(L1_Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(L1_Cache_State *c, uint32_t addr) {
    if (c->num_sets == 1) {
        return 0;
    }

    uint32_t mask = ~0;
    mask >>= 32 - (c->set_idx_to + 1);

    uint32_t set_idx = (addr & mask);
    set_idx >>= c->set_idx_from;

    return set_idx;
}

static uint32_t get_tag(L1_Cache_State *c, uint32_t addr) {
  return addr >> (c->set_idx_to + 1);
}

static void write_block(Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

enum Cache_Result l1_cache_access(L1_Cache_State *c, uint32_t addr) {
  uint32_t tag = get_tag(c, addr);

  if (c->total_size == 0) {
    /* if cache is disabled return HIT on second access but evict address
     * immediately */
    Cache_Block *block = c->blocks;

    if (block->valid && (block->tag == tag)) {
      block->valid = false;
#ifdef DEBUG
      printf("0x%X: HIT (cache disabled) tag 0x%X\n", addr, tag);
#endif
      return CACHE_HIT;
    } else {
      block->tag = tag;
      block->valid = true;
#ifdef DEBUG
      printf("0x%X: MISS (cache disabled) tag 0x%X\n", addr, tag);
#endif
      return CACHE_MISS;
    }
  }

  /* increase timestamp for recency */
  c->timestamp++;

  /* calculate set idx */
  uint32_t set_idx = get_set_idx(c, addr);
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
