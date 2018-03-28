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
#include <fstream>

long execute( const string& command, int verbose)
{
#ifdef _ERR2LOG
	if( verbose > -1 )
		xlog << STAT_TAG << "Running command : " << command << endl;
	else
		xlog.close();
#endif

    int exitStatus = system(command.c_str());
	
	return exitStatus;
}

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


long executeViaScript(const string& command, int verbose,
	int (*filter)( const char *, bool ) )
{
	long r;

#ifdef _ERR2LOG
	if( verbose > -1 )
		xlog << STAT_TAG << "Running command : "<< command << endl;
	else
		xlog.close();
#endif

	ofstream scriptFile;

    string script = (tempDir / ("script." SCRIPT_EXT)).generic_string();

    scriptFile.open( script );
	scriptFile << command;
	
	scriptFile.close();

	r = execute( _f( SCRIPT_CMD ) + QUOTE( script ),
        verbose > 0 ? verbose : -1);

#ifdef _ERR2LOG
	if( verbose < 0 )
		logReopen();
#endif

	return r;
}
