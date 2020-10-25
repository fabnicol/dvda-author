/*

File:   videoimport.c
Purpose: imports VIDEO_TS

dvda-author  - Author a DVD-Audio DVD

 Author a DVD-Audio DVD

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net>, 2008-2012

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if !defined HAVE_core_BUILD || !HAVE_core_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/time.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <string.h>
#include "structures.h"
#include "commonvars.h"
#include "videoimport.h"
#include "auxiliary.h"
#include "c_utils.h"
#include "ports.h"
#include "menu.h"
#include "sound.h"


void get_video_system_file_size(char * path_to_VIDEO_TS,  int maximum_VTSI_rank, uint64_t sector_pointer_VIDEO_TS, uint32_t *relative_sector_pointer_VTSI, globalData* globals)
{

    if (path_to_VIDEO_TS == NULL || relative_sector_pointer_VTSI == NULL)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE("path_to_VIDEO_TS and relative_sector_pointer_VTSI should be allocated.")
        }

    int k=0;

    int len = strlen (path_to_VIDEO_TS);
    int vtsi_path_length=len +13 +1;

//  Requires std=c99

    char  temp[vtsi_path_length];

    FILE  *temp_file;

    STRING_WRITE(temp,  "%s/VIDEO_TS.IFO", path_to_VIDEO_TS)

    if ((temp_file=fopen(temp, "rb")) == NULL)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not open VIDEO_TS.IFO. There must be a valid DVD-VIDEO file structure under VIDEO_TS.")
    }

//  Retrieving size of VIDEO_TS.IFO
//  The size of VIDEO_TS.IFO + VIDEO_TS.VOB + VIDEO_TS.BUP
//  is at offset 0xC (0-based, one must add 1 to get real sector size)

    if (fseek(temp_file, 0xC, SEEK_SET) != 0)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not seek offset 0x0C of VIDEO_TS.IFO")
    }

    fread_endian(relative_sector_pointer_VTSI, 0, temp_file);

    relative_sector_pointer_VTSI[0] += sector_pointer_VIDEO_TS + 1;

    foutput(MSG_TAG "Maximum rank of VTSI:  %d\n", maximum_VTSI_rank);

    for (k=1; k< maximum_VTSI_rank; k++)
    {

        STRING_WRITE(temp,  "%s/VTS_%02d_0.IFO", path_to_VIDEO_TS, k)

        if ((temp_file=fopen (temp, "rb")) == NULL)
        {
            foutput(ERR "Impossible to open file '%s'\n", temp);
            EXIT_ON_RUNTIME_ERROR
        }

 // retrieving size of VTS_XX_0.IFO

        if (fseek(temp_file, 0xC, SEEK_SET) !=0)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not seek offset 0xC of VTS....IFO")
        }

        fread_endian(relative_sector_pointer_VTSI, k, temp_file);

        relative_sector_pointer_VTSI[k] += relative_sector_pointer_VTSI[k-1] +1;

        if (globals->debugging) foutput(INF "Retrieving relative sector pointer to VTSI %d : %"PRIu32"\n", k+1, relative_sector_pointer_VTSI[k]);

        fclose(temp_file);
    }

}


void get_video_PTS_ticks(char* path_to_VIDEO_TS, uint32_t *videotitlelength, uint8_t nvideolinking_groups, uint8_t* VTSI_rank, globalData* globals)
{
    int k;
    int len = strlen (path_to_VIDEO_TS);
    int vtsi_path_length=len +13 +1;
    uint8_t hours=0, minutes=0, seconds=0;

// Requires std=c99

    char  temp[vtsi_path_length];

    FILE  *temp_file;

// Parsing again rather than integrating to videoimport function for development purposes

    for (k=0 ; k< nvideolinking_groups ; k++)
    {

        STRING_WRITE(temp,  "%s/VTS_%02d_0.IFO", path_to_VIDEO_TS, VTSI_rank[k])

        if ((temp_file=fopen (temp, "rb")) == NULL)
        {
            foutput(ERR "Impossible to open file '%s'\n", temp);
            EXIT_ON_RUNTIME_ERROR
        }

//  retrieving length of VTS_XX_0.IFO in PTS ticks
//  offsets 4, 5, 6 of  VTS_ PGC are coded in BCD(a hex represents a decimal).
//  maybe unavailable on some discs or located at other offsets than PLAYBACK_TIME_OFFSET

        fseek(temp_file, PLAYBACK_TIME_OFFSET, SEEK_SET);

        if (fread(&hours, 1, 1, temp_file) != 1)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not read 1 byte at offset 4 of PGC")
        }

        if (fread(&minutes, 1, 1, temp_file) != 1)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not read 1 byte at offset 5 of PGC")
        }

        if (fread(&seconds, 1, 1, temp_file) != 1)
        {
             EXIT_ON_RUNTIME_ERROR_VERBOSE( "Could not read 1 byte at offset 6 of PGC")
        }

// frames will not be considered

        videotitlelength[k] = 90000 *(3600 * BCD(hours)  + 60 *BCD(minutes)  + BCD(seconds));

        if (globals->debugging)
            foutput("\n"MSG_TAG "Linked video group=%d \n       hours=%x  minutes=%x  seconds=%x\n       PTS ticks=%"PRIu32" length (seconds)=%"PRIu32" \n",
                    VTSI_rank[k],
                    hours,
                    minutes,
                    seconds,
                    videotitlelength[k],
                    videotitlelength[k]/90000);

    }

}


extern char *mpeg2dec, *mpeg2enc, *pgmtoy4m, *mplex, *extract_ac3, *ac3dec;

void import_topmenu(char* video_vob_path, pic* img, bool MIX_TYPE, globalData* globals)
{
 initialize_binary_paths(CREATE_MJPEGTOOLS, globals);
 initialize_binary_paths(CREATE_MPEG2DEC, globals);

// Limitation to be removed later?

 img->nmenus=1;
 int s=strlen(globals->settings.tempdir);

 if (MIX_TYPE == USE_VTS_SOUNDTRACK)
 {

//  FREE(img->soundtrack[0]
//  extract lpcm from VTS or else convert mp2/ac3 to lpcm  then assign img->soundtrack[0]=result

  initialize_binary_paths(CREATE_EXTRACT_AC3, globals);
  FREE(img->soundtrack[0][0])
  img->soundtrack[0][0]=calloc(STRLEN_SEPARATOR + s + 20 + 3 + 4 + 1, sizeof(char));
  sprintf(img->soundtrack[0][0], "%s"SEPARATOR"%s%u%s", globals->settings.tempdir, "extracted_soundtrack",0, ".wav");

  const char* argsextract[]={extract_ac3, video_vob_path, "-",  "-s", "|",
                             ac3dec, "-o", "wav", "-p", img->soundtrack[0][0], NULL};

  char* cml=get_full_command_line(argsextract, globals);
  errno=system(win32quote((const char*) cml));
  free(cml);
 }

 char framerate[strlen(img->framerate)+3];
 sprintf(framerate, "%s%s", img->framerate, ":1");
 char imported_topmenu[strlen(globals->settings.tempdir)+28+1];
 sprintf(imported_topmenu, "%s%s", globals->settings.tempdir, "/imported_topmenu_video.m2v");

 const char* args[]={mpeg2dec, "-s", "-o", "pgmpipe", quote(video_vob_path),"|",
                     pgmtoy4m, "-i", "p", "-r", framerate, "|", mpeg2enc, "-f", "8", "-o", quote(imported_topmenu),NULL};
 char* cml=get_full_command_line(args, globals);
 errno=system(win32quote((const char*) cml));
 free(cml);

 img->backgroundmpg=calloc(img->nmenus, sizeof(char*));
 FREE(img->backgroundmpg[0])
 img->backgroundmpg[0]=calloc(1+ s + 17+3+ 4+1, sizeof(char));

 sprintf(img->backgroundmpg[0], "%s"SEPARATOR"%s%u%s", globals->settings.tempdir, "background_movie_",0, ".mpg");

 launch_lplex_soundtrack(img, "lpcm", globals);

 const char*  argsmplex[]={mplex, "-f", "8", "-L", "48000:2:16",
                           "-o", img->backgroundmpg[0], imported_topmenu, img->soundtrack[0][0], NULL};
 run(mplex, argsmplex, WAIT, FORK, globals);

}
#endif
