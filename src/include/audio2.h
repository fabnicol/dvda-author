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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#ifdef __GNU_LIBRARY__
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include "export.h"

#include "stream_decoder.h"
#include "fixwav.h"
#include "fixwav_manager.h"
#include "auxiliary.h"
#include "commonvars.h"
#include "command_line_parsing.h"
#include "winport.h"
#ifndef WITHOUT_sox
#include "sox.h"
#include "libsoxconvert.h"
#endif
#include "multichannel.h"
#include "file_input_parsing.h"
#include "c_utils.h"
#include "ats.h"
#include "structures.h"


#define AFMT_WAVE 1
#define AFMT_FLAC 2
#define AFMT_MLP 5
#define AFMT_LPCM 6
#define AFMT_OGG_FLAC 3
#define NO_AFMT_FOUND 4
#define AFMT_WAVE_GOOD_HEADER 10
#define AFMT_WAVE_FIXED 11


int audiofile_getinfo(fileinfo_t* info, globalData* );
int flac_getinfo(fileinfo_t* info, globalData*);
int audio_open(fileinfo_t* info, globalData*);
uint32_t audio_read(fileinfo_t* info, uint8_t* _buf, uint32_t *bytesinbuffer, globalData* globals);
uint32_t audio_read_merged(fileinfo_t* info, uint8_t* _buf, uint32_t *bytesinbuffer, globalData* globals);
int audio_close(fileinfo_t* info, globalData* );
int fixwav_repair(fileinfo_t *info, globalData*);
int launch_sox(fileinfo_t *info, globalData* globals);
command_t *scan_audiofile_characteristics(command_t *command, globalData*);
void read_defaults(void);
uint8_t wav2cga_channels(fileinfo_t *info, globalData*);
uint8_t get_cga_index(const char* cga);
uint8_t check_cga_assignment(long cgaint);
int calc_info(fileinfo_t* info, globalData*);
int decode_mlp_file(fileinfo_t* info, globalData* globals);
bool audit_mlp_header(uint8_t* header, fileinfo_t* info, bool, globalData*);

char* replace_file_extension(const char * filename, const char* infix, const char* new_extension, globalData*);
#endif
