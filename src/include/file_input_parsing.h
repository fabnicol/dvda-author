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
static const uint8_t default_cga[6] = {0,  1,  7,  3,   16,   17};  //default channel assignment
static const char* cga_define[21] = {"Mono",
                                     "Stereo",
                                     "Lf-Rf-S2",
                                     "Lf-Rf-Ls2-Rs2",
                                     "Lf-Rf-Lfe2",
                                     "Lf-Rf-Lfe2-S2",
                                     "Lf-Rf-Lfe2-Ls2-Rs2",
                                     "Lf-Rf-C2",
                                     "Lf-Rf-C2-S2",
                                     "Lf-Rf-C2-Ls2-Rs2",
                                     "Lf-Rf-C2-Lfe2",
                                     "Lf-Rf-C2-Lfe2-S2",
                                     "Lf-Rf-C2-Lfe2-Ls2-Rs2",
                                     "Lf-Rf-C-S2",
                                     "Lf-Rf-C-Ls2-Rs2",
                                     "Lf-Rf-C-Lfe2",
                                     "Lf-Rf-C-Lfe2-S2",
                                     "Lf-Rf-C-Lfe2-Ls2-Rs2",
                                     "Lf-Rf-Ls-Rs-Lfe2",
                                     "Lf-Rf-Ls-Rs-C2",
                                     "Lf-Rf-Ls-Rs-C2-Lfe2"};  //Litteral  channel assignment

static inline uint8_t get_cga_index(const char* cga)
{
    for (uint8_t u = 0; u < 21; ++u)
    {
        if (strcmp(cga_define[u], cga) == 0) return u;
    }

    return 0xFF;
}

// Channel group assignment (CGA)
//
//        1    2        3         4        5       6
//0       M
//1       L     R
//2       Lf    Rf      S*
//3       Lf    Rf      Ls*      Rs*
//4       Lf    Rf      Lfe*
//5       Lf    Rf      Lfe*     S*
//6       Lf    Rf      Lfe*     Ls*      Rs*
//7       Lf    Rf      C*
//8       Lf    Rf      C*       S*
//0x9     Lf    Rf      C*       Ls*      Rs*
//0xA-10  Lf    Rf      C*       Lfe*
//0xB-11  Lf    Rf      C*       Lfe*     S*
//0xC-12  Lf    Rf      C*       Lfe*     Ls*      Rs*
//0xD-13  Lf    Rf      C        S*
//0xE-14  Lf    Rf      C        Ls*      Rs*
//0xF-15  Lf    Rf      C        Lfe*
//0x10-16 Lf    Rf      C        Lfe*     S*
//0x11-17 Lf    Rf      C        Lfe*     Ls*      Rs*
//0x12-18 Lf    Rf      Ls       Rs       Lfe*
//0x13-19 Lf    Rf      Ls       Rs       C*
//0x14-20 Lf    Rf      Ls       Rs       C*       Lfe*
// Keys:
// * In Group2
// Each group must have either same sample rate or be even multiples (e.g. 96kHz/48 kHz or 88.2 kHz/44.1 kHz)
// Within groups, bitrate may differ but sample rate cannot.
// M: Mono
// Lf: Left front
// Rf: Right front
// Ls: Left surround (behind)
// Rs: Right front
// C:  Center
// Lfe: Low Frequency Effect (Subwoofer)
// S: Surround (just one behind)
// Ls: Left  surround
// Rs: Right surround

// used for MLP

static uint8_t cga_to_channel(uint8_t cgaint)
{
    switch (cgaint)
    {
      case 0:
        return 1;

      case 1:
        return 2;

      case 2:
      case 4:
      case 7:
        return 3;

      case 3:
      case 5:
      case 8:
      case 10:
      case 13:
      case 15:
        return 4;

      case 6:
      case 9:
      case 11:
      case 14:
      case 16:
      case 18:
      case 19:
        return 5;

      case 12:
      case 17:
      case 20:
        return 6;

    default:
       return 0;
    }
}

static uint8_t check_cga_assignment(long cgaint)
{
    if (cgaint >= 0 && cgaint <= 20) return (uint8_t) cgaint;

    return 0xFF;
}




#endif // FILE_INPUT_PARSING_H_INCLUDED
