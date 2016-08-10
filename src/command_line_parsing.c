#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifndef __WIN32__
#include <unistd.h>
#include <fcntl.h>
#endif
#include "getopt.h"
#include <sys/time.h>
#include "dvda-author.h"
#include "commonvars.h"
#include "structures.h"
#include "c_utils.h"
#include "audio2.h"
#include "audio.h"
#include "auxiliary.h"
#include "ports.h"
#include "file_input_parsing.h"
#include "launch_manager.h"
#include "dvda-author.h"
#include "fixwav_auxiliary.h"
#include "fixwav_manager.h"
#include "command_line_parsing.h"
#include "libsoxconvert.h"
#include "menu.h"
#ifndef WITHOUT_lplex
#include "sound.h"
#endif
#include "videoimport.h"

/*  #define _GNU_SOURCE must appear before <string.h> and <getopt.h> for strndup  and getopt_long*/

globalData globals;
unsigned int startsector;
extern char* OUTDIR, *LOGFILE, *WORKDIR, *TEMPDIR, *LPLEXTEMPDIR;
static fileinfo_t ** files;
uint16_t totntracks;
uint8_t maxbuttons; // to be used in xml.c and menu.c as extern globals
uint8_t resbuttons; // to be used in xml.c and menu.c as extern globals
uint8_t ndvdvtitleset1=0,ndvdvtitleset2=0;
uint8_t mirror_st_flag=0;
uint8_t* ndvdvslides=NULL; 
uint8_t* ndvdvtracks=NULL;

_Bool soundtracks_flag=0;
_Bool dvdv_tracks_given=0;
_Bool lplex_slides_flag=0;
_Bool dvdv_import_flag=0;
_Bool mirror_flag=0;
_Bool full_hybridate_flag=0;
_Bool         hybridate_flag=0;

char  *stillpic_string=NULL;
char  **pics_per_track=NULL;
char ***dvdv_track_array=NULL;
char ***dvdv_slide_array=NULL;
char ***picks_per_track_double_array=NULL;

void parse_double_entry_command_line(char* input_string, char**** DOUBLE_ARRAY, uint8_t** COUNTER_ARRAY, uint8_t* TOTAL, short int audit_flag, char separator) 
{
    errno=0;
    char** array=NULL; 
    array=fn_strtok(input_string, separator, array, 0, NULL, NULL);
    *TOTAL=arraylength(array); 
    if (globals.veryverbose) 
    {
        fprintf(stderr, ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Found %d DVD-VIDEO group(s)/titleset(s)\n", *TOTAL);
        for (int u=0; u < *TOTAL; u++) fprintf(stderr, ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Found group/titleset %d files: %s\n", u+1, array[u]);
    }
    
    *DOUBLE_ARRAY=(char ***) calloc(*TOTAL, sizeof(char***)); 
    if (NULL == *DOUBLE_ARRAY) EXIT_ON_RUNTIME_ERROR 
            
            for (int titleset=0; titleset < *TOTAL; titleset++) 
    {
        (*DOUBLE_ARRAY)[titleset]=fn_strtok(array[titleset], ',', (*DOUBLE_ARRAY)[titleset], 0,NULL, NULL);
        *COUNTER_ARRAY=calloc(*TOTAL, sizeof(uint8_t));
        *COUNTER_ARRAY[titleset]=arraylength(*DOUBLE_ARRAY[titleset]);
#if DEBUG
        if (globals.veryverbose) 
        {
            if (audit_flag == AUDIT_DVD_VIDEO_AUDIO_FORMAT)
                fprintf(stderr, ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Found %d audio track(s) for DVD-VIDEO titleset %d\n", *COUNTER_ARRAY[titleset], *TOTAL);
            else
                if (audit_flag == NO_FIXWAV_AUDIT)
                    fprintf(stderr, ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Found %d slide(s) for DVD-VIDEO titleset %d\n", *COUNTER_ARRAY[titleset], *TOTAL);
        }
#endif
        for (int track=0; track < *COUNTER_ARRAY[titleset]; track++)
        {
#ifndef WITHOUT_lplex
            if ((audit_flag == AUDIT_DVD_VIDEO_AUDIO_FORMAT) || (audit_flag == AUDIT_STRICT_TOPMENU_AUDIO_FORMAT)) 
                errno=audit_soundtrack((*DOUBLE_ARRAY)[titleset][track], audit_flag);
#endif
            // else case: images, noop.

            
            if (errno) 
            {
                fprintf(stderr, 
                        ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Track %s is not DVD-VIDEO compliant\n       Exiting...\n", 
                        (*DOUBLE_ARRAY)[titleset][track]);
                EXIT_ON_RUNTIME_ERROR 
            }
            else 
                if (globals.debugging)
                    foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Checked that track %s is DVD-VIDEO compliant\n", 
                            (*DOUBLE_ARRAY)[titleset][track]);
        }
    }
    
    free(array);
}


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
    static uint8_t ntracks[9]={0};
    
    static uint8_t playtitleset[9]= {0};
        
    extern char *optarg;
    extern int optind, opterr;
    int k, c;
    
    static char ALLOWED_OPTIONS[256];
    // Allowing for 30 non-print characters for short options
    // Note that the :/:: diacritics are only needed for short options and that long options argument status is defined in struct longopts, so
    // this trick is OK if only long options are used for non-print short options.
    if (!user_command_line)
    {
        
        for (k=0; k < 30; k++)
            ALLOWED_OPTIONS[k]=k;
        strcat(ALLOWED_OPTIONS, ALLOWED_OPTIONS_PRINT);
    }
    
    int errmsg;
    _Bool allocate_files=0, logrefresh=0, refresh_tempdir=1, refresh_outdir=1;  // refreshing output and temporary directories by default
    _Bool download_new_version_flag=0, check_version_flag=0, force_download_flag=0;
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
    
    if (user_command_line)     globals.silence=0;
    
    /* crucial: initialise before any call to getopt */
    optind=0;
    opterr=1;
    
#ifdef LONG_OPTIONS
    int longindex=0;
    
    static struct option  longopts[]=
    {
        {"debug", no_argument, NULL, 'd'},
        {"no-output", no_argument, NULL, 10},
        {"maxverbose", no_argument, NULL, 11},
        {"veryverbose", no_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h'},
        {"input", required_argument, NULL, 'i'},
        {"log", required_argument, NULL, 'l'},
        {"logrefresh", required_argument, NULL, 'L'},
        {"no-videozone", no_argument, NULL, 'n'},
        {"output", required_argument, NULL, 'o'},
        {"lplex-output", required_argument, NULL, 20},
        {"autoplay", no_argument, NULL, 'a'},
        {"startsector", required_argument, NULL, 'p'},
        {"pause", optional_argument, NULL, 'P'},
        {"quiet", no_argument, NULL, 'q'},
        {"version", no_argument, NULL, 'v'},
        {"disable-lexer", no_argument, NULL, 'W'},
        {"playlist", required_argument, NULL, 'Z'},
        {"cga", required_argument, NULL, 'c'},
        {"text", optional_argument, NULL, 'k'},
        {"tempdir", required_argument, NULL, 'D'},
        {"lplex-tempdir", required_argument, NULL, 19},
        {"workdir", required_argument, NULL, 'X'},
        {"datadir", required_argument, NULL, '9'},
        {"loghtml", no_argument, NULL, 1},
        {"no-refresh-tempdir",no_argument, NULL, 4},
        {"no-refresh-outdir",no_argument, NULL, 5},
        {"extract", required_argument, NULL, 'x'},
    #if !HAVE_core_BUILD
        {"play", required_argument, NULL, 12},
        {"player", required_argument, NULL, 13},
        {"videodir", required_argument, NULL, 'V'},
        {"fixwav", optional_argument, NULL, 'F'},
        {"fixwav-virtual", optional_argument, NULL, 'f'},
        {"sox", optional_argument, NULL, 'S'},
        {"videolink", required_argument, NULL, 'T'},
        {"loop", optional_argument, NULL, 'U'},
        {"rights", required_argument, NULL, 'w'},
        {"pad-cont", no_argument, NULL, 'C'},
        {"topmenu", optional_argument, NULL, 'm'},
        {"menustyle", required_argument, NULL, '0'},
        {"xml", required_argument, NULL, 'M'},
        {"spuxml", required_argument, NULL, 'H'},
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
        {"background-mpg", required_argument, NULL, 'B'},
        {"soundtracks", required_argument, NULL, 'Q'},
        {"topmenu-colors", required_argument, NULL, 'y'},
        {"topmenu-palette", required_argument, NULL, 'Y'},
        {"blankscreen", required_argument, NULL, 'N'},
        {"screentext", required_argument, NULL, 'O'},
        {"highlightformat", required_argument, NULL, 'K'},
        {"font", required_argument, NULL, 'J'},
        {"fontname", required_argument, NULL, 14},
        {"fontsize", required_argument, NULL, 15},
        {"fontwidth", required_argument, NULL, 16},
        {"duration", required_argument, NULL, 'u'},
        {"stillpics", required_argument, NULL, '3'},
        {"norm", required_argument, NULL, '4'},
        {"aspect", required_argument, NULL, '5'},
        {"nmenus", required_argument, NULL, '6'},
        {"ncolumns", required_argument, NULL, '7'},
        {"activemenu-palette", required_argument, NULL, '8'},
        {"background-colors", required_argument, NULL, 2},
        {"bindir",required_argument, NULL, 3},
        {"topmenu-slides",required_argument, NULL, 6},
        {"download",optional_argument, NULL, 7},
        {"check-version",no_argument, NULL, 8},
        {"import-topmenu",required_argument, NULL, 9},
        {"dvdv-tracks",required_argument, NULL, 17},
        {"dvdv-slides",required_argument, NULL, 18},
        {"dvdv-import",no_argument, NULL, 21},
        {"mirror",no_argument, NULL, 22},
        {"mirror-strategy",required_argument, NULL, 23},
        {"hybridate",no_argument, NULL, 24},
        {"full-hybridate",no_argument, NULL, 25},
        {"merge",required_argument, NULL, 26},
    #endif
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
                            globals.settings.logfile=strdup(optarg);
                            globals.logfile=1;
                            
                            if (optarg[0] == '-')
                            {
                                globals.logfile=0;
                                foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Enter a log path next time!");
                                exit(EXIT_FAILURE);
                            }
                            else
                                globals.logfile=1;
                        }
                        
                        break;
                        
                    case 1 :
                        globals.loghtml=1;
                        break;
                        
                    case 7:
                        download_new_version_flag=1;
                        check_version_flag=1;
                        if (optarg) force_download_flag=1;
                        break;
                        
                    case 8:
                        check_version_flag=1;
                        
                        break;
                        
                }
            }
                
           
                if (((user_command_line) || (!globals.enable_lexer)) && (!globals.silence))
                {
                    
                    if (globals.logfile)
                    {
                        if (logrefresh)
                            globals.journal=fopen(globals.settings.logfile, "wb");
                        else
                            globals.journal=fopen(globals.settings.logfile, "ab");
                        
                    }
                    
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

                    ngroups=nvideolinking_groups=n_g_groups=0;
                    if (globals.veryverbose)
                        foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Overriding configuration file specifications for audio input");
                    // Useless to continue parsing
                    //reset++;
                    break;
                    
                case 'D' :
                    FREE(globals.settings.tempdir);
                    globals.settings.tempdir=strndup(optarg, MAX_OPTION_LENGTH);
                    foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Temporary directory is: ", optarg);
                    normalize_temporary_paths(NULL);
                    break;
                    
                case 19:
                    FREE(globals.settings.lplextempdir);
                    globals.settings.lplextempdir=strndup(optarg, MAX_OPTION_LENGTH);
                    foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Lplex temporary directory is: ", optarg);
                    break;
                    
                case 20:
                    FREE(globals.settings.lplexoutdir);
                    globals.settings.lplexoutdir=strndup(optarg, MAX_OPTION_LENGTH);
                    foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Lplex output directory is: ", optarg);
                    break;
                    
                case 'X':
                    free(globals.settings.workdir);
                    globals.settings.workdir=strndup(optarg, MAX_OPTION_LENGTH);
                    foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Working directory is: ", optarg);
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
        foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Parsing user command line");
        print_commandline(argc, argv);
        
        foutput("%c", '\n');
    }
    
    if (globals.logfile) foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Log file is: ", globals.settings.logfile);
      
    /* COMMAND-LINE PARSING: second pass to determine memory allocation (thereby avoiding heap loss)
     * We give up getopt here to allow for legacy "Dave" syntax with multiple tracks as -g arguments
     * (not compatible with getopt or heavy to implement with it.  */
    
    if (globals.debugging) foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  First scan of track list for memory allocation...");
    
    // n_g_groups count command-line groups of type -g, -j (join groups), -s (single track groups)
        // ngiven_channels: number of given channels for group index n_g_group and at track 0-based rank ntracks
    // given_channel: the mono channel given
    
    uint8_t ngiven_channels[9][99] = {{0}};
   
    for (k=1; k < argc; k++)
    {
        if (argv[k][0] != '-' || argv[k][1] == '\0') continue;
        switch (argv[k][1])
        {
        
        case 'j' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Join group");
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
            break;
            
         case '-':
            if ((strlen(argv[k]) != 7 ) || (strcmp(argv[k]+2, "merge") != 0))
              break;
              
            k++;
            
            ntracks[n_g_groups]++;
            for (;k < argc; k++)
            {
               if (argv[k][0] !='-')
               {
                    ngiven_channels[n_g_groups][ntracks[n_g_groups]]++;
               }
                else
                  break;
                
            }
            
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
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Access rights (octal mode)=%o\n", globals.access_rights);
            break;
            
        case 's' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Single track group");
        case 'g' :
        case 'j' :
            u++;
            allocate_files=1;
            break;
            
        case 'i' :
            
            allocate_files=1;
            globals.settings.indir=strndup(optarg, MAX_OPTION_LENGTH);
            
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Input directory is: ", 	optarg);
            
            if ((dir=opendir(optarg)) == NULL)
                EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Input directory could not be opened")
                        
                        change_directory(globals.settings.indir);
            audiodir=parse_directory(dir, ntracks, n_g_groups, 0, files_dummy);
            
            ngroups=audiodir.ngroups;
            
            memmove(ntracks, audiodir.ntracks, 9*sizeof(uint8_t));
            
            if (closedir(dir) == -1)
                foutput( "%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to close dir");
            
            change_directory(globals.settings.workdir);
            
            break;
            
        case 10:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Will run with no output.");
            globals.nooutput=1;
            globals.logfile=0;
            break;
            
        case 11:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Will run with maximal debugging information.");
            globals.debugging=1;
            globals.veryverbose=1;
            globals.maxverbose=1;
            break;
            
        case 'o' :
            
            globals.settings.outdir=strndup(optarg, MAX_OPTION_LENGTH);
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Output directory is: ", optarg);
            
            break;
            
            
        case 5:
            refresh_outdir=0;
            break;
            
        case 4:
            refresh_tempdir=0;
            break;
            
#if !HAVE_core_BUILD
            
        case 'T':
            
            allocate_files=1;
            
            if (nvideolinking_groups == MAXIMUM_LINKED_VTS)
            {
                foutput(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Error: there must be a maximum of %d video linking groups\n      Ignoring additional links...\n\n", MAXIMUM_LINKED_VTS);
                break;
            }
            
            // VTSI_rank is the rank of the VTS that is linked to in VIDEO_TS directory
            VTSI_rank[nvideolinking_groups]=atoi(optarg);
            
            if   (VTSI_rank[nvideolinking_groups] > 99)
                EXIT_ON_RUNTIME_ERROR_VERBOSE( ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  There must be a maximum of 99 video titlesets in video zone. Try again...\n\n")
                        
                        if (nvideolinking_groups == 0)
                        maximum_VTSI_rank=VTSI_rank[nvideolinking_groups];
            else
                maximum_VTSI_rank=MAX(VTSI_rank[nvideolinking_groups], maximum_VTSI_rank);
            
            globals.videozone=1;
            globals.videolinking=1;
            
            increment_ngroups_check_ceiling(&ngroups, &nvideolinking_groups);
            
            break;
            
            // Should be done as early as main globals are set, could be done a bit earlier (adding an extra parse)
            
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
            files=dynamic_memory_allocate(files, ngiven_channels, ntracks, ngroups, n_g_groups, nvideolinking_groups);
        }
            
            /* COMMAND-LINE PARSING: fourth pass to assign filenames without allocating new memory (pointing to argv) */
            
            int m, ngroups_scan=0;
    
    if ((n_g_groups)&&(globals.debugging)) foutput("%s", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Assigning command-line filenames...\n");
    
    for (k=0; k < argc; ++k)
    {
        if (argv[k][0] != '-') continue;
        switch (argv[k][1])
        {
        
        case 's' :
            files[ngroups_scan][0].single_track=1;   // no break
        case 'j' :
            files[ngroups_scan][0].contin=1;
            if (globals.debugging) foutput("%s%d\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Continuity requested for group ", ngroups_scan+1);
            files[ngroups_scan][0].join_flag=1;     //  no break
            if (globals.debugging) foutput("%s%d\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Join flag set for group ", ngroups_scan+1);
            
        case 'g' :
            
            ++k;
            for (m=0; m+k < argc; m++)
            {
                if (argv[m + k][0] != '-')
                {
                    if (globals.veryverbose)
                      foutput("       files[%d][%d].filename=%s\n", ngroups_scan, m, argv[m + k]);
                    

                    files[ngroups_scan][m].filename = argv[m+k];

                    //char b[150]={0};
                    //strcpy(b, argv[m+k]);
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
                                foutput("       files[%d][%d].filename=%s\n", ngroups_scan, m, argv[m+k]);
                            if ((m+k) < argc) strcpy(files[ngroups_scan][m].filename,strdup(argv[m+k]));
                        }
                    }
                    else
                    if (strcmp(argv[m+k]+2,"merge") == 0)
                    {
                     if (ngiven_channels[ngroups_scan][m])
                     {
                       strcpy(files[ngroups_scan][m].filename,"merged channels");
                       files[ngroups_scan][m].channels=ngiven_channels[ngroups_scan][m];
                       files[ngroups_scan][m].mergeflag=1;
                       
                       if (globals.veryverbose)
                       {
                         foutput("       files[%d][%d].filename=%s\n", ngroups_scan, m, "merged channels:");
                       }
                       
                       for (int u=0; u < ngiven_channels[ngroups_scan][m]; u++)  
                       {
                        if (globals.veryverbose)
                          foutput("                               %s\n", argv[m+k+u]);
                        
                        strcpy(files[ngroups_scan][m].given_channel[u],argv[m+k+u]);
                       }
                       
                       m+=ngiven_channels[ngroups_scan][m];
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
                if (globals.debugging) foutput("       files[%d][%d].cga=%s\n", ngroups_scan, m, argv[m+k]);
                uint8_t cgaint=atoi(argv[m+k]);
                
                if (check_cga_assignment(cgaint))
                    files[ngroups_scan][m].cga=cgaint;
                else if (globals.debugging) foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Found illegal channel group assignement value, using standard settings.");
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
#if !HAVE_core_BUILD
    int nvideolinking_groups_scan=0, strlength=0;
    char *piccolorchain,
            *activepiccolorchain,
            *palettecolorchain,
            *fontchain,
            *durationchain=NULL,
            *h,
            *min,
            *sec,
            *still_options_string=NULL,
            *import_topmenu_path=NULL,
            *player="vlc";
    
    _Bool import_topmenu_flag=0;
    uint16_t npics[totntracks];
#endif
    char** textable=NULL;
    _Bool extract_audio_flag=0;
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
        
        case 'a' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Autoplay on.");
            globals.autoplay=1;
            break;
            
        case 't' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Enhanced debugging-level verbosity");
            break;
            
        case 'd' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Debugging-level verbosity");
            break;
            
        case 'x' :
            extract_audio_flag=1;
            FREE(globals.settings.indir)
                    globals.settings.indir=strdup(optarg);
            break;
            
        case 'n' :
            // There is no videozone in this case
            if (globals.videolinking)
            {
                foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You cannot delete video zone with -n if -V is activated too.\n      Ignoring -n...");
                break;
            }
            globals.videozone=0;
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  No video zone");
            break;
            
        case 'i' :
            if ((dir=opendir(optarg)) == NULL)
                EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Input directory could not be opened")
                        
                        change_directory(globals.settings.indir);
            
            parse_directory(dir, ntracks, n_g_groups, READTRACKS, files);
            
            change_directory(globals.settings.workdir);
            
            if (closedir(dir) == -1)
                foutput( "%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to close dir");
            
            /* all-important, otherwise irrelevant EXIT_ON_RUNTIME_ERROR will be generated*/
            
            errno=0;
            break;
            
            
        case 'p' :
            startsector=(int32_t) strtoul(optarg, NULL, 10);
            errmsg=errno;
            switch (errmsg)
            {
            case   EINVAL :
                foutput( "%s\n",  ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Incorrect offset value");
                clean_exit(EXIT_SUCCESS);
                break;
            case   ERANGE :
                EXIT_ON_RUNTIME_ERROR_VERBOSE( ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Offset range--overflowing LONG INT.");
                break;
            }
            errno=0;
            
            if (startsector)
                foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Using start sector: %"PRId32"\n", startsector);
            else
            {
                foutput(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Illegal negative start sector of %"PRId32"...falling back on automatic start sector\n", startsector);
                startsector=-1;
            }
            
            break;
            
        case 'P':
            if ((optarg != NULL) && (strcmp(optarg, "0") == 0))
            {
                globals.end_pause=0;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  End pause will be suppressed.");
            }
            else
            {
                globals.end_pause=1;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Adding end pause.");
            }
            break;
            
            
        case 'W' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Lexer was deactivated");
            globals.enable_lexer=0;
            break;
            
        case 'Z' :
            foutput("%s%d\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Duplicate group #", nplaygroups+1);
            globals.playlist=1;
            nplaygroups++;
            if (nplaygroups > 8)
            {
                if (globals.debugging) foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  There cannot be more than 9 copy groups. Skipping...");
                break;
            }
            playtitleset[nplaygroups]=atoi(optarg);
            
            break;
            
        case 'c' :
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Channel group assignement activated.");
            globals.cga=1;
            break;
            
            
        case 'k' :
            foutput("%s",ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Generates text table in IFO files.\n\n");
            globals.text=1;
            textable=fn_strtok(optarg, ',' , textable, 0,NULL,NULL);
            break;
            
#if !HAVE_core_BUILD
            
        case 12:
            
            player=NULL;
            extract_audio_flag=1;
            FREE(globals.settings.indir)
                    globals.settings.indir=strdup(optarg);
            
            break;
            
        case  13:
            
            player=strdup(optarg);
            break;
            
            // case 'g': c=0; break;
        case '9':
            /* --datadir is the directory  where the menu/ files are located. Under* nix it automatically installed under /usr/share/applications/dvda-author by the autotools
               With other building modes or platforms however, it may be useful to indicate where the menu/ directory will be*/
            // We use realloc here to allow for prior allocation (.conf file etc.) without memory loss
            
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using data directory %s\n", optarg);
            strlength=strlen(optarg);
            img->soundtrack[0][0]=realloc(img->soundtrack[0][0], (strlength+1+1+16)*sizeof(char)); // "silence.wav"
            if (img->soundtrack[0][0]) sprintf(img->soundtrack[0][0], "%s"SEPARATOR"%s", optarg, "menu"SEPARATOR"silence.wav");
            img->activeheader=realloc(img->activeheader, (strlength+1+1+17)*sizeof(char));  // activeheader
            if (img->activeheader) sprintf(img->activeheader, "%s"SEPARATOR"%s", optarg, "menu"SEPARATOR"activeheader");
            free(globals.settings.datadir);
            globals.settings.datadir=strdup(optarg);
            
            break;
            
            
        case 'A':
            
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  topmenu VOB: ", optarg);
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
                
                img->npics =(uint16_t*) calloc(totntracks, sizeof(uint16_t));
                for (k=0; k < totntracks; k++)
                    img->npics[k]=1;
                
                img->stillpicvobsize=(uint32_t*) calloc(totntracks, sizeof(uint32_t));
            }
            
            break;
            
        case '1':
            
            if (!img->active)
            {
                foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  still pictures VOB: ", optarg);
                img->stillvob=strndup(optarg, MAX_OPTION_LENGTH);
            }
            img->npics =(uint16_t*) calloc(totntracks, sizeof(uint16_t));
            for (k=0; k < totntracks; k++)
                img->npics[k]=1;
            img->stillpicvobsize=(uint32_t*) calloc(totntracks, sizeof(uint32_t));
            break;
            
            //  'x' Must come AFTER 'o' and 'w'
            
        case 'I':
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Run mkisofs to author disc image.");
            globals.runmkisofs=1;
            if (optarg)
            {
                free(globals.settings.dvdisopath);
                globals.settings.dvdisopath=strndup(optarg, MAX_OPTION_LENGTH);
                foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  ISO file path is: ", optarg);
            }
            break;
            
        case 'r':
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Make ISO image then run cdrecord to burn disc image.");
            globals.runmkisofs=1;
            if ((optarg) && (strlen(optarg) >4 ) )
                globals.cdrecorddevice=strndup(optarg, MAX_OPTION_LENGTH);
            else
            {
                foutput("%s%s%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Device command ", (optarg)?optarg:"", " will be interpolated.\n       Run cdrecord -scanbus to check for available drivers");
                globals.cdrecorddevice=strdup("");
            }
            break;
            
        case 'R':
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Make ISO image the run growisofs to burn disc image.");
            globals.runmkisofs=1;
            globals.rungrowisofs=1;
            if ((optarg) && (strlen(optarg) >4 ) )
                globals.cdrecorddevice=strndup(optarg, MAX_OPTION_LENGTH);
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
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  VIDEO_TS input directory is: ", optarg);
            
            break;
            
        case 'U':
            
            foutput("%s", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Loop menu background video\n");
            img->loop=1;
            break;
            

        case 'f':
            globals.fixwav_virtual_enable=1;
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Virtual fixwav enabled.");
            // case 'F' must follow breakless
            
        case 'F':
            
            /* Uses fixwav to fix bad headers*/
            
            globals.fixwav_enable=1;
            globals.fixwav_parameters=optarg;
            globals.fixwav_automatic=1; /* default */
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Bad wav headers will be fixed by fixwav");
            if (optarg != NULL)
            {
                foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  fixwav command line: ", optarg);
                /* sub-option analysis */
                fixwav_parsing(globals.fixwav_parameters);
            }
            
            
            break;

            
#ifndef WITHOUT_sox
            
        case 'S':
            
            /* Uses sox to convert different input formats */
            globals.sox_enable=1;
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Audio formats other than WAV and FLAC will be converted by sox tool.");
            
            break;
#endif
#if 0
        case 1 :
            globals.padding=0;
            if (globals.lossy_rounding)
            {
                globals.lossy_rounding=0;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  --lossy-rounding was neutralized.");
            }
            
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  No audio padding will be performed by core dvda-processes.");
            break;
            
        case 2 :
            globals.minimal_padding=1;
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Minimal padding of audio samples (for evenness).");
            break;
            
        case 'L' :
            globals.lossy_rounding=1;
            if (globals.padding)
            {
                globals.padding=0;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Default padding was neutralized.");
            }
            if (globals.padding_continuous)
            {
                globals.padding_continuous=0;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  --pad-cont was neutralized");
            }
            
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Sample count rounding will be performed by cutting audio files.");
            break;
#endif
#if 0
        case 'C' :
            globals.padding_continuous=1;
            
            if (globals.padding == 0)
            {
                globals.padding=1;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  --no-padding was neutralized");
            }
            if (globals.lossy_rounding)
            {
                globals.lossy_rounding=0;
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  --lossy-rounding was neutralized");
            }
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Pad with last known byte, if padding, not 0s.");
            break;
#endif
            
            
        case 'm' :
            
            if (optarg)
            {
                foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  File(s) %s will be used as (spumuxed) top menu\n", optarg);
                img->topmenu=fn_strtok(optarg, ',' , img->topmenu, 0,NULL,NULL);
                globals.topmenu=Min(globals.topmenu, RUN_DVDAUTHOR);
            }
            else
            {
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Automatic generation of top menu...");
                globals.topmenu=Min(globals.topmenu, AUTOMATIC_MENU);
            }
            
            break;
            
        case 'M' :
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  dvdauthor Xml project: ", optarg);
            globals.xml=strndup(optarg, MAX_OPTION_LENGTH);
            globals.topmenu=Min(globals.topmenu, RUN_DVDAUTHOR);
            break;
            
        case 'H' :
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  spumux Xml project: ", optarg);
            static int spurank;
            while (spurank >= img->nmenus) 	img->nmenus++;
            if (img->nmenus) globals.spu_xml=realloc(globals.spu_xml, img->nmenus*sizeof(char*));
            globals.spu_xml[spurank++]=strndup(optarg, MAX_OPTION_LENGTH);
            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            break;
            
        case 'B':
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  background mpg video: ", optarg);
            if (img->backgroundmpg == NULL)
                img->backgroundmpg=calloc(1, sizeof(char*));
            if (img->backgroundmpg == NULL) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  img->backgroundmpg\n");
            
            img->backgroundmpg=fn_strtok(optarg, ',' , img->backgroundmpg, 0,NULL,NULL);
            
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top background mpg file(s) %s will be used\n", optarg);
            
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            break;
            
        case 'u':
            
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  duration of background mpg video: ", optarg);
            durationchain=strndup(optarg, MAX_OPTION_LENGTH);
            
            h=strtok(durationchain, ":");
            min=strtok(NULL, ":");
            sec=strtok(NULL, ":");
            if ((h == NULL) || (min == NULL) || (sec == NULL))
            {
                foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  format must be --duration hh:mm:ss");
                break;
            }
            img->h=atoi(h);
            img->min=atoi(min);
            img->sec=atoi(sec);
            
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
                    EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Color chain is illegal: enter text:highlight:select color separated by a colon");
                errno=0;
                if (img->textcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu palette text color: %s %lx\n", img->textcolor_palette, strtoul(img->textcolor_palette,NULL,16));
                //if (img->textcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu palette background color: %s %lx\n", img->bgcolor_palette, strtoul(img->bgcolor_palette,NULL,16));
                if (img->textcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu palette highlight color: %s %lx\n", img->highlightcolor_palette, strtoul(img->highlightcolor_palette,NULL,16));
                if (img->textcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu palette select action color: %s %lx\n", img->selectfgcolor_palette, strtoul(img->selectfgcolor_palette,NULL,16));
                
                if (errno == ERANGE)
                {
                    EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  At least one YCrCb coding overflows: check switch --palette")
                }
                else
                {
                    if (errno)
                        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Check switch --palette")
                }
            }
            else
                EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Color chain could not be allocated");
            
            break;
            
        case 'y':
            piccolorchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (piccolorchain)
                
            {
                if (strcmp(piccolorchain, "norefresh") == 0)
                {
                    img->refresh=0;
                    foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Menu pics will not be refreshed...");
                    break;
                }
                
                
                free(img->textcolor_pic);
                img->textcolor_pic= strtok(piccolorchain, ":");
                img->bgcolor_pic=strdup(strtok(NULL, ":"));
                img->highlightcolor_pic=strdup(strtok(NULL, ":"));
                img->selectfgcolor_pic=strdup(strtok(NULL, ":"));
                if ((img->selectfgcolor_pic == NULL)|| (img->highlightcolor_pic ==NULL) || (img->bgcolor_pic == NULL) || (img->textcolor_pic ==NULL))
                    EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Picture color chain is illegal: enter text,background,highlight,select color\n        separated by a colon, with rgb components by commas");
                if (img->textcolor_pic) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu text color: rgb(%s)\n", img->textcolor_pic);
                if (img->bgcolor_pic) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu background color: rgb(%s)\n", img->bgcolor_pic);
                if (img->highlightcolor_pic) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu highlight color: rgb(%s)\n", img->highlightcolor_pic);
                if (img->selectfgcolor_pic) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Top menu select action color: rgb(%s)\n", img->selectfgcolor_pic);
                
            }
            else
                EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Picture color chain could not be allocated");
            
            if ((strcmp(img->selectfgcolor_pic, img->highlightcolor_pic) == 0) || (strcmp(img->textcolor_pic, img->highlightcolor_pic) == 0) || (strcmp(img->textcolor_pic, img->selectfgcolor_pic) == 0))
            {
                foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You should use different color values for menu pics: resetting to defaults");
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
                    foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Active menu pics will not be refreshed...");
                    break;
                }
                
                
                free(img->activetextcolor_palette);
                img->activetextcolor_palette= strtok(activepiccolorchain, ":");
                
                img->activebgcolor_palette=strdup(strtok(NULL, ":"));
                img->activehighlightcolor_palette=strdup(strtok(NULL, ":"));
                img->activeselectfgcolor_palette=strdup(strtok(NULL, ":"));
                if ((img->activeselectfgcolor_palette == NULL)|| (img->activehighlightcolor_palette ==NULL) || (img->activebgcolor_palette == NULL) || (img->activetextcolor_palette ==NULL))
                    EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Active picture color chain is illegal: enter text,background,highlight,select color\n        separated by a colon, with rgb components by commas");
                if (img->activetextcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Active menu text color: rgb(%s)\n", img->activetextcolor_palette);
                if (img->activebgcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Active menu background color: rgb(%s)\n", img->activebgcolor_palette);
                if (img->activehighlightcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Active menu highlight color: rgb(%s)\n", img->activehighlightcolor_palette);
                if (img->activeselectfgcolor_palette) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Active menu select action color: rgb(%s)\n", img->activeselectfgcolor_palette);
                
            }
            else
                EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Active picture color chain could not be allocated");
            
            if ((strcmp(img->activeselectfgcolor_palette, img->activehighlightcolor_palette) == 0) || (strcmp(img->activetextcolor_palette, img->activehighlightcolor_palette) == 0) || (strcmp(img->activetextcolor_palette, img->activeselectfgcolor_palette) == 0))
            {
                foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You should use different color values for active menu pics: resetting to defaults");
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
            if (globals.veryverbose) foutput("%s %s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Screen textchain is:", img->screentextchain);
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
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
                    EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Font chain is illegal: enter font,font size,font width (width in pixels for size=10)");
                
                if (img->textfont) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Font: %s\n", img->textfont);
                foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Point size: %d\n", img->pointsize);
                foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Font width: %d\n", img->fontwidth);
                
            }
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;
            
        case 14:
            fontchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (fontchain)
            {
                free(img->textfont);
                img->textfont=fontchain;
                
                if (img->textfont) foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fontname: %s\n", img->textfont);
            }
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;
            
        case 15:
            fontchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (fontchain)
            {
                img->pointsize=(int8_t) atoi(fontchain);
                foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Point size: %d\n", img->pointsize);
            }
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;
            
        case 16:
            fontchain=strndup(optarg, MAX_OPTION_LENGTH);
            if (fontchain)
            {
                img->fontwidth=(int8_t) atoi(fontchain);
                foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Font width: %d\n", img->fontwidth);
            }
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            break;
            
            
        case '2':
            still_options_string=strdup(optarg);
            break;
            
        case '4':
            /* default is PAL, 25 */
            img->norm=strdup(optarg);
            if (strcmp(optarg,"ntsc") == 0)
            {
                img->framerate[0]='3';
                img->framerate[1]='0';
                free(img->blankscreen);
                img->blankscreen=strdup(DEFAULT_BLANKSCREEN_NTSC);
                img->backgroundpic[0]=strdup(DEFAULT_BACKGROUNDPIC_NTSC);
                foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Video standard is %s", img->norm);
                
            }
            else if ((strcmp(optarg,"pal") != 0) && (strcmp(optarg,"secam") != 0))
            {
                foutput("%s\n",ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Only options are 'ntsc', 'secam' or (default) 'pal'.");
                clean_exit(EXIT_FAILURE);
            }
            
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
                foutput("%s\n",ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Only aspect ratios are 1 (1:1), 2 (4:3), 3 (16:9) or 4 (2.21:1).");
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using aspect ratio: %s\n", img->aspectratio);
            break;
            
        case '6':
            
            img->nmenus=atoi(optarg);
            
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using %d menu screens.\n", img->nmenus);
            break;
            
            
            
        case '7':
            img->ncolumns=atoi(optarg);
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using %d menu columns.\n", img->ncolumns);
            
            break;
            
            
        case 3:
            
            strlength=strlen(optarg);
            globals.settings.bindir=realloc(globals.settings.bindir, (strlength+1)*sizeof(char));
            strcpy(globals.settings.bindir, optarg);
            
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using directory %s for auxiliary binaries.\n", optarg);
            break;
            
        case 9:
            import_topmenu_flag=1;
            import_topmenu_path=strdup(optarg);
            
            globals.topmenu=RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR;
            break;
#endif
        }
    }
    
    
#if !HAVE_core_BUILD
    if (check_version_flag)
    {
#ifdef __WIN32__
        change_directory(globals.settings.bindir);
#endif
        download_latest_version(download_new_version_flag, force_download_flag);
        if (ngroups == 0) clean_exit(EXIT_SUCCESS);
    }
#endif
    
    change_directory(globals.settings.workdir);
    
    /* Here it is necessary to check and normalize: temporary directory, number of menus before copying files and allocating new memory */
    // Cleaning operations
    
    
    if (user_command_line)
    {
        errno=0;
        if ((refresh_outdir) && (!globals.nooutput))
        {
            clean_directory(globals.settings.outdir);
            clean_directory(globals.settings.lplexoutdir);
            if (errno) foutput("%s\n",ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  No output directory to be cleaned");
        }
        else
        {
            if ((globals.debugging)&& (!globals.nooutput))
                foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Output directory %s has been preserved.\n", globals.settings.outdir);
        }
        
        
        if (!globals.nooutput)
        {
            errno=secure_mkdir(globals.settings.outdir, 0777);
            if (errno)
            {
                if (errno != EEXIST) perror("\n"ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  mkdir outdir");  // EEXIST error messages are often spurious
            }
            
            
            errno=0;
            if (refresh_tempdir)
            {
                clean_directory(globals.settings.tempdir);
                if (errno) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  clean");
            }
            errno=secure_mkdir(globals.settings.tempdir, globals.access_rights);
            errno += secure_mkdir(globals.settings.lplextempdir, globals.access_rights);
            if (errno)
            {
                if (errno != EEXIST) 
                {
                    perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not create temporary directory\n");
                }
            }
            else if (refresh_tempdir)
            {
                if (globals.debugging)
                    foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  DVD-Audio temporary directory %s has been removed and recreated.\n", globals.settings.tempdir);
            }
            else
            {
                if (globals.debugging)
                    foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  DVD-Audio temporary directory %s has been preserved.\n", globals.settings.tempdir);
            }
            errno=0;
        }
        
    }
    
    
    if (extract_audio_flag)
    {
        extract_list_parsing(globals.settings.indir, &extract);
#if !HAVE_core_BUILD
        
        ats2wav_parsing(globals.settings.indir, &extract, player);
#else
        ats2wav_parsing(globals.settings.indir, &extract, NULL);
#endif
        
        return(NULL);
    }
    
    
    // Coherence checks
    // You first have to test here.
#if !HAVE_core_BUILD
    menu_characteristics_coherence_test(img, ngroups);
    
#ifndef __CB__
#if !HAVE_mpeg2enc || !HAVE_mplex  || !HAVE_jpeg2yuv
    if (globals.topmenu <= RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR)
    {
        foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  You need mplex, mpeg2enc and jpeg2yuv to author\n       a background screen, please install these applications.");
        foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Continuing without menu authoring...");
        globals.topmenu=NO_MENU;
    }
    
#endif
#endif
    
    /* Fifth pass: it is now possible to safely copy files to temporary directory for menu and still pic creation  */
    // First parsing for input files (pics and mpgs)
    
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
        case 'Q':
            
#if  HAVE_lplex || HAVE_lplex_BUILD
            
            if (img->backgroundmpg)
            {
                foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Background mpg file already specified, skipping...");
                break;
            }
            
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Soundtrack(s) to be muxed into background mpg video: ", optarg);
            
            if (!optarg)
            {
                foutput("%s", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Resetting soundtrack input to default soundtrack...\n");
            }
            else
            {
                free(img->soundtrack[0][0]);
                img->audioformat=strdup("pcm");
                errno=0;
                
                char** array=NULL;
                array=fn_strtok(optarg, ':', array, img->nmenus, cutloop, NULL);
                
                img->soundtrack=(char ***) calloc(img->nmenus, sizeof(char**));
                
                if (!img->soundtrack) break;
                
                for (u=0; u < img->nmenus; u++)
                {
                    img->soundtrack[u]  =fn_strtok(array[u], ',', img->soundtrack[u], 0,NULL, NULL);
                }
                
                int v;
                for (u=0; u < img->nmenus; u++)
                    for (v=0; v < arraylength(img->soundtrack[u]); v++)
                        errno+=audit_soundtrack(img->soundtrack[u][v],AUDIT_STRICT_TOPMENU_AUDIO_FORMAT);
                
            }
            
            soundtracks_flag=1;
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 17:
            
#if  HAVE_lplex || lplex_BUILD
            
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Generate DVD-VIDEO audio tracks");
            if (globals.veryverbose) 
            {
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Will create DVD-VIDEO from following audio files:");
            }
            
            if (!optarg)
            {
                foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  No audio valid file paths were given on command line\n");
                EXIT_ON_RUNTIME_ERROR
            }
            else
            {
                parse_double_entry_command_line(optarg, &dvdv_track_array, &ndvdvtracks, &ndvdvtitleset1, AUDIT_DVD_VIDEO_AUDIO_FORMAT, ':');
                if (ndvdvtracks == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE("ndvdtracks null")
                dvdv_tracks_given=1;
            }
            
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 18:
            
#if HAVE_lplex || HAVE_lplex_BUILD
            
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Generate DVD-VIDEO slides");
            if (globals.veryverbose) 
            {
                foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Will create DVD-VIDEO slides from following files:");
            }
            
            if (!optarg)
            {
                foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  No audio file paths were given on command line\n");
                EXIT_ON_RUNTIME_ERROR
            }
            else
            {
                parse_double_entry_command_line(optarg, &dvdv_slide_array, &ndvdvslides, &ndvdvtitleset2, NO_FIXWAV_AUDIT,':');
                lplex_slides_flag=1;
            }
            
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 21:
            
#if HAVE_lplex || HAVE_lplex_BUILD
            
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Import DVD-Audio tracks to DVD-Video zone.");
            dvdv_import_flag=1;
            
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
            
        case 22:
#if HAVE_lplex || HAVE_lplex_BUILD
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Make mirror: import DVD-Audio tracks into DVD-Video zone\n       and resample them if necessary.");
            mirror_flag=1;
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 23:
#if HAVE_lplex || HAVE_lplex_BUILD
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Make mirror: import DVD-Audio tracks into DVD-Video zone\n       and resample them if necessary.");
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Mirroring strategy: %s\n",optarg);
            mirror_flag=1;
            if (strcmp(optarg, "high") == 0) mirror_st_flag=HIGH;
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 24:
#if HAVE_lplex || HAVE_lplex_BUILD
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Will create minimal hybrid disk.");
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Mirroring strategy: %s\n","high");
            hybridate_flag=1;
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 25:
#if HAVE_lplex || HAVE_lplex_BUILD
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Will create full hybrid disk.");
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Mirroring strategy: %s\n","high");
            full_hybridate_flag=1;
#else
            foutput("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Feature is unsupported. Install lplex from http://audioplex.sourceforge.net to activate it.\n");
#endif
            break;
            
        case 6 :
            
            img->topmenu_slide=calloc(img->nmenus, sizeof(char***));
            img->topmenu_nslides=calloc(img->nmenus, sizeof(uint16_t));
            
            if (!img->topmenu_slide) break;
            else
            {
                
                
                errno=0;
                
                char** array=NULL;
                array=fn_strtok(optarg, ':', array, img->nmenus, cutloop, NULL);
                
                if (!array)
                {
                    img->topmenu_slide[0]   =calloc(1, sizeof(char**));
                    img->topmenu_slide[0][0]=strdup(img->backgroundpic[0]);
                    img->topmenu_nslides[0] =1;
                    errno=0;
                }
                
                for (u=0; u < img->nmenus; u++)
                {
                    img->topmenu_slide[u]  =fn_strtok(array[u], ',', img->topmenu_slide[u], 0,NULL, NULL);
                    img->topmenu_nslides[u]=arraylength(img->topmenu_slide[u]);
                }
            }
            globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            
            break;
            
        case 'b':
            
            if (img->backgroundmpg)
            {
                foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Background mpg file already specified, skipping...");
                break;
            }
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  background jpg file(s) for generating mpg video: ", optarg);
            
            str=strdup(optarg);
            
            img->backgroundpic=fn_strtok(str,',',img->backgroundpic,0,NULL,NULL);
            int backgroundpic_arraylength=0;
            if ((backgroundpic_arraylength=arraylength(img->backgroundpic)) < img->nmenus)
            {
                
                foutput("%s\n",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You did not give enough filenames, completing with last one");
                for (u=0; u + backgroundpic_arraylength < img->nmenus; u++)
                    copy_file(img->backgroundpic[backgroundpic_arraylength-1], img->backgroundpic[u+backgroundpic_arraylength]);
            }
            
            
            free(str);
            globals.topmenu=Min(globals.topmenu, RUN_SPUMUX_DVDAUTHOR);
            img->refresh=1;
            
            break;
            
        case 'N':
            
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using %s top menu background picture.\n", optarg);
            
            
            img->blankscreen=strdup(optarg);
            int len, len2;
            len=strlen(img->blankscreen);
            len2=strlen(img->backgroundpic[0]);
            if  ((img->blankscreen[len-1] != 'g') || (img->blankscreen[len-2] != 'n') || (img->blankscreen[len-3] != 'p'))
            {
                foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  You should use a .png background picture... exiting.");
                clean_exit(EXIT_FAILURE);
            }
            // note that within a switch, some compilers do not authorize char dest[len+1]
            
            img->backgroundpic[0][len2-2]='p';
            img->backgroundpic[0][len2-3]='j';
            
            
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR );
            
            break;
            
        case 'E':
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  highlight png file(s) for generating mpg video: ", optarg);
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Check that your image doe not have more than 4 colors, including transparency.");
            str=strdup(optarg);
            
            img->highlightpic=fn_strtok(str,',',img->highlightpic,0,NULL,NULL);
            int highlight_arraylength=0;
            if ((highlight_arraylength=arraylength(img->highlightpic)) < img->nmenus)
            {
                
                foutput("%s\n",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You did not give enough filenames, completing with last one");
                for (u=0; u + highlight_arraylength < img->nmenus; u++)
                    copy_file(img->highlightpic[highlight_arraylength-1], img->highlightpic[u+highlight_arraylength]);
            }
            
            
            free(str);
            
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR );
            img->refresh=1;
            break;
            
        case 'e' :
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  select png file(s) for generating mpg video: ", optarg);
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Check that your image doe not have more than 4 colors, including transparency.");
            str=strdup(optarg);
            
            img->selectpic=fn_strtok(str,',',img->selectpic,0,NULL,NULL);
            int select_arraylength=0;
            if ((select_arraylength=arraylength(img->selectpic)) < img->nmenus)
            {
                
                foutput("%s\n",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You did not give enough filenames, completing with last one");
                for (u=0; u + select_arraylength < img->nmenus; u++)
                    copy_file(img->selectpic[select_arraylength-1], img->selectpic[u+select_arraylength]);
            }
            
            
            free(str);
            
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR );
            img->refresh=1;
            break;
            
            
        case 'G' :
            
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  image png file(s) for generating mpg video: ", optarg);
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Check that your image doe not have more than 4 colors, including transparency.");
            str=strdup(optarg);
            
            img->imagepic=fn_strtok(str,',',img->imagepic,0,NULL,NULL);
            int image_arraylength=0;
            if ((image_arraylength=arraylength(img->imagepic)) < img->nmenus)
            {
                
                foutput("%s\n",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You did not give enough filenames, completing with last one");
                for (u=0; u + image_arraylength < img->nmenus; u++)
                    copy_file(img->imagepic[image_arraylength -1], img->imagepic[u+image_arraylength ]);
            }
            
            
            free(str);
            
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR );
            img->refresh=1;
            break;
            
        case 2:
            
            foutput("%s%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Background color(s) for top (and active) menus : ", optarg);
            str=strdup(optarg);
            
            img->backgroundcolors=fn_strtok(str,':', img->backgroundcolors,0,NULL,NULL);
            int bgcolors_arraylength=0;
            if ((bgcolors_arraylength=arraylength(img->backgroundcolors)) < img->nmenus)
            {
                
                foutput("%s\n",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You did not give enough colors, completing with last one");
                for (u=0; u + bgcolors_arraylength < img->nmenus; u++)
                    img->backgroundcolors[u+bgcolors_arraylength]=img->backgroundcolors[bgcolors_arraylength -1];
            }
            
            free(str);
            globals.topmenu=Min(globals.topmenu, RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR);
            break;
            
            
        }
    }
    
    
    if ((img->nmenus)&&(img->blankscreen))
    {
        
        if (globals.veryverbose) foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Converting overlay .png blankscreen to .jg blankscreen for mpg authoring...");
        
        char* convert=NULL;
        char cl[500]; //do not use command as an array name !
        convert=create_binary_path(convert, CONVERT, SEPARATOR CONVERT_BASENAME);
        unlink(img->backgroundpic[0]);
        errno=0;
        change_directory(globals.settings.datadir);
        snprintf(cl, 500, "%s %s %s", convert, img->blankscreen , img->backgroundpic[0]);
        if (globals.veryverbose) foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Launching convert with command line %s\n",  cl);
        if (system(win32quote(cl)) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" System command failed")
        fflush(NULL);
    }
    
    _Bool menupic_input_coherence_test=0;
    
    if ((img->imagepic) && (img->highlightpic) && (img->selectpic))
    {
        for (u=0;  u < img->nmenus; u++)
        {
            if ((img->imagepic[u]) && (img->highlightpic[u]) && (img->selectpic[u]))
            {
                path_t *i,*h,*s;
                i=parse_filepath(img->imagepic[u]);
                h=parse_filepath(img->highlightpic[u]);
                s=parse_filepath(img->selectpic[u]);
                
                if ((i->isfile) && (h->isfile) && (s->isfile))
                    menupic_input_coherence_test=1;
            }
            else
            {
                if ((img->imagepic[u]) || (img->highlightpic[u]) || (img->selectpic[u]))
                {
                    foutput("%s",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You should enter three menu-authoring custom-made .png pictures, for main image, highlight and select action.\nn       Reverting to automatic mode.\n\n");
                    globals.topmenu=Min(globals.topmenu, RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR);
                }
            }
        }
    }
    
    // Now copying to temporary directory, depending on type of menu creation, trying to minimize work, depending of type of disc build.
    char* dest;
    
    // Operations related to top menu creation
    // TODO: consider adding silence.wav to silent slideshows.
    
    ///////////////////////
    // Some sanity tests //
    ///////////////////////
    
    if (soundtracks_flag)
    {
        if (img->topmenu_slide)
            launch_lplex_soundtrack(img, "mpeg");
        else
            if (import_topmenu_flag)
                import_topmenu(import_topmenu_path, img, MIX_NEW_SOUNDTRACK);
    }
    else
        if (import_topmenu_flag)
            import_topmenu(import_topmenu_path, img, USE_VTS_SOUNDTRACK);
    
    if (hybridate_flag)
    {
        dvdv_import_flag=1;
        mirror_flag=0;
        mirror_st_flag=0;
    }
    
    if (full_hybridate_flag)
    {
        mirror_flag=1;
        mirror_st_flag=HIGH;
        dvdv_import_flag=0;
    }
    
    if ( !dvdv_import_flag &&
         ( lplex_slides_flag  &&    (
               ( !ndvdvslides || !dvdv_slide_array )
               || ( !dvdv_tracks_given && !mirror_flag )))) 
    {  
        fprintf(stderr, "ndvdvslides[0]=%d\n", ndvdvslides[0]);
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Incoherent command line: slides requested for Lplex"J"...yet no audio tracks or slides given.")
    }
    
    if (dvdv_tracks_given)
    {
        
        if (ndvdvtitleset1 != ndvdvtitleset2)
        {
            fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Titleset count for slides (%d) and tracks (%d) is not the same.\n Fix the issue and relaunch.\n", ndvdvtitleset1, ndvdvtitleset2);
            EXIT_ON_RUNTIME_ERROR
        }
    }
    
    if ( dvdv_import_flag && mirror_flag )
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  You should not use --mirror along with --import-dvdv: do you really want to resample?\n       Exiting...\n");
    
    switch (globals.topmenu)
    {
    case TEMPORARY_AUTOMATIC_MENU:
    
    case AUTOMATIC_MENU:
    
    case RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR :
        change_directory(globals.settings.datadir);
        
        dest=copy_file2dir_rename(img->backgroundpic[0], globals.settings.tempdir, "bgpic0.jpg");
        
        if (img->nmenus > 1)
            for (u=1; u < img->nmenus; u++)
            {
                char name[13];
                sprintf(name, "%s%d%s", "bgpic", u,".jpg");
                dest=copy_file2dir_rename(img->backgroundpic[0], globals.settings.tempdir, name);
            }
        if (dest == NULL)  EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Failed to copy background .jpg pictures to temporary directory.")
                free(dest);
        
    case RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR:
        change_directory(globals.settings.datadir);
        
        dest=copy_file2dir(img->blankscreen, globals.settings.tempdir);
        
        if (dest == NULL)  EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Failed to copy background .png blankscreen to temporary directory.")
                free(dest);
        
        if (!menupic_input_coherence_test)
            normalize_temporary_paths(img);
        
        change_directory(globals.settings.workdir);
        
    case RUN_SPUMUX_DVDAUTHOR:
        if ((img->imagepic==NULL) && (img->selectpic==NULL)&& (img->highlightpic==NULL))
        {
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  You need all subtitle images");
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Continuing with menu picture authoring...");
            globals.topmenu=Min(RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR, globals.topmenu);
        }
        break;
        
        
    case RUN_DVDAUTHOR:
        if ((img->backgroundmpg) && (globals.xml))
        {
            errno=0;
            if (fopen(globals.xml, "rb") != NULL)
                fclose(fopen(globals.xml, "rb"));
            if (!errno) puts(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  --> dvdauthor requirement...OK");
        }
        else errno=1;
        break;
        
    case TS_VOB_TYPE:
        if (img->tsvob)
        {
            errno=0;
            
            FILE *f;
            if ((f=fopen(img->tsvob, "rb")) != NULL)
            {
                fclose(f);
                puts(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  --> top vob requirement...OK");
            }
            
            
        }
        break;
    case NO_MENU:
        break;
    default:
        errno=1;
    }
    
    if ((errno)&&(user_command_line))
    {
        foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Not enough information. Continuing with automatic menu authoring...");
        globals.topmenu=AUTOMATIC_MENU;
        // retest now
        menu_characteristics_coherence_test(img, ngroups);
        errno=0;
    }
    
    // Operations related to stills
        
    if (stillpic_string)
    {
        // otherwise, if stillpic_strings given and no active menu, do this,
        
        // heap-allocations is not possible if char** is not returned by function
        // A simple char* would well be allocated by function, not a char**.
        
        fprintf(stderr, ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  stillpic_string=%s\n", stillpic_string);
        pics_per_track=fn_strtok(stillpic_string, ':', pics_per_track, 0,NULL,NULL);
        uint16_t dim,DIM=0,w;
        
        img->npics =(uint16_t*) calloc(totntracks, sizeof(uint16_t));
        if (img->npics == NULL)
        {
            perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" img->npics");
            goto standard_checks;
        }
        if (pics_per_track)
            w=arraylength(pics_per_track);
        else
        {
            perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  pics_per_track");
            goto standard_checks;
        }
        if (w > totntracks)
        {
            fprintf(stderr, "\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Too many tracks on --stillpics: %d\n",w);
            goto standard_checks;
        }
        else if (w < totntracks)
        {
            fprintf(stderr, "\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  You forgot at least one track on --stillpics:\n  total number of tracks:%d whilst pic string array has length %d\n", totntracks, w);
            goto standard_checks;
        }
        
        picks_per_track_double_array=calloc(totntracks, sizeof(char**));
        if (picks_per_track_double_array == NULL) EXIT_ON_RUNTIME_ERROR;
        
        for (k=0; k < totntracks; k++)
        {
            if (globals.debugging) 
                fprintf(stderr,ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  Parsing pictures for track %d\n",k);
            
            picks_per_track_double_array[k]=fn_strtok(pics_per_track[k],
                                                      ',',
                                                      picks_per_track_double_array[k],
                                                      -1,
                                                      create_stillpic_directory,
                                                      NULL);
            
            dim=0;
            w=0;
            
            if (picks_per_track_double_array[k]) 
            {
                while (picks_per_track_double_array[k][w] != NULL)
                {
                    if (picks_per_track_double_array[k][w][0] != 0) dim++;
                    w++;
                }
            }
            else
            {
                perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  picks_per_track_double_array");
                goto standard_checks;
            }
            
            npics[k]=(k)? dim+npics[k-1]: dim;
            img->npics[k]=dim;
            DIM+=dim;
            if (globals.debugging) fprintf(stderr, "\n"ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  number of pics for track %d: npics[%d] = %d\n", k,k, dim);
            
            if (img->npics[k] > 99)
            {
                foutput("%s", "\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  The maximum number of pics per track is 99.\n");
                EXIT_ON_RUNTIME_ERROR_VERBOSE("Exiting...");
            }
        }
        
        FREE(pics_per_track)
                img->stillpicvobsize=(uint32_t*) calloc(DIM, sizeof(uint32_t));
        if (img->stillpicvobsize == NULL)
        {
            perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  still pic vob size array");
            goto standard_checks;
        }
        img->count=DIM;
        if (globals.debugging) fprintf(stderr,ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  Total of %d pictures\n", img->count);
    }
    // or allocate img->blankscreen for dvdv slides by default.
   
    
    if (still_options_string)
        still_options_parsing(still_options_string, img);
   
    
#endif
    
    // Final standard checks
    
standard_checks:
    
    if (nplaygroups > ngroups-nvideolinking_groups)
    {
        if (globals.debugging) foutput(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  There cannot be more copy groups than audio groups. Limiting to %d groups...\n", ngroups-nvideolinking_groups);
        nplaygroups=ngroups-nvideolinking_groups;
    }
    
    if ( nplaygroups+ngroups > 8)
    {
        if (globals.debugging) foutput("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  There cannot be more copy groups than audio groups. Limiting to 9 groups...");
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
    
    if (user_command_line)    
    {
        scan_wavfile_audio_characteristics(command);
    }
        
    process_dvd_video_zone(command);
  
    user_command_line++;
    return(command);
}


void process_dvd_video_zone(command_t* command)
{
#if ! HAVE_core_BUILD
  
 #if HAVE_lplex  
    if (hybridate_flag || full_hybridate_flag)
    {
        
        lplex_slides_flag=1;
        int g=0;
        dvdv_slide_array=calloc(command->ngroups, sizeof(char**));
        if (dvdv_slide_array == NULL) EXIT_ON_RUNTIME_ERROR
        dvdv_slide_array[0]=calloc(command->ntracks[g], sizeof(char *));        
        if (dvdv_slide_array[0] == NULL) EXIT_ON_RUNTIME_ERROR
        if (ndvdvslides == NULL) ndvdvslides=calloc(command->ngroups, sizeof(uint8_t));
        if (ndvdvslides == NULL) EXIT_ON_RUNTIME_ERROR
        
        int N=command->ntracks[0];
        int T=0, TT=0;
        
        for (int t=0;  t < totntracks; t++)
        {
            if (g < command->ngroups-1 && t >= N)
            {
                T = 0;
                TT=0;
                N += command->ntracks[g];    
                g++;
                dvdv_slide_array[g]=calloc(command->ntracks[g], sizeof(char *));
                if (dvdv_slide_array[g] == NULL) EXIT_ON_RUNTIME_ERROR
            }
            
            
          if (full_hybridate_flag || (hybridate_flag && files[g][T].dvdv_compliant))          
          {
            if (picks_per_track_double_array == NULL  ||
                 picks_per_track_double_array[t] == NULL ||
                 picks_per_track_double_array[t][0] == NULL)
                 {
                   char slide[strlen(globals.settings.workdir)+STRLEN_SEPARATOR+strlen(img->blankscreen)+1];
                   sprintf(slide, "%s%s%s", globals.settings.workdir,SEPARATOR,img->blankscreen);
                          dvdv_slide_array[g][TT]=strdup(slide);  
                 }
                 else
                 {
                      dvdv_slide_array[g][TT]=strdup(picks_per_track_double_array[t][0]);
                 }  
                 
            ndvdvslides[g]++;
            TT++;
          }
          
         T++;   
        }
        
        for (int u=0; u <= g ; u++) fprintf(stderr, "\n[PICS]  %d\n", ndvdvslides[u]);

    }
    
    FREE(picks_per_track_double_array)
    free(stillpic_string);
       
    if (dvdv_tracks_given)
    {
        globals.videozone=0;
        
        foutput("%s\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  With --dvdv-tracks, no testing of audio file compliance will be performed!");
        launch_lplex_hybridate(img, 
                               "dvd", 
                               (const char***) dvdv_track_array, 
                               (const uint8_t*) ndvdvtracks, 
                               (const char***) dvdv_slide_array, 
                               ndvdvslides,
                               (const int) ndvdvtitleset1); 
        
        free(dvdv_track_array);
        free(dvdv_slide_array);
        free(ndvdvslides);
        free(ndvdvtracks);
    }
    
    if (dvdv_import_flag)
    {
        ndvdvtitleset1=0;
        dvdv_track_array=(char***) calloc(command->ngroups, sizeof(char**));  // is a maximum
        ndvdvtracks=(uint8_t*) calloc(command->ngroups, sizeof(uint8_t));        
        int* cut_table[command->ngroups];
        int delta_titlesets=0;
        
        for (int group=0; group < command->ngroups; group++)
        {

            for (int track=0; track < command->ntracks[group]; track++)
            {
                if (ndvdvtracks == NULL) EXIT_ON_RUNTIME_ERROR
                if(files[group][track].dvdv_compliant)                        
                {
                    if (globals.veryverbose) 
                    {
                        foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Tested DVD-Video compliant: %s\n", files[group][track].filename);
                        foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  group %d track %d: bits per sample=%d samplerate=%d\n", 
                                       group,
                                       track, 
                                       files[group][track].bitspersample,
                                       command->files[group][track].samplerate);
                    }

                    ndvdvtracks[group]++;
                    
                }
                else
                {
                    if (globals.veryverbose) 
                    {
                        foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Failed to be tested DVD-Video compliant: %s\n", command->files[group][track].filename);
                        foutput(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  group %d track %d: bits per sample=%d samplerate=%d\n", group, track, command->files[group][track].bitspersample, command->files[group][track].samplerate);
                    }
                }
                
            }
            
            dvdv_track_array[group]=(char**) calloc(ndvdvtracks[group], sizeof(char*)); 
            uint32_t lplex_audio_characteristics_test[ndvdvtracks[group]];
            memset(lplex_audio_characteristics_test, '0', ndvdvtracks[group]);
            cut_table[group]=(int*) calloc(ndvdvtracks[group], sizeof(int));
            cut_table[group][0]=1;    
            
            int TT=0;
            for (int track=0; track < command->ntracks[group]; track++) 
            {
                
                if (command->files[group][track].dvdv_compliant)
                {
                   dvdv_track_array[group][TT]=strdup(command->files[group][track].filename); 
                   lplex_audio_characteristics_test[TT]=(((uint8_t)files[group][track].bitspersample)<<16)
                                                             |(((uint8_t)(files[group][track].samplerate/1000)) << 8)
                                                             |files[group][track].channels;
                                                              
                   if  (TT && lplex_audio_characteristics_test[TT] != lplex_audio_characteristics_test[TT-1])
                   {
                       foutput(ANSI_COLOR_RED"\n[WAR]"ANSI_COLOR_RESET
                               "  Lplex requests that tracks have same audio-characteristics for in a given titleset.\n       Found different audio for tracks %s and %s", 
                               dvdv_track_array[group][TT],
                               dvdv_track_array[group][TT-1]);
                       
                       foutput(ANSI_COLOR_RED"\n[WAR]"ANSI_COLOR_RESET"  %d, %d\n", lplex_audio_characteristics_test[TT], lplex_audio_characteristics_test[TT-1]);                            
                       
                       foutput("%s\n", ANSI_COLOR_RED"\n[WAR]"ANSI_COLOR_RESET"  Adding titleset");
                       delta_titlesets++;
                       cut_table[group][TT]=1;
                   }
                   
                   TT++;
                }
                
            }
            
            if (ndvdvtracks[group]) ndvdvtitleset1++;
    
        }
        
   
        globals.videozone=0;
        
       if (ndvdvtitleset1 == 0)
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  You requested hybridation yet no DVD-Video standard-compliant\n       audio file was found on input.\n") 
        
       if (delta_titlesets)
           {
            uint8_t new_ntracks[9]={0};
            uint8_t ndvdvslides[9]={0};
            
            char*** new_dvdv_track_array;
            char*** new_dvdv_slide_array;
            if (globals.veryverbose) 
                   foutput(ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  %d titlesets will have to be added.\n", delta_titlesets);
                   
             if (ndvdvtitleset1+delta_titlesets > 99)      
               EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Exceeded 99 titleset limit.\n       Redesign your audio input so that you do not have more than 99 different audio formats in a row.")
             
             new_dvdv_track_array=(char***) calloc(ndvdvtitleset1 + delta_titlesets,sizeof(dvdv_track_array));
             new_dvdv_slide_array=(char***) calloc(ndvdvtitleset1 + delta_titlesets, sizeof(dvdv_slide_array));
            
             int newgroup=-1;
             
             for (int group=0; group < ndvdvtitleset1 && newgroup < ndvdvtitleset1+delta_titlesets; group++)
             {
                   for (int track=0; track < ndvdvtracks[group]; track++)
                   {
                     if (cut_table[group][track]) newgroup++;
                     new_ntracks[newgroup]++;
                   }
             }
             
             for (int newgroup=0; newgroup< ndvdvtitleset1 +delta_titlesets; newgroup++)
             {
               new_dvdv_track_array[newgroup]=calloc(new_ntracks[newgroup], sizeof(dvdv_track_array[newgroup]));
               new_dvdv_slide_array[newgroup]=calloc(new_ntracks[newgroup], sizeof(dvdv_slide_array[newgroup]));
               ndvdvslides[newgroup]=0;
             }
             
             newgroup=0;
             
             for (int group=0; group < ndvdvtitleset1 ; group++)
             {
                int N=0;
                while (N < ndvdvtracks[group])
                {
                  for (int track=0; track < new_ntracks[newgroup]; track++)
                  {
                    new_dvdv_track_array[newgroup][track]=strdup(dvdv_track_array[group][track+N]);
                    new_dvdv_slide_array[newgroup][track]=strdup(dvdv_slide_array[group][track+N]);
                    ndvdvslides[newgroup]++;
                    free(dvdv_slide_array[group][track+N]);
                    free(dvdv_track_array[group][track+N]);
                  }
                  N+=new_ntracks[newgroup];
                  newgroup++;
                }
                
                  free(dvdv_slide_array[group]);
                  free(dvdv_track_array[group]);
             }
             
            launch_lplex_hybridate(img, 
                                   "dvd", 
                                   (const char***) new_dvdv_track_array, 
                                   (const uint8_t*) new_ntracks, 
                                   (const char***) new_dvdv_slide_array, 
                                   ndvdvslides,
                                   (const int) ndvdvtitleset1+delta_titlesets); 
                                   
            for (int group=0; group < ndvdvtitleset1+delta_titlesets; group++) 
            {
                for (int track=0; track < new_ntracks[group]; track++)
                {
                      free(new_dvdv_track_array[group][track]);
                      free(new_dvdv_slide_array[group][track]);    
                }
                
                 if (group < ndvdvtitleset1) free(cut_table[group]);
                 free(new_dvdv_track_array[group]);
                 free(new_dvdv_slide_array[group]);
            }
           }
     else
     {
          launch_lplex_hybridate(img, 
                               "dvd", 
                               (const char***) dvdv_track_array, 
                               (const uint8_t*) ndvdvtracks, 
                               (const char***) dvdv_slide_array, 
                               ndvdvslides,
                               (const int) ndvdvtitleset1); 
                               
            for (int group=0; group < ndvdvtitleset1; group++) 
            {
              for (int track=0; track < ndvdvtracks[group]; track++)
              {
             //    free(dvdv_track_array[group][track]);
                 free(dvdv_slide_array[group][track]);    
              }
              
             // free(cut_table[group]);
            //  free(dvdv_track_array[group]);
            //  free(dvdv_slide_array[group]);
            }
     }   
        
    }
    
    if (mirror_flag)
    {
        ndvdvtitleset1=0;
        dvdv_track_array=(char***) calloc(command->ngroups, sizeof(char**));  // now lo longer a maximum
        int* cut_table[command->ngroups];
        int delta_titlesets=0;
        
        for (int group=0; group < command->ngroups; group++)
        {
            dvdv_track_array[group]=(char**) calloc(command->ntracks[group], sizeof(char*)); 
            uint32_t lplex_audio_characteristics_test[command->ntracks[group]];
            memset(lplex_audio_characteristics_test, '0', command->ntracks[group]);
            cut_table[group]=(int*) calloc(command->ntracks[group], sizeof(int));
            
            for (int track=0; track < command->ntracks[group]; track++)
            {
                
                if(files[group][track].dvdv_compliant) 
                {
                    dvdv_track_array[group][track]=strdup(files[group][track].filename);
                    lplex_audio_characteristics_test[track]=(((uint8_t)files[group][track].bitspersample)<<16)
                                                             |(((uint8_t)(files[group][track].samplerate/1000)) << 8)
                                                             |files[group][track].channels;
                }
                else
                {
                    int new_sample_rate=0;
                    int new_bit_rate=0;
                    unsigned sample_floor;
                    unsigned bps_floor;
                                        
                    if (mirror_st_flag == HIGH)
                    {
                     sample_floor=48000;
                     bps_floor=16;
                    }
                    else //LOW
                    {
                     sample_floor=96000;
                     bps_floor=24;
                    }

                    new_sample_rate=(files[group][track].samplerate <= sample_floor)? 48000 : 96000;
                    new_bit_rate=(files[group][track].bitspersample <= bps_floor)? 16 : 24;
                         
                    lplex_audio_characteristics_test[track]=(new_bit_rate<<16)|((new_sample_rate/1000)<<8)|files[group][track].channels;                    
                    int size=strlen(files[group][track].filename);
                    if (strcmp(files[group][track].filename+size-4, ".wav") !=0)
                    {
                        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Automatic mirroring is only supported for wav files.")
                    }
                    
                    char newpath[size+4];
                    strncpy(newpath, files[group][track].filename,size-4);
                    sprintf(newpath+size-4, "%s", ".res.wav");
                    
                    dvdv_track_array[group][track]=strdup(newpath);
                    char new_bit_rate_str[3]={0};
                    char new_sample_rate_str[6]={0};
                    sprintf(new_bit_rate_str, "%d", new_bit_rate);
                    sprintf(new_sample_rate_str, "%d", new_sample_rate);
                    resample(files[group][track].filename,dvdv_track_array[group][track],new_bit_rate_str, new_sample_rate_str);
                }
            }
            
            /* Now checking the lplex constraint on same-type audio characteristics per titleset */
            cut_table[group][0]=1;    
            for (int i=1; i < command->ntracks[group]; i++)
               if  (lplex_audio_characteristics_test[i] != lplex_audio_characteristics_test[i-1])
                {
                  foutput(ANSI_COLOR_RED"\n[WAR]"ANSI_COLOR_RESET
                          "  Lplex requests that tracks have same audio-characteristics for in a given titleset.\n       Found different audio for tracks %s and %s", 
                          dvdv_track_array[group][i],
                          dvdv_track_array[group][i-1]);
                          
                  foutput(ANSI_COLOR_RED"\n[WAR]"ANSI_COLOR_RESET"  %d, %d\n", lplex_audio_characteristics_test[i], lplex_audio_characteristics_test[i-1]);                            
                  
                  foutput("%s\n", ANSI_COLOR_RED"\n[WAR]"ANSI_COLOR_RESET"  Adding titleset");
                  delta_titlesets++;
                  cut_table[group][i]=1;
                }
                

        }
        
       uint8_t new_ntracks[9]={0};
       uint8_t ndvdvslides[9]={0};
               
       char*** new_dvdv_track_array;
       char*** new_dvdv_slide_array;
        
       if (delta_titlesets)
           {
             if (globals.veryverbose) 
                   foutput(ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  %d titlesets will have to be added.\n", delta_titlesets);
                   
             if (command->ngroups+delta_titlesets > 99)      
               EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Exceeded 99 titleset limit.\n       Redesign your audio input so that you do not have more than 99 different audio formats in a row.")
             
             new_dvdv_track_array=(char***) calloc(command->ngroups + delta_titlesets,sizeof(dvdv_track_array));
             new_dvdv_slide_array=(char***) calloc(command->ngroups + delta_titlesets, sizeof(dvdv_slide_array));
            
             int newgroup=-1;
             
             for (int group=0; group < command->ngroups && newgroup < command->ngroups+delta_titlesets; group++)
             {
                   for (int track=0; track < command->ntracks[group]; track++)
                   {
                     if (cut_table[group][track]) newgroup++;
                     new_ntracks[newgroup]++;
                   }
             }
             
             for (int newgroup=0; newgroup< command->ngroups +delta_titlesets; newgroup++)
             {
               new_dvdv_track_array[newgroup]=calloc(new_ntracks[newgroup], sizeof(dvdv_track_array[newgroup]));
               new_dvdv_slide_array[newgroup]=calloc(new_ntracks[newgroup], sizeof(dvdv_slide_array[newgroup]));
               ndvdvslides[newgroup]=0;
             }
             
             newgroup=0;
             
             for (int group=0; group < command->ngroups ; group++)
             {
                int N=0;
                while (N < command->ntracks[group])
                {
                  for (int track=0; track < new_ntracks[newgroup]; track++)
                  {
                    new_dvdv_track_array[newgroup][track]=strdup(dvdv_track_array[group][track+N]);
                    new_dvdv_slide_array[newgroup][track]=strdup(dvdv_slide_array[group][track+N]);
                    ndvdvslides[newgroup]++;
                    free(dvdv_slide_array[group][track+N]);
                    free(dvdv_track_array[group][track+N]);
                  }
                  N+=new_ntracks[newgroup];
                  newgroup++;
                }
                
                free(dvdv_slide_array[group]);
                free(dvdv_track_array[group]);
             }
           }
           
            
        globals.videozone=0;
        
        if (delta_titlesets)
        {
            launch_lplex_hybridate(img, 
                                   "dvd", 
                                   (const char***) new_dvdv_track_array, 
                                   (const uint8_t*) new_ntracks, 
                                   (const char***) new_dvdv_slide_array, 
                                   ndvdvslides,
                                   (const int) command->ngroups+delta_titlesets); 
                                   
            for (int group=0; group < command->ngroups+delta_titlesets; group++) 
            {
                for (int track=0; track < new_ntracks[group]; track++)
                {
                    free(new_dvdv_track_array[group][track]);
                    free(new_dvdv_slide_array[group][track]);    
                }
                
                if (group < command->ngroups) free(cut_table[group]);
                free(new_dvdv_track_array[group]);
                free(new_dvdv_slide_array[group]);
            }
        }
        else
        {
            launch_lplex_hybridate(img, 
                                   "dvd", 
                                   (const char***) dvdv_track_array, 
                                   (const uint8_t*) command->ntracks, 
                                   (const char***) dvdv_slide_array, 
                                   ndvdvslides,
                                   (const int) command->ngroups); 
                               
            for (int group=0; group < command->ngroups; group++) 
            {
              for (int track=0; track < command->ntracks[group]; track++)
              {
                  free(dvdv_track_array[group][track]);
                  free(dvdv_slide_array[group][track]);    
              }
              
              free(cut_table[group]);
              free(dvdv_track_array[group]);
              free(dvdv_slide_array[group]);
            }
        }
        
    }
#endif

#endif 
    
}

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
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav: simple mode activated, advanced features deactivated.");
            globals.fixwav_automatic=0;
            break;
            
        case 1:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav: prepending header to raw file.");
            globals.fixwav_prepend=1;
            break;
            
        case 2:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav: file header will be repaired in place.");
            globals.fixwav_in_place=1;
            break;
            
        case 3:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav: interactive mode activated.");
            globals.fixwav_interactive=1;
            break;
            
        case 4:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav: padding activated.");
            globals.fixwav_padding=1;
            break;
            
        case 5:
            foutput("%s\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav: pruning silence at end of files.");
            globals.fixwav_prune=1;
            break;
            
        case 6:
            
            FREE(globals.fixwav_suffix)
                    globals.fixwav_suffix=strndup(value, MAX_OPTION_LENGTH);
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav output suffix: %s\n", globals.fixwav_suffix);
            break;
            
        case 7:
            globals.fixwav_force=1;
            foutput("%s", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav will be launched before SoX for seriously mangled headers.\n");
            break;
            
        case 8:
            globals.fixwav_cautious=1;
            foutput("%s", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav will ask user permission to overwrite files in place.\n");
            break;
            
        case 9:
            FREE(globals.settings.fixwav_database)
                    globals.settings.fixwav_database=strndup(value, MAX_OPTION_LENGTH);
            if (!globals.nooutput) {
                secure_mkdir(globals.settings.fixwav_database, 0755);
                foutput("%s       %s%s", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Fixwav will output info chunk from wav headers to:\n", globals.settings.fixwav_database, SEPARATOR "database\n");
            }
            break;
        }
    }
    
    return;
}


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
        foutput("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Analysing --extract/--play suboptions...");
    
    
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
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Incorrect --extract suboptions, format is --extract=group1:track1,...,groupN:trackN\n       Skipping...");
            return;
        }
        int trackindex=atoi(subchunk+2);
        
        if ((groupindex > 8) || (groupindex < 0) || (nextractgroup == 8) || (trackindex > 98))
        {
            groupindex=0;
            foutput("%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Incorrect --extract/--play suboption, exceeding limits reset to 0.");
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
        foutput("%s",ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  EXTRACTING: titleset   |   track\n");
        int k;
        for (j=0; j < 9; j++)
            for (k=0; k < 99; j++)
                if ((extract->extracttitleset[j]) && (extract->extracttrackintitleset[j][k]))
                    foutput( ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"                   %02d      |      %02d\n", j, k );
    }
    
    /* all-important, otherwise irrelevant EXIT_ON_RUNTIME_ERROR will be generated*/
    
    extract->nextractgroup[0]=nextractgroup;
    errno=0;
    FREE(chain)
            
}


void ats2wav_parsing(const char * arg, extractlist* extract, char* player)
{
    
    char * chain, list[9];
    DIR *dir;
    memset(list, 0, 9);
    
    chain=strdup(arg);
    globals.settings.indir=calloc(strlen(arg)+1+9, sizeof(char));
    
    if (globals.settings.indir == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not allocate global settings")
            
            sprintf(globals.settings.indir, "%s"SEPARATOR"AUDIO_TS", chain);
    
    change_directory(globals.settings.indir);
    if ((dir=opendir(globals.settings.indir)) == NULL)
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not open output directory")
                
                foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Extracting audio from %s\n", globals.settings.indir);
    
    
    if (extract->nextractgroup[0])
    {
        
        parse_disk(dir, globals.access_rights, extract, player);
    }
    else
        
        parse_disk(dir, globals.access_rights, NULL, player);
    
    if (closedir(dir) == -1)
        foutput( "%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to close dir");
    
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
    if (img->options == NULL) perror(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  still options parsing");
    for (k=0; k<img->count; k++)
    {
        img->options[k]=calloc(1, sizeof(stilloptions));
        if (img->options[k] == NULL) perror(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  still options parsing");
    }
    // TODO: free them
    
    
    
    while ((subopt = getsubopt(&chain, tokens, &value)) != -1)
    {
        switch (subopt)
        {
        case 0:
            temp=atoi(value);
            if (temp >= img->count)
            {
                foutput(""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Index %d should be lower than %d. Start at index 0. Skipping...\n", temp, img->count);
                break;
            }
            rank=temp;
            foutput("%s%d\n", ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Options for still #", rank);
            break;
            
        case 1:
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  #%d: Manual browsing enabled.\n", rank);
            img->options[rank]->manual=1;
            break;
            
        case 2:
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  #%d: start effect is: %s.\n", rank, value);    //  or: cut, fade, dissolve, top, bottom, left, right
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
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  #%d: end effect is: %s.\n", rank, value);
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
                foutput("%s", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Lag should be lower than 16, skipping...\n");
                break;
            }
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  #%d: effect lag is: %d*0.32s=%fs.\n", rank, lag, (float) lag*0.32);
            img->options[rank]->lag=lag;
            break;
            
        case 5:
            img->options[rank]->onset=atoi(value);
            break;
            
        case 6:
            img->options[rank]->manual=1;
            img->options[rank]->active=1;
            foutput(ANSI_COLOR_MAGENTA"[PAR]"ANSI_COLOR_RESET"  Using active menus for #%d.\n", rank);
            break;
            
        }
    }
    
    return;
}
#endif
