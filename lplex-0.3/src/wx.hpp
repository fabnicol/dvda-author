/*
	wx.hpp - misc wxwidgets extensions.
	Copyright (C) 2006-2011 Bahman Negahban

    Adapted to C++-17 in 2018 by Fabrice Nicol

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

#include "util.h"


bool fs_MakeDirs( const fs::path& dirName );
string fs_GetTempDir();
bool fs_DeleteDir( const fs::path& dirName );
size_t fs_GetAllDirs( const string& dirName, vector<string>& dirs );
size_t fs_DirSize( const fs::path& dirName );
bool fs_validPath( const fs::path&  filename );
void fs_fixSeparators( char * path );

class _wxStopWatch
{
public:
	int m, s;

	void pause()
	{
		// Pause();
		// m = Time() / 60000;
		// s = ( Time() % 60000 ) / 1000;
		// INFO( _f( "Elapsed time=%d:%02d\n", m, s ) );
		// SCRN( INFO_TAG << _f( "Done - %d min %02d sec.\n", m, s ) );
	}

    void Start() {}
};



#endif
