#ifndef PRIVATE_C_UTILS_H_INCLUDED
#define PRIVATE_C_UTILS_H_INCLUDED

/* MACROS*/

#ifdef __WIN32__
    #ifdef __MSYS__
    #define MKDIR(X, Y) mkdir(X, Y)
    #else
    #define	MKDIR(X, Y) mkdir(X)
    #endif
#else
#define MKDIR(X, Y) mkdir(X, Y)
#endif




#endif // PRIVATE_C_UTILS_H_INCLUDED
