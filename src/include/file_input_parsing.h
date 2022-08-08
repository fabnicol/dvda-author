/*
File:    file_input_parsing.h
Purpose: parses input directories

dvda-author  - Author a DVD-Audio DVD

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> 2007, 2008

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

#ifndef FILE_INPUT_PARSING_H_INCLUDED
#define FILE_INPUT_PARSING_H_INCLUDED
#include "auxiliary.h"
#define off64_t long long
#include <dirent.h>



int read_tracks(char full_path[CHAR_BUFSIZ], uint8_t *ntracks, char * parent_directory, char* filename, uint8_t ngroups_scan, globalData*);
parse_t parse_directory(DIR *dir,  uint8_t* ntracks, uint8_t n_g_groups, int action, fileinfo_t **files, globalData*);
#if 0 // DEPRECATED
int parse_disk(const DIR*,  mode_t mode,extractlist *extract);
                                       //1 //2 //3  //4  //5  //6
#endif

#endif // FILE_INPUT_PARSING_H_INCLUDED
