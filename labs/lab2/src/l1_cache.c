#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "l1_cache.h"
#include "l2_cache.h"

static int bit_length(uint32_t n) {
  uint32_t l = 0;

  while (n >>= 1)
    ++l;

  return l;
}

void l1_cache_init(L1_Cache_State *c, char *label, int total_size,
                   int block_size, int num_ways, L2_Cache_State *l2) {
  c->label = label;
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
  c->l2 = l2;

  debug_l1(
      "%s: %d bytes total %d bytes block %d ways %d sets (addr[%d:%d])\n\n",
      c->label, c->total_size, c->block_size, c->num_ways, c->num_sets,
      c->set_idx_to, c->set_idx_from);
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
  /* increase timestamp for recency */
  c->timestamp++;

  /* calculate set idx */
  uint32_t tag = get_tag(c, addr);
  uint32_t set_idx = get_set_idx(c, addr);
  Cache_Block *set = c->blocks + set_idx * c->num_ways;
  Cache_Block *block;

  /* check if addr is in cache */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag)) {
      debug_l1("%s: [0x%X] HIT in set %d way %d tag 0x%X\n", c->label, addr,
               set_idx, way, tag);
      write_block(block, tag, c->timestamp);
      return CACHE_HIT;
    }
  }

  debug_l1("%s: [0x%X] MISS in set %d tag 0x%X\n", c->label, addr, set_idx,
           tag);

  /* addr not in cache -> probe L2 cache */
  l2_cache_probe(c->l2, addr, c);

  return CACHE_MISS;
}

void l1_insert_block(L1_Cache_State *c, uint32_t addr) {
  c->timestamp++;
  /* calculate set idx */
  uint32_t tag = get_tag(c, addr);
  uint32_t set_idx = get_set_idx(c, addr);
  Cache_Block *set = c->blocks + set_idx * c->num_ways;
  Cache_Block *block;

  /* check that addr is not in cache */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    assert(!(block->valid && (block->tag == tag)));
  }

  /* try to insert into invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
      debug_l1("%s: [0x%X] inserted (invalid) into set %d way %d tag 0x%X\n",
               c->label, addr, set_idx, way, tag);
      write_block(block, tag, c->timestamp);
      return;
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

  debug_l1("%s: [0x%X] inserted (lru) in set %d way %d tag 0x%X\n", c->label,
           addr, set_idx, (int)(block - set), tag);

  write_block(block, tag, c->timestamp);
}
