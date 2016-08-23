/*

File:   c_utils.c
Purpose: utility library

Copyright Fabrice Nicol <fabnicol@users.sourceforge.net>, 2008-2016

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
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

#include <stdlib.h>
#include <stdio.h>

#ifndef __unix__
 #undef __STRICT_ANSI__
 #include <io.h>
#else
 #include <unistd.h>
#endif

#include <dirent.h>
#include <stdarg.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef __unix__
 #include <sys/wait.h>
#endif

#include <sys/types.h>

/* printf is a public macro
 * foutput_c is a private one
 * using both public and private macros */
//#include "ports.h"
#include "c_utils.h"
#include "private_c_utils.h"
// here include your main application's header containing a globalData structure with main application globals.
#include "structures.h"
#include "libiberty.h"
#include "winport.h"

#undef __MS_types__
extern globalData globals;

void pause_dos_type()
{
    char reply;
    char buffer[150];
    puts("Press twice on Enter to continue...");

    do
    {
        int l = scanf("%c", &reply);
        char* r = fgets(buffer, Min(150, l), stdin);
        if (reply == '\n' && r != NULL) return;
    }
    while(1);



}


void erase_file(const char* path)
{
    FILE *f;

    if  ((f=fopen(path, "rb")) != NULL)
    {
        fclose(f);
        unlink(path);
    };
    errno=0;
}


#if HAVE_curl

int download_file_from_http_server(const char* curlpath, const char* filename, const char* server)
{
    char command[strlen(server)+strlen(filename)+strlen(curlpath) + 32];

    sprintf(command, "%s -# -f  -o %s --location %s/%s", curlpath, filename, server, filename);
    fprintf(stderr, ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  downloading %s from server %s\n", filename, server);
    if (globals.veryverbose) fprintf(stderr, ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  ...%s\n", command);
    return system(win32quote(command));
}

int download_fullpath(const char* curlpath, const char* filename, const char* fullpath)
{
    char command[30+1+strlen(filename)+strlen(fullpath)];
    sprintf(command, "%s -# -f -o %s --location %s", curlpath, filename, fullpath);
    if (globals.veryverbose) printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  downloading: %s\n", command);
    return system(win32quote(command));
}

#endif


// From Yves Mettier's "C en action" (2009, ENI)
// Patched somehow.

char *fn_get_current_dir_name (void)
{
    char *cwd;
    int len = 64;
    char* r;
    if (NULL == (cwd = malloc (len * sizeof *cwd)))
    {
        printf ("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Not enough memory for my_get_cwd.\n");
        exit (EXIT_FAILURE);
    }
    while ((NULL == (r = getcwd (cwd, len))) && (ERANGE == errno))
    {
        len += 32;
        if(NULL == (cwd = realloc (cwd, len * sizeof *cwd)))
        {
            printf ("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Not enough memory for my_get_cwd.\n");
            exit (EXIT_FAILURE);
        }
    }
    if (r)
        return (cwd);
    free (cwd);
    return (NULL);
}

char* make_absolute(char* filepath)
{
    path_t* path_info = parse_filepath(filepath);
    char buffer[2*CHAR_BUFSIZ] = {0};

    for (int r = 0; r < path_info->separators; ++r)
    {
        if (path_info->pwd_position[r])
        {
            if (r == 0)
            {
                char* current_dir = fn_get_current_dir_name();

                if (current_dir == NULL || current_dir[0] == '\0') return "";
                int u = 0;
                for(char c; (c = current_dir[u]) != '\0'; ++u)  buffer[u] = c;

                buffer[u] = SEPARATOR[0];

                u = 0;
                for(char c; (c = filepath[u]) != '\0'; ++u)     buffer[u] = c;

                return (make_absolute(buffer));

            }
            else
            {
                int u = 0;
                int counter = 0;
                while(counter < r)
                {
                    buffer[u] = filepath[u];
                    if (filepath[u] == SEPARATOR[0]) ++counter;
                    ++u;
                }

                buffer[u] =  SEPARATOR[0];
                ++u;
                char c;
                for(int j = u; (c = filepath[u + 2]) != '\0'; ++j)  buffer[j] = c;

                return (make_absolute(buffer));
            }
        }
        else
        if (path_info->cdup_position[r])
        {

            int u = 0;
            int counter = 0;

            while(counter < r)
            {
                if (counter < r - 1) buffer[u] = filepath[u];
                if (filepath[u] == SEPARATOR[0]) ++counter;
                ++u;
            }

            buffer[u] =  SEPARATOR[0];
            ++u;

            char c;
            for(int j = u; (c = filepath[u]) != '\0'; ++j)     buffer[j] = c;
            return (make_absolute(buffer));
        }
    }

    return(strdup(buffer));

    // it is up to the user to deallocate filepath string input
}


void action_dir_post (const char *root, const char *dir)
{
    if (rmdir (dir))
    {
        printf (ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to erase directory %s/%s \n"
                "(errno = %s)\n", root, dir, strerror (errno));
        exit (EXIT_FAILURE);
    }
}


void action_file (const char *file)
{
    if (unlink (file))
    {
        printf (ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to erase file %s \n"
                "(errno = %s)\n", file, strerror (errno));
        exit (EXIT_FAILURE);
    }
}


typedef struct slist_t
{
    char *name;
    int is_dir;
    struct slist_t *next;
} slist_t;


int rmdir_recursive (char *root, char *dirname)
{
    char *cwd;
    cwd=fn_get_current_dir_name();
    if (chdir (dirname) == -1)
    {
        if (errno == ENOTDIR)
        return 0;
        //printf ( ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  chdir() issue with dirname=%s\n", dirname);
        else return (-1);
    }

    slist_t *names = NULL;
    slist_t *sl;

    DIR *FD;
    struct dirent *f;
    char *new_root;

    if (root)
    {
        int rootlen = strlen (root);
        int dirnamelen = strlen (dirname);
        if (NULL ==
                (new_root =
                     malloc ((rootlen + dirnamelen + 2) * sizeof *new_root)))
        {
            printf ("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  malloc issue\n");
            exit (EXIT_FAILURE);
        }
        memcpy (new_root, root, rootlen);
        new_root[rootlen] = '/';
        memcpy (new_root + rootlen + 1, dirname, dirnamelen);
        new_root[rootlen + dirnamelen + 1] = '\0';
    }
    else
        new_root = strdup (dirname);


    if (NULL == (FD = opendir (".")))
    {
        printf ("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  opendir() issue\n");
        return (-1);
    }
    sl = names;
    while ((f = readdir (FD)))
    {
        struct stat st;
        slist_t *n;
        if (!strcmp (f->d_name, "."))
            continue;
        if (!strcmp (f->d_name, ".."))
            continue;
        if (stat (f->d_name, &st))
            continue;
        if (NULL == (n = malloc (sizeof *n)))
        {
            printf ("%s", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  memory issue\n");
            exit (EXIT_FAILURE);
        }
        n->name = strdup (f->d_name);
        if (S_ISDIR (st.st_mode))
            n->is_dir = 1;
        else
            n->is_dir = 0;
        n->next = NULL;
        if (sl)
        {
            sl->next = n;
            sl = n;
        }
        else
        {
            names = n;
            sl = n;
        }
    }
    closedir (FD);


    for (sl = names; sl; sl = sl->next)
    {
        if (!sl->is_dir)
            action_file (sl->name);
    }


    for (sl = names; sl; sl = sl->next)
    {
        if (sl->is_dir)
        {
            // action_dir_pre (new_root, sl->name);
            rmdir_recursive (new_root, sl->name);
            action_dir_post (new_root, sl->name);
        }
    }


    free (new_root);
    while (names)
    {
        slist_t *prev;
        free (names->name);
        prev = names;
        names = names->next;
        free (prev);
    }
    chdir (cwd);
    free (cwd);
    return (0);
}

// End of Yves Mettier code

/* -------
* rmdir_global
*
* Erases a directory without testing for errors.
* Returns errno
* ------- */


int rmdir_global(char* path)
{
    int error=rmdir_recursive(NULL, path);
    return (error);
}

void unix2dos_filename(char* path)
{
    /* does not assume null-terminated strings, may be tab of chars */

    for (uint u = 0; u < strlen(path); ++u)
    {
       switch (path[u])
       {
         case '/' :  path[u] = '\\'; break;
         case '.' :  if (u == 0 && path[1] == '/') ++path; ++path; break;  /* ./dir -> dir */
         default :  break;
       }
    }
}

void dos2Unix_filename(char* path)
{
    /* does not assume null-terminated strings, may be tab of chars */

    for (uint u = 0; u < strlen(path); ++u)
    {
       if (path[u] == '\\')  path[u] = '/';
#      if defined(__MSYS__) || defined (__CYGWIN__)
       if (u == 1 && path[1] == ':')
       {
           path[1] = path[0];
           path[0] = '/';            /*  C: -> /c ; useless to lower case */
       }
#      endif
    }
}

void fix_separators(char * path)
{
    /* does not assume null-terminated strings, may be tab of chars */

    if (SEPARATOR[0] == '\\')
        unix2dos_filename(path);
    else
        dos2Unix_filename(path);
}

/* -------
*  end_sep
*
*  Returns input , adding a trailing separator if missing.
*  Note: does not deallocate input
* ------- */


char* end_sep(const char *path)
{
    int s = strlen(path);
    char* out;
    if (path[s - 1] != '/' && path[s - 1] != '\\')
    {
     out = calloc(s + 2, sizeof(char));
     if (out == NULL)
     {
         perror("Allocation end_sep");
         clean_exit(EXIT_FAILURE);
     }

     memcpy(out, path, s * sizeof(char));

     out[s + 1] = SEPARATOR[0];
     // null termination is ensuered by calloc.
    }

    return out;
}


/*-------
* parse_filepath
*
* Examine filepath structure, notably: extension, filename, path, rawfilename, isfile
* Returns structure path_t
* ------- */

path_t *parse_filepath(const char* filepath)
{
    path_t *chain = calloc(1, sizeof(path_t));
    if (chain == NULL) return NULL;
    int u, last_separator_position=0, dot_position=0;
    for (u=0; chain->length == 0 ; ++u)
    {
        switch (filepath[u])
        {
        case  '.' :
            if (filepath[u+1] != '/' && filepath[u+1] != '\\')
            {
                if (filepath[u+1] != '.')
                {
                   dot_position = u;
                }
                else
                {
                   chain->cdup_position[chain->separators] = true;
                }
            }
            else
                chain->pwd_position[chain->separators] = false;

            break;

        case  '/' :

#       if defined(__WIN32__) || defined(_WIN32) || defined (_WIN64) || defined(__WIN32) || defined(__MSYS__)
        case  '\\':
#       endif

            chain->separators++;
            last_separator_position = u;
            break;
        case  '\0':
            chain->length = u;
            break;

        }
    }

    chain->extension = strdup(filepath+dot_position);
    chain->filename = strdup(filepath+last_separator_position+1);
    chain->path = strdup(filepath);
    chain->path[last_separator_position] = 0;
    chain->rawfilename=strdup(filepath);
    chain->rawfilename[dot_position] = 0;
    chain->rawfilename += last_separator_position+1;
    if (last_separator_position >1)
    {
        for (u=last_separator_position-1; u>=0 ; u--)
        {
            if (chain->path[u] == '/'
#ifdef __WIN32__
                    || chain->path[u] == '\\'
#endif
               )
            {
                last_separator_position=u;
                break;
            }
        }

        chain->directory=strdup(chain->path+last_separator_position+1);  // directory in which filepath is placed, without leading path.
    }
    else chain->directory=NULL; // We're at the root.

    errno=0;
    FILE* f=fopen(filepath, "rb");
    if ((errno) || (f == NULL))
    {
        chain->isfile=0;
        if (globals.veryverbose)
        {
            printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Path %s is not a file\n", filepath);
        }
    }
    else
    {
        chain->isfile=1;
        fclose(f);
    }
    errno=0;


    return chain;
}

/* -------
*  concatenate
*
*  Concatenates two strings into the forst argument.
*  allocates heap memory to produce the result of the concatenation of two strings and returns the length of the concatenate
*  the result of the concatenation is placed into dest which is reallocated
*  dest should either be not allocated or previously allocated with m/calloc: not static arrays.
*  returns -1 if either argument is null or 0 on error
*  ------- */

char * concatenate(char* dest, const char* str1, const char* str2)
{
    if ((!str1) || (!str2)) return NULL;
    errno=0;
    uint16_t s1=strlen(str1);
    uint16_t s2=strlen(str2);

    dest=realloc(dest, (s1+s2+1)*sizeof(char));

    memcpy(dest, str1, s1);
    memcpy(dest+s1, str2, s2);
    dest[s1+s2]=0;
    if (errno) return NULL;
    else return dest;

}

/*-------
* clean_directory
*
* Erases direcory and check for any error
* ------- */

_Bool clean_directory(char* path)
{

    errno=0;
    if (path == NULL) return (EXIT_FAILURE);

    if (globals.veryverbose) printf("%s%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Cleaning directory ", path);

    errno=rmdir_global(path);

    if (errno)
    {
        if (globals.veryverbose)
            printf("%s%s\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Failed to clean directory ", path);
        return 0;
    }
    else
    {
        if (globals.veryverbose)
            printf("%s\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  OK.");
        return 1;
    }
}

/*-------
* htmlize
*
* This postprocessing procedure converts a tagged log
* (with "ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET",
*  "ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET",
*  "ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET",
*  "ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET") into an Html logpage
* To be launched at end of program, not in a thread
* ------- */

void htmlize(char* logpath)
{

FILE* src=fopen(logpath, "rb");
if (src == NULL) return;
char* loghtmlpath=calloc(strlen(logpath)+6, 1);
if (loghtmlpath == NULL) return;
sprintf(loghtmlpath,"%s%s",logpath,".html");

FILE* dest=fopen(loghtmlpath, "wb");
if (dest == NULL) return;

#define NAVY            "<p><span style=\"color: navy; font-size: 10pt;  \">"
#define RED             "<p><span style=\"color: red;  font-size: 12pt   \">"
#define GREY            "<br/><span style=\"color: grey; font-size: 8pt; \">"
#define GREEN           "<p><span style=\"color: green; font-size: 10pt; \">"
#define ORANGE          "<p><span style=\"color: orange;font-size: 12pt; \">"
#define MAROON          "<br/><span style=\"color: maroon;font-size: 8pt;\">"
#define PURPLE          "<br/><span style=\"color: purple;font-size: 8pt;\">"
#define CLOSETAG1        "</span>"
#define CLOSETAG2        "</span></p>"
#define HEADER "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n\
<HTML><HEAD><TITLE>dvda-author " VERSION " Html log</TITLE>\n\
</HEAD><BODY>\n"

int length=strlen(NAVY);
char line[1000];

fwrite(HEADER, strlen(HEADER), 1, dest);

do
{

    char* line_ent = fgets(line, 1000, src);

    if (line_ent == NULL) return;

    int linelength=strlen(line);

    if ((line[0] == '[') && (line[1] == 'I') && (line[2]=='N') && (line[3] == 'F') && (line[4]==']'))
    {
        fwrite(NAVY, length, 1, dest);
        fwrite(line, linelength, 1, dest);
        fwrite(CLOSETAG2, 11, 1, dest);
        fputc('\n', dest);
    }
    else if ((line[0] == '[') && (line[1] == 'M') && (line[2]=='S') && (line[3] == 'G') && (line[4]==']'))
    {
        fwrite(GREEN, length, 1, dest);
        fwrite(line, linelength, 1, dest);
        fwrite(CLOSETAG2, 11, 1, dest);
        fputc('\n', dest);
    }
    else if ((line[0] == '[') && (line[1] == 'D') && (line[2]=='E') && (line[3] == 'V') && (line[4]==']'))
    {
        fwrite(PURPLE, length, 1, dest);
        fwrite(line, linelength, 1, dest);
        fwrite(CLOSETAG1, 7, 1, dest);
        fputc('\n', dest);
    }
    else if ((line[0] == '[') && (line[1] == 'D') && (line[2]=='B') && (line[3] == 'G') && (line[4]==']'))
    {
        fwrite(MAROON, length, 1, dest);
        fwrite(line, linelength, 1, dest);
        fwrite(CLOSETAG1, 7, 1, dest);
        fputc('\n', dest);
    }

    else if ((line[0] == '[') && (line[1] == 'W') && (line[2]=='A') && (line[3] == 'R') && (line[4]==']'))
    {
        fwrite(ORANGE, length, 1, dest);
        fwrite(line, linelength, 1, dest);
        fwrite(CLOSETAG2, 11, 1, dest);
        fputc('\n', dest);
    }
    else if ((line[0] == '[') && (line[1] == 'E') && (line[2]=='R') && (line[3] == 'R') && (line[4]==']'))
    {
        fwrite(RED, length, 1, dest);
        fwrite(line, linelength, 1, dest);
        fwrite(CLOSETAG2, 11, 1, dest);
        fputc('\n', dest);
    }
    else
    {
        // Skipping white lines (spaces and tabs) or line feeds, yet not justifying
        int u=0;
        while ((line[u]) && (isspace(line[u]))) u++;
        if (line[u])
        {
            fwrite(GREY, length, 1, dest);
            fwrite(line, linelength, 1, dest);
            fwrite(CLOSETAG1, 7, 1, dest);
            fputc('\n', dest);
        }
    }


    }
    while (!feof(src));

    fwrite("</BODY></HTML>\n", 15, 1, dest);
    free(loghtmlpath);
    fclose(src);
    fclose(dest);

#undef NAVY
#undef RED
#undef GREY
#undef GREEN
#undef ORANGE
#undef CLOSETAG
#undef HEADER

}


/*-------
* clean_exit
*
* Logs time; flushes all streams;  erase empty backup dirs; closes log;
* ------- */

void clean_exit(int message)
{
    fflush(NULL);

    if (globals.logfile)
    {
        fclose(globals.journal);
    }

    exit(message);
}


/*-------
* s_dir_exists
*
* Tests for existence of directory securely :
* terminates if path cannot be accessed.
* ------- */

_Bool s_dir_exists(const char* path)
{
  struct stat info;

    if (stat(path, &info) != 0)
    {
        printf( "[ERR] Cannot access %s\n", path);
        clean_exit(EXIT_FAILURE);
    }
    else
    if (info.st_mode & S_IFDIR)
    {
        printf( "[WAR] Directory %s already exists.\n", path);
        errno = 0;
        return true;
    }

    return false;
}

/*-------
* secure_mkdir
*
* Creates directories safely:
* balks out if directory exists, if path is null/empty,
* or cannot be allocated. Stops execution.
* ------- */

_Bool s_mkdir (const char *path)
{
    return (secure_mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_ISVTX) == 0);
}

int secure_mkdir (const char *path, mode_t mode)
{

 /* test for existence */

    if (s_dir_exists(path)) return 0;


    int i=0, len;
    if (path == NULL || path[0]=='\0')
    {
     fprintf(stderr, "%s","\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not create directory with empty or null path.\n");
     clean_exit(EXIT_FAILURE);
    }
    len = strlen (path);

    // requires std=c99


    if  ((len<1) && (globals.debugging))
    {
        printf("%s\n",ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Path length could not be allocated by secure_mkdir:\n       Your compiler may not be C99-compliant");
        clean_exit(EXIT_FAILURE);
    }

    char d[len+1];
    memset(d, 0, len+1);

    memmove(d, path, len+1);

    if (d == NULL)
    {
        perror("\n"ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET" Error: could not allocate directory path string\n");
        clean_exit(EXIT_FAILURE);
    }


    for (i = 1; i < len; i++)
    {
        if (('/' == d[i]) || ('\\' == d[i]))
        {
#           if defined __WIN32__ || defined __CYGWIN__
              if (d[i-1] == ':') continue;
#           endif

            d[i] = '\0';

            errno = 0;
            if ((MKDIR(d, mode) == -1))
            {
                fprintf(stderr, "Impossible to create directory '%s'\n", d);
                perror("\n"ANSI_COLOR_RED"[ERR]"ANSI_COLOR_RESET"  mkdir ");  // EEXIST error messages are often spurious
                puts(path);
                clean_exit(EXIT_FAILURE);
            }

            d[i] = '/';

        }

    }
    // loop stops before end of string as dirpaths can optionally end in '/' under *nix

    if  (MKDIR(path, mode) == -1)
    {

        printf(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to create directory '%s'\n", path);
        printf("       permission was: %d\n       %s\n", mode, strerror(errno));
        errno=0;
        clean_exit(EXIT_FAILURE);
    }

    return(errno);
}

/* --------
* get_cl
*
* Returns a quoted version of command line starting at 0-based rank of second argument.
* Quotes are not inserted with hypenated options/switches or in presence of quotes or
* of a pipe
* ------- */

char* get_cl(const char** args, uint16_t start)
{
    if (args == NULL) return NULL;
    uint16_t tot = 0, i = start, j, shift = 0;
    uint16_t size[BUFSIZ*10];
    while (args[i])
    {
        size[i] = strlen(args[i]);
        tot += size[i];
        i++;
    }

    char* cml=calloc(tot + i + 2 * i, sizeof(char)); // 2*i for quotes, i for spaces

    if (cml == NULL) perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  get_command_line\n");

    for (j = start; j < i; j++)
    {
        _Bool do_quote=((args[j][0] != '"') && (args[j][0] != '-') && (args[j][0] != '|')) ;
        memcpy(cml + shift, (do_quote)? quote(args[j]): args[j] , size[j] + 2 * do_quote);
        shift += size[j] + 1 + 2 * do_quote;
        cml[shift - 1]=0x20;
    }

    cml[shift - 1]=0;
    if (globals.debugging) printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Command line: %s\n", cml);

    return cml;
}

/* --------
* get_command_line
*
* Returns a quoted version of command line starting at first real option/command/flag.
* Quotes are not inserted with hypenated options/switches or in presence of quotes or
* of a pipe
* ------- */

const char* get_command_line(const char** args)
{
    return get_cl(args, 1);
}

/*--------
* get_full_command_line
*
* Same as above yet also returns application name
* ------- */


char* get_full_command_line(const char** args)
{
    return get_cl(args, 0);
}


/*--------
* starter
*
* Starts computing execution time
* ------- */


void starter(compute_t *timer)
{
// The Mingw compiler does not support getrusage
#   ifndef __MINGW32__
    getrusage(RUSAGE_SELF, timer->start);
    getrusage(RUSAGE_SELF, timer->nothing);
    timer->nothing->ru_utime.tv_sec -= timer->start->ru_utime.tv_sec;
    timer->nothing->ru_stime.tv_sec -= timer->start->ru_stime.tv_sec;

#endif

}

/*--------
* print_commandline
*
* Prints command line
* ------- */

void print_commandline(int argc, char * const argv[])
{
    int i=0;

    if (globals.debugging) printf("%s \n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Running:");

    for (i=0; i < argc ; i++)
        printf("%s ",  argv[i]);
    printf("%s", "\n\n");

}

/*--------
* print_time
*
* Prints time in either format DD Mon. YY, Hour  Minute Second if verbose == TRUE
* or only Hour Minute Second otherwise
* ------- */

char* print_time(int verbose)
{
    char outstr[200];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL)
    {
        perror("localtime");
        exit(EXIT_FAILURE);
    }


    if (verbose)
    {
        if (strftime(outstr, sizeof(outstr), "%d %b %Y, %Hh %Mm %Ss", tmp) == 0)
        {
            printf("%s\n", "strftime returned 0");
            exit(EXIT_FAILURE);
        }
    }

    else if (strftime(outstr, sizeof(outstr), "%Hh %Mm %Ss", tmp) == 0)
    {
        printf("%s\n", "strftime returned 0");
        exit(EXIT_FAILURE);
    }

    if (verbose)
    {
        printf("\nCurrent time is: %s", outstr);
        return NULL;
    }
    else
        return(strdup(outstr));
}

/*--------
* change_directory
*
* Carefully changes directory, reporting errors
* ------- */

void change_directory(const char * filename)
{

    if (chdir(filename) == -1)
    {
        if (errno == ENOTDIR)
            printf(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  %s is not a directory\n", filename);
        else
        {
           if (NULL != filename)
           {
             fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to cd to %s \n.", filename);
             perror(ANSI_COLOR_RED"\n[ERR]");
           }
           else   
             fprintf(stderr, "%s",ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Null path\n.");
           exit(EXIT_FAILURE);
        }
    }
    else if (globals.debugging)
        printf(ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  Current working directory is now %s\n", filename);
}


/*--------
* traverse_directory(path, function, boolean)
*
* Applies function to all regular (not symlink or special) file elements of dir
* Recurses into subdirectories if last element is TRUE
* Note :  function may have 4 optional arguments, the first of a const char*, practically a path or filename
* ------- */

int traverse_directory(const char* src, void (*f)(const char GCC_UNUSED *, void GCC_UNUSED *, void GCC_UNUSED *), _Bool recursive, void GCC_UNUSED* arg2, void GCC_UNUSED* arg3)
{
    DIR *dir_src;

    struct stat buf;
    struct dirent *d;

    printf("%c", '\n');

    if (globals.debugging)  printf("%s%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Traversing dir = ", src);

    change_directory(src);

    dir_src=opendir(".");

    while ((d = readdir(dir_src)) != NULL)
    {
        if (d->d_name[0] == '.') continue;

        if (stat(d->d_name, &buf) == -1)
        {
            perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" stat in traverse_directory\n");
            exit(EXIT_FAILURE);
        }

        if (recursive && S_ISDIR(buf.st_mode))
        {

            errno = traverse_directory(d->d_name, f, recursive, arg2, arg3);

            continue;
        }

        if (S_ISREG(buf.st_mode))
        {
            if (globals.debugging) printf("%s = %s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Processing file=", d->d_name);
            f((const char*) d->d_name, arg2, arg3);
        }

        /* does not copy other types of files(symlink, sockets etc). */

        else continue;
    }

    if (globals.debugging)   printf("%s", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Done. Backtracking... \n\n");
    closedir(dir_src);
    return(errno);
}

/*------- start of private */


void copy_file_wrapper(const char* filename, void* out, void GCC_UNUSED *permissions)
{
    const char* dest = (const char*) out;
    //const mode_t mode = (mode_t) *permissions;
    char path[BUFSIZ] = {0};
    sprintf(path, "%s%c%s", dest, '/', filename);
    copy_file(filename,  path);
}

/*------- end of private */

/*--------
* copy_directory
*
* Copies directory contest to path, with permissions
* Recurses into subdirectories.
* Returns errno.
* ------- */

int copy_directory(const char* src, const char* dest, mode_t mode)
{
    struct stat buf;

    if (stat(dest, &buf) == -1)
    {
        perror("\n"ANSI_COLOR_RED "\n[ERR]" ANSI_COLOR_RESET"  copy_directory could not compute directory size\n");
        exit(EXIT_FAILURE);
    }

    printf("%c", '\n');

    if (globals.debugging)  printf("%s%s\n", ANSI_COLOR_BLUE" [INF] "ANSI_COLOR_RESET"  Creating directory ", dest);

    if (secure_mkdir(dest, mode) == 0)
    {

      if (globals.debugging)   printf(ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET"  Copying in %s ...\n", src);

       traverse_directory(src, copy_file_wrapper, true, (void*) dest, (void*) &mode);
    }
    else
    if (globals.debugging)
        printf("%s%s\n", ANSI_COLOR_BLUE" [INF] "ANSI_COLOR_RESET"  No files copied to ", dest);

    return(errno);
}

/*--------
* file_exists
*
* Tests for file existence
* ------- */

_Bool file_exists(const char* filepath)
{
    return(access(filepath, F_OK) != -1 );
}

void stat_file_wrapper(const char *filename, void *total_size, void GCC_UNUSED *unused)
{
    uint64_t* sum = (uint64_t *) total_size;

    *sum += stat_file_size(filename);

    total_size = (void* ) sum;
}

/* stat_dir_files
 *
 * Computes the non-recursive sum of root-directory file sizes in a given directory.
 * Note : for the whole (recursive) size, taking the stat st_size value would do the trick */

int stat_dir_files(const char* src)
{
    struct stat buf;
    if (stat(src, &buf) == -1)
    {
        perror("\n"ANSI_COLOR_RED "\n[ERR]" ANSI_COLOR_RESET" Directory not recognized.\n");
        exit(EXIT_FAILURE);
    }

    printf("%c", '\n');

    if (globals.debugging)  printf("%s%s\n", ANSI_COLOR_BLUE" [INF] "ANSI_COLOR_RESET" Summing up directory file sizes...", src);

    uint64_t total_size = 0;

    traverse_directory(src, stat_file_wrapper, false, (void*) &total_size, NULL);

    return(errno);
}

#if 0
 // vanilla code

int copy_directory(const char* src, const char* dest, mode_t mode)
{

    DIR *dir_src;

    struct stat buf;
    struct dirent *f;
    char path[BUFSIZ];

    if (globals.nooutput) return 0;

    if (stat(dest, &buf) == -1)
    {
        perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  copy_directory could not compute directory size\n");
        exit(EXIT_FAILURE);
    }

    printf("%c", '\n');

    if (globals.debugging)  printf("%s%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Creating dir=", dest);

    secure_mkdir(dest, mode);

    if (globals.debugging)   printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Copying in %s ...\n", src);
    change_directory(src);

    dir_src=opendir(".");

    while ((f=readdir(dir_src)) != NULL)
    {
        if (f->d_name[0] == '.') continue;
        STRING_WRITE(path, "%s%c%s", dest, '/', f->d_name)
        if (stat(f->d_name, &buf) == -1)
        {
            perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" stat in copy_directory\n");
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(buf.st_mode))
        {

            if (globals.debugging) printf("%s %s %s %s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Copying dir=", f->d_name, " to=", path);

            errno=copy_directory(f->d_name, path, mode);

            continue;
        }
        if (S_ISREG(buf.st_mode))
        {
            if (globals.debugging) printf("%s%s to= %s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Copying file=", f->d_name, path);
            errno=copy_file(f->d_name, path);
        }
        /* does not copy other types of files(symlink, sockets etc). */

        else continue;
    }

    if (globals.debugging)   printf("%s", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Done. Backtracking... \n\n");
    closedir(dir_src);
    return(errno);
}

#endif

int copy_file_no_p(FILE *infile, FILE *outfile)
{


    char buf[BUFSIZ];
    clearerr(infile);
    clearerr(outfile);
    double counter=0;
    size_t chunk=0;

    if (globals.veryverbose) printf("%s","       |");
    errno=0;
    while (!feof(infile))
    {

        chunk=fread(buf, sizeof(char), BUFSIZ, infile);

        if (ferror(infile))
        {
            fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Read error\n");
            return(-1);
        }


        fwrite(buf, chunk* sizeof(char), 1 , outfile);
        counter++;
        if (globals.veryverbose) putchar('-');

        if (ferror(outfile))
        {
            fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Write error\n");
            return(-1);
        }
    }



    if (globals.veryverbose)
    {
        putchar('|');
        counter=counter-1;
        counter=(counter*sizeof(char)*BUFSIZ+chunk)/1024;
        printf("\n"ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Copied %.2lf KB.\n", counter);
        if (!errno) puts("\n"ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Copy completed.");
        else
            puts("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Copy failed.");

    }
    return(errno);
}

// Adapted from Yve Mettier's O'Reilly "C en action" book, chapter 10.

int copy_file(const char *existing_file, const char *new_file)
{

    FILE *fn, *fe;
    int errorlevel;
    if (globals.debugging) fprintf(stderr, "\n"ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  Copying file %s\n", existing_file);
    if (NULL == (fe = fopen(existing_file, "rb")))
    {
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to open file '%s' in read mode\n", existing_file);
        exit(-1);
    }
    if (NULL == (fn = fopen(new_file, "wb")))
    {
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to open file '%s' in write mode\n", new_file);
        fclose(fe);
        exit(-1);
    }


    errorlevel=copy_file_no_p(fe, fn);
    fclose(fe);
    fclose(fn);
    if (globals.debugging && errorlevel == 0)
      fprintf(stderr, ANSI_COLOR_YELLOW"[DBG]"ANSI_COLOR_RESET"  File was copied as: %s\n", new_file);

    return(errorlevel);
}

char* copy_file2dir(const char *existing_file, const char *new_dir)
{
// existence of new_dir is not tested
// existence of dile dest is tested and if exists, duplication generates file counter in filename
// to ensure continuity of counters, copy file operations should come in a "closely-knit loop" without
// copy file operations in between for other files

    static uint32_t counter;
    int errorlevel;
    if (globals.veryverbose) printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Copying file %s to directory %s\n", existing_file, new_dir);

    path_t* filestruct=parse_filepath(existing_file);
    if (!filestruct) return NULL;

    char* dest=calloc(filestruct->length+strlen(new_dir)+1+6+2, sizeof(char)); // 6-digit counter +1 underscore; size is a maximum
    if (dest == NULL) return NULL;
    sprintf(dest, "%s%s%s", new_dir, SEPARATOR, filestruct->filename);
    counter++;
    // overwrite
     sprintf(dest, "%s%s%s_%d%s", new_dir, SEPARATOR, filestruct->rawfilename, counter, filestruct->extension);
    
    errorlevel=copy_file(existing_file, dest);

    free(filestruct);
    
    errno=0;
    if (errorlevel) return NULL;
    else return(dest);

}

char* copy_file2dir_rename(const char *existing_file, const char *new_dir, char* newfilename)
{
// existence of new_dir is not tested
// existence of file dest is tested and if exists, copy overwrites it


    int errorlevel;
    if (globals.veryverbose) printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Copying file %s to directory %s\n", existing_file, new_dir);

    char *dest=calloc(strlen(newfilename)+strlen(new_dir)+1+1, sizeof(char));
    sprintf(dest, "%s%s%s", new_dir, SEPARATOR, newfilename);

    path_t *filedest=parse_filepath(dest);
    if (!filedest) return NULL;


    if (filedest->isfile)
    {
        //overwrite
        unlink(dest);
    }

    errorlevel=copy_file(existing_file, dest);

    free(filedest);
    errno=0;
    if (errorlevel) return NULL;
    else return(dest);

}

int cat_file(const char *existing_file, const char *new_file)
{
    FILE *fn, *fe;
    int errorlevel;

    if (NULL == (fe = fopen(existing_file, "rb")))
    {
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to open file '%s' \n in read mode.\n", existing_file);
        return(-1);
    }
    if (NULL == (fn = fopen(new_file, "ab")))
    {
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Impossible to open file '%s' in append mode.\n", new_file);
        fclose(fe);
        return(-1);
    }


    errorlevel=copy_file_no_p(fe, fn);
    fclose(fe);
    fclose(fn);

    return(errorlevel);
}

int copy_file_p(FILE *infile, FILE *outfile, uint32_t position,uint64_t output_size)
{
    char buf[BUFSIZ];
    clearerr(infile);
    clearerr(outfile);
    size_t chunk=0;
    uint64_t count=0;

    if (fseek(infile, position, SEEK_SET) == -1) return -1;
    end_seek(outfile);
    if (output_size)
    {

        while ((output_size > count)&&(!feof(infile)))
        {

            chunk= ((output_size-count) < BUFSIZ) ? fread(buf, sizeof(char), (size_t) output_size -count, infile) : fread(buf, sizeof(char), BUFSIZ, infile) ;

            count+=chunk*sizeof(char);


            if (ferror(infile))
            {
                fprintf(stderr, "Read error\n");
                return(-1);
            }


            fwrite(buf, chunk* sizeof(char), 1 , outfile);


            if (ferror(outfile))
            {
                fprintf(stderr, "Write error\n");
                return(-1);
            }
        }
        return((count < output_size) ? PAD : NO_PAD);
    }

    while (!feof(infile))
    {

        chunk= fread(buf, sizeof(char), BUFSIZ, infile) ;

        count+=chunk*sizeof(char);


        if (ferror(infile))
        {
            fprintf(stderr, "Read error\n");
            return(-1);
        }


        fwrite(buf, chunk* sizeof(char), 1 , outfile);


        if (ferror(outfile))
        {
            fprintf(stderr, "Write error\n");
            return(-1);
        }
    }

    return(NO_PAD);
}



int get_endianness()
{
    long i=1;
    const char *p=(const char *) &i;
    if (p[0] == 1) return LITTLE_ENDIAN;
    return BIG_ENDIAN;
}


 inline uint8_t read_info_chunk(uint8_t* pt, uint8_t* chunk)
{
    pt+=4;
    /* there may be non-printable characters after I... info chunk labels */
    while (!isprint(*pt)) pt++;

    /* this functin should be called only when it it certain that there will be at least one 0 in the area pointed to by pt within the size of chunk */
    int result=snprintf((char *) chunk, MAX_LIST_SIZE, "%s", pt);
    if (result >0)
        return(1);
    else
        return(0);

}

void parse_wav_header(WaveData* info, infochunk* ichunk)
{
    FILE * infile=info->INFILE;
    uint8_t haystack[MAX_HEADER_SIZE] = {0};
    if (infile == NULL)
    {
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Input file is null pointer.\n");
        exit(-1);
    }
    fseek(infile, 0, SEEK_SET);
    int count = fread(haystack, 1, MAX_HEADER_SIZE, infile);
    if (count != MAX_HEADER_SIZE)
    {
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not read %d characters from input file\n", MAX_HEADER_SIZE);
        fprintf(stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Just read %d\n", count);
        ichunk->span=0;
        return;
    }
    uint8_t *pt=&haystack[0];
    uint8_t span=0;

    fseek(infile, 0, SEEK_SET);
    // PATCH 09.07

    do
    {
        if ((pt=memchr(haystack+span+1, 'f', MAX_HEADER_SIZE-1-span)) != NULL)
        {
          if ((*(pt + 1) == 'a') && (*(pt + 2) == 'c') && (*(pt + 3) == 't')) 
          {
           ichunk->is_extensible=1;   
           break;
          }
          span=pt-haystack;
        }
        else 
        break;

    }
    while ( span < MAX_HEADER_SIZE-7);
    
    span=0;
      
    do
    {
        
        if ((pt=memchr(haystack+span+1, 'd', MAX_HEADER_SIZE-1-span)) == NULL)
        {
            printf(""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Could not find substring 'data' among %d characters\n", MAX_HEADER_SIZE);
            if (globals.debugging)
            {
                hexdump_header(infile, MAX_HEADER_SIZE);
            }
            ichunk->span=0;
            return;
        }

        span=pt-haystack;

        if ((*(pt + 1) == 'a') && (*(pt + 2) == 't') && (*(pt + 3) == 'a')) break;

    }
    while ( span < MAX_HEADER_SIZE-7);

    pt=&haystack[0];
    
    if (span > 36)
    {
        /* header is non-standard, looking for INFO chunks */
        /* The RIFF standard defines the following chunks, of which only INAM, IART, ICMT, ICOP, ICRD, IGNR will be parsed */
        /*
        IARL  Archival Location. Indicates where the subject of the file is archived.
        IART  Artist. Lists the artist of the original subject of the file. For example, "Michaelangelo."
        ICMS  Commissioned. Lists the name of the person or organization that commissioned the subject of the file. For example, "Pope Julian II."
        ICMT  Comments. Provides general comments about the file or the subject of the file. If the comment is several sentences long, end each sentence with a period. Do not include newline characters.
        ICOP  Copyright. Records the copyright information for the file. For example, "Copyright Encyclopedia International 1991." If there are multiple copyrights, separate them by a semicolon followed by space.
        ICRD  Creation date. Specifies the date the subject of the file was created. List dates in year-month-day format, padding one-digit months and days with a zero on the left. For example, "1553-05-03" for May 3, 1553.
        ICRP  Cropped. Describes whether an image has been cropped and, if so, how it was cropped. For example, "lower right corner."
        IDIM  Dimensions. Specifies the size of the original subject of the file. For example, "8.5 in h, 11 in w."
        IDPI  Dots Per Inch. Stores dots per inch setting of the digitizer used to produce the file, such as "300."
        IENG  Engineer. Stores the name of the engineer who worked on the file. If there are multiple engineers, separate the names by a semicolon and a blank. For example, "Smith, John; Adams, Joe."
        IGNR  Genre. Describes the original work, such as, "landscape," "portrait," "still life," etc.
        IKEY  Keywords. Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon and a blank. For example, "Seattle; aerial view; scenery."
        ILGT  Lightness. Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used.
        IMED  Medium. Describes the original subject of the file, such as, "computer image," "drawing," "lithograph," and so forth.
        INAM  Name. Stores the title of the subject of the file, such as, "Seattle From Above."
        IPLT  Palette Setting. Specifies the number of colors requested when digitizing an image, such as "256."
        IPRD  Product. Specifies the name of the title the file was originally intended for, such as "Encyclopedia of Pacific Northwest Geography."
        ISBJ  Subject. Describes the conbittents of the file, such as "Aerial view of Seattle."
        ISFT  Software. Identifies the name of the software package used to create the file, such as "Microsoft WaveEdit."
        ISHP  Sharpness. Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used).
        ISRC  Source. Identifies the name of the person or organization who supplied the original subject of the file. For example, "Trey Research."
        ISRF  Source Form. Identifies the original form of the material that was digitized, such as "slide," "paper," "map," and so forth. This is not necessarily the same as IMED.
        */

        uint8_t infoindex=0;

        do
        {
            if ((pt=memchr(haystack+infoindex+1, 0x49, span-1-infoindex)) == NULL)
            {

                if (globals.debugging)
                {
                    if (ichunk->found)
                        printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Found %d info chunks among %d characters\n       INAM %s\n       IART %s\n       ICMT %s\n       ICOP %s\n       ICRD %s\n       IGNR %s\n",
                               ichunk->found, span, ichunk->INAM, ichunk->IART, ichunk->ICMT, ichunk->ICOP, ichunk->ICRD, ichunk->IGNR);
                    else
                        printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Could not find info chunks among %d characters\n", span);

                }
                break;
            }


            infoindex=pt-haystack;

            if ((*(pt + 1) == 'N') && (*(pt + 2) == 'A') && (*(pt + 3) == 'M'))
                ichunk->found=read_info_chunk(pt, ichunk->INAM);
            else if ((*(pt + 1) == 'A') && (*(pt + 2) == 'R') && (*(pt + 3) == 'T'))
                ichunk->found+=read_info_chunk(pt, ichunk->IART);
            else if ((*(pt + 1) == 'C'))
            {
                if ((*(pt + 2) == 'M') && (*(pt + 3) == 'T'))
                    ichunk->found+=read_info_chunk(pt, ichunk->ICMT);
                else if ((*(pt + 2) == 'O') && (*(pt + 3) == 'P'))
                    ichunk->found+=read_info_chunk(pt, ichunk->ICOP);
                else if ((*(pt + 2) == 'R') && (*(pt + 3) == 'D'))
                    ichunk->found+=read_info_chunk(pt, ichunk->ICRD);
            }
            else if ((*(pt + 1) == 'G') && (*(pt + 2) == 'N') && (*(pt + 3) == 'R'))
                ichunk->found+=read_info_chunk(pt, ichunk->IGNR);

        }
        while (infoindex < span-4);
    }


    if (globals.debugging)
    {
        if (span != 36) printf( ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Size of header is non-standard (scanned %d characters)\n", span );
        else printf("%s", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Size of header is standard\n");

        if (span < 36)
        {
            printf( "[MSF]  Size of header is too short, some audio will be lost (%d bytes).\n", 36-span );
        }
    }

    ichunk->span=span;

    return;
}

/* -------
 * secure_open
 *
 * tries to open file path and allocate file pointer.
 * Exits on failure, otherwise seeks start of file */

void secure_open(const char *path, const char *context, FILE* f)
{
    fclose(f);
    if ( (f=fopen( path, context ))  == NULL )
    {
        printf(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not open '%s'\n", path);
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_SET);
}

/* -------
 * end_seek
 *
 * seeks end of file and returns 0 on success. Verbose.
 * ------- */

int end_seek(FILE *outfile)
{
    if ( fseek(outfile, 0, SEEK_END) == -1)
    {
        printf( "\n%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Error seeking to end of output file" );
        printf( "%s\n", "     File was not changed\n" );
        return(FAIL);
    }
    return(0);
}



/* --------
* hexdump_header
*
* This function displays the first HEADER_SIZE bytes of the file
* in both hexadecimal and ASCII
* -------*/


void hexdump_header(FILE* infile, uint8_t header_size)
{
    unsigned char data[ HEX_COLUMNS ];
    size_t i, size=0, count=0, input=0;
    printf( "%c", '\n' );
    do
    {

        memset(data, 0, HEX_COLUMNS);

        /* Print the base address. */
        printf("%08lX:  ", (long unsigned)count);


        input= Min(header_size -count, HEX_COLUMNS);
        count+=HEX_COLUMNS;


        size = fread(data, 1, input, infile);

        if ( size ==  input)
        {

            /* Print the hex values. */
            for ( i = 0; i < HEX_COLUMNS; i++ )
                printf("%02X ", data[i]);

            /* Print the characters. */
            for ( i = 0; i < HEX_COLUMNS; i++ )
                printf("%c", (i < input)? (isprint(data[i]) ? data[i] : '.') : ' ');

            printf("%c", '\n');
        }
        else
        {
            printf("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Header was not properly read by hexdump_header()");
        }

        /* break on partial buffer */
    }
    while ( count < header_size );


    printf( "%c", '\n' );

}

/* -------
 * hexdump_pointer
 *
 * On at most 16 columns print hexadecimal values of array
 * with base addressed on the left. Number of columns as
 * second argument.
 * ------- */

void hexdump_pointer(uint8_t* tab,  size_t tabsize)
{
    size_t i, count=0, input=0;

    fprintf(stderr, "%c", '\n' );

    do
    {
        /* Print the base address. */

        fprintf(stderr,"%08lX:  ", (long unsigned)count);
        input= Min(tabsize - count, HEX_COLUMNS);

        for (i = 0; i < input; i++)
            fprintf(stderr, "%02X ", tab[i+count]);

        count += HEX_COLUMNS;

        /* Print the characters. */

        for (i = 0; i < HEX_COLUMNS; i++)
            fprintf(stderr,"%c", (i < input)? (isprint(tab[i]) ? tab[i] : '.') : ' ');

        fprintf(stderr, "%c", '\n');

        /* break on partial buffer */
    }
    while (count < tabsize);

    fprintf(stderr, "%c", '\n' );
}

void fread_endian(uint32_t * p, int t, FILE *f)
{
    /*  CPU_IS_LITTLE_ENDIAN or CPU_IS_BIG_ENDIAN are defined by  configure script */

#   if !defined    CPU_IS_LITTLE_ENDIAN    &&  !defined    CPU_IS_BIG_ENDIAN

    /* it is necessary to test endianness here */
    static char u;
    static _Bool little;

    /* testing just on first entry */

    if  (!u)   little=(get_endianness() == LITTLE_ENDIAN) ;

    /* fread fills in MSB first so shift one byte for each  one-byte scan  */

    if (little)
    {
        uint8_t c = fread(p + t, 4, 1, f) ;

        if (c < 4) return;

        p[t]= (p[t] << 8  &  0xFF0000)  |   (p[t]<<16 & 0xFF00)  |   (p[t]<<24 & 0xFF) |  (p[t] & 0xFF000000);
    }
    else
    {
        /*Big endian  case*/

        uint8_t c = fread(p+t, 1 ,4,  f) ;

        if (c < 4) return;
    }
    fflush(f);

#   elif   defined CPU_IS_BIG_ENDIAN

    fread(p+t, 1 ,4,  f) ;

#   elif   defined CPU_IS_LITTLE_ENDIAN

    fread(p+t, 4 ,1,  f) ;
    p[t]= (p[t] << 8  &  0xFF0000)  |   (p[t]<<16 & 0xFF00)  |   (p[t]<<24 & 0xFF) |  (p[t] & 0xFF000000);

#   endif

    return;
}

/*--------
* win32quote
*
* Returns a quoted version of argument exclusively on the WInodws platform.
* Otherwise returns argument.
* ------- */

char* win32quote(const char* path)
{
// comment: this function leaks some memory if path has been allocated on heap. Yet using free(path) could
// yield segfaults, should path not be allocated with malloc() or strdup() but using static arrays. Preferring safety here.
#   if defined (__WIN32__) || defined (_WIN32) || defined (_WIN64) || defined (__WIN32) || defined (__WIN64) || defined(__MSYS__)

    if (!path) return NULL;
    int size=strlen(path)+2+1;
    char buff[size];
    strcpy(buff+1, path);
    buff[0]='"';
    buff[size-1]=0;
    buff[size-2]='"';

    char* result=strdup(buff);

    if (result) return (result);
    else
    {
        printf(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not allocate quoted string for %s.\n", path);
        return NULL;
    }

#else

    return (char*) path;

#endif
}


char* quote(const char* path)
{
// comment: this function leaks some memory if path has been allocated on heap. Yet using free(path) could
// yield segfaults, should path not be allocated with malloc() or strdup() but using static arrays. Preferring safety here.
    if (!path) return NULL;
    int size=strlen(path)+2+1;
    char buff[size];
    strcpy(buff+1, path);
    buff[0]='"';
    buff[size-1]=0;
    buff[size-2]='"';
    char* result=strdup(buff);
    if (result) return (result);
    else
    {
        printf(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not allocate quoted string for %s.\n", path);
        return NULL;
    }
}


// Launches an application in a fork and duplicates its stdout into stdout; waits for it to return;


int run(const char* application, char*  args[], const int option)
{
errno=0;
#if !defined __WIN32__
    int pid;
    int tube[2];
    char c;

    char msg[strlen(application)+1+7];
    memset(msg, '0', sizeof(msg));

    if (pipe(tube))
    {
        perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  pipe run\n");
        return errno;
    }

    switch (pid = fork())
    {
    case -1:
        foutput("%s%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not launch ", application);
        break;
    case 0:
        close(tube[0]);
        dup2(tube[1], STDERR_FILENO);
        execv(application, args);
        foutput("%s%s%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Runtime failure in ", application," child process");
        perror("");
        return errno;

    default:
        close(tube[1]);
        dup2(tube[0], STDIN_FILENO);
        while (read(tube[0], &c, 1) == 1) foutput("%c", c);
        if (option != NOWAIT) waitpid(pid, NULL, option);
        close(tube[0]);

    }
#else
    char* s=get_command_line(args);
    char cml[strlen(application)+1+strlen(s)+1+2];
    sprintf(cml, "\"%s\" %s",  application, s);
    free(s);
    if (globals.debugging) foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Running: %s\n ", cml);
    errno=system(cml);
#endif

    return errno;
}

  uint64_t parse_file_for_sequence(FILE* fp, uint8_t* tab, size_t sizeoftab)
{

    if (fp == NULL)
    {

        return -1;
    }
    size_t i=0;
    int c;
    while (i < sizeoftab)
    {
        if ((c=fgetc(fp)) == EOF) break;
        else
        {
            if   (c == tab[i])
                i++;
            else
                i=0;
        }
    }
    long  fileoffset;

    if (i == sizeoftab)
    {
        if ((size_t) (fileoffset=ftell(fp)) < sizeoftab*8)
            return -1; // error
    }
    else fileoffset=0; // not found

    return (uint64_t) fileoffset;

}
