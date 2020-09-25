#include "cache.h"

void cache_init(Cache *c, int total_size, int block_size, int num_ways) {
   c->num_ways = num_ways;
   c->num_sets = (total_size / num_ways) / block_size;
}

int cache_is_hit(Cache *c, uint32_t addr) {
 
    //TODO Calculate set bits
    return 1;
}
