/*

File:    atsi.h
Purpose: Create an Audio Titleset Information file

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

#ifndef _ATSI_H
#define _ATSI_H

#include <stdio.h>
#include <stdint.h>

#include "audio2.h"
#include "structures.h"
int create_atsi(command_t *command, char* audiotsdir,uint8_t titleset,uint8_t* atsi_sectors, uint16_t * ntitlepics, globalData* );
int get_afmt(fileinfo_t* info, audioformat_t* audioformats, int* numafmts, globalData*);
#endif
