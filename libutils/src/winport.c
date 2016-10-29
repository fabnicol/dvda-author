/*

File:   winport.c
Purpose: windows port function

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net>, 2008

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include "winport.h"
#include "private_c_utils.h"
#include "c_utils.h"
#include "structures.h"

extern globalData global_utils ;

/* met issues on GNU/Linux with stat st_size field ( a few bytes off real size. USe below code from
   http://www.securecoding.cert.org  */

#include <unistd.h>


uint64_t read_file_size(FILE * fp, const char* filename)
{
    if (filename == NULL) return 0;
    uint64_t size = 0;

    if (fp)
    {
        /* get size */
            size=stat_file_size(filename);
    }


    return size;
}

int truncate_from_end(char* filename, uint64_t offset)
{
    return truncate(filename, offset);
}





