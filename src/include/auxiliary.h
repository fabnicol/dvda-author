/*
File:    auxiliary.h
Purpose: auxiliary functions and macros

dvda-author  - Author a DVD-Audio DVD

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
#ifndef  AUXILIARY_H_INCLUDED
#define AUXILIARY_H_INCLUDED
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <sys/types.h>
#ifndef __WIN32__
#include <dirent.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include "audio2.h"
#include "c_utils.h"
#include "commonvars.h"
#include "structures.h"

#ifdef COMPILER_IS_GCC
    #define GCC_ATTRIBUTE_UNUSED __attribute__((unused))
#else
   #define GCC_ATTRIBUTE_UNUSED
#endif

// debugging macros
#define prs(X) do {if (X) foutput("PRS: %s : %s\n", #X, X);} while (0);
#define prd(X) foutput("PRD: %s : %d\n", #X, X);
#define pr(X) foutput("PR: %s\n", #X);
/* All macros are below except for OS-specific macros in ports.h*/

// Binary-coded digital numbers are hex numbers that should be read as their decimal values

#define BCD(X)   ((X)/16*10 + (X)%16)
#define BCD_REVERSE(X)   ((X)/10*16 + (X)%10)

#define EXIT_ON_RUNTIME_ERROR_VERBOSE(X)  do {  foutput(ERR "%s\n       Exiting...\n", X) ; fflush(NULL); clean_exit(EXIT_FAILURE, globals);  } while(0);
#define EXIT_ON_RUNTIME_ERROR  EXIT_ON_RUNTIME_ERROR_VERBOSE("")
#define EXITING EXIT_ON_RUNTIME_ERROR


#define HEADER(X, Y)      do{ \
                          foutput("\n%s\n",     " ---------- " X" version "Y" ----------");\
                          foutput("\n%s\n", INFO_GNU);\
    if (! globals->silence) print_time(1); foutput("%s", "\n");}while(0);


#define FREE(X)  { free(X) ; X = NULL; }

#define EVEN(X)  (int16_t) ((X % 2) ? X+1 : X)

#define EXPL(X,Y,Z,...)  do {if (Y) \
                                {if (!globals->logfile) \
                                    fprintf(stderr, "%s" X, Z, __VA_ARGS__);  \
                                else \
                                    foutput("%s" X, Z, __VA_ARGS__); \
                                }\
                            } while(0);

#define EXPLAIN(X,...)       EXPL(X,globals->veryverbose, DBG "Now ",__VA_ARGS__)

#define EXPLAIN_DEV(...)     EXPL("%s %d\n", globals->maxverbose, DEV,__VA_ARGS__ )


/* end of macros */

void help(void);
void version(globalData*);
bool increment_ngroups_check_ceiling(uint8_t *ngroups, uint8_t *nvideolinking_groups, globalData* );
fileinfo_t** dynamic_memory_allocate(fileinfo_t **  files, uint8_t ngiven_channels[9][99], uint8_t* ntracks, uint8_t   ngroups, uint8_t n_g_groups, uint8_t nvideolinking_groups, globalData*);
void free_memory(command_t *command, globalData*);
void check_settings_file(void);
int create_file(char* audiotsdir, char* basename, uint8_t* array, size_t size, globalData* globals);
char** fn_strtok(char* chain, char delim,
                 char** array, uint32_t* size,
                 int32_t count, int  (*f)(char*, int32_t , globalData*), char* remainder, globalData* globals);
int cutloop(char GCC_ATTRIBUTE_UNUSED *c, int32_t count, globalData*);
int arraylength(char ** tab);
extern FILE* aob_log;

static void GCC_UNUSED open_aob_log(globalData* globals)
{
    if (globals->settings.outfile == NULL || globals->settings.outfile[0] == '\0')
    {
        fprintf(stderr, "%s\n", ERR "Empty log-decode filepath.");
        exit(-1);
    }
    aob_log = fopen(globals->settings.outfile, "ab");
    if (aob_log == NULL)
    {
        fprintf(stderr, "%s%s%s\n", ERR "AOB log *", globals->settings.outfile, "* could not be opened.");
        fflush(NULL);
        exit(-1);
    }
}

static void GCC_UNUSED close_aob_log()
{
    fclose(aob_log);
}
#endif // AUXILIARY_H_INCLUDED
