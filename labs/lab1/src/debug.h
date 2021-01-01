#ifndef _DEBUG_H_
#define _DEBUG_H_

/* enable/disable global debugging */
#define DEBUG 0

#if DEBUG
#    define debug(fmt...) printf(fmt);
#else
#    define debug(fmt...) ((void)0)
#endif

#endif
