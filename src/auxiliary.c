/*
File:    auxiliary.c
Purpose: on-line help and auxiliary functions

dvda-author  - Author a DVD-Audio DVD

(C) Dave Chapman <dave@dchapman.com> 2005
(C) Revised version with zone-to-zone linking Fabrice Nicol <fabnicol@users.sourceforge.net> 2007, 2008

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

/* do not use code beautifiers/formatters for this file*/
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <stdarg.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "structures.h"
#include "ports.h"
#include "audio2.h"
#include "auxiliary.h"
#include "c_utils.h"
#include "file_input_parsing.h"
#include "ports.h"
#include "commonvars.h"
#include "menu.h"

extern globalData globals;
extern char* INDIR, *OUTDIR, *LOGFILE, *TEMPDIR, *LINKDIR, *WORKDIR;

void version()
{

    foutput("%s%s%s", "dvda-author version ", VERSION, "\nCopyright  2005 Dave Chapman; 2007-2012 Fabrice Nicol;\n2008-2009 Lee and Tim Feldkamp\n\n");
    foutput("%s","See file AUTHORS for other contributors.\n\n");
    foutput("%s","Latest version available from http://dvd-audio.sourceforge.net/\n\n");
    foutput("%s","This is free software; see the source for copying conditions.\n\nWritten by Dave Chapman, Fabrice Nicol, Lee and Tim Feldkamp.\n");
    return;
}

void help()
{

#ifdef __WIN32__
    system("mode con cols=85 lines=50");
    system("title DVD-A author Help");
#endif

// Use double \n for help2man to work correctly between options
// there is a bug that moves around options in help2man when a : is used on the first line of option comment so do not use it

printf("%s", "\ndvda-author "VERSION" creates high-resolution DVD-Audio discs\n\nfrom .wav, .flac and other audio files.\n\n");
printf("%s","Usage: dvda-author [OPTION]...\n");

printf("%s","\nOptions:\n\n");
printf("%s","Output options\n\n");

printf("%s","-h, --help               Diplay this help.\n\n");
printf("%s","-v, --version            Diplay version.\n\n");
printf("%s","-q, --quiet              Quiet mode.\n\n");
printf("%s","-d, --debug              Increased verbosity (debugging level)\n\n");

printf("%s","-t, --veryverbose        Like -d with enhanced verbosity for sample counts.\n\n");
printf("%s","  , --maxverbose         Like -t with maximum verbosity on audio buffers (devel info).\n\n");
printf("%s","    --no-output          Does not produce any file structure except for --fixwav."J"Computations will be performed.\n\n\n\n");
printf("%s","-P, --pause              Insert a final pause before exiting.\n\n");
printf("%s","-P0, --pause=0           Suppress a final pause before exiting"J"if specified in configuration file.\n\n");
printf("%s","-l, --log  path          Ouput a log to filepath."J"Argument must be supplied.\n\n");
printf("%s","-L, --logrefresh path    Same as -l but prior log will be erased on launching again.\n\n");
printf("%s","    --loghtml            Transform the log into HTML format, with colorized messages, adjacent to log with extension .html added.\n\n");
printf("%s","-k, --text               Generates text table in IFO files"J"Under development, implemented for 1-group discs."J"Use file information as arguments separated by commas.\n\n");
printf("%s","-W, --disable-lexer      Deactivates configuration file parsing.\n\n");

printf("%s","Playback options\n\n");

printf("%s","-a, --autoplay           Launches playback on loading disc.\n\n");

printf("%s","Authoring options\n\n");

printf("%s","   Soundfile authoring\n\n");

printf("%s","\n\nSupported audio types:   .wav\n");
#ifndef WITHOUT_FLAC
printf("%s",    J".flac and .oga (Ogg FLAC, see below)\n");
#endif
#ifndef WITHOUT_SOX
printf("%s", J"SoX-supported formats with -S enabled\n");

printf("%s", J"except for lossy formats.\n");
#endif
printf("%s","    --project [file]     Launches a dvda-author project."J"The project file should have the same syntax as dvda-author.conf [see file HOW_TO.conf]"J"By default, launches dvda-author.dap in the current directory.\n\n");
printf("%s","-i, --input directory    Input directory with audio files."J"Each subdirectory is a group.\n\n");
printf("%s","-o, --output directory   Output directory.\n\n");

printf("%s","-x, --extract disc[list] Extract DVD-Audio to directory -o."J"Groups are labelled g1, g2..."J"Optional comma-separated list of groups to be extracted\n"J"may be appended to disc path.\n"J"Tracks to be extracted may be listed ranked per group and separated by colons.\n\n");

printf("%s","    --play disc[list]    Play DVD-Audio tracks."J"Optional comma-separated list of groups to be played\n"J"may be appended to disc path.\n"J"Tracks to be played may be listed ranked per group and separated by colons.\n\n");

printf("%s","    --player PLAYER      Using a player that can be launched"J"by invocation of PLAYER (launch name or path to binary)"J"Player must return at end of each file."J"Default is 'vlc (files) vlc://quit'\n\n");
printf("%s","-p, --startsector NNN    Specify the number of the first sector"J"of the AUDIO_PP.IFO file in the output of mkisofs.\n\n");
printf("%s","                         If NNN=0, falling back on 281 (default).\n"J"Without -p start sector will be computed automatically.\n\n");
printf("%s","-g                       You may specify up to 9 groups of tracks."J"Minimum: one group.\n");
printf("%s","                         Enter full path to files if input directory is not set"J"by [-i].\n\n");
printf("%s","-j, --joingaps           Like -g with special processing to avoid gaps."J"Minimum: one group.\n");
printf("%s","-s,                      Like -g with all tracks merged into one single track."J"Minimum: one group.\n");
printf("%s","-z, --title           Separate two consecutive titles when files have same audio"J"characteritics within a group.\n");
printf("%s","-Z, --playlist           You may specify up to 9 group copies."J"Total number of groups and copy groups should not exceed 9.\n");
printf("%s","-n, --no-videozone       Do not generate an empty VIDEO_TS directory.\n\n");
printf("%s","-w, --rights             Access rights to directories created (octal values)\n\n");
printf("%s","-c, --cga                Enter channel group assignment right after group (-g, -j or -s).\n\n");
#ifndef WITHOUT_fixwav
printf("%s","-F, --fixwav(options)    Bad wav headers will be fixed by fixwav."J"Can be run alone without DVD-AUDIO output by adding --nooutput.\n\n");
printf("%s","-f, --fixwav-virtual(options)  Use .wav header repair utility "J"without any write operation.\n\n");
#endif
#ifndef WITHOUT_sox
printf("%s","-S, --sox                Use SoX to convert files to .wav."J"By default, only flac, Ogg FLAC "J"and .wav files are accepted.\n\n");
#endif
#if !HAVE_core_BUILD
printf("%s","    --no-padding         Block padding of audio files by dvda-author.\n\n");
printf("%s","    --minimal-padding    Only pad for evenness of sample count.\n\n");
printf("%s","-C, --pad-cont           When padding, pad with last known byte, not 0.\n\n");
printf("%s","-L, --lossy-rounding     Sample count rounding will be performed by cutting audio files.\n\n");

printf("%s","   Menu authoring\n\n");

printf("%s","-m, --topmenu(=mpgfiles) Generates top menu from comma-separated list of mpgfiles."J"Without argument, automatic menu generation is launched.\n\n");
printf("%s","-u, --duration hh:mm:ss Duration of top menu file, if provided."J"It is mandatory when --topmenu has an argument file.\n\n");
printf("%s","-M, --xml filepath       Generates dvdauthor xml project"J"to filepath.\n\n");
printf("%s","-H, --spuxml filepath    Generates spumux xml project"J"to filepath.\n\n");
printf("%s","-G, --image file         Menu Background image for customized menu authoring.\n\n");
printf("%s","-E, --highlight file     Menu Highlight image for customized menu authoring.\n\n");
printf("%s","-e, --select  file       Menu Select image "J"image that appears on pressing Enter with remote control\n");
printf("%s","                         usually Background with a change in text color.\n\n");
printf("%s","-N, --blankscreen file   For automatic menu authoring, you can replace black video background with this image.\n\n");
printf("%s","-O, --screentext string  Text for top menu. Format is"J"\"album_text=group1_text=text(track11),text(track21),...,text(trackn1):group2_text=text(track12),text(track22)...,text(trackn2):...\""J"with text(tracknk) the text for track n of group k and"J"groupk_text the text for group k.\n\n");
printf("%s","-U, --loop               loop background video.\n\n");
printf("%s","-K, --highlightformat    -1 for automatic menu authoring"J"with little square before titles, 0 for underlining, 1 for button highlight.\n\n");
printf("%s","-J, --font a,b,c         Font name,font size,font width"J"(number of pixels for width of font size 10)."J"Font name should be compatible with Image magick specifications (mogrify -list font).\n\n");
printf("%s","    --fontname a             Font name.\n");
printf("%s","    --fontsize b             Font size.\n");
printf("%s","    --fontwidth b             Font width.\n");
printf("%s","-Y, --topmenu-palette string     Text for system palette. Format is"J"either \"norefresh\", to block the refreshing of menu images, or:"J"textcolor:highlight_color:select_action color"J"in alpha-YCrCb 32-bit hexa coding. Here textcolor is the non-highlighted text for tracks, "J","J"highlight_color is the underline or mobile motif color,"J"and select_action_color is album and group labels"J"as well as color of tracks on pressing the highlighted track.\n\n");
printf("%s","-8, --activemenu-palette string     Text for menu colors. Format is:"J"textcolor:highlight_text_color:highlight_color:select_action color"J"in alpha-YCrCb 32-bit hexa coding. Here textcolor is the non-highlighted text for tracks, "J"highlight_text_color is the color of album and group labels and highlighted text,"J"highlight_color is the underline or mobile motif color,"J"and select_action_color is on pressing the highlighted track.\n\n");
printf("%s","-y, --topmenu-colors string     Text for menu colors. This is a developer's switch. "J"Use the -palette switches for modifying display colors. "J"This switch determines the colors of pictures generated in the temporary directory before creating the mpg background files."J" Format is either \"norefresh\", to block the refreshing of menu images, or "J"textcolor:backgroundcolor:highlightcolor:select action color in rgb values a,b,c between 0 and 255.\n\n");
printf("%s","-b, --background         Background jpg files (comma-separated) to create a background mpg file"J"into which titles are multiplexed."J"Specify as many files as there are menus, or the last file will be duplicated for missing menu files.\n\n");
printf("%s","    --background-colors  Background RGB colors to colorize background mpg files"J"into which titles are multiplexed."J"Specify as many colors as there are menus, or the last color will be duplicated for missing menu colors."J"Syntax is r,g,b:r2,g2,b2:...:rk,gkbk for --nmenus=k.\n\n");
printf("%s","-B, --background-mpg list  Background mpg file(s) in a comma-separated list"J"into which titles are multiplexed.\n\n");
printf("%s","    --topmenu-slides file(s) .jpg image files to be multiplexed with sound tracks (see option below) into a slideshow."J"By default a black screen will be used."J"Each menu screen should have at least one associated .jpg slide. List of slides is comma-separated for each menu."J"Menu lists are colon-separated: menu1_pic1,menu1_pic2:menu2_pic1,menu2_pic2, etc.\n\n");
printf("%s","-Q, --soundtracks file(s)  Background wav file(s)"J"to be multiplexed into a slideshow, with option --topmenu-slides."J"By default a silent track will be multiplexed."J"Each menu screen should have its own sound track. List of tracks follows same usage as for --topmenu-slides."J"Note that currently with several menus there can be only one track/slide per menu.\n\n");
printf("%s","-A, --topvob f           Import already authored top vob menu f.\n\n");
printf("%s","-A, --import-topmenu f   Import VIDEO_TS stream (VOB format) into AUDIO_TS top menu (AUDIO_TS.VOB).\n\n");
printf("%s","-0, --menustyle desc     Specifies top menu style"J"By default, tracks are listed under group headers."J"If desc='hierarchical', the first menu screen lists groups."J"If desc='active', all tracks will have an associated still picture with menu links that remain active while listening to the track.\n\n");
printf("%s","-1, --stillvob f         Import already authored still pictures vob.\n\n");
printf("%s","-2, --stilloptions ...   Still picture options (add after --stillpics). Each option applies to ranked pic, e.g."J"rank=0,manual,starteffect=fade,rank=1,starteffect=dissolve."J"Suboptions are:"J"rank=[number], starteffect=[effect], endeffect=[effect]"J"manual, lag=[number], start=[number], active"J"See details below. \n\n");
printf("%s","-3, --stillpics          Background jpg files to create one or more still pictures"J"for each track.See usage below."J"If a track has no still picture, use two hyphens in a row."J"You may otherwise indicate a directory containing pictures"J"named pic_abc.jpg, with a,b,c between 0 and 9.\n\n");
printf("%s","-3, --stillpics dir/     Directory for background jpg files to create one still picture for each track."J"Pics should be named pic_000.jpg, ..., up to pic_999.jpg (maximum).\n\n");
printf("%s","-4, --norm               Argument is 'ntsc', 'pal' or 'secam', depending on TV standard.\n\n");
printf("%s","-5, --aspect             Set the playback aspect ratio code of the encoded video. By default, this value is inferred from  the input header.\n\n");
printf("%s","                         1  - 1:1 display"J"2  - 4:3 display"J"3  - 16:9 display"J"4  - 2.21:1 display\n\n");
printf("%s","-6, --nmenus int         Generates int top menus (default 1).\n\n");
printf("%s","-7, --ncolumns int       Top menus will have at most int columns (default 3).\n\n");


printf("%s","   Disc authoring\n\n");
printf("%s","-I, --mkisofs(=file)     Run mkisofs to author disc image using file"J"as an ISO image. If file is empty, use tempdir/dvd.iso.\n\n");
printf("%s","-r, --cdrecord(=a,b,c)   Run cdrecord to burn disc image."J"Unless specified, --mkisofs will be automatically triggered with default tempdir/dvd.iso value."J"Device is of the form a,b,c, see cdrecord -scanbus. It can be omitted"J"if there is just one writer.\n\n");
printf("%s","-R, --growisofs /dev/dvd Run growisofs to burn disc image."J"Device is of the form /dev/scd0 under many GNU/Linux distributions."J"It cannot be omitted.\n\n");

printf("%s","-V, --videodir directory Path to VIDEO_TS directory\n\n");
printf("%s","-T, --videolink rank     Rank of video titleset linked to in video zone"J"(XX in VTS_XX_0.IFO)."J"In this case the path to the VIDEO_TS linked to"J"must be indicated.\n\n");
#endif

printf("%s","Software configuration\n\n");

printf("%s","-D, --tempdir directory  Temporary directory.\n\n");
printf("%s","-X, --workdir directory  Working directory: current directory in command line relative paths."J"By default, the current directory."J"With Code::Blocks and similar IDE, you may have to specify your root package directory as argument to --workdir.\n\n");
printf("%s","    --no-refresh-tempdir Do not erase and recreate the temporary directory on launch.\n\n");
printf("%s","    --no-refresh-outdir  Do not erase and recreate the output directory on launch.\n\n");
#if !HAVE_core_BUILD
printf("%s","-W, --bindir path        Path to auxiliary binaries.\n\n");
#if HAVE_curl
printf("%s","    --check-version  Only check whether this is the latest version of dvda-author, does not download.\n\n");
printf("%s","    --download  Download the latest version of dvda-author. Triggers --check-version.\n\n");
printf("%s","    --download=force  Download the latest version of dvda-author even if the current one is the same."J"Maybe useful for reinstalling or in case of impaired source code.\n\n");
#endif

printf("%s","Sub-options\n\n");
#ifndef WITHOUT_FIXWAV

printf("%s", "\n    fixwav sub-options:\n\n"\
"simple-mode"\
K"Deactivate default automatic mode and advanced options.\n"\
K"User will be asked for more information.\n\n"\
"prepend"\
K"Prepend header to raw file\n\n"\
"in-place"\
K"Correct header in the original file (not advised)\n\n"\
"cautious"\
K"Be cautious when overwriting files in-place\n\n"\
"interactive"\
K"Request information from user.\n\n"\
"padding"\
K"Pad files according to WAV standard\n\n"\
"prune"\
K"Cuts off silence at end of files\n\n"\
"force"\
K"Launches fixwav before SoX for mangled headers\n\n"\
"output=sf"\
K"Copy corrected file to new filepath with string suffix sf\n\n"\
"infodir=db\n"\
K"Copy info chunks from wav headers to file db"SEPARATOR"database\n\n"\
"  Sub-options should be separated by commas and appended\n\n\
  after short option or after = sign if long option is used:\n\n\
  -f/-Fsuboption or --fixwav(-virtual)=suboption\n\n\
  without any whitespace in between them.\n\n\
  Example: --fixwav=simple-mode,prepend,interactive,output=new\n\
");
#endif

printf("%s", "\n    Still pictures:\n\n"\
K"p11,p21,...,pn1-p22,p22,...,pn2-...\n\n"\
K"with tracks separated by hyphens and pictures by commas.\n\n"\
K" Examples: -g ~/a.wav --stillpics image1.jpg,image2.jpg,image3.jpg-image4.jpg,image5.jpg,image6.jpg\n\n"\
K" If there are no pics for a track use -- as below (no pics for second track):\n\n"\
K"           -g ~/a.wav ~/b.wav ~/c.wav --stillpics image1.jpg,image2.jpg,image3.jpg--image4.jpg,image5.jpg,image6.jpg\n\n"
);
printf("%s", "\n    Still picture transition effects:\n\n"\
"rank=k"\
K"k is the absolute rank of stillpic (0-based) to which the following options apply (order-dependent).\n\n"\
"start=k"\
K"picture starts at k sec from start of track.\n\n"\
"manual"\
K"Enable browsable (manual advance) pictures (experimental).\n\n"\
"starteffect=effect"\
K"transition effect at start of pic: cut (default), fade, dissolve, top-wipe, bottom-wipe, left-wipe, right-wipe.\n\n"\
"endeffect=effect"\
K"like starteffect at end of pic show (under development)\n\n"\
"lag=k"\
K"k is the duration of transition effect in multiples of 0.32 second (k < 16).\n\n"\
"active"\
K"menu links will be displayed on still picture and remain active while listening.\n\n"\
K" Example: --stilloptions rank=0,start=03,starteffect=fade,lag=12,rank=1,start=20,starteffect=dissolve,lag=15\n\n"\
"\n\n"\
K"Transition effects like fade or dissolve may vary depending on hardware.\n\n"\
K"End effects may be visible only when several pictures are used for a track slide.\n\n"\
K"If a track has just one still pic, only start effects may be visible.\n\n"
);

printf("%s","\n\nNote: for optional arguments noted (=...) above, usage is either"J" -xY, with x the option flag and Y the argument, or"J" --option=argument.\n");
#endif

printf("%s","\n\nThere must be a maximum of 9 audio groups.\n\n");
printf("%s","Each subdirectory of an audio input directory will contain titles\n\nfor a separate audio group.\n\n\
A number between 1 and 9 must be included as the second character of the\n\nsubdirectory relative name.\n\n");
printf("%s", "Full Input/Output paths must be specified unless default settings are set.\n\n");
printf("\n%s", "By default, defaults are set in /full path to dvda-author folder/defaults\n\n");


printf("%s", "Examples:\n");
printf("%s", "\n\
-create a 3-group DVD-Audio disc (legacy syntax):\n\n\
  dvda-author -g file1.wav file2.flac -g file3.flac -g file4.wav\n\n");
#if !HAVE_core_BUILD
printf("%s", "-create a hybrid DVD disc with both AUDIO_TS mirroring audio_input_directory\n\n\
  and VIDEO_TS imported from directory VID, outputs disc structure to directory\n\n");
printf("%s", " DVD_HYBRID and links video titleset #2 of VIDEO_TS to AUDIO_TS:\n\n");
printf("%s","  dvda-author -i ~"SEPARATOR"audio"SEPARATOR"audio_input_directory"K"-o DVD_HYBRID -V Video"SEPARATOR"VID -T 2 \n\n");
printf("%s", "-create an audio folder from an existing DVD-Audio disc:\n\n\
  dvda-author --extract /media/cdrom0,1,3:2,5:3,6,7 -o dir\n\n");
printf("%s","will extract titlesets 1,3 (track 2),5 (track 3), 6,7 of the disc to\n\n\
dir"SEPARATOR"g1, dir"SEPARATOR"g3, dir"SEPARATOR"g5, dir"SEPARATOR"g6, dir"SEPARATOR"g7 respectively.\n\n");
printf("%s", "-play group 1 and track 2 of group 3 of an existing DVD-Audio disc using VLC:\n\n\
  dvda-author --play /media/cdrom0,1,3:2 --player /usr/bin/vlc\n\n");

printf("%s", "\nRequired compile-time constants:\n\n_GNU_SOURCE, __CB__ if compiling with Code::Blocks or similar IDE.\n\n");
#endif

printf("%s", "Optional compile-time constants:\n\nLONG_OPTIONS for the above long options (starting with --)\n\n\
SHORT_OPTIONS_ONLY to block all long options.\n\n\
LOCALE to recompile for another locale than the default \"C\".\n\n\
SETTINGSFILE to specify default filepath of the configuration file.\n\n\
FLAC__HAS_OGG to enable Ogg FLAC support.\n\n\
_LARGEFILE_SOURCE,_LARGE_FILES,_FILE_OFFSET_BITS=64\n\n\
to enable large file support.\n\n\
ALWAYS_INLINE forces code inlining.\n\n\
WITHOUT_SOX to compile without SoX code\n\n\
WITHOUT_FLAC to compile without FLAC/OggFLAC code\n\n\
WITHOUT_FIXWAV to compile without fixwav code\n\n");

printf("%s", "\nReport bugs to fabnicol@users.sourceforge.net\n");
return;
}

void check_settings_file()
{

    /* If a command-line build system is not used, e.g. with editors like Code::Blocks, it is unwieldy to define SETTINGSFILE so creating automatically to avoid crashes */
    /* It may also be useful if configuration file was installed in a directory with inappropriate access rights */

    if (fopen(SETTINGSFILE, "rb") ==  NULL)
    {
        foutput("[WAR]  Could not open settings file, creating one in %s...\n", SETTINGSFILE);
        FILE* settingsfile=fopen(SETTINGSFILE, "wb");
        if (settingsfile == NULL)
        {
           foutput("[ERR]  Could not create settings file in path %s\n       Check that you have adequate administrative rights\n       Exiting...\n", SETTINGSFILE);
           clean_exit(EXIT_FAILURE);
        }

        fprintf(settingsfile, "%s","\
## dvda-author configuration file\n\
## These parameters override hard-code defaults.\n\
## Comment out parameters with # to unselect them.\n\
## lexer will skip lines beginning with # or without any content or with just white space or tabs.\n\
## Extra white space or tabs will be skipped.\n\
## debugging verbosity level\n\
#	debug");


        fclose(settingsfile);
    }

}

_Bool increment_ngroups_check_ceiling(uint8_t *ngroups, uint8_t * nvideolinking_groups)
{

    if (*ngroups < 9)
    {
        if (nvideolinking_groups != NULL)
        {
            if (*nvideolinking_groups + *ngroups < 9)
                ++*nvideolinking_groups;
            else
            {
                foutput("[ERR]  DVD-Audio only supports up to 9 groups; audio groups=%d; video-linking groups=%d\n", *ngroups, *nvideolinking_groups);
                clean_exit(EXIT_SUCCESS);
            }
        }
        ++*ngroups;
    }
    else
    {
        if (nvideolinking_groups != NULL)
            foutput("[ERR]  DVD-Audio only supports up to 9 groups; audio groups=%d; video-linking groups=%d\n", *ngroups, *nvideolinking_groups);
        else
            foutput("[ERR]  DVD-Audio only supports up to 9 groups; audio groups=%d\n", *ngroups);
        clean_exit(EXIT_SUCCESS);
    }
    return 1;
}

fileinfo_t** dynamic_memory_allocate(fileinfo_t **  files,uint8_t* ntracks,  uint8_t  ngroups, uint8_t n_g_groups, uint8_t nvideolinking_groups)
{

    float memory=0;
    int i, j;

    /*   n_g_groups: number of g-type audio groups ('Dave code usage')
     *   nvideolinking_groups: number of video-linking groups
     *   ngroups   : total number of groups
     *   ngroups = n_g_groups + nvideolinking_groups
     */

    /* It is crucial to use calloc as boolean flags are used */

    if ((files= (fileinfo_t **) calloc(ngroups, sizeof(fileinfo_t *))) == NULL)
        EXIT_ON_RUNTIME_ERROR

        for (i=0 ; i < n_g_groups; i++)
        {
            if ((files[i]=(fileinfo_t *) calloc(ntracks[i], sizeof(fileinfo_t))) == NULL)
                EXIT_ON_RUNTIME_ERROR
                memory+=(float) (ntracks[i])*sizeof(fileinfo_t)/1024;
            if (globals.debugging)
                foutput("[MSG]  g-type  audio group  :  %d   Allocating:  %d  track(s)  (strings=%.1f kB)\n", i,  ntracks[i], memory);
        }

    for (i=n_g_groups ; i < ngroups-nvideolinking_groups; i++)
    {

        if ((files[i]=(fileinfo_t *) calloc(ntracks[i], sizeof(fileinfo_t)) )== NULL)
            EXIT_ON_RUNTIME_ERROR

            for (j=0; j < ntracks[i]; j++)
                if ((files[i][j].filename=calloc(CHAR_BUFSIZ, sizeof(char)) )== NULL)
                    EXIT_ON_RUNTIME_ERROR
                    memory+=(float) (ntracks[i])*(sizeof(fileinfo_t) + CHAR_BUFSIZ)/1024; // CHAR_BUFSIZ characters assigned later on by strdup

        if (globals.debugging)
            foutput("[MSG]  Directory audio group:  %d   Allocating:  %d  track(s)  (strings=%.1f kB)\n", i,  ntracks[i], memory);
    }
    for (i=ngroups-nvideolinking_groups ; i < ngroups; i++)
    {
        if ((files[i]=(fileinfo_t *) calloc(1, sizeof(fileinfo_t))) == NULL)
            EXIT_ON_RUNTIME_ERROR
            memory+=(float) sizeof(fileinfo_t)/1024;
        /* sanity check: 0 tracks should be allocated */
        if (globals.debugging)
            foutput("[MSG]  Video-linking group  :  %d   Allocating:  %d  track(s)  (strings=%.1f kB)\n", i, ntracks[i], memory);
    }

    return files;
}


void free_memory(command_t *command)
{
    int i, j;
#if !HAVE_core_BUILD
    initialize_binary_paths(FREE_BINARY_PATH_SPACE);
#endif
    if (command)
    {
        short int naudio_groups=command->ngroups-command->nvideolinking_groups;

        for (i=command->n_g_groups; i < naudio_groups ; i++)
        {

            for (j=0; j < command->ntracks[i]; j++)
            {
                if (globals.debugging)
                    foutput("[INF]  Freeing i=%d  j=%d\n",i, j );
                FREE(command->files[i][j].filename)
                //FREE(command->files[i][j].filetitle)

            }


        }

        for (i=0; i < naudio_groups ; i++)
           for (j=0; j < command->ntracks[i]; j++)
              FREE(command->files[i][j].audio)

        for (i=0; i < command->ngroups; i++)
            FREE(command->files[i])
        FREE(command->files)
     }

    FREE(globals.settings.outdir)
    FREE(globals.settings.workdir)
    FREE(globals.settings.tempdir)
    FREE(globals.settings.indir)
    FREE(globals.settings.linkdir)
    //FREE(globals.settings.logfile)
    FREE(globals.settings.settingsfile)
    //FREE(globals.settings.fixwav_database)
    FREE(globals.settings.dvdisopath)
    FREE(globals.settings.stillpicdir)
    FREE(globals.xml)



if (command)
{
    //FREE(command->img->selectpic)
    //FREE(command->img->imagepic)
    //FREE(command->img->backgroundmpg)
    FREE(command->img->soundtrack[0][0])
    FREE(command->img->textcolor_palette)
    //FREE(command->img->highlightcolor_pic/palette)  : do not free
    //FREE(command->img->selectfgcolor_pic/palette)
    FREE(command->img->textcolor_pic)
    FREE(command->img->textfont)
//    FREE(command->img->blankscreen)
    FREE(command->img->screentextchain)
    FREE(command->img->norm)
    FREE(command->img->framerate)
//    FREE(command->img->stillpicvobsize)
    FREE(command->img->aspectratio)

    j=0;
    FREE2(command->textable)
   // FREE2(command->img->backgroundpic)
}


}

void create_file(char* audiotsdir, char* basename, uint8_t* array, size_t size)
{
  char outfile[strlen(audiotsdir)+strlen(basename)+1+1];
  sprintf(outfile, "%s"SEPARATOR"%s",audiotsdir, basename);
  foutput("[INF]  Creating %s\n",outfile);

  unlink(outfile); // I sometimes had issues under linux when unlink was not called in rare cases. Reset errno to 0 just after.
  errno=0;
  FILE* f;
  if (!globals.nooutput)
  {
      f=fopen(outfile,"wb");
  if (f == NULL)
    fprintf(stderr, "[ERR] %s could not be opened properly.\n", basename);
  if (errno) perror("[ERR] ");
  errno=0;

    if (fwrite(array, 1, size, f) == size )
    foutput("%s%s%s\n", "[MSG]  ", outfile," was created.");
    else
    fprintf(stderr, "[ERR]  %s could not be created properly -- fwrite error.\n", basename);


    if (fclose(f)== EOF)
    fprintf(stderr, "[ERR]  %s could not be closed properly.", basename);
  }

}
// fn_strtok is a strtok replacement function that, unlike strtok, takes care of the input chain and can be invoked safely in loops or whose delimiting char can be changed without
// using strtok_r to get safe.
// It is mingwin32 portable, unlike strtok_r (current version 4.4).
// It fills in an array of strings with extracted substrings, heap-allocating just what is requested: do not forget to free memory when possible.
// Array is NULL-terminated.
// Also, for each substring s, action f can optionally be performed on s, which may save loops.
// action f is always performed when f is not NULL, and breaks the extraction loop if f returns 0 on string. f takes a counter as second arg.
// function's last arg is the remainder of non-cut chain (after all f-loops)
// Returns NULL on error or array of extracted strings
// remainder should be allocated prior to call

char** fn_strtok(char* chain, char delim, char** array, uint32_t count, int  (*f)(char*, uint32_t ), char* remainder)
{
  if (chain == NULL) return NULL;
  //
  char *s=strdup(chain);

  if (s == NULL) return NULL;
  errno=0;

  uint32_t j=1, k=0;
  int32_t cut[strlen(s)/2];
  cut[0]=-1;
  do  if (s[j] == delim)
      {
        cut[++k]=j;

      }
  while (s[j++] != '\0');
  cut[k+1]=j-1;
  // exactly k=size-2 cuts, k+1 substrings and array filled with k+1 substrings and NULL-terminating pointer, so array is sized size*sizeof(char)
  uint32_t size=k+2;
  array=(char**) calloc(size, sizeof(char*));
  if (array == NULL)
  {
      perror("[ERR]  fn_strtok");
      return NULL;
  }
  k=0;

  while (k<= (size-2))
    {

      array[k]=calloc(cut[k+1]-cut[k], sizeof (char));
      if (array[k] == NULL)
         { perror("[ERR]  fn_strtok, array[k]"); return NULL;}

      memcpy(array[k], s+cut[k]+1, cut[k+1]-cut[k]-1);
      array[k][cut[k+1]-cut[k]-1]=0;
      k++;
      if ((*f) && ((*f)(NULL, count) == 0)) break;

    }
  array[k]=NULL;
  if ((remainder) && (s[cut[k]] !='\0'))
   strcpy(remainder, s+cut[k]+1);
  else remainder=NULL;
  free(s);

  if (errno) return NULL;
  else return array;
}

// This loop cut may be useful to use with fn_strtok
// first arg is spurious

int cutloop(char GCC_ATTRIBUTE_UNUSED*c, uint32_t count)
{
    static uint32_t loop;
    loop++;
    if (count > loop) return 1;
    else
     loop=0;
    return 0;
}

int arraylength(char ** tab)
  {
int w=0;
if (tab) while (tab[w] != NULL)  w++;
return w;
}

 // if installed with autotools, if bindir overrides then use override, otherwise use config.h value;
// if not installed with autotools, then use command line value or last-resort hard-code set defaults and test for result
#if !HAVE_core_BUILD
char* create_binary_path(char* local_variable, char* symbolic_constant, char* basename)
{

    if (symbolic_constant[0])
    {
        if (globals.settings.bindir == NULL)
            local_variable=strdup(symbolic_constant);
        else
            local_variable=win32quote(concatenate(local_variable, globals.settings.bindir, basename));
    }
    else
        local_variable=win32quote(concatenate(local_variable, globals.settings.bindir, basename));
    if (globals.debugging) foutput("[MSG]  Path to %s is %s from bindir=%s and basename=%s\n", basename, local_variable,globals.settings.bindir, basename);
    return local_variable;

}

void download_latest_version(_Bool download_new_version_flag,_Bool force_download_flag)
{
   #if HAVE_curl

      extern char* curl;

      initialize_binary_paths(CREATE_CURL);

      erase_file("version.current");

      FILE* versionfile;
      int error=download_file_from_http_server(curl, "version.current", "dvd-audio.sourceforge.net");
      if ((error == 0) && (NULL != (versionfile=fopen("version.current", "rb"))))
      {
	char year[5]={0};
	char month[5]={0};
	char build[6]={0};

	if (NULL == fgets(year, 5, versionfile)) clean_exit(EXIT_FAILURE);
	if (NULL == fgets(month, 5, versionfile)) clean_exit(EXIT_FAILURE);
	if (NULL == fgets(build, 6, versionfile)) clean_exit(EXIT_FAILURE);

	year[2]=0;
	month[2]=0;
	build[3]=0;
	char* version=strdup(VERSION);
	char this_year[3]={version[0], version[1], '\0'};
	char this_month[3]={version[3], version[4], '\0'};
	char this_build[4]={version[6], version[7], version[8], '\0'};
	if (globals.veryverbose)
	{
	foutput("[MSG]  Current version is: %s-%s ", year, month);
	foutput("build %s\n", build);
	foutput("[MSG]  Software version is: %s-%s ", this_year, this_month);
	foutput("build %s\n", this_build);
	}

	if (atoi(this_year) < atoi(year) || atoi(this_month) < atoi(month) || atoi(this_build) < atoi(build))
	  foutput("[INF]  A more recent version has been released (%s-%s build %s)\n       Download it from http://dvd-audio.sourceforge.net\n       You can also trigger download by relaunching with dvda-author --download.\n", year, month, build);
	else
	{
	  foutput("%s", "[MSG]  This version is the latest version available.\n");
	  if (download_new_version_flag)
	  {

	    if (!force_download_flag)
	    {
	      foutput("%s", "[MSG]  You do not need to download the new package.\n");
	      foutput("%s", "[MSG]  To force downloading use --download=force instead\n       Now exiting...\n");
	      clean_exit(EXIT_SUCCESS);
	    }
	    else
	      foutput("%s", "[MSG]  Downloading the current package anyhow. Please wait...\n");

	  }
	}



	if (download_new_version_flag)
	{
	  char dvda_author_fullpath[350]={0};
	  unlink("dvda-author-update.tar.gz");
	  errno=0;
	  sprintf(dvda_author_fullpath, "http://sourceforge.net/projects/dvd-audio/files/dvda-author/%s-%s.%s/%s-%s.%s-%s.tar.gz/download", "dvda-author", year, month, "dvda-author", year, month, build);
	  error=download_rename_from_http_server(curl, "dvda-author-update.tar.gz", dvda_author_fullpath);
	  FILE* package;
	  if ((error == 0) && (NULL != (package=fopen("dvda-author-update.tar.gz", "rb"))))
	  {

	    fclose(package);
	    foutput("[MSG]  New version %s-%s build %s was downloaded as dvda-author-update.tar.gz\n", year, month, build);

	  }
	  else
	    foutput("%s", "[MSG]  Failed to download new version.\n");

	  exit(EXIT_SUCCESS);

	}
	free(version);
	initialize_binary_paths(FREE_MEMORY);


      }
     #endif
}
#endif
