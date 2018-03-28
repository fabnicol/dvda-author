/*
	wx.hpp - misc wxwidgets extensions.
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



#ifndef WX_HPP_INCLUDED
#define WX_HPP_INCLUDED

#ifndef LPLEX_PCH_INCLUDED
#include "lplex_precompile.h"
#endif

using namespace std;

#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/string.h>
#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/app.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/timer.h>
#include <wx/process.h>
#include <wx/txtstrm.h>

#include "util.h"

#ifndef wxSEP
#define wxSEP wxFILE_SEP_PATH
#endif

bool _wxMakeDirs( const char *dirName );
wxString _wxEndSep( const char *path );
wxString _wxGetTempDir();
bool _wxDeleteDir( const char *dirName );
bool _wxEmptyDir( const char *dirName );
size_t _wxGetAllDirs( const wxString& dirName, wxArrayString *dirs );
size_t _wxDirSize( const char *dirName );
const char * _wxValidPath( const char * filename );
void _wxFixSeparators( char * path );

class _wxStopWatch : public wxStopWatch
{
public:
	int m, s;

	void pause()
	{
		Pause();
		m = Time() / 60000;
		s = ( Time() % 60000 ) / 1000;
		INFO( _f( "Elapsed time=%d:%02d\n", m, s ) );
		SCRN( INFO_TAG << _f( "Done - %d min %02d sec.\n", m, s ) );
	}
};

class _wxFileKiller : public wxDirTraverser
{
public:

	wxArrayString dirs;

	_wxFileKiller( const wxString& dirName ) {}
	~_wxFileKiller()
	{
		for( int i = dirs.GetCount() - 1; i > -1; i-- )
			if( ! (wxRmdir( dirs[i] )) )
				ERR( _f( "Unable to remove \'%s\'\n", dirs[i].mb_str() ) );
	}

	virtual wxDirTraverseResult OnFile( const wxString& filename )
	{
		if( ! wxRemoveFile( filename ) )
			ERR( _f("Unable to remove \'%s\'\n", filename.mb_str() ) );
		return wxDIR_CONTINUE;
	}

	virtual wxDirTraverseResult OnDir( const wxString& dirName )
	{
		dirs.Add( dirName );
		return wxDIR_CONTINUE;
	}
};


#endif
