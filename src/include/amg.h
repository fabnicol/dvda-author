/*

File:    amg.h
Purpose: Create an Audio Manager (AUDIO_TS.IFO)

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

#ifndef _AMG_H
#define _AMG_H

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "audio2.h"
#include "structures.h"

uint8_t* create_amg(char* audiotsdir, command_t *command, sect* sectors, uint32_t *videotitlelength, uint32_t* relative_sector_pointer_VTSI,
                    uint8_t *numtitles, uint8_t** ntitletracks, uint64_t** titlelength, uint16_t ** ntitlepics);
uint16_t create_tracktables(command_t* command, uint8_t naudio_groups, uint8_t *numtitles, uint8_t *ntitletracks[], uint64_t *titlelength[], uint16_t **ntitlepics);
uint32_t create_topmenu(char* audiotsdir, command_t* command);
int create_stillpics(char* audiotsdir, uint8_t naudio_groups, uint8_t *numtitles, uint16_t **ntitlepics, pic* img, sect* sectors, uint16_t totntracks, uint8_t* ntracks);

#endif
