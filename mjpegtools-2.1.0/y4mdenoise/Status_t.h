#ifndef __STATUS_T_H__
#define __STATUS_T_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// The result of an operation.
enum Status_t
{
	g_kNoError,			// No error (i.e. success)
	g_kInternalError,	// Unspecified internal error.

	g_kOutOfMemory,		// Free store exhausted.
};

#endif // __STATUS_T_H__
