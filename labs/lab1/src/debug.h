#ifndef _DEBUG_H_
#define _DEBUG_H_

/* enable/disable global debugging */
#define DEBUG 0
#define DEBUG_PIPE (0 & DEBUG)

#if DEBUG
#    define debug(debug, fmt...) debug ? printf(fmt) : ((void)0)
#else
#    define debug(debug, fmt...) ((void)0)
#endif

#endif
