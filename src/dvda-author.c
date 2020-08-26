/*

dvda-author2.c  - Author a DVD-Audio DVD

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> July 2008-2013

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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef __WIN32__
#include <unistd.h>
#endif
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#define off64_t long long
#include <dirent.h>
#include <locale.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "commonvars.h"
#include "structures.h"
#include "c_utils.h"
#include "audio2.h"
#include "auxiliary.h"
#include "ports.h"
#include "file_input_parsing.h"
#include "launch_manager.h"
#include "command_line_parsing.h"
#include "lexer.h"
#include "commonvars.h"
#include "dvda-author.h"
/*  Global  options */


static char *LPLEXTEMPDIR;
char* TEMPDIR;

 command_t* lexer_analysis(command_t* command, lexer_t* lexer, const char* config_file, bool config_type, globalData *globals)
 {
    int i;
    lexer->nlines=MAX_LEXER_LINES;

    lexer->commandline=(char** ) calloc(MAX_LEXER_LINES, sizeof(char *));
    if (lexer->commandline == NULL) perror("\n"ERR "lexer\n");
    for (i=0; i < MAX_LEXER_LINES; i++)
    {
        lexer->commandline[i]=(char* ) calloc(2*MAX_OPTION_LENGTH, sizeof(char));
        if (lexer->commandline[i] == NULL) perror("\n"ERR "lexer\n");
    }

    if (config_type == CONFIGURATION_FILE) check_settings_file();

    errno=0;

    config_lexer(config_file, lexer, globals);

    if (command == NULL)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Could not allocate command-line structure")
    }

    command=command_line_parsing(lexer->nlines, lexer->commandline, command, globals);

    for (i=0; i < MAX_LEXER_LINES; i++)
        FREE(lexer->commandline[i])

        FREE(lexer->commandline)

        return command;
 }

static inline void allocate_paths(char** s, const char* dir, unsigned long length, globalData *globals)
{
    if (s != NULL) free(*s);
    *s = calloc(length + 10, sizeof(char));
    sprintf(*s, "%s"SEPARATOR"%s", globals->settings.tempdir, dir);
}

int main(int argc,  char* const argv[])
{

    int i=0;
    errno=0;

    if (errno) perror(ERR "Initial allocation\n");

    lexer_t   lexer_init;
    lexer_t   *lexer=&lexer_init;

    /****
     *  create a static command_t structure
     *  in command_line_parsing.c from default command line created by lexer.
     ****/

#ifndef __MINGW32__
    struct rusage nothing, start;
    compute_t timer= {&nothing, &start};
    starter(&timer);
#endif

    /*  DEFAULT   SETTINGS  */

    // Locale, time and log management

    setlocale(LC_ALL, LOCALE);

    char* currentdir = fn_get_current_dir_name();

    if (currentdir == NULL)
    {
      fprintf(stderr, "%s\n", ERR "Répertoire courant non alloué");
      exit(-1);
    }

    int currentdirlength=strlen(currentdir);
    char TEMPDIRROOT[currentdirlength + 14];
    TEMPDIR = calloc(currentdirlength + 20, sizeof(char));
    LPLEXTEMPDIR = calloc(currentdirlength + 26, sizeof(char));
    char *EXECDIR = calloc(MAX(currentdirlength, 20) + 4 + 25, sizeof(char));  // /usr/local/bin or /usr/bin under *NIX, "currentdir" directory/bin otherwise (win32...)
    // 4 for "/bin and be liberal and allow 25 more characters for the executable name.

#if defined BINDIR && ! defined LOCAL_BUILD
    memcpy(EXECDIR, BINDIR, strlen(BINDIR));
#else
    sprintf(EXECDIR, "%s" SEPARATOR "bin", currentdir);
#endif

    char *DATADIR=strdup(currentdir);
    char ***SNDT=calloc(1, sizeof(char**));
    SNDT[0]=calloc(1, sizeof(char*));
    SNDT[0][0]=strdup(DEFAULT_SOUNDTRACK);

    sprintf(TEMPDIRROOT, "%s%s%s", currentdir,(currentdir[0] == 0)? "" : SEPARATOR , TEMPDIR_SUBFOLDER_PREFIX DVDA_AUTHOR_BASENAME);
    sprintf(TEMPDIR, "%s"SEPARATOR"%s", TEMPDIRROOT, "temp");
    sprintf(LPLEXTEMPDIR, "%s"SEPARATOR"%s", TEMPDIRROOT, "temp.lplex");

    // Global settings are hard-code set by default as follows:
    errno=0;

    /* requests C99 */

    defaults def = {
        .settingsfile = strdup(SETTINGSFILE),
        .logfile = NULL,   // logfile path should be supplied on command line
        .indir = NULL,
        .outdir = NULL,
        .outfile = NULL,
        .lplexoutdir = NULL,
        .workdir = strdup(currentdir),
        .tempdir = NULL,
        .lplextempdir = NULL,
        .linkdir = NULL,
        .bindir =  EXECDIR, //bindir,
        .datadir = DATADIR,
        .fixwav_database = strdup(STANDARD_FIXWAV_DATABASE_PATH),
        .dvdisopath = NULL,
        .stillpicdir = NULL

    };

    uint32_t tab0[9] = {0};
    tab0[0] = 1;
    uint32_t tab1[9] = {0};
    tab1[0] = 1;
    uint32_t tab2[9] = {0};
    tab2[0] = 1;
    char**   tab3    = calloc(1, sizeof(char*));
    tab3[0] = strdup(DEFAULT_BACKGROUNDPIC);

    globalData globals=
    {

        /*top menu*/        NO_MENU,  // no top menu
        /*nooutput*/        false,
        /*runmkisofs*/      false,  // do not run mkisofs
        /*autoplay*/        false,  // no autoplay
        /*text table*/      false,  // no text table
        /*silence*/         false,
                            true,   // enabling lexer
        /*logfile*/	        false,  // no log
        /*loghtml*/         false,  // text log
        /* logdecode */     false,  // no forensic analysis of system files
        /*videozone*/       true,   // generates video zone
        /*videolinking*/    false,  // no video link
        /*decode */         false,  // decode MLP to WAV
        /*pipe*/   false,  // slow down extraction for buffering tp extrernal tools
        /*play*/            false,  // pipe to ffplay or other player
        /*playlist*/        false,  // no playlist
        /*cga*/             false,  // no explicit channel group assignement
        /*end_pause*/       false,  // no end pause
/*strict audio extraction */false,  // do not systematically check headers
        /*devel verbosity*/ false,  // no enhanced verbosity
        /*very verbose*/    false,  // not very verbose
        /*debugging*/       false,  // no debugging-level verbosity
        /*padding*/         false,  // gapless join for like audio tracks, no padding
        /*padding_continuous*/   false,  // no continuous padding
        /*lossy_rounding*/  false,  // No audio loss
        /*rungrowisofs*/    false,  // Do not burn with growisofs
#ifndef WITHOUT_sox
        /*sox_enable*/      false,  // no use of SoX
#endif

        /*fixwav_enable*/               true,  // use of fixwav
        /*fixwav_virtual_enable*/       true,  // use of fixwav (virtual headers)
        /* automatic behaviour */       true,
        /* do not prepend a header */   false,
        /* do not correct in place */   false,
        /* be cautious on overwrites*/  false,
        /* not interactive */           false,
        /* no padding */    false,
        /* prune */         false,
        /* force */         false,
        /* textablesize */  false,
        /* topmenusize */   false,
        .grouptextsize = tab1,
        .tracktextsize = tab2,
        .backgroundmpgsize = 0,
        .backgroundpicsize = 1,
        .soundtracksize =    tab0,
        /* topmenu_slidesize */ 0,
        /* highlightpicsize */  0,
        /* selectpicsize */     0,
        /* imagepicsize */      0,
        /* backgroundcolorssize */ 0,
        /* fixwav output suffix*/
        strdup(STANDARD_FIXWAV_SUFFIX),
        /*fixwav_parameters*/ NULL,
        /*xml filepath*/    NULL,
        /*spumux xml*/      NULL,
        /*cdrecord dev*/    NULL,
        /* aob path for decoding */ NULL,
        /*player*/            strdup(PLAYER),  // pipe to ffplay
        /*player-path*/     strdup(PLAYER_PATH),  // full path to player
        /*journal (log)*/   NULL, //(FILE*)
        /*access rights*/   DEFAULT_ACCESS_RIGHTS,
        /* it is necessary to use strdup as these settings may be overridden dynamically */
// Paths:

        def
    };


    pic     img0=
    {
        1, // always refresh menu pics by default (change of syntax, 10.06). Can be overridden with --colors norefresh
        0, // no loop
        0,  // list menus, not hierarchical
        0, // no active menus
        NULL,
        NULL,
        NULL,
        tab3, // black screen for jpg video mpg authoring
        strdup(DEFAULT_BLANKSCREEN), // black screen for png authoring
        NULL, //backgroundmpg
        NULL, //backgroundcolors
        strdup(DEFAULT_ACTIVEHEADER),
        NULL, //topmenu
        NULL,
        NULL, //stillvob
        NULL, //tsvob
        SNDT, //soundtrack  silence.wav
        strdup(DEFAULT_AUDIOFORMAT),
        strdup(DEFAULT_ALBUMCOLOR), //top menu pic album color
        strdup(DEFAULT_GROUPCOLOR), //top menu pic group color
        strdup(DEFAULT_ARROWCOLOR), //top menu pic arrow color
        strdup(DEFAULT_TEXTCOLOR_PIC), //top menu pic textcolor

        strdup(DEFAULT_BGCOLOR_PIC), //topmenu pic background color no strdup here (same-string optarg)
        strdup(DEFAULT_HCOLOR_PIC), //topmenu pic highlight color  no strdup here (same-string optarg)
        strdup(DEFAULT_SELCOLOR_PIC), //topmenu pic select action color no strdup here (same-string optarg)

        strdup(DEFAULT_ACTIVETEXTCOLOR_PALETTE), //top menu pic textcolor
        strdup(DEFAULT_ACTIVEBGCOLOR_PALETTE), //topmenu pic background color no strdup here (same-string optarg)
        strdup(DEFAULT_ACTIVEHCOLOR_PALETTE), //topmenu pic highlight color  no strdup here (same-string optarg)
        strdup(DEFAULT_ACTIVESELCOLOR_PALETTE), //topmenu pic select action color no strdup here (same-string optarg)

        strdup(DEFAULT_TEXTCOLOR_PALETTE), //system palette textcolor
        strdup(DEFAULT_BGCOLOR_PALETTE), //palette background color        no strdup here (same-string optarg)
        strdup(DEFAULT_HCOLOR_PALETTE), //palette highlight color    no strdup here (same-string optarg)
        strdup(DEFAULT_SELCOLOR_PALETTE), //palette select action color red   no strdup here (same-string optarg)

        strdup(DEFAULT_TEXTFONT), //textfont Courier
        NULL,
        strdup(PAL_FRAME_RATE),   // PAL frame rate by default
        strdup(PAL_NORM),
        "2", // 4:3 ratio
        strdup(DEFAULT_ASPECT_RATIO), // 4:3 ratio
        0, //  pointsize 30
        DEFAULT_FONTWIDTH, //  fontwidth 6
        UNDERLINE,
        0,  //h    // O length is usually OK
        0,  // min
        0,   // sec
        NOPICS,  // neither still pics nor video top menu
        0,   // no topmenu screen
        DEFAULT_MENU_NCOLUMNS,
        0,   // maxbuttons
        0,   // resbuttons
        0,  // pic count
        NULL, // npics table
        NULL,
        NULL, // no still pic vob size
        NULL, // no menu vob size
        NULL // still pic options

    };

    if (! globals.settings.tempdir) globals.settings.tempdir=strdup(TEMPDIR);
    if (! globals.settings.lplextempdir) globals.settings.lplextempdir=LPLEXTEMPDIR;
    if (! globals.settings.stillpicdir) globals.settings.stillpicdir=strdup(globals.settings.tempdir);
    unsigned long s = strlen(globals.settings.tempdir);

    allocate_paths(&globals.settings.indir, "audio", s, &globals);
    allocate_paths(&globals.settings.outdir, "output", s, &globals);
    allocate_paths(&globals.settings.lplexoutdir, "output", s, &globals);

#ifdef DEBUG
    fprintf(stderr, "globals.settings.tempdir  %s\nglobals.settings.lplextempdir  %s\nglobals.settings.stillpicdir  %s\nglobals.settings.indir  %s\nglobals.settings.outdir  %s\nglobals.settings.lplexoutdir  %s\ncurrentdir  %s\nglobals.settings.workdir  %s\n",
            globals.settings.tempdir,globals.settings.lplextempdir,globals.settings.stillpicdir,globals.settings.indir,
            globals.settings.outdir,globals.settings.lplexoutdir,currentdir, globals.settings.workdir);

#endif

    // Null arg is no longer supported, yet...

    if (argc == 1)
    {
        fprintf(stderr, "\n%s", "dvda-author syntax:\n------------------\n");
        help();
        return(errno);
    }

    // Now we are sure argc > 0

    // If a default setting textfile exists, it overrides the above hard-coded defaults
    // Path to defaults settings file must be "Path/to/DVD-A author folder/dvda-author.conf"
    // or otherwise defined by symbolic variable SETTINGSFILE at compile time.
    // Default settings, either hard-code set or by defaults textfile, can be overridden by command-line.

    /* Lexer extracts default command line from dvda-author.conf and returns corresponding argc, argv
       yet this is useless if command line is just "--version" or "--help" or equivalents */


    bool project_flag=0;
    char* project_filepath=NULL;
    command_t command0, *command=NULL;
    command0.img=&img0;
    command=&command0;

    for (i=1; i < argc ; i++)
        if (strcmp(argv[i],"--disable-lexer") == 0 || strcmp(argv[i] , "-W") ==0 || strcmp(argv[i], "--version") ==0 || strcmp(argv[i], "--help") ==0 || strcmp(argv[i], "-v") ==0 || strcmp(argv[i], "-h") == 0)
        {
            globals.enable_lexer=0;
                goto launch;
        }
        else
        if (strcmp(argv[i], "--project") == 0)
        {
           project_flag=1;
           if (i+1 < argc && argv[i+1][0] != '-')
               project_filepath=strdup(argv[i+1]);
           else
              project_filepath=strdup(DEFAULT_DVDA_AUTHOR_PROJECT_FILENAME);

           path_t *pstruct=parse_filepath(project_filepath, &globals);
           if (pstruct && pstruct->isfile)
           {
             //if (globals->debugging)
             fprintf(stderr, INF "Parsing project file %s\n", project_filepath);
           }
           else
           {
            fprintf(stderr, ERR "Failed to parse project file %s\n       Exiting...\n", project_filepath);
            clean_exit(EXIT_FAILURE, &globals);
           }
           free(pstruct);
        }

    lexer_analysis(command, lexer, SETTINGSFILE, CONFIGURATION_FILE, &globals);

        /* launch core processes after parsing user command-line, possibly overriding defaut values */


launch:

        if (project_flag)

          launch_manager(lexer_analysis(command, lexer, project_filepath, PROJECT_FILE, &globals), &globals);
        else

          launch_manager(command_line_parsing(argc, argv, command, &globals), &globals);

    // allocated in command_line_parsing()

    /* Compute execution time and exit */

    COMPUTE_EXECTIME

    FREE(currentdir)

    fflush(NULL);
    if ((globals.loghtml) && (globals.logfile)) htmlize(globals.settings.logfile);
    if ((globals.logfile) && (globals.journal)) fclose(globals.journal);

    if (globals.end_pause) pause_dos_type();

    if (errno && globals.veryverbose)
    {
        perror(WAR "Detected runtime errors");
    }

    return(errno);
}

