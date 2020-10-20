#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "l2_cache.h"
#include "l1_cache.h"

void l2_cache_init(L2_Cache_State *c) {
  c->total_size = 256 * 1024;
  c->num_ways = 16;
  c->num_sets = 512;

  c->set_idx_from = 5;
  c->set_idx_to = 13;

  c->blocks =
      (Cache_Block *)calloc(c->num_sets * c->num_ways, sizeof(Cache_Block));

  c->timestamp = 0;

  c->l1_notifications = list_new();
}

void l2_cache_free(L2_Cache_State *c) { free(c->blocks); }

static uint32_t get_set_idx(L2_Cache_State *c, uint32_t addr) {
    uint32_t mask = ~0;
    mask >>= 32 - (c->set_idx_to + 1);

    uint32_t set_idx = (addr & mask);
    set_idx >>= c->set_idx_from;

    return set_idx;
}

static void write_block(Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

static void notify_l1_cache(L2_Cache_State *l2, uint32_t addr, L1_Cache_State *l1) {
  /* tag is address minus offset */
  uint32_t tag = CACHE_BLOCK_ALIGNED_ADDR(addr);
  L2_L1_Notification *notification;
  /* check for pending notification */
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(l2->l1_notifications, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    notification = (L2_L1_Notification *)node->val;

    if (notification->tag == tag) {
      /* notification exists -> nothing to do */
      debug_l2("L1 notification for %s (0x%x) exists (%d cylces left)\n",
               l1->label, notification->tag, notification->cycles);
      goto out;
    }
  }

  /* no pending notification -> insert one */
  notification = (L2_L1_Notification *)malloc(sizeof(L2_L1_Notification));
  notification->tag = tag;
  notification->cycles = 14;
  notification->l1 = l1;
  list_rpush(l2->l1_notifications, list_node_new(notification));
  debug_l2("L1 notification for %s (0x%x) inserted (%d cylces left)\n",
           l1->label, notification->tag, notification->cycles);

out:
  list_iterator_destroy(it);
  return;
}

void l2_cache_probe(L2_Cache_State *l2, uint32_t addr, L1_Cache_State *l1) {
  // TODO defer probing if MSHR is full
  /* increase timestamp for recency */
  l2->timestamp++;

  /* calculate set idx */
  uint32_t set_idx = get_set_idx(l2, addr);
  uint32_t tag = CACHE_BLOCK_ALIGNED_ADDR(addr);
  Cache_Block *set = l2->blocks + set_idx * l2->num_ways;
  Cache_Block *block;

  /* check if addr is in cache */
  for (int way = 0; way < l2->num_ways; ++way) {
    block = set + way;
    if (block->valid && (block->tag == tag)) {
      debug_l2("[0x%X] HIT in set %d way %d\n", tag, set_idx, way);
      write_block(block, tag, l2->timestamp);
      notify_l1_cache(l2, addr, l1);

      return;
    }
  }

  debug_l2("[0x%X] MISS in set %d\n", tag, set_idx);

  /* addr not in cache -> allocate MSHR */
  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = l2->mshrs + i;
    if (mshr->valid == true && mshr->done == false && mshr->tag == tag) {
      /* MSHR already allocated -> done */
      debug_l2("[0x%X] MSHR already allocated\n", tag);
      return;
    }
  }

  for (int i = 0; i < L2_MSHR_SIZE; ++i) {
    L2_MSHR *mshr = l2->mshrs + i;
    if (mshr->valid == false) {
      mshr->valid = true;
      mshr->done = false;
      mshr->tag = tag;
      debug_l2("[0x%X] MSHR allocated\n", tag);
      return;
    }
  }

  /* no MSHR available -> should not happen */
  assert(0);
}

void l2_process_l1_notifications(L2_Cache_State *l2) {
    L2_L1_Notification *notification;
    /* check for pending notification */
    list_node_t *node;
    list_iterator_t *it = list_iterator_new(l2->l1_notifications, LIST_HEAD);
    while ((node = list_iterator_next(it))) {
      notification = (L2_L1_Notification *)node->val;

      if (notification->cycles > 0) {
          notification->cycles--;
      } else {
        debug_l2("tag 0x%x inserted into %s\n", notification->tag,
                 notification->l1->label);
        l1_insert_block(notification->l1, notification->tag);
        list_remove(l2->l1_notifications, node);
      }
  }

  list_iterator_destroy(it);
}
