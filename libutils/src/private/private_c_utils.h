#ifndef PRIVATE_C_UTILS_H_INCLUDED
#define PRIVATE_C_UTILS_H_INCLUDED

/* MACROS*/

#if defined(__WIN32__) || defined (_WIN32) || defined (_WIN64) || defined (__CYGWIN__) || defined (__MSYS__)
#  ifndef SEPARATOR
#    define SEPARATOR "\\"
#  endif
#  if defined(__MSYS__) || defined (__CYGWIN__)
#    define MKDIR(X, Y) mkdir(X, Y)
#  else
#    define	MKDIR(X, Y) mkdir(X)
#  endif
#else
#  define MKDIR(X, Y) mkdir(X, Y)
#  ifndef SEPARATOR
#    define SEPARATOR '\\'
#  endif
#endif


#endif // PRIVATE_C_UTILS_H_INCLUDED
