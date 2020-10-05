#include "cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG

__attribute__((unused)) static void printBits(size_t const size,
                                              void const *const ptr) {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  for (i = size - 1; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  printf("\n");
}

static int bit_length(uint32_t n) {
  assert(n % 2 == 0);

  uint32_t l = 0;

  while (n >>= 1)
    ++l;

  return l;
}

void cache_init(Cache_State *c, int total_size, int block_size, int num_ways) {
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
  c->set_idx_to = c->set_idx_from + bit_length(c->num_sets) - 1;

  /* init sets*ways cache blocks */
  c->blocks =
      (Cache_Block *)calloc(c->num_sets * c->num_ways, sizeof(Cache_Block));

  c->timestamp = 0;
}

void cache_free(Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(Cache_State *c, uint32_t addr) {
  uint32_t mask = ~0;
  mask >>= 32 - (c->set_idx_to + 1);

  uint32_t set_idx = (addr & mask);
  set_idx >>= c->set_idx_from;

  return set_idx;
}

static void write_block(Cache_Block *block, uint32_t addr, int timestamp) {
  block->valid = true;
  block->addr = addr;
  block->last_access = timestamp;
}

enum Cache_Result cache_access(Cache_State *c, uint32_t addr) {
  if (c->total_size == 0) {
    /* if cache is disabled return HIT on second access but evict address
     * immediately */
    Cache_Block *block = c->blocks;

    if (block->addr == addr) {
      block->addr = -1;
      return CACHE_HIT;
    } else {
      block->addr = addr;
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
    if (block->addr == addr) {
#ifdef DEBUG
      printf("0x%X: HIT in set %d way %d\n", addr, set_idx, way);
#endif
      write_block(block, addr, c->timestamp);
      return CACHE_HIT;
    }
  }

  /* addr not in cache -> find invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
#ifdef DEBUG
      printf("0x%X: MISS (invalid) in set %d way %d\n", addr, set_idx, way);
#endif
      write_block(block, addr, c->timestamp);
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
  printf("0x%X: MISS (lru) in set %d way %d\n", addr, set_idx,
         (int)(block - set));
#endif

  write_block(block, addr, c->timestamp);
  return CACHE_MISS;
}
