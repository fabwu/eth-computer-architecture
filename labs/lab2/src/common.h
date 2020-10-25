#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdbool.h>
#include <stdint.h>

#include "debug.h"
#include "list.h"

/* cache block size is 32 byte across the memory hierarchy */
#define CACHE_BLOCK_SIZE 32
#define CACHE_BLOCK_MASK (CACHE_BLOCK_SIZE - 1)
#define CACHE_BLOCK_ALIGNED_ADDR(ADDR) ((ADDR) & ~CACHE_BLOCK_MASK)

typedef struct L1_Cache_State L1_Cache_State;
typedef struct L2_Cache_State L2_Cache_State;
typedef struct Memory_State Memory_State;

/* generic cache block to passed around the memory hierarchy */
typedef struct Cache_Block {
  /* addr saved in this cache block */
  uint32_t tag;
  /* ptr to data or instruction cache */
  L1_Cache_State *l1;
} Cache_Block;

#endif
