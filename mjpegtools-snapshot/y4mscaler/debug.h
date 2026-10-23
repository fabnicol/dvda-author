#ifndef _YS_debug_h_
#define _YS_debug_h_


#ifdef _YS_DEV_BRANCH_
#  define DBG(format, args...) fprintf(stderr, format , ## args)
#  define _YS_DEBUG 1
#else
#  define DBG(format, args...) 
#endif


#endif /* _YS_debug_h_ */


