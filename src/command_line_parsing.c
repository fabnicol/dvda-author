#if HAVE_CONFIG_H && !defined __CB__
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include "getopt.h"
#include <sys/time.h>
#include "dvda-author.h"
#include "commonvars.h"
#include "structures.h"
#include "c_utils.h"
#include "audio2.h"
#include "auxiliary.h"
#include "ports.h"
#include "file_input_parsing.h"
#include "launch_manager.h"
#include "dvda-author.h"
#ifndef WITHOUT_FIXWAV
#include "fixwav_auxiliary.h"
#include "fixwav_manager.h"
#endif
#include "command_line_parsing.h"
#include "menu.h"



/*  #define _GNU_SOURCE must appear before <string.h> and <getopt.h> for strndup  and getopt_long*/


globalData globals;
unsigned int startsector;
extern char* OUTDIR, *LOGFILE, *WORKDIR, *TEMPDIR;
static fileinfo_t ** files;
uint16_t totntracks;
uint8_t maxbuttons; // to be used in xml.c and menu.c as extern globals
uint8_t resbuttons; // to be used in xml.c and menu.c as extern globals


#ifdef __WIN32__
DWORD WINAPI log_thread_function()
{
    if (logrefresh)
        freopen(globals.settings.logfile, "wb", stdout);
    else
        freopen(globals.settings.logfile, "ab", stdout);
    return 0;
}
#endif

command_t *command_line_parsing(int argc, char* const argv[], command_t *command)
{

// It is crucial that the reinitialization of optarg be with optind=0, not optind=1

    /* command_t member initialisation: static typing ensures 0-initialisation and
     * preservation of values set at configuration stage */

    static uint8_t user_command_line;
    static uint8_t ngroups;
    uint8_t n_g_groups=0;
    static uint8_t nplaygroups;
    static uint8_t nvideolinking_groups;
    static uint8_t maximum_VTSI_rank;

    static uint8_t VTSI_rank[MAXIMUM_LINKED_VTS];
    static uint8_t ntracks[9];

    static uint8_t playtitleset[9]= {0};
    memset(playtitleset, 0, 9);

    extern char *optarg;
    extern int optind, opterr;
    int k, c;

    int errmsg;
    _Bool allocate_files=0, logrefresh=0, refresh_tempdir=1, refresh_outdir=1;  // refreshing output and temporary directories by default
    DIR *dir;
    parse_t  audiodir;
    extractlist extract;



#ifdef img
#undef img
#endif
#define img command->img  // already allocated, just for notational purposes



    char **argv_scan=calloc(argc, sizeof(char*));

    startsector=-1; /* triggers automatic computing of startsector (Lee and Tim Feldman) */


    /* Initialisation: default group values are overridden if and only if groups are added on command line
     * Other values are left statically determined by first launch of this function                          */

    // By default, videozone is generated ; use -n to deactivate.

    // When lexer is deactivated, parse command line directly.

    if (!globals.enable_lexer) user_command_line=1;

    /* distributed dvda-author.conf silences dafault configuration file option verbosity (q) */
    /* you can alter this by commenting out #q in dvda-author.conf before install */
    /* for parsing user command line, revert to default verbose mode, unless -q is set */

    if (user_command_line) globals.silence=0;




    /* crucial: initialise before any call to getopt */
    optind=0;
    opterr=1;

#ifdef LONG_OPTIONS
    int longindex=0;

    static struct option  longopts[]=
    {

        {"debug", no_argument, NULL, 'd'},
        {"veryverbose", no_argument, NULL, 't'},
        {"fixwav", optional_argument, NULL, 'F'},
        {"fixwav-virtual", optional_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'h'},
        {"input", required_argument, NULL, 'i'},
        {"log", required_argument, NULL, 'l'},
        {"logrefresh", required_argument, NULL, 'L'},
        {"no-videozone", no_argument, NULL, 'n'},
        {"output", required_argument, NULL, 'o'},
        {"autoplay", no_argument, NULL, 'a'},
        {"startsector", required_argument, NULL, 'p'},
        {"pause", optional_argument, NULL, 'P'},
        {"quiet", no_argument, NULL, 'q'},
        {"sox", optional_argument, NULL, 'S'},
        {"videolink", required_argument, NULL, 'T'},
        {"loop", optional_argument, NULL, 'U'},
        {"version", no_argument, NULL, 'v'},
        {"videodir", required_argument, NULL, 'V'},
        {"rights", required_argument, NULL, 'w'},
     //   {"no-padding", no_argument, NULL, '\1'},
     //   {"minimal-padding", no_argument, NULL, '\2'},
        {"extract", required_argument, NULL, 'x'},
        {"disable-lexer", no_argument, NULL, 'W'},
        {"pad-cont", no_argument, NULL, 'C'},
     //   {"lossy-rounding", no_argument, NULL, 'L'},
        {"playlist", required_argument, NULL, 'Z'},
        {"cga", required_argument, NULL, 'c'},
        {"topmenu", optional_argument, NULL, 'm'},
        {"menustyle", required_argument, NULL, '0'},
        {"xml", required_argument, NULL, 'M'},
        {"spuxml", required_argument, NULL, 'H'},
        {"text", optional_argument, NULL, 'k'},
        {"tempdir", required_argument, NULL, 'D'},
        {"workdir", required_argument, NULL, 'X'},
        {"datadir", required_argument, NULL, '9'},
        {"topvob", required_argument, NULL, 'A'},
        {"stillvob", required_argument, NULL, '1'},
        {"stilloptions", required_argument, NULL, '2'},
        {"mkisofs", optional_argument, NULL, 'I'},
        {"cdrecord", optional_argument, NULL, 'r'},
        {"growisofs", required_argument, NULL, 'R'},
        {"highlight", required_argument, NULL, 'E'},
        {"select", required_argument, NULL, 'e'},
        {"image", required_argument, NULL, 'G'},
        {"background", required_argument, NULL, 'b'},
        {"background-colors", required_argument, NULL, 2},
        {"background-mpg", required_argument, NULL, 'B'},
        {"soundtrack", required_argument, NULL, 'Q'},
        {"topmenu-colors", required_argument, NULL, 'y'},
        {"topmenu-palette", required_argument, NULL, 'Y'},
        {"blankscreen", required_argument, NULL, 'N'},
        {"screentext", required_argument, NULL, 'O'},
        {"highlightformat", required_argument, NULL, 'K'},
        {"font", required_argument, NULL, 'J'},
        {"duration", required_argument, NULL, 'u'},
        {"stillpics", required_argument, NULL, '3'},
        {"norm", required_argument, NULL, '4'},
        {"aspect", required_argument, NULL, '5'},
        {"nmenus", required_argument, NULL, '6'},
        {"ncolumns", required_argument, NULL, '7'},
        {"activemenu-palette", required_argument, NULL, '8'},
        {"loghtml", no_argument, NULL, 1},
        {"bindir",required_argument, NULL, 3},
        {"no-refresh-tempdir",no_argument, NULL, 4},
        {"no-refresh-outdir",no_argument, NULL, 5},
        {NULL, 0, NULL, 0}
    };
#endif



    /* getopt is now used for command line parsing. To ensure compatibility with prior "Dave" versions, the easier way out
     *  is to duplicate the command line. Otherwise getopt reorders options/non-options and multiple arguments of -g ...
     *  are consequently misplaced */

    /* 0-reset only on command-line parsing in case groups have been defined in config file */

    for (k=0; k<argc; k++)
        if ((argv_scan[k]=strdup(argv[k])) == NULL)
            EXIT_ON_RUNTIME_ERROR
            /* COMMAND-LINE  PARSING: first pass for global behaviour option: log, help, version, verbosity */


#ifdef LONG_OPTIONS
            while ((c=getopt_long(argc, argv_scan, ALLOWED_OPTIONS, longopts, &longindex)) != -1)
#else
            while ((c=getopt(argc, argv_scan, ALLOWED_OPTIONS)) != -1)
#endif

            {
                switch (c)
                {
                    /* On modern *nix platform this trick ensures long --help and --version options even if LONG_OPTIONS
                     * is not defined. Not operational with Mingw to date 	*/


#ifndef LONG_OPTIONS
                case '-' :
                    if  (strcmp(optarg, "version") == 0)
                    {
                        version();
                        break;
                    }

                    if  (strcmp(optarg, "help") == 0)
#endif

                    case 'h' :
                    globals.silence=0;
                    help();
                    clean_exit(EXIT_SUCCESS);
                    break;

                case 'v' :
                    globals.silence=0;
                    version();
                    clean_exit(EXIT_SUCCESS);
                    break;

                case 'q' :
                    globals.silence=1;
                    globals.debugging=0;
                    // Radical measure yet not portable outside the *nix realm ?

                    break;

                case 't':
                    globals.veryverbose=1;

                    // no break

                case 'd':

                    globals.debugging=1;
                    globals.silence=0;
                    break;


               case 'L':
                    logrefresh=1; // no break

               case 'l' :



                    if (optarg)
                    {
                      globals.settings.logfile=strndup(optarg, MAX_OPTION_LENGTH);
                      globals.logfile=1;
                      if (optarg[0] == '-')
                          { globals.logfile=0; printf("%s\n", "[ERR]  Enter a log path next time!"); exit(EXIT_FAILURE);}

                    }

                    break;

              case 1 :
                    globals.loghtml=1;

                    break;


                }
            }



    if (globals.silence)
        freopen(LOGFILE, "ab", stdout); // to hush up stdout only.

#ifndef __WIN32__
    if (globals.logfile)
    {
     if (user_command_line)
        switch (fork())
        {
        case -1 :
            break;
        case  0 :

            if (logrefresh)
                freopen(globals.settings.logfile, "wb", stdout);
            else
                freopen(globals.settings.logfile, "ab", stdout);


        }
    }
#else

#include <windows.h>


    HANDLE a_thread;
    DWORD a_threadId;

  // Create a new thread.
    a_thread = CreateThread(NULL, 0, logthread_function, NULL,
0, &a_threadId);

    if (a_thread == NULL)
    {
        perror("[ERR]  Thread creation failed");
        htmlize(globals.settings.logfile)
        return(EXIT_FAILURE);
    }

#endif

    if (((user_command_line) || (!globals.enable_lexer)) && (!globals.silence))
    {
        HEADER(PROGRAM, VERSION)
        SINGLE_DOTS
    }

    optind=0;
    opterr=1;



    if (user_command_line)
    {

        int j;
        for (j=0; j < argc; j++)
#ifdef LONG_OPTIONS
            while ((c=getopt_long(argc, argv_scan, ALLOWED_OPTIONS, longopts, &longindex)) != -1)
#else
            while ((c=getopt(argc, argv_scan, ALLOWED_OPTIONS)) != -1)
#endif
            {
                switch (c)
                {
                case 's':    // single-track group file input (command line)
                case 'j':  // join-group file input (command line)
                case 'g':  // normal group file input (command line)
                case 'T':  // video title input
                case 'i':  // directory audio imput


                    memset(ntracks, 0, 9);
                    ngroups=nvideolinking_groups=n_g_groups=0;
                    if (globals.veryverbose)
                        printf("%s\n", "[INF]  Overriding configuration file specifications for audio input");
                    // Useless to continue parsing
                    //reset++;
                    break;

                case 'D' :
                    FREE(globals.settings.tempdir);
                    globals.settings.tempdir=strndup(optarg, MAX_OPTION_LENGTH);
                    printf("%s%s\n", "[PAR]  Temporary directory is: ", optarg);
                    normalize_temporary_paths(NULL);
                    break;


                case 'X':
                    free(globals.settings.workdir);
                    globals.settings.workdir=strndup(optarg, MAX_OPTION_LENGTH);
                    printf("%s%s\n", "[PAR]  Working directory is: ", optarg);
                    // WARNING never launch a command line with --mkisofs in the WORKDIR directory
                    change_directory(globals.settings.workdir);
                    //reset++;
                    break;


                }
            }

    }

    optind=0;
    opterr=1;


    if ((globals.debugging) && (user_command_line))
    {
        printf("%s\n", "[INF]  Parsing user command line");
        print_commandline(argc, argv);

        printf("%c", '\n');
    }

    if (globals.logfile) printf("%s%s\n", "[PAR]  Log file is: ", globals.settings.logfile);


    /* COMMAND-LINE PARSING: second pass to determine memory allocation (thereby avoiding heap loss)
     * We give up getopt here to allow for legacy "Dave" syntax with multiple tracks as -g arguments
     * (not compatible with getopt or heavy to implement with it.  */

    if (globals.debugging) printf("%s\n", "[INF]  First scan of track list for memory allocation...");

    // n_g_groups count command-line groups of type -g, -j (join groups), -s (single track groups)



    for (k=1; k < argc; k++)
    {
        if (argv[k][0] != '-') continue;
        switch (argv[k][1])
        {

        case 'j' :
            printf("%s\n", "[PAR]  Join group");
        case 's' :
        case 'g' :

            k++;

            for (; k < argc; k++)
            {
                /*  To explicitly change titles within the same group even if files_i and file_i+1 have same audio characterictics, use:
                    -g/-j/-s file_1 ... file_i -| file_i+1 file_i+2 ... -g ...
                */
                // PATCH 09.07


                if (argv[k][0] !='-')
                     ntracks[n_g_groups]++;
                else
                {
                    if (argv[k][1] == 'z')
                        continue;
                    else
                        break;
                }

            }

            increment_ngroups_check_ceiling(&n_g_groups, NULL);
            k--;
        }
    }

    ngroups += n_g_groups;

    optind=0;
    opterr=1;

    fileinfo_t **files_dummy=NULL;

    /* COMMAND-LINE PARSING: third pass  to determine memory allocation with non-g options and getopt */
    static int u;


#ifdef LONG_OPTIONS
    while ((c=getopt_long(argc, argv_scan, ALLOWED_OPTIONS, longopts, &longindex)) != -1)
#else
    while ((c=getopt(argc, argv_scan, ALLOWED_OPTIONS)) != -1)
#endif

    {

        switch (c)
        {


        case 'w':
            /* input must be in octal form */
            globals.access_rights=(mode_t) strtol(optarg, NULL, 8);
            printf("[PAR]  Access rights (octal mode)=%o\n", globals.access_rights);
            break;

        case 's' :
            printf("%s\n", "[PAR]  Single track group");
        case 'g' :
        case 'j' :
            u++;
            allocate_files=1;
            break;

        case 'i' :

            allocate_files=1;
            globals.settings.indir=strndup(optarg, MAX_OPTION_LENGTH);

            printf("%s%s\n", "[PAR]  Input directory is: ", 	optarg);
            DIR *dir;

            if ((dir=opendir(optarg)) == NULL)
                EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Input directory could not be opened")

                change_directory(globals.settings.indir);
            audiodir=parse_directory(dir, ntracks, n_g_groups, 0, files_dummy);

            ngroups=audiodir.ngroups;

            memmove(ntracks, audiodir.ntracks, 9*sizeof(uint8_t));

            if (closedir(dir) == -1)
                printf( "%s\n", "[ERR]  Impossible to close dir");

            change_directory(globals.settings.workdir);

            break;

        case 'o' :

            globals.settings.outdir=strndup(optarg, MAX_OPTION_LENGTH);
            printf("%s%s\n", "[PAR]  Output directory is: ", optarg);

            break;


        case 5:
            refresh_outdir=0;
            break;

        case 4:
            refresh_tempdir=0;
            break;


        case 'T':

            allocate_files=1;

            if (nvideolinking_groups == MAXIMUM_LINKED_VTS)
            {
                printf("[ERR]  Error: there must be a maximum of %d video linking groups\n      Ignoring additional links...\n\n", MAXIMUM_LINKED_VTS);
                break;
            }

            // VTSI_rank is the rank of the VTS that is linked to in VIDEO_TS directory
            VTSI_rank[nvideolinking_groups]=atoi(optarg);

            if   (VTSI_rank[nvideolinking_groups] > 99)
                EXIT_ON_RUNTIME_ERROR_VERBOSE( "[ERR]  There must be a maximum of 99 video titlesets in video zone. Try again...\n\n")

                if (nvideolinking_groups == 0)
                    maximum_VTSI_rank=VTSI_rank[nvideolinking_groups];
                else
                    maximum_VTSI_rank=MAX(VTSI_rank[nvideolinking_groups], maximum_VTSI_rank);

            globals.videozone=1;
            globals.videolinking=1;

            increment_ngroups_check_ceiling(&ngroups, &nvideolinking_groups);

            break;

            // Should be done as early as main globals are set, could be done a bit earlier (adding an extra parse)
#ifndef WITHOUT_FIXWAV

        case 'F' :
        case 'f' :


            /* adjusting fixwav library globals to current ones
             * use local variables to initialise */

            if ((optarg != NULL) && (strstr(optarg, "help")))
            {
                clean_exit(EXIT_SUCCESS);
            }
            break;

#endif


        }
    }

    /* Here the group parameters are known: ngroups (total),  n_g_groups (legacy -g syntax), nvideolinking_groups */

    /* command line copy is now useless: freeing space */

    for (k=0; k<argc; k++)
        FREE(argv_scan[k])
        FREE(argv_scan)

        /* Performing memory allocation (calloc)
        *
        *  Ordering
        *  -----------
        *  Groups are ordered according to the following order : g-type groups (-g, -j, -s) < directory groups < video-linking groups
        *
        *  Allocation
        *  ------------
        *  g-type groups are granted "for free" as they are allocated by command-line argv parsing
        *  Directory groups are costly as they must bee allocated freshly
        *  Video-linking groups have just one track and are reordered with highest ranks */


        /* Allocate memory if and only if groups are to be (re)created on command line */

        if (allocate_files)
        {
            files=dynamic_memory_allocate(files, ntracks, ngroups, n_g_groups, nvideolinking_groups);
        }

    /* COMMAND-LINE PARSING: fourth pass to assign filenames without allocating new memory (pointing to argv) */


    int m, ngroups_scan=0;

    if ((n_g_groups)&&(globals.debugging)) printf("%s", "[INF]  Assigning command-line filenames...\n");

    for (k=0; k < argc; k++)
    {
        if (argv[k][0] != '-') continue;
        switch (argv[k][1])
        {

        case 's' :
            files[ngroups_scan][0].single_track=1;   // no break
        case 'j' :
            files[ngroups_scan][0].contin=1;
            if (globals.debugging) printf("%s%d\n", "[MSG]  Continuity requested for group ", ngroups_scan+1);
            files[ngroups_scan][0].join_flag=1;     //  no break
            if (globals.debugging) printf("%s%d\n", "[MSG]  Join flag set for group ", ngroups_scan+1);

        case 'g' :

            k++;
            for (m=0; m+k < argc; m++)
            {
                if (argv[m+k][0] !='-')
                {
                    if (globals.veryverbose)
                        printf("       files[%d][%d].filename=%s\n", ngroups_scan, m, argv[m+k]);
                    files[ngroups_scan][m].filename=strdup(argv[m+k]);
                    /* to create distinct titles out of a series of audio files which have same audio characteristics, use -| in between the series of files
                       of each title within the same group. This is not authorized to cleave groups, only titles */
                }
                else
                {
                    //PATCH 09.07 (overflow)
                    if (argv[m+k][1] == 'z')
                    {
                        if (m < ntracks[ngroups_scan])
                        {

                            files[ngroups_scan][m].newtitle= 1;
                            k++;
                            if (globals.veryverbose)
                                printf("       files[%d][%d].filename=%s\n", ngroups_scan, m, argv[m+k]);
                            if ((m+k) < argc) files[ngroups_scan][m].filename=strdup(argv[m+k]);
                        }
                    }
                    else
                        break;
                }
            }

            k+=m-1;
            ngroups_scan++;
            break;

        case 'c' :

            k++;
            globals.cga=1;
            for (m=0; (m+k < argc)&&(argv[m+k][0] !='-'); m++)
            {
                if (globals.debugging) printf("       files[%d][%d].cga=%s\n", ngroups_scan, m, argv[m+k]);
                uint8_t cgaint=atoi(argv[m+k]);

                if (check_cga_assignment(cgaint))
                    files[ngroups_scan][m].cga=cgaint;
                else if (globals.debugging) printf("%s", "[ERR]  Found illegal channel group assignement value, using standard settings.");
            }
            k+=m-1;
        }
    }

    /* COMMAND-LINE  PARSING: fourth pass for main arguments and non-g filename assignment */
    // Changing scanning variable names for ngroups_scan and nvideolinking_groups_scan
   if (totntracks == 0)
        for (k=0; k < ngroups-nvideolinking_groups; k++)
            totntracks+=ntracks[k];
    ngroups_scan=0;
    int nvideolinking_groups_scan=0, strlength=0;
    char* piccolorchain, *activepiccolorchain, *palettecolorchain, *fontchain, *durationchain=NULL,  *h, *min, *sec, **textable=NULL, **tab=NULL,**tab2=NULL, *stillpic_string=NULL;
    uint16_t npics[totntracks];

    optind=0;
    opterr=1;

#ifdef LONG_OPTIONS
    while ((c=getopt_long(argc, argv, ALLOWED_OPTIONS, longopts, &longindex)) != -1)
#else
    while ((c=getopt(argc, argv, ALLOWED_OPTIONS)) != -1)
#endif
    {
        switch (c)
        {

       // case 'g': c=0; break;
        case '9':
            /* --datadir is the directory  where the menu/ files are located. Under* nix it automatically installed under /usr/share/applications/dvda-author by the autotools
               With other building modes or platforms however, it may be useful to indicate where the menu/ directory will be*/
            // We use realloc here to allow for prior allocation (.conf file etc.) without memory loss

            printf("[PAR]  Using data directory %s\n", optarg);
            strlength=strlen(optarg);
            img->blankscreen=realloc(img->blankscreen, (strlength+1+1+5+strlen(DEFAULT_BLANKSCREEN))*sizeof(char));
            if (img->blankscreen) sprintf(img->blankscreen, "%s"SEPARATOR"%s", optarg, "menu"SEPARATOR"black_" NORM ".png");
            img->backgroundpic[0]=realloc(img->backgroundpic[0], (strlength+1+1+5+strlen(DEFAULT_BACKGROUNDPIC))*sizeof(char));
            if (img->backgroundpic[0]) sprintf(img->backgroundpic[0], "%s"SEPARATOR"%s", optarg, "menu"SEPARATOR"black_" NORM ".jpg");
            img->soundtrack=realloc(img->soundtrack, (strlength+1+1+16)*sizeof(char)); // "silence.wav"
            if (img->soundtrack) sprintf(img->soundtrack, "%s"SEPARATOR"%s", optarg, "menu"SEPARATOR"silence.wav");
            img->activeheader=realloc(img->activeheader, (strlength+1+1+17)*sizeof(char));  // activeheader
            if (img->activeheader) sprintf(img->activeheader, "%s"SEPARATOR"%s", optarg, "menu"SEPARATOR"activeheader");
            break;


        case 'A':

            printf("%s%s\n", "[PAR]  topmenu VOB: ", optarg);
            img->tsvob=strdup(optarg);
            globals.topmenu=Min(globals.topmenu, TS_VOB_TYPE);

            break;

        case '0':
            if (strcmp(optarg, "hierarchical") == 0)
            {
                img->hierarchical=1;
            }
            else if (strcmp(optarg, "active") == 0)
            {
                img->active=1;
                globals.topmenu=Min(globals.topmenu, ACTIVE_MENU_ONLY);
                img->npics =(uint16_t*) calloc(totntracks, sizeof(uint16_t));
                for (k=0; k < totntracks; k++)
                    img->npics[k]=1;

                img->stillpicvobsize=(uint32_t*) calloc(totntracks, sizeof(uint32_t));
            }

            break;

        case '1':

            if (!img->active)
            {
                printf("%s%s\n", "[PAR]  still pictures VOB: ", optarg);
                img->stillvob=strndup(optarg, MAX_OPTION_LENGTH);
            }
               img->npics =(uint16_t*) calloc(totntracks, sizeof(uint16_t));
            for (k=0; k < totntracks; k++)
                img->npics[k]=1;
            img->stillpicvobsize=(uint32_t*) calloc(totntracks, sizeof(uint32_t));
            break;

            //  'x' Must come AFTER 'o' and 'w'

        case 'I':
            printf("%s\n", "[PAR]  Run mkisofs to author disc image.");
            globals.runmkisofs=1;
            if (optarg)
            {
                free(globals.settings.dvdisopath);
                globals.settings.dvdisopath=strndup(optarg, MAX_OPTION_LENGTH);
                printf("%s%s\n", "[PAR]  ISO file path is: ", optarg);
            }
            break;

        case 'r':
            printf("%s\n", "[PAR]  Make ISO image then run cdrecord to burn disc image.");
            globals.runmkisofs=1;
            if ((optarg) && (strlen(optarg) >4 ) )
                globals.cdrecorddevice=strndup(optarg, MAX_OPTION_LENGTH);
            else
            {
                printf("%s%s%s\n", "[WAR]  Device command ", (optarg)?optarg:"", " will be interpolated.\n       Run cdrecord -scanbus to check for available drivers");
                globals.cdrecorddevice=strdup("");
            }
            break;

        case 'R':
            printf("%s\n", "[PAR]  Make ISO image the run growisofs to burn disc image.");
            globals.runmkisofs=1;
            globals.rungrowisofs=1;
            if ((optarg) && (strlen(optarg) >4 ) )
                globals.cdrecorddevice=strndup(optarg, MAX_OPTION_LENGTH);
            break;


        case 'a' :
            printf("%s\n", "[PAR]  Autoplay on.");
            globals.autoplay=1;
            break;

        case 't' :
            printf("%s\n", "[PAR]  Enhanced debugging-level verbosity");
            break;

        case 'd' :
            printf("%s\n", "[PAR]  Debugging-level verbosity");
            break;

        case 'x' :

            extract_list_parsing(optarg, &extract);
            ats2wav_parsing(optarg, &extract);
            break;

        case 'T':

            ngroups_scan++;
            nvideolinking_groups_scan++;

            // allowing for a single title in video-linking group
            //  videolinkg groups are allocated in last position whatever the form of the command line
            ntracks[ngroups-nvideolinking_groups+nvideolinking_groups_scan-1]=0;

            files[ngroups-nvideolinking_groups+nvideolinking_groups_scan-1][0].first_PTS=0x249;
            // all other characteristics of videolinking titles ar null (handled by memset)

            break;

        case 'V' :
            //  video-linking directory to VIDEO_TS structure

            globals.videozone=1;
            free(globals.settings.linkdir);
            globals.settings.linkdir=strndup(optarg, MAX_OPTION_LENGTH);
            printf("%s%s\n", "[PAR]  VIDEO_TS input directory is: ", optarg);

            break;

        case 'n' :
            // There is no videozone in this case

            if (globals.videolinking)
            {
                printf("%s\n", "[WAR]  You cannot delete video zone with -n if -V is activated too.\n      Ignoring -n...");
                break;
            }
            globals.videozone=0;
            printf("%s\n", "[PAR]  No video zone");
            break;

        case 'i' :


            if ((dir=opendir(optarg)) == NULL)
                EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Input directory could not be opened")

                change_directory(globals.settings.indir);

            parse_directory(dir, ntracks, n_g_groups, READTRACKS, files);

            change_directory(globals.settings.workdir);

            if (closedir(dir) == -1)
                printf( "%s\n", "[ERR]  Impossible to close dir");

            /* all-important, otherwise irrelevant EXIT_ON_RUNTIME_ERROR will be generated*/

            errno=0;

            break;


        case 'p' :

            startsector=(int32_t) strtoul(optarg, NULL, 10);
            errmsg=errno;
            switch (errmsg)
            {
            case   EINVAL :
                printf( "%s\n",  "[ERR]  Incorrect offset value");
                clean_exit(EXIT_SUCCESS);
                break;
            case   ERANGE :
                EXIT_ON_RUNTIME_ERROR_VERBOSE( "[ERR]  Offset range--overflowing LONG INT.");
                break;
            }
            errno=0;

            if (startsector)
                printf("[MSG]  Using start sector: %"PRId32"\n", startsector);
            else
            {
                printf("[ERR]  Illegal negative start sector of %"PRId32"...falling back on automatic start sector\n", startsector);
                startsector=-1;
            }

            break;

        case 'P':
            if ((optarg != NULL) && (strcmp(optarg, "0") == 0))
            {
                globals.end_pause=0;
                printf("%s\n", "[PAR]  End pause will be suppressed.");
            }
            else
            {
                globals.end_pause=1;
                printf("%s\n", "[PAR]  Adding end pause.");
            }
            break;


        case 'U':

            printf("%s", "[PAR]  Loop menu background video\n");
            img->loop=1;
            break;

#ifndef WITHOUT_FIXWAV
        case 'f':
            globals.fixwav_virtual_enable=1;
            printf("%s\n", "[PAR]  Virtual fixwav enabled.");
            // case 'F' must follow breakless

        case 'F':

            /* Uses fixwav to fix bad headers*/

            globals.fixwav_enable=1;
            globals.fixwav_parameters=optarg;
            globals.fixwav_automatic=1; /* default */
            printf("%s\n", "[PAR]  Bad wav headers will be fixed by fixwav");
            if (optarg != NULL)
            {
                printf("%s%s\n", "[PAR]  fixwav command line: ", optarg);
                /* sub-option analysis */
                fixwav_parsing(globals.fixwav_parameters);
            }


            break;
#endif

#ifndef WITHOUT_SOX

        case 'S':

            /* Uses sox to convert different input formats */
            globals.sox_enable=1;
            printf("%s\n", "[PAR]  Audio formats other than WAV and FLAC will be converted by sox tool.");

            break;
#endif
#if 0
        case 1 :
            globals.padding=0;
            if (globals.lossy_rounding)
            {
                globals.lossy_rounding=0;
                printf("%s\n", "[PAR]  --lossy-rounding was neutralized.");
            }

            printf("%s\n", "[PAR]  No audio padding will be performed by core dvda-processes.");
            break;

        case 2 :
            globals.minimal_padding=1;
            printf("%s\n", "[PAR]  Minimal padding of audio samples (for evenness).");
            break;

        case 'L' :
            globals.lossy_rounding=1;
            if (globals.padding)
            {
                globals.padding=0;
                printf("%s\n", "[PAR]  Default padding was neutralized.");
            }
            if (globals.padding_continuous)
            {
                globals.padding_continuous=0;
                printf("%s\n", "[PAR]  --pad-cont was neutralized");
            }

            printf("%s\n", "[PAR]  Sample count rounding will be performed by cutting audio files.");
            break;
#endif
#if 0
        case 'C' :
            globals.padding_continuous=1;

            if (globals.padding == 0)
            {
                globals.padding=1;
                printf("%s\n", "[PAR]  --no-padding was neutralized");
            }
            if (globals.lossy_rounding)
            {
                globals.lossy_rounding=0;
                printf("%s\n", "[PAR]  --lossy-rounding was neutralized");
            }
            printf("%s\n", "[PAR]  Pad with last known byte, if padding, not 0s.");
            break;
#endif
        case 'W' :
            printf("%s\n", "[PAR]  Lexer was deactivated");
            globals.enable_lexer=0;
            break;

        case 'Z' :
            printf("%s%d\n", "[PAR]  Duplicate group #", nplaygroups+1);
            globals.playlist=1;
            nplaygroups++;
            if (nplaygroups > 8)
            {
                if (globals.debugging) printf("%s\n", "[ERR]  There cannot be more than 9 copy groups. Skipping...");
                break;
            }
            playtitleset[nplaygroups]=atoi(optarg);

            break;

        case 'c' :
            printf("%s\n", "[PAR]  Channel group assignement activated.");
            globals.cga=1;
            break;

        case 'm' :

            if (optarg)
            {
                if (img->backgroundmpg == NULL)
                    img->backgroundmpg=calloc(1, sizeof(char*));
                if (img->backgroundmpg == NULL) perror("[ERR]  img->backgroundmpg\n");
                img->backgroundmpg[0]=strndup(optarg, MAX_OPTION_LENGTH);
                printf("[PAR]  Top background mpg file %s will be used\n", img->backgroundmpg[0]);
                printf("%s", "[PAR]  Lower-level authoring options are overruled.\n");
                globals.topmenu=Min(globals.topmenu, RUN_DVDAUTHOR);
            }
            else
            {
                printf("%s\n", "[PAR]  Automatic generation of top menu...");
                globals.topmenu=Min(globals.topmenu, AUTOMATIC_MENU);
            }

            break;

        case 'k' :
            printf("%s","[PAR]  Generates text table in IFO files.\n\n");
            globals.text=1;
            textable=fn_strtok(optarg, ',' , textable, 0,NULL,NULL);
            break;

        case 'M' :
            printf("%s%s\n", "[PAR]  dvdauthor Xml project: ", optarg);
            globals.xml=strndup(optarg, MAX_OPTION_LENGTH);
            globals.topmenu=Min(globals.topmenu, RUN_DVDAUTHOR);
            break;

        case 'H' :
            printf("%s%s\n", "[PAR]  spumux Xml project: ", optarg);
            static int spurank;
            while (spurank >= img->nmenus) 	img->nmenus++;
            if (img->nmenus) globals.spu_xml=realloc(globals.spu_xml, img->nmenus*sizeof(char*));
            globals.spu_xml[spurank++]=strndup(optarg, MAX_OPTION_LENGTH);
            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            break;




        case 'B':
            printf("%s%s\n", "[PAR]  background mpg video: ", optarg);
            if (img->backgroundmpg == NULL)
                img->backgroundmpg=calloc(1, sizeof(char*));
            if (img->backgroundmpg == NULL) perror("[ERR]  img->backgroundmpg\n");
            img->backgroundmpg[0]=strndup(optarg, MAX_OPTION_LENGTH);
            printf("[PAR]  Top background mpg file %s will be used\n", img->backgroundmpg[0]);
            printf("%s", "[PAR]  Lower-level authoring options are overruled.\n");

            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            break;

        case 'u':

            printf("%s%s\n", "[PAR]  duration of background mpg video: ", optarg);
            durationchain=strndup(optarg, MAX_OPTION_LENGTH);

            h=strtok(durationchain, ":");
            min=strtok(NULL, ":");
            sec=strtok(NULL, ":");
            if ((h == NULL) || (min == NULL) || (sec == NULL))
            {
                printf("%s\n", "[ERR]  format must be --duration hh:mm:ss");
                break;
            }
            img->h=atoi(h);
            img->min=atoi(min);
            img->sec=atoi(sec);

            break;



        case 'Q':

            if (img->backgroundmpg)
            {
                printf("%s\n", "[ERR]  Background mpg file already specified, skipping...");
                break;
            }
            free(img->soundtrack);
            printf("%s%s\n", "[PAR]  soundtrack to be muxed into background mpg video: ", optarg);
            img->soundtrack=strndup(optarg, MAX_OPTION_LENGTH);
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            break;



        case 'Y':
            palettecolorchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (palettecolorchain)
            {
                free(img->textcolor_palette);
                img->textcolor_palette= strtok(palettecolorchain, ":");
                //img->bgcolor_palette=strdup(strtok(NULL, ":"));
                img->highlightcolor_palette=strdup(strtok(NULL, ":"));
                img->selectfgcolor_palette=strdup(strtok(NULL, ":"));
                if ((img->selectfgcolor_palette == NULL)|| (img->highlightcolor_palette ==NULL) ||  (img->textcolor_palette ==NULL))
                    EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Color chain is illegal: enter text:highlight:select color separated by a colon");
                errno=0;
                if (img->textcolor_palette) printf("[PAR]  Top menu palette text color: %s %lx\n", img->textcolor_palette, strtoul(img->textcolor_palette,NULL,16));
                //if (img->textcolor_palette) printf("[PAR]  Top menu palette background color: %s %lx\n", img->bgcolor_palette, strtoul(img->bgcolor_palette,NULL,16));
                if (img->textcolor_palette) printf("[PAR]  Top menu palette highlight color: %s %lx\n", img->highlightcolor_palette, strtoul(img->highlightcolor_palette,NULL,16));
                if (img->textcolor_palette) printf("[PAR]  Top menu palette select action color: %s %lx\n", img->selectfgcolor_palette, strtoul(img->selectfgcolor_palette,NULL,16));

                if (errno == ERANGE)
                {
                    EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  At least one YCrCb coding overflows: check switch --palette")
                }
                else
                {
                    if (errno)
                        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Check switch --palette")
                    }
            }
            else
                EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Color chain could not be allocated");

            break;

        case 'y':
            piccolorchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (piccolorchain)

            {
                if (strcmp(piccolorchain, "norefresh") == 0)
                {
                    img->refresh=0;
                    printf("%s\n", "[PAR]  Menu pics will not be refreshed...");
                    break;
                }


                free(img->textcolor_pic);
                img->textcolor_pic= strtok(piccolorchain, ":");
                img->bgcolor_pic=strdup(strtok(NULL, ":"));
                img->highlightcolor_pic=strdup(strtok(NULL, ":"));
                img->selectfgcolor_pic=strdup(strtok(NULL, ":"));
                if ((img->selectfgcolor_pic == NULL)|| (img->highlightcolor_pic ==NULL) || (img->bgcolor_pic == NULL) || (img->textcolor_pic ==NULL))
                    EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Picture color chain is illegal: enter text,background,highlight,select color\n        separated by a colon, with rgb components by commas");
                if (img->textcolor_pic) printf("[PAR]  Top menu text color: rgb(%s)\n", img->textcolor_pic);
                if (img->bgcolor_pic) printf("[PAR]  Top menu background color: rgb(%s)\n", img->bgcolor_pic);
                if (img->highlightcolor_pic) printf("[PAR]  Top menu highlight color: rgb(%s)\n", img->highlightcolor_pic);
                if (img->selectfgcolor_pic) printf("[PAR]  Top menu select action color: rgb(%s)\n", img->selectfgcolor_pic);

            }
            else
                EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Picture color chain could not be allocated");

            if ((strcmp(img->selectfgcolor_pic, img->highlightcolor_pic) == 0) || (strcmp(img->textcolor_pic, img->highlightcolor_pic) == 0) || (strcmp(img->textcolor_pic, img->selectfgcolor_pic) == 0))
            {
                printf("%s\n", "[WAR]  You should use different color values for menu pics: resetting to defaults");
                free(img->textcolor_pic);
                free(img->highlightcolor_pic);
                free(img->selectfgcolor_pic);
                img->textcolor_pic=strdup(DEFAULT_TEXTCOLOR_PIC);
                img->highlightcolor_pic=strdup(DEFAULT_HCOLOR_PIC);
                img->selectfgcolor_pic=strdup(DEFAULT_SELCOLOR_PIC);
            }


            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;

            break;


        case '8':
            activepiccolorchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (activepiccolorchain)

            {
                if (strcmp(activepiccolorchain, "norefresh") == 0)
                {
                    img->refresh=0;
                    printf("%s\n", "[PAR]  Active menu pics will not be refreshed...");
                    break;
                }


                free(img->activetextcolor_palette);
                img->activetextcolor_palette= strtok(activepiccolorchain, ":");
                img->activebgcolor_palette=strdup(strtok(NULL, ":"));
                img->activehighlightcolor_palette=strdup(strtok(NULL, ":"));
                img->activeselectfgcolor_palette=strdup(strtok(NULL, ":"));
                if ((img->activeselectfgcolor_palette == NULL)|| (img->activehighlightcolor_palette ==NULL) || (img->activebgcolor_palette == NULL) || (img->activetextcolor_palette ==NULL))
                    EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Active picture color chain is illegal: enter text,background,highlight,select color\n        separated by a colon, with rgb components by commas");
                if (img->activetextcolor_palette) printf("[PAR]  Active menu text color: rgb(%s)\n", img->activetextcolor_palette);
                if (img->activebgcolor_palette) printf("[PAR]  Active menu background color: rgb(%s)\n", img->activebgcolor_palette);
                if (img->activehighlightcolor_palette) printf("[PAR]  Active menu highlight color: rgb(%s)\n", img->activehighlightcolor_palette);
                if (img->activeselectfgcolor_palette) printf("[PAR]  Active menu select action color: rgb(%s)\n", img->activeselectfgcolor_palette);

            }
            else
                EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Active picture color chain could not be allocated");

            if ((strcmp(img->activeselectfgcolor_palette, img->activehighlightcolor_palette) == 0) || (strcmp(img->activetextcolor_palette, img->activehighlightcolor_palette) == 0) || (strcmp(img->activetextcolor_palette, img->activeselectfgcolor_palette) == 0))
            {
                printf("%s\n", "[WAR]  You should use different color values for active menu pics: resetting to defaults");
                free(img->activetextcolor_palette);
                free(img->activehighlightcolor_palette);
                free(img->activeselectfgcolor_palette);
                img->activetextcolor_palette=strdup(DEFAULT_ACTIVETEXTCOLOR_PALETTE);
                img->activehighlightcolor_palette=strdup(DEFAULT_ACTIVEHCOLOR_PALETTE);
                img->activeselectfgcolor_palette=strdup(DEFAULT_ACTIVESELCOLOR_PALETTE);
            }


            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;

            break;

        case 'O':
            img->screentextchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (globals.veryverbose) printf("%s %s\n", "[PAR]  Screen textchain is:", img->screentextchain);
            globals.topmenu=Min(globals.topmenu, AUTOMATIC_MENU);
            img->refresh=1;
            break;

        case 'K':
            img->highlightformat=(int8_t) atoi(optarg);
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;

        case '3':

            stillpic_string=strdup(optarg);
            break;

        case 'J':
            fontchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (fontchain)
            {
                free(img->textfont);
                img->textfont=strtok(fontchain, ",");
                img->pointsize=(int8_t) atoi(strtok(NULL, ","));
                img->fontwidth=(int8_t) atoi(strtok(NULL, ","));
                if ((img->textfont == NULL)|| (img->pointsize <1) || (img->fontwidth < 1) )
                    EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Font chain is illegal: enter font,font size,font width (width in pixels for size=10)");

                if (img->textfont) printf("[PAR]  Font: %s\n", img->textfont);
                if (img->pointsize) printf("[PAR]  Point size: %d\n", img->pointsize);
                if (img->fontwidth) printf("[PAR]  Font width: %d\n", img->fontwidth);

            }
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;

        case '2':
            still_options_parsing(optarg, img);
            break;

        case '4':
            /* default is PAL, 25 */
            if (strcasecmp(optarg,"ntsc") == 0)
            {
                img->framerate[1]='4';
                img->norm[0]='n';
            }
            else if (strcasecmp(optarg,"pal") == 0)
                img->norm[0]='p';
            else if (strcasecmp(optarg,"secam") == 0)
                img->norm[0]='s';
            else
                printf("%s\n","[ERR]  Only options are 'ntsc', 'secam' or (default) 'pal'.");
            break;

        case '5':

            img->aspect=optarg;
            if (optarg[0] == '1')
                img->aspectratio=strdup("1:1");
            else if (optarg[0] == '2')
                img->aspectratio=strdup("4:3");
            else if (optarg[0] == '3')
                img->aspectratio=strdup("16:9");
            else if (optarg[0] == '4')
                img->aspectratio=strdup("2.21:1");
            else
                printf("%s\n","[ERR]  Only aspect ratios are 1 (1:1), 2 (4:3), 3 (16:9) or 4 (2.21:1).");
            printf("[PAR]  Using aspect ratio: %s\n", img->aspectratio);
            break;

        case '6':
            img->nmenus=atoi(optarg);
            printf("[PAR]  Using %d menu screens.\n", img->nmenus);
            break;

        case 'N':
            printf("[PAR]  Using %s top menu background picture.\n", optarg);
            free(img->blankscreen);
            img->blankscreen=strdup(optarg);
            break;


        case '7':
            img->ncolumns=atoi(optarg);
            printf("[PAR]  Using %d menu columns.\n", img->ncolumns);

            break;


        case 3:

            strlength=strlen(optarg);
            globals.settings.bindir=realloc(globals.settings.bindir, (strlength+25)*sizeof(char)); // 25 char for later application names
            memcpy(globals.settings.bindir, optarg, strlength);
            printf("[PAR]  Using directory %s for auxiliary binaries.\n", optarg);
            break;



        }
    }

// Cleaning operations
if (user_command_line)
{
    errno=0;
    if (refresh_outdir)
    {
            clean_directory(globals.settings.outdir);
            if (errno) perror("[ERR]  clean");
    }
    else
    {
        if (globals.debugging)
        printf("[MSG]  Output directory %s has been preserved.\n", globals.settings.outdir);
    }
    change_directory(globals.settings.workdir);

    errno=secure_mkdir(globals.settings.outdir, 0777, OUTDIR);
    if (errno)
    {
            if (errno != EEXIST) perror("[WAR]  mkdir outdir");  // EEXIST error messages are often spurious
    }
     else
    {
    }

    errno=0;
    if (refresh_tempdir)
    {
            clean_directory(globals.settings.tempdir);
            if (errno) perror("[ERR]  clean");
    }
    errno=secure_mkdir(globals.settings.tempdir, globals.access_rights, TEMPDIR);
    if (errno)
    {
            if (errno != EEXIST) perror("[WAR]  mkdir temp");
    }
    else
    if (refresh_tempdir)
    {
       if (globals.debugging)
        printf("[PAR]  Temporary directory %s has been removed and recreated.\n", globals.settings.tempdir);
    }
    else
    {
       if (globals.debugging)
        printf("[PAR]  Temporary directory %s has been preserved.\n", globals.settings.tempdir);
    }
    errno=0;

}

#ifdef __WIN32__
#else

#endif

if (globals.topmenu == NO_MENU) goto stillpic_parsing;


        // Coherence checks


        if ((globals.topmenu <= ACTIVE_MENU_ONLY) && (globals.topmenu > AUTOMATIC_MENU))
        {
            printf("%s\n", "[WAR]  Top menu authoring is not automatic.\n       You must give extra information\n");
            switch (globals.topmenu)
            {
                /* other tests to be added for :
                #define RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR -1
                #define RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR 0
                #define RUN_SPUMUX_DVDAUTHOR    1 // automate some of the authoring process (run spumux and dvdauthor)
                */
            case TS_VOB_TYPE:

                if (img->tsvob)
                {
                    errno=0;
                    FILE *f;
                    if ((f=fopen(img->tsvob, "rb")) != NULL)
                    {
                        fclose(f);
                        puts("[MSG]  --> top vob requirement...OK");
                    }

                }
                break;

            case RUN_DVDAUTHOR:

                if ((img->backgroundmpg) && (globals.xml))
                {
                    errno=0;
                    if (fopen(globals.xml, "rb") != NULL)
                        fclose(fopen(globals.xml, "rb"));
                    if (!errno) puts("[MSG]  --> dvdauthor requirement...OK");
                }
                else errno=1;
                break;

            default:  errno=1;
            }


            if (errno)
            {

                printf("%s\n", "[WAR]  Not enough information. Continuing with automatic menu authoring...");
                globals.topmenu=AUTOMATIC_MENU;
                errno=0;
            }

        }




    if (globals.topmenu <= RUN_SPUMUX_DVDAUTHOR)
    {
        if ((img->imagepic==NULL) && (img->selectpic==NULL)&& (img->highlightpic==NULL))
        {
            printf("%s\n", "[WAR]  You need all subtitle images");
            printf("%s\n", "[WAR]  Continuing with menu picture authoring...");
            globals.topmenu=Min(RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR, globals.topmenu);
        }
    }





    // put this check befor the img->ncolumns check
    // LIMITATION hopefully to be relaxed later on

   if ((img->nmenus > 1)&&(img->active))
    {
        printf("%s", "[WAR]  Active menus can only be used with simple menus for version "VERSION"\n       Using img->nmenus=1...\n");
        img->nmenus=1;
    }
    if ((img->hierarchical)&&(img->active))
    {
        printf("%s", "[WAR]  Active menus cannot be used with hierarchical menus for version "VERSION"\n       Choosing hierarchical menus...\n");
        img->active=0; img->hierarchical=1;
    }


    if (img->nmenus == 0)
    {
        if (img->ncolumns == 0) img->ncolumns=DEFAULT_MENU_NCOLUMNS;  // just in case, not to divide by zero, yet should not arise unless...
        if (img->hierarchical) img->nmenus=ngroups+1; // list of groups and one menu per group only (--> limitation to be indicated)
        else

            img->nmenus=ngroups/img->ncolumns + (ngroups%img->ncolumns > 0);  // number of columns cannot be higher than img->ncolumns; adjusting number of menus to ensure this.
        printf("[MSG]  With %d columns, number of menus will be %d\n", img->ncolumns, img->nmenus);
    }
    else
    {
     if ((img->hierarchical) && (img->nmenus == 1))
     {
       printf("%s", "[WAR]  Hierarchical menus should have at least two screens...\n       Incrementing value for --nmenus=1->2\n");
       img->nmenus++;
     }

     img->ncolumns=ngroups/(img->nmenus-img->hierarchical)+(ngroups%(img->nmenus-img->hierarchical) >0);
    }



    if (globals.topmenu <= ACTIVE_MENU_ONLY) normalize_temporary_paths(img);

    maxbuttons=Min(MAX_BUTTON_Y_NUMBER-2,totntracks)/img->nmenus;
    resbuttons=Min(MAX_BUTTON_Y_NUMBER-2,totntracks)%img->nmenus;

#ifndef __CB__
#if !defined HAVE_MPEG2ENC || !defined HAVE_JPEG2YUV || !defined HAVE_MPLEX
    if (globals.topmenu <= RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR)
    {
        printf("%s\n", "[ERR]  You need mplex, mpeg2enc and jpeg2yuv to author\n       a background screen, please install these applications.");
        printf("%s\n", "[WAR]  Continuing without menu authoring...");
        globals.topmenu=NO_MENU;
    }

#endif
#endif

// Now possible overrides once img->nmenus and default tempdir values are known:
char * str=NULL;
optind=0;
opterr=1;

#ifdef LONG_OPTIONS
    while ((c=getopt_long(argc, argv, ALLOWED_OPTIONS, longopts, &longindex)) != -1)
#else
    while ((c=getopt(argc, argv, ALLOWED_OPTIONS)) != -1)
#endif
    {
        switch (c)
        {

        case 'b':

            if (img->backgroundmpg)
            {
                printf("%s\n", "[ERR]  Background mpg file already specified, skipping...");
                break;
            }
            printf("%s%s\n", "[PAR]  background jpg file(s) for generating mpg video: ", optarg);

            str=strdup(optarg);

            img->backgroundpic=fn_strtok(str,',',img->backgroundpic,0,NULL,NULL);
            int backgroundpic_arraylength=0;
            if ((backgroundpic_arraylength=arraylength(img->backgroundpic)) < img->nmenus)
            {
                    int u;
                    printf("%s\n","[WAR]  You did not give enough filenames, completing with last one");
                    for (u=0; u + backgroundpic_arraylength < img->nmenus; u++)
                     copy_file(img->backgroundpic[backgroundpic_arraylength-1], img->backgroundpic[u+backgroundpic_arraylength]);
            }


            free(str);
            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            img->refresh=1;

            break;

        case 'E':
            printf("%s%s\n", "[PAR]  highlight png file(s) for generating mpg video: ", optarg);
            str=strdup(optarg);

            img->highlightpic=fn_strtok(str,',',img->highlightpic,0,NULL,NULL);
            int highlight_arraylength=0;
            if ((highlight_arraylength=arraylength(img->highlightpic)) < img->nmenus)
            {
                    int u;
                    printf("%s\n","[WAR]  You did not give enough filenames, completing with last one");
                    for (u=0; u + highlight_arraylength < img->nmenus; u++)
                     copy_file(img->highlightpic[highlight_arraylength-1], img->highlightpic[u+highlight_arraylength]);
            }


            free(str);

            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;

        case 'e' :
            printf("%s%s\n", "[PAR]  select png file(s) for generating mpg video: ", optarg);
            str=strdup(optarg);

            img->selectpic=fn_strtok(str,',',img->selectpic,0,NULL,NULL);
            int select_arraylength=0;
            if ((select_arraylength=arraylength(img->selectpic)) < img->nmenus)
            {
                    int u;
                    printf("%s\n","[WAR]  You did not give enough filenames, completing with last one");
                    for (u=0; u + select_arraylength < img->nmenus; u++)
                     copy_file(img->selectpic[select_arraylength-1], img->selectpic[u+select_arraylength]);
            }


            free(str);

            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;


        case 'G' :
            printf("%s%s\n", "[PAR]  image png file(s) for generating mpg video: ", optarg);
            str=strdup(optarg);

            img->imagepic=fn_strtok(str,',',img->imagepic,0,NULL,NULL);
            int image_arraylength=0;
            if ((image_arraylength=arraylength(img->imagepic)) < img->nmenus)
            {
                    int u;
                    printf("%s\n","[WAR]  You did not give enough filenames, completing with last one");
                    for (u=0; u + image_arraylength < img->nmenus; u++)
                     copy_file(img->imagepic[image_arraylength -1], img->imagepic[u+image_arraylength ]);
            }


            free(str);

            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;

        case 2:

            printf("%s%s\n", "[PAR]  Background color(s) for top (and active) menus : ", optarg);
            str=strdup(optarg);

            img->backgroundcolors=fn_strtok(str,':', img->backgroundcolors,0,NULL,NULL);
            int bgcolors_arraylength=0;
            if ((bgcolors_arraylength=arraylength(img->backgroundcolors)) < img->nmenus)
            {
                    int u;
                    printf("%s\n","[WAR]  You did not give enough colors, completing with last one");
                    for (u=0; u + bgcolors_arraylength < img->nmenus; u++)
                      img->backgroundcolors[u+bgcolors_arraylength]=img->backgroundcolors[bgcolors_arraylength -1];
            }

            free(str);
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            break;


        }
    }

// This had to be postponed after command line parsing owing to tempdir chain user input

stillpic_parsing:

if (stillpic_string)
{
            // heap-allocations is not possible if char** is not returned by function
            // A simple char* would well be allocated by function, not a char**.

            tab=fn_strtok(stillpic_string, '-', tab, 0,NULL,NULL);
            uint16_t dim,DIM=0,w;

            img->npics =(uint16_t*) calloc(totntracks, sizeof(uint16_t));
            if (img->npics == NULL)
            {
                perror("[ERR] img->npics");
                goto standard_checks;
            }
            if (tab)
                w=arraylength(tab);
            else
            {
                perror("[ERR]  tab");
                goto standard_checks;
            }
            if (w > totntracks)
            {
                perror("[ERR]  Too many tracks on --stillpics");
                goto standard_checks;
            }
            else if (w < totntracks)
            {
                perror("[ERR]  You forgot at least one track on --stillpics");
                goto standard_checks;
            }

            for (k=0; k < totntracks; k++)
            {
                tab2=fn_strtok(tab[k], ',', tab2, -1,create_stillpic_directory,NULL);
                dim=0;
                w=0;
                if (tab2) while (tab2[w] != NULL)
                    {
                        if (tab2[w][0] != 0) dim++;
                        w++;
                    }
                else
                {
                    perror("[ERR]  tab2");
                    goto standard_checks;
                }
                npics[k]=(k)? dim+npics[k-1]: dim;
                img->npics[k]=dim;
                DIM+=dim;
                if (globals.veryverbose) printf("  --> npics[%d] = %d\n", k, dim);
                FREE(tab2)
                if (img->npics[k] > 99)
                {
                    printf("[ERR]  The maximum number of pics per track is 99.\n");
                    EXIT_ON_RUNTIME_ERROR_VERBOSE("Exiting...");
                }
            }

            FREE(tab)
            img->stillpicvobsize=(uint32_t*) calloc(DIM, sizeof(uint32_t));
            if (img->stillpicvobsize == NULL)
            {
                perror("[ERR]  still pic vob size array");
                goto standard_checks;
            }
            img->count=DIM;
            if (globals.veryverbose) printf("[MSG]  Total of %d pictures\n", img->count);
            free(stillpic_string);

}

standard_checks:

    if (nplaygroups > ngroups-nvideolinking_groups)
    {
        if (globals.debugging) printf("[ERR]  There cannot be more copy groups than audio groups. Limiting to %d groups...\n", ngroups-nvideolinking_groups);
        nplaygroups=ngroups-nvideolinking_groups;
    }

    if ( nplaygroups+ngroups > 8)
    {
        if (globals.debugging) printf("%s\n", "[ERR]  There cannot be more copy groups than audio groups. Limiting to 9 groups...");
        nplaygroups=MAX(0, 9-ngroups);
    }


    // ngroups does not include copy groups from then on -- nplaygroups are just virtual (no added bytes to disc)
    // number of groups=ngroups+nplaygroups
    // number of audio groups=ngroups-nvideolinking_groups

    // End of coherence checks



    command_t command0=
    {
        ngroups,
        n_g_groups,
        nplaygroups,
        playtitleset,
        nvideolinking_groups,
        maximum_VTSI_rank,
        VTSI_rank,
        ntracks,
        img,
        files,
        textable,
    };


    errno=0;
    memcpy(command, &command0, sizeof(command0));
    user_command_line++;

    return(command);
}

#ifndef WITHOUT_FIXWAV
void fixwav_parsing(char *ssopt)
{
    int subopt;
    char * chain=ssopt;
    char* value=NULL;
    char* tokens[]=
    { "simple-mode","prepend","in-place","interactive","padding","prune","output","force", "cautious", "infodir", NULL};

    while ((subopt = getsubopt(&chain, tokens, &value)) != -1)
    {
        switch (subopt)
        {
        case 0:
            printf("%s\n", "[PAR]  Fixwav: simple mode activated, advanced features deactivated.");
            globals.fixwav_automatic=0;
            break;

        case 1:
            printf("%s\n", "[PAR]  Fixwav: prepending header to raw file.");
            globals.fixwav_prepend=1;
            break;

        case 2:
            printf("%s\n", "[PAR]  Fixwav: file header will be repaired in place.");
            globals.fixwav_in_place=1;
            break;

        case 3:
            printf("%s\n", "[PAR]  Fixwav: interactive mode activated.");
            globals.fixwav_interactive=1;
            break;

        case 4:
            printf("%s\n", "[PAR]  Fixwav: padding activated.");
            globals.fixwav_padding=1;
            break;

        case 5:
            printf("%s\n", "[PAR]  Fixwav: pruning silence at end of files.");
            globals.fixwav_prune=1;
            break;

        case 6:

            FREE(globals.fixwav_suffix)
            globals.fixwav_suffix=strndup(value, MAX_OPTION_LENGTH);
            printf("[PAR]  Fixwav output suffix: %s\n", globals.fixwav_suffix);
            break;

        case 7:
            globals.fixwav_force=1;
            printf("%s", "[PAR]  Fixwav will be launched before SoX for seriously mangled headers.\n");
            break;

        case 8:
            globals.fixwav_cautious=1;
            printf("%s", "[PAR]  Fixwav will ask user permission to overwrite files in place.\n");
            break;

        case 9:
            FREE(globals.settings.fixwav_database)
            globals.settings.fixwav_database=strndup(value, MAX_OPTION_LENGTH);
            secure_mkdir(globals.settings.fixwav_database, 0755, DEFAULT_DATABASE_FOLDER);
            printf("%s       %s%s", "[PAR]  Fixwav will output info chunk from wav headers to:\n", globals.settings.fixwav_database, SEPARATOR "database\n");
            break;
        }
    }

    return;
}
#endif

void extract_list_parsing(const char *arg, extractlist* extract)
{
    char * chain, *subchunk=NULL, control=0;
    int j;
    _Bool cutgroups=0;

    memset(extract, 0, sizeof(extractlist));
    uint8_t nextractgroup=0;
    nextractgroup=extract->nextractgroup[0];
    chain=strdup(arg);

    cutgroups=(strchr(chain, ',') == NULL)? 0: 1 ;

    if (!cutgroups) return;

    /* strtok modifies its first argument.
    * If ',' not found, returns all the string, otherwise cuts it */

    strtok(chain, ",");

    if (globals.debugging)
        printf("%s\n", "[INF]  Analysing --extract suboptions...");


    control=1;

    // Now strtok will return NULL if ',' not found, otherwise * to start of token

    while (1)
    {
        if (cutgroups)
        {
            if (((subchunk=strtok(NULL, ",")) == NULL) || (control > 8))
                break;
        }
        else if (control > 1) break;


        int groupindex=(int) *subchunk-'0';
        char colon=*(subchunk+1);
        if ((colon != ':') || (strlen(subchunk) < 3))
        {
            printf("%s\n", "[WAR]  Incorrect --extract suboptions, format is --extract=group1:track1,...,groupN:trackN\n       Skipping...");
            return;
        }
        int trackindex=atoi(subchunk+2);

        if ((groupindex > 8) || (groupindex < 0) || (nextractgroup == 8) || (trackindex > 98))
        {
            groupindex=0;
            printf("%s\n", "[WAR]  Incorrect --extract suboption, exceeding limits reset to 0.");
            nextractgroup=0;
            trackindex=0;
        }
        nextractgroup++;
        extract->extracttitleset[groupindex]=1;
        extract->extracttrackintitleset[groupindex][trackindex]=1;

        control++;
    }

    if (globals.debugging)
    {
        printf("%s","[PAR]  EXTRACTING: titleset   |   track\n");
        int k;
        for (j=0; j < 9; j++)
            for (k=0; k < 99; j++)
                if ((extract->extracttitleset[j]) && (extract->extracttrackintitleset[j][k]))
                    printf( "[PAR]                   %02d      |      %02d\n", j, k );
    }

    /* all-important, otherwise irrelevant EXIT_ON_RUNTIME_ERROR will be generated*/

    extract->nextractgroup[0]=nextractgroup;
    errno=0;
    FREE(chain)

}


void ats2wav_parsing(const char * arg, extractlist* extract)
{

    char * chain, list[9];
    DIR *dir;
    memset(list, 0, 9);

    chain=strdup(arg);
    globals.settings.indir=calloc(strlen(arg)+1+9, sizeof(char));

    if (globals.settings.indir == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Could not allocate global settings")

        sprintf(globals.settings.indir, "%s"SEPARATOR"AUDIO_TS", chain);

    change_directory(globals.settings.indir);
    if ((dir=opendir(globals.settings.indir)) == NULL)
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Could not open output directory")

        printf("[INF]  Extracting audio from %s\n", globals.settings.indir);


    if (extract->nextractgroup[0])
    {

        parse_disk(dir, globals.access_rights,  OUTDIR, extract);
    }
    else

        parse_disk(dir, globals.access_rights, OUTDIR, NULL);

    if (closedir(dir) == -1)
        printf( "%s\n", "[ERR]  Impossible to close dir");

    /* all-important, otherwise irrelevant EXIT_ON_RUNTIME_ERROR will be generated*/


    errno=0;
    change_directory(globals.settings.workdir);
    FREE(chain)
}
#ifdef img
#undef img

void still_options_parsing(char *ssopt, pic* img)
{
    int subopt, k;
    char* chain=ssopt;
    char* value=NULL;
    char* tokens[]= {"rank", "manual","starteffect","endeffect","lag","start","active", NULL};
    static uint32_t rank, temp, lag;

    if (img->options == NULL) img->options=calloc(img->count, sizeof(stilloptions*));
    if (img->options == NULL) perror("[ERR]  still options parsing");
    for (k=0; k<img->count; k++)
    {
        img->options[k]=calloc(1, sizeof(stilloptions));
        if (img->options[k] == NULL) perror("[ERR]  still options parsing");
    }
    // TODO: free them
    while ((subopt = getsubopt(&chain, tokens, &value)) != -1)
    {
        switch (subopt)
        {
        case 0:
            temp=atoi(value);
            if (temp > img->count)
            {
                printf("%s\n", "[WAR]  Too many options, skipping...");
                break;
            }
            rank=temp;
            printf("%s%d\n", "[PAR]  Options for still #", rank);
            break;

        case 1:
            printf("[PAR]  #%d: Manual browsing enabled.\n", rank);
            img->options[rank]->manual=1;
            break;

        case 2:
            printf("[PAR]  #%d: start effect is: %s.\n", rank, value);    //  or: cut, fade, dissolve, top, bottom, left, right
            switch (value[0])
            {
            case 'c':
                img->options[rank]->starteffect=CUT|lag;
                break;
            case 'f':
                img->options[rank]->starteffect=FADE|lag;
                break;
            case 'd':
                img->options[rank]->starteffect=DISSOLVE|lag;
                break;
            case 't':
                img->options[rank]->starteffect=WIPEFROMTOP|lag;
                break;
            case 'b':
                img->options[rank]->starteffect=WIPEFROMBOTTOM|lag;
                break;
            case 'l':
                img->options[rank]->starteffect=WIPEFROMLEFT|lag;
                break;
            case 'r':
                img->options[rank]->starteffect=WIPEFROMRIGHT|lag;
                break;

            }
            break;

        case 3:
            printf("[PAR]  #%d: end effect is: %s.\n", rank, value);
            switch (value[0])
            {
            case 'c':
                img->options[rank]->endeffect=CUT|lag;
                break;
            case 'f':
                img->options[rank]->endeffect=FADE|lag;
                break;
            case 'd':
                img->options[rank]->endeffect=DISSOLVE|lag;
                break;
            case 't':
                img->options[rank]->endeffect=WIPEFROMTOP|lag;
                break;
            case 'b':
                img->options[rank]->endeffect=WIPEFROMBOTTOM|lag;
                break;
            case 'l':
                img->options[rank]->endeffect=WIPEFROMLEFT|lag;
                break;
            case 'r':
                img->options[rank]->endeffect=WIPEFROMRIGHT|lag;
                break;
            }
            break;

        case 4:
            lag=atoi(value);
            if (lag > 15)
            {
                printf("%s", "[WAR]  Lag should be lower than 16, skipping...\n");
                break;
            }
            printf("[PAR]  #%d: effect lag is: %d*0.32s=%fs.\n", rank, lag, (float) lag*0.32);
            img->options[rank]->lag=lag;
            break;

        case 5:
            img->options[rank]->onset=atoi(value);
            break;

        case 6:
            img->options[rank]->manual=1;
            img->options[rank]->active=1;
            printf("[PAR]  Using active menus for #%d.\n", rank);
            break;

        }
    }

    return;
}
#endif
