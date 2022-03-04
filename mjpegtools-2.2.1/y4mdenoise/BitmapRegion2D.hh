#ifndef __BITMAPREGION2D_H__
#define __BITMAPREGION2D_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// BitmapRegion2D tracks a 2-dimensional region of arbitrary points,
// implemented with a bitmap.

#include <assert.h>
#include <string.h>
#include <strings.h>
#include "Region2D.hh"



// Define this to compile in code to double-check and debug the region.
#ifndef NDEBUG
//	#define DEBUG_BITMAPREGION2D
#endif // NDEBUG

// Define this to enable the more specific method to subtract one
// bitmap-region from another.
// Disabled because, given how the code currently uses it, it's slightly
// slower (adding maybe 1% to the runtime).
//#define DIRECT_BITMAPREGION_SUBTRACT



// Part of BitmapRegion2D<> breaks gcc 2.95.  An earlier arrangement of
// the code also broke gcc 3.3, but that seems to be in remission now.
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define GCC_295_WORKAROUND
#endif
#endif



// The 2-dimensional region class.  Parameterized by the numeric type
// to use for point indices, and the numeric type to use to count the
// contained number of points.
template <class INDEX, class SIZE>
class BitmapRegion2D : public Region2D<INDEX,SIZE>
{
private:
	typedef Region2D<INDEX,SIZE> BaseClass;
		// Keep track of who our base class is.
public:
	typedef typename BaseClass::Extent Extent;
		// (Wouldn't we automatically have access to Extent because
		// we're a subclass of Region2D<>?  Why is this needed?)

	BitmapRegion2D();
		// Default constructor.  Must be followed by Init().

	BitmapRegion2D (Status_t &a_reStatus, INDEX a_tnWidth,
			INDEX a_tnHeight);
		// Initializing constructor.  Creates an empty region with the
		// given extent.

	BitmapRegion2D (Status_t &a_reStatus,
			const BitmapRegion2D<INDEX,SIZE> &a_rOther);
		// Copy constructor.

	void Init (Status_t &a_reStatus, INDEX a_tnWidth, INDEX a_tnHeight);
		// Initializer.  Must be called on default-constructed regions.

	template <class OTHER>
	void Assign (Status_t &a_reStatus, const OTHER &a_rOther);
		// Make the current region a copy of the other region.

	void Assign (Status_t &a_reStatus,
			const BitmapRegion2D<INDEX,SIZE> &a_rOther);
		// Make the current region a copy of the other region.

	virtual ~BitmapRegion2D();
		// Destructor.

	SIZE NumberOfPoints (void) const;
		// Return the total number of points contained by the region.

	void Clear (void);
		// Clear the region, emptying it of all extents.

	void Union (INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd);
		// Add the given horizontal extent to the region.  Note that
		// a_tnXEnd is technically one past the end of the extent.

	void Union (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd);
		// Add the given horizontal extent to the region.  Note that
		// a_tnXEnd is technically one past the end of the extent.
		// (This version is for interface compatibility with the
		// set-based region.)

	void Union (Status_t &a_reStatus,
			const BitmapRegion2D<INDEX,SIZE> &a_rOther);
		// Make the current region represent the union between itself
		// and the other given region.

	void Merge (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd);
		// Merge this extent into the current region.
		// The new extent can't intersect the region in any way.

	void Merge (BitmapRegion2D<INDEX,SIZE> &a_rOther);
		// Merge the other region into ourselves, emptying the other
		// region.  Like Union(), but doesn't allocate any new memory.
		// Also, the other region can't intersect us in any way, and
		// both regions must use the same allocator.

	void Move (BitmapRegion2D<INDEX,SIZE> &a_rOther);
		// Move the contents of the other region into the current
		// region.

	bool CanMove (const BitmapRegion2D<INDEX,SIZE> &a_rOther) const;
		// Returns true if the other region's contents can be moved
		// into the current region.

	bool CanMove (const Region2D<INDEX,SIZE> &a_rOther) const
			{ return false; }
	void Move (Region2D<INDEX,SIZE> &a_rOther) { assert (false); }
		// (We can move between BitmapRegion2D<>s, but not any arbitrary
		// Region2D<> subclass.)

	void Intersection (Status_t &a_reStatus,
			const BitmapRegion2D<INDEX,SIZE> &a_rOther);
		// Make the current region represent the intersection between
		// itself and the other given region.

	void Subtract (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd);
		// Subtract the given horizontal extent from the region.  Note
		// that a_tnXEnd is technically one past the end of the extent.

	template <class OTHER>
	void Subtract (Status_t &a_reStatus, const OTHER &a_rOther);
		// Subtract the other region from the current region, i.e.
		// remove from the current region any areas that exist in the
		// other region.

	#ifdef DIRECT_BITMAPREGION_SUBTRACT
	void Subtract (Status_t &a_reStatus,
			const BitmapRegion2D<INDEX,SIZE> &a_rOther);
	#endif // DIRECT_BITMAPREGION_SUBTRACT
		// Subtract the other region from the current region, i.e.
		// remove from the current region any areas that exist in the
		// other region.

	bool DoesContainPoint (INDEX a_tnY, INDEX a_tnX) const;
		// Returns true if the region contains the given point.

	void SetPoint (INDEX a_tnY, INDEX a_tnX);
		// Add the given point to the region.
		// (Basically a one-point Union().)

	// A structure that implements flood-fills using BitmapRegion2D<> to
	// do the work.  (The definition of this class follows the
	// definition of Region<>.)
	class FloodFillControl;

	template <class CONTROL>
	void FloodFill (Status_t &a_reStatus, CONTROL &a_rControl,
			bool a_bVerify, bool a_bExpand);
		// Flood-fill the current region.
		// If a_bVerify is true, all existing region points are re-tested.
		// If a_bExpand is true, all points bordering the current region
		// are tested for inclusion.

	template <class OTHER>
	void MakeBorder (Status_t &a_reStatus, const OTHER &a_rOther);
		// Make the current region represent the border of the other
		// region, i.e. every pixel that's adjacent to a pixel that's
		// in the region, but isn't already in the region.

	// An iterator for the bitmap-oriented region.
	class ConstIterator
	{
	protected:
		friend class BitmapRegion2D<INDEX,SIZE>;
			// Let the region implement our operations.
		const BitmapRegion2D<INDEX,SIZE> *m_pThis;
			// The region that we're iterating through.
		Extent m_oExtent;
			// The extent we currently represent.
	public:
		ConstIterator() { m_pThis = NULL; }
		ConstIterator (const ConstIterator &a_rOther)
			: m_pThis (a_rOther.m_pThis),
			m_oExtent (a_rOther.m_oExtent) {}
		const Extent &operator*() const { return m_oExtent; }
		ConstIterator& operator++();
		ConstIterator operator++(int) { ConstIterator oTmp = *this;
			++*this; return oTmp; }
		ConstIterator& operator--();
		ConstIterator operator--(int) { ConstIterator oTmp = *this;
			--*this; return oTmp; }
		bool operator== (const ConstIterator &a_rOther) const
			{ return (m_pThis == a_rOther.m_pThis
				&& m_oExtent == a_rOther.m_oExtent) ? true : false; }
		bool operator!= (const ConstIterator &a_rOther) const
			{ return (m_pThis != a_rOther.m_pThis
				|| m_oExtent != a_rOther.m_oExtent) ? true : false; }
	};

	ConstIterator Begin (void) const;
	ConstIterator End (void) const;
		// Allow our client to iterate through the extents & get their
		// values.

	void UnionSurroundingExtents (Status_t &a_reStatus,
			INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd);
		// Add all extents surrounding the given extent.
		// Used by FloodFill() and MakeBorder().

	void IteratorForward (ConstIterator &a_ritHere) const;
		// Move one of our iterators forward.

	void IteratorBackward (ConstIterator &a_ritHere) const;
		// Move one of our iterators backward.

	static int FindFirstSetBit (unsigned int a_nWord, int a_nSkip);
	static int FindFirstClearBit (unsigned int a_nWord, int a_nSkip);
	static int FindLastSetBit (unsigned int a_nWord, int a_nSkip);
	static int FindLastClearBit (unsigned int a_nWord, int a_nSkip);
		// Find the index of the first/last set/clear bit, skipping
		// the first/last a_nSkip bits.
		// Used to implement iterator increment/decrement.

private:
	INDEX m_tnWidth, m_tnHeight;
		// The extent of the bitmap.

	unsigned int *m_pnPoints;
		// The bitmap representing the region.

	SIZE m_tnBitmapInts;
		// The number of integers in our bitmap.

	INDEX m_tnXMin, m_tnXMax, m_tnYMin, m_tnYMax;
		// The portion of the bitmap that may contain extents.
		// Used to speed up IteratorForward().

	static int8_t m_anSetBitsPerByte[256];
		// The number of set bits in every possible byte value.

	static bool m_bSetBitsPerByte;
		// Whether the set-bits-per-byte table has been set up.

#ifndef NDEBUG

// Count the number of region objects in existence.
private:
	static uint32_t sm_ulInstances;
public:
	static uint32_t GetInstances (void) { return sm_ulInstances; }

#endif // NDEBUG
};



// A structure that implements flood-fills using BitmapRegion2D<> to
// do the work.
template <class INDEX, class SIZE>
class BitmapRegion2D<INDEX,SIZE>::FloodFillControl
#ifndef GCC_295_WORKAROUND
	: public Region2D<INDEX,SIZE>::template
		FloodFillControl<BitmapRegion2D<INDEX,SIZE> >
#endif // ! GCC_295_WORKAROUND
{
private:
	typedef Region2D<INDEX,SIZE> BaseRegion;
	typedef BitmapRegion2D<INDEX,SIZE> OurRegion;
	typedef typename BaseRegion::template FloodFillControl<OurRegion>
			BaseClass;
		// Keep track of who our base class is.
public:
#ifdef GCC_295_WORKAROUND
	// (Although these fields are public, they should be considered
	// opaque to the client.)

	BitmapRegion2D<INDEX,SIZE> m_oToDo;
		// Extents that still need to be be tested.

	BitmapRegion2D<INDEX,SIZE> m_oAlreadyDone;
		// Extents that have been tested.

	BitmapRegion2D<INDEX,SIZE> m_oNextToDo;
		// Extents contiguous with those that have just been added
		// to the flood-filled area.

	typedef typename BitmapRegion2D<INDEX,SIZE>::ConstIterator
		ConstIterator;
	typedef typename BitmapRegion2D<INDEX,SIZE>::Extent Extent;
		// The iterator/extent type for running through the above
		// regions.
#endif // GCC_295_WORKAROUND

	FloodFillControl();
		// Default constructor.  Must be followed by a call to Init().

	FloodFillControl (Status_t &a_reStatus, INDEX a_tnWidth,
			INDEX a_tnHeight);
		// Initializing constructor.

	void Init (Status_t &a_reStatus, INDEX a_tnWidth, INDEX a_tnHeight);
		// Initializer.  Must be called on default-constructed objects.
};



// Default constructor.  Must be followed by Init().
template <class INDEX, class SIZE>
BitmapRegion2D<INDEX,SIZE>::BitmapRegion2D()
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // NDEBUG

	// No bitmap yet.
	m_tnWidth = m_tnHeight = INDEX (0);
	m_tnXMin = m_tnXMax = m_tnYMin = m_tnYMax = INDEX (0);
	m_pnPoints = NULL;
	m_tnBitmapInts = SIZE (0);
}



// Initializing constructor.  Creates an empty region.
template <class INDEX, class SIZE>
BitmapRegion2D<INDEX,SIZE>::BitmapRegion2D (Status_t &a_reStatus,
	INDEX a_tnWidth, INDEX a_tnHeight)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // NDEBUG

	// No bitmap yet.
	m_tnWidth = m_tnHeight = INDEX (0);
	m_tnXMin = m_tnXMax = m_tnYMin = m_tnYMax = INDEX (0);
	m_pnPoints = NULL;
	m_tnBitmapInts = SIZE (0);

	// Save the width & height.
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;
	m_tnXMin = a_tnWidth;
	m_tnXMax = INDEX (0);
	m_tnYMin = a_tnHeight;
	m_tnYMax = INDEX (0);

	// Allocate the bitmap.
	m_tnBitmapInts = (SIZE (m_tnWidth) * SIZE (m_tnHeight))
		/ (g_knBitsPerByte * sizeof (unsigned int)) + 1;
	m_pnPoints = new unsigned int[m_tnBitmapInts];
	if (m_pnPoints == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}

	// No points yet.
	for (SIZE i = 0; i < m_tnBitmapInts; ++i)
		m_pnPoints[i] = 0U;
}



// Copy constructor.
template <class INDEX, class SIZE>
BitmapRegion2D<INDEX,SIZE>::BitmapRegion2D (Status_t &a_reStatus,
	const BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // NDEBUG

	// No bitmap yet.
	m_tnWidth = m_tnHeight = INDEX (0);
	m_pnPoints = NULL;
	m_tnBitmapInts = SIZE (0);

	// Copy the width, height, and active area.
	m_tnWidth = a_rOther.m_tnWidth;
	m_tnHeight = a_rOther.m_tnHeight;
	m_tnXMin = a_rOther.m_tnXMin;
	m_tnXMax = a_rOther.m_tnXMax;
	m_tnYMin = a_rOther.m_tnYMin;
	m_tnYMax = a_rOther.m_tnYMax;

	// Allocate the bitmap.
	m_tnBitmapInts = (SIZE (m_tnWidth) * SIZE (m_tnHeight))
		/ (g_knBitsPerByte * sizeof (unsigned int)) + 1;
	m_pnPoints = new unsigned int[m_tnBitmapInts];
	if (m_pnPoints == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}

	// Copy the bitmap.
	memcpy (m_pnPoints, a_rOther.m_pnPoints,
		m_tnBitmapInts * sizeof (unsigned int));
}



// Initializer.  Must be called on default-constructed regions.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Init (Status_t &a_reStatus, INDEX a_tnWidth,
	INDEX a_tnHeight)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Save the width & height.
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;
	m_tnXMin = a_tnWidth;
	m_tnXMax = INDEX (0);
	m_tnYMin = a_tnHeight;
	m_tnYMax = INDEX (0);

	// Allocate the bitmap.
	m_tnBitmapInts = (SIZE (m_tnWidth) * SIZE (m_tnHeight))
		/ (g_knBitsPerByte * sizeof (unsigned int)) + 1;
	m_pnPoints = new unsigned int[m_tnBitmapInts];
	if (m_pnPoints == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}

	// No points yet.
	for (SIZE i = 0; i < m_tnBitmapInts; ++i)
		m_pnPoints[i] = 0U;
}



// Make the current region a copy of the other region.
template <class INDEX, class SIZE>
template <class OTHER>
void
BitmapRegion2D<INDEX,SIZE>::Assign (Status_t &a_reStatus,
	const OTHER &a_rOther)
{
	typename OTHER::ConstIterator itExtent;
		// Used to loop through extents to copy.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Clear our contents, in preparation for being assigned.
	Clear();

	// Loop through the other region's extents, clip them to the current
	// region's boundaries, and add them to the current region.
	for (itExtent = a_rOther.Begin();
		 itExtent != a_rOther.End();
		 ++itExtent)
	{
		// Get the extent.
		typename OTHER::Extent oExtent = *itExtent;

		// Add it to the current region.
		Union (oExtent.m_tnY, oExtent.m_tnXStart, oExtent.m_tnXEnd);
	}
}



// Make the current region a copy of the other region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Assign (Status_t &a_reStatus,
	const BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// If the region is not the exact same size, use the more
	// generic version.
	if (m_tnWidth != a_rOther.m_tnWidth
		|| m_tnHeight != a_rOther.m_tnHeight)
	{
		Assign<BitmapRegion2D<INDEX,SIZE> > (a_reStatus, *this);
		return;
	}

	// Make sure the region is the exact same size.
	assert (m_tnWidth == a_rOther.m_tnWidth
		&& m_tnHeight == a_rOther.m_tnHeight);

	// Copy the other region's bitmap.
	memcpy (m_pnPoints, a_rOther.m_pnPoints,
		m_tnBitmapInts * sizeof (unsigned int));

	// Copy its active area.
	m_tnXMin = a_rOther.m_tnXMin;
	m_tnXMax = a_rOther.m_tnXMax;
	m_tnYMin = a_rOther.m_tnYMin;
	m_tnYMax = a_rOther.m_tnYMax;
}



// Destructor.
template <class INDEX, class SIZE>
BitmapRegion2D<INDEX,SIZE>::~BitmapRegion2D()
{
	// Free up the bitmap.
	delete[] m_pnPoints;

	// One less instance.
	#ifndef NDEBUG
	--sm_ulInstances;
	#endif // NDEBUG
}



// Return the total number of points contained by the region.
template <class INDEX, class SIZE>
SIZE
BitmapRegion2D<INDEX,SIZE>::NumberOfPoints (void) const
{
	// To make this fast, we precalculate a table, indexed by byte
	// value, that contains the number of set bits in that byte.
	if (!m_bSetBitsPerByte)
	{
		for (int i = 0; i < 256; ++i)
		{
			int8_t nBits = 0;
			uint8_t nVal = uint8_t (i);
			for (int j = 0; j < 8; ++j)
			{
				if (nVal & 1)
					++nBits;
				nVal >>= 1;
			}
			m_anSetBitsPerByte[i] = nBits;
		}

		// We only have to do this once.
		m_bSetBitsPerByte = true;
	}

	SIZE tnI, tnLimit;
		// Used to loop through the bitmap's bytes.
	SIZE tnPoints;
		// The number of points we tally.

	// Loop through the bitmap's bytes, look up how many set bits
	// there are in each byte, count them up.
	uint8_t *pnPoints = (uint8_t *) m_pnPoints;
	tnLimit = m_tnBitmapInts
		* (sizeof (unsigned int) / sizeof (uint8_t));
	tnPoints = 0;
	for (tnI = 0; tnI < tnLimit; ++tnI)
		tnPoints += m_anSetBitsPerByte[pnPoints[tnI]];

	// Return the number of points we counted.
	return tnPoints;
}



// Clear the region, emptying it of all extents.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Clear (void)
{
	// Clear all the bits.
	memset (m_pnPoints, 0, ARRAYSIZE (unsigned int, m_tnBitmapInts));

	// Reset the active area.
	m_tnXMin = m_tnWidth;
	m_tnXMax = INDEX (0);
	m_tnYMin = m_tnHeight;
	m_tnYMax = INDEX (0);
}



// Add the given point to the region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::SetPoint (INDEX a_tnY, INDEX a_tnX)
{
	// Make sure this point is in range.
	assert (a_tnY >= 0 && a_tnY < m_tnHeight);
	assert (a_tnX >= 0 && a_tnX < m_tnWidth);

	// Factor this extent into the active area.
	m_tnXMin = Min (m_tnXMin, a_tnX);
	m_tnXMax = Max (m_tnXMax, INDEX (a_tnX + 1));
	m_tnYMin = Min (m_tnYMin, a_tnY);
	m_tnYMax = Max (m_tnYMax, a_tnY);

	// Set the point.
	SIZE tnI = a_tnY * m_tnWidth + a_tnX;
	m_pnPoints[tnI / (g_knBitsPerByte * sizeof (unsigned int))]
		|= (1U << (tnI % (g_knBitsPerByte * sizeof (unsigned int))));
}



// Add the given horizontal extent to the region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Union (INDEX a_tnY, INDEX a_tnXStart,
	INDEX a_tnXEnd)
{
	SIZE tnI;
		// The index of the bitmap integer to modify.
	INDEX tnX;
		// Used to loop through points.

	// Make sure they gave us a non-empty extent.
	assert (a_tnXStart < a_tnXEnd);

	// If this extent is completely outside the region, skip it.
	if (a_tnY < 0 || a_tnY >= m_tnHeight
	|| a_tnXStart >= m_tnWidth || a_tnXEnd <= 0)
		return;

	// If this extent is partially outside the region, clip it.
	if (a_tnXStart < 0)
		a_tnXStart = 0;
	if (a_tnXEnd > m_tnWidth)
		a_tnXEnd = m_tnWidth;
	if (a_tnXStart < 0)
		a_tnXStart = 0;
	if (a_tnXEnd > m_tnWidth)
		a_tnXEnd = m_tnWidth;

	// Factor this extent into the active area.
	m_tnXMin = Min (m_tnXMin, a_tnXStart);
	m_tnXMax = Max (m_tnXMax, a_tnXEnd);
	m_tnYMin = Min (m_tnYMin, a_tnY);
	m_tnYMax = Max (m_tnYMax, a_tnY);

	// Loop through all the points, set them.
	for (tnX = a_tnXStart; tnX < a_tnXEnd; ++tnX)
	{
		tnI = a_tnY * m_tnWidth + tnX;
		m_pnPoints[tnI / (g_knBitsPerByte
				* sizeof (unsigned int))]
			|= (1U << (tnI % (g_knBitsPerByte
				* sizeof (unsigned int))));
	}
}



// Add the given horizontal extent to the region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Union (Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Call the non-failing version.
	Union (a_tnY, a_tnXStart, a_tnXEnd);
}



// Make the current region represent the union between itself
// and the other given region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Union (Status_t &a_reStatus,
	const BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the region is the exact same size.
	assert (m_tnWidth == a_rOther.m_tnWidth
		&& m_tnHeight == a_rOther.m_tnHeight);

	// Unify with the other region's bitmap.
	for (SIZE i = 0; i < m_tnBitmapInts; ++i)
		m_pnPoints[i] |= a_rOther.m_pnPoints[i];

	// Factor the other region's active area into our own.
	m_tnXMin = Min (m_tnXMin, a_rOther.m_tnXMin);
	m_tnXMax = Max (m_tnXMax, a_rOther.m_tnXMax);
	m_tnYMin = Min (m_tnYMin, a_rOther.m_tnYMin);
	m_tnYMax = Max (m_tnYMax, a_rOther.m_tnYMax);
}



// Merge this extent into the current region.
// The new extent can't intersect the region in any way.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Merge (Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd)
{
	// For a bitmap, Union() is the same thing.
	Union (a_reStatus, a_tnY, a_tnXStart, a_tnXEnd);
}



// Merge the other region into ourselves, emptying the other region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Merge (BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// Make sure the region is the exact same size.
	assert (m_tnWidth == a_rOther.m_tnWidth
		&& m_tnHeight == a_rOther.m_tnHeight);

	// For a bitmap, this is easy.
	Status_t eStatus = g_kNoError;
	Union (eStatus, a_rOther);
	assert (eStatus == g_kNoError);
	a_rOther.Clear();
}



// Move the contents of the other region into the current region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Move (BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// Make sure we're allowed to do this.
	assert (CanMove (a_rOther));

	// Empty the current region.
	Clear();

	// Swap the contained bitmaps.
	unsigned int *pnPoints = m_pnPoints;
	m_pnPoints = a_rOther.m_pnPoints;
	a_rOther.m_pnPoints = pnPoints;

	// Swap the active area.
	INDEX tnActive = a_rOther.m_tnXMin;
	a_rOther.m_tnXMin = m_tnXMin;
	m_tnXMin = tnActive;
	tnActive = a_rOther.m_tnXMax;
	a_rOther.m_tnXMax = m_tnXMax;
	m_tnXMax = tnActive;
	tnActive = a_rOther.m_tnYMin;
	a_rOther.m_tnYMin = m_tnYMin;
	m_tnYMin = tnActive;
	tnActive = a_rOther.m_tnYMax;
	a_rOther.m_tnYMax = m_tnYMax;
	m_tnYMax = tnActive;
}



// Returns true if the other region's contents can be moved
// into the current region.
template <class INDEX, class SIZE>
bool
BitmapRegion2D<INDEX,SIZE>::CanMove
	(const BitmapRegion2D<INDEX,SIZE> &a_rOther) const
{
	// We can do this if the regions are the same size.
	return (m_tnWidth == a_rOther.m_tnWidth
		&& m_tnHeight == a_rOther.m_tnHeight);
}



// Make the current region represent the intersection between
// itself and the other given region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Intersection (Status_t &a_reStatus,
	const BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the region is the exact same size.
	assert (m_tnWidth == a_rOther.m_tnWidth
		&& m_tnHeight == a_rOther.m_tnHeight);

	// Intersect with the other region's bitmap.
	for (SIZE i = 0; i < m_tnBitmapInts; ++i)
		m_pnPoints[i] &= a_rOther.m_pnPoints[i];

	// The active area may have shrunk, but we don't recalculate it;
	// that would take too long.
}



// Subtract the given horizontal extent from the region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Subtract (Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd)
{
	SIZE tnI;
		// The index of the bitmap integer to modify.
	INDEX tnX;
		// Used to loop through points.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a non-empty extent.
	assert (a_tnXStart < a_tnXEnd);

	// If this extent is completely outside the region, skip it.
	if (a_tnY < 0 || a_tnY >= m_tnHeight
	|| a_tnXStart >= m_tnWidth || a_tnXEnd <= 0)
		return;

	// If this extent is partially outside the region, clip it.
	if (a_tnXStart < 0)
		a_tnXStart = 0;
	if (a_tnXEnd > m_tnWidth)
		a_tnXEnd = m_tnWidth;
	if (a_tnXStart < 0)
		a_tnXStart = 0;
	if (a_tnXEnd > m_tnWidth)
		a_tnXEnd = m_tnWidth;

	// Loop through all the points, clear them.
	for (tnX = a_tnXStart; tnX < a_tnXEnd; ++tnX)
	{
		tnI = a_tnY * m_tnWidth + tnX;
		m_pnPoints[tnI / (g_knBitsPerByte
				* sizeof (unsigned int))]
			&= (~(1U << (tnI % (g_knBitsPerByte
				* sizeof (unsigned int)))));
	}

	// The active area may have shrunk, but we don't recalculate it;
	// that would take too long.
}



// Subtract the other region from the current region, i.e.
// remove from the current region any areas that exist in the
// other region.
template <class INDEX, class SIZE>
template <class OTHER>
void
BitmapRegion2D<INDEX,SIZE>::Subtract (Status_t &a_reStatus,
	const OTHER &a_rOther)
{
	typename OTHER::ConstIterator itExtent;
		// Used to loop through the other region's extents.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Loop through the other region's extents, subtract each one.
	for (itExtent = a_rOther.Begin();
		 itExtent != a_rOther.End();
		 ++itExtent)
	{
		Subtract (a_reStatus, (*itExtent).m_tnY, (*itExtent).m_tnXStart,
			(*itExtent).m_tnXEnd);
		assert (a_reStatus == g_kNoError); 	// (should always succeed.)
		if (a_reStatus != g_kNoError)
			return;
	}

	// The active area may have shrunk, but we don't recalculate it;
	// that would take too long.
}



#ifdef DIRECT_BITMAPREGION_SUBTRACT

// Subtract the other region from the current region, i.e.
// remove from the current region any areas that exist in the
// other region.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::Subtract (Status_t &a_reStatus,
	const BitmapRegion2D<INDEX,SIZE> &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the region is the exact same size.
	assert (m_tnWidth == a_rOther.m_tnWidth
		&& m_tnHeight == a_rOther.m_tnHeight);

	// If either region is empty, we're done.
	if (m_tnXMin >= m_tnXMax || m_tnYMin > m_tnYMax
			|| a_rOther.m_tnXMin >= a_rOther.m_tnXMax
			|| a_rOther.m_tnYMin > a_rOther.m_tnYMax)
		return;

	// Calculate the active area common to the two regions.
	INDEX tnXMin = Max (m_tnXMin, a_rOther.m_tnXMin);
	INDEX tnXMax = Min (m_tnXMax, a_rOther.m_tnXMax);
	INDEX tnYMin = Max (m_tnYMin, a_rOther.m_tnYMin);
	INDEX tnYMax = Min (m_tnYMax, a_rOther.m_tnYMax);

	// If the two active areas don't overlap, we're done.
	if (tnXMin >= tnXMax || tnYMin > tnYMax)
		return;

	// Loop through the common active area, subtract extents.
	// If the active area encompasses most of the X range, do this faster.
	INDEX tnBitsPerWord = g_knBitsPerByte * sizeof (unsigned int);
	if (tnXMin <= tnBitsPerWord && tnXMax >= m_tnWidth - tnBitsPerWord)
	{
		// Find the range of words that correspond to the active area.
		SIZE tnWordIndex = (SIZE (tnYMin) * SIZE (m_tnWidth)
			+ SIZE (tnXMin)) >> Limits<unsigned int>::Log2Bits;
		SIZE tnLastWordIndex = (SIZE (tnYMax) * SIZE (m_tnWidth)
			+ SIZE (tnXMax - INDEX (1)))
				>> Limits<unsigned int>::Log2Bits;

		// Loop through these words, subtract extents.
		for (SIZE tnI = tnWordIndex; tnI <= tnLastWordIndex; ++tnI)
			m_pnPoints[tnI] &= (~(a_rOther.m_pnPoints[tnI]));
	}
	else
	{
		// Loop through the common active area of each line.
		for (INDEX tnY = tnYMin; tnY <= tnYMax; ++tnY)
		{
			// Find the range of words that correspond to the active area
			// on this line.
			SIZE tnWordIndex = (SIZE (tnY) * SIZE (m_tnWidth)
				+ SIZE (tnXMin)) >> Limits<unsigned int>::Log2Bits;
			SIZE tnLastWordIndex = (SIZE (tnY) * SIZE (m_tnWidth)
				+ SIZE (tnXMax - INDEX (1)))
					>> Limits<unsigned int>::Log2Bits;

			// Loop through these words, subtract extents.
			for (SIZE tnI = tnWordIndex; tnI <= tnLastWordIndex; ++tnI)
				m_pnPoints[tnI] &= (~(a_rOther.m_pnPoints[tnI]));
		}
	}

	// The active area may have shrunk, but we don't recalculate it;
	// that would take too long.
}

#endif // DIRECT_BITMAPREGION_SUBTRACT



// Returns true if the region contains the given point.
template <class INDEX, class SIZE>
bool
BitmapRegion2D<INDEX,SIZE>::DoesContainPoint
	(INDEX a_tnY, INDEX a_tnX) const
{
	// Make sure the indices are within our extent.
	assert (a_tnY >= 0 && a_tnY < m_tnHeight);
	assert (a_tnX >= 0 && a_tnX < m_tnWidth);

	// Find the bit & return if it's set.
	SIZE tnI = a_tnY * m_tnWidth + a_tnX;
	return (m_pnPoints[tnI / (g_knBitsPerByte * sizeof (unsigned int))]
		& (1U << (tnI % (g_knBitsPerByte * sizeof (unsigned int)))));
}



// Flood-fill the current region.
template <class INDEX, class SIZE>
template <class CONTROL>
void
BitmapRegion2D<INDEX,SIZE>::FloodFill (Status_t &a_reStatus,
	CONTROL &a_rControl, bool a_bVerify, bool a_bExpand)
{
	typename CONTROL::ConstIterator itExtent;
		// An extent we're examining.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// How we set up depends on whether we're to verify all existing
	// region extents.
	if (a_bVerify)
	{
		// The to-do-list becomes the current region, and we empty
		// out the current region too.
		a_rControl.m_oAlreadyDone.Clear();
		if (a_rControl.m_oToDo.CanMove (*this))
		{
			// Grab the extents right out of the current region.
			a_rControl.m_oToDo.Clear();
			a_rControl.m_oToDo.Move (*this);
		}
		else
		{
			// Copy the extents from the current region, then empty it.
			a_rControl.m_oToDo.Assign (a_reStatus, *this);
			if (a_reStatus != g_kNoError)
				return;
			Clear();
		}
	}
	else
	{
		// The already-done list starts with the region, i.e. every
		// extent already known to be in the region can avoid getting
		// tested.
		a_rControl.m_oAlreadyDone.Assign (a_reStatus, *this);
		if (a_reStatus != g_kNoError)
			return;

		// Start the to-do list with a border around all the extents in
		// the region.
		a_rControl.m_oToDo.MakeBorder (a_reStatus, *this);
		if (a_reStatus != g_kNoError)
			return;
	}

	// Now pull each extent out of the to-do list.  Determine which
	// sub-extents are part of the region.  Add those found extents
	// to the region.  Then add all extent surrounding the found extents
	// to the to-do-next list.
	itExtent = a_rControl.m_oToDo.Begin();
	while (itExtent != a_rControl.m_oToDo.End())
	{
		INDEX tnX;
			// Where we're looking for extents.

		// Get the extent, converting the type if necessary.
		typename CONTROL::Extent oExtent;
		oExtent.m_tnY = (*itExtent).m_tnY;
		oExtent.m_tnXStart = (*itExtent).m_tnXStart;
		oExtent.m_tnXEnd = (*itExtent).m_tnXEnd;
		INDEX tnY = oExtent.m_tnY;

		// If this extent shouldn't be considered, skip it.
		if (!a_rControl.ShouldUseExtent (oExtent))
			goto nextExtent;

		// Make sure our client left us with a valid extent.
		assert (oExtent.m_tnXStart < oExtent.m_tnXEnd);
		assert (tnY == oExtent.m_tnY);		// (Sanity check)

		// Run through the pixels described by this extent, see if
		// they can be added to the region, and remember where to
		// search next.
		for (tnX = oExtent.m_tnXStart; tnX < oExtent.m_tnXEnd; ++tnX)
		{
			// We're about to check this point.  Put it in the
			// already-checked list now.
			a_rControl.m_oAlreadyDone.SetPoint (tnY, tnX);

			// Is this point in the region?
			if (a_rControl.IsPointInRegion (tnX, tnY))
			{
				// This point is in the region.
				SetPoint (tnY, tnX);

				// Now add all surrounding points to the to-do-next list.
				if (a_bExpand)
				{
					// Add the extent above this one.
					if (tnY > 0
					&& !a_rControl.m_oToDo.DoesContainPoint (tnY - 1, tnX)
					&& !a_rControl.m_oAlreadyDone.DoesContainPoint
							(tnY - 1, tnX))
						a_rControl.m_oNextToDo.SetPoint (tnY - 1, tnX);

					// Add the extent to the left.
					if (tnX > 0
					&& !a_rControl.m_oToDo.DoesContainPoint (tnY, tnX - 1)
					&& !a_rControl.m_oAlreadyDone.DoesContainPoint
							(tnY, tnX - 1))
						a_rControl.m_oNextToDo.SetPoint (tnY, tnX - 1);

					// Add the extent to the right.
					if (tnX < m_tnWidth - 1
					&& !a_rControl.m_oToDo.DoesContainPoint (tnY, tnX + 1)
					&& !a_rControl.m_oAlreadyDone.DoesContainPoint
							(tnY, tnX + 1))
						a_rControl.m_oNextToDo.SetPoint (tnY, tnX + 1);

					// Add the extent below this one.
					if (tnY < m_tnHeight - 1
					&& !a_rControl.m_oToDo.DoesContainPoint (tnY + 1, tnX)
					&& !a_rControl.m_oAlreadyDone.DoesContainPoint
							(tnY + 1, tnX))
						a_rControl.m_oNextToDo.SetPoint (tnY + 1, tnX);
				}
			}
		}

nextExtent:
		// Move to the next extent.
		++itExtent;

		// If the to-do list needs to be replenished, do so.
		if (itExtent == a_rControl.m_oToDo.End())
		{
			// Replenish the to-do list.
			a_rControl.m_oToDo.Clear();
			a_rControl.m_oToDo.Move (a_rControl.m_oNextToDo);

			// Make sure no points will be checked repeatedly.
			// (This is an expensive check, so only do it if we've
			// turned on the other expensive checks.)
			#ifdef DEBUG_BITMAPREGION2D
			{
				SIZE nPointsBefore = a_rControl.m_oToDo.NumberOfPoints();
				a_rControl.m_oToDo.Subtract (a_reStatus,
					a_rControl.m_oAlreadyDone);
				assert (a_reStatus == g_kNoError);
				SIZE nPointsAfter = a_rControl.m_oToDo.NumberOfPoints();
				assert (nPointsBefore == nPointsAfter);
			}
			#endif // DEBUG_BITMAPREGION2D

			// Start over at the beginning.
			itExtent = a_rControl.m_oToDo.Begin();
		}
	}
}



// Make the current region represent the border of the other
// region, i.e. every pixel that borders the region but isn't
// already in the region.
template <class INDEX, class SIZE>
template <class OTHER>
void
BitmapRegion2D<INDEX,SIZE>::MakeBorder (Status_t &a_reStatus,
	const OTHER &a_rOther)
{
	typename OTHER::ConstIterator itExtent;
		// Used to loop through the other region's extents.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Start with an empty region.
	Clear();

	// For every extent in the other region, add every surrounding
	// extent.  That creates a region that looks like the other region,
	// but also contains the border we're after.
	for (itExtent = a_rOther.Begin();
		 itExtent != a_rOther.End();
		 ++itExtent)
	{
		// Add the surrounding extents.
		UnionSurroundingExtents (a_reStatus, (*itExtent).m_tnY,
			(*itExtent).m_tnXStart, (*itExtent).m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	}

	// Finally, subtract the other region.  That punches a hole in us,
	// and creates the border region we're after.
	Subtract (a_reStatus, a_rOther);
	if (a_reStatus != g_kNoError)
		return;
}



// Add all extents surrounding the given extent.  Used by FloodFill().
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::UnionSurroundingExtents
	(Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
	INDEX a_tnXEnd)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Add the extent above this one.
	if (a_tnY > 0)
		Union (a_tnY - 1, a_tnXStart, a_tnXEnd);

	// Add the extent to the left.
	if (a_tnXStart > 0)
		Union (a_tnY, a_tnXStart - 1, a_tnXStart);

	// Add the extent to the right.
	if (a_tnXEnd < m_tnWidth)
		Union (a_tnY, a_tnXEnd, a_tnXEnd + 1);

	// Add the extent below this one.
	if (a_tnY < m_tnHeight - 1)
		Union (a_tnY + 1, a_tnXStart, a_tnXEnd);
}



// Move one of our iterators forward.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::IteratorForward
	(ConstIterator &a_ritHere) const
{
	SIZE tnWordIndex, tnBitIndex;
		// The word/bit index that corresponds to the x/y pixel index.
	SIZE tnLastWordIndex, tnLastBitIndex;
		// The word/bit index for the last pixel in the active area or
		// in the current line.
	unsigned int nWord;
		// A bitmap word being examined.
	unsigned int nMask;
		// A bitmask we generate.

	// If the old extent ended outside of the active area, move
	// into the active area.
	// This naturally detects an empty active-area, too.
	if (a_ritHere.m_oExtent.m_tnY < m_tnYMin)
	{
		a_ritHere.m_oExtent.m_tnXStart
			= a_ritHere.m_oExtent.m_tnXEnd = m_tnXMin;
		a_ritHere.m_oExtent.m_tnY = m_tnYMin;
	}
	else if (a_ritHere.m_oExtent.m_tnXEnd >= m_tnXMax)
	{
		a_ritHere.m_oExtent.m_tnXStart
			= a_ritHere.m_oExtent.m_tnXEnd = m_tnXMin;
		++a_ritHere.m_oExtent.m_tnY;
	}
	else if (a_ritHere.m_oExtent.m_tnXEnd < m_tnXMin)
	{
		a_ritHere.m_oExtent.m_tnXStart
			= a_ritHere.m_oExtent.m_tnXEnd = m_tnXMin;
	}

	// If we're past the active area, we're done.
	if (a_ritHere.m_oExtent.m_tnY > m_tnYMax)
	{
		a_ritHere = End();
		return;
	}

	// We'll start looking for a new extent after the end of the old
	// extent.  Find the word/bit index for that, and the word itself.
	tnBitIndex = SIZE (a_ritHere.m_oExtent.m_tnY) * SIZE (m_tnWidth)
		+ SIZE (a_ritHere.m_oExtent.m_tnXEnd);
	tnWordIndex = tnBitIndex >> Limits<unsigned int>::Log2Bits;
	tnBitIndex -= tnWordIndex << Limits<unsigned int>::Log2Bits;
	nWord = m_pnPoints[tnWordIndex];

	// Find the index of the last word, given the active area.
	tnLastBitIndex = SIZE (m_tnYMax) * SIZE (m_tnWidth) + SIZE (m_tnXMax);
	tnLastWordIndex = tnLastBitIndex >> Limits<unsigned int>::Log2Bits;
	//tnLastBitIndex -= tnLastWordIndex << Limits<unsigned int>::Log2Bits;

	// Get rid of any set bits that have been iterated over already.
	nWord &= ((~0U) << tnBitIndex);

	// Skip all clear bits.  First check the current word to see if the
	// rest of its bits are clear, then skip over all all-clear words.
	if (nWord == 0U)
	{
		// The rest of the current word's bits are clear.
		++tnWordIndex;
		tnBitIndex = 0;

		// Skip over all all-clear words.
		while (tnWordIndex <= tnLastWordIndex
				&& m_pnPoints[tnWordIndex] == 0U)
			++tnWordIndex;

		// If we reached the end of the bitmap, let our caller know.
		if (tnWordIndex > tnLastWordIndex)
		{
			a_ritHere = End();
			return;
		}

		// Get the current word.
		nWord = m_pnPoints[tnWordIndex];
	}

	// Look for the next set bit in the current word.
	assert (nWord != 0U);
	#if 0
	tnBitIndex = FindFirstSetBit (nWord, tnBitIndex);
	#else
	tnBitIndex = ffs (nWord) - 1;
	#endif

	// Calculate the x/y index of this first set bit, and the word/bit
	// index of the end of the line.
	tnLastBitIndex = tnWordIndex * Limits<unsigned int>::Bits
		+ tnBitIndex;
	a_ritHere.m_oExtent.m_tnY = INDEX (tnLastBitIndex / m_tnWidth);
	a_ritHere.m_oExtent.m_tnXStart = tnLastBitIndex
		- SIZE (a_ritHere.m_oExtent.m_tnY) * SIZE (m_tnWidth);
	tnLastBitIndex = SIZE (a_ritHere.m_oExtent.m_tnY)
		* SIZE (m_tnWidth) + SIZE (m_tnXMax);
	tnLastWordIndex = tnLastBitIndex >> Limits<unsigned int>::Log2Bits;
	tnLastBitIndex -= tnLastWordIndex << Limits<unsigned int>::Log2Bits;

	// Skip all set bits.  First check the current word to see if the
	// rest of its bits are set, then skip over all all-set words.
	nMask = ((~0U) << tnBitIndex);
	if ((nWord & nMask) == nMask)
	{
		// The rest of the current word's bits are set.
		++tnWordIndex;
		tnBitIndex = 0;

		// Skip over all all-set words.
		while (tnWordIndex <= tnLastWordIndex
				&& tnWordIndex < m_tnBitmapInts
				&& m_pnPoints[tnWordIndex] == (~0U))
			++tnWordIndex;

		// If we reached the end of the line, let our caller know.
		if (tnWordIndex > tnLastWordIndex)
		{
			a_ritHere.m_oExtent.m_tnXEnd = m_tnXMax;
			return;
		}

		// Get the current word.
		nWord = m_pnPoints[tnWordIndex];
	}

	// Look for the next clear bit in the current word.
	assert (nWord != (~0U));
	#if 0
	tnBitIndex = FindFirstClearBit (nWord, tnBitIndex);
	#else
	nWord |= ~((~0U) << tnBitIndex);
	tnBitIndex = ffs (~nWord) - 1;
	#endif

	// Clip it to the end of the line.
	if (tnWordIndex == tnLastWordIndex
			&& tnBitIndex > tnLastBitIndex)
		tnBitIndex = tnLastBitIndex;

	// Calculate the x index of this first clear bit; it becomes the
	// end of the extent.
	tnLastBitIndex = tnWordIndex * Limits<unsigned int>::Bits
		+ tnBitIndex;
	a_ritHere.m_oExtent.m_tnXEnd = tnLastBitIndex
		- SIZE (a_ritHere.m_oExtent.m_tnY) * SIZE (m_tnWidth);
	assert (a_ritHere.m_oExtent.m_tnXEnd <= m_tnWidth);
}



// Move one of our iterators backward.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::IteratorBackward
	(ConstIterator &a_ritHere) const
{
	// Not written yet.
	assert (false);
}



// Find the first set bit.
template <class INDEX, class SIZE>
int
BitmapRegion2D<INDEX,SIZE>::FindFirstSetBit (unsigned int a_nWord,
	int a_nSkip)
{
	int nLow, nHigh;
		// The search range.

	// Skip the first a_nSkip bits by clearing them.
	a_nWord &= ((~0U) << a_nSkip);

	// Make sure at least one bit is set.
	assert (a_nWord != 0U);

	// Find the range of all-clear LSB bits, i.e. the highest index
	// such that it and all lower-index bits are zero.
	nLow = 0;	// faster than nLow = a_nSkip?
	nHigh = Limits<unsigned int>::Bits - 1;
	while (nLow < nHigh)
	{
		// Look in the middle.
		int nCurrent = (nLow + nHigh) >> 1;

		// Generate a mask where bits 0-nCurrent are all set, and
		// the remaining bits are clear.
		unsigned int nCurrentMask = ~((~0U) << (nCurrent + 1));

		// If the nCurrent LSB bits are all clear, look above nCurrent,
		// otherwise look below nCurrent.
		if ((a_nWord & nCurrentMask) == 0U)
			nLow = nCurrent + 1;
		else
			nHigh = nCurrent;
	}

	// Return the index of the first set bit.
	assert (nLow == nHigh);
	return nLow;
}



// Find the first clear bit.
template <class INDEX, class SIZE>
int
BitmapRegion2D<INDEX,SIZE>::FindFirstClearBit (unsigned int a_nWord,
	int a_nSkip)
{
	int nLow, nHigh;
		// The search range.

	// Skip the first a_nSkip bits by setting them.
	a_nWord |= ~((~0U) << a_nSkip);

	// Make sure at least one bit is clear.
	assert (a_nWord != (~0U));

	// Find the range of all-set LSB bits.
	nLow = 0;	// faster than nLow = a_nSkip?
	nHigh = Limits<unsigned int>::Bits - 1;
	while (nLow < nHigh)
	{
		// Look in the middle.
		int nCurrent = (nLow + nHigh) >> 1;

		// Generate a mask where bits 0-nCurrent are all set, and
		// the remaining bits are clear.
		unsigned int nCurrentMask = ~((~0U) << (nCurrent + 1));

		// If the nCurrent LSB bits are all set, look above nCurrent,
		// otherwise look below nCurrent.
		if ((a_nWord & nCurrentMask) == nCurrentMask)
			nLow = nCurrent + 1;
		else
			nHigh = nCurrent;
	}

	// Return the index of the first clear bit.
	assert (nLow == nHigh);
	return nLow;
}



// Find the last set bit.
template <class INDEX, class SIZE>
int
BitmapRegion2D<INDEX,SIZE>::FindLastSetBit (unsigned int a_nWord,
	int a_nSkip)
{
	// Not written yet.
	assert (false);
	return 0;
}



// Find the last clear bit.
template <class INDEX, class SIZE>
int
BitmapRegion2D<INDEX,SIZE>::FindLastClearBit (unsigned int a_nWord,
	int a_nSkip)
{
	// Not written yet.
	assert (false);
	return 0;
}



// Get the first extent in the region.
template <class INDEX, class SIZE>
typename BitmapRegion2D<INDEX,SIZE>::ConstIterator
BitmapRegion2D<INDEX,SIZE>::Begin (void) const
{
	ConstIterator itBegin;
		// The iterator we construct.

	// Start before the beginning of this region.
	itBegin.m_pThis = this;
	itBegin.m_oExtent.m_tnY = 0;
	itBegin.m_oExtent.m_tnXStart = 0;
	itBegin.m_oExtent.m_tnXEnd = 0;

	// Move to the first valid extent & return it.
	return ++itBegin;
}



// Get one past the last extent in the region.
template <class INDEX, class SIZE>
typename BitmapRegion2D<INDEX,SIZE>::ConstIterator
BitmapRegion2D<INDEX,SIZE>::End (void) const
{
	ConstIterator itEnd;
		// The iterator we construct.

	// Since extents can't be zero-length, a zero-length extent
	// indicates the end.
	itEnd.m_pThis = this;
	itEnd.m_oExtent.m_tnY = 0;
	itEnd.m_oExtent.m_tnXStart = 0;
	itEnd.m_oExtent.m_tnXEnd = 0;
	return itEnd;
}



// Move forward to the next extent.
template <class INDEX, class SIZE>
typename BitmapRegion2D<INDEX,SIZE>::ConstIterator&
BitmapRegion2D<INDEX,SIZE>::ConstIterator::operator++()
{
	// Make sure we know what region we're iterating through.
	assert (m_pThis != NULL);

	// It does all the work.
	m_pThis->IteratorForward (*this);
	return *this;
}



// Move backward to the previous extent.
template <class INDEX, class SIZE>
typename BitmapRegion2D<INDEX,SIZE>::ConstIterator&
BitmapRegion2D<INDEX,SIZE>::ConstIterator::operator--()
{
	// Make sure we know what region we're iterating through.
	assert (m_pThis != NULL);

	// It does all the work.
	m_pThis->IteratorBackward (*this);
	return *this;
}



// The set-bits-per-byte table.
// Used to implement NumberOfPoints().
template <class INDEX, class SIZE>
int8_t BitmapRegion2D<INDEX,SIZE>::m_anSetBitsPerByte[256];
template <class INDEX, class SIZE>
bool BitmapRegion2D<INDEX,SIZE>::m_bSetBitsPerByte = false;



// Default constructor.  Must be followed by a call to Init().
template <class INDEX, class SIZE>
BitmapRegion2D<INDEX,SIZE>::FloodFillControl::FloodFillControl()
{
	// Nothing to do.
}



// Initializing constructor.
template <class INDEX, class SIZE>
BitmapRegion2D<INDEX,SIZE>::FloodFillControl::FloodFillControl
	(Status_t &a_reStatus, INDEX a_tnWidth, INDEX a_tnHeight)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize ourselves.
	Init (a_reStatus, a_tnWidth, a_tnHeight);
	if (a_reStatus != g_kNoError)
		return;
}



// Initializer.  Must be called on default-constructed objects.
template <class INDEX, class SIZE>
void
BitmapRegion2D<INDEX,SIZE>::FloodFillControl::Init
	(Status_t &a_reStatus, INDEX a_tnWidth, INDEX a_tnHeight)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize our helper regions.
	BaseClass::m_oToDo.Init (a_reStatus, a_tnWidth, a_tnHeight);
	if (a_reStatus != g_kNoError)
		return;
	BaseClass::m_oAlreadyDone.Init (a_reStatus, a_tnWidth, a_tnHeight);
	if (a_reStatus != g_kNoError)
		return;
	BaseClass::m_oNextToDo.Init (a_reStatus, a_tnWidth, a_tnHeight);
	if (a_reStatus != g_kNoError)
		return;
}



#ifndef NDEBUG
template <class INDEX, class SIZE>
uint32_t BitmapRegion2D<INDEX,SIZE>::sm_ulInstances;
#endif // NDEBUG



#endif // __BITMAPREGION2D_H__
