#include "debug.h"
#include "cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int bit_length(uint32_t n) {
  uint32_t l = 0;

  while (n >>= 1)
    ++l;

  return l;
}

void cache_init(Cache_State *c, int total_size, int block_size, int num_ways, Cache_Policy policy) {
  c->total_size = total_size;
  c->block_size = block_size;
  c->num_ways = num_ways;

  if (c->total_size == 0) {
    /* if cache is disabled just allocate one block */
    c->blocks = (Cache_Block *)calloc(1, sizeof(Cache_Block));
    return;
  }

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

  c->policy = policy;
  c->timestamp = 0;
  c->insert_counter = 0;
}

void cache_free(Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(Cache_State *c, uint32_t addr) {
    if (c->num_sets == 1) {
        return 0;
    }

    uint32_t mask = ~0;
    mask >>= 32 - (c->set_idx_to + 1);

    uint32_t set_idx = (addr & mask);
    set_idx >>= c->set_idx_from;

    return set_idx;
}

static uint32_t get_tag(Cache_State *c, uint32_t addr) {
  return addr >> (c->set_idx_to + 1);
}

static void write_block(Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

static void cache_lru_lru_replacement(Cache_State *c, Cache_Block *set, uint32_t addr) {
  /* no invalid block -> evict LRU block */
  Cache_Block *block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    if (block->last_access > temp_block->last_access) {
      block = temp_block;
    }
  }

  uint32_t tag = get_tag(c, addr);
  debug("0x%X: MISS (lru/lru) replaced tag 0x%X in set %d way %d with tag 0x%X\n",
        addr, block->tag, get_set_idx(c, addr), (int)(block - set), tag);

  /* insert at LRU location */
  write_block(block, tag, block->last_access);
}

static void cache_lru_mru_replacement(Cache_State *c, Cache_Block *set, uint32_t addr) {
  /* evict LRU block */
  Cache_Block *block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    if (block->last_access > temp_block->last_access) {
      block = temp_block;
    }
  }

  uint32_t tag = get_tag(c, addr);
  debug("0x%X: MISS (lru/mru) replaced tag 0x%X in set %d way %d with tag 0x%X\n",
        addr, block->tag, get_set_idx(c, addr), (int)(block - set), tag);

  /* insert at MRU location */
  write_block(block, tag, c->timestamp);
}

static void cache_mru_mru_replacement(Cache_State *c, Cache_Block *set, uint32_t addr) {
  /* evict MRU block */
  Cache_Block *block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    if (block->last_access < temp_block->last_access) {
      block = temp_block;
    }
  }

  uint32_t tag = get_tag(c, addr);
  debug("0x%X: MISS (mru/mru) replaced tag 0x%X in set %d way %d with tag 0x%X\n",
        addr, block->tag, get_set_idx(c, addr), (int)(block - set), tag);

  /* insert at MRU location */
  write_block(block, tag, c->timestamp);
}

static void cache_mru_lru_replacement(Cache_State *c, Cache_Block *set, uint32_t addr) {
  /* evict MRU block */
  Cache_Block *block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    if (block->last_access < temp_block->last_access) {
      block = temp_block;
    }
  }

  int lru_timestamp = set->last_access;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    if (lru_timestamp > temp_block->last_access) {
      lru_timestamp = temp_block->last_access;
    }
  }

  uint32_t tag = get_tag(c, addr);
  debug("0x%X: MISS (mru/lru) replaced tag 0x%X in set %d way %d with tag 0x%X\n",
        addr, block->tag, get_set_idx(c, addr), (int)(block - set), tag);

  write_block(block, tag, lru_timestamp);
}

static void cache_fifo_replacement(Cache_State *c, Cache_Block *set, uint32_t addr) {
  Cache_Block *block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    // pick oldest block
    if (temp_block->insert_timestamp < block->insert_timestamp) {
      block = temp_block;
    }
  }

  uint32_t tag = get_tag(c, addr);
  debug("0x%X: MISS (fifo) replaced tag 0x%X in set %d way %d with tag 0x%X\n",
        addr, block->tag, get_set_idx(c, addr), (int)(block - set), tag);

  c->insert_counter++;
  block->insert_timestamp = c->insert_counter;

  write_block(block, tag, c->timestamp);
}

static void cache_lifo_replacement(Cache_State *c, Cache_Block *set, uint32_t addr) {
  Cache_Block *block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    Cache_Block *temp_block = set + way;
    // pick oldest block
    if (temp_block->insert_timestamp > block->insert_timestamp) {
      block = temp_block;
    }
  }

  uint32_t tag = get_tag(c, addr);
  debug("0x%X: MISS (lifo) replaced tag 0x%X in set %d way %d with tag 0x%X\n",
        addr, block->tag, get_set_idx(c, addr), (int)(block - set), tag);

  c->insert_counter++;
  block->insert_timestamp = c->insert_counter;

  write_block(block, tag, c->timestamp);
}

enum Cache_Result cache_access(Cache_State *c, uint32_t addr) {
  uint32_t tag = get_tag(c, addr);

  if (c->total_size == 0) {
    /* if cache is disabled return HIT on second access but evict address
     * immediately */
    Cache_Block *block = c->blocks;

    if (block->valid && (block->tag == tag)) {
      block->valid = false;

      debug("0x%X: HIT (cache disabled) tag 0x%X\n", addr, tag);

      return CACHE_HIT;
    } else {
      block->tag = tag;
      block->valid = true;

      debug("0x%X: MISS (cache disabled) tag 0x%X\n", addr, tag);

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
      debug("0x%X: HIT in set %d way %d tag 0x%X\n", addr, set_idx, way, tag);

      write_block(block, tag, c->timestamp);
      return CACHE_HIT;
    }
  }

  /* addr not in cache -> find invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
      debug("0x%X: MISS (invalid) in set %d way %d tag 0x%X\n", addr, set_idx,
             way, tag);

      c->insert_counter++;
      block->insert_timestamp = c->insert_counter;
      write_block(block, tag, c->timestamp);

      return CACHE_MISS;
    }
  }

  replacement_func_t policies[LAST_CACHE_POLICY];
  policies[CACHE_LRU_LRU] = &cache_lru_lru_replacement;
  policies[CACHE_LRU_MRU] = &cache_lru_mru_replacement;
  policies[CACHE_MRU_MRU] = &cache_mru_mru_replacement;
  policies[CACHE_MRU_LRU] = &cache_mru_lru_replacement;
  policies[CACHE_FIFO] = &cache_fifo_replacement;
  policies[CACHE_LIFO] = &cache_lifo_replacement;
  policies[c->policy](c, set, addr);

  return CACHE_MISS;
  /* TODO
   *
   *  general:
   *  - FIFO
   *  - LIFO
   *
   *  replace/insert:
   *  - LRU/LRU
   *  - LRU/MRU
   *  - MRU/MRU
   *  - MRU/LRU
   *
   *   check other paper for other methods
   */
}
