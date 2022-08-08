/*

File:    ats.h
Purpose: Create an Audio Titleset

dvda-author  - Author a DVD-Audio DVD

(C) Dave Chapman <dave@dchapman.com> 2005

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

#ifndef _ATS_H
#define _ATS_H

#include <stdio.h>
#include <stdint.h>

#include "audio2.h"
#include "decode.h"

int create_ats(char* audiotsdir,int titleset,fileinfo_t* files, int ntracks, globalData*);
void pack_scr(uint8_t scr_bytes[6],uint64_t SCR_base, uint16_t SCR_ext);
void pack_pts_dts(uint8_t PTS_DTS_data[10], uint32_t pts, uint32_t dts);
int decode_ats(globalData*);
int read_pes_packet(FILE* fp, fileinfo_t* info, uint8_t* audio_buf, globalData*);
uint32_t convert_to_PTS(fileinfo_t* info);
extern FILE* aob_log;

#ifndef PACK_POSITION
# define PACK_POSITION
#endif
#ifndef FIRST_PACK
# define FIRST_PACK   0
#endif
# define LAST_PACK    1
# define MIDDLE_PACK  2
# define END_OF_AOB 3
# define CUT_PACK 4
# define CUT_PACK_RMDR 5
# define VALID true
# define NORMAL 0
# define INVALID false


#endif
