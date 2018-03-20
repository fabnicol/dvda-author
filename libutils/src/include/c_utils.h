/*

File:   c_utils.h
Purpose: utility library header

libc_utils.a utility library

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net>, 2008

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



#ifndef C_UTILS_H_INCLUDED
#define C_UTILS_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "version.h"
//#include "fixwav_manager.h"

#if defined __WIN32__ || defined _WIN32 || defined __WIN32 || defined __WIN64 || defined _WIN64
#define SEPARATOR "\\"
#define STRLEN_SEPARATOR 2
#else
#define SEPARATOR  "/"
#define STRLEN_SEPARATOR 1
#endif

#include <sys/types.h>
#if defined __WIN32__ || defined _WIN32 || defined __WIN32 || defined __WIN64 || defined _WIN64
#include <sys/stat.h>
//#else
//#include <io.h>
#endif
#include <string.h>
#include <stdint.h>
#ifndef __MINGW32__
#include <sys/resource.h>
#endif



#ifdef __GNUC__
#define GCC_INLINE __attribute__((always_inline))
#define GCC_UNUSED __attribute__((__unused__))
#endif

#define MAX_OPTION_LENGTH 3000

/* sets the size of character-type buffers (command-line parsing etc). */

// if CHAR_BUSFIZ not defined at compile time, then is 512
#ifndef CHAR_BUFSIZ
#define CHAR_BUFSIZ    1024
#endif
#define MAX_HEADER_SIZE 255

#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE   64
#endif

#define HEX_COLUMNS 8
#define FAIL 10

#define PAD 1
#define NO_PAD 0

#ifndef LITTLE_ENDIAN
    #define LITTLE_ENDIAN  1
    #define BIG_ENDIAN  0
#endif

#ifdef NO_ANSI_COLORS
    #define ANSI_COLOR_RED     ""
    #define ANSI_COLOR_GREEN   ""
    #define ANSI_COLOR_YELLOW  ""
    #define ANSI_COLOR_BLUE    ""
    #define ANSI_COLOR_MAGENTA ""
    #define ANSI_COLOR_CYAN    ""
    #define ANSI_COLOR_RESET   ""

    #define MSG_TAG "[MSG]  "
    #define INF "[INF]  "
    #define ERR "\n[ERR]  "
    #define DBG "[DBG]  "
    #define WAR "[WAR]  "
    #define DEV "[DEV]  "
    #define PAR "[PAR]  "

#else
    #define ANSI_COLOR_RED     "\x1b[31m"
    #define ANSI_COLOR_GREEN   "\x1b[32m"
    #define ANSI_COLOR_YELLOW  "\x1b[33m"
    #define ANSI_COLOR_BLUE    "\x1b[34m"
    #define ANSI_COLOR_MAGENTA "\x1b[35m"
    #define ANSI_COLOR_CYAN    "\x1b[36m"
    #define ANSI_COLOR_RESET   "\x1b[0m"

    #define MSG_TAG ANSI_COLOR_GREEN    "[MSG]  "   ANSI_COLOR_RESET
    #define INF     ANSI_COLOR_BLUE     "[INF]  "   ANSI_COLOR_RESET
    #define ERR     ANSI_COLOR_RED      "\n[ERR]  " ANSI_COLOR_RESET
    #define DBG     ANSI_COLOR_MAGENTA  "[DBG]  "   ANSI_COLOR_RESET
    #define WAR     ANSI_COLOR_YELLOW   "[WAR]  "   ANSI_COLOR_RESET
    #define DEV     ANSI_COLOR_CYAN     "[DEV]  "   ANSI_COLOR_RESET
    #define PAR     ANSI_COLOR_MAGENTA  "[PAR]  "   ANSI_COLOR_RESET
#endif


#define ERR_STRING_LENGTH   "ERR: string was truncated, maximum length is %d"

#define Min(X ,Y)    (((X) <= (Y))  ? (X):(Y))
#define MAX(X ,Y)    (((X) <= (Y))  ? (Y):(X))

/* STRING_WRITE is devised for strings allocated by arrays */

#define STRING_WRITE(X,Z,...)	 do { int chres, Y;\
                                      Y=sizeof(X); \
                                      if    ( ( chres = snprintf(X, Y , Z, __VA_ARGS__) ) >=  Y )  \
									  printf("\n"ERR_STRING_LENGTH"\n", Y);\
                                      else   if (chres < 0 ) fprintf(stderr, "\n"ANSI_COLOR_RED"\n[ERR] Error message:"ANSI_COLOR_RESET"  %s\nCheck source code %s, line %d",  strerror(errno), __FILE__, __LINE__); } while(0);


#define STRING_WRITE_CHAR_BUFSIZ(X,Z,...)	 do { int chres;\
													 if    ( ( chres = snprintf(X, CHAR_BUFSIZ*sizeof(char) , Z, __VA_ARGS__) ) >=  CHAR_BUFSIZ )  \
														   fprintf(stderr, "\n"ERR_STRING_LENGTH"\n", CHAR_BUFSIZ);\
														   else   if (chres < 0 ) printf( "\n\n[ERR] Error message:  %s\nCheck source code %s, line %d",  strerror(errno), __FILE__, __LINE__); } while(0);

#ifdef foutput
#undef foutput
#endif

#define foutput(X,...)   do { if (!globals.silence) printf(X, __VA_ARGS__);\
                               if (globals.logfile) \
                                fprintf(globals.journal, X, __VA_ARGS__);} while(0)



/* ERROR MANAGEMENT

    Error management conventions:
       EXIT_SUCCESS is returned on errors caused by ill-formed user input.
       EXIT_FAILURE is reserved for internal errors (segfaults, etc.)
	EXIT_ON_ERROR is a macro for uncommented EXIT_FAILURE
	EXIT_ON_ERROR_VERBOSE  is a macro for commented EXIT_FAILURE (one argument string)
	in other contexts (EXIT_SUCCESS, complex EXIT_FAILURE... )  clean_exit is used, see auxiliaray.c
*/


#define NOWAIT -1

/* Structures */


typedef struct
{
 _Bool    isfile;
 _Bool cdup_position[CHAR_BUFSIZ];
 _Bool pwd_position[CHAR_BUFSIZ];
 char*    directory;
 char*    extension;
 char*    rawfilename;
 char*    filename;
 char*    path;
 uint16_t separators;
 uint16_t length;
}path_t;

typedef struct
 {
    struct rusage *nothing;
    struct rusage *start;
} compute_t;


typedef struct
{
   _Bool isopen;
   uint64_t filesize;
   char* filename;
   FILE* fp;
} filestat_t ;


uint64_t filesize(filestat_t f);
char* filename(filestat_t f);
FILE* fileptr(filestat_t f);

filestat_t filestat(_Bool b, uint64_t s, const char* fn, FILE* fp);

typedef struct
  {
    /* pointers */

    char* database;
    char* filetitle;

    /* global behavior booleans are set to zero by default, being global */
    _Bool automatic;  /* whether automatic processing mode is selected */
    _Bool prepend;  /* whether new header is prepended to raw data or overwrites old header */
    _Bool in_place; /* whether old file is overwritten */
    _Bool cautious; /* whether to ask user before overwrite */
    _Bool interactive; /* whether interactive dialogs will be used */
    /* global diagnosis values */
    _Bool padding; /* whether files should be end-padded */
    _Bool prune; /* whether files ending with 00 should be pruned */
    _Bool virtual;
    short int repair;
    uint32_t padbytes;
    uint32_t prunedbytes;


    filestat_t infile;
    filestat_t outfile;

    /* header substructure */

  } WaveData;

typedef struct
  {
    _Bool       is_extensible;
    _Bool       has_fact;
    uint8_t     ichunks;
    uint8_t*    header_in;
    uint8_t*    header_out;
    uint16_t     header_size_in; /* size of header */
    uint16_t     header_size_out; /* size of header */
    uint16_t	 wFormatTag;	/* should be 1 for PCM-code */
    uint16_t	 channels;	/* 1 Mono, 2 Stereo */
    uint16_t	 nBlockAlign;	/* samplesize*/
    uint16_t     cbSize;  /* 0 or 22 */
    uint16_t	 wBitsPerSample;	/* 8, 12, 16, or 24 bit */
    uint32_t     dwChannelMask;  /* channel mapping to hardware */
    uint32_t	 ckID;	/* 'RIFF' */
    uint32_t	 ckSize;	/* filelen */
    uint32_t	 WAVEID;	/* 'WAVE' */

    uint32_t	 fmt_ckID;	/* 'fmt ' */
    uint32_t	 fmt_ckSize;	/* length of fmt_ckID = 16 */
    uint32_t	 dwSamplesPerSec
;	/* frequence of sample */
    uint32_t	nAvgBytesPerSec; /* bytes per second */
    uint32_t    fact_chunk; /* 'fact'*/
    uint32_t    fact_length; /* length of fact chunk - 8 in bytes = 4*/
    uint32_t    n_spl;       /* number of samples written out */
    uint32_t	 data_ckID;	/* 'data' */
    uint32_t	 data_cksize;	/* samplecount */
    /* RIFF info chunks to be parsed: INAM, IART, ICMT, ICOP, ICRD, IGNR */
    uint8_t INAM[MAX_LIST_SIZE];
    uint8_t IART[MAX_LIST_SIZE];
    uint8_t ICMT[MAX_LIST_SIZE];
    uint8_t ICOP[MAX_LIST_SIZE];
    uint8_t ICRD[MAX_LIST_SIZE];
    uint8_t IGNR[MAX_LIST_SIZE];

  } WaveHeader;

/* Prototypes */

void htmlize(char* logpath);

char * conc(const char* str1, const char* str2);
char * filepath(const char* str1, const char* str2);
void pause_dos_type();
_Bool clean_directory(char* path);
void clean_exit(int message);
void starter(compute_t *timer);
char* print_time(int);
int secure_mkdir ( const char *path, mode_t mode);
_Bool s_mkdir (const char *path);
void print_commandline(int argc_count, char * const argv[]);
void change_directory(const char * filename);
int copy_file(const char *existing_file, const char *new_file);
int copy_directory(const char* src, const char* dest, mode_t mode);
int cat_file(const char *existing_file, const char *new_file);
int copy_file_p(FILE *infile, FILE *outfile, uint32_t position, uint64_t output_size);
_Bool file_exists(const char* filepath);
int stat_dir_files(const char* src);
_Bool s_dir_exists(const char* path);
int traverse_directory(const char* src, void (*f)(const char GCC_UNUSED*, void GCC_UNUSED *, void GCC_UNUSED *), _Bool recursive, void GCC_UNUSED* arg2, void GCC_UNUSED* arg3);
int get_endianness();
void hexdump_header(FILE* infile, uint8_t header_size);
void hexdump_pointer(uint8_t* tab,  size_t tabsize);
void hex2file(FILE* out, uint8_t* tab,  size_t tabsize);
void hex2file_csv(FILE* out, uint8_t* tab,  size_t tabsize);

int end_seek(FILE* outfile);
void parse_wav_header(WaveData* info, WaveHeader* ichunk);
const char* get_command_line(const char* args[]);
char* get_full_command_line(const char** args);
// These functions should be inlined hence in a header file
char* copy_file2dir(const char *existing_file, const char *new_dir);
char* copy_file2dir_rename(const char *existing_file, const char *new_dir, char* newfilename);
path_t *parse_filepath(const char* filepath);
char* make_absolute(char* filepath);
char *fn_get_current_dir_name (void);
int  rmdir_global(char* path);
int  rmdir_recursive (char *root, char *dirname);
#if HAVE_curl
int download_file_from_http_server(const char* curlpath, const char* file, const char* server);
int download_fullpath(const char* curlpath, const char* filename, const char* fullpath);
#endif
void erase_file(const char* path);
char* quote(const char* path);
char* win32quote(const char* path);
int run(const char* application, const char*  args[], const int option);
uint64_t  parse_file_for_sequence(FILE* fp, uint8_t* tab, size_t sizeoftab);
void test_field(uint8_t* tab__, uint8_t* tab, int size,const char* label, FILE* fp, FILE* log, _Bool write, _Bool);
void rw_field(uint8_t* tab, int size,const char* label, FILE* fp, FILE* log);


inline static void  uint32_copy(uint8_t* buf, uint32_t x)
{
    buf[0]=(x>>24)&0xff;
    buf[1]=(x>>16)&0xff;
    buf[2]=(x>>8)&0xff;
    buf[3]=x&0xff;
}

inline static void uint32_copy_reverse(uint8_t* buf, uint32_t x)
{
    buf[0]=x&0xff;
    buf[1]=(x&0xff00)>>8;
    buf[2]=(x&0xff0000)>>16;
    buf[3]=x>>24;
}


inline static void uint16_copy(uint8_t* buf, uint16_t x)
{
    buf[0]=(x>>8)&0xff;
    buf[1]=x&0xff;
}

inline static void uint16_copy_reverse(uint8_t* buf, uint16_t x)
{
    buf[0]=x&0xff;
    buf[1]=(x&0xff00)>>8;
}


inline static uint32_t uint32_read(uint8_t* buf)
{
	return( buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] );
}

inline static uint16_t uint16_read(uint8_t* buf)
{
	return( buf[0] << 8 | buf[1] );
}

inline static uint32_t uint32_read_reverse(uint8_t* buf)
{
	return( buf[0]  | buf[1] << 8 | buf[2] << 16 | buf[3] << 24);
}

inline static uint16_t uint16_read_reverse(uint8_t* buf)
{
	return( buf[0]  | buf[1] << 8);
}

uint8_t read_info_chunk(uint8_t* pt, uint8_t* chunk);


#endif // C_UTILS_H_INCLUDED
