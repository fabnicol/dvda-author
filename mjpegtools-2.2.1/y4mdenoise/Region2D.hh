#ifndef __REGION2D_H__
#define __REGION2D_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// Region2D tracks a 2-dimensional region of arbitrary points.
//
// It's the base class for more specific implementations of regions,
// which only have to be compatible in the interface sense -- they
// don't have to support being called properly through a base-class
// pointer, they only have to be interchangeable when used as template
// parameters.  Region2D contains the interface definition & all the
// code that will work with any implementation of regions.

#include "Status_t.h"
#include <iostream>



// The 2-dimensional region class.  Parameterized by the numeric type
// to use for point indices, and the numeric type to use to count the
// contained number of points.
// This was an attempt to create a generic base class for 2-dimensional
// regions.  It can't be done without making many methods virtual, which
// would prevent them from being inlined, and thus would impact
// performance.  Many methods do have a non-region-specific
// implementation, just not enough.
// The commented-out method prototypes are methods to be implemented by
// subclasses.  Not all methods have to be implemented, depending on
// whether it's appropriate for the subclass, but that may impact how
// widely the subclass may be used.
template <class INDEX, class SIZE>
class Region2D
{
public:
	// An extent within the region, representing a continuous series of
	// points in a horizontal line.  Used by iterator classes to return
	// successive parts of the region.
	class Extent
	{
	public:
		INDEX m_tnY, m_tnXStart, m_tnXEnd;
			// The y-index, and the start/end x-indices, of the extent.
			// Note that m_tnXEnd is technically one past the end.

		Extent() {}
			// Default constructor.

		Extent (INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd)
				: m_tnY (a_tnY), m_tnXStart (a_tnXStart),
					m_tnXEnd (a_tnXEnd) {}
			// Initializing constructor.

		bool operator== (const Extent &a_rOther) const
				{ return (m_tnY == a_rOther.m_tnY
					&& m_tnXStart == a_rOther.m_tnXStart
					&& m_tnXEnd == a_rOther.m_tnXEnd) ? true : false; }
			// Equality operator.

		bool operator!= (const Extent &a_rOther) const
				{ return (m_tnY != a_rOther.m_tnY
					|| m_tnXStart != a_rOther.m_tnXStart
					|| m_tnXEnd != a_rOther.m_tnXEnd) ? true : false; }
			// Inequality operator.

		inline bool operator < (const Extent &a_rOther) const;
			// Less-than operator.
	};

	Region2D();
		// Default constructor.  Must be followed by Init().

	template <class REGION>
	Region2D (Status_t &a_reStatus, const REGION &a_rOther);
		// Copy constructor.
	
	//void Init (Status_t &a_reStatus);
		// Initializer.  Must be called on default-constructed regions.

	template <class REGION>
	void Assign (Status_t &a_reStatus, const REGION &a_rOther);
		// Make the current region a copy of the other region.

	virtual ~Region2D();
		// Destructor.

	//SIZE NumberOfPoints (void) const;
		// Return the total number of points contained by the region.

	void Clear (void);
		// Clear the region, emptying it of all extents.

	//void Union (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
	//		INDEX a_tnXEnd);
		// Add the given horizontal extent to the region.  Note that
		// a_tnXEnd is technically one past the end of the extent.

	template <class REGION, class REGION_TEMP>
	void UnionDebug (Status_t &a_reStatus, INDEX a_tnY,
			INDEX a_tnXStart, INDEX a_tnXEnd, REGION_TEMP &a_rTemp);
		// Add the given horizontal extent to the region.  Note that
		// a_tnXEnd is technically one past the end of the extent.
		// Exhaustively (i.e. slowly) verifies the results, using a
		// much simpler algorithm.
		// Requires the use of a temporary region, usually of the
		// final subclass' type, in order to work.  (Since that can't
		// be known at this level, a template parameter is included for
		// it.)

	template <class REGION>
	void Union (Status_t &a_reStatus, const REGION &a_rOther);
		// Make the current region represent the union between itself
		// and the other given region.

	template <class REGION, class REGION_O, class REGION_TEMP>
	void UnionDebug (Status_t &a_reStatus,
			REGION_O &a_rOther, REGION_TEMP &a_rTemp);
		// Make the current region represent the union between itself
		// and the other given region.
		// Exhaustively (i.e. slowly) verifies the results, using a
		// much simpler algorithm.
		// Requires the use of a temporary region, usually of the
		// final subclass' type, in order to work.  (Since that can't
		// be known at this level, a template parameter is included for
		// it.)

	//void Merge (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
	//		INDEX a_tnXEnd);
		// Merge this extent into the current region.
		// The new extent can't intersect any of the region's existing
		// extents (including being horizontally contiguous).

	//void Merge (Region2D<INDEX,SIZE> &a_rOther);
		// Merge the other region into ourselves, emptying the other
		// region.  Like Union(), but doesn't allocate any new memory.
		// Also, the other region can't intersect any of the region's
		// existing extents (including being horizontally contiguous),
		// and both regions must use the same allocator.

	//void Move (Region2D<INDEX,SIZE> &a_rOther);
		// Move the contents of the other region into the current
		// region.
		// The current region must be empty.

	template <class REGION>
	bool CanMove (const REGION &a_rOther) const { return false; }
		// Returns true if the other region's contents can be moved
		// into the current region.
		// (False by default; subclasses will have to implement
		// specialized methods that explain when this is true.)

	template <class REGION>
	void Move (REGION &a_rOther) { assert (false); }
		// Let clients know we can't move extents between regions of
		// different types by default.

	//void Intersection (Status_t &a_reStatus,
	//		const Region2D<INDEX,SIZE> &a_rOther);
		// Make the current region represent the intersection between
		// itself and the other given region.

	//void Subtract (Status_t &a_reStatus, INDEX a_tnY,
	//		INDEX a_tnXStart, INDEX a_tnXEnd);
		// Subtract the given horizontal extent from the region.  Note
		// that a_tnXEnd is technically one past the end of the extent.

	template <class REGION_TEMP>
	void SubtractDebug (Status_t &a_reStatus, INDEX a_tnY,
			INDEX a_tnXStart, INDEX a_tnXEnd, REGION_TEMP &a_rTemp);
		// Subtract the given horizontal extent from the region.  Note
		// that a_tnXEnd is technically one past the end of the extent.
		// Exhaustively (i.e. slowly) verifies the results, using a
		// much simpler algorithm.
		// Requires the use of a temporary region, usually of the
		// final subclass' type, in order to work.  (Since that can't
		// be known at this level, a template parameter is included for
		// it.)

	template <class REGION>
	void Subtract (Status_t &a_reStatus, const REGION &a_rOther);
		// Subtract the other region from the current region, i.e.
		// remove from the current region any extents that exist in the
		// other region.
	
	template <class REGION, class REGION_O, class REGION_TEMP>
	void SubtractDebug (Status_t &a_reStatus, REGION_O &a_rOther,
			REGION_TEMP &a_rTemp);
		// Subtract the other region from the current region, i.e.
		// remove from the current region any extents that exist in the
		// other region.
		// Exhaustively (i.e. slowly) verifies the results, using a
		// much simpler algorithm.
		// Requires the use of a temporary region, usually of the
		// final subclass' type, in order to work.  (Since that can't
		// be known at this level, a template parameter is included for
		// it.)

	//typedef ... ConstIterator;
	//ConstIterator Begin (void) const { return m_setExtents.Begin(); }
	//ConstIterator End (void) const { return m_setExtents.End(); }
		// Allow our client to iterate through the extents & get their
		// values.
	
	//bool DoesContainPoint (INDEX a_tnY, INDEX a_tnX);
		// Returns true if the region contains the given point.

	//bool DoesContainPoint (INDEX a_tnY, INDEX a_tnX,
	//		ConstIterator &a_ritHere);
		// Returns true if the region contains the given point.
		// Backpatches the extent that contains the point, or the
		// extent before where it should be.

	// A structure that helps implement flood-fills.  Clients should
	// create a derived (or structurally compatible) class that, at a
	// minimum, describes whether a new point should be added to the
	// region.  (The definition of this class follows the definition of
	// Region2D<>.)
	template <class REGION> class FloodFillControl;

	template <class CONTROL>
	void FloodFill (Status_t &a_reStatus, CONTROL &a_rControl,
			bool a_bVerify, bool a_bExpand);
		// Flood-fill the current region.
		// If a_bVerify is true, all existing region points are re-tested.
		// If a_bExpand is true, all points bordering the current region
		// are tested for inclusion.

	template <class REGION>
	void MakeBorder (Status_t &a_reStatus, const REGION &a_rOther);
		// Make the current region represent the border of the other
		// region, i.e. every pixel that's adjacent to a pixel that's
		// in the region, but isn't already in the region.

	void UnionSurroundingExtents (Status_t &a_reStatus,
			INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd);
		// Add all extents surrounding the given extent.
		// Used by FloodFill() and MakeBorder().
};



template <class INDEX, class SIZE>
std::ostream &
operator<< (std::ostream &a_rOut,
	const Region2D<INDEX,SIZE> &a_rRegion);



// The flood-fill-control class.
template <class INDEX, class SIZE>
template <class REGION>
class Region2D<INDEX,SIZE>::FloodFillControl
{
public:
	// (Although these fields are public, they should be considered
	// opaque to the client.)

	REGION m_oToDo;
		// Extents that still need to be be tested.

	REGION m_oAlreadyDone;
		// Extents that have been tested.

	REGION m_oNextToDo;
		// Extents contiguous with those that have just been added
		// to the flood-filled area.

	typedef typename REGION::ConstIterator ConstIterator;
	typedef typename REGION::Extent Extent;
		// The iterator/extent type for running through the above
		// regions.

public:
	FloodFillControl();
		// Default constructor.  Must be followed by a call to Init().
	
	FloodFillControl (Status_t &a_reStatus);
		// Initializing constructor.
	
	void Init (Status_t &a_reStatus);
		// Initializer.  Must be called on default-constructed objects.
		// (May not be valid to call on subclasses, depending on
		// whether more parameters are needed for its Init().)

	// Methods to be redefined by clients implementing specific
	// flood-fills.

	bool ShouldUseExtent (Extent &a_rExtent) { return true; }
		// Return true if the flood-fill should examine the given
		// extent.  Clients should redefine this to define their own
		// criteria for when extents should be used, and to modify the
		// extent as needed (e.g. to clip the extent to a bounding box).
	
	bool IsPointInRegion (INDEX a_tnX, INDEX a_tnY) { return false; }
		// Returns true if the given point should be included in the
		// flood-fill.  Clients must redefine this to explain their
		// flood-fill criteria.
};



#if 0

// I'm not sure how to do this yet.  If the template just takes "class
// REGION", I think it'll try to match everything in the world, and
// that wouldn't be good.  But how to do this???
// Also, iterators aren't actually defined at the Region2D<> level.

// Print the region's contents.
template <class INDEX, class SIZE>
std::ostream &
operator<< (std::ostream &a_rOut, const Region2D<INDEX,SIZE> &a_rRegion)
{
	Region2D<INDEX,SIZE>::ConstIterator itHere;
		// Used to loop through the region.
	
	// Print the region header.
	a_rOut << "( ";

	// Print each extent.
	for (itHere = Begin(); itHere != End(); ++itHere)
	{
		// Print the extent.
		a_rOut << '[' << ((*itHere).m_tnY) << ','
			<< ((*itHere).m_tnXStart) << '-' << ((*itHere).m_tnXEnd)
			<< "] ";
	}
	
	// Print the region trailer.
	a_rOut << ")"

	return a_rOut;
}

#endif



// Default constructor.  Must be followed by Init().
template <class INDEX, class SIZE>
Region2D<INDEX,SIZE>::Region2D()
{
	// Nothing to do.
}



// Copy constructor.
template <class INDEX, class SIZE>
template <class REGION>
Region2D<INDEX,SIZE>::Region2D (Status_t &a_reStatus,
	const REGION &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Copy all the extents.
	for (typename REGION::ConstIterator itHere = a_rOther.Begin();
		 itHere != a_rOther.End();
		 ++itHere)
	{
		Merge (a_reStatus, (*itHere).m_tnY, (*itHere).m_tnXStart,
			(*itHere).m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	}
}



// Make the current region a copy of the other region.
template <class INDEX, class SIZE>
template <class REGION>
void
Region2D<INDEX,SIZE>::Assign (Status_t &a_reStatus,
	const REGION &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Assign the other region's extents to ourselves.
	Clear();
	for (typename REGION::ConstIterator itHere = a_rOther.Begin();
		 itHere != a_rOther.End();
		 ++itHere)
	{
		Merge (a_reStatus, (*itHere).m_tnY, (*itHere).m_tnXStart,
			(*itHere).m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	}
}



// Destructor.
template <class INDEX, class SIZE>
Region2D<INDEX,SIZE>::~Region2D()
{
	// Nothing to do.
}



// Add the given horizontal extent to the region.
template <class INDEX, class SIZE>
template <class REGION, class REGION_TEMP>
void
Region2D<INDEX,SIZE>::UnionDebug (Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd, REGION_TEMP &a_rTemp)
{
	typename REGION::ConstIterator itHere;
	typename REGION_TEMP::ConstIterator itHereO;
	INDEX tnX;
		// Used to loop through points.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Calculate the union.
	a_rTemp.Assign (a_reStatus, *this);
	if (a_reStatus != g_kNoError)
		return;
	a_rTemp.Union (a_reStatus, a_tnY, a_tnXStart, a_tnXEnd);
	if (a_reStatus != g_kNoError)
		return;
	
	// Loop through every point in the result, make sure it's in
	// one of the two input regions.
	for (itHereO = a_rTemp.Begin(); itHereO != a_rTemp.End(); ++itHereO)
	{
		const Extent &rHere = *itHereO;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!((rHere.m_tnY == a_tnY
				&& (tnX >= a_tnXStart && tnX < a_tnXEnd))
			|| this->DoesContainPoint (rHere.m_tnY, tnX)))
				goto error;
		}
	}

	// Loop through every point in the original region, make sure
	// it's in the result.
	for (itHere = this->Begin(); itHere != this->End(); ++itHere)
	{
		const Extent &rHere = *itHere;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!a_rTemp.DoesContainPoint (rHere.m_tnY, tnX))
				goto error;
		}
	}

	// Loop through every point in the added extent, make sure it's in
	// the result.
	for (tnX = a_tnXStart; tnX < a_tnXEnd; ++tnX)
	{
		if (!a_rTemp.DoesContainPoint (a_tnY, tnX))
			goto error;
	}

	// The operation succeeded.  Commit it.
	Assign (a_reStatus, a_rTemp);
	if (a_reStatus != g_kNoError)
		return;

	// All done.
	return;

error:
	// Handle deviations.
	fprintf (stderr, "Region2D::Union() failed\n");
	fprintf (stderr, "Input region:\n");
	PrintRegion (*this);
	fprintf (stderr, "Input extent: [%d,%d-%d]\n",
		int (a_tnY), int (a_tnXStart), int (a_tnXEnd));
	fprintf (stderr, "Result:\n");
	PrintRegion (a_rTemp);
	assert (false);
}



// Make the current region represent the union between itself
// and the other given region.
template <class INDEX, class SIZE>
template <class REGION>
void
Region2D<INDEX,SIZE>::Union (Status_t &a_reStatus,
	const REGION &a_rOther)
{
	typename REGION::ConstIterator itHere;
		// Where we are in the other region's extents.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Run through the extents in the other region, add them to the
	// current region.
	for (itHere = a_rOther.Begin();
		 itHere != a_rOther.End();
		 ++itHere)
	{
		// Add this extent to the current region.
		Union (a_reStatus, (*itHere).m_tnY, (*itHere).m_tnXStart,
			(*itHere).m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	}
}



// Make the current region represent the union between itself
// and the other given region.
template <class INDEX, class SIZE>
template <class REGION, class REGION_O, class REGION_TEMP>
void
Region2D<INDEX,SIZE>::UnionDebug (Status_t &a_reStatus,
	REGION_O &a_rOther, REGION_TEMP &a_rTemp)
{
	typename REGION::ConstIterator itHere;
	typename REGION_O::ConstIterator itHereO;
	typename REGION_TEMP::ConstIterator itHereT;
	INDEX tnX;
		// Used to loop through points.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Calculate the union.
	a_rTemp.Assign (a_reStatus, *this);
	if (a_reStatus != g_kNoError)
		return;
	a_rTemp.Union (a_reStatus, a_rOther);
	if (a_reStatus != g_kNoError)
		return;
	
	// Loop through every point in the result, make sure it's in
	// one of the two input regions.
	for (itHereT = a_rTemp.Begin(); itHereT != a_rTemp.End(); ++itHereT)
	{
		const Extent &rHere = *itHereT;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!a_rOther.DoesContainPoint (rHere.m_tnY, tnX)
			&& !this->DoesContainPoint (rHere.m_tnY, tnX))
				goto error;
		}
	}

	// Loop through every point in the first input region, make sure
	// it's in the result.
	for (itHere = this->Begin(); itHere != this->End(); ++itHere)
	{
		const Extent &rHere = *itHere;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!a_rTemp.DoesContainPoint (rHere.m_tnY, tnX))
				goto error;
		}
	}

	// Loop through every point in the second input region, make sure
	// it's in the result.
	for (itHereO = a_rOther.Begin();
		 itHereO != a_rOther.End();
		 ++itHereO)
	{
		const Extent &rHere = *itHereO;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!a_rTemp.DoesContainPoint (rHere.m_tnY, tnX))
				goto error;
		}
	}

	// The operation succeeded.  Commit it.
	Assign (a_reStatus, a_rTemp);
	if (a_reStatus != g_kNoError)
		return;

	// All done.
	return;

error:
	// Handle deviations.
	fprintf (stderr, "Region2D::Union() failed\n");
	fprintf (stderr, "First input region:\n");
	PrintRegion (*this);
	fprintf (stderr, "Second input region:\n");
	PrintRegion (a_rOther);
	fprintf (stderr, "Result:\n");
	PrintRegion (a_rTemp);
	assert (false);
}



// Subtract the other region from the current region, i.e.
// remove from the current region any areas that exist in the
// other region.
template <class INDEX, class SIZE>
template <class REGION, class REGION_O, class REGION_TEMP>
void
Region2D<INDEX,SIZE>::SubtractDebug (Status_t &a_reStatus,
	REGION_O &a_rOther, REGION_TEMP &a_rTemp)
{
	typename REGION::ConstIterator itHere;
	typename REGION_O::ConstIterator itHereO;
	typename REGION_TEMP::ConstIterator itHereT;
	INDEX tnX;
		// Used to loop through points.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Calculate the difference.
	a_rTemp.Assign (a_reStatus, *this);
	if (a_reStatus != g_kNoError)
		return;
	a_rTemp.Subtract (a_reStatus, a_rOther);
	if (a_reStatus != g_kNoError)
		return;
	
	// Loop through every point in the result, make sure it's in
	// the first input region but not the second.
	for (itHereT = a_rTemp.Begin(); itHereT != a_rTemp.End(); ++itHereT)
	{
		const Extent &rHere = *itHereT;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!(this->DoesContainPoint (rHere.m_tnY, tnX)
			&& !a_rOther.DoesContainPoint (rHere.m_tnY, tnX)))
				goto error;
		}
	}

	// Loop through every point in the first input region, and if it's
	// not in the second input region, make sure it's in the result.
	for (itHere = this->Begin(); itHere != this->End(); ++itHere)
	{
		const Extent &rHere = *itHere;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (!a_rOther.DoesContainPoint (rHere.m_tnY, tnX))
			{
				if (!a_rTemp.DoesContainPoint (rHere.m_tnY, tnX))
					goto error;
			}
		}
	}

	// Loop through every point in the second input region, make sure
	// it's not in the result.
	for (itHereO = a_rOther.Begin();
		 itHereO != a_rOther.End();
		 ++itHereO)
	{
		const Extent &rHere = *itHere;
		for (tnX = rHere.m_tnXStart; tnX < rHere.m_tnXEnd; ++tnX)
		{
			if (a_rTemp.DoesContainPoint (rHere.m_tnY, tnX))
				goto error;
		}
	}

	// The operation succeeded.  Commit it.
	Assign (a_reStatus, a_rTemp);
	if (a_reStatus != g_kNoError)
		return;

	// All done.
	return;

error:
	// Handle deviations.
	fprintf (stderr, "Region2D::Subtract() failed\n");
	fprintf (stderr, "First input region:\n");
	PrintRegion (*this);
	fprintf (stderr, "Second input region:\n");
	PrintRegion (a_rOther);
	fprintf (stderr, "Result:\n");
	PrintRegion (a_rTemp);
	assert (false);
}



// Flood-fill the current region.
template <class INDEX, class SIZE>
template <class CONTROL>
void
Region2D<INDEX,SIZE>::FloodFill (Status_t &a_reStatus,
	CONTROL &a_rControl, bool a_bVerify, bool a_bExpand)
{
	typename CONTROL::ConstIterator itExtent;
		// An extent we're examining.
	Extent oFoundExtent;
		// An extent we found to be part of the region.

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
		bool bStartedExtent;
			// true if we've started finding an extent.
		INDEX tnX;
			// Where we're looking for extents.

		// Get the extent, converting the type if necessary.
		Extent oExtent;
		oExtent.m_tnY = (*itExtent).m_tnY;
		oExtent.m_tnXStart = (*itExtent).m_tnXStart;
		oExtent.m_tnXEnd = (*itExtent).m_tnXEnd;

		// We're about to check this extent.  Put it in the
		// already-checked list now.
		a_rControl.m_oAlreadyDone.Union (a_reStatus, oExtent.m_tnY,
			oExtent.m_tnXStart, oExtent.m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	
		// If this extent shouldn't be considered, skip it.
		if (!a_rControl.ShouldUseExtent (oExtent))
			goto nextExtent;

		// Make sure our client left us with a valid extent.
		assert (oExtent.m_tnXStart < oExtent.m_tnXEnd);
		
		// Run through the pixels described by this extent, see if
		// they can be added to the region, and remember where to
		// search next.
		oFoundExtent.m_tnY = oExtent.m_tnY;
		bStartedExtent = false;
		for (tnX = oExtent.m_tnXStart; tnX <= oExtent.m_tnXEnd; ++tnX)
		{
			// Is this point in the region?
			if (tnX < oExtent.m_tnXEnd
			&& a_rControl.IsPointInRegion (tnX, oExtent.m_tnY))
			{
				// This point is in the region.  Start a new extent
				// if we didn't have one already, and add the point
				// to it.
				if (!bStartedExtent)
				{
					oFoundExtent.m_tnXStart = tnX;
					bStartedExtent = true;
				}
				oFoundExtent.m_tnXEnd = tnX + 1;
			}

			// This point is not in the region.  Any extent we're
			// building is done.
			else if (bStartedExtent)
			{
				// Add this extent to the region.
				Union (a_reStatus, oFoundExtent.m_tnY,
					oFoundExtent.m_tnXStart, oFoundExtent.m_tnXEnd);
				if (a_reStatus != g_kNoError)
					return;

				// Now add all surrounding extents to the to-do-next list.
				if (a_bExpand)
				{
					a_rControl.m_oNextToDo.UnionSurroundingExtents
						(a_reStatus, oFoundExtent.m_tnY,
							oFoundExtent.m_tnXStart, oFoundExtent.m_tnXEnd);
					if (a_reStatus != g_kNoError)
						return;
				}

				// Look for another extent.
				bStartedExtent = false;
			}
		}

nextExtent:
		// Move to the next extent.
		++itExtent;

		// If the to-do list needs to be replenished, do so.
		if (itExtent == a_rControl.m_oToDo.End())
		{
			// Replenish the to-do list.
			a_rControl.m_oNextToDo.Subtract (a_reStatus,
				a_rControl.m_oAlreadyDone);
			if (a_reStatus != g_kNoError)
				return;
			a_rControl.m_oToDo.Clear();
			a_rControl.m_oToDo.Move (a_rControl.m_oNextToDo);
			
			// Start over at the beginning.
			itExtent = a_rControl.m_oToDo.Begin();
		}
	}
}



// Make the current region represent the border of the other
// region, i.e. every pixel that borders the region but isn't
// already in the region.
template <class INDEX, class SIZE>
template <class REGION>
void
Region2D<INDEX,SIZE>::MakeBorder (Status_t &a_reStatus,
	const REGION &a_rOther)
{
	typename REGION::ConstIterator itExtent;
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
Region2D<INDEX,SIZE>::UnionSurroundingExtents (Status_t &a_reStatus,
	INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Add the extent above this one.
	Union (a_reStatus, a_tnY - 1, a_tnXStart, a_tnXEnd);
	if (a_reStatus != g_kNoError)
		return;

	// Add the extent in the middle.
	Union (a_reStatus, a_tnY, a_tnXStart - 1, a_tnXEnd + 1);
	if (a_reStatus != g_kNoError)
		return;

	// Add the extent below this one.
	Union (a_reStatus, a_tnY + 1, a_tnXStart, a_tnXEnd);
	if (a_reStatus != g_kNoError)
		return;
}



// Less-than operator.
template <class INDEX, class SIZE>
inline bool
Region2D<INDEX,SIZE>::Extent::operator <
	(const Region2D<INDEX,SIZE>::Extent &a_rOther) const
{
	// Compare on y, then x-start.
	if (m_tnY < a_rOther.m_tnY)
		return true;
	if (m_tnY > a_rOther.m_tnY)
		return false;
	if (m_tnXStart < a_rOther.m_tnXStart)
		return true;
	// if (m_tnXStart >= a_rOther.m_tnXStart)
		return false;
}



// Default constructor.  Must be followed by a call to Init().
template <class INDEX, class SIZE>
template <class REGION>
Region2D<INDEX,SIZE>::FloodFillControl<REGION>::FloodFillControl()
{
	// Nothing to do.
}



// Initializing constructor.
template <class INDEX, class SIZE>
template <class REGION>
Region2D<INDEX,SIZE>::FloodFillControl<REGION>::FloodFillControl
	(Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize ourselves.
	Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



// Initializer.  Must be called on default-constructed objects.
template <class INDEX, class SIZE>
template <class REGION>
void
Region2D<INDEX,SIZE>::FloodFillControl<REGION>::Init
	(Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize our helper regions.
	m_oToDo.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
	m_oAlreadyDone.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
	m_oNextToDo.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



#endif // __REGION2D_H__
