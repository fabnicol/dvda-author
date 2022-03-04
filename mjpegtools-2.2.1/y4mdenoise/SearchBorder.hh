#ifndef __SEARCH_BORDER_H__
#define __SEARCH_BORDER_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include <assert.h>
#include <new>
#include "mjpeg_types.h"
#include "TemplateLib.hh"
#include "Limits.hh"
#include "DoublyLinkedList.hh"
#include "Allocator.hh"
#include "SetRegion2D.hh"
#ifdef	sun
#include <alloca.h>
#endif

// HACK: for development error messages.
#include <stdio.h>



// Define this to print region unions/subtractions.
#ifdef DEBUG_REGION2D
//	#define PRINTREGIONMATH
#endif // DEBUG_REGION2D



// Define this to print details of the search-border's progress.
#ifdef DEBUG_REGION2D
//	#define PRINT_SEARCHBORDER
#endif // DEBUG_REGION2D



// Define this to fix startpoint/endpoint-iterator arrays inline.
// Don't define this to use set-lower-bound operations to fix them.
// I was hoping the inline method was faster, but it doesn't seem to be,
// but I want to preserve the code, for reference if nothing else.
//#define FIX_ITERATOR_ARRAYS_INLINE



// The generic search-border class.  It's parameterized by the numeric
// type to use for pixel indices and a numeric type big enough to hold
// the product of the largest expected frame width/height.
// When constructed, it's configured with the size of the frame in which
// it operates, and the width/height of pixel groups to operate on.
//
// The search border keeps track of all regions on the border between
// the searched area and the not-yet-searched area.  When no new
// pixel-group could possibly intersect a region, that region is removed
// from the border and handed back to the client.
//
// The meaning of the regions it keeps track of is abstract.  The
// original purpose was to help the motion-searcher keep track of moved
// regions, i.e. pixels in the new frame that match pixels in the
// reference frame, only moved.  But it could presumably be used for
// other purposes, e.g. if it assembled regions of pixels that had
// exactly one frame reference, it could be used to detect bit noise,
// film lint/scratches, or even LaserDisc rot.
//
// As the search-border proceeds through the frame, it maintains a list
// of all regions that intersect with the current pixel-group.  That
// way, when it comes time to add a new match, all regions that
// intersect the current pixel-group are already known, and duplicate
// matches can be skipped.
template <class PIXELINDEX, class FRAMESIZE>
class SearchBorder
{
public:
	typedef Region2D<PIXELINDEX,FRAMESIZE> BaseRegion_t;
		// The base class for all our region types.

	#ifdef SET_REGION_IMPLEMENTED_WITH_VECTOR
	typedef Vector<typename BaseRegion_t::Extent,
				typename BaseRegion_t::Extent,
				Ident<typename BaseRegion_t::Extent,
					typename BaseRegion_t::Extent>,
				Less<typename BaseRegion_t::Extent> >
			RegionImp_t;
	#else // SET_REGION_IMPLEMENTED_WITH_VECTOR
	typedef SkipList<typename BaseRegion_t::Extent,
				typename BaseRegion_t::Extent,
				Ident<typename BaseRegion_t::Extent,
					typename BaseRegion_t::Extent>,
				Less<typename BaseRegion_t::Extent> >
			RegionImp_t;
	#endif // SET_REGION_IMPLEMENTED_WITH_VECTOR
		// The container class that implements our set-based region.

	typedef SetRegion2D<PIXELINDEX,FRAMESIZE,RegionImp_t> Region_t;
		// How we use SetRegion2D<>.

	class MovedRegion;
		// A moved region of pixels that has been detected.
		// Derived from Region_t; defined below.

	SearchBorder (typename Region_t::Allocator &a_rAlloc
			= Region_t::Extents::Imp::sm_oNodeAllocator);
		// Default constructor.

	virtual ~SearchBorder();
		// Destructor.

	void Init (Status_t &a_reStatus,
			PIXELINDEX a_tnWidth, PIXELINDEX a_tnHeight,
			PIXELINDEX a_tnSearchRadiusX, PIXELINDEX a_tnSearchRadiusY,
			PIXELINDEX a_tnPGW, PIXELINDEX a_tnPGH,
			FRAMESIZE a_tnMatchSizeThrottle);
		// Initializer.  Provide the dimensions of the frame, of the search
		// window, and of pixel-groups.
		// Also provide the match-size throttle.  The search-border will
		// keep track of regions of this size, but will avoid expensive
		// processing of them, and will expect its client to deal with all
		// such regions before moving to the next pixel-group.

	void StartFrame (Status_t &a_reStatus);
		// Initialize the search-border, i.e. start in the upper-left
		// corner.

	void MoveRight (Status_t &a_reStatus);
		// Move one pixel to the right, adding and removing regions from
		// the potentially-intersecting list.

	void MoveLeft (Status_t &a_reStatus);
		// Move one pixel to the left, adding and removing regions from
		// the potentially-intersecting list.

	void MoveDown (Status_t &a_reStatus);
		// Move down a line.  Find all regions that can no longer
		// be contiguous with new matches, and hand them back to the
		// client.  Then rotate the border structures, making the
		// least-recent current border into the last border, etc.

	FRAMESIZE GetSizeOfLargestActiveRegion (void) const;
		// Return the size of the largest active-region.
		// Returns zero if there are no active-regions.

	bool HasExistingMatch (PIXELINDEX a_tnMotionX,
			PIXELINDEX a_tnMotionY) const;
		// Determine if a region with this motion vector already
		// intersects/borders the current pixel-group (i.e. it doesn't
		// need to be flood-filled and added again).
		// If so, return true.
		// If not, return false.

	void AddNewRegion (Status_t &a_reStatus, MovedRegion &a_rRegion);
		// Add the given region, with the given motion-vector.
		// Causes a_rRegion to be emptied.

	void RemoveRegion (MovedRegion *a_pRegion);
		// Removes the given region from the search-border.

	MovedRegion *ChooseBestActiveRegion (Status_t &a_reStatus,
			FRAMESIZE &a_rtnSecondBestActiveRegionSize);
		// Remove the largest active-region from the search-border and
		// return it.
		// Backpatch the size of the second-largest active-region.

	void FinishFrame (Status_t &a_reStatus);
		// Clean up the search border at the end of a frame, e.g. hand
		// all remaining regions back to the client.

	virtual void OnCompletedRegion (Status_t &a_reStatus,
			MovedRegion *a_pRegion) = 0;
		// Hand a completed region to our client.  Subclasses must
		// override this to describe how to do this.

	void DeleteRegion (MovedRegion *a_pRegion);
		// Delete a region that was returned by ChooseBestActiveRegion()
		// or OnCompletedRegion().

	// A moved region of pixels that has been detected.
	// All extents are in the coordinate system of the new frame; that
	// makes it easy to unify/subtract regions without regard to their
	// motion vector.
	class MovedRegion : public Region_t
	{
	private:
		typedef Region_t BaseClass;
			// Keep track of who our base class is.

		void *operator new (size_t);
		void operator delete (void *) {}
		void *operator new[] (size_t);
		void operator delete[] (void *);
			// Disallow allocation from system memory.
			// (This helps enforce the use of DeleteRegion().)

	public:
		MovedRegion (typename BaseClass::Allocator &a_rAlloc
				= BaseClass::Extents::Imp::sm_oNodeAllocator);
			// Default constructor.  Must be followed by Init().

		MovedRegion (Status_t &a_reStatus, typename BaseClass::Allocator
				&a_rAlloc = BaseClass::Extents::Imp::sm_oNodeAllocator);
			// Initializing constructor.  Creates an empty region.

		MovedRegion (Status_t &a_reStatus, const MovedRegion &a_rOther);
			// Copy constructor.

		void Init (Status_t &a_reStatus);
			// Initializer.  Must be called on default-constructed
			// regions.

		void Assign (Status_t &a_reStatus, const MovedRegion &a_rOther);
			// Make the current region a copy of the other region.

		virtual ~MovedRegion();
			// Destructor.

		inline void Move (MovedRegion &a_rOther);
			// Move the contents of the other region into the current
			// region.
			// The current region must be empty.

		inline void SetMotionVector (PIXELINDEX a_tnX, PIXELINDEX a_tnY);
			// Set the motion vector.

		inline void GetMotionVector (PIXELINDEX &a_rtnX,
				PIXELINDEX &a_rtnY) const;
			// Get the motion vector.

		// Comparison class, suitable for Set<>.
		class SortBySizeThenMotionVectorLength
		{
		public:
			inline bool operator() (const MovedRegion *a_pLeft,
				const MovedRegion *a_pRight) const;
		};

		// Comparison class, suitable for Set<>.
		class SortBySizeThenMotionVectorLengthThenRegionAddress
		{
		public:
			inline bool operator() (const MovedRegion *a_pLeft,
				const MovedRegion *a_pRight) const;
		};

		inline FRAMESIZE GetSquaredMotionVectorLength (void) const;
			// Get the squared length of the motion vector.
			// Needed by SortBySizeThenMotionVectorLength().

		FRAMESIZE m_tnReferences;
			// The number of beginnings/endings of extents of this
			// region that are on the border.  When this goes to zero,
			// that means no other matches could possibly be added to
			// the region, and m_pRegion will get handed back to the
			// search-border's client.

		FRAMESIZE m_tnBorderExtents;
			// The number of region extents that intersect/border the
			// current pixel-group.

	private:
		PIXELINDEX m_tnMotionX, m_tnMotionY;
			// The motion vector associated with this region.

		FRAMESIZE m_tnSquaredLength;
			// The squared length of the motion vector.
			// Used for sorting.

#ifndef NDEBUG

	public:
		PIXELINDEX m_tnX, m_tnY;
			// The location of the pixel-group-match that originally
			// created this region.

		// The number of region objects in existence.
	private:
		static uint32_t sm_ulInstances;
	public:
		static uint32_t GetInstances (void)
			{ return sm_ulInstances; }

#endif // NDEBUG
	};

private:
	typename Region_t::Allocator &m_rSetRegionExtentAllocator;
		// Where we get memory to allocate set-region extents.

	PIXELINDEX m_tnWidth, m_tnHeight;
		// The dimension of each reference frame.

	PIXELINDEX m_tnSearchRadiusX, m_tnSearchRadiusY;
		// The search radius, i.e. how far from the current pixel
		// group we look when searching for possible moved instances of
		// the group.

	PIXELINDEX m_tnPGW, m_tnPGH;
		// The dimension of pixel groups.

	PIXELINDEX m_tnX, m_tnY;
		// The index of the current pixel group.  Actually the index
		// of the top-left pixel in the current pixel group.  This
		// gets moved in a zigzag pattern, back and forth across the
		// frame and then down, until the end of the frame is reached.

	PIXELINDEX m_tnStepX;
		// Whether we're zigging or zagging.

	FRAMESIZE m_tnMatchSizeThrottle;
		// The size (measured in number of pixels) that the largest
		// region in the area of the current pixel-group can be before
		// we avoid expensive processing of its extents.

	// A structure that keeps track of which active-region (if any) exists
	// for a given motion-vector.
	struct MotionVectorMatch
	{
	public:
		FRAMESIZE m_tnActiveExtents;
			// The number of active-region extents that intersect/border
			// the current pixel-group.

		MotionVectorMatch() : m_tnActiveExtents (0) {}
			// Default constructor.
	};

	MotionVectorMatch *m_pMotionVectorMatches;
		// An array of ((2 * m_tnSearchRadiusX + 1)
		// * (2 * m_tnSearchRadiusY + 1) cells that keep track of which
		// active-region, if any, exists for a given motion-vector.

	MotionVectorMatch &GetMotionVectorMatch (PIXELINDEX a_tnMotionX,
			PIXELINDEX a_tnMotionY) const;
		// Get the motion-vector-match cell for this motion-vector.

	void AddMotionVectorMatch (MovedRegion *a_pRegion);
		// Add the given region to the motion-vector-match array.

	void RemoveMotionVectorMatch (MovedRegion *a_pRegion);
		// Remove the given region from the motion-vector-match array.

	// A class that keeps track of region extents on the border between
	// the searched area and the not-yet-searched area, i.e. the only
	// regions that have a chance of growing.
	class BorderExtentBoundary
	{
	public:
		PIXELINDEX m_tnCounterpartIndex;
			// The pixel-index of the ending to go with this beginning, or
			// of the beginning to go with this ending.

		MovedRegion *m_pRegion;
			// The region with the given extent.

		BorderExtentBoundary();
			// Default constructor.

		BorderExtentBoundary (MovedRegion *a_pRegion);
			// Initializing constructor.

		~BorderExtentBoundary();
			// Destructor.

		#ifndef NDEBUG
		bool operator == (const BorderExtentBoundary &a_rOther) const;
		#endif // !NDEBUG
			// Equality operator.

		bool operator < (const BorderExtentBoundary &a_rOther) const
				{ return m_pRegion < a_rOther.m_pRegion; }
			// Less-than operator.
	};

	#ifdef BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
	typedef Set<BorderExtentBoundary, Less<BorderExtentBoundary>,
		Vector<BorderExtentBoundary,BorderExtentBoundary,
			Ident<BorderExtentBoundary,BorderExtentBoundary>,
			Less<BorderExtentBoundary> > >
		BorderExtentBoundarySet;
	#else // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
	typedef Set<BorderExtentBoundary, Less<BorderExtentBoundary>,
		SkipList<BorderExtentBoundary,BorderExtentBoundary,
			Ident<BorderExtentBoundary,BorderExtentBoundary>,
			Less<BorderExtentBoundary> > >
		BorderExtentBoundarySet;
	#endif // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
	typedef typename BorderExtentBoundarySet::Allocator BEBS_Allocator_t;
	BEBS_Allocator_t m_oBorderExtentsAllocator;
	BorderExtentBoundarySet *m_psetBorderStartpoints,
			*m_psetBorderEndpoints;
		// The borders, i.e. the startpoint/endpoints for every
		// region under construction, for every line in the current
		// pixel-group's vertical extent.

	typedef Set<MovedRegion *, typename MovedRegion
			::SortBySizeThenMotionVectorLengthThenRegionAddress>
		MovedRegionSet;
	typedef typename MovedRegionSet::Allocator MovedRegionSetAllocator_t;
	MovedRegionSetAllocator_t m_oMovedRegionSetAllocator;
	MovedRegionSet m_setRegions;
		// All moving areas detected so far.
		// Sorted by decreasing size, then increasing motion vector
		// length, i.e. the order in which they should be applied to
		// the reference-frame version of the new frame.

	typedef Allocator<1> MovedRegionAllocator_t;
	MovedRegionAllocator_t m_oMovedRegionAllocator;
		// Where we get memory to allocate MovedRegion instances.

#ifndef NDEBUG
public:
	static uint32_t GetMovedRegionCount (void)
		{ return MovedRegion::GetInstances(); }
#endif // !NDEBUG
};



// Default constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::SearchBorder
		(typename Region_t::Allocator &a_rAlloc)
	: m_rSetRegionExtentAllocator (a_rAlloc),
	m_oBorderExtentsAllocator (1048576),
	m_oMovedRegionSetAllocator (262144),
	m_setRegions (typename MovedRegion
		::SortBySizeThenMotionVectorLengthThenRegionAddress(),
		m_oMovedRegionSetAllocator),
	m_oMovedRegionAllocator (262144)
{
	// No frame dimensions yet.
	m_tnWidth = m_tnHeight = PIXELINDEX (0);

	// No search radius yet.
	m_tnSearchRadiusX = m_tnSearchRadiusY = PIXELINDEX (0);

	// No pixel-group width/height yet.
	m_tnPGW = m_tnPGH = PIXELINDEX (0);

	// No active search yet.
	m_tnX = m_tnY = m_tnStepX = PIXELINDEX (0);

	// No motion-vector matches yet.
	m_pMotionVectorMatches = NULL;

	// No border-startpoint/endpoint sets yet.
	m_psetBorderStartpoints = m_psetBorderEndpoints = NULL;
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::~SearchBorder()
{
	// Make sure our client didn't stop in the middle of a frame.
	FRAMESIZE tnPixels = FRAMESIZE (m_tnWidth) * FRAMESIZE (m_tnHeight);
	#ifndef NDEBUG
	if (m_psetBorderStartpoints != NULL)
	{
		for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
			assert (m_psetBorderStartpoints[tnI].Size() == 0);
	}
	if (m_psetBorderEndpoints != NULL)
	{
		for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
			assert (m_psetBorderEndpoints[tnI].Size() == 0);
	}
	#endif // !NDEBUG
	assert (m_setRegions.Size() == 0);

	// Free up our motion-vector matches.
	delete[] m_pMotionVectorMatches;

	// Free up our arrays of border-startpoint/endpoint sets.
	if (m_psetBorderStartpoints != NULL)
	{
		for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
			m_psetBorderStartpoints[tnI].~BorderExtentBoundarySet();
		free (m_psetBorderStartpoints);
	}
	if (m_psetBorderEndpoints != NULL)
	{
		for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
			m_psetBorderEndpoints[tnI].~BorderExtentBoundarySet();
		free (m_psetBorderEndpoints);
	}
}



// Initializer.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::Init (Status_t &a_reStatus,
	PIXELINDEX a_tnWidth, PIXELINDEX a_tnHeight,
	PIXELINDEX a_tnSearchRadiusX, PIXELINDEX a_tnSearchRadiusY,
	PIXELINDEX a_tnPGW, PIXELINDEX a_tnPGH,
	FRAMESIZE a_tnMatchSizeThrottle)
{
	FRAMESIZE tnMotionVectorMatches;
		// The number of motion-vector-match cells to allocate.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the width & height are reasonable.
	assert (a_tnWidth > PIXELINDEX (0));
	assert (a_tnHeight > PIXELINDEX (0));

	// Calculate the total number of pixels.
	FRAMESIZE tnPixels = FRAMESIZE (a_tnWidth) * FRAMESIZE (a_tnHeight);

	// Initialize the border-extent-boundary allocator.
	#ifdef BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
	m_oBorderExtentsAllocator.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		goto cleanup0;
	#endif // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR

	// Allocate space for the sets that implement our border-regions.
	// Since the initializing constructor needs to be called for each
	// contained item, we can't use array-new.
	m_psetBorderStartpoints = (BorderExtentBoundarySet *) malloc
		(sizeof (BorderExtentBoundarySet) * tnPixels);
	if (m_psetBorderStartpoints == NULL)
		goto cleanup0;
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		new ((void *)(m_psetBorderStartpoints + tnI))
			BorderExtentBoundarySet (Less<BorderExtentBoundary>(),
				m_oBorderExtentsAllocator);
	m_psetBorderEndpoints = (BorderExtentBoundarySet *) malloc
		(sizeof (BorderExtentBoundarySet) * tnPixels);
	if (m_psetBorderEndpoints == NULL)
		goto cleanup1;
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		new ((void *)(m_psetBorderEndpoints + tnI))
			BorderExtentBoundarySet (Less<BorderExtentBoundary>(),
				m_oBorderExtentsAllocator);

	// Initialize the sets that implement our border-regions.
	{
		#ifdef BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
		typename BorderExtentBoundarySet::InitParams
			oBorderSetInitParams (16, 16);
		#else // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
		typename BorderExtentBoundarySet::InitParams
			oBorderSetInitParams /* defaults are fine */;
		#endif // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
		for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		{
			m_psetBorderStartpoints[tnI].Init (a_reStatus, false,
				oBorderSetInitParams);
			if (a_reStatus != g_kNoError)
				goto cleanup2;
			m_psetBorderEndpoints[tnI].Init (a_reStatus, false,
				oBorderSetInitParams);
			if (a_reStatus != g_kNoError)
				goto cleanup2;
		}
	}
	m_setRegions.Init (a_reStatus, false);
	if (a_reStatus != g_kNoError)
		goto cleanup2;

	// Allocate space for motion-vector matches.
	tnMotionVectorMatches
		= (FRAMESIZE (2) * FRAMESIZE (a_tnSearchRadiusX) + FRAMESIZE (1))
		* (FRAMESIZE (2) * FRAMESIZE (a_tnSearchRadiusY) + FRAMESIZE (1));
	m_pMotionVectorMatches = new MotionVectorMatch[tnMotionVectorMatches];
	if (m_pMotionVectorMatches == NULL)
		goto cleanup2;

	// Finally, store our parameters.
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;
	m_tnSearchRadiusX = a_tnSearchRadiusX;
	m_tnSearchRadiusY = a_tnSearchRadiusY;
	m_tnPGW = a_tnPGW;
	m_tnPGH = a_tnPGH;
	m_tnMatchSizeThrottle = a_tnMatchSizeThrottle;

	// All done.
	return;

	// Clean up after errors.
//cleanup3:
//	delete[] m_pMotionVectorMatches;
cleanup2:
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		m_psetBorderEndpoints[tnI].~BorderExtentBoundarySet();
	free (m_psetBorderEndpoints);
	m_psetBorderEndpoints = NULL;
cleanup1:
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		m_psetBorderStartpoints[tnI].~BorderExtentBoundarySet();
	free (m_psetBorderStartpoints);
	m_psetBorderStartpoints = NULL;
cleanup0:
	;
}



// Default constructor.  Must be followed by Init().
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::MovedRegion
		(typename BaseClass::Allocator &a_rAlloc)
	: BaseClass (a_rAlloc)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // !NDEBUG

	// No motion-vector yet.
	m_tnMotionX = m_tnMotionY = PIXELINDEX (0);
	m_tnSquaredLength = FRAMESIZE (0);

	// No pixel-group-location yet.
	#ifndef NDEBUG
	m_tnX = m_tnY = PIXELINDEX (0);
	#endif // !NDEBUG

	// No references yet.
	m_tnReferences = 0;
	m_tnBorderExtents = 0;
}



// Initializing constructor.  Creates an empty region.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::MovedRegion
		(Status_t &a_reStatus, typename BaseClass::Allocator &a_rAlloc)
	: BaseClass (a_rAlloc)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // !NDEBUG

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// No motion-vector yet.
	m_tnMotionX = m_tnMotionY = PIXELINDEX (0);
	m_tnSquaredLength = FRAMESIZE (0);

	// No pixel-group-location yet.
	#ifndef NDEBUG
	m_tnX = m_tnY = PIXELINDEX (0);
	#endif // !NDEBUG

	// No references yet.
	m_tnReferences = 0;
	m_tnBorderExtents = 0;

	// Initialize ourselves.
	Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



// Copy constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::MovedRegion
		(Status_t &a_reStatus, const MovedRegion &a_rOther)
	: BaseClass (a_reStatus, a_rOther)
{
	// One more instance.
	#ifndef NDEBUG
	++sm_ulInstances;
	#endif // !NDEBUG

	// No motion-vector yet.
	m_tnMotionX = m_tnMotionY = PIXELINDEX (0);
	m_tnSquaredLength = FRAMESIZE (0);

	// No pixel-group-location yet.
	#ifndef NDEBUG
	m_tnX = m_tnY = PIXELINDEX (0);
	#endif // !NDEBUG

	// No references yet.
	m_tnReferences = 0;
	m_tnBorderExtents = 0;

	// If copying our base class failed, leave.
	if (a_reStatus != g_kNoError)
		return;

	// Copy the motion vector.
	m_tnMotionX = a_rOther.m_tnMotionX;
	m_tnMotionY = a_rOther.m_tnMotionY;
	m_tnSquaredLength = a_rOther.m_tnSquaredLength;

	// Copy the pixel-group-location.
	#ifndef NDEBUG
	m_tnX = a_rOther.m_tnX;
	m_tnY = a_rOther.m_tnY;
	#endif // !NDEBUG
}



// Initializer.  Must be called on default-constructed regions.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::Init
	(Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize the base-class.
	BaseClass::Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



// Make the current region a copy of the other region.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::Assign
	(Status_t &a_reStatus, const MovedRegion &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Assign the base class.
	BaseClass::Assign (a_reStatus, a_rOther);
	if (a_reStatus != g_kNoError)
		return;

	// Copy the motion vector.
	m_tnMotionX = a_rOther.m_tnMotionX;
	m_tnMotionY = a_rOther.m_tnMotionY;
	m_tnSquaredLength = a_rOther.m_tnSquaredLength;

	// Copy the pixel-group-location.
	#ifndef NDEBUG
	m_tnX = a_rOther.m_tnX;
	m_tnY = a_rOther.m_tnY;
	#endif // !NDEBUG
}



// Move the contents of the other region into the current region.
// The current region must be empty.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::Move
	(MovedRegion &a_rOther)
{
	// Make sure neither region is already in the search-border, i.e.
	// that neither has any references.
	assert (m_tnReferences == 0);
	assert (m_tnBorderExtents == 0);
	assert (a_rOther.m_tnReferences == 0);
	assert (a_rOther.m_tnBorderExtents == 0);

	// Move the base class.
	BaseClass::Move (a_rOther);

	// Copy the motion vector.
	m_tnMotionX = a_rOther.m_tnMotionX;
	m_tnMotionY = a_rOther.m_tnMotionY;
	m_tnSquaredLength = a_rOther.m_tnSquaredLength;

	// Copy the pixel-group-location.
	#ifndef NDEBUG
	m_tnX = a_rOther.m_tnX;
	m_tnY = a_rOther.m_tnY;
	#endif // !NDEBUG
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::~MovedRegion()
{
	// Make sure there are no references left.
	assert (m_tnReferences == 0);
	assert (m_tnBorderExtents == 0);

	// One less instance.
	#ifndef NDEBUG
	--sm_ulInstances;
	#endif // !NDEBUG

	// Nothing additional to do.
}



// Set the motion vector.
template <class PIXELINDEX, class FRAMESIZE>
inline void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::SetMotionVector
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// Set the motion vector.
	m_tnMotionX = a_tnX;
	m_tnMotionY = a_tnY;

	// Calculate the square of the vector's length.  (It's used for
	// sorting, and we don't want to recalculate it on every comparison.)
	m_tnSquaredLength = FRAMESIZE (m_tnMotionX) * FRAMESIZE (m_tnMotionX)
		+ FRAMESIZE (m_tnMotionY) * FRAMESIZE (m_tnMotionY);
}



// Get the motion vector.
template <class PIXELINDEX, class FRAMESIZE>
inline void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::GetMotionVector
	(PIXELINDEX &a_rtnX, PIXELINDEX &a_rtnY) const
{
	// Easy enough.
	a_rtnX = m_tnMotionX;
	a_rtnY = m_tnMotionY;
}



// Comparison operator.
template <class PIXELINDEX, class FRAMESIZE>
inline bool
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion
	::SortBySizeThenMotionVectorLength::operator()
	(const MovedRegion *a_pLeft, const MovedRegion *a_pRight) const
{
	FRAMESIZE nLeftPoints, nRightPoints;
		// The number of points in each region.
	FRAMESIZE tnLeftLen, tnRightLen;
		// The (squared) length of each motion vector.

	// Make sure they gave us some regions to compare.
	assert (a_pLeft != NULL);
	assert (a_pRight != NULL);

	// First, compare by the number of points in each region.
	// Sort bigger regions first.
	nLeftPoints = a_pLeft->NumberOfPoints();
	nRightPoints = a_pRight->NumberOfPoints();
	if (nLeftPoints > nRightPoints)
		return true;
	if (nLeftPoints < nRightPoints)
		return false;

	// Then compare on motion vector length.
	// Sort smaller vectors first.
	tnLeftLen = a_pLeft->GetSquaredMotionVectorLength();
	tnRightLen = a_pRight->GetSquaredMotionVectorLength();
	if (tnLeftLen < tnRightLen)
		return true;
	// if (tnLeftLen >= tnRightLen)
		return false;
}



// Comparison operator.
template <class PIXELINDEX, class FRAMESIZE>
inline bool
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion
	::SortBySizeThenMotionVectorLengthThenRegionAddress::operator()
	(const MovedRegion *a_pLeft, const MovedRegion *a_pRight) const
{
	FRAMESIZE nLeftPoints, nRightPoints;
		// The number of points in each region.
	FRAMESIZE tnLeftLen, tnRightLen;
		// The (squared) length of each motion vector.

	// Make sure they gave us some regions to compare.
	assert (a_pLeft != NULL);
	assert (a_pRight != NULL);

	// First, compare by the number of points in each region.
	// Sort bigger regions first.
	nLeftPoints = a_pLeft->NumberOfPoints();
	nRightPoints = a_pRight->NumberOfPoints();
	if (nLeftPoints > nRightPoints)
		return true;
	if (nLeftPoints < nRightPoints)
		return false;

	// Then compare on motion vector length.
	// Sort smaller vectors first.
	tnLeftLen = a_pLeft->GetSquaredMotionVectorLength();
	tnRightLen = a_pRight->GetSquaredMotionVectorLength();
	if (tnLeftLen < tnRightLen)
		return true;
	if (tnLeftLen > tnRightLen)
		return false;

	// Finally, disambiguate by region address.
	if (a_pLeft < a_pRight)
		return true;
	//if (a_pLeft >= a_pRight)
		return false;
}



// Get the squared length of the motion vector.
template <class PIXELINDEX, class FRAMESIZE>
inline FRAMESIZE
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion
	::GetSquaredMotionVectorLength (void) const
{
	// Easy enough.
	return m_tnSquaredLength;
}



#ifndef NDEBUG

// The number of region objects in existence.
template <class PIXELINDEX, class FRAMESIZE>
uint32_t
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::sm_ulInstances;

#endif // !NDEBUG



// Default constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::BorderExtentBoundary()
{
	// Fill in the blanks.
	m_tnCounterpartIndex = PIXELINDEX (0);
	m_pRegion = NULL;
}



// Initializing constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::BorderExtentBoundary (MovedRegion *a_pRegion)
{
	// Make sure they gave us a region.
	assert (a_pRegion != NULL);

	// Fill in the blanks.
	m_tnCounterpartIndex = PIXELINDEX (0);
	m_pRegion = a_pRegion;
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::~BorderExtentBoundary()
{
	// Nothing to do.
}



#ifndef NDEBUG

// Equality operator.
template <class PIXELINDEX, class FRAMESIZE>
bool
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::operator == (const BorderExtentBoundary &a_rOther) const
{
	// Compare ourselves, field by field.
	return (m_tnCounterpartIndex == a_rOther.m_tnCounterpartIndex
		&& m_pRegion == a_rOther.m_pRegion);
}

#endif // !NDEBUG



// Initialize the search-border, i.e. start in the upper-left corner.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::StartFrame (Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the borders are empty.
	#ifndef NDEBUG
	FRAMESIZE tnPixels = FRAMESIZE (m_tnWidth) * FRAMESIZE (m_tnHeight);
	assert (m_psetBorderStartpoints != NULL);
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		assert (m_psetBorderStartpoints[tnI].Size() == 0);
	assert (m_psetBorderEndpoints != NULL);
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		assert (m_psetBorderEndpoints[tnI].Size() == 0);
	#endif // !NDEBUG
	assert (m_setRegions.Size() == 0);

	// Make sure there are no leftover moved-regions from last time.
	assert (m_oMovedRegionAllocator.GetNumAllocated() == 0);
	assert (m_rSetRegionExtentAllocator.GetNumAllocated() == 0);

	// Make sure there are no active-extents for any motion-vectors.
	#ifndef NDEBUG
	{
		FRAMESIZE tnMotionVectorMatches
			= (FRAMESIZE (2) * FRAMESIZE (m_tnSearchRadiusX)
				+ FRAMESIZE (1))
			* (FRAMESIZE (2) * FRAMESIZE (m_tnSearchRadiusY)
			* 	+ FRAMESIZE (1));
		for (FRAMESIZE i = 0; i < tnMotionVectorMatches; ++i)
			assert (m_pMotionVectorMatches[i].m_tnActiveExtents == 0);
	}
	#endif // !NDEBUG

	// Start in the upper-left corner, and prepare to move to the right.
	m_tnX = m_tnY = PIXELINDEX (0);
	m_tnStepX = PIXELINDEX (1);
}



// Get the motion-vector-match cell for this motion-vector.
template <class PIXELINDEX, class FRAMESIZE>
typename SearchBorder<PIXELINDEX,FRAMESIZE>::MotionVectorMatch &
SearchBorder<PIXELINDEX,FRAMESIZE>::GetMotionVectorMatch
	(PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY) const
{
	// Make sure the motion-vector is in range.
	assert (a_tnMotionX >= -m_tnSearchRadiusX
		&& a_tnMotionX <= m_tnSearchRadiusX);
	assert (a_tnMotionY >= -m_tnSearchRadiusY
		&& a_tnMotionY <= m_tnSearchRadiusY);

	// Find the corresponding motion-vector-match cell.
	FRAMESIZE tnMajorAxis = FRAMESIZE (2) * FRAMESIZE (m_tnSearchRadiusX)
		+ FRAMESIZE (1);
	FRAMESIZE tnXOffset = FRAMESIZE (a_tnMotionX)
		+ FRAMESIZE (m_tnSearchRadiusX);
	FRAMESIZE tnYOffset = FRAMESIZE (a_tnMotionY)
		+ FRAMESIZE (m_tnSearchRadiusY);
	FRAMESIZE tnIndex = tnXOffset + tnMajorAxis * tnYOffset;

	// Return it to our caller.
	return m_pMotionVectorMatches[tnIndex];
}



// Add the given region to the motion-vector-match array.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::AddMotionVectorMatch
	(MovedRegion *a_pRegion)
{
	// Make sure they gave us a region.
	assert (a_pRegion != NULL);

	// Get the region's motion vector.
	PIXELINDEX tnMotionX, tnMotionY;
	a_pRegion->GetMotionVector (tnMotionX, tnMotionY);

	// Find the corresponding motion-vector-match cell.
	MotionVectorMatch &rMatch
		= GetMotionVectorMatch (tnMotionX, tnMotionY);

	// That's one more active extent.
	++rMatch.m_tnActiveExtents;
	++a_pRegion->m_tnBorderExtents;
}



// Remove the given region from the motion-vector-match array.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::RemoveMotionVectorMatch
	(MovedRegion *a_pRegion)
{
	// Make sure they gave us a region.
	assert (a_pRegion != NULL);

	// Get the region's motion vector.
	PIXELINDEX tnMotionX, tnMotionY;
	a_pRegion->GetMotionVector (tnMotionX, tnMotionY);

	// Find the corresponding motion-vector-match cell.
	MotionVectorMatch &rMatch
		= GetMotionVectorMatch (tnMotionX, tnMotionY);

	// That's one less active extent.
	assert (rMatch.m_tnActiveExtents > 0);
	--rMatch.m_tnActiveExtents;
	assert (a_pRegion->m_tnBorderExtents > 0);
	--a_pRegion->m_tnBorderExtents;
}



// Move one pixel to the right, adding and removing regions from
// the potentially-intersecting list.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MoveRight (Status_t &a_reStatus)
{
	PIXELINDEX tnI;
		// Used to loop through iterators.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we knew we were moving right.
	assert (m_tnStepX == 1);

	// Make sure our client didn't leave behind any regions whose size
	// exceeds the match-size-throttle.
	#ifndef NDEBUG
	{
		typename MovedRegionSet::ConstIterator itMatchThrottle
			= m_setRegions.Begin();
		assert (itMatchThrottle == m_setRegions.End()
			|| (*itMatchThrottle)->NumberOfPoints()
				< m_tnMatchSizeThrottle);
	}
	#endif // !NDEBUG

	// All active regions with a current-border endpoint at old X, and
	// all active regions with a first/last-border endpoint at old X + 1,
	// are no longer active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH + 1; ++tnI)
	{
		// Determine if we're on the first/last-border.
		// We'll need to adjust a few things by 1 if so.
		PIXELINDEX tnIfFirstLast
			= (tnI == 0 || tnI == m_tnPGH + PIXELINDEX (1)) ? 1 : 0;

		// Make sure there could be endpoints here.
		// (The endpoint-X is adjusted downward by 1, because the valid
		// range of endpoint-X values is from 1 to m_tnWidth.)
		PIXELINDEX tnY = m_tnY + tnI - PIXELINDEX (1);
		PIXELINDEX tnX = m_tnX + tnIfFirstLast - PIXELINDEX (1);
		assert (tnY >= 0);
		if (/* tnY >= 0 && */ tnY < m_tnHeight
			&& tnX >= 0 && tnX < m_tnWidth)
		{
			// Get the set of endpoints of interest.
			const BorderExtentBoundarySet &rEndpointSet
				= m_psetBorderEndpoints[tnY * m_tnWidth + tnX];

			// Remove all active regions that have endpoints at this index.
			typename BorderExtentBoundarySet::ConstIterator itEndpoint;
			for (itEndpoint = rEndpointSet.Begin();
				 itEndpoint != rEndpointSet.End();
				 ++itEndpoint)
			{
				RemoveMotionVectorMatch ((*itEndpoint).m_pRegion);
			}
		}
	}

	// All active regions with a current-border startpoint at
	// new X + m_tnPGW, and all active regions with a first/last-border
	// startpoint at new X + m_tnPGW - 1, are now active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH + 1; ++tnI)
	{
		// Determine if we're on the first/last-border.
		// We'll need to adjust a few things by 1 if not.
		PIXELINDEX tnIfNotFirstLast
			= (tnI == 0 || tnI == m_tnPGH + PIXELINDEX (1)) ? 0 : 1;

		// Make sure there could be startpoints here.
		PIXELINDEX tnY = m_tnY + tnI - PIXELINDEX (1);
		PIXELINDEX tnX = m_tnX + m_tnPGW + tnIfNotFirstLast;
		assert (tnY >= 0 && tnX >= 0);
		if (/* tnY >= 0 && */ tnY < m_tnHeight
			/* && tnX >= 0 */ && tnX < m_tnWidth)
		{
			// Get the set of startpoints of interest.
			const BorderExtentBoundarySet &rStartpointSet
				= m_psetBorderStartpoints[tnY * m_tnWidth + tnX];

			// Add all active regions that have startpoints at this index.
			typename BorderExtentBoundarySet::ConstIterator itStartpoint;
			for (itStartpoint = rStartpointSet.Begin();
				 itStartpoint != rStartpointSet.End();
				 ++itStartpoint)
			{
				AddMotionVectorMatch ((*itStartpoint).m_pRegion);
			}
		}
	}

	// Finally, move one step to the right.
	++m_tnX;
}



// Move one pixel to the left, adding and removing regions from
// the potentially-intersecting list.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MoveLeft (Status_t &a_reStatus)
{
	int tnI;
		// Used to loop through iterators.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we knew we were moving left.
	assert (m_tnStepX == -1);

	// Make sure our client didn't leave behind any regions whose size
	// exceeds the match-size-throttle.
	#ifndef NDEBUG
	{
		typename MovedRegionSet::ConstIterator itMatchThrottle
			= m_setRegions.Begin();
		assert (itMatchThrottle == m_setRegions.End()
			|| (*itMatchThrottle)->NumberOfPoints()
				< m_tnMatchSizeThrottle);
	}
	#endif // !NDEBUG

	// All active regions with a current-border startpoint at
	// old X + m_tnPGW, and all active regions with a last-border
	// startpoint at old X + m_tnPGW - 1, are no longer active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH + 1; ++tnI)
	{
		// Determine if we're on the first/last-border.
		// We'll need to adjust a few things by 1 if so.
		PIXELINDEX tnIfFirstLast
			= (tnI == 0 || tnI == m_tnPGH + PIXELINDEX (1)) ? 1 : 0;

		// Make sure there could be startpoints here.
		PIXELINDEX tnY = m_tnY + tnI - PIXELINDEX (1);
		PIXELINDEX tnX = m_tnX + m_tnPGW - tnIfFirstLast;
		assert (tnY >= 0 && tnX >= 0);
		if (/* tnY >= 0 && */ tnY < m_tnHeight
			/* && tnX >= 0 */ && tnX < m_tnWidth)
		{
			// Get the set of startpoints of interest.
			const BorderExtentBoundarySet &rStartpointSet
				= m_psetBorderStartpoints[tnY * m_tnWidth + tnX];

			// Remove all active regions that have startpoints at this
			// index.
			typename BorderExtentBoundarySet::ConstIterator itStartpoint;
			for (itStartpoint = rStartpointSet.Begin();
				 itStartpoint != rStartpointSet.End();
				 ++itStartpoint)
			{
				RemoveMotionVectorMatch ((*itStartpoint).m_pRegion);
			}
		}
	}

	// All active regions with a current-border endpoint at new X, and
	// all active regions with a last-border endpoint at new X + 1, are
	// now active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH + 1; ++tnI)
	{
		// Determine if we're on the first/last-border.
		// We'll need to adjust a few things by 1 if not.
		PIXELINDEX tnIfNotFirstLast
			= (tnI == 0 || tnI == m_tnPGH + PIXELINDEX (1)) ? 0 : 1;

		// Make sure there could be endpoints here.
		// (The endpoint-X is adjusted downward by 1, because the valid
		// range of endpoint-X values is from 1 to m_tnWidth.)
		PIXELINDEX tnY = m_tnY + tnI - PIXELINDEX (1);
		PIXELINDEX tnX = m_tnX - tnIfNotFirstLast - PIXELINDEX (1);
		assert (tnY >= 0);
		if (/* tnY >= 0 && */ tnY < m_tnHeight
			&& tnX >= 0 && tnX < m_tnWidth)
		{
			// Get the set of endpoints of interest.
			const BorderExtentBoundarySet &rEndpointSet
				= m_psetBorderEndpoints[tnY * m_tnWidth + tnX];

			// Add all active regions that have endpoints at this index.
			typename BorderExtentBoundarySet::ConstIterator itEndpoint;
			for (itEndpoint = rEndpointSet.Begin();
				 itEndpoint != rEndpointSet.End();
				 ++itEndpoint)
			{
				AddMotionVectorMatch ((*itEndpoint).m_pRegion);
			}
		}
	}

	// Finally, move one step to the left.
	--m_tnX;
}



// Move down a line, finding all regions that can no longer be
// contiguous with new matches, and handing them back to the client.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MoveDown (Status_t &a_reStatus)
{
	FRAMESIZE tnI;
	typename BorderExtentBoundarySet::Iterator itBorder;
		// Used to run through the last-border.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure our client didn't leave behind any regions whose size
	// exceeds the match-size-throttle.
	#ifndef NDEBUG
	{
		typename MovedRegionSet::ConstIterator itMatchThrottle
			= m_setRegions.Begin();
		assert (itMatchThrottle == m_setRegions.End()
			|| (*itMatchThrottle)->NumberOfPoints()
				< m_tnMatchSizeThrottle);
	}
	#endif // !NDEBUG

	// Run through the last border, disconnect the regions from all
	// endpoints.  If that leaves a region with no references and no
	// siblings, then the region is fully constructed, and it gets
	// handed back to the client.
	if (m_tnY > 0)
	{
		// Run through the startpoints and endpoints separately.  That
		// leaves some dangling references temporarily, but this avoids
		// having to do a set-find for the endpoints.
		for (tnI = 0; tnI < m_tnWidth; ++tnI)
		{
			BorderExtentBoundarySet &rStartpointSet
				= m_psetBorderStartpoints[(m_tnY - PIXELINDEX (1))
					* m_tnWidth + tnI];
			for (itBorder = rStartpointSet.Begin();
				 itBorder != rStartpointSet.End();
				 ++itBorder)
			{
				// Get the startpoint, and its under-construction region.
				const BorderExtentBoundary &rStartpoint = *itBorder;
				MovedRegion *pRegion = rStartpoint.m_pRegion;

				// If this extent intersects/borders the current
				// pixel-group, then a reference to it must be removed
				// from the motion-vector matches.
				PIXELINDEX tnXStart = tnI;
				PIXELINDEX tnXEnd = rStartpoint.m_tnCounterpartIndex;
				PIXELINDEX tnIfFirstLast
					= (m_tnY <= m_tnHeight - m_tnPGH) ? 1 : 0;
				if (tnXEnd >= m_tnX + tnIfFirstLast
					&& tnXStart < m_tnX + m_tnPGW + PIXELINDEX (1)
						- tnIfFirstLast)
				{
					// Remove a reference to this region.
					RemoveMotionVectorMatch (pRegion);
				}

				// That's one less reference to this region.
				assert (pRegion->m_tnReferences > 0);
				--pRegion->m_tnReferences;

				// Make sure there are references left (from the endpoint).
				assert (pRegion->m_tnReferences > 0);
			}
			rStartpointSet.Clear();
		}

		// (Now do the same for the endpoints, ignoring the dangling
		// references.)
		for (tnI = 0; tnI < m_tnWidth; ++tnI)
		{
			BorderExtentBoundarySet &rEndpointSet
				= m_psetBorderEndpoints[(m_tnY - PIXELINDEX (1))
					* m_tnWidth + tnI];

			for (itBorder = rEndpointSet.Begin();
				 itBorder != rEndpointSet.End();
				 ++itBorder)
			{
				// Get the under-construction region.
				MovedRegion *pRegion = (*itBorder).m_pRegion;

				// That's one less reference to this region.
				assert (pRegion->m_tnReferences > 0);
				--pRegion->m_tnReferences;

				// Are there any references left to this region?
				if (pRegion->m_tnReferences == 0)
				{
					// No.  The region is fully constructed.  Make sure
					// there are no border-extent references left.
					assert (pRegion->m_tnBorderExtents == 0);

					// Remove it from our active-regions set.
					typename MovedRegionSet::Iterator itRegion
						= m_setRegions.Find (pRegion);
					assert (itRegion != m_setRegions.End());
					assert ((*itRegion) == pRegion);
					m_setRegions.Erase (itRegion);

					// Move it to the list of regions that'll get applied
					// to the new frame's reference-image representation.
					OnCompletedRegion (a_reStatus, pRegion);
					if (a_reStatus != g_kNoError)
					{
						DeleteRegion (pRegion);
						return;
					}
				}
			}
			rEndpointSet.Clear();
		}
	}

	// Any region with (if m_tnX == 0) a last-border startpoint at
	// m_tnPGW or (if m_tnX == m_tnWidth - m_tnPGW) a last-border
	// endpoint of m_tnX will no longer be contiguous with the current
	// pixel-group, but they will be the next time the search border
	// moves left/right.
	//
	// Similarly, any region with (if m_tnX == 0) a bottommost-current-
	// border startpoint at m_tnPGW or (if m_tnX == m_tnWidth - m_tnPGW) a
	// bottommost-current-border endpoint of m_tnX will now be contiguous
	// with the current pixel-group.
	//
	// But not if we're past the bottom line.  This happens when we're
	// called by FinishFrame().
	if (m_tnY < m_tnHeight - m_tnPGH)
	{
		if (m_tnX == 0)
		{
			// Fix the new last-border.
			{
				// Get the set of startpoints of interest.
				const BorderExtentBoundarySet &rStartpointSet
					= m_psetBorderStartpoints[m_tnY * m_tnWidth + m_tnPGW];

				// Remove all active regions that have startpoints at this
				// index.
				typename BorderExtentBoundarySet::ConstIterator
					itStartpoint;
				for (itStartpoint = rStartpointSet.Begin();
					 itStartpoint != rStartpointSet.End();
					 ++itStartpoint)
				{
					RemoveMotionVectorMatch ((*itStartpoint).m_pRegion);
				}
			}

			// Fix the new bottommost-current-border.
			{
				PIXELINDEX tnY = m_tnY + m_tnPGH;
				PIXELINDEX tnX = m_tnPGW;
				assert (tnY >= 0 && tnX >= 0 && tnX < m_tnWidth);
				if (/* tnY >= 0 && */ tnY < m_tnHeight
					/* && tnX >= 0 && tnX < m_tnWidth */)
				{
					// Get the set of startpoints of interest.
					const BorderExtentBoundarySet &rStartpointSet
						= m_psetBorderStartpoints[tnY * m_tnWidth + tnX];

					// Add all active regions that have startpoints at this
					// index.
					typename BorderExtentBoundarySet::ConstIterator
						itStartpoint;
					for (itStartpoint = rStartpointSet.Begin();
						 itStartpoint != rStartpointSet.End();
						 ++itStartpoint)
					{
						AddMotionVectorMatch ((*itStartpoint).m_pRegion);
					}
				}
			}

			// Find all the extents of regions on the first-border
			// that will intersect/border the new current-pixel-group,
			// and put them into the border-regions set.
			//
			// All extents on the same line as the first-border,
			// whose startpoint X is less than the pixel-group width,
			// are extents that intersect/border the new
			// current-pixel-group.
			for (tnI = 0; tnI < m_tnPGW; ++tnI)
			{
				PIXELINDEX tnY = m_tnY + m_tnPGH + PIXELINDEX (1);
				PIXELINDEX tnX = tnI;
				assert (tnY >= 0 && tnX >= 0 && tnX < m_tnWidth);
				if (/* tnY >= 0 && */ tnY < m_tnHeight
					/* && tnX >= 0 && tnX < m_tnWidth */)
				{
					// Get the set of startpoints of interest.
					const BorderExtentBoundarySet &rStartpointSet
						= m_psetBorderStartpoints[tnY * m_tnWidth + tnX];

					// Loop through all the relevant startpoints, add them
					// to the motion-vector matches.
					typename BorderExtentBoundarySet::ConstIterator
						itStartpoint;
					for (itStartpoint = rStartpointSet.Begin();
						 itStartpoint != rStartpointSet.End();
						 ++itStartpoint)
					{
						AddMotionVectorMatch ((*itStartpoint).m_pRegion);
					}
				}
			}
		}
		else
		{
			assert (m_tnX == m_tnWidth - m_tnPGW);

			// Fix the new last-border.
			{
				// Get the set of endpoints of interest.
				// (The endpoint-X is adjusted downward by 1, because the
				// valid range of endpoint-X values is from 1 to
				// m_tnWidth.)
				const BorderExtentBoundarySet &rEndpointSet
					= m_psetBorderEndpoints[m_tnY * m_tnWidth
						+ (m_tnX - PIXELINDEX (1))];

				// Remove all active regions that have endpoints at this
				// index.
				typename BorderExtentBoundarySet::ConstIterator
					itEndpoint;
				for (itEndpoint = rEndpointSet.Begin();
					 itEndpoint != rEndpointSet.End();
					 ++itEndpoint)
				{
					RemoveMotionVectorMatch ((*itEndpoint).m_pRegion);
				}
			}

			// Fix the new bottommost-current-border.
			{
				// (The endpoint-X is adjusted downward by 1, because the
				// valid range of endpoint-X values is from 1 to
				// m_tnWidth.)
				PIXELINDEX tnY = m_tnY + m_tnPGH;
				PIXELINDEX tnX = m_tnX - PIXELINDEX (1);
				assert (tnY >= 0 && tnX < m_tnWidth);
				if (/* tnY >= 0 && */ tnY < m_tnHeight
					&& tnX >= 0 /* && tnX < m_tnWidth */)
				{
					// Get the set of endpoints of interest.
					const BorderExtentBoundarySet &rEndpointSet
						= m_psetBorderEndpoints[tnY * m_tnWidth + tnX];

					// Remove all active regions that have endpoints at
					// this index.
					typename BorderExtentBoundarySet::ConstIterator
						itEndpoint;
					for (itEndpoint = rEndpointSet.Begin();
						 itEndpoint != rEndpointSet.End();
						 ++itEndpoint)
					{
						AddMotionVectorMatch ((*itEndpoint).m_pRegion);
					}
				}
			}

			// Find all the extents of regions on the first-border
			// that will intersect/border the new current-pixel-group,
			// and put them into the border-regions set.
			if (m_tnY + m_tnPGH + PIXELINDEX (1) < m_tnHeight)
			{
				// All extents on the same line as the first-border,
				// whose endpoint X is greater than (framewidth - PGW),
				// are extents that intersect/border the new
				// current-pixel-group.
				// (The endpoint-X is adjusted downward by 1, because the
				// valid range of endpoint-X values is from 1 to
				// m_tnWidth.  That cancels out the upward adjustment.)
				for (tnI = m_tnX /* + PIXELINDEX (1) - PIXELINDEX (1) */;
					 tnI < m_tnWidth;
					 ++tnI)
				{
					// Get the set of endpoints of interest.
					assert (tnI >= 0 && tnI < m_tnWidth);
					const BorderExtentBoundarySet &rEndpointSet
						= m_psetBorderEndpoints[(m_tnY + m_tnPGH
							+ PIXELINDEX (1)) * m_tnWidth + tnI];

					// Loop through all the relevant endpoints, add their
					// corresponding startpoints to the border-regions set.
					typename BorderExtentBoundarySet::ConstIterator
						itEndpoint;
					for (itEndpoint = rEndpointSet.Begin();
						 itEndpoint != rEndpointSet.End();
						 ++itEndpoint)
					{
						AddMotionVectorMatch ((*itEndpoint).m_pRegion);
					}
				}
			}
		}

		// Finally, move one step down, and prepare to move the other
		// direction across the window.
		++m_tnY;
		m_tnStepX = -m_tnStepX;
	}
}



// Return the size of the largest active-region.
// Returns zero if there are no active-regions.
template <class PIXELINDEX, class FRAMESIZE>
FRAMESIZE
SearchBorder<PIXELINDEX,FRAMESIZE>::GetSizeOfLargestActiveRegion
	(void) const
{
	// Get the largest active-region, if any.
	typename MovedRegionSet::ConstIterator itFirst = m_setRegions.Begin();

	// Return its size, if any.
	return (itFirst == m_setRegions.End()) ? FRAMESIZE (0)
		: (*itFirst)->NumberOfPoints();
}



// Determine if a region with this motion vector already
// intersects/borders the current pixel-group (i.e. it doesn't
// need to be flood-filled and added again).
// If so, return true.
// If not, return false.
template <class PIXELINDEX, class FRAMESIZE>
bool
SearchBorder<PIXELINDEX,FRAMESIZE>::HasExistingMatch
	(PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY) const
{
	// Make sure the motion-vector is in range.
	assert (a_tnMotionX >= -m_tnSearchRadiusX
		&& a_tnMotionX <= m_tnSearchRadiusX);
	assert (a_tnMotionY >= -m_tnSearchRadiusY
		&& a_tnMotionY <= m_tnSearchRadiusY);

	// Find the corresponding motion-vector-match cell.
	MotionVectorMatch &rMatch
		= GetMotionVectorMatch (a_tnMotionX, a_tnMotionY);

	// Return whether there are any regions associated with this
	// motion vector.
	return (rMatch.m_tnActiveExtents > 0) ? true : false;
}



// Add the given region, with the given motion-vector.
// Causes a_rRegion to be emptied.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::AddNewRegion (Status_t &a_reStatus,
	MovedRegion &a_rRegion)
{
	MovedRegion *pRegion;
		// Our copy of the newly added region.
	typename BorderExtentBoundarySet::Iterator itStart, itEnd;
		// Any startpoint/endpoint inserted into the set, as they're
		// inserted.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make space for a new region.
	pRegion = (MovedRegion *) m_oMovedRegionAllocator.Allocate (0,
		sizeof (MovedRegion));
	if (pRegion == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		goto cleanup0;
	}

	// Create a new region.
	::new ((void *) pRegion) MovedRegion (a_reStatus,
		m_rSetRegionExtentAllocator);
	if (a_reStatus != g_kNoError)
		goto cleanup1;

	// Move the contents of the given region into the new region.
	// This also copies the motion vector and empties the given region.
	pRegion->Move (a_rRegion);

	// If the region's size exceeds the match-size-throttle, don't bother
	// adding its extents to the startpoint/endpoint sets.  We expect our
	// client to deal with these regions before moving to the next
	// pixel-group.
	if (pRegion->NumberOfPoints() < m_tnMatchSizeThrottle)
	{
		typename MovedRegion::ConstIterator itHere;
			// Used to loop through the newly added region.
		BorderExtentBoundary oStartpoint, oEndpoint;
			// New startpoints/endpoints as we create them.

		// Set up the unchanging parts of the startpoint and endpoint.
		oStartpoint.m_pRegion = oEndpoint.m_pRegion = pRegion;

		// Loop through every extent of the new region, create a startpoint
		// and endpoint for each one.  If the extent intersects or borders
		// the current pixel-group, make an entry for it in the
		// active-regions list.
		// Extents before the first-border can be ignored -- the
		// search-border traversal will never find them.
		for (itHere = (m_tnY == PIXELINDEX (0)) ? pRegion->Begin()
				: pRegion->LowerBound (m_tnY - PIXELINDEX (1),
					PIXELINDEX (0));
			 itHere != pRegion->End();
			 ++itHere)
		{
			// Get the current extent.
			const typename MovedRegion::Extent &rExtent = *itHere;
			PIXELINDEX tnY = rExtent.m_tnY;
			PIXELINDEX tnXStart = rExtent.m_tnXStart;
			PIXELINDEX tnXEnd = rExtent.m_tnXEnd;

			// Make sure this extent is inside our frame.  (Sanity check.)
			assert (tnY >= 0 && tnY < m_tnHeight);
			assert (tnXStart >= 0);
			assert (tnXStart < tnXEnd);
			assert (tnXEnd <= m_tnWidth);

			// Make sure it's not before the first-border.  (Sanity check.)
			assert (tnY + PIXELINDEX (1) >= m_tnY);

			// Create the startpoint and endpoint that represent this
			// extent.
			oStartpoint.m_tnCounterpartIndex = tnXEnd;
			oEndpoint.m_tnCounterpartIndex = tnXStart;

			// Make a record of this startpoint.
			BorderExtentBoundarySet &rStartpointSet
				= m_psetBorderStartpoints[tnY * m_tnWidth + tnXStart];
			typename BorderExtentBoundarySet::InsertResult
				oStartInsertResult = rStartpointSet.Insert
					(a_reStatus, oStartpoint);
			if (a_reStatus != g_kNoError)
				goto cleanup2;
			assert (oStartInsertResult.m_bInserted);

			// Remember where this startpoint was inserted.
			itStart = oStartInsertResult.m_itPosition;

			// The new startpoint contains another reference to the region.
			++pRegion->m_tnReferences;

			// Make a record of this endpoint.
			// (The endpoint-X is adjusted downward by 1, because the
			// valid range of endpoint-X values is from 1 to m_tnWidth.)
			BorderExtentBoundarySet &rEndpointSet
				= m_psetBorderEndpoints[tnY * m_tnWidth
					+ (tnXEnd - PIXELINDEX (1))];
			typename BorderExtentBoundarySet::InsertResult oEndInsertResult
				= rEndpointSet.Insert (a_reStatus, oEndpoint);
			if (a_reStatus != g_kNoError)
			{
				rStartpointSet.Erase (itStart);
				goto cleanup3;
			}
			assert (oEndInsertResult.m_bInserted);

			// Remember where this endpoint was inserted.
			itEnd = oEndInsertResult.m_itPosition;

			// The new endpoint contains another reference to the region.
			++pRegion->m_tnReferences;

			// If this extent intersects/borders the current pixel-group,
			// then a reference to it must be added to the border-regions
			// set.
			PIXELINDEX tnIfFirstLast
				= (tnY + PIXELINDEX (1) == m_tnY
					|| tnY == m_tnY + m_tnPGH) ? 1 : 0;
			if (tnY + PIXELINDEX (1) >= m_tnY
				&& tnY <= m_tnY + m_tnPGH
				&& tnXEnd >= m_tnX + tnIfFirstLast
				&& tnXStart < m_tnX + m_tnPGW + PIXELINDEX (1)
					- tnIfFirstLast)
			{
				// Add a reference to this region.
				AddMotionVectorMatch (pRegion);
			}
		}
	}

	// Does the search-border have any references to this region?
	if (pRegion->m_tnReferences != 0
		|| pRegion->NumberOfPoints() >= m_tnMatchSizeThrottle)
	{
		// Yes.  Add this region to our set of known regions.
		#ifndef NDEBUG
		typename MovedRegionSet::InsertResult oInsertResult =
		#endif // !NDEBUG
			m_setRegions.Insert (a_reStatus, pRegion);
		if (a_reStatus != g_kNoError)
			goto cleanup2;
		assert (oInsertResult.m_bInserted);
	}
	else
	{
		// No.  The region is fully constructed.  Move it to the list of
		// regions that'll get applied to the new frame's reference-image
		// representation.
		assert (pRegion->m_tnBorderExtents == 0);
		OnCompletedRegion (a_reStatus, pRegion);
		if (a_reStatus != g_kNoError)
		{
			DeleteRegion (pRegion);
			return;
		}
	}

	// All done.
	return;

	// Clean up after errors.
//cleanup4:
//	--pRegion->m_tnReferences;
cleanup3:
	--pRegion->m_tnReferences;
cleanup2:
	RemoveRegion (pRegion);
cleanup1:
	pRegion->~MovedRegion();
	m_oMovedRegionAllocator.Deallocate (0, sizeof (MovedRegion), pRegion);
cleanup0:
	;
}



// Removes the given region from the search-border.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::RemoveRegion (MovedRegion *a_pRegion)
{
	// Make sure they gave us a region to remove.
	assert (a_pRegion != NULL);

	// Make sure it's one of our border-regions, i.e. that it has
	// references.
	// It won't have references if its size exceeds the match-size-
	// throttle.  We expect our client to deal with these regions before
	// moving to the next pixel-group.
	assert (a_pRegion->m_tnReferences > 0
		|| a_pRegion->NumberOfPoints() >= m_tnMatchSizeThrottle);

	// Remove this region from the border-regions-set.
	{
		typename MovedRegionSet::Iterator itErase
			= m_setRegions.Find (a_pRegion);
		assert (itErase != m_setRegions.End());
		assert ((*itErase) == a_pRegion);
		m_setRegions.Erase (itErase);
	}

	// Remove all the startpoints/endpoints that refer to this region.
	// They can be easily found by looping through the region's extents.
	if (a_pRegion->NumberOfPoints() < m_tnMatchSizeThrottle)
	{
		typename MovedRegion::ConstIterator itHere;
			// Used to loop through moved-region extents.
		BorderExtentBoundary oExtent;
			// Used to search for startpoints/endpoints.

		// The extents will all have the same region.
		oExtent.m_pRegion = a_pRegion;

		// Loop through the region's extents, find the corresponding
		// startpoint and endpoint, and remove them.
		// Extents before the first-border can be ignored -- they were not
		// put into the startpoint/endpoint sets.
		for (itHere = (m_tnY == PIXELINDEX (0)) ? a_pRegion->Begin()
				: a_pRegion->LowerBound (m_tnY - PIXELINDEX (1),
					PIXELINDEX (0));
			 itHere != a_pRegion->End();
			 ++itHere)
		{
			typename BorderExtentBoundarySet::Iterator itExtent;
				// A startpoint/endpoint that we find.
			#ifndef NDEBUG
			#ifdef BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
			PIXELINDEX tnEndpoint;
			#else // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
			BorderExtentBoundary *pEndpoint;
			#endif // BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR
			#endif // !NDEBUG
				// The endpoint that corresponds to a found startpoint.

			// Get the details of this extent.
			const typename MovedRegion::Extent &rExtent = *itHere;
			PIXELINDEX tnY = rExtent.m_tnY;
			PIXELINDEX tnXStart = rExtent.m_tnXStart;
			PIXELINDEX tnXEnd = rExtent.m_tnXEnd;

			// Make sure this extent is inside our frame.  (Sanity check.)
			assert (tnY >= 0 && tnY < m_tnHeight);
			assert (tnXStart >= 0);
			assert (tnXStart < tnXEnd);
			assert (tnXEnd <= m_tnWidth);

			// Look for the corresponding startpoint.
			oExtent.m_pRegion = a_pRegion;
			BorderExtentBoundarySet &rStartpointSet
				= m_psetBorderStartpoints[tnY * m_tnWidth + tnXStart];
			itExtent = rStartpointSet.Find (oExtent);

			// Make sure it was found.
			assert (itExtent != rStartpointSet.End());

			// Remember its endpoint.
			#ifndef NDEBUG
			tnEndpoint = (*itExtent).m_tnCounterpartIndex;
			#endif // !NDEBUG

			// Remove the startpoint that corresponds to this extent.
			rStartpointSet.Erase (itExtent);

			// That's one less reference to this region.
			assert (a_pRegion->m_tnReferences > 0);
			--a_pRegion->m_tnReferences;

			// Look for the corresponding endpoint.
			// (The endpoint-X is adjusted downward by 1, because the
			// valid range of endpoint-X values is from 1 to m_tnWidth.)
			BorderExtentBoundarySet &rEndpointSet
				= m_psetBorderEndpoints[tnY * m_tnWidth
					+ (tnXEnd - PIXELINDEX (1))];
			itExtent = rEndpointSet.Find (oExtent);

			// Make sure it was found.
			assert (itExtent != rEndpointSet.End());

			// Make sure it's the endpoint that corresponded to this
			// startpoint.
			assert (tnXEnd == tnEndpoint);

			// Remove the endpoint that corresponds to this extent.
			rEndpointSet.Erase (itExtent);

			// That's one less reference to this region.
			assert (a_pRegion->m_tnReferences > 0);
			--a_pRegion->m_tnReferences;

			// If this extent intersects/borders the current pixel-group,
			// then a reference to it must be removed from the
			// motion-vector matches.
			PIXELINDEX tnIfFirstLast
				= (tnY + PIXELINDEX (1) == m_tnY
					|| tnY == m_tnY + m_tnPGH) ? 1 : 0;
			if (tnY + PIXELINDEX (1) >= m_tnY
				&& tnY <= m_tnY + m_tnPGH
				&& tnXEnd >= m_tnX + tnIfFirstLast
				&& tnXStart < m_tnX + m_tnPGW + PIXELINDEX (1)
					- tnIfFirstLast)
			{
				// Remove a reference to this region.
				RemoveMotionVectorMatch (a_pRegion);
			}
		}
	}

	// Make sure this region has no references left.
	assert (a_pRegion->m_tnReferences == 0);
}



// Loop through all regions that matched the current pixel-group,
// find the single best one, and return it.
// Backpatch the size of the second-best active-region.
template <class PIXELINDEX, class FRAMESIZE>
typename SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion *
SearchBorder<PIXELINDEX,FRAMESIZE>::ChooseBestActiveRegion
	(Status_t &a_reStatus, FRAMESIZE &a_rtnSecondBestActiveRegionSize)
{
	MovedRegion *pSurvivor;
		// The only region to survive pruning -- the biggest one, with
		// the shortest motion vector.
	typename MovedRegionSet::Iterator itFirst, itSecond;
		// Used to find the largest and second-largest active-regions.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Find the largest region and second-largest active-regions.
	itFirst = m_setRegions.Begin();
	assert (itFirst != m_setRegions.End());
	pSurvivor = *itFirst;
	itSecond = itFirst;
	++itSecond;
	a_rtnSecondBestActiveRegionSize = (itSecond == m_setRegions.End())
		? 0 : (*itSecond)->NumberOfPoints();

	// Remove this region from our consideration.
	RemoveRegion (pSurvivor);

	// Return the region.
	return pSurvivor;
}



// Clean up the search border at the end of a frame, e.g. hand all
// remaining regions back to the client.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::FinishFrame (Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// It turns out that we can accomplish this by pretending to move
	// down m_tnPGH+1 lines -- that'll clear out the last-border and the
	// current-border.
	for (int i = 0; i <= m_tnPGH; ++i)
	{
		MoveDown (a_reStatus);
		if (a_reStatus != g_kNoError)
			return;
		++m_tnY;
	}

	// Make sure that emptied the border.
	FRAMESIZE tnPixels = FRAMESIZE (m_tnWidth) * FRAMESIZE (m_tnHeight);
	#ifndef NDEBUG
	assert (m_psetBorderStartpoints != NULL);
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		assert (m_psetBorderStartpoints[tnI].Size() == 0);
	assert (m_psetBorderEndpoints != NULL);
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		assert (m_psetBorderEndpoints[tnI].Size() == 0);
	#endif // !NDEBUG
	assert (m_setRegions.Size() == 0);

	// Make sure there are no active-extents for any motion-vectors.
	#ifndef NDEBUG
	{
		FRAMESIZE tnMotionVectorMatches
			= (FRAMESIZE (2) * FRAMESIZE (m_tnSearchRadiusX)
				+ FRAMESIZE (1))
			* (FRAMESIZE (2) * FRAMESIZE (m_tnSearchRadiusY)
			* 	+ FRAMESIZE (1));
		for (FRAMESIZE i = 0; i < tnMotionVectorMatches; ++i)
			assert (m_pMotionVectorMatches[i].m_tnActiveExtents == 0);
	}
	#endif // !NDEBUG

	// Purge all border-startpoint/endpoint memory.
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		m_psetBorderStartpoints[tnI].Purge();
	for (FRAMESIZE tnI = 0; tnI < tnPixels; ++tnI)
		m_psetBorderEndpoints[tnI].Purge();

	// Make sure our temporary memory allocations have been purged.
	assert (m_oBorderExtentsAllocator.GetNumAllocated() == 0);
}



// Delete a region that was returned by ChooseBestActiveRegion()
// or OnCompletedRegion().
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::DeleteRegion (MovedRegion *a_pRegion)
{
	// Make sure they gave us a region to delete.
	assert (a_pRegion != NULL);

	// Easy enough.
	a_pRegion->~MovedRegion();
	m_oMovedRegionAllocator.Deallocate (0, sizeof (MovedRegion),
		a_pRegion);
}



#endif // __SEARCH_BORDER_H__
