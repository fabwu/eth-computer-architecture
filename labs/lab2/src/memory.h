#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "cache.h"
#include "list.h"

typedef struct Memory_Request {
    /* address of cache line */
    uint32_t tag;
} Memory_Request;

typedef struct Memory_State {
    /* L2 cache */
    L2_Cache_State *l2;
    /* request queue */
    list_t *requests;
} Memory_State;

void memory_init(Memory_State *m, L2_Cache_State *l2);

void memory_add_request(Memory_State *m, uint32_t tag);

#endif
