
//#undef __STRICT_ANSI__

#if !defined HAVE_core_BUILD || !HAVE_core_BUILD

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#ifndef __WIN32__
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include "structures.h"
#include "c_utils.h"
#include "launch_manager.h"
#include "winport.h"
#include "auxiliary.h"
#include "amg.h"
#include "menu.h"
#include "commonvars.h"



// Automated top-menu generation using patched dvdauthor
// We authorize only maximal resolution form input pics (ie: 720x576, pal/secam or 720x480, ntsc)

uint16_t norm_x=PAL_X, norm_y=PAL_Y;  // TODO: adjust for ntsc #define NTSC_Y 480
extern uint16_t totntracks;


void menu_characteristics_coherence_test(pic* img, uint8_t ngroups, globalData* globals)
{
    if (img->active)
    {
        if (globals->topmenu == NO_MENU)
            globals->topmenu=TEMPORARY_AUTOMATIC_MENU; // you need to create a TS_VOB at least temporarily
        if (img->nmenus > 1)
        {
            foutput("%s", WAR "Active menus can only be used with simple menus for version "VERSION"\n       Using img->nmenus=1...\n");
            img->nmenus=1;
        }
        if (img->hierarchical)
        {
            foutput("%s", WAR "Active menus cannot be used with hierarchical menus for version "VERSION"\n       Choosing hierarchical menus...\n");
            img->active=0;
            img->hierarchical=1;
        }
    }

    // default values must be set even if globals->topmenu = NO_MENU

    if (ngroups)
    {

        if (img->nmenus == 0)
        {
            if (img->ncolumns == 0) img->ncolumns=DEFAULT_MENU_NCOLUMNS;  // just in case, not to divide by zero, yet should not arise unless...
            if (img->hierarchical) img->nmenus=ngroups+1; // list of groups and one menu per group only (--> limitation to be indicated)
            else

                img->nmenus=ngroups/img->ncolumns + (ngroups%img->ncolumns > 0);  // number of columns cannot be higher than img->ncolumns; adjusting number of menus to ensure this.
            if (globals->topmenu != NO_MENU) foutput(MSG_TAG "With %d columns, number of menus will be %d\n", img->ncolumns, img->nmenus);
        }
        else
        {
            if ((img->hierarchical) && (img->nmenus == 1))
            {
                foutput("%s", WAR "Hierarchical menus should have at least two screens...\n       Incrementing value for --nmenus=1->2\n");
                img->nmenus++;
            }

            img->ncolumns=ngroups/(img->nmenus-img->hierarchical)+(ngroups%(img->nmenus-img->hierarchical) >0);

            if ((img->ncolumns)*ngroups < img->nmenus-1)
            {
                foutput(WAR "Hierarchical menus should have at most %d*%d+1=%d menus...\n       Resetting value for --nmenus=%d\n", img->ncolumns, ngroups, img->ncolumns*ngroups+1, img->ncolumns*ngroups+1);
                img->nmenus=ngroups*img->ncolumns+1;
            }

        }

        img->maxbuttons=Min(MAX_BUTTON_Y_NUMBER-2,totntracks)/img->nmenus;
        img->resbuttons=Min(MAX_BUTTON_Y_NUMBER-2,totntracks)%img->nmenus;
    }


}



/* patches AUDIO_TS.VOB into an active-menu type AUDIO_SV.VOB at minor processing cost */

void create_activemenu(pic* img, globalData* globals)
{
    if (img->tsvob == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE( "No matrix AUDIO_TS.VOB available for generating active menus.")

    uint8_t j;
    uint64_t i;
    uint64_t activeheadersize=0;

    char* activeheader=copy_file2dir(img->activeheader, globals->settings.tempdir, globals);
    activeheadersize = stat_file_size(activeheader);

    FILE* activeheaderfile=NULL;

    if (!globals->nooutput)
        activeheaderfile=fopen(activeheader, "rb");

    /* processing */

    foutput("%s\n", INF "Using already created top menus.\n");

    uint64_t tsvobsize=0;
    tsvobsize = stat_file_size(img->tsvob);
    if (tsvobsize <= activeheadersize)
    {
        perror(ERR "AUDIO_TS.VOB is too small.\n");
        clean_exit(EXIT_FAILURE, globals) ;
    }
    uint8_t tsvobpt[tsvobsize];
    memset(tsvobpt, 0, tsvobsize);

    FILE * tsvobfile=fopen(img->tsvob, "rb");

    if (!globals->nooutput && fread(tsvobpt, activeheadersize, 1, activeheaderfile) == 0) perror(ERR "fread [active menu authoring, stage 1]");

    if (-1 == fseek(tsvobfile, ACTIVEHEADER_INSERTOFFSET, SEEK_SET)) perror(ERR "fseek [active menu authoring, stage 2]");

    if (fread(tsvobpt+activeheadersize+32, 0x314-ACTIVEHEADER_INSERTOFFSET, 1, tsvobfile) == 0) perror(ERR "fread [active menu authoring, stage 3]");
//nlinks=1;
    i=activeheadersize;

    tsvobpt[i]=0x10;
    tsvobpt[i+3]=(uint8_t) totntracks; // A max of 256 links ?
    tsvobpt[i+4]=(uint8_t) totntracks;
    uint32_copy(&tsvobpt[i+8],   0x32FFDD00);//uint32_copy(&tsvobpt[i+8],   0x0010B0B0);
    uint32_copy(&tsvobpt[i+12],   0x34FFDD00);//uint32_copy(&tsvobpt[i+12],   0x0020B090);

    i=0x314;
    uint32_copy(&tsvobpt[i], 0x01BE04E8);
    i+=4;
    while (i < 0x800)
    {
        tsvobpt[i]=0xFF;
        i++;
    }
    if (-1 == fseek(tsvobfile, 0x800, SEEK_SET)) perror(ERR "fseek [active menu authoring, stage 4]");
    if (fread(tsvobpt+0x800, tsvobsize-0x800, 1, tsvobfile) == 0) perror(ERR "fread, stage 1, create_activemenu");


    /* writing */
    if (img->stillvob == NULL)
    {
        img->stillvob=strdup(img->tsvob);
        if (img->stillvob)
        {
            img->stillvob[strlen(img->stillvob)-5]='V';
            img->stillvob[strlen(img->stillvob)-6]='S';
        }
        else
        {
            perror(ERR " stillvob string allocation.\n");
            return;
        }
    }


    FILE* svvobfile;
    if (!globals->nooutput)
    {
     svvobfile=fopen(img->stillvob, "wb");
     if (svvobfile == NULL)
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "Cannot open AUDIO_SV.VOB for generating active menus.")

     foutput("\n"DBG "Creating active menu: will patch AUDIO_TS.VOB into AUDIO_SV.VOB=%s\n\n",img->stillvob);

     for (j=0; j < totntracks; j++)
         fwrite(tsvobpt, tsvobsize, 1, svvobfile);
     fclose(svvobfile);
    }

    free(activeheader);

    if (globals->topmenu == TEMPORARY_AUTOMATIC_MENU)
    {
        unlink(img->tsvob);
        img->tsvob = NULL;
    }

    return;
}

char* mp2enc=NULL;
char* jpeg2yuv=NULL;
char* mpeg2enc=NULL;
char* mplex=NULL;
char* mogrify=NULL;
char* dvdauthor=NULL;
char* spumux=NULL;
char* convert=NULL;
char* mpeg2dec=NULL;
char* pgmtoy4m=NULL;
static char* curl=NULL;
char* extract_ac3=NULL;
char* ac3dec=NULL;

void initialize_binary_paths(char level, globalData* globals)
{
    ///   saves ressources by ensuring this is done just once  ///
    static uint16_t count1, count2, count3, count4, count5, count6, count7;
    switch (level)
    {

    case CREATE_EXTRACT_AC3:
        if (!count7)
        {
            extract_ac3 = create_binary_path(extract_ac3, EXTRACT_AC3, SEPARATOR EXTRACT_AC3_BASENAME, globals);
            ac3dec      = create_binary_path(ac3dec, AC3DEC, SEPARATOR AC3DEC_BASENAME, globals);
            ++count7;
        }
        break;

    case CREATE_MJPEGTOOLS:
        if (!count1)
        {
            // if installed with autotools, if bindir overrides then use override, otherwise use config.h value;
            // if not installed with autotools, then use command line value or last-resort hard-code set defaults and test for result

            mp2enc   = create_binary_path(mp2enc, MP2ENC, SEPARATOR MP2ENC_BASENAME, globals);
            jpeg2yuv = create_binary_path(jpeg2yuv,JPEG2YUV, SEPARATOR JPEG2YUV_BASENAME, globals);
            mpeg2enc = create_binary_path(mpeg2enc,MPEG2ENC, SEPARATOR MPEG2ENC_BASENAME, globals);
            mplex    = create_binary_path(mplex, MPLEX, SEPARATOR MPLEX_BASENAME, globals);
            pgmtoy4m = create_binary_path(pgmtoy4m, MPLEX, SEPARATOR MPLEX_BASENAME, globals);
            ++count1;
        }
        break;

    case CREATE_SPUMUX:
        if (!count2)
        {
            spumux = create_binary_path(spumux, SPUMUX, SEPARATOR SPUMUX_BASENAME, globals);
            ++count2;
        }
        break;

    case CREATE_DVDAUTHOR:
        if (!count3)
        {
            dvdauthor=create_binary_path(dvdauthor,DVDAUTHOR, SEPARATOR DVDAUTHOR_BASENAME, globals);
            count3++;
        }
        break;

    case CREATE_IMAGEMAGICK:
        if (!count4)
        {
            mogrify=create_binary_path(mogrify, MOGRIFY, SEPARATOR MOGRIFY_BASENAME, globals);
            convert=create_binary_path(convert, CONVERT, SEPARATOR CONVERT_BASENAME, globals);
            count4++;
        }
        break;


    case CREATE_MPEG2DEC:
        if (!count5)
        {
            mpeg2dec=create_binary_path(mpeg2dec, MPEG2DEC, SEPARATOR MPEG2DEC_BASENAME, globals);
            count5++;
        }
        break;

    case CREATE_CURL:
        if (!count6)
        {
            curl=create_binary_path(curl, CURL, SEPARATOR CURL_BASENAME, globals);
            count6++;
        }
        break;

    case FREE_MEMORY:
        if (count1)
        {
            free((char*) mp2enc);
            mp2enc=NULL;
            free((char*) jpeg2yuv);
            jpeg2yuv=NULL;
            free((char*) mpeg2enc);
            mpeg2enc=NULL;
            free((char*) mplex);
            mplex=NULL;
        }
        if (count2) { free((char*) spumux); spumux = NULL; }
        if (count3) { free((char*) dvdauthor); dvdauthor = NULL; }
        if (count4)
        {
            free((char*) mogrify); mogrify = NULL;
            free((char*) convert); convert = NULL;
        }
        if (count5) { free((char*) mpeg2dec); mpeg2dec = NULL; }
        if (count6) { free((char*) curl); curl = NULL;}
        if (count7) { free((char*) ac3dec); free((char*) extract_ac3); ac3dec = NULL; extract_ac3 = NULL;}
        break;
    }
}

static char* pict;

int create_mpg(pic* img, uint16_t rank, char* mp2track, char* tempfile, globalData* globals)
{
    errno=0;
    static unsigned long s;

    if(s==0)
    {
        s = MAX(strlen(globals->settings.stillpicdir) + 26, strlen(img->backgroundpic[rank]) + 1);
        pict  = calloc(s, sizeof(char*));
    }

    // Important memory fix here
    // On SOME Unix platforms (e.g. Fedora 10 vs. Gentoo 201906 or Ubuntu 1904), not all, passing variable-size arrays to fork
    // does not work at RUNTIME. Memory should be allocated on heap.
    // buffer overflow potential issue to be checked here. Widening buffer as first unsatisfactory step. TOD: fix this.



    free(img->backgroundmpg[rank]);

    img->backgroundmpg[rank] = calloc(1 + strlen(globals->settings.tempdir) + 17 + 3 + 4 + 1, sizeof(char));

    if (img->action == STILLPICS)
    {
        if (globals->debugging) foutput("%s%u\n", INF "Creating still picture #", rank+1);

        sprintf(img->backgroundmpg[rank], "%s" SEPARATOR "%s%u%s", globals->settings.tempdir, "background_still_", rank, ".mpg");

        sprintf(pict, "%s" SEPARATOR "pic_%03u.jpg", globals->settings.stillpicdir, rank);  // here stillpic[0] is a subdir.

        if (globals->debugging)
        {
            foutput("%s%d%s\n", DBG "Created still picture path #:", rank + 1, pict);
        }
    }
    else if (img->action == ANIMATEDVIDEO)
    {
        if (globals->debugging) foutput(INF "Creating animated menu rank #%u out of %s\n", rank+1, img->backgroundpic[rank]);
        sprintf(img->backgroundmpg[rank], "%s" SEPARATOR "%s%u%s", globals->settings.tempdir, "background_movie_", rank, ".mpg");
        strcpy(pict, img->backgroundpic[rank]);
        if (img->backgroundcolors)
        {
            if (globals->veryverbose) foutput("%s\n", INF "Colorizing background jpg files prior to multiplexing...");
            char command[500];

            mogrify=create_binary_path(mogrify, MOGRIFY, SEPARATOR MOGRIFY_BASENAME, globals);

            snprintf(command, 500, "%s -fill \"rgb(%s)\" -colorize 66%% %s", mogrify, img->backgroundcolors[rank], img->backgroundpic[rank]);

            if (globals->debugging) foutput(INF "Launching mogrify to colorize menu: %d with command line %s\n", rank, command);
            if (system(win32quote(command)) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE( "System command failed")
                fflush(NULL);
        }
    }

    initialize_binary_paths(CREATE_MJPEGTOOLS, globals);

    char norm[2];
    norm[0]=img->norm[0];
    norm[1]=0;

    char *argsmp2enc[]= {MP2ENC_BASENAME, "-o", mp2track , NULL};
    char *argsjpeg2yuv[]= {JPEG2YUV_BASENAME, "-f", img->framerate, "-I", "p", "-n", "1", "-j", pict, "-A", img->aspectratio, NULL};
    char *argsmpeg2enc[]= {MPEG2ENC_BASENAME,  "-f", "8", "-n", norm,  "-o", tempfile ,"-a", img->aspect, NULL};
    const char *argsmplex[]= {MPLEX_BASENAME, "-f", "8",  "-o", img->backgroundmpg[rank], tempfile, mp2track, NULL};

    //////////////////////////

    if (img->action == ANIMATEDVIDEO )
    {
        if (globals->debugging) foutput("%s\n", INF "Running mp2enc...");

        char soundtrack[strlen(globals->settings.tempdir) + 11];
        sprintf(soundtrack, "%s"SEPARATOR"%s", globals->settings.tempdir, "soundtrack");
        if (file_exists(soundtrack)) unlink(soundtrack);
        errno = 0;
        change_directory(globals->settings.datadir, globals);
        copy_file(img->soundtrack[0][0], soundtrack, globals);
        change_directory(globals->settings.workdir, globals);

        // using freopen to redirect is safer here
#ifndef _WIN32

        int pid1;
        switch (pid1=fork())
        {
            case -1:
                foutput("%s\n", ERR "Could not launch "MP2ENC);
                break;

            case 0:


                if (NULL == freopen(soundtrack, "rb", stdin))
                {
                    perror(ERR "freopen");
                    clean_exit(EXIT_FAILURE, globals);
                }

                dup2(STDOUT_FILENO, STDERR_FILENO);

                if (errno) perror(MP2ENC);
                execv(mp2enc, (char* const*)argsmp2enc);
                foutput("%s\n", ERR "Runtime failure in mp2enc child process");
                return errno;

                break;

            default:
                waitpid(pid1, NULL, 0);
        }
#else
        const char* s=get_command_line(argsmp2enc, globals);
        uint16_t size=strlen(s);
        char cml[strlen(mp2enc)+size+3+strlen(img->soundtrack[0][0])+1+1+2];
        sprintf(cml, "%s %s < %s", mp2enc, s, win32quote(img->soundtrack[0][0]));
        foutput("%s %s\n", INF "Launching: ", cml);
        free((char *) s);
        system(win32quote(cml));
#endif
    }

#ifndef _WIN32


    sync();
    int pid2;
    char c;
    int tube[2];
    int tubeerr[2];
    int tubeerr2[2];

    // Two extra tubes are in order to redirect jpeg2yuv and mpeg2enc stdout messages and realign them with overall stdout messages, otherwise they fall out of sync
    // with one another and dvda-author messages.

    if (pipe(tube) || pipe(tubeerr) || pipe(tubeerr2))
    {
        perror(ERR "Pipe");
        return errno;
    }

    if (globals->debugging)
    {
        foutput("%s %s ...\n", INF "Running ", jpeg2yuv);
     }

     if (globals->veryverbose)
     {
        foutput("%s %s ...\n", INF "Then piping to ...", mpeg2enc);
     }

    // Owing to the piping of the stdout streams (necessary for coherence of output) existence checks must be tightened up.
    // System will freeze should an input file not exit, as mjpegtools to not always exit on system error. This may cause a loop in the piping of jpeg2yuv to mpeg2enc
    // Tight system error strategy in order here
    errno=0;

    FILE *f=fopen(pict, "rb");
    foutput("opening: %s\n", pict);
    if ((errno)||(f == NULL))
    {
        if (img->action == ANIMATEDVIDEO)
        {
            foutput(ERR "menu input files: background pic: %s", pict);
            perror("background");
        }
        else
        {
            foutput(ERR "still pic: %s", pict);
        }
        clean_exit(EXIT_FAILURE, globals);
    }
    fclose(f);
    errno=0;


//    if (mp2track)
//    {
//        FILE* f=fopen(mp2track, "rb");
//
//        if ((errno) || (f == NULL))
//        {
//            perror(ERR "menu input files: mp2 track");
//            globals->topmenu=NO_MENU;
//
//            return(errno);
//        }
//        fclose(f);
//    }


    switch (fork())
    {
        case -1:
            fprintf(stderr,"%s\n", ERR "Could not launch jpeg2yuv");
            break;

        case 0:

            close(tube[0]);
            close(tubeerr[0]);
            dup2(tube[1], STDOUT_FILENO);
            // Piping stdout is required here as STDOUT is not a possible duplicate for stdout
            dup2(tubeerr[1], STDERR_FILENO);
            execv(jpeg2yuv, (char* const*) argsjpeg2yuv);
            fprintf(stderr, "%s\n", ERR "Runtime failure in jpeg2yuv child process");
            perror("menu1");

            return errno;


        default:
            close(tube[1]);
            close(tubeerr[1]);
            dup2(tube[0], STDIN_FILENO);
            if (globals->debugging) foutput("%s\n", INF "Piping to mpeg2enc...");

            switch (pid2 = fork())
            {
            case -1:
                foutput("%s\n", ERR "Could not launch mpeg2enc");
                break;

            case 0:
                // This looks like an extra complication as it could be considered to simply use dup2(STDOUT_FILENO, stdout_FILENO) without further piping
                // However this would reverse the order of jpeg2yuv and mpeg2enc stdout messages, the latter comming first,
                // which is not desirable as jpeg2yuv is piped into mpeg2enc. Hereby we are realigning these msg streams, which even in bash piping are intermingled,
                // making it hard to read/use.
                close(tubeerr2[0]);
                close(STDOUT_FILENO);
                dup2(tubeerr2[1], STDERR_FILENO);
                // End of comment
                execv(mpeg2enc, (char* const*)argsmpeg2enc);
                foutput("%s\n", ERR "Runtime failure in mpeg2enc parent process");
                perror("menu2");
                return errno;

            default:
                waitpid(pid2, NULL, 0);
                dup2(tubeerr[0], STDIN_FILENO);

                while (read(tubeerr[0], &c, 1) == 1) foutput("%c",c);
                close(tubeerr[0]);
                close(tubeerr2[1]);
                dup2(tubeerr2[0], STDIN_FILENO);

                while (read(tubeerr2[0], &c, 1) == 1) foutput("%c",c);
                close(tubeerr2[0]);
                if (globals->debugging) foutput("%s\n", INF "Running mplex...");
                run(mplex, argsmplex, WAIT, FORK, globals);
            }
        close(tube[0]);
    }

#else

     char* mpeg2enccl = get_command_line(argsmpeg2enc, globals);
     char* jpeg2yuvcl = get_command_line(argsjpeg2yuv, globals);

// This is unsatisfactory yet will do for porting purposes.

    const char* mplexcl=get_command_line(argsmplex, globals);

    char cml2[strlen(jpeg2yuv)+1+strlen(jpeg2yuvcl)+3+strlen(mpeg2enc)+1+strlen(mpeg2enccl)+1];

    sprintf(cml2, "%s %s | %s %s",jpeg2yuv, jpeg2yuvcl,mpeg2enc, mpeg2enccl);

    system(win32quote(cml2));

    char cml3[strlen(mplex)+1+strlen(mplexcl)+1];

    sprintf(cml3, "%s %s",mplex, mplexcl);
    system(win32quote(cml3));

    free((char*) jpeg2yuvcl);
    free((char*) mpeg2enccl);
    free((char*) mplexcl);
#endif

    return errno;
}



int generate_background_mpg(pic* img, globalData* globals)
{
    uint16_t rank=0;
    char tempfile[CHAR_BUFSIZ*10];
    char* mp2track;
    errno=0;

    if (strcmp(img->norm, "ntsc") == 0) norm_y=NTSC_Y;  //   x  value is the same as for PAL (720)
    memset(tempfile, '0', sizeof(tempfile));
    sprintf(tempfile, "%s"SEPARATOR"%s", globals->settings.tempdir, "temp.m2v");

    mp2track=(img->action == ANIMATEDVIDEO)? calloc(CHAR_BUFSIZ, sizeof(char)) : NULL;
    if (mp2track)
        sprintf(mp2track, "%s"SEPARATOR"%s", globals->settings.tempdir, "mp2track.mp2");

    if (img->backgroundmpg == NULL) foutput("%s", MSG_TAG "backgroundmpg will be allocated.\n");

    if (globals->debugging) foutput(INF "Launching mjpegtools to create background mpg with nmenus=%d\n", img->nmenus);

    /* now authoring AUDIO_TS.VOB */
    rank=0;

    if (img->action == ANIMATEDVIDEO)
    {
        FREE(img->backgroundmpg);
        img->backgroundmpg=calloc(img->nmenus, sizeof(char*));

        while(rank < img->nmenus)
        {
            create_mpg(img, rank, mp2track, tempfile, globals);
            fflush(NULL);
            rank++;
        }
        globals->backgroundmpgsize = img->nmenus;
    }
    rank=0;

    if (img->action == STILLPICS)
    {
        FREE(img->backgroundmpg);
        img->backgroundmpg=calloc(img->count, sizeof(char*));
        globals->backgroundmpgsize = img->count;
        if (img->backgroundmpg)
            while (rank < img->count)
            {
                create_mpg(img, rank, mp2track, tempfile, globals);
                img->stillpicvobsize[rank]=(uint32_t) (stat_file_size(img->backgroundmpg[rank])/0x800);
                if (img->stillpicvobsize[rank] > 1024) foutput("%s",WAR "Size of slideshow in excess of the 2MB track limit... some stillpics may not be displayed.\n");
                if (rank) cat_file(img->backgroundmpg[rank], img->backgroundmpg[0], globals);
                ++rank;
            }
        // The first backgroundmpg file is the one that is used to create AUDIO_SV.VOB in amg2.c
    }

    FREE(mp2track)

    FREE(pict)

    if ((globals->debugging) && (!errno))
        foutput("%s\n", INF "MPG background authoring OK.");
    return errno;

}


int launch_spumux(pic* img, globalData* globals)
{
    // hush up spumux on stdout if non-verbose mode selected

    //sprintf(spumuxcommand, "%s%s%s%s%s%s%s", "spumux -v 0 ", globals->spu_xml, " < ", img->backgroundmpg, (globals->debugging)? "" : " 2>null ", " 1> ", img->topmenu);

    if (globals->debugging) foutput("%s\n", INF "Launching spumux to create buttons");
    int menu=0;


    initialize_binary_paths(CREATE_SPUMUX, globals);


    while (menu < img->nmenus)
    {
        if (globals->debugging) foutput(INF "Creating menu %d from Xml file %s\n",menu+1, globals->spu_xml[menu]);
        const char *argsspumux[]= {SPUMUX_BASENAME, "-v", "2", globals->spu_xml[menu], NULL};

        // This is to hush up dvdauthor's stdout messages, which interfere out of sequential order with main application stdout messages
        // and anyway could not be logged by  -l;
        // with normal verbosity, stdout messages end up in a tube's dead end, otherwise they are retrieved at the other end on stdout.
        errno=0;
#ifndef __WIN32__

        int firsttubeerr[2];
        if (pipe(firsttubeerr) == -1)
            perror(ERR "Pipe issue with spumux (firsttubeerr[2])");
        char c;


        switch (fork())
        {

        case -1:
            foutput("%s\n", ERR "Could not launch spumux");
            break;

        case 0:
            close(firsttubeerr[0]);
            dup2(firsttubeerr[1], STDERR_FILENO);

           fprintf(stderr, INF "command line: %s %s %s %s %s", spumux, argsspumux[1], argsspumux[2], argsspumux[3], argsspumux[4]);
            if (freopen(img->backgroundmpg[menu], "rb", stdin) == NULL)
            {
                fprintf(stderr, "%s",ERR "freopen (stdin)\n");
                fprintf(stderr, "img->backgroundmpg[%d]=%s errno=%d: %s", menu, img->backgroundmpg[menu], errno, strerror(errno));
                return errno;
            }
            if (freopen(img->topmenu[menu], "wb", stdout) == NULL)
            {
                fprintf(stderr, "%s\n", ERR "freopen (stdout)");
                fprintf(stderr, "img->backgroundmpg[%d]=%s errno=%d: %s", menu, img->topmenu[menu], errno, strerror(errno));
                return errno;
            }

            execv(spumux, (char* const*) argsspumux);
            return errno;


        default:

            close(firsttubeerr[1]);
            dup2(firsttubeerr[0], STDIN_FILENO);
            wait(NULL);

            while (read(firsttubeerr[0], &c, 1) == 1) foutput("%c",c);

            if (errno)
            {
                foutput("%s\n", ERR "Runtime failure in spumux child process");
                perror(ERR "spumux");
                return errno;
            }
            close(firsttubeerr[0]);
        }

#else

        char* s=get_command_line(argsspumux, globals);
        uint16_t size=strlen(s);
        char cml[strlen(spumux)+1+size+3+strlen(img->backgroundmpg[menu])+2+3+strlen(img->topmenu[menu])+2+1];
        sprintf(cml, "%s %s < %s > %s",spumux, s, win32quote(img->backgroundmpg[menu]), win32quote(img->topmenu[menu]));
        system(win32quote(cml));
        free((char*) s);

#endif



        menu++;
    }


    return errno;
}



int launch_dvdauthor(globalData* globals)
{

    initialize_binary_paths(2, globals);

    errno=0;

    if (globals->debugging) foutput("%s\n", INF "Launching dvdauthor to add virtual machine commands to top menu");

    const char* args[]= {dvdauthor, "-o", globals->settings.outdir, "-x", globals->xml, NULL};

    run(dvdauthor, (const char** )args, WAIT, FORK, globals);

#ifndef _WIN32
    sync();
#endif

    return errno;
}



uint16_t x(uint8_t group, uint8_t ngroups)
{
    return  Min(norm_x, (20 + ((norm_x - 20) * group) / ngroups + EMPIRICAL_X_SHIFT));
}

// text is within button (i,j) with left-justified spacing of 10 pixels wrt left border
uint16_t y(uint8_t track, uint8_t maxnumtracks)
{

    int labelheight=(norm_y - 56 - 40 - maxnumtracks * 12) / maxnumtracks;
    int y_top = 56 + track * (labelheight + 12)+ labelheight / 2 ;

    return y_top;

}


int prepare_overlay_img(char* text, int8_t group, pic *img, char* command, char* command2, int menu, char* albumcolor, globalData* globals)
{
    initialize_binary_paths(3, globals);

    int size=strlen(globals->settings.tempdir)+11;
    char picture_save[size];
    sprintf(picture_save, "%s"SEPARATOR"%s", globals->settings.tempdir, "svpic.png");

    if (file_exists(picture_save)) unlink(picture_save);
    errno=0;
    change_directory(globals->settings.datadir, globals);
    if (img->blankscreen)
        copy_file(img->blankscreen, picture_save, globals);
    change_directory(globals->settings.workdir, globals);

    if ((group == -1)&&(text))  // album text
    {
        uint16_t x0= EVEN(x( (group > 0 ? group : 0), img->ncolumns)) ;
        char * q = quote(picture_save);
        snprintf(command, 2*CHAR_BUFSIZ, "%s %s %s \"rgb(%s)\" %s %s %s %d %s %s %d%c%d %c%s%s %s", mogrify,
                 "+antialias", "-fill", albumcolor, "-font", img->textfont, "-pointsize", DEFAULT_POINTSIZE,
                 "-draw", " \"text ", x0, ',' , ALBUM_TEXT_Y0,  '\'', text, "\'\"", q);
        free(q);
        if (globals->debugging) foutput("%s%s\n", INF "Launching mogrify (title) with command line: ", command);
        if (system(win32quote(command)) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE( "System command failed")
            fflush(NULL);
    }

    if ((img->imagepic[menu]==NULL) || (img->highlightpic[menu]==NULL) || (img->imagepic[menu]==NULL))
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE( "pic pathnames");
        return -1;
    }


    copy_file(picture_save, img->imagepic[menu], globals);
    if (globals->debugging) foutput(INF "copying %s to %s for menu #%d\n", picture_save, img->imagepic[menu], menu);
    copy_file(picture_save, img->highlightpic[menu], globals);
    copy_file(picture_save, img->selectpic[menu], globals);
    errno=0;
    snprintf(command, CHAR_BUFSIZ, "%s %s", mogrify, "+antialias");
    snprintf(command2, CHAR_BUFSIZ, "%s %s", mogrify, "+antialias");


    return errno;
}


int mogrify_img(char* text, int8_t group, int8_t track, pic *img, uint8_t maxnumtracks, char* command, char* command2,  int8_t offset, char* textcolor)
{
    errno=0;
    uint16_t x0, y0;

    x0=EVEN(x( (group>0)?group:0, img->ncolumns)) ;
    y0=EVEN(y(track+1-offset, maxnumtracks+4));

// In automatic mode, we underline presupposing -font Courier with approx 1 letter of font 10 =6 pix in width, otherwise sepcify fontwidth

    char *str, *str2;
    str=(char*) calloc(10*CHAR_BUFSIZ, 1);
    if (str == NULL) perror(ERR "mogrify, string");
    str2=(char*) calloc(10*CHAR_BUFSIZ, 1);
    if (str2 == NULL) perror(ERR "mogrify, string 2");

// +antialias is crucial for dvdauthor, otherwise button masks will not be properly detected.
    int16_t deltax0=0,deltax1=0,deltay0=0,deltay1=0;

    if (img->highlightformat == UNDERLINE)
    {
        deltax0=0;
        deltay0=(img->pointsize < 12)? 2 : 4;
        deltax1=EVEN((img->fontwidth * img->pointsize *strlen(text))/10);
        deltay1=deltay0+2;
    }
    else if (img->highlightformat == PRECEDE)
    {
        deltax0=-12;
        deltay0=-8;
        deltax1=-4;
        deltay1=0;
    }
    if (img->highlightformat == BUTTON)
    {

        deltax0=-4;
        deltay0=-4-EVEN((img->fontwidth*img->pointsize)/5);
        deltax1=EVEN((img->fontwidth * img->pointsize *strlen(text))/10)+4;
        deltay1=4;
    }


    if (track!=-1)
    {
        char *q = quote(img->highlightcolor_pic);
        snprintf(str, 10*CHAR_BUFSIZ, " %s \"rgb(%s)\" %s %s %d%s%d %d%s%d%s ",
                 "-fill", q, "-draw", " \"rectangle ", x0+deltax0, ",", y0+deltay0,  x0+ deltax1, ",", y0+deltay1, "\"");   // conversion works badly with -colors < 4
        free(q);
    }

    strcat(command, str);
    snprintf(str2, 10*CHAR_BUFSIZ, " %s \"rgb(%s)\" %s %s %s %d %s %s %d%c%d %s%s%s ",
             "-fill", textcolor, "-font", img->textfont, "-pointsize", (int) floor(img->pointsize*(1 -(track == -1)*0.2)),
             "-draw", " \"text ", x0, ',' , y0, "\'", text, "\'\"");

    strcat(command2, str2);

    if (img->highlightformat == 1) strcat(command, str2); // because text will be overlayed. A more efficient method would be to use convert -composite for many tracks. For just a frew tracks, this is moot.


    if (errno) perror(ERR "mogrify");
    FREE(str)
    FREE(str2)
    return errno;
}


void compute_pointsize(pic* img, uint16_t maxtracklength, uint8_t maxnumtracks, globalData* globals)
{
    if (img->pointsize == 0)
    {
        uint8_t wide= (((norm_x-4*EMPIRICAL_X_SHIFT-2*20)-(img->ncolumns-1)*20)*10)/(img->ncolumns*img->fontwidth*maxtracklength);
        uint8_t delta=(y(1,maxnumtracks)-y(0,maxnumtracks))*3/4;
        uint8_t height=(uint8_t) ((delta*5)/img->fontwidth);

        img->pointsize=Min(wide, height);
    }
    img->pointsize=MAX(MIN_POINTSIZE, img->pointsize);
    img->pointsize=Min(img->pointsize, MAX_POINTSIZE);
}

/* The following function tests presence of characters with pixels likely to intersect underlining motifs thereby causing spumux to crash
   and to avoid this switches ----highlightformat to -1 (little squares) */

  void test_underline(char* text,pic* img, globalData* globals)
{

    int j, s=strlen(text);

    for (j=0; j < s; j++)
        if ((text[j]== 'g') || (text[j]== 'j') || (text[j]== 'p') || (text[j]== 'q') || (text[j]== 'y'))
        {
            if (globals->debugging)
                foutput(INF "Switching to little squares rather than underlining motifs for highlight\n       as %c could cut underlines\n", text[j]);
            img->highlightformat=-1;
        }

}


int generate_menu_pics(command_t* command, pic* img, uint8_t ngroups, uint8_t *ntracks,  globalData* globals)
{
    if ((!img->refresh)||(!img->nmenus))   return 0;
    errno=0;
    FILE* f;
    uint8_t group=0, track=0, buttons=0, menu=0, arrowbuttons=1, groupcount=0, menubuttons;
    uint16_t maxtracklength=0;
    int dim=0, k, j;
    char** grouparray=NULL, **basemotif=NULL, *albumtext=NULL, ***tracktext=NULL, ***grouptext=NULL;

    if (!img->hierarchical)
    {
        img->maxbuttons=Min(MAX_BUTTON_Y_NUMBER-2, totntracks) / img->nmenus;
        img->resbuttons=Min(MAX_BUTTON_Y_NUMBER-2, totntracks) % img->nmenus;
    }

    if (img->screentextchain)
    {
        uint32_t size;

        size=(uint16_t) ((norm_x - 40 - 20 * (img->ncolumns - 1)) / img->ncolumns);

        // to avoid using reentrant version of strtok (strtok_r, not mingw32 protable)

        char remainder[strlen(img->screentextchain)];

        basemotif=fn_strtok(img->screentextchain, '=', basemotif, &size, 1, cutloop, remainder, globals) ;
        albumtext=basemotif[0];

        grouparray=fn_strtok(remainder, ':', grouparray, &dim, 0, NULL, NULL, globals) ;

        tracktext=calloc(dim, sizeof(char**));
        if (tracktext == NULL) perror(ERR "Track text allocation");
        grouptext=calloc(dim, sizeof(char**));
        if (grouptext == NULL) perror(ERR "Group text allocation");

        for (k=0; k < dim; k++)
        {
            char rem[strlen(grouparray[k])];
            grouptext[k]=fn_strtok(grouparray[k], '=', grouptext[k], &globals->grouptextsize[k], 1, cutloop, rem, globals);
            tracktext[k]=fn_strtok(rem, ',', tracktext[k], &globals->tracktextsize[k], 0,NULL, NULL, globals);
            free(grouparray[k]);
        }

        free(grouparray);

        do
        {
            if (img->hierarchical) test_underline(grouptext[group][0],img, globals);
            do
            {
                maxtracklength=MAX(maxtracklength,strlen(tracktext[group][track]));
                if (strlen(tracktext[group][track]) > size) tracktext[group][track][size]='\0';
                test_underline(tracktext[group][track],img, globals);
                track++;

            }
            while (track < ntracks[group]);
            group++;
            track=0;

        }
        while (group < Min(img->ncolumns*img->nmenus,ngroups));
    }
    else
    {
        albumtext=strdup(DEFAULT_ALBUM_HEADER);
        grouptext=(char ***)calloc(ngroups, sizeof(char**));
        tracktext=(char ***)calloc(ngroups, sizeof(char**));
        dim=ngroups;
        globals->grouptextsize = calloc(dim, sizeof(int));
        globals->tracktextsize = calloc(dim, sizeof(int));
        for (k=0; k < dim; k++)
        {
            grouptext[k]=calloc(2, sizeof(char**));

            globals->grouptextsize[k] = 2;
            grouptext[k][0]=calloc(strlen(DEFAULT_GROUP_HEADER_UPPERCASE)+2, sizeof(char));
            sprintf(grouptext[k][0], "%s%d", DEFAULT_GROUP_HEADER_UPPERCASE, k+1);
            tracktext[k]=calloc(ntracks[k]+1, sizeof(char**));
            globals->tracktextsize[k] = ntracks[k]+1;
            for (j=0; j < ntracks[k]; j++)
            {
                tracktext[k][j]=calloc(strlen(DEFAULT_TRACK_HEADER)+3, sizeof(char));
                sprintf(tracktext[k][j], "%s%d", DEFAULT_TRACK_HEADER, j+1);
            }
            tracktext[k][ntracks[k]]=NULL;
            grouptext[k][1]=NULL;
        }


    }

    track=group=0;
    int8_t offset=0;

    do
    {

        if ((f=fopen(img->imagepic[menu],"rb")) == NULL) img->refresh=1;
        else fclose(f);
        if ((f=fopen(img->highlightpic[menu],"rb")) == NULL) img->refresh=1;
        else fclose(f);
        if ((f=fopen(img->selectpic[menu],"rb")) == NULL) img->refresh=1;
        else fclose(f);

        char* command1=calloc(50*CHAR_BUFSIZ,1);
        char* command2=calloc(50*CHAR_BUFSIZ,1);

        char picture_save[CHAR_BUFSIZ+14];
        sprintf(picture_save, "%s/%s%d", globals->settings.tempdir, "svpic",menu);

        if (globals->debugging)  foutput("%s\n", INF "Authoring top menu streams...");

        if (img->hierarchical)
        {
            img->maxbuttons=(menu == 0)? ngroups : Min(MAX_BUTTON_Y_NUMBER-2,ntracks[groupcount]);
            img->resbuttons=0;
        }


        arrowbuttons=(menu < img->nmenus-1)+(menu > 0);
        menubuttons=(menu < img->nmenus-1)? img->maxbuttons : img->maxbuttons+img->resbuttons;

        buttons=0;


        compute_pointsize(img, 10, command->maxntracks, globals);

        prepare_overlay_img(albumtext, -1, img, command1, command2, menu, img->albumcolor, globals);
        //free(albumtext);  // segfault here under linux for unknown reasons. TO: fix it.

        if (img->hierarchical)
        {
            if (menu == 0)
            {
                do
                {


                    /* Vicious issue here: use DEFAULT_GROUP_HEADER such that the underline for highlighting does not cut a letter.
                                   With lower-case "group", this happens as the underline cuts the 'p'. Two ways out: underline lower or use another label/use uppercase
                                   Note: This issue was tested to cause spumux crash */
                    mogrify_img(grouptext[groupcount][0], 0, groupcount, img, command->maxntracks, command1, command2, 0, img->textcolor_pic);
                    groupcount++;
                    buttons++;

                }
                while (groupcount < ngroups);
                groupcount=0;
            }

            else if (groupcount < ngroups)
            {
                mogrify_img(grouptext[groupcount][0], 0, -1, img, command->maxntracks, command1, command2, 0, img->groupcolor);
                offset=track;

                do
                {
                    buttons++;
                    mogrify_img(tracktext[groupcount][track], 0, track, img, command->maxntracks, command1, command2, offset, img->textcolor_pic);
                    track++;
                }
                while ((buttons < menubuttons) && (track < ntracks[groupcount]));


                if (track == ntracks[groupcount])
                {
                    groupcount++;
                    track=0;
                    offset=0;
                }
            }
        }
        else
        {
            do
            {

                mogrify_img(grouptext[groupcount][0], group, -1, img, command->maxntracks, command1, command2, 0, img->groupcolor);
                offset=track;

                do
                {

                    buttons++;
                    mogrify_img(tracktext[groupcount][track], group, track, img, command->maxntracks, command1, command2, offset, img->textcolor_pic);
                    track++;
                }
                while ((buttons < menubuttons) && (track < ntracks[groupcount]));


                if (track == ntracks[groupcount])
                {
                    group++;
                    groupcount++;
                    track=0;
                    offset=0;
                }
                else
                    break;  // changing menus without completing the liste of tracks in the same group
            }
            while ((group < img->ncolumns)&& (groupcount < ngroups));
        }


        if ((img->nmenus > 1) &&(menu < img->nmenus))
            do
            {
                char arrowstring[9]= {0};
                strcpy(arrowstring, (menu == img->nmenus-1)? DEFAULT_PREVIOUS:DEFAULT_NEXT);
                buttons++;
                mogrify_img(arrowstring, img->ncolumns-1, command->maxntracks, img, command->maxntracks, command1, command2, offset, img->arrowcolor);
                if ((menu) && (menu < img->nmenus-1))
                {
                    buttons++;
                    mogrify_img(DEFAULT_PREVIOUS, img->ncolumns-1, command->maxntracks+1, img, command->maxntracks, command1, command2, offset, img->arrowcolor);
                }
            }
            while  (buttons < menubuttons+arrowbuttons);

        char *q = quote(img->imagepic[menu]);
        strcat(command2, q);
        free(q);
        if (globals->veryverbose) foutput(INF "Menu: %d/%d, groupcount: %d/%d.\n       Launching mogrify (image) with command line: %s\n", menu, img->nmenus, groupcount, ngroups, command2);
        if (system(win32quote(command2)) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE( "System command failed");
        free(command2);
        command2 = NULL;
        copy_file(img->imagepic[menu], img->highlightpic[menu], globals);
        q= quote(img->highlightpic[menu]);
        strcat(command1, q);
        free(q);
        if (globals->veryverbose) foutput(INF "Menu: %d/%d, groupcount: %d/%d.\n       Launching mogrify (highlight) with command line: %s\n", menu, img->nmenus, groupcount, ngroups,command1);
        if (system(win32quote(command1)) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE( "System command failed");
        free(command1);
        command1 = NULL;
        char command3[500];
        q  = quote(img->selectfgcolor_pic);
        char* q2 = quote(img->textcolor_pic);
        char* q3 = quote(img->imagepic[menu]);
        char* q4 = quote(img->selectpic[menu]);
        snprintf(command3, sizeof(command3), "%s %s \"rgb(%s)\"  %s \"rgb(%s)\" %s %s", convert, "-fill", q, "-opaque", q2, q3, q4);
        free(q);
        free(q2);
        free(q3);
        free(q4);
        if (globals->veryverbose) foutput(INF "Menu: %d/%d, groupcount: %d/%d.\n       Launching convert (select) with command line: %s\n",menu, img->nmenus, groupcount, ngroups,command3);
        if (system(win32quote(command3)) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE( "System command failed");

        menu++;
        group=0;


    }
    while ((menu < img->nmenus)&& (groupcount < dim));

    for(group=0; group < dim; ++group)
    {
        for (uint32_t i = 0; i < globals->grouptextsize[group]; ++i)
            free(grouptext[group][i]);
        for (uint32_t i = 0; i < globals->tracktextsize[group]; ++i)
            free(tracktext[group][i]);
        free(grouptext[group]);
        free(tracktext[group]);
    }

    free(grouptext);
    free(tracktext);

    if (img->screentextchain)
    {
        free(basemotif[0]);
        free(basemotif[1]);
        free(basemotif);
    }

    if (globals->debugging)
        if (!errno)
            foutput("%s\n", MSG_TAG "Top menu pictures were authored.");

    return errno;
}


int create_stillpic_directory(char* string, int32_t count, globalData* globals)
{
    if (!string)
    {
        fprintf(stderr, ERR "Null string input for stillpic in create_stillpic_directory, with count=%d\n", count);
        exit(-1);
    }

    static int32_t  k;
    change_directory(globals->settings.stillpicdir, globals);
    if (k == count)
    {
        if (globals->debugging) foutput(WAR "Too many pics, only %d sound track%s skipping others...\n", count, (count == 1)? "," : "s,");

        change_directory(globals->settings.workdir, globals);
        return 0;
    }

    if (*string == '\0')
    {
        if (globals->debugging) foutput(INF "Jumping one track for picture rank = %d\n", k);

        change_directory(globals->settings.workdir, globals);
        return 1;
    }

#ifndef __WIN32__
 struct stat buf;

    if (stat(string, &buf) == -1)
    {
        fprintf(stderr, ERR "create_stillpic_directory: could not stat file %s\n", string);
        exit(-1);
    }
    if (S_IFDIR & buf.st_mode)
    {
        if (globals->debugging) foutput(INF "Directory %s will be parsed for still pics\n", string);
        globals->settings.stillpicdir = strdup(string);

        change_directory(globals->settings.workdir, globals);
        return 0;
    }
    if (S_IFREG & buf.st_mode)
    {
  #endif
        char dest[strlen(globals->settings.tempdir) + 13];
        sprintf(dest, "%s"SEPARATOR"pic_%03d.jpg", globals->settings.tempdir, k);
        if (globals->debugging) fprintf(stderr, DBG "Picture %s will be copied to temporary directory as %s.\n", string, dest);

        copy_file(string, dest, globals);

        ++k;

        change_directory(globals->settings.workdir, globals);
        return 1;
#ifndef __WIN32__
    }
#endif

    change_directory(globals->settings.workdir, globals);
    return 0;

}
#endif














