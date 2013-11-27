/*

File:    amg.c
Purpose: Create an Audio Manager (AUDIO_TS.IFO)

dvda-author  - Author a DVD-Audio DVD

(C) Dave Chapman <dave@dchapman.com> 2005
(C) Revised version with zone-to-zone linking Fabrice Nicol <fabnicol@users.sourceforge.net> 2007, 2008,2013

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
// Place here to #define _GNU_SOURCE before <unistd.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "commonvars.h"
// End of comment
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "errno.h"
#include <sys/types.h>
#ifndef __WIN32__
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <sys/stat.h>


#include "structures.h"
#include "c_utils.h"
#include "auxiliary.h"
#include "amg.h"
#include "winport.h"
#include "xml.h"
#include "menu.h"
#include "launch_manager.h"
#include "asvs.h"

extern globalData globals;
extern char* TEMPDIR;
uint16_t  maxntracks;
uint16_t totaltitles;

/* Limitations */

/* Videolinking groups may have just one videolinking title and chapter per group. The sum of videolinking groups and audio groups
*  should not be greater than 9.
*  Video titlesets linked to may have just one identifiable single-chapter title in them, even if they actually have many titles. */


/* TODO: Dave Chapman's original code implements an implicit "automatic titling" mode: a new title is created sequentially when
*   the next file on the list does not have the same audio characteristics as the latest. This could be made optionally manual, to leave
*   room for choice and not depend on the file ordering, or make several titles within audio that have same characteristics */


/* 'playlist groups' are duplicate groups of audio titles */

#define files command->files
#define ntracks command->ntracks
#define ngroups command->ngroups
#define vgroups command->nvideolinking_groups
#define nplaygroups command->nplaygroups
#define playtitleset command->playtitleset
#define img command->img
#define textable command->textable
#define VTSI_rank command->VTSI_rank


uint16_t create_tracktables(command_t* command, uint8_t naudio_groups, uint8_t ntitles[], uint8_t *ntitletracks[], uint64_t *titlelength[], uint16_t **ntitlepics)
{
    
    /* Normal case: audio files */
    // Files are packed together according to audio characteristics: bit rate, sampel rate, number of channels

   for (uint8_t group=0; group < naudio_groups; group++)
   {

    ntitles[group]=0;

    for  (int track=0; track < ntracks[group]; track++)
    {
            /* counts the number of tracks with same-type audio characteristics, per group and title
            *  into ntitletracks[group][numtitles[group]], and corresponding PTS length in titlelength[group][numtitles[group]] */

            // PATCH 13.11 on 12.06

            if (track)
            {
              if ((files[group][track].samplerate != files[group][track-1].samplerate)
                ||(files[group][track].bitspersample != files[group][track-1].bitspersample)
                ||(files[group][track].channels != files[group][track-1].channels)
                ||(files[group][track].cga != files[group][track-1].cga))

                {
                  files[group][track].newtitle=1;
                }
             }
             else
                files[group][track].newtitle=1;

            // PATCH 02 Dec 09 && 12.06
            if (files[group][track].newtitle)
            {
                totaltitles++;
                ntitles[group]++;
            }
            #ifdef DEBUG
            fprintf(stderr, "files[group][track].newtitle=%d\n",group,track, files[group][track].newtitle);
            #endif
     }
    }
    
    uint8_t track;
  
    for  (int group=0; group < naudio_groups; group++)
    {
      ntitletracks[group]=calloc(ntitles[group], sizeof(uint8_t));
      ntitlepics[group]=calloc(ntitles[group],sizeof(uint64_t));
      titlelength[group]=calloc(ntitles[group],sizeof(uint64_t));
        
      if (titlelength[group] == NULL || ntitlepics[group] == NULL || ntitletracks[group] == NULL)
            EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Memory allocation, title track count in AMG")
                    
     track=0;
      
      for (int title=0; title < ntitles[group]; title++)
      {
        while (ntitletracks[group][title] == 0 || !files[group][track].newtitle)
        {
           ntitletracks[group][title]++;
           titlelength[group][title]+=files[group][track].PTS_length;

            if (img)
            {
                    ntitlepics[group][title] += (img->npics)? img->npics[track]: 1;
            }
  
         track++;
        } 
        
        fprintf(stderr, "ntitletracks[%d][%d]=%d\n", group, title, ntitletracks[group][title]);
      }

      maxntracks=MAX(track, maxntracks); 
      if (globals.debugging)  printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Number of titles for group %d is %d\n",group, ntitles[group] );
      
      if (track  != ntracks[group])
      {
        fprintf(stderr, "\nCounted %d tracks instead of %d\n", track, ntracks[group]);
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Incoherent title count")
      }
           
    }

    return track;

}

#if !HAVE_core_BUILD
void allocate_topmenus(command_t *command)
{

    if (img->topmenu == NULL) img->topmenu=calloc(img->nmenus,sizeof(char *));
    if (img->topmenu == NULL) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  img->topmenu 1\n");
    int menu, s=strlen(globals.settings.tempdir);
    for (menu=0; menu < img->nmenus; menu++)
    {
        if (img->topmenu[menu] == NULL) img->topmenu[menu]=calloc(s+13, sizeof(char));
        if (img->topmenu[menu] == NULL)  perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" img->topmenu is null");
        if (img->topmenu[menu]) snprintf(img->topmenu[menu], s+11, "%s"SEPARATOR"%s%d", globals.settings.tempdir,"topmenu", menu);
    }
    return;
}


uint32_t create_topmenu(char* audiotsdir, command_t* command)
{
    // Here authoring top VOB
    // first generate pics if necessary and background mpg from them
    int menu=0;

    char outfile[strlen(audiotsdir)+14];
    sprintf(outfile, "%s"SEPARATOR"AUDIO_TS.VOB", audiotsdir);
    img->action=ANIMATEDVIDEO;


    switch(globals.topmenu)
    {
         // If only active menus, no top menus, create automatic top menus to be unlinked later on
        // unless some extra info is given (then globals.topmenu < ACTIVE_MENU_ONLY)
    case TEMPORARY_AUTOMATIC_MENU:
    case AUTOMATIC_MENU:
    case RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR :

        // do not overwrite !

        generate_background_mpg(img); // do not break;

    case RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR:

            generate_menu_pics(img, ngroups, ntracks, maxntracks);

        // calling xml project file subroutine for dvdauthor

    case RUN_SPUMUX_DVDAUTHOR:

        allocate_topmenus(command);

        errno=generate_spumux_xml(ngroups, ntracks, maxntracks, img);
        if (errno) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  AMG:spumux_xml\n");
        
        errno=launch_spumux(img);
        if (errno) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  AMG:spumux\n");

    case  RUN_DVDAUTHOR :

        if (!globals.xml)
        {
            if (globals.debugging) foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Generating AMGM Xml project for dvdauthor (patched)...");
            errno=generate_amgm_xml(ngroups, ntracks, img);
            if (errno) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  AMG:amgm_xml\n");
        }
        for (menu=0; menu < img->nmenus; menu++)
            if (img->topmenu[menu])
            {
                if (img->menuvobsize == NULL) img->menuvobsize=calloc(img->nmenus, sizeof(uint32_t*));
                if (img->menuvobsize == NULL) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  menuvobsize\n");

                img->menuvobsize[menu]=stat_file_size(img->topmenu[menu])/0x800;
                if (globals.veryverbose) foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Top menu is: %s with size %"PRIu32" KB\n", img->topmenu[menu], img->menuvobsize[menu]);
            }



        launch_dvdauthor();
        break;

    case TS_VOB_TYPE:
        if (img->menuvobsize == NULL)
            img->menuvobsize=calloc(img->nmenus, sizeof(uint32_t*));
        if (img->menuvobsize == NULL) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  menuvobsize\n");

        img->menuvobsize[0]=stat_file_size(img->tsvob)/(0x800*img->nmenus);
        for (menu=0; menu < img->nmenus; menu++)
        {
            img->menuvobsize[menu]=img->menuvobsize[0];
            if (globals.veryverbose) foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Top menu is: %s with size %d KB\n", outfile,img->menuvobsize[menu]);
        }

        copy_file(img->tsvob, outfile);

        break;



    default:
        foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Incoherence of menu status in create_topmenu");
        exit(EXIT_FAILURE);

        break;
    }

    // launch dvdauthor before create_amg, so that the size of AUDIO_TS.VOB can be assessed dynamically



    fflush(NULL);
    // otherwise the ISO file may well be unordered even if AUDIO_TS files are OK after exit
#ifndef __WIN32__
    sync();
#endif
    uint32_t size=0;

    size=(uint32_t) stat_file_size(outfile)/0x800;
    if (globals.debugging) foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Size of AUDIO_TS.VOB is: %u sectors\n" , size );

    img->tsvob=strdup(outfile);
    return (size); //expressed in sectors
}


int create_stillpics(char* audiotsdir, uint8_t naudio_groups, uint8_t *numtitles, uint16_t **ntitlepics, pic* image, sect* sectors, uint16_t totntracks)
{
    char outfile[strlen(audiotsdir)+14];
    int  k;
    foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Creating ASVS...");
    image->action=STILLPICS;

    if (image->stillvob == NULL)
    {
        generate_background_mpg(image);
        if (image->backgroundmpg == NULL)
        {
            image->backgroundmpg=calloc(1, sizeof(char*));
            image->backgroundmpg[0]=calloc(strlen(TEMPDIR)+15, sizeof(char));
            sprintf(image->backgroundmpg[0], "%s%s", TEMPDIR, "background.mpg");
        }

        image->stillvob=image->backgroundmpg[0];
    }
    else
    {
        uint32_t s=stat_file_size(image->stillvob)/(totntracks*0x800);
        for (k=0; k < totntracks; k++)
        {
            image->stillpicvobsize[k]=s;    // hypothesise VOB made from totntracks pics of same size, with just one pic per track
        }

    }



    if (!image->active)
    {
        STRING_WRITE_CHAR_BUFSIZ(outfile, "%s/AUDIO_SV.VOB", audiotsdir)
        copy_file(image->stillvob, outfile);
        image->stillvob=strdup(outfile);
    }


    fflush(NULL);
#ifndef __WIN32__
    sync();
#endif


    create_asvs(audiotsdir, naudio_groups, numtitles, ntitlepics, sectors->asvs, image);

    STRING_WRITE_CHAR_BUFSIZ(outfile, "%s/AUDIO_SV.IFO", audiotsdir)
    sectors->asvs=stat_file_size(outfile)/0x800;

    return(errno);  //expressed in sectors

}
#endif

uint8_t* create_amg(char* audiotsdir, command_t *command, sect* sectors, uint32_t *videotitlelength, uint32_t* relative_sector_pointer_VTSI,
                    uint8_t *numtitles, uint8_t** ntitletracks, uint64_t** titlelength)
{
    errno=0;


    uint16_t i,j=0,k=0,titleset=0, totalplaylisttitles=0, totalaudiotitles=0, titleintitleset;

    _Bool menusector=(globals.topmenu <= TS_VOB_TYPE);  // there is a _TS.VOB in these cases
    uint8_t naudio_groups=ngroups-vgroups-nplaygroups;  // CHECK

    uint8_t amg[sectors->amg*2048];
    uint32_t  sectoroffset[naudio_groups];

    totalaudiotitles=totaltitles;
    totaltitles+=vgroups;

    if (globals.playlist)
        for (j=0; j < nplaygroups; j++)
            totalplaylisttitles+=numtitles[playtitleset[j]];

    totaltitles+=totalplaylisttitles;

    if (globals.debugging) foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  AMG: totaltitles=%d\n", totaltitles);

    memset(amg,0,sizeof(amg));


    memcpy(amg,"DVDAUDIO-AMG",12);

    uint32_copy(&amg[12], 2*sectors->amg + sectors->topvob- 1);		// Relative sector pointer to Last sector in AMG ie size (AUDIO_TS.IFO+AUDIO_TS.VOB+AUDIO_TS.BUP)-1 in sectors
    uint32_copy(&amg[28], sectors->amg-1);		// Last sector in AMGI
    uint16_copy(&amg[32],0x0012); 	// DVD Specifications
    uint16_copy(&amg[38],0x0001); 	// Number of Volumes
    uint16_copy(&amg[40],0x0001); 	// Current Volume
    amg[42]=1;  		    		// Disc Side
    amg[47]=globals.autoplay;
    if (sectors->stillvob) uint32_copy(&amg[48], 2*sectors->amg+ sectors->topvob);		// relative sector pointer to AUDIO_SV.VOB=same value as 0x when no SV.VOB
    amg[62]=0;  					// Number of AUDIO_TS video titlesets (DVD-Audio norm video titlesets are AOBs)
    amg[63]=ngroups; 		        // Number of audio titlesets, must include video linking groups
    uint32_copy(&amg[128],0x07ff);
    uint32_copy(&amg[0xc0], menusector*sectors->amg);  	// Pointer to sector 2
    uint32_copy(&amg[0xc4],1);  	// Pointer to sector 2
    uint32_copy(&amg[0xc8],2);  	// Pointer to sector 3
    uint32_copy(&amg[0xcc], (menusector)? 3 : 0);  	// Pointer to sector 4
    uint32_copy(&amg[0xd4], (globals.text)? 3+(menusector) : 0);  	// Pointer to sector 4 or 5
    uint32_copy(&amg[0x100], (menusector)? 0x53000000 :0); // Unknown;

    uint32_copy(&amg[0x154], (menusector)? 0x00010000:0); // Unknown;

    uint32_copy(&amg[0x15C], (img->h + img->min +img->sec)? 0x00018001:0); // 2ch 48k LPCM audio (signed big endian) in mpeg2 top menu, 1 stream,


    /* Sector 2 */

    i=0x800;
    uint16_copy(&amg[i],totaltitles);		// total number of titles, audio and videolinking titles included
    i+=2;

    // pointer to end of sector table : 4 (bytes used) + 14 (size of table) *number of tables (totaltitles) -1
    uint16_copy(&amg[i],4+14*totaltitles-1);
    i+=2;

    sectoroffset[0]=2*(sectors->amg + sectors->asvs)+sectors->stillvob+sectors->topvob; 								 // Pointer to first ATS_XX_0.IFO

    titleset=0;
    titleintitleset=0;

    // Normal case: audio titles



    for (j=0; j < totalaudiotitles; j++)
    {

        _Bool come_last=(titleintitleset==numtitles[titleset]-1);

        amg[i]=((menusector)? ((come_last)? 0xC0 : 0x80) : 0x80 )|(titleset+1); 			// Table sector 2 first two bytes per title
        amg[++i]=ntitletracks[titleset][titleintitleset];
        i+=3; // 0-padding
        uint32_copy(&amg[i],titlelength[titleset][titleintitleset]);
        i+=4;
        amg[i]=titleset+1;  // Titleset number
        amg[++i]=titleintitleset+1;

        uint32_copy(&amg[++i],sectoroffset[titleset]); // Pointer to ATSI


        i+=4;
        titleintitleset++;
        if (titleintitleset == numtitles[titleset])
        {
            sectoroffset[titleset+1]=sectoroffset[titleset]+(files[titleset][ntracks[titleset]-1].last_sector+1)+sectors->atsi[titleset]*2;
            if (globals.veryverbose)
            {
                if (titleset == 0)
                    foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  sectoroffset[%d]=%u=2*(%d+%d)+%u+%u\n", titleset, sectoroffset[titleset], sectors->amg , sectors->asvs, sectors->stillvob, sectors->topvob);
                foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  sectoroffset[%d]=%u=sectoroffset[%d]+(files[%d][ntracks[%d]-1].last_sector+1)+2*%d\n", titleset+1, sectoroffset[titleset+1], titleset, titleset, titleset, sectors->atsi[titleset]);
            }

            titleintitleset=0;
            titleset++;
        }


    }



    // Case 2: video-linking titles

    // supposing one title per videolinking group

    if (globals.videolinking)
    {
        for (k=0; k < vgroups ; k++)
        {

            /* if (k +1 == #number of videotitles in group)
                amg[i]=0x00|(titleset+1);                  // Table sector 2 first two bytes per title, non-last video linking title
            else
            */
            amg[i]=0x40|(++titleset);                  // Table sector 2 first two bytes per title, last video linking title
            amg[++i]=1;                                        // Experiment limitation: just one video chapter is visible within video zone title
            amg[++i]=1;                                        // Experiment limitation: just one video chapter is visible within video zone title (repeated)
            i++;
            uint32_copy(&amg[++i],  videotitlelength[k]);        //  length of title in PTS ticks
            i+=4;
            amg[i]=VTSI_rank[k];                          // Video zone Titleset number
            amg[++i]=1;                                        // Experiment limitation: just one video title is visible within video zone titleset
            uint32_copy(&amg[++i], relative_sector_pointer_VTSI[VTSI_rank[k]-1]);       // Pointer to VTSI
            i+=4;
        }
    }

    if (globals.playlist)
    {
        //copy of other groups

        for (j=0; j < nplaygroups; j++)
        {
            if (globals.debugging) foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Encoding copy group (#%d)\n", j+1);

            for (k=0; k < numtitles[playtitleset[j]]; k++)
            {
                amg[i]=0x80|(playtitleset[j]+1); 			// Table sector 2 first two bytes per title
                amg[++i]=ntitletracks[playtitleset[j]][k];
                i+=3; // 0-padding
                uint32_copy(&amg[i],titlelength[playtitleset[j]][k]);
                i+=4;
                amg[i]=playtitleset[j]+1;  // Titleset number
                amg[++i]=k+1;
                uint32_copy(&amg[++i], sectoroffset[playtitleset[j]]); // Pointer to ATSI
                i+=4;
            }
        }
    }

    /* Sector 3 */



    i=0x1000;
    uint16_copy(&amg[i],totaltitles);		// total number of titles, audio and videolinking titles included
    i+=2;
    // pointer to end of sector table : 4 (bytes used) + 14 (size of table) *number of tables (totaltitles) -1
    uint16_copy(&amg[i],4+14*totaltitles-1);
    i+=2;
    titleset=0;
    titleintitleset=0;

    // Normal case: audio titles

    for (j=0; j < totalaudiotitles; j++)
    {


        amg[i]= 0x80 |(titleset+1); 			// Table sector 2 first two bytes per title
        amg[++i]=ntitletracks[titleset][titleintitleset];
        i+=3; // 0-padding
        uint32_copy(&amg[i],titlelength[titleset][titleintitleset]);
        i+=4;
        amg[i]=titleset+1;  // Titleset number
        amg[++i]=titleintitleset+1;

        uint32_copy(&amg[++i],sectoroffset[titleset]); // Pointer to ATSI

        i+=4;
        titleintitleset++;
        if (titleintitleset == numtitles[titleset])
        {
            titleintitleset=0;
            titleset++;
        }
    }



    // Case 2: video-linking titles

    // supposing one title per videolinking group

    if (globals.videolinking)
    {
        for (k=0; k < vgroups ; k++)
        {

            /* if (k +1 == #number of videotitles in group)
                amg[i]=0x00|(titleset+1);                  // Table sector 2 first two bytes per title, non-last video linking title
            else

            */
            amg[i]=0x40|(++titleset);                  // Table sector 2 first two bytes per title, last video linking title
            amg[++i]=1;                                        // Experiment limitation: just one video chapter is visible within video zone title
            amg[++i]=1;                                        // Experiment limitation: just one video chapter is visible within video zone title (repeated)
            i++;
            uint32_copy(&amg[++i],  videotitlelength[k]);        //  length of title in PTS ticks
            i+=4;
            amg[i]=VTSI_rank[k];                          // Video zone Titleset number
            amg[++i]=1;                                        // Experiment limitation: just one video title is visible within video zone titleset
            uint32_copy(&amg[++i], relative_sector_pointer_VTSI[VTSI_rank[k]-1]);       // Pointer to VTSI
            i+=4;
        }
    }

    if (globals.playlist)
    {
        //copy of other groups

        for (j=0; j < nplaygroups; j++)
        {
            if (globals.debugging) foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Encoding copy group (#%d)\n", j+1);

            for (k=0; k < numtitles[playtitleset[j]]; k++)
            {
                amg[i]=0x80|(playtitleset[j]+1); 			// Table sector 2 first two bytes per title
                amg[++i]=ntitletracks[playtitleset[j]][k];
                i+=3; // 0-padding
                uint32_copy(&amg[i],titlelength[playtitleset[j]][k]);
                i+=4;
                amg[i]=playtitleset[j]+1;  // Titleset number
                amg[++i]=k+1;
                uint32_copy(&amg[++i], sectoroffset[playtitleset[j]]); // Pointer to ATSI
                i+=4;
            }
        }
    }

    /* Sector 4 */

// Next is generated only if there is a top menu, not if just still pics.


    if (menusector)
    {
        if (globals.debugging) foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Creating menu ifo, AUDIO_TS.IFO sector 4");
        uint64_t menuvobsize_sum=0;

        /* Looks like VMG_PGCI_UT */

        uint32_copy(&amg[0x1800], 0x00010000); // 0-3 Number of language units + 2 bytes of padding
        uint32_copy(&amg[0x1804], 0x00000151+(img->nmenus-1)*0x13A); // 4-7 Address of last byte= length of table-1

        uint32_copy(&amg[0x1808], 0x656E0080); // 'en' for English, cound add 'g'=0x67 as third byte and 0x80 for existence of menu.
        //  compare with VIDEO_TS.IFO, same bytes.
        uint32_copy(&amg[0x180C], 0x00000010); // relative pointer to start of language unit=0x1810

        /* Looks like Language Unit, LU */

        uint16_copy(&amg[0x1810], img->nmenus);
        uint32_copy(&amg[0x1814], 0x00000141+(img->nmenus-1)*0x13A);
        uint32_copy(&amg[0x1818], 0x82000000); // Language Unit: Menu type: Title menu + 3 padding bytes

        uint32_copy(&amg[0x181C], 8*(img->nmenus+1));
        // INterpretation of palette may differ if spumux/dvdauthor was not fed with appropriate "highlighted" background.
        /* Looks like amended PGC */

        // Prerequisites : display is black and whiten simple text. Highlight is black and white,
        //  with necessary highlighting form (underline, square...) in a third colour (which is not relevant).
        // Select action picture must not be equal to display and it is advised to have a background similar to that of img->background (eg both black).

        i=0x1820;

        for (j=0; j < img->nmenus-1; j++)
        {
            i+=4; // 4 bytes of padding
            uint32_copy(&amg[i], 0x13A*(j+1)+ (img->nmenus-j)*0x8);
            i+=4;

        }



        // to visualize a video, it is necessary to have a transparent set of pics (alpha channel activated) except for text/highlight motifs.
        // --colors commands picture authoring. --palette command real display on screen as --colors are only there to facilitate dvdauthor's work.
        // so basically they may be left as default values, whilst --palette allows user specification

        for (j=1; j <= img->nmenus; j++)
        {

            uint32_copy(&amg[i], 0x00000101); // cf PGC cell data structure // reserved: normal cell (0000) + 1s of still time + 1 post command
            i+=4;
            amg[i++]=BCD_REVERSE(img->h);       //  It should be possible to automate this either by generating a temporary dvd-video-like ifo with dvdauthor and the using videoimport.c, or by retrieving the pointers to values by return of dvdauthor (harder). == TODO ==
            amg[i++]=BCD_REVERSE(img->min);
            amg[i++]=BCD_REVERSE(img->sec);
            amg[i]=0xC1; // hour, minute, second, frames in BCD. The frame part will be ignored (C1 seems to be ok for PAL)
            i+=5;
            amg[i]=(img->sec+img->min+img->h)? 0x80 : 0;  // Audio stream status : 0x80 for present, other reservedn except for bits 0 to 2 (stream number).
            i+=0x10;
            uint32_copy(&amg[i], 0x80000000); // additional bits mask the menu.
            i+=0x80;


            if (j < img->nmenus) uint16_copy(&amg[i], j+1);
            i+=2;
            if (j > 1) uint16_copy(&amg[i], j-1);
            i+=2;
            uint16_copy(&amg[i], 1);
            i+=4; // 2 bytes of padding
            uint32_copy(&amg[i], (uint32_t) strtoul(img->textcolor_palette,NULL,16)); // highlight motif colour
            i+=4;
            uint32_copy(&amg[i], (uint32_t) strtoul(img->highlightcolor_palette,NULL,16)); // Colors (a, Y, Cr, Cb): Pattern:: display background (black)    Type 1 (hier level 1)
            i+=4;
            uint32_copy(&amg[i], (uint32_t) strtoul(img->selectfgcolor_palette, NULL, 16)); // Colors (a, Y, Cr, Cb): Pattern:: display foreground(red)
            i+=4;
            uint32_copy(&amg[i], (uint32_t) strtoul(img->bgcolor_palette,NULL,16)); // Colors (a, Y, Cr, Cb): Pattern:: select (action) foreground (green)
            i+=4;
            // other possible streams (lower hierarchical levels)
            uint32_copy(&amg[i], 0x00108080);
            i+=4;
            uint32_copy(&amg[i], 0x00108080); // Colors (a, Y, Cr, Cb)
            i+=4;
            uint32_copy(&amg[i], 0x00108080); // Colors (a, Y, Cr, Cb)
            i+=4;
            uint32_copy(&amg[i], 0x00108080); // Colors (a, Y, Cr, Cb)
            i+=4;
            uint32_copy(&amg[i], 0x00108080);
            i+=4;
            uint32_copy(&amg[i], 0x00108080); // Colors (a, Y, Cr, Cb)   3rd level
            i+=4;
            uint32_copy(&amg[i], 0x00108080); // Colors (a, Y, Cr, Cb)
            i+=4;
            uint32_copy(&amg[i], 0x00108080); // Colors (a, Y, Cr, Cb)
            i+=4;
            uint32_copy(&amg[i], 0x00108080);  // black
            i+=4;
            uint32_copy(&amg[i], 0x00108080);
            i+=4;
            uint32_copy(&amg[i], 0x00108080);
            i+=4;
            uint32_copy(&amg[i], 0x00108080);  // Unknown
            i+=4;
            uint32_copy(&amg[i], 0x00EC0114);
            i+=4;
            uint32_copy(&amg[i], 0x0116012E);
            i+=4;
            uint32_copy(&amg[i], 0x00010003);
            i+=4;
            uint32_copy(&amg[i], 0x00000027);
            if (img->loop)
            {
                uint16_copy(&amg[i+0xC], 0x2004);
                amg[i+7]=1;
            }
            i+=0x24;
            uint32_copy(&amg[i], 0x01000200);
            i+=4;
            if (img->loop) uint16_copy(&amg[i], 0x0001);
            else uint16_copy(&amg[i], 0xFF00);
            i+=2;
            amg[i++]=BCD_REVERSE(img->h);
            amg[i++]=BCD_REVERSE(img->min);
            amg[i++]=BCD_REVERSE(img->sec);
            amg[i++]=0xC1;
            if (j > 1)
            {
                menuvobsize_sum+=img->menuvobsize[j-2]-1;
                uint32_copy(&amg[i], menuvobsize_sum); // in the course of processing dvdauthor over topmenus, one data sector added by spumux is lost
                i+=8; // 4 bytes of padding
                uint32_copy(&amg[i], menuvobsize_sum);  // repeat
                i+=4;
            }
            else i+=12;
            uint32_copy(&amg[i], menuvobsize_sum+img->menuvobsize[img->nmenus-1]-1-1);
            i+=4;
            uint32_copy(&amg[i], 0x00010001);
            i+=4;
        }

        /* coherence test */

        if (globals.veryverbose)
        {
            if (sectors->topvob == menuvobsize_sum+img->nmenus-1+img->menuvobsize[img->nmenus-1]-1) foutput("%s", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Menu vob size coherence test...OK\n");
            else foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Menu vob size coherence test failed: sectors->topvob=%u against %llu\n", sectors->topvob, menuvobsize_sum+img->nmenus-1+img->menuvobsize[img->nmenus-1]-1);
        }



    }

    /* Sector 5 */

    /* partial implementation: is only valid for one group and type of content="Music content", code 0x38. For "song" replace 0x38 with 0x40 below*/

    if ((globals.text)&&(naudio_groups==1))
    {
        if (globals.debugging) foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Creating DVDATXTDT-MG, AUDIO_TS.IFO");
        int c,d;
        d=(sectors->amg-1)*0x800;
        i=d;
        memcpy(&amg[i], "DVDATXTDT-MG", 12);
        i+=12;
        uint32_copy(&amg[i], 1);
        i+=8;
        memcpy(&amg[i], "en",2);
        i+=3;
        amg[i]=0x11;
        i+=4;
        amg[i]=0x1C;
        i+=6;
        amg[i]=0x2A;
        i+=2;
        amg[i]=0x32;
        i=d+0x35;
        amg[i]=0x36+0x8*ntracks[0];
        i=d+0x47;
        amg[i]=0x03+4*ntracks[0];
        i+=3;
        amg[i]=1;
        i+=4;
        amg[i]=2;
        i+=4;
        amg[i]=3;
        i+=4;
        amg[i]=0x38;
        i+=3;
        c=amg[i]=(ntracks[0]+1)*0x10;
        i++;
        amg[i]=0x3;
        i+=4;
        amg[i]=0x38;
        i-=5;
        int trackindex=0;
        uint8_t lcount=c;

        while(trackindex < 2)
        {
            if (textable[trackindex]!= NULL)
                lcount+=strlen(textable[trackindex])+1;
            if (trackindex < ntracks[0]-2)
            {
                i+=8;
                amg[i+1]=0x3;
                amg[i+5]=0x38;
            }
            else if (trackindex == (ntracks[0]-2))
            {
                i+=8;
                amg[i+1]=0x2;
                amg[i+5]=0x3;
                amg[i+9]=0x38;
            }
            else
            {
                i+=0xC;
                amg[i+1]=0x3;
                amg[i+5]=0x38;
            }
            amg[i]=lcount;
            trackindex++;
        }

        for(trackindex=0; trackindex < ntracks[0]-1; trackindex++)
        {
            i+=8;
            if (textable[trackindex]) lcount+=strlen(textable[trackindex])+1;
            amg[i]=lcount;
            if (trackindex < ntracks[0]-2)
            {
                amg[i+1]=0x3;
                amg[i+5]=0x38;
            }
        }
        i++;
        for (trackindex=0; trackindex < ntracks[0]; trackindex++)
        {
            c=(textable[trackindex]) ? strlen(textable[trackindex]) : 0;
            if (c) memcpy(&amg[i], textable[trackindex], c);
            i+=c;
            amg[i]=0x9;
            i++;
        }
        amg[d+0x13]=i-1;
        amg[d+0x1F]=i-1-0x1C;

    }

    errno=0;

    size_t  sizeofamg=sizeof(amg);

    create_file(audiotsdir, "AUDIO_TS.IFO", amg, sizeofamg);

    create_file(audiotsdir, "AUDIO_TS.BUP", amg, sizeofamg);


    if (errno)
        return(NULL);
    else return numtitles;
#undef files
#undef ntracks
#undef ngroups
#undef vgroups
#undef nplaygroups
#undef playtitleset
#undef img
#undef textable
#undef VTSI_rank

}

