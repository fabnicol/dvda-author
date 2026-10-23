#include "config.h"
#include <stdio.h>
#include <assert.h>
#include "SetRegion2D.hh"

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// The type of region we're testing.
typedef SetRegion2D<int16_t,int32_t> Region;
struct Extent { int16_t m_nY; int16_t m_nXStart; int16_t m_nXEnd; };

void
PrintRegion (const Region &a_rRegion)
{
	Region::ConstIterator itHere;
		// Used to loop through the region.
	
	// Print the region header.
	printf ("( ");

	// Print each extent.
	for (itHere = a_rRegion.Begin();
		 itHere != a_rRegion.End();
		 itHere++)
	{
		// Print the extent.
		printf ("[%d,%d-%d] ", (*itHere).m_tnY, (*itHere).m_tnXStart,
			(*itHere).m_tnXEnd);
	}
	
	// Print the region trailer.
	printf (")");
}

int
main (void)
{
	Status_t eStatus;
		// An error that may occur.
	Region oSrcRegion, oTestRegion;
		// Regions being exercised.
	
	// No errors yet.
	eStatus = g_kNoError;
	
	// Initialize our regions.
	oSrcRegion.Init (eStatus);
	if (eStatus != g_kNoError)
		{ printf ("oSrcRegion.Init() failed\n"); return 1; }
	oTestRegion.Init (eStatus);
	if (eStatus != g_kNoError)
		{ printf ("oTestRegion.Init() failed\n"); return 1; }

	// Set up an example region.
	oSrcRegion.Union (eStatus, (int16_t)1, (int16_t)1, (int16_t)4);
	if (eStatus != g_kNoError)
		{ printf ("oSrcRegion.Union() failed\n"); return 1; }
	oSrcRegion.Union (eStatus, (int16_t)1, (int16_t)5, (int16_t)7);
	if (eStatus != g_kNoError)
		{ printf ("oSrcRegion.Union() failed\n"); return 1; }
	oSrcRegion.Union (eStatus, (int16_t)1, (int16_t)10, (int16_t)20);
	if (eStatus != g_kNoError)
		{ printf ("oSrcRegion.Union() failed\n"); return 1; }
	printf ("Original region: ");
	PrintRegion (oSrcRegion);
	printf ("\n");

	// Now test various additions.
	{
		Extent aExtents[] = { {1, 4, 5}, {1, 15, 16}, {1, 6, 10},
				{1, 8, 9}, {1, 6, 9}, {1, 9, 21}, {1, 9, 18},
				{1, 10, 15}, {1, 25, 30} };
			// Extents to test.
		unsigned int i;
			// Used to loop through extents to test.

		// Test each extent in the list.
		for (i = 0; i < sizeof (aExtents) / sizeof (aExtents[0]); i++)
		{
			oTestRegion.Assign (eStatus, oSrcRegion);
			if (eStatus != g_kNoError)
				{ printf ("oTestRegion.Assign() failed\n"); return 1; }
			oTestRegion.Union (eStatus, aExtents[i].m_nY,
				aExtents[i].m_nXStart, aExtents[i].m_nXEnd);
			if (eStatus != g_kNoError)
				{ printf ("oTestRegion.Union() failed\n"); return 1; }
			printf ("After adding [%d,%d-%d]: ", aExtents[i].m_nY,
				aExtents[i].m_nXStart, aExtents[i].m_nXEnd);
			PrintRegion (oTestRegion);
			printf ("\n");
		}
	}

	// Now test various subtractions.
	{
		Extent aExtents[] = { {1, 0, 2}, {1, 1, 2}, {1, 3, 5},
				{1, 3, 6}, {1, 4, 6}, {1, 3, 12}, {1, 0, 12},
				{1, 22, 25}, {1, 15, 17}, {1, 5, 7} };
			// Extents to test.
		unsigned int i;
			// Used to loop through extents to test.

		// Test each extent in the list.
		for (i = 0; i < sizeof (aExtents) / sizeof (aExtents[0]); i++)
		{
			oTestRegion.Assign (eStatus, oSrcRegion);
			if (eStatus != g_kNoError)
				{ printf ("oTestRegion.Assign() failed\n"); return 1; }
			oTestRegion.Subtract (eStatus, aExtents[i].m_nY,
				aExtents[i].m_nXStart, aExtents[i].m_nXEnd);
			if (eStatus != g_kNoError)
				{ printf ("oTestRegion.Subtract() failed\n");
					return 1; }
			printf ("After subtracting [%d,%d-%d]: ", aExtents[i].m_nY,
				aExtents[i].m_nXStart, aExtents[i].m_nXEnd);
			PrintRegion (oTestRegion);
			printf ("\n");
		}
	}

	/*
		Before Subtract:
		( [0,24-26] [1,24-26] )
		Subtract from it:
		( [0,10-12] [0,24-26] [1,10-12] [1,24-26] )
		Result:
		( [0,12-24] [1,12-24] )
	*/
	oSrcRegion.Clear();
	oTestRegion.Clear();
	oSrcRegion.Union (eStatus, 0, 24, 26);
	if (eStatus != g_kNoError)
		{ printf ("oSrcRegion.Union() failed\n");
			return 1; }
	oSrcRegion.Union (eStatus, 1, 24, 26);
	if (eStatus != g_kNoError)
		{ printf ("oSrcRegion.Union() failed\n");
			return 1; }
	oTestRegion.Union (eStatus, 0, 10, 12);
	if (eStatus != g_kNoError)
		{ printf ("oTestRegion.Union() failed\n");
			return 1; }
	oTestRegion.Union (eStatus, 0, 24, 26);
	if (eStatus != g_kNoError)
		{ printf ("oTestRegion.Union() failed\n");
			return 1; }
	oTestRegion.Union (eStatus, 1, 10, 12);
	if (eStatus != g_kNoError)
		{ printf ("oTestRegion.Union() failed\n");
			return 1; }
	oTestRegion.Union (eStatus, 1, 24, 26);
	if (eStatus != g_kNoError)
		{ printf ("oTestRegion.Union() failed\n");
			return 1; }

	oSrcRegion.Subtract (eStatus, oTestRegion);
	if (eStatus != g_kNoError)
		{ printf ("oSrc.Subtract (oTestRegion) failed\n");
			return 1; }

	// All done.
	return 0;
}
