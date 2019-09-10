#ifndef WINPORT_H_INCLUDED
#define WINPORT_H_INCLUDED

#if defined HAVE_CONFIG_H && !defined __CB__
#include "config.h"
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#if defined __WIN32__|| defined _WIN64 || defined _WIN32 || defined __WIN32 || defined __WIN64

#  include <tchar.h>
#  include <windows.h>

// define before strsafe include
#define STRSAFE_NO_DEPRECATE
#  include <strsafe.h>

  void ErrorExit(PTSTR);

#else
#   include <sys/resource.h>
#   include <unistd.h>
#endif

#include "c_utils.h"

 inline static uint64_t stat_file_size(const char* filename)
{

    off_t file_size;

    int fd;
    FILE* fp;
    struct stat st;

    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        close(fd);
        fprintf(stderr, "[ERR]" "   Impossible to open file %s for checking size.\n", filename);
        perror("       ");
        return 0;
    }

    fp = fdopen(fd, "r");

    if (fp == NULL)
    {
        close(fd);
        fprintf(stderr, "[ERR]" "   Impossible to fdopen file %s for checking size.\n", filename);
        perror("       ");
        return 0;
    }

    /* Ensure that the file is a regular file */

    if ((fstat(fd, &st) != 0) || (!S_ISREG(st.st_mode)))
    {
        close(fd);
        fprintf(stderr, "[ERR]" "   Impossible to fstat file %s for checking size.\n", filename);
        perror("       ");
        return 0;

    }

    if (fseeko(fp, 0 , SEEK_END) != 0)
    {
        close(fd);
        fprintf(stderr, "[ERR]" "   Impossible to fseeko file %s for checking size.\n", filename);
        perror("       ");
        return 0;
    }

    file_size = ftello(fp);

    fclose(fp);

    if (file_size == -1)
    {
      perror(ANSI_COLOR_RED "[ERR]");
      return 0;
    }

    return (uint64_t) file_size;
}

uint64_t read_file_size(FILE * fp, const char* filename);
int truncate_from_end(char* filename, uint64_t offset);
#endif


#ifndef S_OPEN
#  define S_OPEN(X, Y) do {  if (file_exists(X.filename) && ! X.isopen)  {  if (! X.filesize) { X.filesize =  stat_file_size(X.filename); } ;  X.fp = fopen(X.filename, Y);  X.isopen = (X.fp != NULL); } } while(0);

#  define S_CLOSE(X) do { if (! globals.pipe && X.isopen && X.fp != NULL) { fclose(X.fp); X.isopen = false; X.fp = NULL; } } while(0);

#ifdef _WIN32


  typedef      HANDLE  FILE_DESCRIPTOR;

  int pkill(const char* p);
  int kill(PROCESS_INFORMATION pi);

void ErrorExit(PTSTR lpszFunction);

#else
     typedef      int  FILE_DESCRIPTOR;
#   ifndef  DWORDLONG
#      define DWORDLONG   uint64_t
#   endif
#   ifndef  DWORD
#      define DWORD unsigned long int
#   endif
#   ifndef PROCESS_INFORMATION
#      define PROCESS_INFORMATION void*
#   endif
#   ifndef STARTUPINFO
#      define STARTUPINFO void*
#   endif
#endif

 void  pipe_to_child_stdin(const char* name,
                          char** args,
                          int GCC_UNUSED  buffer_size,
                          FILE_DESCRIPTOR *g_hChildStd_IN_Rd,
                          FILE_DESCRIPTOR *g_hChildStd_IN_Wr,
                          FILE_DESCRIPTOR *g_hChildStd_ERR_Rd,
                          FILE_DESCRIPTOR *g_hChildStd_ERR_Wr,
                          FILE_DESCRIPTOR *hParentStdErr,
                          PROCESS_INFORMATION *piProcInfo,
                          STARTUPINFO *siStartInfo);

 DWORD write_to_child_stdin(
      uint8_t* chBuf,
      DWORD dwBytesToBeWritten,
      FILE_DESCRIPTOR g_hChildStd_IN_Wr);

DWORD pipe_to_parent_stderr(FILE_DESCRIPTOR GCC_UNUSED  g_hChildStd_ERR_Rd,
                                FILE_DESCRIPTOR GCC_UNUSED  hParentStdErr,
                                int GCC_UNUSED buffer_size);


#endif // WINPORT_H_INCLUDED
