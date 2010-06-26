#if HAVE_CONFIG_H && !defined __CB__
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef __WIN32__
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include "structures.h"
#include "c_utils.h"
#include "commonvars.h"
#include "menu.h"
#include "launch_manager.h"
#include "winport.h"
#include "auxiliary.h"
#include "amg.h"


extern globalData globals;

// Automated top-menu generation using patched dvdauthor
// We authorize only maximal resolution form input pics (ie: 720x576, pal/secam or 720x480, ntsc)

uint16_t norm_x=PAL_X, norm_y=PAL_Y;  // TODO: adjust for ntsc #define NTSC_Y 480
extern uint16_t totntracks;
uint8_t maxbuttons, resbuttons;


/* patches AUDIO_TS.VOB into an active-menu type AUDIO_SV.VOB at minor processing cost */


void create_activemenu(pic* img,uint16_t totntracks)
{


if (img->tsvob == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  No matrix AUDIO_TS.VOB available for generating active menus.")

uint8_t j;
uint64_t i;
uint32_t pathlength=strlen(globals.settings.tempdir);

char activeheader[pathlength+14];
sprintf(activeheader, "%s"SEPARATOR"activeheader", globals.settings.tempdir);
copy_file(img->activeheader, activeheader);

uint16_t activeheadersize=0;
activeheadersize=stat_file_size(activeheader);
FILE* activeheaderfile=fopen(activeheader, "rb");

/* processing */

puts("[INF]  Using already created top menus.\n");
uint64_t tsvobsize=0;
tsvobsize=stat_file_size(img->tsvob);
if (tsvobsize <= activeheadersize) {perror("[ERR]  AUDIO_TS.VOB is too small.\n"); prd(tsvobsize) prd(activeheadersize) exit(EXIT_FAILURE) ;}
uint8_t tsvobpt[tsvobsize];
memset(tsvobpt, 0, tsvobsize);
FILE * tsvobfile=fopen(img->tsvob, "rb");

fread(tsvobpt, activeheadersize, 1, activeheaderfile);

if (0 == fseek(tsvobfile, ACTIVEHEADER_INSERTOFFSET, SEEK_SET)) EXIT_ON_RUNTIME_ERROR_VERBOSE("fseek1")


fread(tsvobpt+activeheadersize+32, 0x314-ACTIVEHEADER_INSERTOFFSET, 1, tsvobfile);

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
while (i < 0x800) { tsvobpt[i]=0xFF; i++; }
if (0 == fseek(tsvobfile, 0x800, SEEK_SET)) EXIT_ON_RUNTIME_ERROR_VERBOSE("fseek2");
fread(tsvobpt+0x800, tsvobsize-0x800, 1, tsvobfile);


/* writing */
if (img->stillvob == NULL)
{
   if (img->stillvob == NULL) img->stillvob=strdup(img->tsvob);
   if (img->stillvob)
   {
      img->stillvob[strlen(img->stillvob)-5]='V';
      img->stillvob[strlen(img->stillvob)-6]='S';
   }
   else { perror("[ERR]   stillvob string allocation.\n"); return;}
}


FILE* svvobfile=fopen(img->stillvob, "wb");
for (j=0; j < totntracks; j++)
  fwrite(tsvobpt, tsvobsize, 1, svvobfile);

fclose(svvobfile);
if (globals.topmenu == ACTIVE_MENU_ONLY) unlink(img->tsvob);
return;
}



int create_mpg(pic* img, uint16_t rank, char* mp2track, char* tempfile, uint8_t ngroups, uint8_t* ntracks)
{
    char pic[CHAR_BUFSIZ*10];

    errno=0;


    img->backgroundmpg[rank]=(char*)calloc(26+strlen(globals.settings.tempdir), sizeof(char));
    if (img->backgroundmpg[rank] == NULL) perror("[ERR]  background mpg allocation\n");

    if (img->action == STILLPICS)
    {
        if (globals.debugging) printf("%s%u\n", "[INF]  Creating still picture #", rank+1);
        sprintf(img->backgroundmpg[rank], "%s"SEPARATOR"%s%u%s", globals.settings.tempdir, "background_still_", rank, ".mpg");
        snprintf(pic, sizeof(pic), "%s/pic_%03u.jpg", globals.settings.stillpicdir, rank);  // here stillpic[0] is a subdir.
    }
    else if (img->action == ANIMATEDVIDEO)
    {
        sprintf(img->backgroundmpg[rank], "%s"SEPARATOR"%s%u%s", globals.settings.tempdir, "background_movie_",rank, ".mpg");
        snprintf(pic, sizeof(pic), "%s", img->backgroundpic[rank]);
        if (globals.debugging) printf("[INF]  Creating animated menu #%u out of %s\n", rank+1, img->backgroundpic[rank]);
    }

    /////   command lines /////

    char* mp2enc=NULL;
    char* jpeg2yuv=NULL;
    char* mpeg2enc=NULL;
    char* mplex=NULL;

        // if installed with autotools, if bindir overrides then use override, otherwise use config.h value;
       // if not installed with autotools, then use command line value or last-resort hard-code set defaults and test for result

    mp2enc=create_binary_path(mp2enc, MP2ENC, SEPARATOR MP2ENC_BASENAME);
    jpeg2yuv=create_binary_path(jpeg2yuv,JPEG2YUV, SEPARATOR JPEG2YUV_BASENAME);
    mpeg2enc=create_binary_path(mpeg2enc,MPEG2ENC, SEPARATOR MPEG2ENC_BASENAME);
    mplex=create_binary_path(mplex, MPLEX, SEPARATOR MPLEX_BASENAME);


    char *argsmp2enc[]= {MP2ENC, "-o", mp2track , NULL};
    char *argsjpeg2yuv[]= {JPEG2YUV, "-f", img->framerate, "-I", "p", "-n", "1", "-j", pic, "-A", img->aspectratio, NULL};
    char *argsmpeg2enc[]= {MPEG2ENC,  "-f", "8", "-n", img->norm,  "-o", tempfile ,"-a", img->aspect, NULL};
    char *argsmplex[]= {MPLEX, "-f", "8",  "-o", img->backgroundmpg[rank], tempfile, mp2track, NULL};
    //////////////////////////

    if (img->action==ANIMATEDVIDEO )
    {
        if (globals.debugging) printf("%s\n", "[INF]  Running mp2enc...");


        char soundtrack[strlen(globals.settings.tempdir)+11];
        sprintf(soundtrack, "%s"SEPARATOR"%s", globals.settings.tempdir, "soundtrack");
        unlink(soundtrack);
        errno=0;
        copy_file(img->soundtrack, soundtrack);


        // using freopen to redirect is safer here
#ifndef __WIN32__


        int pid1;
        switch (pid1=fork())
        {
        case -1:
            printf("%s\n", "[ERR]  Could not launch "MP2ENC);
            break;

        case 0:


            freopen(soundtrack, "rb", stdin);
            dup2(STDOUT_FILENO, STDERR_FILENO);

            if (errno) perror(MP2ENC);
            execv(mp2enc, argsmp2enc);
            printf("%s\n", "[ERR]  Runtime failure in mp2enc child process");
            return errno;

            break;

        default:
            waitpid(pid1, NULL, 0);
        }
#else
        char* s=get_command_line(argsmp2enc);
        uint16_t size=strlen(s);
        char cml[strlen(mp2enc)+1+size+3+strlen(img->soundtrack)+1];
        sprintf(cml, "%s %s < %s", mp2enc, s, img->soundtrack);
        free(s);
        system(cml);
#endif
    }

#ifndef __WIN32__


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
        perror("[ERR]  Pipe");
        return errno;
    }

    if (globals.debugging)
    printf("%s\n", "[INF]  Running jpeg2yuv...");

    // Owing to the piping of the stdout streams (necessary for coherence of output) existence checks must be tightened up.
    // System will freeze should an input file not exit, as mjpegtools to not always exit on system error. This may cause a loop in the piping of jpeg2yuv to mpeg2enc
    // Tight system error strategy in order here
    errno=0;
    if (img->action == ANIMATEDVIDEO)
    {
        FILE *f=fopen(pic, "rb");
        printf("opening: %s\n", pic);
        if ((errno)||(f == NULL))
        {
            printf("[ERR]  menu input files: background pic: %s", pic);
            globals.topmenu=NO_MENU;

            return(errno);
        }
        fclose(f);
        errno=0;
    }

//    if (mp2track)
//    {
//        FILE* f=fopen(mp2track, "rb");
//
//        if ((errno) || (f == NULL))
//        {
//            perror("[ERR]  menu input files: mp2 track");
//            globals.topmenu=NO_MENU;
//
//            return(errno);
//        }
//        fclose(f);
//    }


    switch (fork())
    {
    case -1:
        fprintf(stdout,"%s\n", "[ERR]  Could not launch jpeg2yuv");
        break;

    case 0:

        close(tube[0]);
        close(tubeerr[0]);
        dup2(tube[1], STDOUT_FILENO);
        // Piping stdout is required here as STDOUT is not a possible duplicate for stdout
        dup2(tubeerr[1], STDERR_FILENO);
        execv(jpeg2yuv, argsjpeg2yuv);
        printf("%s\n", "[ERR]  Runtime failure in jpeg2yuv child process");
        perror("menu1");

        return errno;


    default:
        close(tube[1]);
        close(tubeerr[1]);
        dup2(tube[0], STDIN_FILENO);
        if (globals.debugging) printf("%s\n", "[INF]  Piping to mpeg2enc...");

        switch (pid2 = fork())
        {
        case -1:
            printf("%s\n", "[ERR]  Could not launch mpeg2enc");
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
            execv(mpeg2enc, argsmpeg2enc);
            printf("%s\n", "[ERR]  Runtime failure in mpeg2enc parent process");
            perror("menu2");
            return errno;

        default:
            waitpid(pid2, NULL, 0);
            dup2(tubeerr[0], STDIN_FILENO);

            while (read(tubeerr[0], &c, 1) == 1) putchar(c);
            close(tubeerr[0]);
            close(tubeerr2[1]);
            dup2(tubeerr2[0], STDIN_FILENO);

            while (read(tubeerr2[0], &c, 1) == 1) putchar(c);
            close(tubeerr2[0]);
            if (globals.debugging) printf("%s\n", "[INF]  Running mplex...");
            run(mplex, argsmplex, 0);
        }
        close(tube[0]);
    }

#else


// This is unsatisfactory yet will do for porting purposes.

    char* jpegcl;
    jpegcl=get_command_line(argsjpeg2yuv);
    char* mpegcl=get_command_line(argsmpeg2enc);
    char* mplexcl=get_command_line(argsmplex);
    uint16_t sizejpegcl=strlen(jpegcl);
    uint16_t sizempegcl=strlen(mpegcl);
    uint16_t sizemplexcl=strlen(mplexcl);
    char cml2[strlen(mpeg2enc)+1+sizejpegcl+3+sizempegcl+1];
    char cml3[strlen(mplex)+1+sizemplexcl];
    sprintf(cml2, "%s %s | %s %s", jpeg2yuv, jpegcl, mpeg2enc, mpegcl);
    sprintf(cml3, "%s %s",mplex, mplexcl);
    system(cml2);
    system(cml3);
    free(jpegcl);
    free(mpegcl);
    free(mplexcl);
#endif


free(mp2enc);
free(jpeg2yuv);
free(mpeg2enc);
free(mplex);


 return errno;



}


ALWAYS_INLINE_GCC char** readjust_pic_arrays(char** picarray, int nmenus, uint8_t counterposition)
{

    int L=arraylength((void**)picarray), k=L;

    /* Should there be many menus, it is necessary to ensure that background pics come in the same number
     * as the number of menus and if not to duplicate the last background */

    if (nmenus > L)
    {
        picarray=(char**) realloc(picarray,nmenus*sizeof(char*));
    }

    if (picarray== NULL) EXIT_ON_RUNTIME_ERROR

        while(nmenus > k)
        {
            picarray[k]=strdup(picarray[k-1]);
            if (counterposition) picarray[k][strlen(picarray[k])-counterposition]='0'+k; // adds a counter to filepaths
            k++ ;
        }

    return picarray;
    // It is necessary to return, as a function can modify a *, not a ** without returning. Same request as for fn_strtok.
}



int generate_background_mpg(pic* img, uint8_t ngroups, uint8_t* ntracks)
{
    uint16_t rank=0;
    char tempfile[CHAR_BUFSIZ*10];
    char* mp2track;
    errno=0;

    if (strcasecmp(img->norm, "n") == 0) norm_y=NTSC_Y;  //   x  value is the same as for PAL (720)
    memset(tempfile, '0', sizeof(tempfile));
    sprintf(tempfile, "%s"SEPARATOR"%s", globals.settings.tempdir, "temp.m2v");

    mp2track=(img->action == ANIMATEDVIDEO)? calloc(CHAR_BUFSIZ, sizeof(char)) : NULL;
    if (mp2track)
        sprintf(mp2track, "%s"SEPARATOR"%s", globals.settings.tempdir, "mp2track.mp2");

    if (img->backgroundmpg == NULL) printf("%s", "[MSG]  backgroundmpg will be allocated.\n");

    if (globals.debugging) printf("[INF]  Launching mjpegtools to create background mpg with nmenus=%d\n", img->nmenus);

    img->backgroundpic=readjust_pic_arrays(img->backgroundpic, img->nmenus, 0);
    img->highlightpic=readjust_pic_arrays(img->highlightpic, img->nmenus, 5);
    img->selectpic=readjust_pic_arrays(img->selectpic, img->nmenus, 5);
    img->imagepic=readjust_pic_arrays(img->imagepic, img->nmenus, 5);

    /* now authoring AUDIO_TS.VOB */
    rank=0;

    if (img->action == ANIMATEDVIDEO)
    {
        img->backgroundmpg=realloc(img->backgroundmpg, img->nmenus*sizeof(char*));
        while(rank < img->nmenus)
        {
            create_mpg(img, rank, mp2track, tempfile, ngroups, ntracks);
            fflush(NULL);
            rank++;
        }
    }
    rank=0;

    if (img->action == STILLPICS)
    {
        img->backgroundmpg=realloc(img->backgroundmpg, img->count*sizeof(char*));
        if (img->backgroundmpg)
        while (rank < img->count)
        {
            create_mpg(img, rank, mp2track, tempfile, ngroups, ntracks);
            img->stillpicvobsize[rank]=(uint32_t) (stat_file_size(img->backgroundmpg[rank])/0x800);
            if (img->stillpicvobsize[rank] > 1024) printf("[WAR]  Size of slideshow in excess of the 2MB track limit... some stillpics may not be displayed.\n");
            if (rank) cat_file(img->backgroundmpg[rank], img->backgroundmpg[0]);
            rank++;
        }
        // The first backgroundmpg file is the one that is used to create AUDIO_SV.VOB in amg2.c
    }

    FREE(mp2track)

    if ((globals.debugging) && (!errno))
    printf("%s\n", "[INF]  MPG background authoring OK.");
    return errno;

}


int launch_spumux(pic* img)
{
    // hush up spumux on stdout if non-verbose mode selected

    //sprintf(spumuxcommand, "%s%s%s%s%s%s%s", "spumux -v 0 ", globals.spu_xml, " < ", img->backgroundmpg, (globals.debugging)? "" : " 2>null ", " 1> ", img->topmenu);

    if (globals.debugging) printf("%s\n", "[INF]  Launching spumux to create buttons");
    int menu=0;
    char* spumux=NULL;

    spumux=create_binary_path(spumux, SPUMUX, SEPARATOR SPUMUX_BASENAME);


    while (menu < img->nmenus)
    {
        if (globals.debugging) printf("[INF]  Creating menu %d from Xml file %s\n",menu+1, globals.spu_xml[menu]);
        char *argsspumux[]= {"spumux", "-v", "2", globals.spu_xml[menu], NULL};

        // This is to hush up dvdauthor's stdout messages, which interfere out of sequential order with main application stdout messages
        // and anyway could not be logged by  -l;
        // with normal verbosity, stdout messages end up in a tube's dead end, otherwise they are retrieved at the other end on stdout.
        errno=0;
#ifndef __WIN32__

        int firsttubeerr[2];
        if (pipe(firsttubeerr) == -1)
            perror("[ERR]  Pipe issue with spumux (firsttubeerr[2])");
        char c;

        switch (fork())
        {

        case -1:
            printf("%s\n", "[ERR]  Could not launch spumux");
            break;

        case 0:
            close(firsttubeerr[0]);
            dup2(firsttubeerr[1], STDERR_FILENO);

            if (freopen(img->backgroundmpg[menu], "rb", stdin) == NULL)
            {
                perror("[ERR]  freopen (stdin)");
                return errno;
            }
            if (freopen(img->topmenu[menu], "wb", stdout) == NULL)
            {
                perror("[ERR]  freopen (stdout)");
                return errno;
            }
            execv(spumux, argsspumux);
            return errno;


        default:

            close(firsttubeerr[1]);
            dup2(firsttubeerr[0], STDIN_FILENO);
            wait(NULL);

            while (read(firsttubeerr[0], &c, 1) == 1) putchar(c);

            if (errno)
            {
                printf("%s\n", "[ERR]  Runtime failure in spumux child process");
                perror("[ERR]  spumux");
                return errno;
            }
            close(firsttubeerr[0]);
        }

#else

        char* s=get_command_line(argsspumux);
        uint16_t size=strlen(s);
        char cml[strlen(spumux)+1+size+3+strlen(img->backgroundmpg[menu])+3+strlen(img->topmenu[menu])+1];
        sprintf(cml, "%s %s < %s > %s", spumux, s, img->backgroundmpg[menu], img->topmenu[menu]);
        system(cml);
        free(s);

#endif



        menu++;
    }

    free(spumux);
    return errno;
}



int launch_dvdauthor()
{

    char* dvdauthor=NULL;

    dvdauthor=create_binary_path(dvdauthor,DVDAUTHOR, SEPARATOR DVDAUTHOR_BASENAME);

    errno=0;

    printf("%s %s\n",globals.settings.outdir, globals.xml);

    if (globals.debugging) printf("%s\n", "[INF]  Launching dvdauthor to add virtual machine commands to top menu");

    char *args[]= {DVDAUTHOR_BASENAME, "-o", globals.settings.outdir, "-x", globals.xml, NULL};

#ifndef __WIN32__
    run(dvdauthor, args, 0);
#else
        char* s=get_command_line(args);
        uint16_t size=strlen(s);
        char cml[strlen(dvdauthor)+1+size+1];
        sprintf(cml, "%s %s", dvdauthor, s);
        system(cml);
        free(s);
#endif


#ifndef __WIN32__
    sync();
#endif

   free(dvdauthor);
   return errno;
}


ALWAYS_INLINE_GCC  uint16_t x(uint8_t group, uint8_t ngroups)
{
    return   (20+((norm_x-20)*group)/ngroups + EMPIRICAL_X_SHIFT);
}

// text is within button (i,j) with left-justified spacing of 10 pixels wrt left border
ALWAYS_INLINE_GCC uint16_t y(uint8_t track, uint8_t maxnumtracks)
{
    return (56+((norm_y-60)*track+(norm_y-60)/2)/maxnumtracks);    // = 1/2 (y0[j] + y1[j]
}


int prepare_img(char* text, int8_t group, pic *img, uint8_t maxnumtracks, char* command, char* command2, int menu, char* albumcolor)
{

    int size=strlen(globals.settings.tempdir)+11;
    char picture_save[size];
    char* mogrify=NULL;

    mogrify=create_binary_path(mogrify, MOGRIFY, SEPARATOR MOGRIFY_BASENAME);

    sprintf(picture_save, "%s"SEPARATOR"%s", globals.settings.tempdir, "svpic.png");
    unlink(picture_save);
    errno=0;

    if (img->blankscreen)
        copy_file(img->blankscreen, picture_save);

    if ((group == -1)&&(text))  // album text
    {
        uint16_t x0= EVEN(x( (group>0)?group:0, img->ncolumns)) ;
        snprintf(command, 2*CHAR_BUFSIZ, "%s %s %s \"rgb(%s)\" %s %s %s %d %s %s %d%c%d %c%s%s %s", mogrify,
                 "+antialias", "-fill", albumcolor, "-font", img->textfont, "-pointsize", DEFAULT_POINTSIZE,
                 "-draw", " \"text ", x0, ',' , ALBUM_TEXT_Y0,  '\'', text, "\'\"", picture_save);
        if (globals.debugging) printf("%s%s\n", "[INF]  Launching mogrify (title) with command line: ", command);
        if (system(command) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR] System command failed")
        fflush(NULL);
    }

    if ((img->imagepic[menu]==NULL) || (img->highlightpic[menu]==NULL) || (img->imagepic[menu]==NULL))
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  pic pathnames");
        return -1;
    }
    // will overwrite older files

    copy_file(picture_save, img->imagepic[menu]);
    if (globals.debugging) printf("[INF]  copying %s to %s for menu #%d\n", picture_save, img->imagepic[menu], menu);
    copy_file(picture_save, img->highlightpic[menu]);
    copy_file(picture_save, img->selectpic[menu]);
    errno=0;
    snprintf(command, CHAR_BUFSIZ, "%s %s", mogrify, "+antialias");
    snprintf(command2, CHAR_BUFSIZ, "%s %s", mogrify, "+antialias");

    return errno;
}


int mogrify_img(char* text, int8_t group, int8_t track, pic *img, uint8_t maxnumtracks, char* command, char* command2, int menu, int8_t offset, char* textcolor)
{
    errno=0;
    uint16_t x0, y0;

    x0=EVEN(x( (group>0)?group:0, img->ncolumns)) ;
    y0=EVEN(y(track+1-offset, maxnumtracks+4));

// In automatic mode, we underline presupposing -font Courier with approx 1 letter of font 10 =6 pix in width, sinon préciser la fontwidth

    char *str, *str2;
    str=(char*) calloc(10*CHAR_BUFSIZ, 1);
    if (str == NULL) perror("[ERR]  mogrify, string");
    str2=(char*) calloc(10*CHAR_BUFSIZ, 1);
    if (str2 == NULL) perror("[ERR]  mogrify, string 2");

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
        snprintf(str, 10*CHAR_BUFSIZ, " %s \"rgb(%s)\" %s %s %d%s%d %d%s%d%s ",
                 "-fill", img->highlightcolor_pic, "-draw", " \"rectangle ", x0+deltax0, ",", y0+deltay0,  x0+ deltax1, ",", y0+deltay1, "\"");   // conversion works badly with -colors < 4

    strcat(command, str);
    snprintf(str2, 10*CHAR_BUFSIZ, " %s \"rgb(%s)\" %s %s %s %d %s %s %d%c%d %s%s%s ",
             "-fill", textcolor, "-font", img->textfont, "-pointsize", img->pointsize,
             "-draw", " \"text ", x0, ',' , y0, "\'", text, "\'\"");

    strcat(command2, str2);

    if (errno) perror("[ERR]  mogrify");
    FREE(str)
    FREE(str2)
    return errno;
}


void compute_pointsize(pic* img, uint16_t maxtracklength, uint8_t maxnumtracks)
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

ALWAYS_INLINE_GCC  void test_underline(char* text,pic* img)
{

int j, s=strlen(text);

for (j=0; j < s; j++)
   if ((text[j]== 'g') || (text[j]== 'j') || (text[j]== 'p') || (text[j]== 'q') || (text[j]== 'y'))
       {
       	if (globals.debugging)
       	 printf("[INF]  Switching to little squares rather than underlining motifs for highlight\n       as %c could cut underlines\n", text[j]);
       	 img->highlightformat=-1;
       }

}


int generate_menu_pics(pic* img, uint8_t ngroups, uint8_t *ntracks, uint8_t maxntracks)
{
    if (!img->refresh) return 0;

    FILE* f;
    uint8_t group=0, track=0, buttons=0, menu=0, arrowbuttons=1, groupcount=0, menubuttons;

    uint16_t size;
    uint16_t maxtracklength=0;
    int dim=0, k, j;
    char** grouparray=NULL, **basemotif=NULL, *albumtext=NULL, ***tracktext=NULL, ***grouptext=NULL;

    if (!img->hierarchical)
    {
        maxbuttons=Min(MAX_BUTTON_Y_NUMBER-2,totntracks)/img->nmenus;
        resbuttons=Min(MAX_BUTTON_Y_NUMBER-2,totntracks)%img->nmenus;
    }


    if (img->screentextchain)
    {
        size=(uint16_t) ((norm_x-40-20*(img->ncolumns-1))/img->ncolumns);

        // to avoid using reentrant version of strtok (strtok_r, not mingw32 protable)

        char remainder[strlen(img->screentextchain)];
        basemotif=fn_strtok(img->screentextchain, '=', basemotif, 1, cutloop, remainder) ;
        albumtext=basemotif[0];
        grouparray=fn_strtok(remainder, ':', grouparray, 0, fn_puts, NULL) ;

        dim=arraylength((void **)grouparray);
        tracktext=calloc(dim, sizeof(char**));
        if (tracktext == NULL) perror("[ERR]  Track text allocation");
        grouptext=calloc(dim, sizeof(char**));
        if (grouptext == NULL) perror("[ERR]  Group text allocation");

        for (k=0; k < dim; k++)
        {
            char rem[strlen(grouparray[k])];
            grouptext[k]=fn_strtok(grouparray[k], '=', grouptext[k], 1, cutloop, rem);
            tracktext[k]=fn_strtok(rem, ',', tracktext[k], 0,fn_puts, NULL);
            free(grouparray[k]);
        }

        free(grouparray);
        do
        {
            if (img->hierarchical) test_underline(grouptext[group][0],img);
            do
            {
                maxtracklength=MAX(maxtracklength,strlen(tracktext[group][track]));
                if (strlen(tracktext[group][track]) > size) tracktext[group][track][size]='\0';
		test_underline(tracktext[group][track],img);
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
        dim=Min(img->ncolumns*img->nmenus,ngroups);

        for (k=0; k < dim; k++)
        {
            grouptext[k]=calloc(2, sizeof(char**));
            grouptext[k][0]=calloc(strlen(DEFAULT_GROUP_HEADER_UPPERCASE)+2, sizeof(char));
            sprintf(grouptext[k][0], "%s%d", DEFAULT_GROUP_HEADER_UPPERCASE, k+1);
            tracktext[k]=calloc(ntracks[k]+1, sizeof(char**));
            for (j=0; j < ntracks[k]; j++)
            {
                tracktext[k][j]=calloc(strlen(DEFAULT_TRACK_HEADER)+3, sizeof(char));
                sprintf(tracktext[k][j], "%s%d", DEFAULT_TRACK_HEADER, j+1);
            }
            tracktext[k][ntracks[k]]=NULL;
	    grouptext[k][1]=NULL;
        }


    }


    if (ngroups > img->ncolumns*img->nmenus) printf("[WARN]  Limiting menu to %d groups...\n", img->ncolumns*img->nmenus);

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
        sprintf(picture_save, "%s/%s%d", globals.settings.tempdir, "svpic",menu);

        if (globals.debugging)  printf("%s\n", "[INF]  Authoring top menu streams...");

        if (img->hierarchical)
        {
            maxbuttons=(menu == 0)? ngroups : Min(MAX_BUTTON_Y_NUMBER-2,ntracks[groupcount]);
            resbuttons=0;
        }


        arrowbuttons=(menu < img->nmenus-1)+(menu > 0);
        menubuttons=(menu < img->nmenus-1)? maxbuttons : maxbuttons+resbuttons;

        buttons=0;


        compute_pointsize(img, 10, maxntracks);

        prepare_img(albumtext, -1, img, maxntracks, command1, command2, menu, img->albumcolor);

        if (img->hierarchical)
        {
            if (menu == 0)
            {
                do
                {


                    /* Vicious issue here: use DEFAULT_GROUP_HEADER such that the underline for highlighting does not cut a letter.
                    			   With lower-case "group", this happens as the underline cuts the 'p'. Two ways out: underline lower or use another label/use uppercase
                    			   Note: This issue was tested to cause spumux crash */
                    mogrify_img(grouptext[groupcount][0], 0, groupcount, img, maxntracks, command1, command2, menu,0, img->textcolor_pic);
                    groupcount++;
                    buttons++;

                }
                while (groupcount < ngroups);
                groupcount=0;
            }

            else if (groupcount < ngroups)
            {
                mogrify_img(grouptext[groupcount][0], 0, -1, img, maxntracks, command1, command2, menu,0, img->groupcolor);
                offset=track;

                do
                {
                    buttons++;
                    mogrify_img(tracktext[groupcount][track], 0, track, img, maxntracks, command1, command2, menu, offset, img->textcolor_pic);
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

                mogrify_img(grouptext[groupcount][0], group, -1, img, maxntracks, command1, command2, menu,0, img->groupcolor);
                offset=track;

                do
                {

                    buttons++;
                    mogrify_img(tracktext[groupcount][track], group, track, img, maxntracks, command1, command2, menu, offset, img->textcolor_pic);
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
                mogrify_img(arrowstring, img->ncolumns-1, maxntracks, img, maxntracks, command1, command2, menu, offset, img->arrowcolor);
                if ((menu) && (menu < img->nmenus-1))
                {
                    buttons++;
                    mogrify_img(DEFAULT_PREVIOUS, img->ncolumns-1, maxntracks+1, img, maxntracks, command1, command2, menu, offset, img->arrowcolor);
                }
            }
            while  (buttons < menubuttons+arrowbuttons);

        strcat(command2, img->imagepic[menu]);
        if (globals.veryverbose) printf("[INF] Menu: %d/%d, groupcount: %d/%d.\n      Launching mogrify (image) with command line: %s\n", menu, img->nmenus, groupcount, ngroups, command2);
        if (system(command2) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR] System command failed");
        free(command2);

        copy_file(img->imagepic[menu], img->highlightpic[menu]);

        strcat(command1, img->highlightpic[menu]);
        if (globals.veryverbose) printf("[INF] Menu: %d/%d, groupcount: %d/%d.\n      Launching mogrify (highlight) with command line: %s\n", menu, img->nmenus, groupcount, ngroups,command1);
        if (system(command1) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR] System command failed");
        free(command1);
        char command3[500];
        char* convert=NULL;
        convert=create_binary_path(convert, CONVERT, SEPARATOR CONVERT_BASENAME);

        snprintf(command3, sizeof(command3), "%s %s \"rgb(%s)\"  %s \"rgb(%s)\" %s %s", convert, "-fill", img->selectfgcolor_pic, "-opaque", img->textcolor_pic, img->imagepic[menu], img->selectpic[menu]);
        if (globals.veryverbose) printf("[INF] Menu: %d/%d, groupcount: %d/%d.\n      Launching convert (select) with command line: %s\n",menu, img->nmenus, groupcount, ngroups,command3);
        if (system(command3) == -1) EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR] System command failed");

        menu++;
        group=0;


    }
    while ((menu < img->nmenus)&& (groupcount < ngroups));

    for(group=0; group < dim; group++)
    {
        FREE2(grouptext[group]);
        FREE2(tracktext[group]);
    }

    if (img->screentextchain)
    {
    	free(basemotif[0]);
        free(basemotif[1]);
        free(basemotif);
    }

    if (globals.debugging)
        if (!errno)
            printf("%s\n", "[MSG]  Top menu pictures were authored.");

    return errno;
}

int create_stillpic_directory(char* string, uint32_t count)
{
    struct stat buf;
    static uint32_t  k;
    change_directory(globals.settings.workdir);
    if (k == count)
    {
        if (globals.debugging) printf("[WAR]  Too many pics, only %d sound track%s skipping others...\n", count, (count == 1)?",":"s,");
        return 0;
    }

    if (*string == '\0')
    {
        if (globals.debugging) printf("[INF]  Jumping one track for picture rank=%d\n", k);
        return 1;
    }
    if (stat(string, &buf) == -1)
    {
        perror("[ERR]  create_stillpic_directory:stat");
    }
    if (S_IFDIR & buf.st_mode)
    {
        if (globals.debugging) printf("[INF]  Directory %s will be parsed for still pics\n", string);
        globals.settings.stillpicdir=strdup(string);
        return 0;
    }
    if (S_IFREG & buf.st_mode)
    {
        char dest[strlen(globals.settings.tempdir)+13];
        sprintf(dest, "%s"SEPARATOR"pic_%03d.jpg", globals.settings.tempdir, k);
        if (globals.veryverbose) printf("[INF]  Picture %s will be copied to temporary directory as %s.\n", string, dest);
        copy_file(string, dest);
        if (k == 0) globals.settings.stillpicdir=strdup(globals.settings.tempdir);
        k++;
        return 1;
    }
    return 0;

}















