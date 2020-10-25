#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "structures.h"
#include "audio2.h"
#include "ats.h"
#include "atsi.h"
#include "amg.h"
#include "samg.h"
#include "asvs.h"
#include "c_utils.h"
#include "ports.h"
#include "auxiliary.h"
#include "commonvars.h"
#include "menu.h"
#include "winport.h"
#include "file_input_parsing.h"
#include "launch_manager.h"
#include "videoimport.h"
#include "mlp.h"

/* Remark on data structures:
 *   - command-line data belong to 'command' structures
 *   - software-level global variables are packed in 'globals' structures */


extern unsigned int startsector;
extern char* INDIR, *OUTDIR, *LOGDIR, *LINKDIR, *WORKDIR;
extern uint8_t wav2cga_channels(fileinfo_t*,globalData*);
static const char* cga_define[21] = {"Mono",
                                     "L-R",
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

int launch_manager(command_t *command, globalData *globals)

{
    /* sanity check */
    if (command == NULL)
    {
        free_memory(command, globals);
        return(EXIT_SUCCESS);
    }

    errno = 0;
    int i = 0, j = 0;
    int nb_aob_files = 0;

    uint32_t  last_sector;
    uint64_t totalsize=0;
    uint64_t sector_pointer_VIDEO_TS=0;
    sect sectors;


    // Late initialization
    sectors.amg=SIZE_AMG+globals->text+(globals->topmenu <= TS_VOB_TYPE);
    sectors.samg=SIZE_SAMG;
    sectors.asvs= ((img->count) || (img->stillvob) || (img->active))? SIZE_ASVS : 0;
    sectors.topvob=0;
    sectors.stillvob=0;
    memset(sectors.atsi, 0, sizeof(sectors.atsi));

    uint8_t pathlength=strlen(globals->settings.outdir);

    char audiotsdir[pathlength+10];
    char videotsdir[pathlength+10];

    sprintf(audiotsdir, "%s"SEPARATOR"AUDIO_TS", globals->settings.outdir);

    if (!globals->nooutput) secure_mkdir(audiotsdir, globals->access_rights, globals);
    errno=0;
    STRING_WRITE_CHAR_BUFSIZ(videotsdir, "%s"SEPARATOR"VIDEO_TS", globals->settings.outdir)
    if (globals->videozone && !globals->nooutput) secure_mkdir(videotsdir, globals->access_rights, globals);
        errno=0;

    /* Step 1 - parse all audio files and store the file formats, lengths etc */

    SINGLE_DOTS
    change_directory(globals->settings.workdir, globals);

    foutput("\n%s", "DVD Layout\n");
    foutput("%s\n",ANSI_COLOR_BLUE"Group"ANSI_COLOR_GREEN"  Track    "ANSI_COLOR_YELLOW"Rate"ANSI_COLOR_RED" Bits"ANSI_COLOR_RESET"  Ch  CGA    N_Samples  Filename\n");

    // ngroups does not include copy groups from then on -- nplaygroups are just virtual (no added bytes to disc)
    // number of groups=ngroups+nplaygroups
    // number of audio groups=ngroups-nvideolinking_groups

    uint8_t naudio_groups = ngroups-nvideolinking_groups;
    uint8_t nfiles[naudio_groups];
    uint16_t totntracks = 0;
    char singlestar[naudio_groups];
    memset(singlestar, ' ', naudio_groups);
    char joinmark[naudio_groups][99];
    memset(joinmark, ' ', naudio_groups * 99);
    bool singlestar_flag = 0, joinmark_flag = 0;

    for (i = 0; i < naudio_groups; ++i)
    {
        nfiles[i] = ntracks[i];
        totntracks += nfiles[i];

        for (j = 0; j < nfiles[i];  ++j)
        {
            // As files[][] is dynamically allocated with calloc(), 0 values mean command line did not define cga values

            if (files[i][j].cga == 0 || files[i][j].cga == 0xFF)  // non-assigned (calloc 0 value) or assigned with illegal value previously detected as such (0xFF)
                files[i][j].cga = wav2cga_channels(&files[i][j], globals);

            files[i][j].contin_track = (uint8_t) (j != nfiles[i] - 1);

            // MLP necessary for 5+/24/88200+ and 3+/16+/176400+

            if (files[i][j].type != AFMT_MLP)
            {
                if (globals->encode_to_mlp_dvd)
                {
                     if (files[i][j].channels > 2 && files[i][j].samplerate > 96000)
                     {
                        foutput("%s %s %s %d %s %d %s %d %s\n",
                              ANSI_COLOR_RED "[ERR] Surround file ",
                              files[i][j].filename,
                              " cannot be recorded to DVD-Audio even with MLP encoding (",
                              files[i][j].channels,
                              " channels, ",
                              files[i][j].bitspersample,
                              " bits, ",
                              files[i][j].samplerate,
                              " samples per second.)");
                        clean_exit(-1, globals);
                     }
                }
                else
                 if
                (((files[i][j].samplerate > 48000    && files[i][j].bitspersample == 24)
                  || files[i][j].samplerate > 96000)
                 && (files[i][j].channels == 5 || files[i][j].channels == 6)
                )
                {
                  foutput("%s %s %s %d %s %d %s %d %s\n",
                          ANSI_COLOR_RED "[ERR] File ",
                          files[i][j].filename,
                          " cannot be recorded to DVD-Audio without MLP encoding (",
                          files[i][j].channels,
                          " channels, ",
                          files[i][j].bitspersample,
                          " bits, ",
                          files[i][j].samplerate,
                          " samples per second.)");
                  clean_exit(-1, globals);
                }
            }
            else
            {
                if (files[i][j].channels > 2 && files[i][j].samplerate > 96000)
                {
                  foutput("%s %s %s %d %s %d %s %d %s\n",
                          ANSI_COLOR_RED "[ERR] Surround file ",
                          files[i][j].filename,
                          " cannot be recorded to DVD-Audio even with MLP encoding (",
                          files[i][j].channels,
                          " channels, ",
                          files[i][j].bitspersample,
                          " bits, ",
                          files[i][j].samplerate,
                          " samples per second.)");
                  clean_exit(-1, globals);
                }
            }

            totalsize += files[i][j].numbytes;
        }
    }

    for (int i = 0; i < naudio_groups ; ++i)
        for (int j = 0; j < nfiles[i]; ++j)
        {
            if (globals->to_mlp) encode_mlp_file(&files[i][j], globals);
            if (globals->encode_to_mlp_dvd) transport_to_mlp(&files[i][j], globals);
        }

    foutput("\n%s", "DVD MLP Layout\n");
    foutput("%s\n",ANSI_COLOR_BLUE"Group"ANSI_COLOR_GREEN"  Track    "ANSI_COLOR_YELLOW"Rate"ANSI_COLOR_RED" Bits"ANSI_COLOR_RESET"  Ch  CGA        N_Samples  Status  Filename\n");

    for (int i = 0; i < naudio_groups ; ++i)
        for (int j = 0; j < nfiles[i]; ++j)
        {
          if (globals->to_mlp)
            {
                foutput("%c%c  " ANSI_COLOR_BLUE "%d     " ANSI_COLOR_GREEN "%02d" ANSI_COLOR_YELLOW "  %6" PRIu32 "   " ANSI_COLOR_RED "%02d" ANSI_COLOR_RESET "   %d %s %s",
                    joinmark[i][j],
                    singlestar[i],
                    i + 1,
                    j + 1,
                    files[i][j].samplerate,
                    files[i][j].bitspersample,
                    files[i][j].channels,
                    files[i][j].cga < 21 ? cga_define[files[i][j].cga] : "Unknown",
                    "   converted");

                foutput("  To file %s\n", files[i][j].mlp_filename);
            }

            char* MLP_ENCODED = globals->encode_to_mlp_dvd ? "  MLPDVD" : "        ";

            foutput("%c%c  " ANSI_COLOR_BLUE "%d     " ANSI_COLOR_GREEN "%02d" ANSI_COLOR_YELLOW "  %6" PRIu32 "   " ANSI_COLOR_RED "%02d" ANSI_COLOR_RESET "   %d %s   %10" PRIu64 " %s ",
                    joinmark[i][j],
                    singlestar[i],
                    i + 1,
                    j + 1,
                    files[i][j].samplerate,
                    files[i][j].bitspersample,
                    files[i][j].channels,
                    files[i][j].cga < 21 ? cga_define[files[i][j].cga] : "Unknown",
                    files[i][j].numsamples,
                    MLP_ENCODED);

            foutput("%s\n", files[i][j].filename);
        }


    for (i = 0; i < nplaygroups; ++i)
    {
        int numfiles = ntracks[playtitleset[i]];

        for (j = 0; j < numfiles;  ++j)
        {
            foutput("%c%c  %d     %02d  %6"PRIu32"   %02d   %d   %10"PRIu64"   ",'D', singlestar[i], i + 1, j + 1, files[i][j].samplerate, files[i][j].bitspersample, files[i][j].channels, files[i][j].numsamples);
            foutput("%s\n",files[i][j].filename);
        }
    }

    if (singlestar_flag)
        foutput("\n%s\n", "A star indicates a single-track group.");

    if (joinmark_flag)
        foutput("\n%s\n", "An = sign indicates that this file and the following will be joined.");

    if (nplaygroups)
        foutput("\n%s\n", "a D flag indicates a duplicated group (followed by original group rank).");

    foutput("%c\n", '\n');

    foutput(MSG_TAG "Size of raw PCM data: %"PRIu64" bytes (%.2f  MB)\n",totalsize, (float) totalsize/(1024*1024));

    // These are Schily values for mkisofs. However little seems to hinge on this for practical purposes.

    startsector = STARTSECTOR;

    /* main reference track tables */
    // static allocation with C99, arguably faster and possibly safer than calloc()
    uint8_t   numtitles[naudio_groups];
    uint8_t   *ntitletracks[naudio_groups];
    uint16_t  *ntitlepics[naudio_groups];

    uint64_t  *titlelength[naudio_groups];

    uint16_t totntracks0 = create_tracktables(command, naudio_groups,numtitles,ntitletracks,titlelength,ntitlepics, globals);

    if (globals->veryverbose)
    {
        if (totntracks == totntracks0)
         foutput("%s\n", INF "Coherence check on total of tracks... OK");
        else
        foutput(INF "Total of tracks is not coherent: totntracks=%d, return of create_tracktables=%d\n", totntracks, totntracks0);
    }

    int nb_atsi_files = 0;

    for (i = 0; i < naudio_groups; ++i)
    {
        nb_aob_files += create_ats(audiotsdir, i + 1, &files[i][0], nfiles[i], globals);

        /* Audio zone system file  parameters  */

        nb_atsi_files += create_atsi(command, audiotsdir,i,&sectors.atsi[i], &ntitlepics[i][0], globals);
    }

    int nb_asv_files = 0;

    /* creating system VOBs */
#if !HAVE_core_BUILD
    if (globals->topmenu < NO_MENU)
        sectors.topvob = create_topmenu(audiotsdir, command, globals); // if no top menu is requested, but simply active ones, generate matrix top menu and unlink it at the end

    if (sectors.topvob == 0) globals->topmenu = NO_MENU;

    if (img->active)
        {
            if (globals->debugging) foutput("%s", INF "Adding active menu.\n");

            create_activemenu(img, globals);
            if (globals->topmenu == TEMPORARY_AUTOMATIC_MENU)
                sectors.topvob = 0;  //  deleting AUDIO_TS.VOB in this case (just used for creating AUDIO_SV.VOB
        }


    if (img->count || img->stillvob || img->active)
    {

    if (img->stillpicvobsize == NULL)
    // TODO: allocation to be revised
       img->stillpicvobsize=(uint32_t*) calloc(totntracks, sizeof(uint32_t));

     nb_asv_files =
        create_stillpics(
            audiotsdir,
            naudio_groups,
            numtitles,
            ntitlepics,
            img,
            &sectors,
            totntracks,
            globals);


     if (nb_asv_files)
     {
      if (img->stillvob)
             sectors.stillvob=stat_file_size(img->stillvob)/0x800;  //expressed in sectors
           if (globals->debugging) foutput(MSG_TAG "Size of AUDIO_SV.VOB is: %u sectors\n" , sectors.stillvob);
     }

    }
#endif

    if (globals->videozone)
    {
        copy_directory(globals->settings.linkdir, videotsdir, globals->access_rights, globals);
        int nb_video_files = 0;
        if ((nb_video_files  = count_dir_files(&videotsdir[0], globals)) != 0)
        {
          startsector += nb_video_files;
          globals->settings.linkdir = strdup(videotsdir);
        }
        else
        {
          foutput("%s%s%s\n", MSG_TAG "Could not count number of files in ", videotsdir, "\n" MSG_TAG "Disabling VIDEO_TS import and videolinking...");
          ngroups -= nvideolinking_groups;
          nvideolinking_groups = 0;
          globals->videozone = 0;
          globals->videolinking = 0;
        }
    }

    // Starting at 272 (Schily's padding sector count) and increasing count with number of files (system files, plus data files including video files)

    startsector += 3  // AUDIO_TS.IFO + AUDIO_TS.BUP + SAMG, behavior is made stricter below for create_amg
                + (img->tsvob != NULL && sectors.topvob != 0)   // AUDIO_TS.VOB
                + nb_asv_files // AUDIO_SV.VOB + AUDIO_SV.IFO + AUDIO_SV.BUP
                + nb_aob_files + nb_atsi_files; // number of ATS_XX_0.IFO + ATS_XX_0.BUP system files

    // Now starsector iw worth 272 + total number of AOB, VOB and IFO files.

    /* Creating AUDIO_PP.IFO */

    last_sector = create_samg(audiotsdir, command, &sectors, globals);

    /*   sector_pointer_VIDEO_TS= number of sectors for AOBs + 2* sizeof amg + 2* size of ats*ngroups +system vobs +2*sizeof asvs */
#if !defined HAVE_core_BUILD || !HAVE_core_BUILD
    sector_pointer_VIDEO_TS= 2*(sectors.amg+sectors.asvs)+sectors.stillvob+sectors.topvob;

    for (i=0; i < naudio_groups; i++)
    {
        for (j=0; j < ntracks[i]; j++)
        {
            sector_pointer_VIDEO_TS+=files[i][j].last_sector - files[i][j].first_sector+1;
        }
        sector_pointer_VIDEO_TS +=2*sectors.atsi[i];
    }

    if (globals->debugging)
    {
        foutput("       Sector pointer to VIDEO_TS from AUDIO_TS= %"PRIu64" sectors\n", sector_pointer_VIDEO_TS);
        foutput( "%s", INF "Checking coherence of pointers...");

        if (sectors.samg + startsector + sector_pointer_VIDEO_TS != last_sector+1+sectors.atsi[naudio_groups-1])
            foutput("\n"WAR "Pointers to VIDEO_TS are not coherent %"PRIu64" , %"PRIu32"\n",
                   sectors.samg + startsector + sector_pointer_VIDEO_TS, (uint32_t) last_sector+1+sectors.atsi[naudio_groups-1]);
        else
            foutput("%s\n", "    OK");
    }

    foutput(MSG_TAG "Total size of AUDIO_TS: %"PRIu64" sectors\n", sector_pointer_VIDEO_TS + sectors.samg);

    foutput(MSG_TAG "Start offset of  VIDEO_TS in ISO file: %"PRIu64" sectors,  offset %"PRIu64"\n\n", sector_pointer_VIDEO_TS + sectors.samg + startsector,
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
#if !defined HAVE_core_BUILD || !HAVE_core_BUILD
    if (globals->videozone)
    {
        if (globals->videolinking)
        {
            get_video_system_file_size(globals->settings.linkdir, maximum_VTSI_rank, sector_pointer_VIDEO_TS, relative_sector_pointer_VTSI, globals);
            get_video_PTS_ticks(globals->settings.linkdir, videotitlelength, nvideolinking_groups, VTSI_rank, globals);
        }

        change_directory(globals->settings.workdir, globals);
        free(globals->settings.linkdir);
    }
#endif

    uint8_t *title[naudio_groups];
    int
    nb_amg_files =       // normally 2 !
        create_amg(
            audiotsdir,
            command,
            &sectors,
            videotitlelength,
            relative_sector_pointer_VTSI,
            numtitles,
            ntitletracks,
            titlelength,
            globals);

    if (numtitles == NULL || nb_amg_files == 0)
    {
        foutput("%s\n", ERR "Critical error: failed to generate AUDIO_TS.IFO");
        clean_exit(-1, globals);
    }

    for (i = 0; i < naudio_groups; ++i)
    {
        title[i]=(uint8_t *) calloc(ntracks[i], 1);

        if (title[i] == NULL)
        {
            perror(ERR "title[k]");

            EXIT_ON_RUNTIME_ERROR_VERBOSE( "Impossible to create title arrays")
        }
        else
        {
            title[i][0]=0;
            for (j = 1; j < ntracks[i]; ++j)
                title[i][j]=(files[i][j].newtitle) + title[i][j-1];
        }
    }

    int ntotalfiles;

    //
    // checking coherence of startsector by recounting number of files
    // should always be OK unless some hardware issue or user interceptin came in at the worst of times
    //
    ntotalfiles =  count_dir_files(audiotsdir, globals);

    // BUG: if hybridation, videotsdir not defined!
    ntotalfiles += count_dir_files(videotsdir, globals);

    if (startsector == ntotalfiles  + 272)
    {
      int ad = startsector * 2048;
      if (globals->debugging)
         foutput("%s%d%s%d%s%#08X%s\n", MSG_TAG "Coherence test for ISO start sector... OK, : ", startsector, " sectors (adress: ", ad, ", ", ad, ")");
    }
    else
    {
      foutput("%s%s%d%s%d\n", WAR "Coherence test for ISO start sector failed: ", "start sector assessed as: ", startsector, " but should be: ", 272 + ntotalfiles);
      if (globals->debugging)
      {
          foutput("%s%s\n", DBG "img->tsbob not null: ", img->tsvob != NULL ? "yes" : "no");
          foutput("%s%d\n", DBG "sectors.topvob: ",      sectors.topvob);
          foutput("%s%d\n", DBG "nb_aob_files: ",  nb_aob_files);
          foutput("%s%d\n", DBG "nb_asv_files: ",  nb_asv_files);
          foutput("%s%d\n", DBG "nb_atsi_files: ", nb_atsi_files);
      }
    }

    errno=0;

    foutput("%c\n", '\n');
    foutput("%s\n" , ANSI_COLOR_BLUE"Group"ANSI_COLOR_YELLOW"   Title"ANSI_COLOR_GREEN"  Track"ANSI_COLOR_RESET"  First_Sect   Last_Sect  First_PTS  PTS_length cga\n");

    for (i=0; i <naudio_groups; i++)
    {
        for (j=0; j < ntracks[i]; j++)
        {
            foutput("    "ANSI_COLOR_BLUE"%d   "   ANSI_COLOR_YELLOW"%02d/"ANSI_COLOR_RED"%02d"ANSI_COLOR_GREEN"    %2d"ANSI_COLOR_RESET"  %10"PRIu32"  %10"PRIu32"  %10"PRIu32"  ",
                    i+1,
                    title[i][j]+1,numtitles[i],
                    j+1,
                    files[i][j].first_sector,
                    files[i][j].last_sector,
                    files[i][j].first_PTS);

            foutput("%10"PRIu32"  %2d\n",
                    files[i][j].PTS_length,
                    files[i][j].cga);
        }
    }

    foutput("\nTotal number of tracks: %d.\n", totntracks);
    /* freeing */

    foutput("%c", '\n');

    // Crucial, otherwise the ISO file may well be unordered even if AUDIO_TS files are OK after exit
    fflush(NULL);

#if !defined HAVE_core_BUILD || !HAVE_core_BUILD
    //
    if (globals->runmkisofs)
    {

        char dvdisopath[CHAR_BUFSIZ];
        memset(dvdisopath, '0', CHAR_BUFSIZ);
        if (globals->settings.dvdisopath == NULL)
        {
            sprintf(dvdisopath, "%s"SEPARATOR"%s", globals->settings.tempdir, "dvd.iso");
        }
        else memcpy(dvdisopath, globals->settings.dvdisopath, CHAR_BUFSIZ) ;

        if (file_exists(dvdisopath)) unlink(dvdisopath);
        uint64_t size;
        char* mkisofs=NULL;
        errno=0;
        if ((mkisofs=create_binary_path(mkisofs, MKISOFS, SEPARATOR MKISOFS_BASENAME, globals)))
        {
           const char* args[]={mkisofs, "-dvd-audio", "-v", "-o", dvdisopath, globals->settings.outdir, NULL};
           foutput("%s%s%s\n", INF "Launching: ", mkisofs, " to create image");

           run(mkisofs, args, WAIT, FORK, globals);

           free(mkisofs);
        }
        else
                foutput("%s\n", ERR "Could not access mkisofs binary.");


        size=stat_file_size(dvdisopath)/1024;
        if ((!errno) && (size > 4*SIZE_AMG + 2*SIZE_SAMG +1))  foutput(MSG_TAG "Image was created with size %" PRIu64 " KB.\n", size);
        else
        {
            foutput("%s\n", ERR "ISO file creation failed -- fix issue.");
            perror("mkisofs");
        }
    }

    if (globals->cdrecorddevice)
    {

        char dvdisoinput[CHAR_BUFSIZ];
        memset(dvdisoinput, '0', CHAR_BUFSIZ);
        if (globals->settings.dvdisopath == NULL)
        {
            sprintf(dvdisoinput, "%s"SEPARATOR"%s", globals->settings.tempdir, "dvd.iso");
        }
        else memcpy(dvdisoinput, globals->settings.dvdisopath, CHAR_BUFSIZ) ;
        errno=0;

        if (globals->rungrowisofs)
        {
            foutput("\n%s\n", INF "Launching growisofs to burn disc");
            char string[strlen(globals->cdrecorddevice)+2+strlen(dvdisoinput)];
            sprintf(string, "%s%c%s", globals->cdrecorddevice, '=', dvdisoinput);
            char*  args[]={"growisofs", "-Z", string, NULL};
#define GROWISOFS "/usr/bin/growisofs"
            run(GROWISOFS, (const char**) args, WAIT, FORK, globals);
        }
        else
        {

            char *verbosity=(globals->debugging)? "-v":"-s";
#define TEST (globals->cdrecorddevice[0] != '\0')
            int S=TEST? 10:8;
            char* args[S];
            memset(args, 0, S);
            errno=0;
            char* cdrecord=NULL;

            if ((cdrecord=create_binary_path(cdrecord, CDRECORD, SEPARATOR CDRECORD_BASENAME, globals)))
            {
                char *args0[]={cdrecord, verbosity,"blank=fast", "-eject","dev=", globals->cdrecorddevice, dvdisoinput, "-gracetime=", "1",  NULL};
                char *args1[]={cdrecord, verbosity,"blank=fast", "-eject",dvdisoinput, "-gracetime=", "1", NULL};
                if TEST memcpy(args, args0, sizeof(args0));
                else memcpy(args, args1, sizeof(args1));
                foutput("%s%s%s\n", INF "Launching: ", cdrecord, " to create disk");

                run(cdrecord, (const char**) args, WAIT, true, globals);
                free(cdrecord);
             }
            else
               foutput("%s\n", ERR "Could not access to cdrecord binary.");

        }

    }

    #endif
    // freeing files and heap-allocated globals

    for (j=0; j < naudio_groups; j++)
    {
        free(ntitletracks[j]);
        free(titlelength[j]);
        free(ntitlepics[j]);
        free(title[j]);
    }

    free_memory(command, globals);

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
