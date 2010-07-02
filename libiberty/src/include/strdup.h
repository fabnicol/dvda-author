#ifndef STRDUP_H_INCLUDED
#define STRDUP_H_INCLUDED
#ifdef strdup
#undef strdup
#endif
#define strdup __strdup
char * __strdup (const char *s);

#endif // STRDUP_H_INCLUDED
