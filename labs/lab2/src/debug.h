#ifndef _DEBUG_H_
#define _DEBUG_H_

/* enable/disable global debugging */
#define DEBUG 0

/* enable/disable L1 debug messages */
#define DEBUG_L1 1

/* enable/disable L2 debug messages */
#define DEBUG_L2 1

/* enable/disable memory debug messages */
#define DEBUG_MEMORY 1

/* enable/disable interconnect debug messages */
#define DEBUG_INTERCONNECT 1

/* enable this option if L2 should always return a hit */
#define DEBUG_L2_ALWAYS_HIT 0

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

#if DEBUG && DEBUG_INTERCONNECT
#    define debug_int(fmt...) printf("INT: " fmt);
#else
#    define debug_int(fmt...) ((void)0)
#endif

#endif
