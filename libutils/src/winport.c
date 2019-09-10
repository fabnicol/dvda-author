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

#include "winport.h"
#include "private_c_utils.h"
#include "c_utils.h"
#include "structures.h"

extern globalData globals;

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

int  pkill(const char* p)
{
    char* t;
    t = calloc(17 + strlen(p), sizeof(char));
    sprintf(t, "%s%s", "taskkill /im /f ", p);
    int res = system(t);
    free(t);
    return res;
}

int kill(PROCESS_INFORMATION pi)
{
    TerminateProcess(pi.hProcess, 255);
}

// CHAR* name: name of the child
// CHAR** args: command line args of the child
// Open a pipe to feed child's stdin with bytes later to be written
// buffer_size: tehcnical (size of buffer)
// + child's HANDLE
// HANDLE g_hChildStd_IN_Rd: child stdin (read)
// HANDLE g_hChildStd_IN_Wr: child stdin (write)
// HANDLE g_hChildStd_ERR_Rd: child stderr (read)
// HANDLE g_hChildStd_ERR_Wr: child stderr (write)
// returns number of bytes written

void  pipe_to_child_stdin(const char* name,
                          char **args,
                          int GCC_UNUSED  buffer_size,
                          HANDLE *g_hChildStd_IN_Rd,
                          HANDLE *g_hChildStd_IN_Wr,
                          HANDLE *g_hChildStd_ERR_Rd,
                          HANDLE *g_hChildStd_ERR_Wr,
                          HANDLE *hParentStdErr,
                          PROCESS_INFORMATION *piProcInfo,
                          STARTUPINFO *siStartInfo)
{
   SECURITY_ATTRIBUTES saAttr;
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   if ( ! CreatePipe(g_hChildStd_ERR_Rd, g_hChildStd_ERR_Wr, &saAttr, 0) )
      ErrorExit(TEXT("StdERRRd CreatePipe"));

   if ( ! SetHandleInformation(*g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("StdERR SetHandleInformation"));

   if (! CreatePipe(g_hChildStd_IN_Rd, g_hChildStd_IN_Wr, &saAttr, 0))
      ErrorExit(TEXT("Stdin CreatePipe"));

   if ( ! SetHandleInformation(*g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdin SetHandleInformation"));

   *hParentStdErr = GetStdHandle(STD_ERROR_HANDLE);

   BOOL bSuccess = FALSE;

   ZeroMemory(piProcInfo, sizeof(PROCESS_INFORMATION) );
   ZeroMemory(siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo->cb =  sizeof(STARTUPINFO);
   siStartInfo->hStdError = *g_hChildStd_ERR_Wr;
   siStartInfo->hStdOutput = *g_hChildStd_ERR_Wr;
   siStartInfo->hStdInput = *g_hChildStd_IN_Rd;
   siStartInfo->dwFlags |= STARTF_USESTDHANDLES;

   char* cli = get_command_line(args);
   char* commandline = join(name, cli, " ");
   free(cli);

   bSuccess = CreateProcessA(
      name,
      commandline,
      NULL,
      NULL,
      TRUE,
      NORMAL_PRIORITY_CLASS ,
      NULL,
      NULL,
      siStartInfo,
      piProcInfo);

   if ( ! bSuccess )
      ErrorExit(TEXT("CreateProcess"));
   else
   {
      //CloseHandle(piProcInfo->hProcess);
      //CloseHandle(piProcInfo->hThread);
   }

   free(commandline);

   return ;
   // Now write to the child stdin pipe as in follpwing function
}

// Writes to child stdin a buffer chBuf of length dwBytesToBeWritten
// Same handles as above

DWORD write_to_child_stdin(
      void* chBuf,
      DWORD dwBytesToBeWritten,
      HANDLE g_hChildStd_IN_Wr)
{
    DWORD dwWritten;

    BOOL bSuccess = FALSE;

     // Set "stdin" to have binary mode:

     fflush(stdin);
     int result = _setmode( _fileno( stdin ), _O_BINARY );

     if  ( result == -1 )
        {
           perror( ERR "Cannot set mode" );
           clean_exit(EXIT_FAILURE);
        }
     else
     if ( globals.veryverbose)
          foutput("%s", MSG_TAG "'stdout successfully changed to binary mode\n" );

    bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwBytesToBeWritten, &dwWritten, NULL);

    if (globals.debugging)
    {
       if (! bSuccess)  fprintf(stderr, "%s\n", ERR "Error in write process to stdin.");
       if (globals.maxverbose) fprintf(stderr, "%s%lu%s%lu\n", MSG_TAG "Wrote ", dwWritten, " bytes out of ", dwBytesToBeWritten);
    }

    return dwWritten;
}

DWORDLONG pipe_to_parent_stderr(FILE_DESCRIPTOR GCC_UNUSED g_hChildStd_ERR_Rd,
                                                                  FILE_DESCRIPTOR GCC_UNUSED hParentStdErr,
                                                                  int GCC_UNUSED buffer_size)
{
   DWORD dwRead, dwWritten;
   CHAR chBuf[buffer_size];

   BOOL bSuccess = FALSE;

   bSuccess = ReadFile( g_hChildStd_ERR_Rd, chBuf, buffer_size, &dwRead, NULL);

   if (bSuccess)
    bSuccess = WriteFile(hParentStdErr, chBuf,  dwRead, &dwWritten, NULL);

    return dwWritten ;
}

void close_handles(HANDLE g_hChildStd_IN_Rd,
      HANDLE g_hChildStd_IN_Wr,
      HANDLE g_hChildStd_ERR_Rd,
      HANDLE g_hChildStd_ERR_Wr,
      HANDLE hParentStdErr)
{

    if ( ! CloseHandle(g_hChildStd_IN_Wr) )
       ErrorExit(TEXT("StdInWr CloseHandle"));
    if ( ! CloseHandle(g_hChildStd_IN_Rd) )
       ErrorExit(TEXT("StdInRd CloseHandle"));
    if ( ! CloseHandle(g_hChildStd_ERR_Wr) )
       ErrorExit(TEXT("StdErrWr CloseHandle"));
    if ( ! CloseHandle(g_hChildStd_ERR_Rd) )
       ErrorExit(TEXT("StdErrRd CloseHandle"));

   if ( ! CloseHandle(hParentStdErr) )
       ErrorExit(TEXT("Parent stderr CloseHandle"));
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
#else

// name: name of the child
// args: command line args of the child
// Open a pipe to feed child's stdin with bytes later to be writte

// for compatibility with windows, returns long long unsigned int

void pipe_to_child_stdin(const char* name,
                          char** args,
                          int GCC_UNUSED  buffer_size,
                          FILE_DESCRIPTOR g_hChildStd_IN_Rd,
                          FILE_DESCRIPTOR g_hChildStd_IN_Wr,
                          FILE_DESCRIPTOR g_hChildStd_ERR_Rd,
                          FILE_DESCRIPTOR g_hChildStd_ERR_Wr,
                          FILE_DESCRIPTOR hParentStdErr );  // &STDERR_FILENO
{
       char tube[2] = {*tube0, *tube1};
       char tubeerr[2] = {*tubeerr0, *tubeerr1};

        if (pipe(tube) || pipe(tubeerr))
        {
            perror(ERR "Pipe");
            return errno;
        }

        switch (pid = fork())
        {
            case -1:
                fprintf(stderr,"%s\n", ERR "Could not launch ffplay");
                break;

            case 0:
                close(tube[1]);
                close(tubeerr[0]);
                dup2(tube[0], STDIN_FILENO);
                dup2(tubeerr[1], *hParentStdErr);
                execv(ffplay, (char* const*) argsffplay);
                fprintf(stderr, "%s\n", ERR "Runtime failure in ffplay child process");
                perror("");
                return errno;

            default:
                close(tube[0]);
                close(tubeerr[1]);
                dup2(tube[1], STDOUT_FILENO);
                //dup2(tubeerr[0], *hParentStdErr);
        }

        return;
 }

void close_handles(
        int* tube0,
        int* tube1,
        int* tubeerr0,
        int* tubeerr1,
        int* GCC_UNUSED parent_stderr)
{
      close(*tube1]);
      close(*tube0]);
      close(*tubeerr1);
      close(*tubeerr0);
}

unsigned long int write_to_child_stdin(
     void* chBuf,
     unsigned long int dwBytesToBeWritten,
     int* GCC_UNUSED g_hChildStd_IN_Wr)
{
    unsigned long int dwWritten;
    errno = 0;
    dwWritten = fwrite(chBuf, 1, dwBytesToBeWritten,  stdout);

    if (globals.debugging)
    {
       if (errno != 0)  fprintf(stderr, "%s\n", ERR "Error in write process to stdin.");
       if (globals.maxverbose) fprintf(stderr, "%s%lu%s%lu\n", MSG_TAG "Wrote ", dwWritten, " bytes out of ", dwBytesToBeWritten);
    }
    return dwWritten;
}

unsigned long int  pipe_to_parent_stderr(FILE_DESCRIPTOR GCC_UNUSED g_hChildStd_ERR_Rd,
                                         FILE_DESCRIPTOR GCC_UNUSED hParentStdErr,
                                         int GCC_UNUSED buffer_size) {}  // no opt, only there for Windows compatibility

# endif // _WIN32



