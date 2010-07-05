/*

dvda-author2.c  - Author a DVD-Audio DVD

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> July 2008

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

#if HAVE_CONFIG_H && !defined __CB__
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
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

globalData globals;
char *currentdir, *TEMPDIRROOT, *TEMPDIR, *LOGFILE, *INDIR, *OUTDIR, *LINKDIR;

 command_t* lexer_analysis(command_t* command, lexer_t* lexer, const char* config_file, _Bool config_type)
 {
    int i;
    lexer->nlines=MAX_LEXER_LINES;

    lexer->commandline=(char** ) calloc(MAX_LEXER_LINES, sizeof(char *));
    if (lexer->commandline == NULL) perror("[ERR]  lexer");
    for (i=0; i < MAX_LEXER_LINES; i++)
    {
        lexer->commandline[i]=(char* ) calloc(2*MAX_OPTION_LENGTH, sizeof(char));
        if (lexer->commandline[i] == NULL) perror("[ERR]  lexer");
    }

    if (config_type == CONFIGURATION_FILE) check_settings_file();

    errno=0;

    config_lexer(config_file, lexer);


    if (command == NULL)
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Could not allocate command-line structure")

        command=command_line_parsing(lexer->nlines, lexer->commandline, command);

    for (i=0; i < MAX_LEXER_LINES; i++)
        FREE(lexer->commandline[i])

        FREE(lexer->commandline)

        return command;
 }

void normalize_temporary_paths(pic* img)
{
    static size_t s;

    // cannot easily  free(globals.settings.logfile) etc. as embedded  in structure ?
    if (img == NULL)
    {

        s=strlen(globals.settings.tempdir);
        globals.settings.indir=realloc(globals.settings.indir, (s+10)*sizeof(char));
        globals.settings.outdir=realloc(globals.settings.outdir, (s+10)*sizeof(char));
        globals.settings.linkdir=realloc(globals.settings.linkdir, (s+10)*sizeof(char));
        globals.settings.indir=calloc(s+10,1);

        sprintf(globals.settings.indir, "%s"SEPARATOR"%s", globals.settings.tempdir, "audio");
        sprintf(globals.settings.outdir, "%s"SEPARATOR"%s", globals.settings.tempdir, "output");
        sprintf(globals.settings.linkdir, "%s"SEPARATOR"%s", globals.settings.tempdir, "VIDEO_TS");


    }
    else
    {

        if ((img->imagepic) && (img->highlightpic) && (img->selectpic)) return;
        else if ((img->imagepic) || (img->highlightpic) || (img->selectpic))
        {
              printf("[WAR]  You should enter three menu-authoring custom-made .png pictures, for main image, highlight and select action.\nn       Reverting to automatic mode.\n\n");
              globals.topmenu=AUTOMATIC_MENU;
        }

        int menu;

        img->backgroundpic=calloc(img->nmenus+1,sizeof(char*));
  //      img->backgroundmpg=calloc(img->nmenus+1,sizeof(char*));
        img->imagepic=calloc(img->nmenus+1,sizeof(char*));
        img->highlightpic=calloc(img->nmenus+1,sizeof(char*));
        img->selectpic=calloc(img->nmenus+1, sizeof(char*));

        // useless to realloc for just one menu !

        for (menu=0;  menu < img->nmenus; menu++)
        {
    //        img->backgroundmpg[menu]=(char*)calloc(26+s, sizeof(char));
    //        sprintf(img->backgroundmpg[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "background", menu, ".mpg");

            img->backgroundpic[menu]=calloc(s+13, sizeof(char));
            sprintf(img->backgroundpic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "bgpic", menu, ".jpg");

            img->imagepic[menu]=calloc(s+13, sizeof(char));
            sprintf(img->imagepic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "impic", menu, ".png");

            img->highlightpic[menu]=calloc(s+13,sizeof(char));
            sprintf(img->highlightpic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "hlpic", menu, ".png");

            img->selectpic[menu]=calloc(s+13,sizeof(char));
            sprintf(img->selectpic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "slpic", menu, ".png");
        }

        img->imagepic[img->nmenus]=NULL;
        img->highlightpic[img->nmenus]=NULL;
        img->selectpic[img->nmenus]=NULL;
//        img->backgroundmpg[img->nmenus]=NULL;
    }
}


int main(int argc,  char* const argv[])
{

    int i=0;
    errno=0;

    if (errno) perror("[ERR]  Initial allocation\n");

    lexer_t   lexer_init;
    lexer_t   *lexer=&lexer_init;
   /* create a static command_t structure
    *  in command_line_parsing.c from default command line created by lexer.
    */




#ifndef __MINGW32__
    struct rusage nothing, start;
    compute_t timer= {&nothing, &start};
    starter(&timer);
#endif

    /*  DEFAULT   SETTINGS  */

    // Locale, time and log management

    setlocale(LC_ALL, "LOCALE");

    char* h = getenv("PWD");
    currentdir=strdup((h)? h : TEMPDIR_SUBFOLDER_PREFIX);
    int currentdirlength=strlen(currentdir);

    char TEMPDIRROOT[currentdirlength+14];
    TEMPDIR=calloc(currentdirlength+20, sizeof(char));

    char *EXECDIR=calloc(MAX(currentdirlength, 20)+4+25, sizeof(char));  // /usr/local/bin or /usr/bin under *NIX, "currentdir" directory/bin otherwise (win32...)
    // 4 for "/bin and be liberal and allow 25 more characters for the executable name.

    char *DATADIR;
    #ifdef __WIN32__
    DATADIR=strdup(currentdir);
    #else
    DATADIR=strdup(INSTALL_CONF_DIR);
    #endif
    char **BGPIC=calloc(2, sizeof(char*));


    BGPIC[0]=strdup(DEFAULT_BACKGROUNDPIC);
//    IMPIC[0]=calloc(currentdirlength+30, sizeof(char));
//    HLPIC[0]=calloc(currentdirlength+30, sizeof(char));
//    SLPIC[0]=calloc(currentdirlength+30, sizeof(char));


    sprintf(TEMPDIRROOT, "%s%s%s", currentdir,(currentdir[0] == 0)? "" : SEPARATOR , TEMPDIR_SUBFOLDER_PREFIX DVDA_AUTHOR_BASENAME);

    sprintf(TEMPDIR, "%s"SEPARATOR"%s", TEMPDIRROOT, "temp");


    // Global settings are hard-code set by default as follows:
    errno=0;

    globalData globals_init=
    {

        /*top menu*/    NO_MENU,  // no top menu
        /*runmkisofs*/  0,  // do not run mkisofs
        /*autoplay*/    0,  // no autoplay
        /*text table*/  0,  // no text table
        /*silence*/     0,
        1,  // enabling lexer
        /*logfile*/	0,  // no log
        /*loghtml*/     0,  //text log
        /*videozone*/   1,  // generates video zone
        /*videolinking*/0,  // no video link
        /*playlist*/    0,  // no playlist
        /*cga*/         0,  // no explicit channel group assignement
        /*end_pause*/   0,  // no end pause
        /*very verbose*/0,  // not very verbose
        /*debugging*/   0,  // no debugging-level verbosity
#if 0
        /*padding*/     1,  // always padding
        /*padding_continuous*/    0,  // no continuous padding
        /*minimal_padding*/       0,  // no minimal padding
        /*lossy_rounding*/ 0,  // No audio loss
#endif
        /*rungrowisofs*/   0,  // Do not burn with growisofs
#ifndef WITHOUT_SOX
        /*sox_enable*/     0,  // no use of SoX
#endif
#ifndef WITHOUT_FIXWAV
        /*fixwav_enable*/               0,  // no use of fixwav
        /*fixwav_virtual_enable*/       0,  // no use of fixwav (virtual headers)
        /* automatic behaviour */       1,
        /* do not prepend a header */   0,
        /* do not correct in place */   0,
        /* be cautious on overwrites*/  0,
        /* not interactive */           0,
        /* no padding */ 0,
        /* prune */      0,
        /* force */      0,
        /* fixwav output suffix*/
        strdup(STANDARD_FIXWAV_SUFFIX),
        /*fixwav_parameters*/ NULL,

#endif
        /*xml filepath*/  NULL,
        /*spumux xml*/    NULL,
        /*cdrecord dev*/  NULL,
        /*journal (log)*/ NULL, //(FILE*)
        /*access rights*/ DEFAULT_ACCESS_RIGHTS,
        /* it is necessary to use strdup as these settings may be overridden dynamically */
// Paths:

        {
            strdup(SETTINGSFILE),
            NULL,  // logfile path should be supplied on command line
            NULL, // input directory path
            NULL,// output directory path
#ifdef __WIN32__
            strdup(DEFAULT_WORKDIR),// working directory: under Windows, c:\ if not defined at compile time, otherwise 'currentdir' environment variable
#else
            strdup(currentdir),
#endif
            NULL,// temporary directory
            NULL,   // videolinked directory path
            EXECDIR, //bindir
            DATADIR,
            /*fixwav_database*/
            strdup(STANDARD_FIXWAV_DATABASE_PATH),
            NULL,
            NULL
        }
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
        &BGPIC[0], // black screen for jpg video mpg authoring
        strdup(DEFAULT_BLANKSCREEN), // black screen for png authoring
        NULL, //backgroundmpg
        NULL, //backgroundcolors
        strdup(DEFAULT_ACTIVEHEADER),
        NULL, //topmenu
        NULL, //stillvob
        NULL, //tsvob
        strdup(DEFAULT_SOUNDTRACK), //soundtrack  silence.wav
        strdup(DEFAULT_ALBUMCOLOR), //top menu pic textcolor
        strdup(DEFAULT_GROUPCOLOR), //top menu pic textcolor
        strdup(DEFAULT_ARROWCOLOR), //top menu pic textcolor
        strdup(DEFAULT_TEXTCOLOR_PIC), //top menu pic textcolor

        DEFAULT_BGCOLOR_PIC, //topmenu pic background color no strdup here (same-string optarg)
        DEFAULT_HCOLOR_PIC, //topmenu pic highlight color  no strdup here (same-string optarg)
        DEFAULT_SELCOLOR_PIC, //topmenu pic select action color no strdup here (same-string optarg)

        strdup(DEFAULT_ACTIVETEXTCOLOR_PALETTE), //top menu pic textcolor
        DEFAULT_ACTIVEBGCOLOR_PALETTE, //topmenu pic background color no strdup here (same-string optarg)
        DEFAULT_ACTIVEHCOLOR_PALETTE, //topmenu pic highlight color  no strdup here (same-string optarg)
        DEFAULT_ACTIVESELCOLOR_PALETTE, //topmenu pic select action color no strdup here (same-string optarg)

        strdup(DEFAULT_TEXTCOLOR_PALETTE), //system palette textcolor
        DEFAULT_BGCOLOR_PALETTE, //palette background color        no strdup here (same-string optarg)
        DEFAULT_HCOLOR_PALETTE, //palette highlight color    no strdup here (same-string optarg)
        DEFAULT_SELCOLOR_PALETTE, //palette select action color red   no strdup here (same-string optarg)

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
        0,  // pic count
        NULL, // npics table
        NULL, // no still pic vob size
        NULL, // no menu vob size
        NULL // still pic options

    };

    globals=globals_init;
    globals.settings.tempdir=TEMPDIR;
#ifdef BINDIR
    memcpy(globals.settings.bindir, BINDIR, strlen(BINDIR));
#else
    memcpy(globals.settings.bindir, currentdir, currentdirlength);
#endif
    normalize_temporary_paths(NULL);

    // Null arg is no longer supported, yet...

    if (argc == 1)
    {
        printf("\n%s", "dvda-author syntax:\n------------------\n");
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


    _Bool project_flag=0;
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

           path_t *pstruct=parse_filepath(project_filepath);
           if (pstruct && pstruct->exists)
           {
             //if (globals.debugging)
             printf("[INF]  Parsing project file %s\n", project_filepath);
           }
           else
           {
            printf("[ERR]  Failed to parse project file %s\n       Exiting...\n", project_filepath);
            clean_exit(EXIT_FAILURE);
           }
           free(pstruct);
        }

    lexer_analysis(command, lexer, SETTINGSFILE, CONFIGURATION_FILE);

        /* launch core processes after parsing user command-line, possibly overriding defaut values */


launch:

        if (project_flag)

          launch_manager(lexer_analysis(command, lexer, project_filepath, PROJECT_FILE));
        else

          launch_manager(command_line_parsing(argc, argv, command));

    // allocated in command_line_parsing()

    /* Compute execution time and exit */

    COMPUTE_EXECTIME

    FREE(currentdir)

    fflush(NULL);
    if ((globals.loghtml) && (globals.logfile)) htmlize(globals.settings.logfile);

    if (globals.end_pause) pause_dos_type();

    return(errno);

}

