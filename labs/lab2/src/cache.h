#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdbool.h>
#include <stdint.h>

#include "list.h"

enum Cache_Result { CACHE_MISS, CACHE_HIT };

#define CACHE_BLOCK_SIZE 32 
#define CACHE_BLOCK_MASK (CACHE_BLOCK_SIZE - 1)
#define CACHE_BLOCK_ALIGNED_ADDR(ADDR) ((ADDR) & ~CACHE_BLOCK_MASK)

typedef struct L1_Cache_State L1_Cache_State;
typedef struct L2_Cache_State L2_Cache_State;
typedef struct L2_L1_Notification L2_L1_Notification;

typedef struct Cache_Block {
  /* addr saved in this cache block */
  uint32_t tag;
  /* valid bit */
  bool valid;
  /* timestamp for last access */
  int last_access;
} Cache_Block;

typedef struct L2_MSHR {
  /* cache block address (i.e. address without offset) */
  uint32_t tag;
  /* indicates if MSHR is valid */
  bool valid;
  /* indicates if MSHR was served */
  bool done;
} L2_MSHR;

struct L2_L1_Notification {
  /* address to insert into L1 cache */
  uint32_t tag;
  /* remaining cycles */
  int cycles;
  /* ptr to L1 cache */
  L1_Cache_State *l1;
};

#define L2_MSHR_SIZE 16

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
  Cache_Block *blocks;
  /* miss status holding register */
  L2_MSHR mshrs[L2_MSHR_SIZE];
  /* pending notifications for L1 caches */
  list_t *l1_notifications;
};

struct L1_Cache_State {
  /* name of cache */
  char *label;
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
  Cache_Block *blocks;
  /* ptr to L2 cache */
  L2_Cache_State *l2;
};

#endif
