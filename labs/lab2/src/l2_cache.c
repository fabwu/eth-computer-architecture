#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "l2_cache.h"

void l2_cache_init(L2_Cache_State *l2, Interconnect_State *interconnect) {
  l2->total_size = 256 * 1024;
  l2->num_ways = 16;
  l2->num_sets = 512;

  l2->set_idx_from = 5;
  l2->set_idx_to = 13;

  l2->blocks = (L2_Cache_Block *)calloc(l2->num_sets * l2->num_ways,
                                        sizeof(L2_Cache_Block));

  l2->timestamp = 0;

  l2->interconnect = interconnect;
  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = l2->mshrs + i;
    mshr->done = true;
  }
}

void l2_cache_free(L2_Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(L2_Cache_State *c, uint32_t addr) {
  uint32_t mask = ~0;
  mask >>= 32 - (c->set_idx_to + 1);

  uint32_t set_idx = (addr & mask);
  set_idx >>= c->set_idx_from;

  return set_idx;
}

static void write_block(L2_Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

static bool cache_block_equal(Cache_Block *a, Cache_Block *b) {
  return a != NULL && b != NULL && a->tag == b->tag && a->l1 == b->l1;
}

void l2_cache_probe(L2_Cache_State *l2, struct Cache_Block *b) {
  /* L1 cannot probe L2 if no MSHR is free */
  if (l2->mshr_count >= L2_MSHR_SIZE) {
    debug_l2("[0x%X] all MSHRs in use\n", b->tag);
    return;
  }

#if DEBUG_L2_ALWAYS_HIT
  /* useful for e.g. test L2 hit latency */
  debug_l2("ALWAYS HIT IS ENABLED!\n");
  interconnect_l2_to_l1(l2->interconnect, b);
  return;
#endif

  /* increase timestamp for recency */
  l2->timestamp++;

  uint32_t tag = b->tag;
  uint32_t set_idx = get_set_idx(l2, tag);
  L2_Cache_Block *set = l2->blocks + set_idx * l2->num_ways;
  L2_Cache_Block *block;

  /* check if addr is in cache */
  for (int way = 0; way < l2->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag)) {
      debug_l2("[0x%X] HIT in set %d way %d\n", tag, set_idx, way);
      /* promote block to MRU position */
      write_block(block, tag, l2->timestamp);
      /* send cache block back to L1 */
      interconnect_l2_to_l1(l2->interconnect, b);
      return;
    }
  }

  /* addr not in cache */
  debug_l2("[0x%X] MISS in set %d\n", tag, set_idx);

  /* check if MSHR for tag already exists */
  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = l2->mshrs + i;
    if (!mshr->done && cache_block_equal(mshr->cache_block, b)) {
      /* MSHR already allocated -> done */
      debug_l2("[0x%X] MSHR already allocated\n", tag);
      return;
    }
  }

  /* no MSHR exists -> allocate one and send request to memory */
  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = l2->mshrs + i;
    if (mshr->done) {
      mshr->valid = true;
      mshr->done = false;
      mshr->cache_block = b;
      l2->mshr_count++;
      debug_l2("[0x%X] MSHR allocated\n", tag);
      interconnect_l2_to_mem(l2->interconnect, b);
      return;
    }
  }

  /* no MSHR available -> should not happen */
  assert(0);
}

void l2_insert_block(L2_Cache_State *c, Cache_Block *b) {
  c->timestamp++;

  uint32_t tag = b->tag;
  uint32_t set_idx = get_set_idx(c, tag);
  L2_Cache_Block *set = c->blocks + set_idx * c->num_ways;
  L2_Cache_Block *block;

  /* check that addr is not in cache */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag))
       goto out; 
  }

  /* try to insert into invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
      debug_l2("[0x%X] inserted (invalid) into set %d way %d\n", tag, set_idx,
               way);
      write_block(block, tag, c->timestamp);
      goto out;
    }
  }

  /* no invalid block -> evict LRU block */
  block = set + 0;
  for (int way = 1; way < c->num_ways; ++way) {
    L2_Cache_Block *temp_block = set + way;
    if (block->last_access > temp_block->last_access) {
      block = temp_block;
    }
  }

  debug_l2("[0x%X] inserted (lru) in set %d way %d\n", tag, set_idx,
           (int)(block - set));
  /* add at MRU position */
  write_block(block, tag, c->timestamp);

out:
  /* free MSHR */
  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = c->mshrs + i;
    if (!mshr->done && cache_block_equal(mshr->cache_block, b)) {
      if (mshr->valid) {
        /* insert into L1 if MSHR was not cancelled before */
        interconnect_l2_to_l1_no_latency(c->interconnect, b);
      } else {
        free(b);
      }

      mshr->valid = false;
      mshr->done = true;
      c->mshr_count--;
      return;
    }
  }
}

void l2_cancel_cache_access(L2_Cache_State *l2, Cache_Block *b) {
  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = l2->mshrs + i;
    if (!mshr->done && cache_block_equal(mshr->cache_block, b)) {
      /* invalidate MSHR from initial L1 cache and tag */
      mshr->valid = false;
    }
  }
}
