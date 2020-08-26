#ifndef __SETREGION2D_H__
#define __SETREGION2D_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

// SetRegion2D tracks a 2-dimensional region of arbitrary points.
// It's implemented by a sorted set of Extents.  This is pretty fast
// overall, and is very space efficient, even with sparse regions.

#include "Region2D.hh"
#include "Set.hh"



// Define this to compile in code to double-check and debug the region.
#ifndef DEBUG_REGION2D
//	#define DEBUG_SETREGION2D
#endif // DEBUG_REGION2D



// The 2-dimensional region class.  Parameterized by the numeric type
// to use for point indices, and the numeric type to use to count the
// contained number of points.
template <class INDEX, class SIZE,
	class SETIMP = SkipList<typename Region2D<INDEX,SIZE>::Extent,typename Region2D<INDEX,SIZE>::Extent,Ident<typename Region2D<INDEX,SIZE>::Extent,typename Region2D<INDEX,SIZE>::Extent>,Less<typename Region2D<INDEX,SIZE>::Extent> > >
class SetRegion2D : public Region2D<INDEX,SIZE>
{
private:
	typedef Region2D<INDEX,SIZE> BaseClass;
		// Keep track of who our base class is.
public:
	typedef typename BaseClass::Extent Extent;
		// (Wouldn't we automatically have access to Extent because
		// we're a subclass of Region2D<>?  Why is this needed?)
	typedef Set<Extent,Less<Extent>,SETIMP> Extents;
		// The container type that stores our extents.
	typedef typename Extents::InitParams InitParams;
		// The structure for our container's initialization parameters.

private:
	Extents m_setExtents;
		// The extents that make up the region.

public:
	typedef typename Extents::Allocator Allocator;
		// The type of allocator to use to allocate region extents.

	explicit SetRegion2D (Allocator &a_rAlloc
			= Extents::Imp::sm_oNodeAllocator);
		// Default constructor.  Must be followed by Init().

	SetRegion2D (Status_t &a_reStatus,
			const InitParams &a_rInitParams = InitParams(),
			Allocator &a_rAlloc = Extents::Imp::sm_oNodeAllocator);
		// Initializing constructor.  Creates an empty region.

	SetRegion2D (Status_t &a_reStatus,
			const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther);
		// Copy constructor.

	void Init (Status_t &a_reStatus,
			const InitParams &a_rInitParams = InitParams());
		// Initializer.  Must be called on default-constructed regions.

	void Assign (Status_t &a_reStatus,
			const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther);
		// Make the current region a copy of the other region.

	virtual ~SetRegion2D();
		// Destructor.

#ifdef DEBUG_SETREGION2D

	void SetDebug (bool a_bDebug);
		// Set whether to run the region invariant before and after
		// methods.

	void Invariant (void) const;
		// Thoroughly analyze the region for structural integrity.

#endif // DEBUG_SETREGION2D

	inline SIZE NumberOfPoints (void) const;
		// Return the total number of points contained by the region.

	void Clear (void);
		// Clear the region, emptying it of all extents.

	void Purge (void);
		// Purge all internally-allocated memory.

	void Union (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd);
		// Add the given horizontal extent to the region.  Note that
		// a_tnXEnd is technically one past the end of the extent.

	void Merge (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd);
		// Merge this extent into the current region.
		// The new extent can't intersect any of the region's existing
		// extents (including being horizontally contiguous).

	void Merge (SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther);
		// Merge the other region into ourselves, emptying the other
		// region.  Like Union(), but doesn't allocate any new memory.
		// Also, the other region can't intersect any of the region's
		// existing extents (including being horizontally contiguous),
		// and both regions must use the same allocator.

	void Move (SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther);
		// Move the contents of the other region into the current
		// region.
		// The current region must be empty.

	bool CanMove (const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther) const;
		// Returns true if the other region's contents can be moved
		// into the current region.

	bool CanMove (const Region2D<INDEX,SIZE> &a_rOther) const
			{ return false; }
	void Move (Region2D<INDEX,SIZE> &a_rOther) { assert (false); }
		// (We can move between SetRegion2D<>s, but not any arbitrary
		// Region2D<> subclass.)

	void Intersection (Status_t &a_reStatus,
			const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther);
		// Make the current region represent the intersection between
		// itself and the other given region.

	void Subtract (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd);
		// Subtract the given horizontal extent from the region.  Note
		// that a_tnXEnd is technically one past the end of the extent.

	void Subtract (Status_t &a_reStatus,
			const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther);
		// Subtract the other region from the current region, i.e.
		// remove from the current region any extents that exist in the
		// other region.

	void SubtractWithOffset (Status_t &a_reStatus,
			const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther,
			INDEX a_tnXOffset, INDEX a_tnYOffset);
		// Subtract the other region from the current region, but offset
		// the other region by the given amounts.

	void Offset (INDEX a_tnXOffset, INDEX a_tnYOffset);
		// Move all extents by the given offset.

	typedef typename Extents::ConstIterator ConstIterator;
	ConstIterator Begin (void) const { return m_setExtents.Begin(); }
	ConstIterator End (void) const { return m_setExtents.End(); }
		// Allow our client to iterate through the extents & get their
		// values.

	bool DoesContainPoint (INDEX a_tnY, INDEX a_tnX);
		// Returns true if the region contains the given point.

	bool DoesContainPoint (INDEX a_tnY, INDEX a_tnX,
			ConstIterator &a_ritHere);
		// Returns true if the region contains the given point.
		// Backpatches the extent that contains the point, or the
		// extent before where it should be.

	ConstIterator LowerBound (INDEX a_tnY, INDEX a_tnX) const
			{ Extent oExtent (a_tnY, a_tnX, a_tnX);
			  return m_setExtents.LowerBound (oExtent); }
		// Return the position of the first extent whose beginning
		// is >= the given point.

	ConstIterator UpperBound (INDEX a_tnY, INDEX a_tnX) const
			{ Extent oExtent (a_tnY, a_tnX, a_tnX);
			  return m_setExtents.UpperBound (oExtent); }
		// Return the position of the first extent whose beginning
		// is > the given extent.

	// A structure that implements flood-fills using SetRegion2D<> to
	// do the work.
	class FloodFillControl;

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

private:
	SIZE m_tnPoints;
		// The total number of points contained by the region.

	void Subtract (Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart,
			INDEX a_tnXEnd, const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther,
			typename Extents::ConstIterator &a_ritHere);
		// Subtract the given horizontal extent from the region.  Note
		// that a_tnXEnd is technically one past the end of the extent.
		// a_ritHere is a reference to the iterator that this extent
		// came from in a_rOther, or a_rOther.m_setExtents.End() if
		// there is no such iterator.  If there are no extents that
		// intersect the given extent, ritHere is backpatched with the
		// location of the first extent in a_rOther to intersect it.
		// This allows region subtractions to skip past irrelevant
		// areas.

	void SubtractWithOffset (Status_t &a_reStatus,
			INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd,
			INDEX a_tnXOffset, INDEX a_tnYOffset,
			const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther,
			typename Extents::ConstIterator &a_ritHere);
		// Subtract the given horizontal extent from the region, with
		// the given offset.

	#ifdef DEBUG_SETREGION2D
	bool m_bDebug;
	#endif // DEBUG_SETREGION2D
		// true if the invariant should be checked.

#ifndef NDEBUG

// Count the number of region objects in existence.
private:
	static uint32_t sm_ulInstances;
public:
	static uint32_t GetInstances (void) { return sm_ulInstances; }

#endif // NDEBUG
};



// The flood-fill-control class.
template <class INDEX, class SIZE, class SETIMP>
class SetRegion2D<INDEX,SIZE,SETIMP>::FloodFillControl
{
private:
	typedef SetRegion2D<INDEX,SIZE,SETIMP> Region_t;
		// Keep track of our region class.
public:
	// (Although these fields are public, they should be considered
	// opaque to the client.)

	Region_t m_oToDo;
		// Extents that still need to be be tested.

	Region_t m_oAlreadyDone;
		// Extents that have been tested.

	Region_t m_oNextToDo;
		// Extents contiguous with those that have just been added
		// to the flood-filled area.

	typedef typename Region_t::ConstIterator ConstIterator;
	typedef typename Region_t::Extent Extent;
		// The iterator/extent type for running through the above
		// regions.
	typedef typename Region_t::Allocator Allocator;
		// The allocator type for region extents.

public:
	explicit FloodFillControl (Allocator &a_rAllocator
			= Region_t::Extents::Imp::sm_oNodeAllocator);
		// Default constructor.  Must be followed by a call to Init().

	FloodFillControl (Status_t &a_reStatus, Allocator &a_rAllocator
			= Region_t::Extents::Imp::sm_oNodeAllocator);
		// Initializing constructor.

	void Init (Status_t &a_reStatus);
		// Initializer.  Must be called on default-constructed objects.
		// (May not be valid to call on subclasses, depending on
		// whether more parameters are needed for its Init().)

	void Purge (void);
		// Purge all internally-allocated memory.

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



// Default constructor.  Must be followed by Init().
template <class INDEX, class SIZE, class SETIMP>
SetRegion2D<INDEX,SIZE,SETIMP>::SetRegion2D (Allocator &a_rAlloc)
	: m_setExtents (Less<Extent>(), a_rAlloc)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // NDEBUG

	// No points yet.
	m_tnPoints = 0;

	// Check the invariant by default; they'll have to specifically
	// request not to.
	#ifdef DEBUG_SETREGION2D
	m_bDebug = true;
	#endif // DEBUG_SETREGION2D
}



// Initializing constructor.  Creates an empty region.
template <class INDEX, class SIZE, class SETIMP>
SetRegion2D<INDEX,SIZE,SETIMP>::SetRegion2D (Status_t &a_reStatus,
		const InitParams &a_rInitParams, Allocator &a_rAlloc)
	: m_setExtents (a_reStatus, false, a_rInitParams, Less<Extent>(),
		a_rAlloc)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // NDEBUG

	// No points yet.
	m_tnPoints = 0;

	// Check the invariant by default; they'll have to specifically
	// request not to.
	#ifdef DEBUG_SETREGION2D
	m_bDebug = true;
	#endif // DEBUG_SETREGION2D
}



// Copy constructor.
template <class INDEX, class SIZE, class SETIMP>
SetRegion2D<INDEX,SIZE,SETIMP>::SetRegion2D (Status_t &a_reStatus,
		const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther)
	: m_setExtents (a_reStatus, false, Less<Extent>(),
		a_rOther.m_setExtents.m_oImp.m_rNodeAllocator)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // NDEBUG

	// No points yet.
	m_tnPoints = 0;

	// Check the invariant by default; they'll have to specifically
	// request not to.
	#ifdef DEBUG_SETREGION2D
	m_bDebug = true;
	#endif // DEBUG_SETREGION2D

	// If the construction of m_setExtents failed, bail.
	if (a_reStatus != g_kNoError)
		return;

	// Copy all the extents.
	m_setExtents.Insert (a_reStatus, a_rOther.m_setExtents.Begin(),
		a_rOther.m_setExtents.End());
	if (a_reStatus != g_kNoError)
		return;

	// Now we have as many points as the copied region.
	m_tnPoints = a_rOther.m_tnPoints;

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D
}



// Initializer.  Must be called on default-constructed regions.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Init (Status_t &a_reStatus,
	const InitParams &a_rInitParams)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize our set of extents.
	m_setExtents.Init (a_reStatus, false, a_rInitParams);
	if (a_reStatus != g_kNoError)
		return;
}



// Make the current region a copy of the other region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Assign (Status_t &a_reStatus,
	const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure both regions are intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	a_rOther.Invariant();
	#endif // DEBUG_SETREGION2D

	// Assign the other region's extents to ourselves.
	m_setExtents.Assign (a_reStatus, a_rOther.m_setExtents);
	if (a_reStatus != g_kNoError)
		return;

	// Now we have as many points as they do.
	m_tnPoints = a_rOther.m_tnPoints;

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D
}



// Destructor.
template <class INDEX, class SIZE, class SETIMP>
SetRegion2D<INDEX,SIZE,SETIMP>::~SetRegion2D()
{
	// One less instance.
	#ifndef NDEBUG
	--sm_ulInstances;
	#endif // NDEBUG
}



#ifdef DEBUG_SETREGION2D

// Set whether to run the region invariant before and after
// methods.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::SetDebug (bool a_bDebug)
{
	// Easy enough.
	m_bDebug = a_bDebug;

	// Have the set of extents check itself too, to make sure we didn't
	// do anything to break it.
	#ifdef DEBUG_SKIPLIST
	m_setExtents.SetDebug (a_bDebug);
	#endif // DEBUG_SKIPLIST
}



// Thoroughly analyze the region for structural integrity.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Invariant (void) const
{
	SIZE tnPoints;
		// Our total of the number of points in the region.
	typename Extents::ConstIterator itHere, itNext;
		// Used to run through the extents.

	// Only check the invariant if they requested we do.
	if (!m_bDebug)
		return;

	// Make sure the contained set is intact.  (That will verify that
	// the extents are sorted properly, so we don't have to do that
	// here.)
	#ifdef DEBUG_SKIPLIST
	m_setExtents.Invariant();
	#endif // DEBUG_SKIPLIST

	// Run through the extents, make sure that they're not contiguous
	// with each other, and count up the number of contained points.
	tnPoints = 0;
	for (itHere = m_setExtents.Begin();
		 itHere != m_setExtents.End();
		 itHere = itNext)
	{
		// Find the next extent.
		itNext = itHere;
		++itNext;

		// Make sure that the extents aren't horizontally contiguous.
		// (Horizontally contiguous extents should have been merged
		// together.)
		assert (itNext == m_setExtents.End()
			|| (*itHere).m_tnY != (*itNext).m_tnY
			|| (*itHere).m_tnXEnd < (*itNext).m_tnXStart);

		// Add the number of points in this extent to our total.
		tnPoints += (*itHere).m_tnXEnd - (*itHere).m_tnXStart;
	}

	// Make sure the point total is accurate.
	assert (m_tnPoints == tnPoints);
}

#endif // DEBUG_SETREGION2D



// Return the total number of points contained by the region.
template <class INDEX, class SIZE, class SETIMP>
inline SIZE
SetRegion2D<INDEX,SIZE,SETIMP>::NumberOfPoints (void) const
{
	// Easy enough.
	return m_tnPoints;
}



// Clear the region, emptying it of all extents.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Clear (void)
{
	// Easy enough.
	m_setExtents.Clear();
	m_tnPoints = 0;
}



// Purge all internally-allocated memory.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Purge (void)
{
	// Make sure the region is empty.
	assert (m_tnPoints == 0);

	// Easy enough.
	m_setExtents.Purge();
}



// Add the given horizontal extent to the region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Union (Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd)
{
	Extent oKey;
		// An extent being searched for.
	Extent oInserted;
		// The extent being added, modified to account for the extents
		// already present in the region.
	typename Extents::Iterator itStart, itEnd;
		// The range of existing extents that gets removed because of
		// the addition of the new extent.
	typename Extents::Iterator itHere;
		// An extent being examined and/or modified.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a non-empty extent.
	assert (a_tnXStart < a_tnXEnd);

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D

	// The extent we'll be inserting starts as the extent they asked
	// to add.  That may get modified based on the nature of the extents
	// already present in the region.
	oInserted.m_tnY = a_tnY;
	oInserted.m_tnXStart = a_tnXStart;
	oInserted.m_tnXEnd = a_tnXEnd;

	// Find the first extent that may get removed because of this new
	// extent.
	// (That's the one before the first existing extent
	// that's > (y, x-start).)
	oKey.m_tnY = a_tnY;
	oKey.m_tnXStart = a_tnXStart;
	itStart = m_setExtents.UpperBound (oKey);
	--itStart;

	// Does the found extent intersect the new extent?
	if (itStart != m_setExtents.End()
	&& (*itStart).m_tnY == a_tnY
	&& (*itStart).m_tnXEnd >= a_tnXStart)
	{
		// The found extent intersects with the new extent.

		// If the found extent contains the new extent, exit now;
		// the region already contains the new extent, and no
		// modifications are necessary.
		if ((*itStart).m_tnXEnd >= a_tnXEnd)
			return;

		// The found extent will be removed, and the inserted extent
		// will start in the same location (which we know is less than
		// or equal to the inserted extent's current start, thanks to
		// the search we did earlier).
		oInserted.m_tnXStart = (*itStart).m_tnXStart;

		// If the next extent in the region doesn't intersect the one
		// being added, we can modify this extent & be done, without
		// having to do a 2nd upper-bound search.
		itEnd = itStart;
		++itEnd;
		if (itEnd == m_setExtents.End()
			|| (*itEnd).m_tnY != (*itStart).m_tnY
			|| (*itEnd).m_tnXStart > a_tnXEnd)
		{
			// We can modify this one extent & be done.  Keep the
			// largest end.
			if (oInserted.m_tnXEnd < (*itStart).m_tnXEnd)
				oInserted.m_tnXEnd = (*itStart).m_tnXEnd;

			// Adjust the number of points we contain.
			m_tnPoints += (oInserted.m_tnXEnd - oInserted.m_tnXStart)
				- ((*itStart).m_tnXEnd - (*itStart).m_tnXStart);

			// Modify the extent.
			*itStart = oInserted;

			// Make sure we're intact.
			#ifdef DEBUG_SETREGION2D
			Invariant();
			#endif // DEBUG_SETREGION2D

			// We're done.
			return;
		}
	}
	else
	{
		// The found extent doesn't intersect with the new extent.
		// Therefore, it won't get modified or removed by the addition
		// of the new extent.  Move past it.
		++itStart;
	}

	// Find the last extent that may get removed because of this new
	// extent.  Start by searching for the first existing extent
	// that's > (y, x-end), then move back one.)
	oKey.m_tnY = a_tnY;
	oKey.m_tnXStart = a_tnXEnd;
	itEnd = m_setExtents.UpperBound (oKey);
	--itEnd;

	// Does the found extent intersect the new extent?
	if (itEnd != m_setExtents.End()
	&& (*itEnd).m_tnY == a_tnY
	&& (*itEnd).m_tnXStart <= a_tnXEnd)
	{
		// Yes.  That extent will get replaced, and its endpoint may be
		// used by the inserted extent.
		if (oInserted.m_tnXEnd < (*itEnd).m_tnXEnd)
			oInserted.m_tnXEnd = (*itEnd).m_tnXEnd;
	}

	// In either case, move ahead again, to get back to the end of the
	// range we'll be removing.
	++itEnd;

	// We now have the actual extent to be inserted, and the range of
	// existing extents to be removed.

	// Run through the extents to be removed, count the number of points
	// they represent, and subtract that from our separately-maintained
	// total number of points in the region.
	for (itHere = itStart; itHere != itEnd; ++itHere)
		m_tnPoints -= (*itHere).m_tnXEnd - (*itHere).m_tnXStart;

	// If the range to be replaced has at least one existing item, then
	// move the start of the range forward by one, and overwrite the
	// old start with the extent to be inserted, i.e. avoid a memory
	// allocation if at all possible.
	if (itStart != itEnd)
	{
		// Keep track of the location of the extent to be overwritten.
		itHere = itStart;

		// Move past the extent that'll be overwritten.
		++itStart;

		// Remove all extents that were found to conflict with the one
		// being inserted.
		m_setExtents.Erase (itStart, itEnd);

		// Store the new extent.
		*itHere = oInserted;
	}

	// Otherwise, no extents are being removed, and we just insert the
	// new extent.
	else
	{
		#ifndef NDEBUG
		typename Extents::InsertResult oInsertResult =
		#endif // NDEBUG
			m_setExtents.Insert (a_reStatus, oInserted);
		if (a_reStatus != g_kNoError)
			return;
		assert (oInsertResult.m_bInserted);
	}

	// The region now contains this many more points.
	m_tnPoints += oInserted.m_tnXEnd - oInserted.m_tnXStart;

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D
}



// Merge this extent into the current region.
// The new extent can't intersect the region in any way.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Merge (Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd)
{
	Extent oExtent;
		// The new extent being added.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Generate the new extent.
	oExtent.m_tnY = a_tnY;
	oExtent.m_tnXStart = a_tnXStart;
	oExtent.m_tnXEnd = a_tnXEnd;

	// We will contain this many more points.
	m_tnPoints += a_tnXEnd - a_tnXStart;

	// Add this extent to the current region.
	#ifndef NDEBUG
	typename Extents::InsertResult oInsertResult =
	#endif // NDEBUG
		m_setExtents.Insert (a_reStatus, oExtent);
	if (a_reStatus != g_kNoError)
		return;
	assert (oInsertResult.m_bInserted);

	// Make sure the new extent is not contiguous with the extent
	// in front of & behind it.
	#ifndef NDEBUG
	{
		typename Extents::Iterator itNew, itOther;

		// Get the location of the newly moved extent.
		itNew = oInsertResult.m_itPosition;

		// Make sure it's not contiguous with the extent before it.
		itOther = itNew;
		--itOther;
		assert (itOther == m_setExtents.End()
			|| (*itOther).m_tnY != (*itNew).m_tnY
			|| (*itOther).m_tnXEnd < (*itNew).m_tnXStart);

		// Make sure it's not contiguous with the extent after it.
		itOther = itNew;
		++itOther;
		assert (itOther == m_setExtents.End()
			|| (*itNew).m_tnY != (*itOther).m_tnY
			|| (*itNew).m_tnXEnd < (*itOther).m_tnXStart);
	}
	#endif // NDEBUG
}



// Merge the other region into ourselves, emptying the other region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Merge
	(SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther)
{
	// Make sure we can move extents between regions.
	assert (CanMove (a_rOther));

	// If the current region is empty, just move all the other region's
	// extents over at once.
	if (m_tnPoints == 0)
	{
		Move (a_rOther);
	}
	else
	{
		typename Extents::Iterator itHere, itNext;
			// Where we are in the other region's extents.

		// Run through the extents in the other region, move them to the
		// current region, and make sure the new extent isn't contiguous
		// with any of our existing extents.
		for (itHere = a_rOther.m_setExtents.Begin();
			 itHere != a_rOther.m_setExtents.End();
			 itHere = itNext)
		{
			// Get the location of the next extent.  (We have to do this
			// because, after we call Move(), itHere isn't valid any
			// more.)
			itNext = itHere;
			++itNext;

			// We will contain this many more points.
			m_tnPoints += (*itHere).m_tnXEnd - (*itHere).m_tnXStart;

			// Move this extent to the current region.
			#ifndef NDEBUG
			typename Extents::InsertResult oInsertResult =
			#endif // NDEBUG
				m_setExtents.Move (a_rOther.m_setExtents, itHere);
			assert (oInsertResult.m_bInserted);

			// Make sure the new extent is not contiguous with the
			// extent in front of & behind it.
			#ifndef NDEBUG
			{
				typename Extents::Iterator itNew, itOther;

				// Get the location of the newly moved extent.
				itNew = oInsertResult.m_itPosition;

				// Make sure it's not contiguous with the extent before
				// it.
				itOther = itNew;
				--itOther;
				assert (itOther == m_setExtents.End()
					|| (*itOther).m_tnY != (*itNew).m_tnY
					|| (*itOther).m_tnXEnd < (*itNew).m_tnXStart);

				// Make sure it's not contiguous with the extent after
				// it.
				itOther = itNew;
				++itOther;
				assert (itOther == m_setExtents.End()
					|| (*itNew).m_tnY != (*itOther).m_tnY
					|| (*itNew).m_tnXEnd < (*itOther).m_tnXStart);
			}
			#endif // NDEBUG
		}
	}

	// Now the other region is empty.
	a_rOther.m_tnPoints = 0;
}



// Move the contents of the other region into the current region.
// The current region must be empty.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Move
	(SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther)
{
	// Make sure we're allowed to do this.
	assert (CanMove (a_rOther));

	// Make sure the current region is empty.
	assert (m_tnPoints == 0);

	// Move the extents.
	m_setExtents.Move (a_rOther.m_setExtents);

	// Now we have all their points.
	m_tnPoints = a_rOther.m_tnPoints;
	a_rOther.m_tnPoints = 0;
}



// Returns true if the other region's contents can be moved
// into the current region.
template <class INDEX, class SIZE, class SETIMP>
bool
SetRegion2D<INDEX,SIZE,SETIMP>::CanMove
	(const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther) const
{
	// Return whether our extents can be moved.
	return m_setExtents.CanMove (a_rOther.m_setExtents);
}



// Make the current region represent the intersection between
// itself and the other given region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Intersection (Status_t &a_reStatus,
	const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Not implemented yet.
	assert (false);
	a_reStatus = g_kInternalError;
}



// Subtract the given horizontal extent from the region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Subtract
	(Status_t &a_reStatus, INDEX a_tnY,
	INDEX a_tnXStart, INDEX a_tnXEnd,
	const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther,
	typename Extents::ConstIterator &a_ritHere)
{
	Extent oKey;
		// Used to search through the existing extents.
	typename Extents::Iterator itStart, itEnd;
		// The range of existing extents that gets removed because of
		// the subtraction of the new extent.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a non-empty extent.
	assert (a_tnXStart < a_tnXEnd);

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D

	// Find the first extent that may get removed or modified by this
	// subtraction.  (That's the first existing extent
	// that's >= (y, x-start).)
	oKey.m_tnY = a_tnY;
	oKey.m_tnXStart = a_tnXStart;
	itStart = m_setExtents.LowerBound (oKey);

	// Figure out if no existing extents intersect with the beginning
	// of the extent we're trying to subtract.
	if (itStart == m_setExtents.End()
	|| (*itStart).m_tnY != a_tnY
	|| (*itStart).m_tnXStart > a_tnXStart)
	{
		// The found extent is past any extent that the beginning would
		// intersect with.  Move back one.
		--itStart;

		// See if this extent intersects the beginning of the range
		// we're trying to subtract.
		if (itStart == m_setExtents.End()
		|| (*itStart).m_tnY != a_tnY
		|| (*itStart).m_tnXEnd <= a_tnXStart)
		{
			// Move forward again.  If that extent is on a different
			// line, or it starts after the removed range ends, then
			// leave: there's nothing to subtract.
			++itStart;
			if (itStart == m_setExtents.End()
			|| (*itStart).m_tnY != a_tnY
			|| (*itStart).m_tnXStart >= a_tnXEnd)
			{
				// There's nothing to subtract.  Find the first extent
				// in a_rOther that may intersect with one of our
				// extents.
				if (a_ritHere != a_rOther.m_setExtents.End()
					&& itStart != m_setExtents.End())
				{
					a_ritHere = a_rOther.m_setExtents.UpperBound
						(*itStart);
					--a_ritHere;
				}
				return;
			}
		}
	}

	// We found the first extent that may intersect with the range we're
	// trying to subtract.

	// Sanity check: make sure the found extent is on the same line as
	// the range we're trying to subtract.
	assert (itStart != m_setExtents.End() && (*itStart).m_tnY == a_tnY);

	// We have 4 cases: that extent has its beginning chopped off,
	// its end chopped off, gets removed completely, or gets broken in
	// half.
	if ((*itStart).m_tnXStart >= a_tnXStart)
	{
		if ((*itStart).m_tnXEnd > a_tnXEnd)
		{
			// The found extent gets its beginning cut off, and then
			// we're done.
			m_tnPoints -= a_tnXEnd - (*itStart).m_tnXStart;
			(*itStart).m_tnXStart = a_tnXEnd;
			return;
		}
		else
		{
			// The found extent gets removed completely.
			// Fall through to deal with the end of the range being
			// subtracted.
		}
	}
	else
	{
		if ((*itStart).m_tnXEnd > a_tnXEnd)
		{
			// The found extent gets broken in half.

			Extent oInserted;
				// The second half of the broken extent.

			// Prepare to break the extent in half, by generating the
			// second half of the extent.
			oInserted.m_tnY = a_tnY;
			oInserted.m_tnXStart = a_tnXEnd;
			oInserted.m_tnXEnd = (*itStart).m_tnXEnd;

			// Modify the found extent so that it becomes the first
			// half of the broken extent.  This is safe because it
			// doesn't affect the consistency of the region.
			(*itStart).m_tnXEnd = a_tnXStart;

			// Insert the second half of the broken extent.
			{
				#ifndef NDEBUG
				typename Extents::InsertResult oInsertResult =
				#endif // NDEBUG
					m_setExtents.Insert (a_reStatus, oInserted);
				if (a_reStatus != g_kNoError)
					return;
				assert (oInsertResult.m_bInserted);
			}

			// We removed this many points.
			m_tnPoints -= a_tnXEnd - a_tnXStart;

			// We're done.
			return;
		}
		else
		{
			// The found extent gets its end cut off.

			// Remember the end of the found extent.
			INDEX nOldEnd = (*itStart).m_tnXEnd;

			// Cut off the end of the found extent.
			m_tnPoints -= (*itStart).m_tnXEnd - a_tnXStart;
			(*itStart).m_tnXEnd = a_tnXStart;

			// If the extent being subtracted ends at the same place
			// as this found extent, then we're done.
			if (nOldEnd == a_tnXEnd)
				return;

			// Move forward.
			++itStart;

			// If the next extent starts after the end of this subtraction,
			// we're done.
			// (This seemed like a good idea, but actualy seems to make it
			// run slightly slower in practice!  Argh.)
			#if 0
			assert (itStart == m_setExtents.End()
				|| (*itStart).m_tnY >= a_tnY);
			if (itStart == m_setExtents.End()
				|| (*itStart).m_tnY > a_tnY
				|| ( /* (*itStart).m_tnY == a_tnY
					&& */ (*itStart).m_tnXStart > a_tnXEnd))
				return;
			#endif

			// Fall through to deal with the end of the range being
			// subtracted.
		}
	}

	// Find the last extent that may get removed or modified by this
	// subtraction.  (That's the first existing extent
	// that's >= (y, x-end).)
	oKey.m_tnY = a_tnY;
	oKey.m_tnXStart = a_tnXEnd;
	itEnd = m_setExtents.LowerBound (oKey);

	// If that put us past any extent we'll have to remove, we can just
	// skip to the end, where the range we located will be removed.
	// Otherwise, we have to figure out the implications of the end of
	// the subtracted range.
	if (itEnd == m_setExtents.End()
	|| (*itEnd).m_tnY != a_tnY
	|| (*itEnd).m_tnXStart > a_tnXEnd)
	{
		// Move back one.
		--itEnd;

		// If that didn't put us out of the range of existing extents
		// that are getting removed, we have 2 cases: either the entire
		// extent is getting removed, or its beginning is getting
		// chopped off.
		if (itEnd != m_setExtents.End() && (*itEnd).m_tnY == a_tnY)
		{
			// How much of this extent is getting removed?
			if ((*itEnd).m_tnXEnd <= a_tnXEnd)
			{
				// The entire extent is getting removed.  Move forward
				// again.
				++itEnd;
			}
			else
			{
				// The found extent's beginning is getting chopped off.
				// (The search above ensures that this extent starts
				// before the end of the range being removed, but we
				// check our sanity anyway.)
				assert ((*itEnd).m_tnXStart < a_tnXEnd);
				m_tnPoints -= a_tnXEnd - (*itEnd).m_tnXStart;
				(*itEnd).m_tnXStart = a_tnXEnd;
			}
		}
	}

	// Loop through the range of extents to remove, subtract their
	// contribution to our point total.
	for (typename Extents::Iterator itHere = itStart;
		 itHere != itEnd;
		 itHere++)
	{
		m_tnPoints -= (*itHere).m_tnXEnd - (*itHere).m_tnXStart;
	}

	// Remove any range of extents found.
	m_setExtents.Erase (itStart, itEnd);

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D
}



// Subtract the given horizontal extent from the region, with the given
// offset.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::SubtractWithOffset (Status_t &a_reStatus,
	INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd,
	INDEX a_tnXOffset, INDEX a_tnYOffset,
	const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther,
	typename Extents::ConstIterator &a_ritHere)
{
	Extent oKey;
		// Used to search through the existing extents.
	typename Extents::Iterator itStart, itEnd;
		// The range of existing extents that gets removed because of
		// the subtraction of the new extent.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a non-empty extent.
	assert (a_tnXStart < a_tnXEnd);

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D

	// Find the first extent that may get removed or modified by this
	// subtraction.  (That's the first existing extent
	// that's >= (y, x-start).)
	oKey.m_tnY = a_tnY + a_tnYOffset;
	oKey.m_tnXStart = a_tnXStart + a_tnXOffset;
	itStart = m_setExtents.LowerBound (oKey);

	// Figure out if no existing extents intersect with the beginning
	// of the extent we're trying to subtract.
	if (itStart == m_setExtents.End()
	|| (*itStart).m_tnY != a_tnY + a_tnYOffset
	|| (*itStart).m_tnXStart > a_tnXStart + a_tnXOffset)
	{
		// The found extent is past any extent that the beginning would
		// intersect with.  Move back one.
		--itStart;

		// See if this extent intersects the beginning of the range
		// we're trying to subtract.
		if (itStart == m_setExtents.End()
		|| (*itStart).m_tnY != a_tnY + a_tnYOffset
		|| (*itStart).m_tnXEnd <= a_tnXStart + a_tnXOffset)
		{
			// Move forward again.  If that extent is on a different
			// line, or it starts after the removed range ends, then
			// leave: there's nothing to subtract.
			++itStart;
			if (itStart == m_setExtents.End()
			|| (*itStart).m_tnY != a_tnY + a_tnYOffset
			|| (*itStart).m_tnXStart >= a_tnXEnd + a_tnXOffset)
			{
				// There's nothing to subtract.  Find the first extent
				// in a_rOther that may intersect with one of our
				// extents.
				if (a_ritHere != a_rOther.m_setExtents.End()
					&& itStart != m_setExtents.End())
				{
					oKey.m_tnY = (*itStart).m_tnY - a_tnYOffset;
					oKey.m_tnXStart = (*itStart).m_tnXStart - a_tnXOffset;
					a_ritHere = a_rOther.m_setExtents.UpperBound (oKey);
					--a_ritHere;
				}
				return;
			}
		}
	}

	// We found the first extent that may intersect with the range we're
	// trying to subtract.

	// Sanity check: make sure the found extent is on the same line as
	// the range we're trying to subtract.
	assert (itStart != m_setExtents.End()
		&& (*itStart).m_tnY == a_tnY + a_tnYOffset);

	// We have 4 cases: that extent has its beginning chopped off,
	// its end chopped off, gets removed completely, or gets broken in
	// half.
	if ((*itStart).m_tnXStart >= a_tnXStart + a_tnXOffset)
	{
		if ((*itStart).m_tnXEnd > a_tnXEnd + a_tnXOffset)
		{
			// The found extent gets its beginning cut off, and then
			// we're done.
			m_tnPoints -= (a_tnXEnd + a_tnXOffset) - (*itStart).m_tnXStart;
			(*itStart).m_tnXStart = a_tnXEnd + a_tnXOffset;
			return;
		}
		else
		{
			// The found extent gets removed completely.
			// Fall through to deal with the end of the range being
			// subtracted.
		}
	}
	else
	{
		if ((*itStart).m_tnXEnd > a_tnXEnd + a_tnXOffset)
		{
			// The found extent gets broken in half.

			Extent oInserted;
				// The second half of the broken extent.

			// Prepare to break the extent in half, by generating the
			// second half of the extent.
			oInserted.m_tnY = a_tnY + a_tnYOffset;
			oInserted.m_tnXStart = a_tnXEnd + a_tnXOffset;
			oInserted.m_tnXEnd = (*itStart).m_tnXEnd;

			// Modify the found extent so that it becomes the first
			// half of the broken extent.  This is safe because it
			// doesn't affect the consistency of the region.
			(*itStart).m_tnXEnd = a_tnXStart + a_tnXOffset;

			// Insert the second half of the broken extent.
			{
				#ifndef NDEBUG
				typename Extents::InsertResult oInsertResult =
				#endif // NDEBUG
					m_setExtents.Insert (a_reStatus, oInserted);
				if (a_reStatus != g_kNoError)
					return;
				assert (oInsertResult.m_bInserted);
			}

			// We removed this many points.
			m_tnPoints -= a_tnXEnd - a_tnXStart;

			// We're done.
			return;
		}
		else
		{
			// The found extent gets its end cut off.

			// Remember the end of the found extent.
			INDEX nOldEnd = (*itStart).m_tnXEnd + a_tnXOffset;

			// Cut off the end of the found extent.
			m_tnPoints -= (*itStart).m_tnXEnd - (a_tnXStart + a_tnXOffset);
			(*itStart).m_tnXEnd = a_tnXStart + a_tnXOffset;

			// If the extent being subtracted ends at the same place
			// as this found extent, then we're done.
			if (nOldEnd == a_tnXEnd + a_tnXOffset)
				return;

			// Move forward, and fall through to deal with the end of
			// the range being subtracted.
			++itStart;
		}
	}

	// Find the last extent that may get removed or modified by this
	// subtraction.  (That's the first existing extent
	// that's >= (y, x-end).)
	oKey.m_tnY = a_tnY + a_tnYOffset;
	oKey.m_tnXStart = a_tnXEnd + a_tnXOffset;
	itEnd = m_setExtents.LowerBound (oKey);

	// If that put us past any extent we'll have to remove, we can just
	// skip to the end, where the range we located will be removed.
	// Otherwise, we have to figure out the implications of the end of
	// the subtracted range.
	if (itEnd == m_setExtents.End()
	|| (*itEnd).m_tnY != a_tnY + a_tnYOffset
	|| (*itEnd).m_tnXStart > a_tnXEnd + a_tnXOffset)
	{
		// Move back one.
		--itEnd;

		// If that didn't put us out of the range of existing extents
		// that are getting removed, we have 2 cases: either the entire
		// extent is getting removed, or its beginning is getting
		// chopped off.
		if (itEnd != m_setExtents.End()
			&& (*itEnd).m_tnY == a_tnY + a_tnYOffset)
		{
			// How much of this extent is getting removed?
			if ((*itEnd).m_tnXEnd <= a_tnXEnd + a_tnXOffset)
			{
				// The entire extent is getting removed.  Move forward
				// again.
				++itEnd;
			}
			else
			{
				// The found extent's beginning is getting chopped off.
				// (The search above ensures that this extent starts
				// before the end of the range being removed, but we
				// check our sanity anyway.)
				assert ((*itEnd).m_tnXStart < a_tnXEnd + a_tnXOffset);
				m_tnPoints -= (a_tnXEnd + a_tnXOffset)
					- (*itEnd).m_tnXStart;
				(*itEnd).m_tnXStart = a_tnXEnd + a_tnXOffset;
			}
		}
	}

	// Loop through the range of extents to remove, subtract their
	// contribution to our point total.
	for (typename Extents::Iterator itHere = itStart;
		 itHere != itEnd;
		 itHere++)
	{
		m_tnPoints -= (*itHere).m_tnXEnd - (*itHere).m_tnXStart;
	}

	// Remove any range of extents found.
	m_setExtents.Erase (itStart, itEnd);

	// Make sure we're intact.
	#ifdef DEBUG_SETREGION2D
	Invariant();
	#endif // DEBUG_SETREGION2D
}



// Subtract the given horizontal extent from the region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Subtract (Status_t &a_reStatus,
	INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd)
{
	// Defer to our private helper function.
	ConstIterator itUnused = m_setExtents.End();
	Subtract (a_reStatus, a_tnY, a_tnXStart, a_tnXEnd, *this,
		itUnused);
}



// Subtract the other region from the current region, i.e.
// remove from the current region any areas that exist in the
// other region.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Subtract (Status_t &a_reStatus,
	const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther)
{
	typename Extents::ConstIterator itHere, itNext;
		// Where we are in the other region's extents.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Run through the extents in the other region, subtract them from
	// the current region.
	for (itHere = a_rOther.m_setExtents.Begin();
		 itHere != a_rOther.m_setExtents.End();
		 itHere = itNext)
	{
		// Subtract this extent from the current region.
		itNext = itHere;
		Subtract (a_reStatus, (*itHere).m_tnY, (*itHere).m_tnXStart,
			(*itHere).m_tnXEnd, a_rOther, itNext);
		if (a_reStatus != g_kNoError)
			return;

		// If this region is now empty, leave.
		if (m_setExtents.Size() == 0)
			return;

		// If there was no intersection, then itNext has the location of
		// the first extent in a_rOther that intersects one of our
		// extents.  Otherwise, move forward.
		if (itHere == itNext)
			++itNext;
	}
}



// Subtract the other region from the current region, but offset
// the other region by the given amounts.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::SubtractWithOffset (Status_t &a_reStatus,
	const SetRegion2D<INDEX,SIZE,SETIMP> &a_rOther,
	INDEX a_tnXOffset, INDEX a_tnYOffset)
{
	typename Extents::ConstIterator itHere, itNext;
		// Where we are in the other region's extents.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Run through the extents in the other region, subtract them from
	// the current region.
	for (itHere = a_rOther.m_setExtents.Begin();
		 itHere != a_rOther.m_setExtents.End();
		 itHere = itNext)
	{
		// Subtract this extent from the current region.
		itNext = itHere;
		SubtractWithOffset (a_reStatus, (*itHere).m_tnY,
			(*itHere).m_tnXStart, (*itHere).m_tnXEnd,
			a_tnXOffset, a_tnYOffset, a_rOther, itNext);
		if (a_reStatus != g_kNoError)
			return;

		// If this region is now empty, leave.
		if (m_setExtents.Size() == 0)
			return;

		// If there was no intersection, then itNext has the location of
		// the first extent in a_rOther that intersects one of our
		// extents.  Otherwise, move forward.
		if (itHere == itNext)
			++itNext;
	}
}



// Move all extents by the given offset.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::Offset
	(INDEX a_tnXOffset, INDEX a_tnYOffset)
{
	typename Extents::Iterator itHere;
		// Where we're offsetting an extent.

	// Run through all the extents, offset them.  (NOTE: during this
	// operation, the skip-list's sorting is broken, but we don't
	// leave it broken.
	for (itHere = m_setExtents.Begin();
		 itHere != m_setExtents.End();
		 ++itHere)
	{
		Extent &rExtent = *itHere;
		rExtent.m_tnY += a_tnYOffset;
		rExtent.m_tnXStart += a_tnXOffset;
		rExtent.m_tnXEnd += a_tnXOffset;
	}
}



// Returns true if the region contains the given point.
template <class INDEX, class SIZE, class SETIMP>
bool
SetRegion2D<INDEX,SIZE,SETIMP>::DoesContainPoint (INDEX a_tnY, INDEX a_tnX)
{
	Extent oKey;
	ConstIterator itKey;
		// Used to search for the extent that would contain this point.

	// Find the extent that would contain this point.
	oKey.m_tnY = a_tnY;
	oKey.m_tnXStart = a_tnX;
	itKey = m_setExtents.UpperBound (oKey);
	--itKey;

	// Return whether it does.
	if (itKey != m_setExtents.End()
	&& (*itKey).m_tnY == a_tnY
	&& a_tnX >= (*itKey).m_tnXStart
	&& a_tnX < (*itKey).m_tnXEnd)
		return true;
	else
		return false;
}



// Returns true if the region contains the given point.
template <class INDEX, class SIZE, class SETIMP>
bool
SetRegion2D<INDEX,SIZE,SETIMP>::DoesContainPoint (INDEX a_tnY, INDEX a_tnX,
	ConstIterator &a_ritHere)
{
	Extent oKey;
	ConstIterator itKey;
		// Used to search for the extent that would contain this point.

	// Find the extent that would contain this point.
	oKey.m_tnY = a_tnY;
	oKey.m_tnXStart = a_tnX;
	itKey = m_setExtents.UpperBound (oKey);
	--itKey;

	// Pass it back to our caller.
	a_ritHere = itKey;

	// Return whether it does.
	if (itKey != m_setExtents.End()
	&& (*itKey).m_tnY == a_tnY
	&& a_tnX >= (*itKey).m_tnXStart
	&& a_tnX < (*itKey).m_tnXEnd)
		return true;
	else
		return false;
}



// Flood-fill the current region.
template <class INDEX, class SIZE, class SETIMP>
template <class CONTROL>
void
SetRegion2D<INDEX,SIZE,SETIMP>::FloodFill (Status_t &a_reStatus,
	CONTROL &a_rControl, bool a_bVerify, bool a_bExpand)
{
	typename CONTROL::ConstIterator itExtent;
		// An extent we're examining.
	Extent oFoundExtent;
		// An extent we found to be part of the region.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make the compiler shut up.
	oFoundExtent.m_tnXStart = oFoundExtent.m_tnXEnd = 0;

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
							oFoundExtent.m_tnXStart,
							oFoundExtent.m_tnXEnd);
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
template <class INDEX, class SIZE, class SETIMP>
template <class REGION>
void
SetRegion2D<INDEX,SIZE,SETIMP>::MakeBorder (Status_t &a_reStatus,
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
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::UnionSurroundingExtents
	(Status_t &a_reStatus, INDEX a_tnY, INDEX a_tnXStart, INDEX a_tnXEnd)
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



#ifndef NDEBUG
template <class INDEX, class SIZE, class SETIMP>
uint32_t SetRegion2D<INDEX,SIZE,SETIMP>::sm_ulInstances;
#endif // NDEBUG



// Default constructor.  Must be followed by a call to Init().
template <class INDEX, class SIZE, class SETIMP>
SetRegion2D<INDEX,SIZE,SETIMP>::FloodFillControl::FloodFillControl
		(Allocator &a_rAllocator)
	: m_oToDo (a_rAllocator),
	  m_oAlreadyDone (a_rAllocator),
	  m_oNextToDo (a_rAllocator)
{
	// Nothing to do.
}



// Initializing constructor.
template <class INDEX, class SIZE, class SETIMP>
SetRegion2D<INDEX,SIZE,SETIMP>::FloodFillControl::FloodFillControl
		(Status_t &a_reStatus, Allocator &a_rAllocator)
	: m_oToDo (a_rAllocator),
	  m_oAlreadyDone (a_rAllocator),
	  m_oNextToDo (a_rAllocator)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize ourselves.
	Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



// Initializer.  Must be called on default-constructed objects.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::FloodFillControl::Init
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



// Purge all internally-allocated memory.
template <class INDEX, class SIZE, class SETIMP>
void
SetRegion2D<INDEX,SIZE,SETIMP>::FloodFillControl::Purge (void)
{
	// Get rid of the contents of our internal regions.
	// (These aren't cleared at the end of FloodFill() because we'd
	// rather reuse existing region-extent allocations, since that
	// involves less allocator thrashing and region-extent insertions.)
	m_oToDo.Clear();
	m_oAlreadyDone.Clear();
	m_oNextToDo.Clear();

	// Now purge internally-allocated memory.
	m_oToDo.Purge();
	m_oAlreadyDone.Purge();
	m_oNextToDo.Purge();
}



#endif // __SETREGION2D_H__
