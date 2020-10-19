#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "l2_cache.h"
#include "l1_cache.h"

void l2_cache_init(L2_Cache_State *c) {
  c->label = "L2 cache";
  c->total_size = 256 * 1024;
  c->block_size = 32;
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

static uint32_t get_tag(L2_Cache_State *c, uint32_t addr) {
  return addr >> (c->set_idx_to + 1);
}

static void write_block(Cache_Block *block, uint32_t tag, int timestamp) {
  block->valid = true;
  block->tag = tag;
  block->last_access = timestamp;
}

static void notify_l1_cache(L2_Cache_State *l2, uint32_t addr, L1_Cache_State *l1) {
  /* tag is address minus offset */
  uint32_t tag = (addr >> l2->set_idx_from) << l2->set_idx_from;
  L2_L1_Notification *notification;
  /* check for pending notification */
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(l2->l1_notifications, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    notification = (L2_L1_Notification *)node->val;

    if (notification->tag == tag) {
      /* notification exists -> nothing to do */
#ifdef DEBUG
      printf("%s: L1 notification for %s (0x%x) exists (%d cylces left)\n",
             l2->label, l1->label, notification->tag, notification->cycles);
#endif
      goto out;
    }
  }

  /* no pending notification -> insert one */
  notification = (L2_L1_Notification *)malloc(sizeof(L2_L1_Notification));
  notification->tag = tag;
  notification->cycles = 14;
  notification->l1 = l1;
  list_rpush(l2->l1_notifications, list_node_new(notification));
#ifdef DEBUG
  printf("%s: L1 notification for %s (0x%x) inserted (%d cylces left)\n",
         l2->label, l1->label, notification->tag, notification->cycles);
#endif

out:
  list_iterator_destroy(it);
  return;
}

void l2_cache_probe(L2_Cache_State *c, uint32_t addr, L1_Cache_State *l1) {
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
    //TODO Remove this 
    write_block(block, tag, c->timestamp);
    if (block->valid && (block->tag == tag)) {
#ifdef DEBUG
      printf("%s: [0x%X] HIT in set %d way %d tag 0x%X\n", c->label, addr, set_idx, way, tag);
#endif
      write_block(block, tag, c->timestamp);
      notify_l1_cache(c, addr, l1);

      return;
    }
  }

  //TODO forward request to DRAM
  return;
  
  /* addr not in cache -> find invalid block */
  for (int way = 0; way < c->num_ways; ++way) {
    block = set + way;
    if (!block->valid) {
#ifdef DEBUG
      printf("%s: [0x%X] MISS (invalid) in set %d way %d tag 0x%X\n", c->label, addr, set_idx,
             way, tag);
#endif
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

#ifdef DEBUG
  printf("%s: [0x%X] MISS (lru) in set %d way %d tag 0x%X\n", c->label, addr, set_idx,
         (int)(block - set), tag);
#endif

  write_block(block, tag, c->timestamp);
  return;
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
#ifdef DEBUG
        printf("%s: tag 0x%x inserted into %s\n", l2->label, notification->tag,
               notification->l1->label);
#endif
        l1_insert_block(notification->l1, notification->tag);
        list_remove(l2->l1_notifications, node);
      }
  }

  list_iterator_destroy(it);
}
