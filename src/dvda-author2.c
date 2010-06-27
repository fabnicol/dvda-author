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
char *home, *TEMPDIRROOT, *TEMPDIR, *LOGFILE, *INDIR, *OUTDIR, *LINKDIR;

void normalize_temporary_paths(pic* img)
{
    static size_t s;

    // cannot easily  free(globals.settings.logfile) etc. as embedded  in structure ?
    if (img == NULL)
    {

        s=strlen(globals.settings.tempdir);
        globals.settings.logfile=realloc(globals.settings.logfile, (s+10)*sizeof(char));
        globals.settings.indir=realloc(globals.settings.indir, (s+10)*sizeof(char));
        globals.settings.outdir=realloc(globals.settings.outdir, (s+10)*sizeof(char));
        globals.settings.linkdir=realloc(globals.settings.linkdir, (s+10)*sizeof(char));
        globals.settings.indir=calloc(s+10,1);

        sprintf(globals.settings.logfile, "%s"SEPARATOR"%s", globals.settings.tempdir, "log.txt");
        sprintf(globals.settings.indir, "%s"SEPARATOR"%s", globals.settings.tempdir, "audio");
        sprintf(globals.settings.outdir, "%s"SEPARATOR"%s", globals.settings.tempdir, "output");
        sprintf(globals.settings.linkdir, "%s"SEPARATOR"%s", globals.settings.tempdir, "VIDEO_TS");


    }
    else
    {



        int menu;
            img->backgroundpic=realloc(img->backgroundpic,(img->nmenus+1)*sizeof(char*));
            img->imagepic=realloc(img->imagepic,(img->nmenus+1)*sizeof(char*));
            img->highlightpic=realloc(img->highlightpic,(img->nmenus+1)*sizeof(char*));
            img->selectpic=realloc(img->selectpic, (img->nmenus+1)*sizeof(char*));


        // useless to realloc for just one menu !

        char* img_save=NULL;


        for (menu=0;  menu < img->nmenus; menu++)
        {

            if (img->backgroundpic[menu]) img_save=strdup(img->backgroundpic[menu]);
            img->backgroundpic[menu]=realloc(img->backgroundpic[menu], (s+13)*sizeof(char));
            sprintf(img->backgroundpic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "bgpic", menu, ".jpg");
            if (img_save) copy_file(img_save, img->backgroundpic[menu]);


            img->imagepic[menu]=calloc(s+13, sizeof(char));
            sprintf(img->imagepic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "impic", menu, ".png");


            img->highlightpic[menu]=calloc(s+13,sizeof(char));
            sprintf(img->highlightpic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "hlpic", menu, ".png");

             img->selectpic[menu]=calloc(s+13,sizeof(char));
             sprintf(img->selectpic[menu], "%s"SEPARATOR"%s%d%s", globals.settings.tempdir, "slpic", menu, ".png");


        }
        img->backgroundpic[img->nmenus]=NULL;
        img->imagepic[img->nmenus]=NULL;
        img->highlightpic[img->nmenus]=NULL;
        img->selectpic[img->nmenus]=NULL;



    }
}


int main(int argc,  char* const argv[])
{

    int i=0;
    errno=0;

    if (errno) perror("[ERR]  Initial allocation\n");

    lexer_t   lexer_init;
    lexer_t   *lexer=&lexer_init;

#ifndef __MINGW32__
    struct rusage nothing, start;
    compute_t timer= {&nothing, &start};
    starter(&timer);
#endif

    /*  DEFAULT   SETTINGS  */

    // Locale, time and log management

    setlocale(LC_ALL, "LOCALE");


    char* h = getenv("HOME");
    home=strdup((h)? h : TEMPDIR_SUBFOLDER_PREFIX);
    int homelength=strlen(home);

    char TEMPDIRROOT[homelength+14];
    TEMPDIR=calloc(homelength+20, sizeof(char));

    char *EXECDIR=calloc(MAX(homelength, 20)+4+25, sizeof(char));  // /usr/local/bin or /usr/bin under *NIX, "home" directory/bin otherwise (win32...)
                                                                 // 4 for "/bin and be liberal and allow 25 more characters for the executable name.
    char **BGPIC=calloc(2, sizeof(char*));
    char **IMPIC=calloc(2, sizeof(char*));
    char **HLPIC=calloc(2, sizeof(char*));
    char **SLPIC=calloc(2, sizeof(char*));


    BGPIC[0]=strdup(DEFAULT_BACKGROUNDPIC);
//    IMPIC[0]=calloc(homelength+30, sizeof(char));
//    HLPIC[0]=calloc(homelength+30, sizeof(char));
//    SLPIC[0]=calloc(homelength+30, sizeof(char));


    sprintf(TEMPDIRROOT, "%s%s%s", home,(home[0] == 0)? "" : SEPARATOR , TEMPDIR_SUBFOLDER_PREFIX DVDA_AUTHOR_BASENAME);

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
            strdup(DEFAULT_LOGFILE),  // logfile path
            NULL, // input directory path
            NULL,// output directory path
            #ifdef __WIN32__
            strdup(DEFAULT_WORKDIR),// working directory: under Windows, c:\ if not defined at compile time, otherwise 'home' environment variable
            #else
            strdup(home),
            #endif
            NULL,// temporary directory
            NULL,   // videolinked directory path
            EXECDIR, //bindir
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
//        &HLPIC[0], //highlightpic
//        &SLPIC[0], //selectpic
//        &IMPIC[0], //imagepic
NULL,
NULL,
NULL,
        &BGPIC[0], // black screen for jpg video mpg authoring
        strdup(DEFAULT_BLANKSCREEN), // black screen for png authoring
        NULL, //backgroundmpg
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
    memcpy(globals.settings.bindir, home, homelength);
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

    command_t command0, *command=NULL;

    for (i=1; i < argc ; i++)
        if (strcmp(argv[i],"--disable-lexer") *strcmp(argv[i] , "-W") *strcmp(argv[i], "--version")*strcmp(argv[i], "--help")*strcmp(argv[i], "-v")*strcmp(argv[i], "-h") == 0)
        {
            globals.enable_lexer=0;

            {

                command0.img=&img0;
                command=&command0;
                goto launch;
            }

        }

    lexer->nlines=MAX_LEXER_LINES;

    lexer->commandline=(char** ) calloc(MAX_LEXER_LINES, sizeof(char *));
    if (lexer->commandline == NULL) perror("[ERR]  lexer");
    for (i=0; i < MAX_LEXER_LINES; i++)
    {
        lexer->commandline[i]=(char* ) calloc(2*MAX_OPTION_LENGTH, sizeof(char));
        if (lexer->commandline[i] == NULL) perror("[ERR]  lexer");
    }

    check_settings_file();

    errno=0;
    config_lexer(SETTINGSFILE, lexer);

    /* create a static command_t structure
    *  in command_line_parsing.c from default command line created by lexer.
    */


    command0.img=&img0;
    command=&command0;

    if (command == NULL)
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Could not allocate command-line structure")

        command_line_parsing(lexer->nlines, lexer->commandline, command);

    for (i=0; i < MAX_LEXER_LINES; i++)
        FREE(lexer->commandline[i])

        FREE(lexer->commandline)

        /* launch core processes after parsing user command-line, possibly overriding defaut values */


launch:

        launch_manager(command_line_parsing(argc, argv, command));
    // allocated in command_line_parsing()

    /* Compute execution time and exit */


    COMPUTE_EXECTIME

    FREE(home)

    if (globals.end_pause) pause_dos_type();

    return(errno);

}

