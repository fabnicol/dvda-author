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

#include <iostream>
#include <experimental/filesystem>
#include "util.h"

namespace fs = std::experimental::filesystem;
using namespace std;    




// ----------------------------------------------------------------------------
//    fs_GetTempDir :
// ----------------------------------------------------------------------------
//    Returns path to system temp directory, including trailing setarator.
// ----------------------------------------------------------------------------


string fs_GetTempDir()
{
    string temp = (fs::temp_directory_path() / "dummy").generic_string();
    remove(temp.c_str());
}



// ----------------------------------------------------------------------------
//    fs_EmptyDir :
// ----------------------------------------------------------------------------
//    Deletes the contents of folder <dirName>.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------


bool fs_EmptyDir( const fs::path& dirName )
{
	bool res = fs::remove_all(dirName) > 0;
	if (res)
	{
		res = fs::create_directory(dirName);
	}
	
	return res;
}




// ----------------------------------------------------------------------------
//    fs_DeleteDir :
// ----------------------------------------------------------------------------
//    Deletes the folder <dirName> entirely.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------


bool fs_DeleteDir(const fs::path& dirName)
{
	return(fs::remove_all(dirName) > 0);
}

// ----------------------------------------------------------------------------
//    fs_MakeDirs :
// ----------------------------------------------------------------------------
//    Creates all nonexistent folders in path <dirName>.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------

bool fs_MakeDirs( const fs::path& dirName )
{
    return fs::create_directories(dirName);
}

// ----------------------------------------------------------------------------
//    fs_GetAllDirs :
// ----------------------------------------------------------------------------
//    Collects all subfolder names under folder <dirName> into array <dirs>.
//
//    Returns number of subfolders on success, 0 on fail or no subfolders
// ----------------------------------------------------------------------------

size_t fs_GetAllDirs( const string& dirName, vector<string>& dirs )
{
	 int n = 0;
	 for(auto& p: fs::directory_iterator(dirName))
	 {
        if (fs::is_directory(p.path()))
		{
            dirs.emplace_back(p.path().generic_string());
		    ++n;
		}
	 }
	 
	return n;
}

// ----------------------------------------------------------------------------
//    fs_DirSize :
// ----------------------------------------------------------------------------
//    Returns size of given directory <dirName> on success, 0 on fail
// ----------------------------------------------------------------------------

size_t fs_DirSize( const fs::path& dirName )
{
 size_t total_size = 0;
 
 for(auto& p: fs::directory_iterator(dirName))
 {
      if (fs::is_directory(p.path()))
		{
            total_size += fs_DirSize(p.path());
		}
	  else
      if (fs::is_regular_file(p.path()))
		{
            total_size += fs::file_size(p.path());
		}
 }

 return total_size;
}


// ----------------------------------------------------------------------------
//    fs_validPath :
// ----------------------------------------------------------------------------
//    Returns <filename> trimmed of nonexistent part, if any.
// ----------------------------------------------------------------------------

const char * fs_validPath( const fs::path& filename )
{
    fs::path p = filename.parent_path();
    if (fs::exists(p)) return p.generic_string().c_str(); else return nullptr;
}


// ----------------------------------------------------------------------------
//    fs_fixSeparators :
// ----------------------------------------------------------------------------
//    Ensures separators in <path> conform to system.
// ----------------------------------------------------------------------------


void fs_fixSeparators( char * path )
{
    if( SEPARATOR == "\\" )
    {
        int i = 0;
        while (path[i] != '\0')
        {
            if (path[i] == '/') path[i] = SEPARATOR[0];
            ++i;
        }
    }
	else
    {
        int i = 0;
        while (path[i] != '\0')
        {
            if (path[i] == '\\') path[i] = SEPARATOR[0];
            ++i;
        }
    }
}
