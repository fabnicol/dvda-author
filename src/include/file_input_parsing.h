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



int read_tracks(char full_path[CHAR_BUFSIZ], uint8_t *ntracks, char * parent_directory, char* filename, uint8_t ngroups_scan);
parse_t parse_directory(DIR *dir,  uint8_t* ntracks, uint8_t n_g_groups, int action, fileinfo_t **files);
int parse_disk(const char* audiots_chain,  mode_t mode,extractlist *extract);
                        //1 //2 //3  //4  //5  //6
static uint8_t cgadef[]={0,  1,  7,  3,   16,   17};

//cga\nchannels
//    1    2      3       4       5       6
//0   C   
//1   L    R
//2   L    R      S
//3   L    R      Ls      Rs
//4   L    R      Ls      Rs
//5   L    R      Lfe
//6   L    R      Lfe     S
//7   L    R      Lfe     Ls      Rs      
//8   L    R      C
//9   L    R      C       S
//10  L    R      C       Ls      Rs
//11  L    R      C       Lfe
//12  L    R      C       Lfe     S
//13  L    R      C       Lfe     Ls      Rs
//14  L    R      C       S
//15  L    R      C       Ls      Rs
//16  L    R      C       Lfe
//17  L    R      C       Lfe     S
//18  L    R      C       Lfe     Ls      Rs
//19  L    R      Ls      Rs      Lfe
//20  L    R      Ls      Rs      Lfe
//21  L    R      Ls      Rs      C       Lfe

inline static _Bool check_cga_assignment(int cgaint)
{
    int k;
    /*   Valid Channel group assignment values are:


    */


    static uint8_t cga_size=sizeof(cgadef);


    for (k=0; k<cga_size; k++)
        if (cgaint == cgadef[k]) return 1;

    return 0;
}

#endif // FILE_INPUT_PARSING_H_INCLUDED
