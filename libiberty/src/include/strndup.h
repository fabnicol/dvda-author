#ifndef STRNDUP_H_INCLUDED
#define STRNDUP_H_INCLUDED
#ifdef strndup
#undef strndup
#endif
#include <stdio.h>
#include <sys/types.h>

char * strndup (const char *s , size_t n);

#endif // STRNDUP_H_INCLUDED
