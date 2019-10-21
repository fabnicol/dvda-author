/*
	platform.h - platform-related utilities.
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


#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED

#ifndef LPLEX_PCH_INCLUDED
#  include "lplex_precompile.h"
#endif
#include <filesystem>
#include "wx.hpp"

using namespace std;
namespace  fs = std::filesystem;


#if defined __WIN32 || defined _WIN32 || defined _WIN64 || defined __WIN64 || defined MINGW32 || defined MINGW64 || defined(__MINGW32__) || defined(__MINGW64__)
#  define SEPARATOR  "\\"
#  define USER  "USERNAME"
#  define HOME  "C:\\Users"
#  ifndef USE_C_RMDIR
#    define USE_C_RMDIR  // as of Apr. 2018, mingw64 port of g++-7.3.0 has a permissions file bug for fs::remove_all
#  endif
#  ifndef lplex_win32
#    define lplex_win32
#  endif
#else
#  define SEPARATOR "/"
#  define USER "USER"
#  define HOME "HOME"
#  ifndef lplex_linux
#    define lplex_linux
#  endif
#endif

#  define lplex_console

extern string 	shebang;
extern fs::path lplexConfig, configDir, dataDir, binDir, tempDir, isoPath, readOnlyPath, projectDotLplex;

// endian swap macros (from dvdread/bswap.h)

inline uint64_t bswap(uint64_t i)
{
	return (
		(((i) & 0xff00000000000000LL) >> 56) |
		(((i) & 0x00ff000000000000LL) >> 40) |
		(((i) & 0x0000ff0000000000LL) >> 24) |
		(((i) & 0x000000ff00000000LL) >>  8) |
		(((i) & 0x00000000ff000000LL) <<  8) |
		(((i) & 0x0000000000ff0000LL) << 24) |
		(((i) & 0x000000000000ff00LL) << 40) |
		(((i) & 0x00000000000000ffLL) << 56));
}
inline uint32_t bswap(uint32_t i)
{
	return (
		(((i) & 0xff000000) >> 24) |
		(((i) & 0x00ff0000) >>  8) |
		(((i) & 0x0000ff00) <<  8) |
		(((i) & 0x000000ff) << 24));
}
inline uint16_t bswap(uint16_t i)
{
	return (
		(((i) & 0xff00) >> 8) |
		(((i) & 0x00ff) << 8));
}

// ----------------------------------------------------------------------------
#ifdef lplex_bigendian
// ----------------------------------------------------------------------------

#define lEndian(i) bswap(i)
#define bEndian(i) i

#else

#define lEndian(i) i
#define bEndian(i) bswap(i)

#endif


// ----------------------------------------------------------------------------
#ifdef lplex_linux
// ----------------------------------------------------------------------------

#if ! defined( lplex_wxasync )
#if ! defined( lplex_console )
#define lplex_wxasync
#endif
#endif

//#define HAS_STRICMP
#define stricmp strcasecmp
#define strnicmp strncasecmp

#define SCRIPT_CMD "bash "
#define SCRIPT_EXT "sh"

#define NUL "/dev/null"
//#define CON "/dev/console"
#define CON "/dev/tty"

#ifndef lplex_nocolor
#define ANSI_COLOR
#endif

extern int endPause;
inline void _pause() { cerr << "\npress <enter> to close..."; cin.get(); }
//inline void _pause() { system( "echo press any key to close; read -n 1" ); }
#define device(d) d
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
inline string volumeLabel( const char *path, bool mustBeRoot )
	{ return ""; }
#pragma GCC diagnostic pop


inline bool initPlatform()
{
    fs::path home = fs::path(getenv(HOME));
    fs::path appdata = home / ".config";
    configDir = appdata / string("lplex");
    lplexConfig = configDir / "lplex.ini";
    isoPath = configDir / "iso";
    binDir = fs::path("/usr/bin");
    dataDir = configDir / "data" ;
    readOnlyPath = home;
    tempDir = home / "temp";
    fs::create_directories(tempDir);
	shebang = "#!/usr/local/bin/lplex -P 1\n";
	endPause = true;
    return true;
}


#endif

// ----------------------------------------------------------------------------
#ifdef lplex_win32
// ----------------------------------------------------------------------------

#include <windows.h>

#if ! defined( lplex_wxasync )
#define lplex_wxasync
#endif

#define SCRIPT_CMD ""
#define SCRIPT_EXT "bat"
#define NUL "nul"
#define CON "con"

#ifndef lplex_nocolor
#define WIN32_COLOR
#endif

extern int endPause;
inline void _pause()
	{ cerr << "\npress any key to close..."; system( "pause>nul" ); }
//inline void _pause() { cerr << "\npress <enter> to close..."; cin.get(); }
#if 0
inline const char* device( dev_t device )
	{ char dev[] = { 'A' + device, '\0' }; return dev; }
#endif

inline bool initPlatform()
{

    string home = string(HOME) + SEPARATOR + getenv(USER);
    string appdata = string(getenv("APPDATA"));
    configDir = appdata + string(SEPARATOR  "lplex"  SEPARATOR);
    lplexConfig = configDir / "lplex.ini";
    isoPath = fs::path("iso");
    binDir =  fs::path("bin");
    dataDir = appdata /  fs::path("lplex/data");
    readOnlyPath = home;
    tempDir = fs::temp_directory_path()/fs::path("lplex");
    if (! fs::exists(tempDir))
      fs::create_directories(tempDir);

    shebang = "#!/usr/local/bin/lplex -P 1\n";
    endPause = true;
    return true;
}

// ----------------------------------------------------------------------------
//    volumeLabel :
// ----------------------------------------------------------------------------
//    Returns volume label of <path>, rejecting non-root paths if <mustBeRoot>.
//    (win32 only)
// ----------------------------------------------------------------------------


inline string volumeLabel( const char * path, bool mustBeRoot=false )
{
    return fs::path(path).root_path().string();
}


#endif


#endif
