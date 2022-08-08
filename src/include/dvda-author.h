/*
File:    dvda-author.h
Purpose: initializing macros/structures

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

#ifndef  DVDA_AUTHOR_H
#define  DVDA_AUTHOR_H

// Always end the lists of options with an option that has :: (to ensure space for spec_index(2)[2]) */
#include "commonvars.h"
#include "structures.h"
#include "c_utils.h"
/* allowed options for lexer.h */

// g, j, s are treated by getopt as non-argumental although they take a non-limited number of (non-getopt compliant) arguments

void normalize_temporary_paths(pic* img, globalData* globals);
#define  PAL_NORM "pal"
command_t* lexer_analysis(command_t* command, lexer_t* lexer, const char* config_file, bool config_type, globalData*);
#endif // DVDA-AUTHOR_H_INCLUDED

