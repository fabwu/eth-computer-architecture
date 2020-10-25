#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "l1_cache.h"

static int bit_length(uint32_t n) {
  uint32_t l = 0;

  while (n >>= 1)
    ++l;

  return l;
}

void l1_cache_init(L1_Cache_State *c, char *label, int total_size, int num_ways,
                   Interconnect_State *i) {
  c->label = label;
  c->total_size = total_size;
  c->num_ways = num_ways;

  c->num_sets = (c->total_size / c->num_ways) / CACHE_BLOCK_SIZE;
  c->set_idx_from = bit_length(CACHE_BLOCK_SIZE);

  if (c->num_sets == 1) {
    c->set_idx_to = c->set_idx_from;
  } else {
    c->set_idx_to = c->set_idx_from + bit_length(c->num_sets) - 1;
  }

  /* init sets*ways cache blocks */
  c->blocks = (L1_Cache_Block *)calloc(c->num_sets * c->num_ways,
                                       sizeof(L1_Cache_Block));

  c->timestamp = 0;
  c->interconnect = i;
}

void l1_cache_free(L1_Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(L1_Cache_State *c, uint32_t addr) {
  uint32_t mask = ~0;
  mask >>= 32 - (c->set_idx_to + 1);

  uint32_t set_idx = (addr & mask);
  set_idx >>= c->set_idx_from;

  return set_idx;
}

static void write_block(L1_Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

Cache_Result l1_cache_access(L1_Cache_State *c, uint32_t addr) {
  /* increase timestamp for recency */
  c->timestamp++;

  /* calculate set idx */
  uint32_t tag = CACHE_BLOCK_ALIGNED_ADDR(addr);
  uint32_t set_idx = get_set_idx(c, addr);
  L1_Cache_Block *set = c->blocks + set_idx * c->num_ways;
  L1_Cache_Block *block;

  /* check if addr is in cache */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag)) {
      debug_l1("%s: [0x%X] HIT in set %d way %d\n", c->label, tag, set_idx,
               way);
      write_block(block, tag, c->timestamp);
      return CACHE_HIT;
    }
  }

  debug_l1("%s: [0x%X] MISS in set %d\n", c->label, tag, set_idx);

  /* addr not in cache -> probe L2 cache */
  Cache_Block *b = (Cache_Block *)malloc(sizeof(Cache_Block));
  b->tag = tag;
  b->l1 = c;
  interconnect_l1_to_l2(c->interconnect, b);

  return CACHE_MISS;
}

void l1_insert_block(struct Cache_Block *b) {
  L1_Cache_State *c = b->l1;
  
  c->timestamp++;

  uint32_t tag = b->tag;
  uint32_t set_idx = get_set_idx(c, tag);
  L1_Cache_Block *set = c->blocks + set_idx * c->num_ways;
  L1_Cache_Block *block;

  /* check that addr is not already in cache */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag)) {
      /* if block is already in cache don't insert it again */
      goto out;
    }
  }

  /* try to insert into invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
      debug_l1("%s: [0x%X] inserted (invalid) into set %d way %d\n", c->label,
               tag, set_idx, way);
      write_block(block, tag, c->timestamp);
      goto out;
    }
  }

  /* no invalid block -> evict LRU block */
  block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    L1_Cache_Block *temp_block = set + way;
    if (block->last_access > temp_block->last_access) {
      block = temp_block;
    }
  }

  debug_l1("%s: [0x%X] inserted (lru) in set %d way %d\n", c->label, tag,
           set_idx, (int)(block - set));

  write_block(block, tag, c->timestamp);

out:
  /* always free cache block */
  free(b);
}

void l1_cancel_cache_access(L1_Cache_State *c, uint32_t addr) {
  Cache_Block b;
  b.tag = CACHE_BLOCK_ALIGNED_ADDR(addr);
  b.l1 = c;

  interconnect_l1_to_l2_cancel(c->interconnect, &b);
}
