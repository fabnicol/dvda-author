/*

File:   winport.c
Purpose: windows port function

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include "winport.h"
#include "private_c_utils.h"
#include "c_utils.h"
#include "structures.h"

extern globalData global_utils ;

/* met issues on GNU/Linux with stat st_size field ( a few bytes off real size. USe below code from
   http://www.securecoding.cert.org  */

#include <unistd.h>


uint64_t read_file_size(FILE * fp, const char* filename)
{
    if (filename == NULL) return 0;
    uint64_t size = 0;

    if (fp)
    {
        /* get size */
            size=stat_file_size(filename);
    }


    return size;
}

int truncate_from_end(char* filename, uint64_t offset)
{
    return truncate(filename, offset);
}

# ifdef _WIN32

void kill(const char* p)
{
    char* t;
    calloc(t, 17 + strlen(p));
    sprintf(t, "%s%s", "taskkill /im /f ", p);
    system(t);
    free(t);
}

// CHAR* name: name of the child
// CHAR** args: command line args of the child
// Open a pipe to feed child's stdin with bytes later to be written
// + child's HANDLE
// HANDLE g_hChildStd_IN_Rd: child stdin (read)
// HANDLE g_hChildStd_IN_Wr: child stdin (write)
// HANDLE g_hChildStd_ERR_Rd: child stderr (read)
// HANDLE g_hChildStd_ERR_Wr: child stderr (write)
// returns number of bytes written

DWORDLONG pipe_to_child_stdin(CHAR* name,
                          CHAR** args,
                          HANDLE g_hChildStd_IN_Rd,
                          HANDLE g_hChildStd_IN_Wr,
                          HANDLE g_hChildStd_ERR_Rd,
                          HANDLE g_hChildStd_ERR_Wr)
{
   SECURITY_ATTRIBUTES saAttr;
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   if ( ! CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0) )
      ErrorExit(TEXT("StdERRRd CreatePipe"));

   if ( ! SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("StdERR SetHandleInformation"));

   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
      ErrorExit(TEXT("Stdin CreatePipe"));

   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdin SetHandleInformation"));

   TCHAR szCmdline[]=TEXT(name); //+args
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE;

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = g_hChildStd_ERR_Wr;
   siStartInfo.hStdOutput = g_hChildStd_ERR_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

   bSuccess = CreateProcess(NULL,
      szCmdline,
      NULL,
      NULL,
      TRUE,
      0,
      NULL,
      NULL,
      &siStartInfo,
      &piProcInfo);

   if ( ! bSuccess )
      ErrorExit(TEXT("CreateProcess"));
   else
   {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
   }

   DWORD dwRead, dwWritten;
   CHAR chBuf[BUFSIZE];
   BOOL bSuccess = FALSE;
   HANDLE hParentStdErr = GetStdHandle(STD_ERROR_HANDLE);
   DWORDLONG   dwlWrittentBytes;

   for (; dwlWrittentBytes < MAXUINT64;)
   {
      bSuccess = ReadFile( g_hChildStd_ERR_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      if( ! bSuccess) break;

      bSuccess = WriteFile(hParentStdErr, chBuf,
                           dwRead, &dwWritten, NULL);
      if (! bSuccess ) break;
   }

   dwlWrittentBytes += dwWritten;

   return dwlWrittentBytes;
   // Now write to the child stdin pipe as in follwing function

}

// Writes to child stdin a buffer chBuf of length dwBytesToBeWritten
// Same handles as above
// returns number of bytes written

DWORD write_to_child_stdin(
      void* chBuf,
      DWORD dwBytesToBeWritten,
      HANDLE g_hChildStd_IN_Rd,
      HANDLE g_hChildStd_IN_Wr,
      HANDLE g_hChildStd_ERR_Rd,
      HANDLE g_hChildStd_ERR_Wr)
{
    DWORD dwRead, dwWritten;

    BOOL bSuccess = FALSE;

    bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwBytesToBeWritten, &dwWritten, NULL);

    if (globals.debugging)
    {
       if (! bSuccess)  fprintf(stderr, "%s\n", ERR "Error in write process to stdin.");
       if (globals.maxverbose) fprintf(stderr, "%s%d%s%d\n", MSG_TAG "Wrote ", dwWritten, " bytes out of ", dwBytesToBeWritten);
    }

    if ( ! CloseHandle(g_hChildStd_IN_Wr) )
       ErrorExit(TEXT("StdInWr CloseHandle"));

    return dwWritten;
}

// Microsoft website boilerplate, public domain.

void ErrorExit(PTSTR lpszFunction)
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);

    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}


# endif // _WIN32



