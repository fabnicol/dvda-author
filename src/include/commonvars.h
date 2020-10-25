/*
File:    commonvars.h
Purpose: defines and macros

dvda-author  - Author a DVD-Audio DVD

(C) Revised version with zone-to-zone linking Fabrice Nicol <fabnicol@users.sourceforge.net> 2007, 2008

The latest version can be found at http://dvd-audio.sourceforge.net

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

#ifndef COMMONVARS_H_INCLUDED
#define COMMONVARS_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libiberty.h"

#ifndef START_MLP_BYTESHIFT
#define START_MLP_BYTESHIFT 23
#endif

// With Code::Blocks and similar IDE, specify your root package directory as argument to --workdir

#ifdef __CB__
#define INSTALL_CONF_DIR "."
#endif

#ifndef WEBSITE
#define WEBSITE "http://dvd-audio.sourceforge.net"
#endif

#ifndef FORENSIC_MLP_DECODE
#define FORENSIC_MLP_DECODE 1
#endif

#define bool _Bool

/* This sanity check macros forces LONG_OPTIONS when _GNU_SOURCE has been defined as a compile-time option
 * unless SHORT_OPTIONS_ONLY has been defined to block long options irrespective of the source version
 * _GNU_SOURCE si independently requested by strndup
 * GNU Autoconf scripts define _LONG_OPTIONS among others in config.h thanks to
 * macro AC_USE_SYSTEM_EXTENSIONS in the top configure.ac */

#define FREE_BINARY_PATH_SPACE 4

#define CONFIGURATION_FILE 1
#define PROJECT_FILE 0
#define DEFAULT_DVDA_AUTHOR_PROJECT_FILENAME "dvda-author.dap"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#else
#if !defined(LONG_OPTIONS) && !defined(SHORT_OPTIONS_ONLY)
#define LONG_OPTIONS
#endif
#endif


#define PROGRAM ANSI_COLOR_RED"DVD-A author"ANSI_COLOR_RESET

/* filesystem constants expressed in sectors */

#define DEFAULT_ACCESS_RIGHTS 0755
#ifndef STARTSECTOR
#  define STARTSECTOR  272
#endif

// sizes are in sector
#define SIZE_AMG  3
#define SIZE_ASVS 2
#define SIZE_SAMG 64
#define SIZE_ASVS_DEFAULT 2   // AUDIO_SV.IFO
// buffer size in bits
#define AUDIO_BUFFER_SIZE 65536

#define MAX_GROUPS     9
#define MAX_GROUP_ITEMS        99

#define HIGH 2

/* default backup directories */

#define DEFAULT  "Audio"
#define DEFAULT_AOB_FOLDER   "AOB"
#define DEFAULT_SYSTEM_FOLDER  "SYSTEM"
#define DEFAULT_DATABASE_FOLDER "DATABASE"

/*  A DVD-compliant disc can have up to 99 video title sets.  */

#define MAXIMUM_LINKED_VTS  9
#define PLAYBACK_TIME_OFFSET 0x1014
/* For IDE builds, define SETTINGSFILE in IDE as the install path to dvda-author.conf */
#ifndef SETTINGSFILE
/* either define SETTINGSFILE at compile time or let configure find the right install dir */
#ifdef INSTALL_CONF_DIR
#define SETTINGSFILE   INSTALL_CONF_DIR "/dvda-author.conf"  // to install with autotools build system or with #define elsewhere
#else
#define SETTINGSFILE    "dvda-author.conf" // as used in /src, will refer to root directory dvda-author.conf
#endif
#endif

#define CGA_FILE  "cgafile"


#define DEFAULT_LOGFILE "log"

#define STANDARD_FIXWAV_SUFFIX "_fix_"
#define STANDARD_FIXWAV_DATABASE_PATH "database"

#ifndef BINDIR
#if ! defined _WIN32 && ! defined LOCAL_BUILD
#define BINDIR "/usr/bin"
#endif
#endif


#if !defined HAVE_NO_MKISOFS_PATCH || HAVE_NO_MKISOFS_PATCH == 0
#define HAVE_NO_MKISOFS_PATCH 0
#endif

#if !defined HAVE_NO_DVDAUTHOR_PATCH || HAVE_NO_DVDAUTHOR_PATCH == 0
#define HAVE_NO_DVDAUTHOR_PATCH 0
#endif

#ifndef PATCH_LABEL
#define PATCH_LABEL ""
#endif

#ifndef VLC_BASENAME
#define VLC_BASENAME "vlc"
#endif

#ifndef MKISOFS_BASENAME
#define MKISOFS_BASENAME "mkisofs" PATCH_LABEL
#endif

#ifndef LPLEX_BASENAME
#define LPLEX_BASENAME "lplex"
#endif


#ifndef SOX_BASENAME
#define SOX_BASENAME "sox"
#endif

#ifndef CDRECORD_BASENAME
#define CDRECORD_BASENAME "cdrecord"
#endif

#ifndef DVDAUTHOR_BASENAME
#define DVDAUTHOR_BASENAME "dvdauthor" PATCH_LABEL
#endif

#ifndef JPEG2YUV_BASENAME
#define JPEG2YUV_BASENAME  "jpeg2yuv"
#endif

#ifndef FFPLAY_BASENAME
#define FFPLAY_BASENAME  "ffplay"
#endif

#ifndef DVDA_AUTHOR_BASENAME
#define DVDA_AUTHOR_BASENAME "dvda-author"
#endif

#ifndef SPUMUX_BASENAME
#define SPUMUX_BASENAME  "spumux" PATCH_LABEL
#endif


#ifndef MPEG2ENC_BASENAME
#define MPEG2ENC_BASENAME "mpeg2enc"
#endif

#ifndef MPEG2DEC_BASENAME
#define MPEG2DEC_BASENAME "mpeg2dec"
#endif

#ifndef EXTRACT_AC3_BASENAME
#define EXTRACT_AC3_BASENAME "extract_ac3"
#endif

#ifndef AC3DEC_BASENAME
#define AC3DEC_BASENAME "ac3dec"
#endif


#ifndef MOGRIFY_BASENAME
#define MOGRIFY_BASENAME "mogrify"
#endif

#ifndef CONVERT_BASENAME
#define CONVERT_BASENAME "convert"
#endif


#ifndef MP2ENC_BASENAME
#define MP2ENC_BASENAME  "mp2enc"
#endif

#ifndef PGMTOY4M_BASENAME
#define PGMTOY4M_BASENAME  "pgmtoy4m"
#endif

#ifndef MPLEX_BASENAME
#define MPLEX_BASENAME  "mplex"
#endif

#ifndef CURL_BASENAME
#define CURL_BASENAME "curl"
#endif

#ifndef LPLEX
#define LPLEX ""
#endif


#ifndef SOX
#define SOX ""
#endif

#ifndef MKISOFS
#define MKISOFS ""
#endif

#ifndef CDRECORD
#define CDRECORD ""
#endif

#ifndef DVDAUTHOR
#define DVDAUTHOR ""
#endif

#ifndef JPEG2YUV
#define JPEG2YUV ""
#endif

#ifndef FFPLAY
#define FFPLAY ""
#endif

#ifndef SPUMUX
#define SPUMUX ""
#endif


#ifndef MPEG2ENC
#define MPEG2ENC ""
#endif

#ifndef PGMTOY4M
#define PGMTOY4M ""
#endif

#ifndef MPEG2DEC
#define MPEG2DEC ""
#endif

#ifndef MOGRIFY
#define MOGRIFY ""
#endif

#ifndef CONVERT
#define CONVERT ""
#endif

#ifndef MP2ENC
#define MP2ENC ""
#endif

#ifndef MPLEX
#define MPLEX ""
#endif

#ifndef CURL
#define CURL ""
#endif

#ifndef EXTRACT_AC3
#define EXTRACT_AC3 ""
#endif

#ifndef AC3DEC
#define AC3DEC ""
#endif

#ifndef VLC
#define VLC ""
#endif

#ifndef _WIN32
#  define PLAYER FFPLAY_BASENAME
#  define VLC_PATH  "/usr/bin/vlc"
#  define PLAYER_PATH  ""
#else
#  define PLAYER VLC_BASENAME
#  define VLC_PATH  "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe"
#  define PLAYER_PATH  VLC_PATH
#endif

#ifdef _WIN32
#  define EXE ".exe"
#  define NO_ANSI_COLORS
#else
#  define EXE ""
#endif

#define CREATE_EXTRACT_AC3 7
#define CREATE_CURL     6
#define CREATE_MPEG2DEC 5
#define FREE_MEMORY     4
#define CREATE_IMAGEMAGICK 3
#define CREATE_DVDAUTHOR 2
#define CREATE_SPUMUX 1
#define CREATE_MJPEGTOOLS 0

#ifndef TEMPDIR_SUBFOLDER_PREFIX
#ifdef __WIN32__
#define TEMPDIR_SUBFOLDER_PREFIX ""   //[Windows hates dots]
#else
#define TEMPDIR_SUBFOLDER_PREFIX "."  //[Linux-style]
#endif
#endif

#ifndef DEFAULT_WORKDIR
#ifdef __WIN32__
#define DEFAULT_WORKDIR "C:\\"
#endif
#endif

#define TOPMENU_NAME "topmenu"
#define TOPMENU_MIN_SIZE  32
#define DEFAULT_MENU_NCOLUMNS 3

#ifndef DEFAULT_SOUNDTRACK
/* either define SETTINGSFILE at compile time or let configure find the right install dir */
#define DEFAULT_SOUNDTRACK   "menu/silence.wav"  // to install with autotools build system or with #define elsewhere
#endif

#ifndef NORM
#define NORM "PAL_720x576"
#endif

// Colors in YCrCb format for palette and either labels (red) HTML or rgb code for pics.
// It is always necessary to have distinct colors in pic generation, even if they will not surface on screen (this is because of the inner workings of dvdauthor and should be imporved)
// The palettes only matter for actual rendition of colors. _PIC colors are what is actually generated, this is for advanced usage and should not have  importance for most users (--colors)
//
#define DEFAULT_ALBUMCOLOR "0,0,255" //blue              //Theoretical values, there are fewer possibilities for now, see comments below
#define DEFAULT_GROUPCOLOR  "0,0,255" // must be the same
#define DEFAULT_ARROWCOLOR "255,255,255"           //red
#define DEFAULT_TEXTCOLOR_PALETTE "0xE6807F"  // pure white text, highlighted or not highlighted
#define DEFAULT_BGCOLOR_PALETTE   "0x286DF0"  // album, group and select action text (turns navy blue on pressing the highlighted track)
#define DEFAULT_HCOLOR_PALETTE    "0x51F05A"  //red underline
#define DEFAULT_SELCOLOR_PALETTE  "0x88B33A"  // ochre background
#define DEFAULT_TEXTCOLOR_PIC "255,255,255"  // white  //
#define DEFAULT_BGCOLOR_PIC   "0,0,0"  // black
#define DEFAULT_HCOLOR_PIC   "255,0,0"  //red
#define DEFAULT_SELCOLOR_PIC  "0,0,255"  // blue
#define DEFAULT_ACTIVETEXTCOLOR_PALETTE "0xE6807F"  // white non-highlighted and highlighted text
#define DEFAULT_ACTIVEBGCOLOR_PALETTE   "0xE6807F"  // broken
#define DEFAULT_ACTIVEHCOLOR_PALETTE    "0x902235"	//green	 underline color
#define DEFAULT_ACTIVESELCOLOR_PALETTE "0x88B33A"  // ochre : the text turns ochre on pressing the remote for the highlighted text "0x286DF0"  // blue


#ifdef _WIN32
#define DEFAULT_TEXTFONT  "Arial"
#define FORK false
#else
#define DEFAULT_TEXTFONT  "Courier-Bold"
#define FORK true
#endif

#define DEFAULT_SVHIGHLIGHTTEXT
#define DEFAULT_SVHIGHLIGHTMOTIF	"0x51F05A"  //red		//    SVHIGHLIGHTMOTIF
#define DEFAULT_BLANKSCREEN "menu/black_" NORM ".png"
#define DEFAULT_BLANKSCREEN_NTSC "menu/black_" "NTSC_720x480" ".png"    // In principle blank for preparing titles yet can have some background
#define DEFAULT_BACKGROUNDPIC "menu/black_" NORM ".jpg"  // for mpeg authoring
#define DEFAULT_BACKGROUNDPIC_NTSC "menu/black_" "NTSC_720x480" ".jpg"  // for mpeg authoring
#define DEFAULT_ACTIVEHEADER "menu/activeheader"
#define DEFAULT_ASPECT_RATIO "4:3"
#define DEFAULT_POINTSIZE  25
#define DEFAULT_FONTWIDTH  6  // 6 pixels for size 10 in Courier
#define DEFAULT_SCREENTEXT "ALBUM=Group 1 - Track 1,Group 1 - Track 2:Group 2 - Track 1,Group 2 - Track 2";
#define MAX_LEXER_LINES  100
#define MAXSTILLPICVOBSIZE  20 * 1024 * 1024  // 20 MB limit for stillvobs
#define ACTIVEHEADER_INSERTOFFSET  0xBB
#define DEFAULT_AUDIOFORMAT "mp2"
#define AUDIT_STRICT_TOPMENU_AUDIO_FORMAT  1
#define AUDIT_DVD_VIDEO_AUDIO_FORMAT  0
#define NO_FIXWAV_AUDIT  2
#define TEMPORARY_AUTOMATIC_MENU -5
#define AUTOMATIC_MENU -4 // automate all menu authoring process

// for values higher than -2 and lower than 4 top menu is generated

#define RUN_MJPEG_GENERATE_PICS_SPUMUX_DVDAUTHOR -1
#define RUN_GENERATE_PICS_SPUMUX_DVDAUTHOR 0
#define RUN_SPUMUX_DVDAUTHOR 1 // automate some of the authoring process (run spumux and dvdauthor)
#define RUN_DVDAUTHOR 2 // input menu is given, run dvdauthor
#define TS_VOB_TYPE 3
#define NO_MENU 5  // do not do anything

#define MIX_NEW_SOUNDTRACK 1
#define USE_VTS_SOUNDTRACK 0

#define ANIMATEDVIDEO 1
#define STILLPICS     2
#define NOPICS        0

#define UNDERLINE 0
#define PRECEDE -1
#define BUTTON 1

#define DEFAULT_ALBUM_HEADER "Album"
#define DEFAULT_TRACK_HEADER "Track "
#define DEFAULT_GROUP_HEADER "Group"
#define DEFAULT_GROUP_HEADER_UPPERCASE "GROUP "  // use uppercase, see explanation in menu.c
#define DEFAULT_NEXT "Next"
#define DEFAULT_PREVIOUS "Previous"

#define ALBUM_TEXT_Y0 48
#define MAX_POINTSIZE  35
#define MIN_POINTSIZE  7
#define MAX_BUTTON_NUMBER 36
#define MAX_BUTTON_Y_NUMBER 34 // empirical limitation in Y space
#define EMPIRICAL_X_SHIFT  25  // empirical shift cut on the X axis at least for PAL/Secam -- 720 pixels not being really displayed. TODO: explain.
#define CUT 0
#define FADE 0x10
#define DISSOLVE 0x20
#define WIPEFROMTOP 0x30
#define WIPEFROMBOTTOM 0x40
#define WIPEFROMLEFT 0x50
#define WIPEFROMRIGHT 0x60


#define PAL_FRAME_RATE "25"

#ifndef PAL_X
#define PAL_X 720   // ntsc 720
#endif
#ifndef PAL_Y
#define PAL_Y 576   // ntsc 480
#endif
#ifndef NTSC_Y
#define NTSC_Y 480
#endif

#define INFO_EXECTIME1  MSG_TAG "User execution time: %u minutes %u seconds %u microseconds\n"
#define INFO_EXECTIME2  MSG_TAG "System execution time: %u minutes %u seconds %u microseconds\n"

#ifndef LOCALE
#define LOCALE "C"
#endif

#define READTRACKS 1

#define INFO_GNU   ANSI_COLOR_BLUE "[LICENSE]" ANSI_COLOR_RESET "  Copyright Dave Chapman 2005-Fabrice Nicol 2007-2019\n\
<fabnicol@users.sourceforge.net>-Lee and Tim Feldkamp 2008-2009\n\
This file is part of dvda-author.\n\
dvda-author is free software: you can redistribute it and/or modify \n\
it under the terms of the GNU General Public License as published by \n\
the Free Software Foundation, either version 3 of the License, or \n\
(at your option) any later version.\n\
dvda-author is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of \n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \n\
GNU General Public License for more details.\n\
You should have received a copy of the GNU General Public License \n\
along with dvda-author.  If not, see http://www.gnu.org/licenses/.\n"

#define SINGLE_DOTS  if (!globals->silence)  foutput("\n%s\n",         "------------------------------------------------------------");
#define DOUBLE_DOTS  if (!globals->silence)  foutput("\n%s",              "============================================================");
#define J "\n\n                         "  //left-aligns definition strings
#define K "\n                  "  // short left-aligns definition strings


#endif
