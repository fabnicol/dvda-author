/*

File:    audio2.h
Purpose: Deal with reading the input audio files.

dvda-author  - Author a DVD-Audio DVD

(C) Dave Chapman <dave@dchapman.com> 2005, revised by Fabrice Nicol, 2008

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

#ifndef _AUDIO_H
#define _AUDIO_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "structures.h"


#define AFMT_WAVE 1
#define AFMT_FLAC 2
#define AFMT_MLP 5
#define AFMT_LPCM 6
#define AFMT_OGG_FLAC 3
#define NO_AFMT_FOUND 4
#define AFMT_WAVE_GOOD_HEADER 10
#define AFMT_WAVE_FIXED 11


int audiofile_getinfo(fileinfo_t* info);
int flac_getinfo(fileinfo_t* info);
int audio_open(fileinfo_t* info);
uint32_t audio_read(fileinfo_t* info, uint8_t* _buf, uint32_t *bytesinbuffer);
int audio_close(fileinfo_t* info);
int fixwav_repair(fileinfo_t *info);
int launch_sox(char** filename);
command_t *scan_audiofile_characteristics(command_t *command);
void read_defaults(void);
uint8_t wav2cga_channels(fileinfo_t *info);
uint8_t get_cga_index(const char* cga);
uint8_t check_cga_assignment(long cgaint);
int calc_info(fileinfo_t* info);
#endif
