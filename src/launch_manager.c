#if HAVE_CONFIG_H && !defined __CB__
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef __WIN32__
#include <sys/wait.h>
#endif
#include "structures.h"
#include "audio2.h"
#include "ats.h"
#include "atsi.h"
#include "amg.h"
#include "samg.h"
#include "asvs.h"
#include "videoimport.h"
#include "c_utils.h"
#include "ports.h"
#include "auxiliary.h"
#include "commonvars.h"
#include "menu.h"
#include "winport.h"
#include "file_input_parsing.h"
#include "launch_manager.h"

/* Remark on data structures:
 *   - command-line data belong to 'command' structures
 *   - software-level global variables are packed in 'globals' structures */
extern globalData globals;
extern unsigned int startsector;
extern char* INDIR, *OUTDIR, *LOGDIR, *LINKDIR, *WORKDIR, *TEMPDIR;

// getting rid of some arrows
#define files command->files
#define ntracks command->ntracks
#define ngroups command->ngroups
#define nvideolinking_groups command->nvideolinking_groups
#define nplaygroups command->nplaygroups
#define playtitleset command->playtitleset
#define img command->img
#define textable command->textable
#define VTSI_rank command->VTSI_rank
#define maximum_VTSI_rank command->maximum_VTSI_rank

// This function cleans up the list of input files by filtering out non-compliant audio input (format undefined or impossible to correct by other options)
// Group and track numbers are readjusted and shifted down (recursively) when one or more files are rejected

command_t *assign_audio_characteristics(uint8_t* I, command_t *command)
{

    short int i, j, l, delta=0, error=0;
    i=I[0];
    j=I[1];

    // retrieving information as to sound file format

    error=wav_getinfo(&files[i][j]);

    // dealing with format information

    switch (error)
    {
    case AFMT_WAVE:
        if (globals.debugging) foutput("[MSG]  Found WAVE format for %s\n", files[i][j].filename);
        files[i][j].type=AFMT_WAVE;
        j++;
        break;
    case AFMT_WAVE_FIXED:
        if (globals.debugging) foutput("[MSG]  Found WAVE format (fixed) for %s\n", files[i][j].filename);
        files[i][j].type=AFMT_WAVE;
        j++;
        break;
    case AFMT_WAVE_GOOD_HEADER:
        if (globals.debugging) foutput("[MSG]  Found WAVE format (original) for %s\n", files[i][j].filename);
        files[i][j].type=AFMT_WAVE;
        j++;
        break;

#ifndef WITHOUT_FLAC
    case AFMT_FLAC:
        if (globals.debugging) foutput("[MSG]  Found FLAC format for %s\n", files[i][j].filename);
        error=flac_getinfo(&files[i][j]);
        j++;
        break;
#if !defined WITHOUT_OGG
#if HAVE_OGG_FLAC
    case AFMT_OGG_FLAC:
        if (globals.debugging) foutput("[MSG]  Found Ogg FLAC format for %s\n", files[i][j].filename);
        error=flac_getinfo(&files[i][j]);
        j++;
        break;
#endif
#endif
#endif

    case NO_AFMT_FOUND:
        if (globals.debugging) foutput("[ERR]  No compatible format was found for %s\n       Skipping file...\n", files[i][j].filename);

        // House-cleaning rules: getting rid of files with unknown format

        // taking off one track;

        ntracks[i]--;

        // group demotion: if there is no track left in groups, taking off one group

        if (ntracks[i] == 0)
        {
            // taking off one group

            ngroups--;

            // getting out of both loops, check end on inner=end of outer
            if (i == ngroups-nvideolinking_groups)
              {
              	  if (i) return(command);
                  else exit(EXIT_FAILURE);
              }

            // shifting indices for ntracks: all groups have indices decremented, so ntracks[g+1] is now ntracks[g]

            for (l=i; l < ngroups-nvideolinking_groups; l++)
            {
                ntracks[l]=ntracks[l+1];
                if (globals.debugging)
                    foutput("[INF]  Shifting track count for group=%d->%d\n", l+1, l+2);
            }
            // delta is a flag for group demotion
            delta=1;


        }
        // shifting indices for files (two cases: delta=0 for same group track-shifting, delta=1 for group demotion

        for (l=j; l < ntracks[i+delta]; l++)
        {
            // a recursion is unavoidable save for j=last track in group
            int i_shift=i+delta;
            int l_shift=l+1-delta;
            if (globals.debugging)
                foutput("[INF]  Shifting indices for group=%d->%d, track=%d->%d\n", i+1, i_shift+1, l+1, l_shift+1);

            files[i][l]=files[i_shift][l_shift];
        }
        break;
    }

// assigning channel
// if AFMT was found, j will have been incremented earlier
// otherwise it is necessary to reparse again files[i][j] as indices have been shifted

    I[0]=i, I[1]=j;

    _Bool increment_group=(I[1] == ntracks[I[0]]);

    I[1] *= 1-increment_group;
    I[0] += increment_group;

    if (I[0] == ngroups-nvideolinking_groups)

        return command;
// recursion
    if (files[I[0]][I[1]].filename) assign_audio_characteristics(I, command);
    return(command);
}


int launch_manager(command_t *command)

{



    /* sanity check */
    if (command == NULL)
    {
        free_memory(command);
        return(EXIT_SUCCESS);
    }

    errno=0;
    int i, j, error;
    uint8_t indices[2]={0,0};
    uint32_t  last_sector;
    uint64_t totalsize=0;
    uint64_t sector_pointer_VIDEO_TS=0;
    sect sectors;


    // Late initialization
    sectors.amg=SIZE_AMG+globals.text+(globals.topmenu <= TS_VOB_TYPE);
    sectors.samg=SIZE_SAMG;
    sectors.asvs= ((img->count) || (img->stillvob) || (img->active))? SIZE_ASVS : 0;
    sectors.topvob=0;
    sectors.stillvob=0;
    memset(sectors.atsi, 0, sizeof(sectors.atsi));

    uint8_t pathlength=strlen(globals.settings.outdir);
    char audiotsdir[pathlength+10];
    char videotsdir[pathlength+10];
    sprintf(audiotsdir, "%s"SEPARATOR"AUDIO_TS", globals.settings.outdir);

    if (!globals.nooutput) secure_mkdir(audiotsdir, globals.access_rights, DEFAULT);
    errno=0;

    if (globals.videozone)
    {
        STRING_WRITE_CHAR_BUFSIZ(videotsdir, "%s"SEPARATOR"VIDEO_TS", globals.settings.outdir)
        if (!globals.nooutput) secure_mkdir(videotsdir, globals.access_rights, DEFAULT);
        errno=0;
    }

    /* Step 1 - parse all audio files and store the file formats, lengths etc */

    SINGLE_DOTS
    change_directory(globals.settings.workdir);
    assign_audio_characteristics(indices, command);


    foutput("\n%s\n", "DVD Layout:\n\n");
    foutput("%s\n", "Group  Track    Rate Bits  Ch        Length  Filename\n");

    // ngroups does not include copy groups from then on -- nplaygroups are just virtual (no added bytes to disc)
    // number of groups=ngroups+nplaygroups
    // number of audio groups=ngroups-nvideolinking_groups

    uint8_t naudio_groups=ngroups-nvideolinking_groups;
    uint8_t nfiles[naudio_groups];
    uint16_t totntracks=0;
    char singlestar[naudio_groups];
    memset(singlestar, ' ', naudio_groups);
    char joinmark[naudio_groups][99];
    memset(joinmark, ' ', naudio_groups*99);
    _Bool singlestar_flag=0, joinmark_flag=0;
    unsigned int ppadd = 0, approximation;



    for (i=0; i < naudio_groups; i++)
    {
        nfiles[i]=ntracks[i];
        totntracks+=nfiles[i];

        if (files[i][0].single_track)
        {
            /* a star indicates a track into which other tracks (may) have been merged */
            // use nfiles defined above
            singlestar[i]='*';
            singlestar_flag=1;
            ntracks[i]=1;

            uint64_t tl=0;
            int ii;
            for (ii=0;ii<nfiles[i];ii++)
                tl += files[i][ii].PTS_length;

            files[i][0].PTS_length = tl;
            files[i][0].last_sector = files[i][nfiles[i]-1].last_sector;
            if (globals.debugging) foutput("[MSG]  group %d will be single-track\n", i);

        }

        for (j=0; j < nfiles[i];  j++)
        {
            // As files[][] is dynamically allocated with calloc(), 0 values mean command line did not define cga values
            if (files[i][j].cga == 0) files[i][j].cga=cgadef[files[i][j].channels-1];


            if (files[i][0].single_track)
            {

                if ((j) && ((files[i][j].samplerate!=files[i][j-1].samplerate)
                            || (files[i][j].bitspersample!=files[i][j-1].bitspersample)
                            || (files[i][j].channels!=files[i][j-1].channels)
                            || (files[i][j].newtitle)))
                {
                    foutput("[WAR]  File %s (group %d, track %d) cannot be merged\n       into a single track, stopping here...\n", files[i][j].filename, i, j);
                    //nfiles=j+1;
                    break;
                }


                if (j) files[i][0].numsamples+=files[i][j].numsamples;
                //PATCH
            }

            if (files[i][0].contin)
            {
                files[i][j].joingap=1;
                joinmark_flag=1;

                if (j<nfiles[i]-1)
                {
                    files[i][j].contin_track=1;
                    joinmark[i][j]='=';

                }
            }

            files[i][j].rmdr = (files[i][j].numbytes) % files[i][j].sampleunitsize;
            if (files[i][j].padd)  files[i][j].numsamples++;
            files[i][j].PTS_length=(90000.0*files[i][j].numsamples)/files[i][j].samplerate;
            if (j)
            {
                if  (files[i][j-1].rmdr >0)
                {
                    files[i][j].offset = files[i][j-1].rmdr;
                    files[i][j].numsamples=((files[i][j].numbytes + files[i][j].offset)/files[i][j].sampleunitsize)*files[i][j].sampleunitsize/(files[i][j].channels*files[i][j].bitspersample/8);
                    files[i][j].PTS_length=(90000.0*files[i][j].numsamples)/files[i][j].samplerate;
                }
            }


            foutput("%c%c  %d     %02d  %6"PRIu32"   %02d   %d   %10"PRIu64"   ",joinmark[i][j], singlestar[i], i+1, j+1, files[i][j].samplerate, files[i][j].bitspersample, files[i][j].channels, files[i][j].numsamples);
            foutput("%s\n",files[i][j].filename);
            totalsize+=files[i][j].numbytes;

        }

    }


    for (i=0; i < nplaygroups; i++)
    {
        int numfiles=ntracks[playtitleset[i]];

        for (j=0; j < numfiles;  j++)
        {

            foutput("%c%c  %d     %02d  %6"PRIu32"   %02d   %d   %10"PRIu64"   ",'D', singlestar[i], i+1, j+1, files[i][j].samplerate, files[i][j].bitspersample, files[i][j].channels, files[i][j].numsamples);
            foutput("%s\n",files[i][j].filename);

        }
    }

    if (singlestar_flag)
        printf ("\n%s\n", "A star indicates a single-track group.");

    if (joinmark_flag)
        printf ("\n%s\n", "An = sign indicates that this file and the following will be joined.");

    if (nplaygroups)
        printf ("\n%s\n", "a D flag indicates a duplicated group (followed by original group rank).");

    foutput("%c\n", '\n');

    foutput("[MSG]  Size of raw PCM data: %"PRIu64" bytes (%.2f  MB)\n",totalsize, (float) totalsize/(1024*1024));


    /* This approximation was contributed by Lee and Tim feldkamp */

    approximation=275+3*naudio_groups+ppadd;

    /* End of formula */

    switch (startsector)
    {

    case -1:

        startsector=approximation; /* automatic computing of startsector (Lee and Tim Feldman) */
        foutput("[MSG]  Using start sector based on AOBs: %d\n",approximation);
        break;

    case  0:
        startsector=STARTSECTOR; /* default value is 281 (Dave Chapman setting) */
        foutput("%s", "[MSG]  Using default start sector 281\n");
        break;

    default:

        foutput("[MSG]  Using specified start sector %d instead of estimated %d\n",startsector,approximation);
    }

    /* main reference track tables */
    // static allocation with C99, arguably faster and possibly safer than calloc()
    uint8_t   numtitles[naudio_groups];
    uint8_t   *ntitletracks[naudio_groups];
    uint16_t  *ntitlepics[naudio_groups];

    uint64_t  *titlelength[naudio_groups];

    uint16_t totntracks0=create_tracktables(command, naudio_groups,numtitles,ntitletracks,titlelength,ntitlepics);
    if (globals.veryverbose)
    {
	    if (totntracks == totntracks0)
		 foutput("%s\n", "[INF]  Coherence check on total of tracks... OK");
	    else
		printf("[INF]  Total of tracks is not coherent: totntracks=%d, return of create_tracktables=%d\n", totntracks, totntracks0);
    }


    for (i=0; i < naudio_groups; i++)
    {
        error=process_ats(audiotsdir,i+1,&files[i][0], nfiles[i], "wb+");
        ppadd-=error;
        /* Audio zone system file  parameters  */

        error=create_atsi(command, audiotsdir,i,&sectors.atsi[i], &ntitlepics[i][0]);
    }

    /* creating system VOBs */
#if !HAVE_CORE_BUILD
    if (globals.topmenu < NO_MENU)  sectors.topvob=create_topmenu(audiotsdir, command); // if no top menu is requested, but simply active ones, generate matrix top menu and unlink it at the end

    if (img->active)
        {
            if (globals.debugging) foutput("%s", "[INF]  Adding active menu.\n");

            create_activemenu(img);
        }

    if ((img->count) || (img->stillvob) || (img->active))
    {

	if (img->stillpicvobsize == NULL)
	// allocation to be revised
	   img->stillpicvobsize=(uint32_t*) calloc(totntracks, sizeof(uint32_t));

        create_stillpics(
            audiotsdir,
            naudio_groups,
            numtitles,
            ntitlepics,
            img,
            &sectors,
            totntracks);
        if (img->stillvob)
             sectors.stillvob=stat_file_size(img->stillvob)/0x800;  //expressed in sectors
        if (globals.debugging) foutput("[MSG]  Size of AUDIO_SV.VOB is: %u sectors\n" , sectors.stillvob);

    }
#endif





    /* Creating AUDIO_PP.IFO */

    last_sector=create_samg(audiotsdir, command, &sectors);

    /*   sector_pointer_VIDEO_TS= number of sectors for AOBs + 2* sizeof amg + 2* size of ats*ngroups +system vobs +2*sizeof asvs */
#if !HAVE_CORE_BUILD
    sector_pointer_VIDEO_TS= 2*(sectors.amg+sectors.asvs)+sectors.stillvob+sectors.topvob;

    for (i=0; i < naudio_groups; i++)
    {
        for (j=0; j < ntracks[i]; j++)
        {
            sector_pointer_VIDEO_TS+=files[i][j].last_sector - files[i][j].first_sector+1;
        }
        sector_pointer_VIDEO_TS +=2*sectors.atsi[i];
    }

    if (globals.debugging)
    {
        foutput("       Sector pointer to VIDEO_TS from AUDIO_TS= %"PRIu64" sectors\n", sector_pointer_VIDEO_TS);
        foutput( "%s", "[INF]  Checking coherence of pointers...");

        if (sectors.samg + startsector + sector_pointer_VIDEO_TS != last_sector+1+sectors.atsi[naudio_groups-1])
            foutput("\n[WAR]  Pointers to VIDEO_TS are not coherent %"PRIu64" , %"PRIu32"\n",
                   sectors.samg + startsector + sector_pointer_VIDEO_TS, (uint32_t) last_sector+1+sectors.atsi[naudio_groups-1]);
        else
            foutput("%s\n", "    OK");
    }

    foutput("[MSG]  Total size of AUDIO_TS: %"PRIu64" sectors\n", sector_pointer_VIDEO_TS + sectors.samg);

    foutput("[MSG]  Start offset of  VIDEO_TS in ISO file: %"PRIu64" sectors,  offset %"PRIu64"\n\n", sector_pointer_VIDEO_TS + sectors.samg + startsector,
           (sector_pointer_VIDEO_TS + sectors.samg + startsector)*2048);
#endif
    /* Creating AUDIO_TS.IFO */

    uint32_t  relative_sector_pointer_VTSI[nvideolinking_groups];
    uint32_t  videotitlelength[nvideolinking_groups];

    memset(relative_sector_pointer_VTSI, 0, nvideolinking_groups*4);
    memset(videotitlelength, 0, nvideolinking_groups*4);

//  relative_sector_pointer_VTSI=absolute_se+relative_sector_pointer_in_VTS

    /*
    *   Version 200806: added to function create_amg:
    *
    *   int VTSI_rank[N]
    *								VTSI_rank[k]	= rang of k-th video titleset linked to in video zone (< 10)
    *
    *   uint32_t  relative_sector_pointer_VTSI[N]
    *								relative_sector_pointer_VTSI[k] = & VTS_XX_0.IFO -&AUDIO_TS.IFO, expressed in sectors, in which XX=VTSI_rank[k]
    *
    *   uint32_t  videotitlelength[[N]
    *							   videotitlelength[k] = length of title linked to in PTS ticks
    *
    *   N= number of video linking groups in audio zone ( + number of audio groups < 10)
    *
    *
    */

// returns relative_sector_pointer_VTSI and videotitlelength
#if !HAVE_CORE_BUILD
    if (globals.videolinking)
    {

        get_video_system_file_size(globals.settings.linkdir, maximum_VTSI_rank, sector_pointer_VIDEO_TS, relative_sector_pointer_VTSI);
        get_video_PTS_ticks(globals.settings.linkdir, videotitlelength, nvideolinking_groups, VTSI_rank);

        char    newpath[CHAR_BUFSIZ];
        STRING_WRITE_CHAR_BUFSIZ(newpath, "%s%s", globals.settings.outdir, "/VIDEO_TS")
        if (globals.videozone)
            copy_directory(globals.settings.linkdir, newpath, globals.access_rights);
        change_directory(globals.settings.workdir);
    }
#endif

    uint8_t *title[naudio_groups];

    create_amg(
        audiotsdir,
        command,
        &sectors,
        videotitlelength,
        relative_sector_pointer_VTSI,
        numtitles,
        ntitletracks,
        titlelength
        );


    // Lax behaviour

    if (numtitles == NULL)
    {
        foutput("%s\n", "[ERR]  Critical error: failed to generate AUDIO_TS.IFO");
        foutput("%s\n", "[ERR]  Continuing with non-compliant DVD-Audio structure...");
        goto SUMMARY;
    }

    for (i=0; i<naudio_groups; i++)
    {
        if ((title[i]=(uint8_t *) calloc(ntracks[i], 1)) == NULL) perror("[ERR]  title[k]");
        {
            title[i][0]=0;
            for (j=1; j<ntracks[i]; j++)
                title[i][j]=(files[i][j].newtitle)+title[i][j-1];
        }
    }

SUMMARY:

    errno=0;

    foutput("%c\n", '\n');
    foutput("%s\n" , "Group   Title  Track  First Sect   Last Sect  First PTS  PTS length cga\n");

    for (i=0; i <naudio_groups; i++)
    {
        for (j=0; j < ntracks[i]; j++)
        {
            foutput("    %d  %2d /%2d    %2d  %10"PRIu32"  %10"PRIu32"  %10"PRIu64"  ",i+1, title[i][j]+1,numtitles[i], j+1,files[i][j].first_sector,files[i][j].last_sector,files[i][j].first_PTS);
            foutput("%10"PRIu64"  %2d\n",files[i][j].PTS_length, files[i][j].cga);
        }
    }

    foutput("\nTotal number of tracks: %d.\n", totntracks);
    /* freeing */

    foutput("%c", '\n');

    // Crucial, otherwise the ISO file may well be unordered even if AUDIO_TS files are OK after exit
    fflush(NULL);

#if !HAVE_CORE_BUILD
    //
    if (globals.runmkisofs)
    {

        char dvdisopath[CHAR_BUFSIZ];
        memset(dvdisopath, '0', CHAR_BUFSIZ);
        if (globals.settings.dvdisopath == NULL)
        {
            sprintf(dvdisopath, "%s"SEPARATOR"%s", globals.settings.tempdir, "dvd.iso");
        }
        else memcpy(dvdisopath, globals.settings.dvdisopath, CHAR_BUFSIZ) ;

        unlink(dvdisopath);
        uint64_t size;
        char* mkisofs=NULL;
        char *args[]={MKISOFS_BASENAME, "-dvd-audio", "-v", "-o", dvdisopath, globals.settings.outdir, NULL};

        errno=0;
        if ((mkisofs=create_binary_path(mkisofs, MKISOFS, SEPARATOR MKISOFS_BASENAME)))
        {
           foutput("%s\n", "[INF]  Launching mkisofs to create image");
           run(mkisofs, args, 0);
        }
        else
                foutput("%s\n", "[ERR]  Could not access mkisofs binary.");

        FREE(mkisofs);

        size=stat_file_size(dvdisopath)/1024;
        if ((!errno) && (size > 4*SIZE_AMG + 2*SIZE_SAMG +1))  foutput("[MSG]  Image was created with size %llu KB.", size);
        else
            foutput("%s\n", "[ERR]  ISO file creation failed -- fix issue.");

    }


    if (globals.cdrecorddevice)
    {

        char dvdisoinput[CHAR_BUFSIZ];
        memset(dvdisoinput, '0', CHAR_BUFSIZ);
        if (globals.settings.dvdisopath == NULL)
        {
            sprintf(dvdisoinput, "%s"SEPARATOR"%s", globals.settings.tempdir, "dvd.iso");
        }
        else memcpy(dvdisoinput, globals.settings.dvdisopath, CHAR_BUFSIZ) ;
        errno=0;

        if (globals.rungrowisofs)
        {
            foutput("\n%s\n", "[INF]  Launching growisofs to burn disc");
            char string[strlen(globals.cdrecorddevice)+2+strlen(dvdisoinput)];
            sprintf(string, "%s%c%s", globals.cdrecorddevice, '=', dvdisoinput);
            char *args[]={"growisofs", "-Z", string, NULL};
#define GROWISOFS "/usr/bin/growisofs"
            run(GROWISOFS, args, 0);
        }
        else
        {

            char *verbosity=(globals.debugging)? "-v":"-s";
#define TEST (globals.cdrecorddevice[0] != '\0')
            int S=TEST? 8:6;
            char *args[S],
            *args0[]={CDRECORD_BASENAME, verbosity,"blank=fast", "-eject","dev=", globals.cdrecorddevice, dvdisoinput, "-gracetime=", "1",  NULL},
            *args1[]={CDRECORD_BASENAME, verbosity, "blank=fast", "-eject",dvdisoinput, "-gracetime=", "1", NULL};
            if TEST memcpy(args, args0, sizeof(args0));
            else memcpy(args, args1, sizeof(args1));

            errno=0;
            char* cdrecord=NULL;

            if ((cdrecord=create_binary_path(cdrecord, CDRECORD, SEPARATOR CDRECORD_BASENAME)))
            {
               foutput("\n%s\n", "[INF]  Launching cdrecord to burn disc");
               run(cdrecord, args, NOWAIT);
            }
            else
               foutput("%s\n", "[ERR]  Could not access to cdrecord binary.");

            FREE(cdrecord);
        }
    }

    #endif
    // freeing files and heap-allocated globals

#ifndef __WIN32__
    while (waitpid(-1, NULL, 0) >0);
#endif






    for (j=0; j < naudio_groups; j++)
    {
        FREE(ntitletracks[j])
        FREE(titlelength[j])
        FREE(title[j])
    }

    free_memory(command);

    return (errno);
}
#undef files
#undef ntracks
#undef ngroups
#undef vgroups
#undef nplaygroups
#undef playtitleset
#undef img
#undef textable
#undef VTSI_rank
