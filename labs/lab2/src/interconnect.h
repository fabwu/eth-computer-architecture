#ifndef _INTERCONNECT_H_
#define _INTERCONNECT_H_

#include "common.h"

typedef struct Interconnect_State {
  /* latency queue */
  list_t *messages;
  /* ptr to L2 cache */
  L2_Cache_State *l2;
  /* ptr to memory */
  Memory_State *m;
} Interconnect_State;

/* init interconnect */
void interconnect_init(Interconnect_State *i, L2_Cache_State *l2,
                       Memory_State *m);

/* free memory allocate by interconnect */
void interconnect_free(Interconnect_State *i);

/* simulate a cycle i.e. process latency queue */
void interconnect_cycle(Interconnect_State *i);

/* send cache probe from L1 to L2 */
void interconnect_l1_to_l2(Interconnect_State *i, Cache_Block *b);

/* send cancellation from L1 to L2 */
void interconnect_l1_to_l2_cancel(Interconnect_State *i, Cache_Block *b);

/* insert block from L2 to L1 */
void interconnect_l2_to_l1(Interconnect_State *i, struct Cache_Block *b);

/* insert block in the same cycle from L2 to L2 */
void interconnect_l2_to_l1_no_latency(Interconnect_State *i,
                                      struct Cache_Block *b);

/* send memory request from L2 to memory */
void interconnect_l2_to_mem(Interconnect_State *i, struct Cache_Block *b);

/* insert block from memory to L2 */
void interconnect_mem_to_l2(Interconnect_State *i, struct Cache_Block *b);

#endif
