/*
	wx.cpp - misc wxwidgets extensions.
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



#include "wx.hpp"



// ----------------------------------------------------------------------------
//    _wxGetTempDir :
// ----------------------------------------------------------------------------
//    Returns path to system temp directory, including trailing setarator.
// ----------------------------------------------------------------------------


wxString _wxGetTempDir()
{
	wxString temp = wxFileName::CreateTempFileName("dummy");
	wxRemoveFile(temp);
	return wxFileName(temp).GetPath
		( wxPATH_NATIVE | wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR );
}



// ----------------------------------------------------------------------------
//    _wxEmptyDir :
// ----------------------------------------------------------------------------
//    Deletes the contents of folder <dirName>.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------


bool _wxEmptyDir( const char *dirName )
{
	wxLogNull xx;
	wxDir dir( dirName );
	if( !dir.IsOpened() )
	{
		ERR( "couldn't open " << dirName << "\n" );
		return false;
	}

	_wxFileKiller killer( dirName );
	dir.Traverse( killer );

	return true;
}



// ----------------------------------------------------------------------------
//    _wxDeleteDir :
// ----------------------------------------------------------------------------
//    Deletes the folder <dirName> entirely.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------


bool _wxDeleteDir( const char *dirName )
{
	wxLogNull xx;
	_wxEmptyDir( dirName );
	return wxRmdir( dirName );
}



// ----------------------------------------------------------------------------
//    _wxMakeDirs :
// ----------------------------------------------------------------------------
//    Creates all nonexistent folders in path <dirName>.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------


bool _wxMakeDirs( const char *dirName )
{
	wxFileName parent = wxFileName( dirName ).GetPath();

	if ( ! wxDir::Exists( parent.GetFullPath() ) )
		if( ! _wxMakeDirs( parent.GetFullPath() ) )
			ERR( "couldn't create \'" << parent.GetFullPath() << "\'\n" );

	if ( ! wxDir::Exists( dirName ) )
		return wxMkdir( dirName );

	return true;
}



// ----------------------------------------------------------------------------
//    _wxGetAllDirs :
// ----------------------------------------------------------------------------
//    Collects all subfolder names under folder <dirName> into array <dirs>.
//
//    Returns number of subfolders on success, 0 on fail or no subfolders
// ----------------------------------------------------------------------------


size_t _wxGetAllDirs( const wxString& dirName, wxArrayString *dirs )
{
	size_t n=0;
	wxString filename;
	wxLogNull xx;

	wxDir dir( dirName );
	if( !dir.IsOpened() )
	{
		ERR( "couldn't open " << dirName << "\n" );
		return 0;
	}

	bool cont = dir.GetFirst( &filename, wxEmptyString,
		wxDIR_DIRS | wxDIR_HIDDEN );

	while( cont )
	{
		n++;
		dirs->Add( dirName + wxSEP + filename );
		cont = dir.GetNext( &filename );
	}

	return n;
}



// ----------------------------------------------------------------------------
//    _wxDirSize :
// ----------------------------------------------------------------------------
//    Returns size of given directory <dirName> on success, 0 on fail
// ----------------------------------------------------------------------------



size_t _wxDirSize( const char *dirName )
{
	wxLogNull xx;
	class _wxDirSizer : public wxDirTraverser
	{
	public:
		size_t size;

		_wxDirSizer() : size(0) {}

		virtual wxDirTraverseResult OnFile( const wxString& filename )
		{
			size += statsize( filename.mb_str() );
			return wxDIR_CONTINUE;
		}

		virtual wxDirTraverseResult OnDir( const wxString& dirName )
		{
			return wxDIR_CONTINUE;
		}
	};

	wxDir dir( dirName );
	if( !dir.IsOpened() )
	{
		ERR( "couldn't open " << dirName << "\n" );
		return 0;
	}

	_wxDirSizer total;
	dir.Traverse( total );

	return total.size;
}



// ----------------------------------------------------------------------------
//    _wxEndSep :
// ----------------------------------------------------------------------------
//    Returns <path>, adding a trailing separator if missing.
// ----------------------------------------------------------------------------


wxString _wxEndSep( const char *path )
{
#if 0
	wxFileName fullPath( path );
	if( ! fullPath.IsDir() )
		fullPath =  fullPath.GetFullPath() + wxSEP;

	return fullPath.GetFullPath();
#endif
	wxString str( path );
	if( str.Right(1) != wxSEP )
		str += wxSEP;
	return str;
}



// ----------------------------------------------------------------------------
//    _wxValidPath :
// ----------------------------------------------------------------------------
//    Returns <filename> trimmed of nonexistent part, if any.
// ----------------------------------------------------------------------------


const char * _wxValidPath( const char * filename )
{
	wxFileName fName( filename );
	while( fName.GetFullPath() != wxEmptyString && ! fName.DirExists( fName.GetFullPath() ) )
		fName = fName.GetPath();
	return ( fName.GetFullPath() == wxEmptyString ?
		NULL : (const char *)fName.GetFullPath().mb_str() );
}


// ----------------------------------------------------------------------------
//    _wxFixSeparators :
// ----------------------------------------------------------------------------
//    Ensures separators in <path> conform to system.
// ----------------------------------------------------------------------------


void _wxFixSeparators( char * path )
{
	if( wxSEP == '\\' )
		wxUnix2DosFilename( path );
	else
		wxDos2UnixFilename( path );
}
