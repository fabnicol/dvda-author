/*
File:    videoimport.h
Purpose: imports VIDEO_TS structrures

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

#ifndef VIDEOIMPORT_H_INCLUDED
#define VIDEOIMPORT_H_INCLUDED
#include <inttypes.h>
#include "c_utils.h"
void get_video_system_file_size(char * path_to_VIDEO_TS,  int maximum_VTSI_rank, uint64_t absolute_sector_pointer_VIDEO_TS, uint32_t *relative_sector_pointer_VTSI, globalData*);
void get_video_PTS_ticks(char * path_to_VIDEO_TS, uint32_t *videotitlelength, uint8_t nvideolinking_groups, uint8_t* VTSI_rank, globalData*);
void fread_endian(uint32_t * p, int t, FILE *f);
void import_topmenu(char* video_vob_path, pic* img, bool MIX_TYPE, globalData*);
#endif // VIDEOIMPORT_H_INCLUDED
