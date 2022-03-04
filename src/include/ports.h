/*
File:    ports.h
Purpose: ports to Windows Mingw compiler

dvda-author  - Author a DVD-Audio DVD

(C) Revised version with zone-to-zone linking Fabrice Nicol <fabnicol@users.sourceforge.net> 2007, 2008

The latest version can be found at http://dvd-audio.sourceforge.net

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

#ifndef PORTS_INCLUDED
#define PORTS_INCLUDED


/* The debian Mingwin compiler ignores access rights */

#ifdef __MINGW32__
#define off64_t long long
#include <io.h>
#define COMPUTE_EXECTIME

#else

#include <sys/resource.h>
#include "commonvars.h"

/* a compute_t *timer pointer must be included equally */

#define COMPUTE_EXECTIME    do{ \
	struct rusage end;\
	getrusage(RUSAGE_SELF, &end);\
	unsigned int SEC1=end.ru_utime.tv_sec - timer.start->ru_utime.tv_sec - timer.nothing->ru_utime.tv_sec, SEC2=end.ru_stime.tv_sec- timer.start->ru_stime.tv_sec - timer.nothing->ru_stime.tv_sec;\
    unsigned int MICRSEC1=end.ru_utime.tv_usec - timer.start->ru_utime.tv_usec - timer.nothing->ru_utime.tv_usec, MICRSEC2=end.ru_stime.tv_usec- timer.start->ru_stime.tv_usec - timer.nothing->ru_stime.tv_usec;\
                fprintf(stderr, INFO_EXECTIME1 "\n",  SEC1/60, SEC1%60, MICRSEC1 );\
                fprintf(stderr, INFO_EXECTIME2 "\n",  SEC2/60, SEC2%60, MICRSEC2 );\
				} while(0);



#endif






#endif // PORTS_INCLUDED
