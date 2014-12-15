#ifndef WINPORT_H_INCLUDED
#define WINPORT_H_INCLUDED

#if HAVE_CONFIG_H && !defined __CB__
#include "config.h"
#endif
#include "c_utils.h"
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#ifndef MKDIR
#ifdef __WIN32__
#include <tchar.h>
#include <windows.h>
#include <sys/stat.h>
#include <string.h>
#endif

#ifndef __MINGW32__
#include <sys/resource.h>
#endif
#else
#include <sys/stat.h>
#endif


#if defined __WIN32__

 inline static uint64_t stat_file_size(const char* filename)
{
     HANDLE hFile;

     hFile = CreateFile(filename,                // name of the write
                       GENERIC_WRITE,          // open for writing
                       0,                      // do not share
                       NULL,                   // default security
                       OPEN_EXISTING,         // existing file only
                       FILE_ATTRIBUTE_NORMAL,  // normal file
                       NULL);                  // no attr. template
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        return(0);
    }

    BOOL result=0;

    LARGE_INTEGER move, distance;
    PLARGE_INTEGER Distance=&distance;  // signed 64-bit type (*int64_t)

    move.QuadPart=0;
    result=SetFilePointerEx(hFile, move, Distance, FILE_END );

    CloseHandle(hFile);
    if (!result)
    {
            return(0);
    }
    return((uint64_t) distance.QuadPart);
}


int truncate_from_end(TCHAR*filename, uint64_t offset);
uint64_t read_file_size(FILE* fp, TCHAR* filename);

#else

 inline static uint64_t stat_file_size(const char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0) return(0);
    return((uint64_t) buf.st_size);
}
uint64_t read_file_size(FILE * fp, const char* filename);
int truncate_from_end(char* filename, uint64_t offset);
#endif

#endif // WINPORT_H_INCLUDED
