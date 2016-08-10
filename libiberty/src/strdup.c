/* Copyright (C) 1991, 1996, 1997, 1998, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */
#ifndef STRDUP_H
#define STRDUP_H


#include <stdlib.h>
#include <string.h>

#ifdef strdup
#undef strdup
#endif


#include "strdup.h"

/* Duplicate S, returning an identical malloc'd string.  */
char *
strdup (const char *s)
{
    size_t len = strlen (s) + 1;
    void *new = malloc (len);

    if (new == NULL)
        return NULL;

    return (char *) memcpy (new, s, len);
}
#endif

