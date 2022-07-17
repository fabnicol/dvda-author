/*
	color.h - screen color output using win32 and ansi console commands.
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



#ifndef CONSCOLOR_H
#define CONSCOLOR_H

#include <iostream>

//#define ANSI_RESET   "\e[2;37;0m"
#define ANSI_RESET   "\033[0m"


enum{ ansi=1, win32=2 };
typedef int colorval;
extern colorval consoleColorReset;
typedef struct{ const char *ansiStr; int winVal; } colorIndex;
extern colorIndex colorTable[];
extern int colorMode;

struct colorText
{
	enum
	{
		BOLD=0,
		BLINK,
		UNDERLINE,
		REVERSE,
		LTREVERSE,

		BLACK,
		RED,
		GREEN,
		YELLOW,
		BLUE,
		MAGENTA,
		CYAN,
		WHITE,

		GRAY,
		LTRED,
		LTGREEN,
		LTYELLOW,
		LTBLUE,
		LTMAGENTA,
		LTCYAN,
		LTWHITE,

		BGBLACK,
		BGRED,
		BGGREEN,
		BGYELLOW,
		BGBLUE,
		BGMAGENTA,
		BGCYAN,
		BGWHITE,

		BGLTRED,
		BGLTGREEN,
		BGLTYELLOW,
		BGLTBLUE,
		BGLTMAGENTA,
		BGLTCYAN,
		BGLTWHITE
	};

	const char *txt; colorval color;
};

#if defined(ANSI_COLOR)

inline void consoleColorInit() {}
inline void consoleColorRestore() { if( ::colorMode ) cerr << ANSI_RESET << flush; }
inline void consoleColorSet( colorval color )
{
	if( ::colorMode ) cerr << colorTable[color].ansiStr << flush;
}

inline std::ostream& operator << ( std::ostream& os, colorText c )
{
	if( ::colorMode )
		os << colorTable[c.color].ansiStr << c.txt << ANSI_RESET;
	else
		os << c.txt;
	return os;
}


#elif defined(WIN32_COLOR)

#include <windows.h>

//#define BLACK      0
//#define BLUE       1
//#define GREEN      2
//#define CYAN       3
//#define RED        4
//#define MAGENTA    5
//#define YELLOW     6
//#define WHITE      7
//
//#define GRAY       8
//#define LTBLUE     9
//#define LTGREEN   10
//#define LTCYAN    11
//#define LTRED     12
//#define LTMAGENTA 13
//#define LTYELLOW  14
//#define LTWHITE   15

inline void consoleColorInit()
{
	if( ::colorMode == win32 )
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(h,&csbi);
		consoleColorReset = csbi.wAttributes;
	}
}

inline void consoleColorRestore()
{
	if( ::colorMode == win32 )
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(h, consoleColorReset);
	}
	else if( ::colorMode == ansi )
		cerr << ANSI_RESET << flush;
}

inline void consoleColorSet( colorval color )
{
	if( ::colorMode == win32 )
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(h,&csbi);
		SetConsoleTextAttribute(h, ((csbi.wAttributes | 0x0f) ^ 0x0f) | colorTable[color].winVal );
	}
	else if( ::colorMode == ansi )
		cerr << colorTable[color].ansiStr << flush;
}


inline std::ostream& operator << ( std::ostream& os, colorText c )
{
	if( ::colorMode == win32 )
	{
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(h,&csbi);
		SetConsoleTextAttribute(h, ((csbi.wAttributes | 0x0f) ^ 0x0f) | colorTable[c.color].winVal );
		os << c.txt;
		SetConsoleTextAttribute(h, csbi.wAttributes);
	}
	else if( ::colorMode == ansi )
		os << colorTable[c.color].ansiStr << c.txt << ANSI_RESET;
	else
		os << c.txt;
	return os;
}

#endif

#define _BLACK(t)      (colorText){t,colorText::BLACK}
#define _BLUE(t)       (colorText){t,colorText::BLUE}
#define _GREEN(t)      (colorText){t,colorText::GREEN}
#define _CYAN(t)       (colorText){t,colorText::CYAN}
#define _RED(t)        (colorText){t,colorText::RED}
#define _MAGENTA(t)    (colorText){t,colorText::MAGENTA}
#define _YELLOW(t)     (colorText){t,colorText::YELLOW}
#define _WHITE(t)      (colorText){t,colorText::WHITE}

#define _GRAY(t)       (colorText){t,colorText::GRAY}
#define _LTBLUE(t)     (colorText){t,colorText::LTBLUE}
#define _LTGREEN(t)    (colorText){t,colorText::LTGREEN}
#define _LTCYAN(t)     (colorText){t,colorText::LTCYAN}
#define _LTRED(t)      (colorText){t,colorText::LTRED}
#define _LTMAGENTA(t)  (colorText){t,colorText::LTMAGENTA}
#define _LTYELLOW(t)   (colorText){t,colorText::LTYELLOW}
#define _LTWHITE(t)    (colorText){t,colorText::LTWHITE}

#endif
