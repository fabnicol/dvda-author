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
#ifndef DAVE_OFFSET
#define DAVE_OFFSET 0
#endif

int process_ats(char* audiotsdir,int titleset,fileinfo_t* files, int ntracks, const char* ioflag,char* player);
void pack_scr(uint8_t scr_bytes[6],uint64_t SCR_base, uint16_t SCR_ext);
void pack_pts_dts(uint8_t PTS_DTS_data[10],uint32_t pts, uint32_t dts);
void  write_search_sequence(uint8_t* sequence, size_t sizeofsequence , FILE* filepointer, const char* ioflag);
int process_pes_packet(FILE* fp, fileinfo_t* info, uint8_t* audio_buf, int bytesinbuffer, uint64_t pack_in_title,int pack_in_file,const char* ioflag);
void process_pack_header(FILE* fp,  uint64_t SCRint, const char* ioflag);
#endif
