#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "memory.h"

void memory_init(Memory_State *m, L2_Cache_State *l2) {
  m->l2 = l2;
  m->requests = list_new();
}

void memory_add_request(Memory_State *m, uint32_t tag) {
  Memory_Request *request = (Memory_Request *)malloc(sizeof(Memory_Request));
  list_rpush(m->requests, list_node_new(request));
  debug_mem("hello %d\n", 1);
}
