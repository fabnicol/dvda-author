/*
	wx.cpp - misc wxwidgets extensions.
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

#include <iostream>
#include <filesystem>
#include "util.h"

namespace fs = std::filesystem;
using namespace std;




// ----------------------------------------------------------------------------
//    fs_GetTempDir :
// ----------------------------------------------------------------------------
//    Returns path to system temp directory, including trailing setarator.
// ----------------------------------------------------------------------------


string fs_GetTempDir()
{
    string temp = (fs::temp_directory_path() / "dummy").string();
    remove(temp.c_str());
    return temp;
}




// ----------------------------------------------------------------------------
//    fs_DeleteDir :
// ----------------------------------------------------------------------------
//    Deletes the folder <dirName> entirely.
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------

#include <dirent.h>
#include <sys/stat.h>
bool fs_DeleteDir(const fs::path& dirName)
{
#ifndef USE_C_RMDIR
        return(fs::remove_all(dirName) > 0);
#else
    if (! fs::exists(dirName)) return true;

    cerr << "Removing " << dirName << endl;

    typedef struct slist_t
    {
        char* name;
        int is_dir;
        struct slist_t *next;
    } slist_t;


    char* root = normalize_windows_paths(dirName.parent_path().string().c_str());   // .c_str() does not work directly under mingw64
    char* dirname = normalize_windows_paths(dirName.filename().string().c_str());
    char *cwd;
    cwd = (char*) normalize_windows_paths(fs::current_path().string().c_str());

        if (chdir(dirName.string().c_str()) == -1)
        {
            cerr << "[ERR] Could not change dir to " << dirName << endl;
            if (errno == ENOTDIR)
            return true;
            //printf ( ERR "chdir() issue with dirname=%s\n", dirname);
            else return (false);
        }

        slist_t *names = nullptr;
        slist_t *sl;

        DIR *FD;
        struct dirent *f;
        char *new_root;

        if (root)
        {
            int rootlen = strlen (root);
            int dirnamelen = strlen (dirname);
            if (nullptr ==
                    (new_root = (char*)
                         malloc ((rootlen + dirnamelen + 2) * sizeof *new_root)))
            {
                cerr <<  "[ERR] malloc issue\n";
                exit (EXIT_FAILURE);
            }
            memcpy (new_root, root, rootlen);
            new_root[rootlen] = SEPARATOR[0];
            memcpy (new_root + rootlen + 1, dirname, dirnamelen);
            new_root[rootlen + dirnamelen + 1] = '\0';
        }
        else
            new_root = strdup (dirname);


        if (nullptr == (FD = opendir (".")))
        {
            cerr << "[ERR] opendir() issue\n";
            return (-1);
        }
        sl = names;
        while ((f = readdir (FD)))
        {
            struct stat st;
            slist_t *n;
            if (!strcmp (f->d_name, "."))
                continue;
            if (!strcmp (f->d_name, ".."))
                continue;
            if (stat (f->d_name, &st))
                continue;
            if (nullptr == (n = (slist_t*) malloc (sizeof *n)))
            {
                cerr << "[ERR] memory issue\n";
                throw;
            }
            n->name = strdup (f->d_name);
            if (S_ISDIR (st.st_mode))
                n->is_dir = 1;
            else
                n->is_dir = 0;
            n->next = nullptr;
            if (sl)
            {
                sl->next = n;
                sl = n;
            }
            else
            {
                names = n;
                sl = n;
            }
        }
        closedir (FD);


        for (sl = names; sl; sl = sl->next)
        {
            if (!sl->is_dir)
            {
                remove(sl->name);
            }

        }


        for (sl = names; sl; sl = sl->next)
        {
            if (sl->is_dir)
            {

                fs_DeleteDir(fs::path(new_root) / fs::path(sl->name));
                rmdir (sl->name);
                if (fs::exists(fs::path(sl->name)))
                {
                    cerr << "[ERR] Impossible to erase directory " << sl->name << endl ;
                    throw;
                }
            }
        }


        free (new_root);
        while (names)
        {
            slist_t *prev;
            free (names->name);
            prev = names;
            names = names->next;
            free (prev);
        }


        if (fs::exists(cwd) && chdir (cwd) != 0) perror("[ERR]  chdir");
        rmdir(normalize_windows_paths(dirName.string().c_str()));
        if (fs::exists(dirName)) cerr << "[ERR] Directory " << dirName << " not deleted" << endl;
        return (true);
#endif
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
            dirs.emplace_back(p.path().string());
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
//    Tests existence of path.
// ----------------------------------------------------------------------------

bool fs_validPath( const fs::path& p )
{
    return (fs::exists(p));
}


// ----------------------------------------------------------------------------
//    fs_fixSeparators :
// ----------------------------------------------------------------------------
//    Ensures separators in <path> conform to system.
// ----------------------------------------------------------------------------


void fs_fixSeparators( char * path )
{
    if( strcmp(SEPARATOR, "\\") == 0 )
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
