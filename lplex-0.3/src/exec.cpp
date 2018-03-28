/*
	exec.cpp - functions to launch external processes.
	Copyright (C) 2006-2011 Bahman Negahban

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2 of the License, or (at your
	option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
	Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation,
	Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/



#include "util.h"


#if defined( lplex_wxasync )

#if defined( lplex_console ) && defined( lplex_win32 )

void win32getError()
{
	LPVOID sysMsg;

	::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, ::GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&sysMsg, 0, NULL );

	ERR( "[win32 SDK " << ::GetLastError() << "] " << (char*)sysMsg );
	::LocalFree( sysMsg );
}

long win32getExitCode( long pid )
{
	DWORD exitCode = 0;
	if( ! ::GetExitCodeProcess(
			::OpenProcess( PROCESS_ALL_ACCESS,FALSE, pid ), &exitCode ) )
		win32getError();
	return (long)exitCode;
}

#else


class _wxProcess : public wxProcess
{
public:
	int status;
	bool isRunning;
	_wxProcess( int flags ) :
		wxProcess( flags ), status(123), isRunning(true) {}
	// warning: somehow wxwidgets never calls OnTerminate() in console mode.
	virtual void OnTerminate(int pid, int state)
	{
		status = state;
		isRunning = false;
	}
};

#endif


											// wxEXEC_ASYNC is supported in win32 console
											// and both win32 and unix guis


// ----------------------------------------------------------------------------
//    execute : using wxEXEC_ASYNC
// ----------------------------------------------------------------------------
//    Executes given command as asynchronous process, then monitors its stderr
//    output.
//
//    Arguments:
//       <command>   - command to execute
//       <verbose>   - whether to pass intercepted messages to stderr
//       <filter>    - function pointer to handler for intercepted messages
//
//    Returns 0 on success, -1 on fail
// ----------------------------------------------------------------------------


long execute( const wxString& command, int verbose,
	int (*filter)( const char *, bool ) )
{
#ifdef _ERR2LOG
	if( verbose > -1 )
		xlog << STAT_TAG << "Running command : " << command << endl;
	else
		xlog.close();
#endif

#if defined( lplex_console )
	#define wxEXEC_FLAGS wxEXEC_ASYNC | wxEXEC_NODISABLE | wxEXEC_NOHIDE
#else
	#define wxEXEC_FLAGS wxEXEC_ASYNC
#endif

	char c, line[512];
	int p = 0;
	long pid;

#if defined( lplex_console ) && defined( lplex_win32 )

	// Workaround: In console mode the wxExecute/wxProcess method never calls
	// wxProcess::Onterminate(), and trying ::GetExitCodeProcess() always
	// returns STILL_ACTIVE; but if the command is explicitly launched with
	// "cmd /c", then ::GetExitCodeProcess() gives a valid result afterwards.
	// There's an added wrinkle in XP: cmd strips the first and last
	// characters if they're quotes, so the entire command string needs to be
	// wrapped in a sacrificial set of quotes so as to preserve any necessary
	// initial quote.

	wxProcess *process = wxProcess::Open( _f("cmd /c \"") + command + "\"", wxEXEC_FLAGS );
	if( ! process )
#else
	_wxProcess *process = new _wxProcess( wxPROCESS_REDIRECT );
	if( ( pid = wxExecute( command, wxEXEC_FLAGS, process ) ) < 1 )
#endif
	{
		ERR( "Can't open process.\n" );
		return -1;
	}

	wxInputStream *pstderr = process->GetErrorStream();

#if defined( lplex_console )
	while( ( c = pstderr->GetC() ) && pstderr->LastRead() )
	{
#else
	while( process->isRunning )
	{
		wxYieldIfNeeded();
		while( pstderr->CanRead() )
		{
			c = pstderr->GetC();
#endif

#ifdef _ERR2LOG
			if( c == '\r' )
			{
				line[p++] = '\0';
				if( filter )
					filter( line, true );
#ifdef lplex_win32
				if( pstderr->Peek() != '\n' )
#endif
					p = 0;
			}
			else if( c == '\n' )
			{
				line[p++] = '\0';
				if( filter )
					filter( line, true );
				xlog << line << c;
				p = 0;
			}
			else
				line[p++] = c;
#endif

			if( verbose < 1 )
			{
				if( ! filter ) blip();
			}
			else
			{
				cerr << c;
			}
#if ! defined( lplex_console )
		}
#endif
	}

#if defined( lplex_console ) && defined( lplex_win32 )
	long exitStatus = win32getExitCode( process->GetPid() );
#else
	long exitStatus = process->status;
#endif

	if( verbose > -1 )
	{
		if( exitStatus )
			ERRv("Process exit code=" << exitStatus << "\n" );
		else
			INFO("Process exit code=" << exitStatus << "\n" );
	}
#ifdef _ERR2LOG
	else
		logReopen();
#endif

	delete process;
	return exitStatus;
}


#elif ! defined( no_popen )

											// wxwidgets doesn't support wxEXEC_ASYNC
											// in unix console, so use popen() instead...


// ----------------------------------------------------------------------------
//    execute : using popen()
// ----------------------------------------------------------------------------
//    Executes given command as an asynchronous process using popen(), then
//    monitors its stderr output.
//
//    Arguments:
//       <command>   - command to execute
//       <verbose>   - whether to pass intercepted messages to stderr
//       <filter>    - function pointer to handler for intercepted messages
//
//    Returns process exit code
// ----------------------------------------------------------------------------



long execute( const wxString& command, int verbose,
	int (*filter)( const char *, bool ) )
{
#ifdef _ERR2LOG
	if( verbose > -1 )
		xlog << STAT_TAG << "Running command : " << command << endl;
	else
		xlog.close();
#endif

	char c, line[512];
	int p = 0, exitStatus;
	FILE *fp;

	wxString cmd = command + " 2>&1 1>/dev/null";
	cmd.Replace("|", " 2>/dev/null |" );
	fp = popen( cmd, "r" );
//   fp = popen( _f( "%s 2>&1 1>/dev/null", cmd.mb_str() ), "r" );

	if( fp == NULL )
		return -1;

	while( ( c = getc( fp ) ) != EOF )
	{
#ifdef _ERR2LOG
			if( c == '\r' )
			{
				line[p++] = '\0';
				if( filter )
					filter( line, true );
				p = 0;
			}
			else if( c == '\n' )
			{
				line[p++] = '\0';
				if( filter )
					filter( line, true );
				xlog << line << c;
				p = 0;
			}
			else
				line[p++] = c;
#endif

			if( verbose < 1 )
			{
				if( ! filter ) blip();
			}
			else
			{
				cerr << c << flush;
			}
	}

//   fflush( fp );
	exitStatus = pclose( fp );

//   if (exitStatus == -1) {
//      /* Error reported by pclose() */
//   } else {
//      /* Use macros described under wait() to inspect `status' in order
//         to determine success/failure of command executed by popen() */
//   }

	if( verbose > -1 )
	{
		if( exitStatus )
			ERRv("Process exit code=" << exitStatus << "\n" );
		else
			INFO("Process exit code=" << exitStatus << "\n" );
	}

#ifdef _ERR2LOG
	else
		logReopen();
#endif

	return exitStatus;
}


#else
											// last resort: faking async using 'script'.
											// hopefully never necessary...

// ----------------------------------------------------------------------------
//    execute : using 'script'
// ----------------------------------------------------------------------------
//    Executes given command as a synchronous process using bash 'script'
//    command to simultaneously display and capture screen output.
//
//    Arguments:
//       <command>   - command to execute
//       <verbose>   - whether to pass intercepted messages to stderr
//       <filter>    - function pointer to handler for intercepted messages
//
//    Returns process exit code
// ----------------------------------------------------------------------------



long execute( const wxString& command, int verbose,
	int (*filter)( const char *, bool ) )
{
											// (unescaped quotes-within-quotes sometimes
											// trip up the bash "script" command)
	wxString cmd = command;
	cmd.Replace( "\"", "\\\"");

#ifdef _ERR2LOG
	if( verbose != -1 )
		xlog << STAT_TAG << "Running command : " << command << endl;
#endif

	wxTextFile scriptFile;
	wxString script = _wxGetTempDir() + "lplex_syncscript.sh";
	wxString output = _wxGetTempDir() + "lplex_syncerr.txt";
	scriptFile.Create( script );

	scriptFile.AddLine( "#!/bin/sh" );
	scriptFile.AddLine( _f( "rm " ) + QUOTE( output ) + " >/dev/null 2>&1" );
	if( verbose )
		scriptFile.AddLine( _f( "script -q -f -c " ) + QUOTE( cmd /*command*/ )
			+ " " + QUOTE( output ) );
	else
		scriptFile.AddLine( command + ">" + QUOTE( output ) + " 2>&1" );

	scriptFile.Write();
	scriptFile.Close();

	long r = wxExecute( _f( "bash " ) + QUOTE( script ),
		wxEXEC_SYNC | wxEXEC_NODISABLE | wxEXEC_NOHIDE );

#ifdef _ERR2LOG
	scriptFile.Open( output );
	int lineCt = scriptFile.GetLineCount()/* - ( verbose ? 1 : 0 )*/;

	for( int i = ( verbose < 1 ? 0 : 1 ); i < lineCt; i++ )
	{
		wxString line = scriptFile.GetLine( i );
		if( filter && filter( line, false ) )
			continue;
		else
			xlog << line << endl;
	}

	scriptFile.Close();
#endif

	return r;
}

#endif




// ----------------------------------------------------------------------------
//    executeViaScript :
// ----------------------------------------------------------------------------
//    Wraps given command inside a shell script which it then executes as
//    a monitored asynchronous process.
//
//    Arguments:
//       <command>   - command to execute
//       <verbose>   - whether to pass intercepted messages to stderr
//       <filter>    - function pointer to handler for intercepted messages
//
//    Returns 0 on success, -1 on fail
// ----------------------------------------------------------------------------


long executeViaScript(const wxString& command, int verbose,
	int (*filter)( const char *, bool ) )
{
	long r;

#ifdef _ERR2LOG
	if( verbose > -1 )
		xlog << STAT_TAG << "Running command : "<< command << endl;
	else
		xlog.close();
#endif

	wxTextFile scriptFile;
//   wxString script = _wxGetTempDir() + "script." + SCRIPT_EXT;
	wxString script = tempDir + "script." + SCRIPT_EXT;

	scriptFile.Create( script );
	scriptFile.AddLine( command );
	scriptFile.Write();
	scriptFile.Close();

	r = execute( _f( SCRIPT_CMD ) + QUOTE( script ),
		verbose > 0 ? verbose : -1, filter );

#ifdef _ERR2LOG
	if( verbose < 0 )
		logReopen();
#endif

	return r;
}

