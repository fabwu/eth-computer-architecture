#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdbool.h>
#include <stdint.h>

//#define DEBUG

enum Cache_Result { CACHE_MISS, CACHE_HIT };

typedef struct Cache_Block {
  /* addr saved in this cache block */
  uint32_t tag;
  /* valid bit */
  bool valid;
  /* timestamp for last access */
  int last_access;
} Cache_Block;

#endif
