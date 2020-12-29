#include "cache.h"
#include <stdio.h>

int main() {
    Cache_State c;
    cache_init(&c, 1024, 32, 4, CACHE_MRU_LRU);
    cache_access(&c, 1);
    cache_access(&c, 1025);
    cache_access(&c, 2049);
    cache_access(&c, 4097);
    cache_access(&c, 1);
    cache_access(&c, 8193);
    cache_access(&c, 16385);

    for(int i = 0; i < c.num_sets * c.num_ways; i++) {
        Cache_Block block = c.blocks[i];
        if (block.valid) {
            printf("tag 0x%X access %d insert %d\n", block.tag, block.last_access, block.insert_timestamp);
        }
    }
}
