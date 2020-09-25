#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

typedef struct Cache {
    int num_ways;
    int num_sets;
} Cache;

void cache_init(Cache *c, int total_size, int block_size, int num_ways);

int cache_is_hit(Cache *c, uint32_t addr);

#endif
