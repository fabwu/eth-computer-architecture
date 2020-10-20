#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG 1
#define DEBUG_L1 1
#define DEBUG_L2 1
#define DEBUG_MEMORY 1

#if DEBUG && DEBUG_L1
#    define debug_l1(fmt...) printf(fmt);
#else
#    define debug_l1(fmt...) ((void)0)
#endif

#if DEBUG && DEBUG_L2
#    define debug_l2(fmt...) printf("L2: " fmt);
#else
#    define debug_l2(fmt...) ((void)0)
#endif

#if DEBUG && DEBUG_MEMORY
#    define debug_mem(fmt...) printf("MEM: " fmt);
#else
#    define debug_mem(fmt...) ((void)0)
#endif



#endif
