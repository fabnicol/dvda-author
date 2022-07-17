#ifndef __MOTION_SEARCHER_H__
#define __MOTION_SEARCHER_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include <assert.h>
#include "mjpeg_types.h"
#include "TemplateLib.hh"
#include "Limits.hh"
#include "DoublyLinkedList.hh"
#include "ReferenceFrame.hh"
#include "SetRegion2D.hh"
#include "BitmapRegion2D.hh"
#include "Vector.hh"

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



// Define this to prevent reference-frame pixels from being used more
// than once.
#define USE_REFERENCEFRAMEPIXELS_ONCE



// Define this to throttle the output of the pixel-sorter, using only
// those matches with the lowest sum-of-absolute-differences.
#define THROTTLE_PIXELSORTER_WITH_SAD



// Define this to implement set-regions with vectors.
// Don't define this to implement set-regions with skip-lists.
#define SET_REGION_IMPLEMENTED_WITH_VECTOR



// Define this to implement the SearchBorder's border-extent-boundary-sets
// with vectors.
// Don't define this to implement the SearchBorder's
// border-extent-boundary-sets with skip-lists.
//
// (Even though this option is specific to the internals of SearchBorder,
// it's defined here, since all of our other configuration switches are
// here.)
#define BORDEREXTENTBOUNDARYSET_IMPLEMENTED_WITH_VECTOR



// Define this to use bitmap regions to implement zero-motion
// flood-fill.
#define ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS



// Define this to use bitmap regions to implement match-throttle
// flood-fill.
#define MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS



// Define this to use bitmap regions to implement pruning flood-fill.
//#define PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS



// Define this to use a bitmap region to implement the
// used-reference-pixel region.  Don't define it to use a set-based
// region to implement the used-reference-pixel region.
#define USED_REFERENCE_PIXELS_REGION_IS_BITMAP



// We'll be using this variant of the search-border.
// (SearchBorder uses SET_REGION_IMPLEMENTED_WITH_VECTOR to configure.)
#include "SearchBorder.hh"

// We'll be using this variant of the search-window.
#ifdef EXPAND_REGIONS
	#define OPTIONALLY_SORT_PIXEL_GROUPS
#endif // EXPAND_REGIONS
#ifdef THROTTLE_PIXELSORTER_WITH_SAD
	#define CALCULATE_SAD
#endif // THROTTLE_PIXELSORTER_WITH_SAD
#include "SearchWindow.hh"
#undef CALCULATE_SAD
#undef OPTIONALLY_SORT_PIXEL_GROUPS



// The generic motion-searcher class.  It's parameterized by the size of
// elements in the pixels, the dimension of the pixels, the numeric type
// to use in tolerance calculations, the numeric type to use for pixel
// indices, a numeric type big enough to hold the product of the largest
// expected frame width/height, the width/height of pixel groups to
// operate on, a numeric type big enough to hold pixel-dimension *
// pixel-group-width * pixel-group-height bits and serve as an array
// index, and the types of pixels, reference pixels, and reference
// frames to operate on.  When constructed, it's configured with the
// number of frames over which to accumulate pixel values, the search
// radius (in separate x and y directions), the error tolerances, and
// throttle values for the number of matches and the size of matches.
//
// Pixel values are tracked over several frames.  The idea is, if the
// motion searcher can prove that a particular pixel in several frames
// is really the same pixel, it can average together all the pixel's
// values to calculate a more accurate value for it.  Therefore, output
// is delayed by the number of frames specified in the constructor, to
// give the motion-searcher the slack to do this.
//
// The motion-searcher works on groups of pixels.  It iterates through a
// frame, looking for groups of pixels within the search radius that
// match the current group (within the error tolerance).  It sorts the
// found pixel-groups by sum-of-absolute-differences, keeps the best
// match-count-throttle matches, and flood-fills each one, storing the
// results in the search-border.  Future pixel-sorter matches are
// checked against this set, to filter out duplicate matches (and the
// associated flood-fills).
//
// Since this process can generate a lot of information, two throttles
// are used -- one on the number of matches, and one on the size of the
// largest match.  If either is exceeded, the largest matches found are
// applied to the frame before searching finishes, until neither throttle
// is exceeded.  This decreases the amount of work to finish the search,
// since some of the frame has been resolved already.  If the throttle
// values are too low, quality may be impacted, but if the throttle
// values are too high, performance may be impacted.
//
// Similarly, if there are no matches for the current pixel-group, then
// it is considered new information, and new reference pixels are
// allocated for the group and applied to the new frame before the
// search is finished.  However, these pixels may be overwritten later
// by regions that are applied to the frame.  This attempts to improve
// the efficiency of processing frames that don't match the previous
// frame, in a way that doesn't impact quality.
//
// Once the entire frame has been searched, the found regions are
// applied to the frame, from largest match to smallest, and always
// flood-filled first to get rid of any resolved pixels.  If such a region
// is no longer the largest region, it's put back into the pool, and the
// largest remaining region is evaluated instead.
//
// Any areas of the frame not resolved by this method are new
// information, and new reference pixels are allocated for them.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK,
	class PIXEL = Pixel<PIXEL_NUM,DIM,PIXEL_TOL>,
	class REFERENCEPIXEL
		= ReferencePixel<PIXEL_TOL,PIXEL_NUM,DIM,PIXEL>,
	class REFERENCEFRAME
		= ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE> >
class MotionSearcher
{
public:
	typedef PIXEL Pixel_t;
		// Our pixel type.

	typedef REFERENCEPIXEL ReferencePixel_t;
		// Our reference pixel type.

	typedef REFERENCEFRAME ReferenceFrame_t;
		// Our reference frame type.

	typedef PIXEL_NUM PixelValue_t;
		// The numeric type to use in pixel values, in each dimension
		// of our pixels.

	typedef PIXEL_TOL Tolerance_t;
		// The numeric type to use in tolerance calculations.

	MotionSearcher();
		// Default constructor.

	virtual ~MotionSearcher();
		// Destructor.

	void Init (Status_t &a_reStatus, int a_nFrames,
			PIXELINDEX a_tnWidth, PIXELINDEX a_tnHeight,
			PIXELINDEX a_tnSearchRadiusX, PIXELINDEX a_tnSearchRadiusY,
			PixelValue_t a_nZeroTolerance, PixelValue_t a_nTolerance,
			FRAMESIZE a_nMatchCountThrottle,
			FRAMESIZE a_nMatchSizeThrottle);
		// Initializer.  Provide the number of frames over which to
		// accumulate pixel data, the dimensions of the frames, the
		// search radius, the error tolerances, and the match throttles.

	const ReferenceFrame_t *GetFrameReadyForOutput (void);
		// If a frame is ready to be output, return it, otherwise return
		// NULL.
		// Call this once before each call to AddFrame(), to ensure that
		// AddFrame() has the space to accept another frame.  Note that
		// this implies the data in the returned frame will be
		// invalidated by AddFrame().

	void AddFrame (Status_t &a_reStatus, const Pixel_t *a_pPixels);
		// Add another frame to be analyzed into the system.
		// The digested version will eventually be returned by either
		// GetFrameReadyForOutput() or GetRemainingFrames().

	const ReferenceFrame_t *GetRemainingFrames (void);
		// Once there is no more input, call this repeatedly to get the
		// details of the remaining frames, until it returns NULL.

	void Purge (void);
		// Purge ourselves of temporary structures that aren't normally
		// freed at the end of a frame.  (Most are.)
		// Should be called every once in a while (e.g. every 10 frames).

private:
	int m_nFrames;
		// The number of reference frames we use.

	PIXELINDEX m_tnWidth;
	PIXELINDEX m_tnHeight;
	FRAMESIZE m_tnPixels;
		// The dimensions of each reference frame.

	PIXELINDEX m_tnSearchRadiusX, m_tnSearchRadiusY;
		// The search radius, i.e. how far from the current pixel
		// group we look when searching for possible moved instances of
		// the group.

	Tolerance_t m_tnTolerance, m_tnTwiceTolerance;
		// The error tolerance, i.e. the largest difference we're
		// willing to tolerate between pixels before considering them
		// to be the same pixel.  Also, twice the tolerance.

	Tolerance_t m_tnZeroTolerance;
		// The error tolerance for the zero-motion pass.

	FRAMESIZE m_nMatchCountThrottle;
		// How many matches we're willing to have for the current
		// pixel group before we just flood-fill the largest one and
		// apply it now.

	FRAMESIZE m_nMatchSizeThrottle;
		// The size (measured in number of pixels) that the largest
		// region in the area of the current pixel-group can be before
		// we just flood-fill the largest one and apply it now.

	PixelAllocator<REFERENCEPIXEL,FRAMESIZE> m_oPixelPool;
		// Our source for new reference pixels.

	ReferenceFrame_t **m_ppFrames;
		// Our reference frames; an array of pointers to frames.

	int m_nFirstFrame, m_nLastFrame;
		// The range of frames that contain useful info.
		// When both equal 0, no frames contain useful info.
		// When m_nFirstFrame is 0 and m_nLastFrame is m_nFrames,
		// it's time for GetFrameReadyForOutput() to emit a frame.
		// When m_nFirstFrame is greater than zero but less than
		// m_nLastFrame, it means our client is calling
		// GetRemainingFrames().

	ReferenceFrame_t *m_pNewFrame;
		// The reference-frame representation of the new frame.
		// Points to one of the instances in m_ppFrames[].

	ReferenceFrame_t *m_pReferenceFrame;
		// The reference frame, against which the new frame is
		// compared.
		// Points to one of the instances in m_ppFrames[].

	const Pixel_t *m_pNewFramePixels;
		// The pixels of the new frame (i.e. the raw version).

	PIXELINDEX m_tnX, m_tnY;
		// The index of the current pixel group.  Actually the index
		// of the top-left pixel in the current pixel group.  This
		// gets moved in a zigzag pattern, back and forth across the
		// frame and then down, until the end of the frame is reached.

	PIXELINDEX m_tnStepX;
		// Whether we're zigging or zagging.

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
	typedef typename Region_t::Allocator RegionAllocator_t;
		// How we use SetRegion2D<>.

	typedef BitmapRegion2D<PIXELINDEX,FRAMESIZE> BitmapRegion_t;
		// How we use BitmapRegion2D<>.

	typedef SearchBorder<PIXELINDEX,FRAMESIZE> BaseSearchBorder_t;
		// The base class for the type of search-border we'll be using.

	typedef typename BaseSearchBorder_t::MovedRegion MovedRegion;
		// A moved region of pixels that has been detected.

	RegionAllocator_t m_oRegionAllocator;
		// Used by all our set-regions to allocate space for their extents.

	typedef Set<MovedRegion *,
		typename MovedRegion::SortBySizeThenMotionVectorLength>
		MovedRegionSet;
	typedef typename MovedRegionSet::Allocator MovedRegionAllocator_t;
	MovedRegionAllocator_t m_oMovedRegionSetAllocator;
	MovedRegionSet m_setRegions;
		// All moving areas detected so far.
		// Sorted by decreasing size, then increasing motion vector
		// length, i.e. the order in which they should be applied to
		// the reference-frame version of the new frame.

	#ifdef USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	BitmapRegion_t m_oUsedReferencePixels;
	#else // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	Region_t m_oUsedReferencePixels;
	#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
		// The region describing all parts of the reference frame that
		// have been found in the new frame.

	#ifdef USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	BitmapRegion_t m_oTestedZeroMotionPixels;
	#else // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	Region_t m_oTestedZeroMotionPixels;
	#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
		// The region describing all parts of the reference frame that
		// have been tested in the zero-motion pass.

	MovedRegion m_oMatchThrottleRegion;
		// The region that's applied to the frame before motion
		// detection is finished.  Allocated here to avoid lots of
		// creation & destruction.

	#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	BitmapRegion_t m_oFloodFillRegion;
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		// The region that does the flood-filling for the
		// match-throttle-region.

	void ApplyRegionToNewFrame (Status_t &a_reStatus,
			const MovedRegion &a_rRegion);
		// Apply this region to the new frame.

	typedef SearchWindow<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
			PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
			REFERENCEFRAME> SearchWindow_t;
		// The type of search-window we'll be using.

	SearchWindow_t m_oSearchWindow;
		// The search window.  It contains all the cells needed to
		// analyze the image.

#ifdef THROTTLE_PIXELSORTER_WITH_SAD

	// A pixel group that matches the current pixel-group, with the
	// given sum-of-absolute-differences.
	class MatchedPixelGroup
	{
	public:
		Tolerance_t m_tnSAD;
			// The sum-of-absolute-differences.

		const typename SearchWindow_t::PixelGroup *m_pGroup;
			// The pixel group.

		MatchedPixelGroup();
			// Default constructor.

		MatchedPixelGroup (Tolerance_t a_tnSAD,
				const typename SearchWindow_t::PixelGroup *a_pGroup);
			// Initializing constructor.

		~MatchedPixelGroup();
			// Destructor.

		// A comparison class, suitable for Set<>.
		class SortBySAD
		{
		public:
			inline bool operator() (const MatchedPixelGroup &a_rLeft,
				const MatchedPixelGroup &a_rRight) const;
		};
	};

	typedef Set<MatchedPixelGroup,
			typename MatchedPixelGroup::SortBySAD>
			MatchedPixelGroupSet;
	typedef typename MatchedPixelGroupSet::Allocator MPGS_Allocator_t;
		// The type for a set of matched pixel-groups.

	MPGS_Allocator_t m_oMatchAllocator;
	MatchedPixelGroupSet m_setMatches;
		// All the matches for the current pixel-group that we
		// want to use.

#endif // THROTTLE_PIXELSORTER_WITH_SAD

	// The search-border, specialized to put completed regions into
	// m_setRegions.
	class SearchBorder_t : public BaseSearchBorder_t
	{
	private:
		typedef BaseSearchBorder_t BaseClass;
			// Keep track of who our base class is.

	public:
		SearchBorder_t (MovedRegionSet &a_rsetRegions,
				typename MovedRegion::Allocator &a_rAlloc
					= MovedRegion::Extents::Imp::sm_oNodeAllocator);
			// Constructor.  Provide a reference to the set where
			// completed regions will be stored.

		virtual void OnCompletedRegion (Status_t &a_reStatus,
				typename SearchBorder_t::MovedRegion *a_pRegion);
			// Tell BaseSearchBorder_t how to hand us a completed region.

	private:
		MovedRegionSet &m_rsetRegions;
			// A reference to our list of completed regions.
	};

	SearchBorder_t m_oSearchBorder;
		// The search border, i.e. all regions on the border between
		// the searched area and the not-yet-searched area, the regions
		// still under construction.

	FRAMESIZE SearchBorder_AddNewRegion (Status_t &a_reStatus,
			PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY);
		// Add a new region, with the given motion vector, to the
		// search border.
		// Flood-fills its area before adding.
		// Returns the size of the added region.

	FRAMESIZE SearchBorder_MatchThrottle (Status_t &a_reStatus,
			FRAMESIZE a_nMatchCount,
			PIXELINDEX &a_rtnMotionX, PIXELINDEX &a_rtnMotionY);
		// Get the best region that matched the current pixel-group
		// (which is usually the largest active-region).  Expand it as far
		// as it can go, i.e. flood-fill in its area.  If it's no longer
		// the largest region, put it back and try again.  Otherwise, apply
		// it to the new frame now.
		//
		// Pass the number of pixel-group matches as an argument.  If a
		// best-region candidate is found to have no unresolved pixels,
		// it's no longer considered a pixel-group match, and that may lead
		// to the discovery that the match-count-throttle is no longer
		// being exceeded.
		//
		// Returns the number of points flood-filled, and backpatches the
		// motion vector associated with the best region.
		// May return zero if analysis reveals that the match-throttles
		// weren't really exceeded.

	// A class that helps implement the zero-motion flood-fill.
	class ZeroMotionFloodFillControl
		#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public BitmapRegion_t::FloodFillControl
		#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public Region_t::FloodFillControl
		#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	{
	private:
		#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename BitmapRegion_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
		#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename Region_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
		#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

		MotionSearcher *m_pMotionSearcher;
			// The motion-searcher we're working for.

	public:
		#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		ZeroMotionFloodFillControl();
		#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		ZeroMotionFloodFillControl
				(typename BaseClass::Allocator &a_rAllocator
					= Region_t::Extents::Imp::sm_oNodeAllocator);
		#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
			// Default constructor.  Must be followed by Init().

		void Init (Status_t &a_reStatus,
				MotionSearcher *a_pMotionSearcher);
			// Initializer.

		// Redefined FloodFillControl methods.

		bool ShouldUseExtent (typename
				ZeroMotionFloodFillControl::BaseClass::Extent &a_rExtent);
			// Return true if the flood-fill should examine the given
			// extent.  May modify the extent.

		bool IsPointInRegion (PIXELINDEX a_tnX, PIXELINDEX a_tnY);
			// Returns true if the given point should be included in the
			// flood-fill.
	};

	friend class ZeroMotionFloodFillControl;
		// Allow the zero-motion flood-fill-control class direct access.

	ZeroMotionFloodFillControl m_oZeroMotionFloodFillControl;
		// Used to implement flood-filling the result of the
		// zero-motion search.

	// A class that helps implement the match-throttle flood-fill.
	class MatchThrottleFloodFillControl
		#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public BitmapRegion_t::FloodFillControl
		#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public Region_t::FloodFillControl
		#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	{
	private:
		#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename BitmapRegion_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
		#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename Region_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
		#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	public:
		#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		MatchThrottleFloodFillControl();
		#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		MatchThrottleFloodFillControl
				(typename BaseClass::Allocator &a_rAllocator
					= Region_t::Extents::Imp::sm_oNodeAllocator);
		#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
			// Default constructor.  Must be followed by Init().

		void Init (Status_t &a_reStatus,
				MotionSearcher *a_pMotionSearcher);
			// Initializer.

		void SetupForFloodFill (PIXELINDEX a_tnMotionX,
				PIXELINDEX a_tnMotionY);
			// Set up to to a flood-fill.  Provide the motion vector.
			// Call this before doing a flood-fill with this control
			// object.

		// Redefined FloodFillControl methods.

		bool ShouldUseExtent (typename Region_t::Extent &a_rExtent);
			// Return true if the flood-fill should examine the given
			// extent.  May modify the extent.

		bool IsPointInRegion (PIXELINDEX a_tnX, PIXELINDEX a_tnY);
			// Returns true if the given point should be included in the
			// flood-fill.

	private:
		MotionSearcher *m_pMotionSearcher;
			// The motion-searcher we're working for.

		PIXELINDEX m_tnMotionX, m_tnMotionY;
			// The motion vector to be used for this flood-fill.
	};

	friend class MatchThrottleFloodFillControl;
		// Allow the match-throttle flood-fill-control class direct access.

	MatchThrottleFloodFillControl m_oMatchThrottleFloodFillControl;
		// Used to implement flood-filling a region before
		// motion-searching is done.

	// A class that helps implement the pruning flood-fill, i.e. the one
	// that removes resolved pixels from a candidate moved-region.
	class PruningFloodFillControl
		#ifdef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public BitmapRegion_t::FloodFillControl
		#else // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public Region_t::FloodFillControl
		#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
	{
	private:
		#ifdef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename BitmapRegion_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
		#else // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename Region_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
		#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
	public:
		#ifdef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		PruningFloodFillControl();
		#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		PruningFloodFillControl
				(typename BaseClass::Allocator &a_rAllocator
					= Region_t::Extents::Imp::sm_oNodeAllocator);
		#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
			// Default constructor.  Must be followed by Init().

		void Init (Status_t &a_reStatus,
				MotionSearcher *a_pMotionSearcher);
			// Initializer.

		void SetupForFloodFill (PIXELINDEX a_tnMotionX,
				PIXELINDEX a_tnMotionY);
			// Set up to to a flood-fill.  Provide the motion vector.
			// Call this before doing a flood-fill with this control
			// object.

		// Redefined FloodFillControl methods.

		bool ShouldUseExtent (typename Region_t::Extent &a_rExtent);
			// Return true if the flood-fill should examine the given
			// extent.  May modify the extent.

		bool IsPointInRegion (PIXELINDEX a_tnX, PIXELINDEX a_tnY);
			// Returns true if the given point should be included in the
			// flood-fill.

	private:
		MotionSearcher *m_pMotionSearcher;
			// The motion-searcher we're working for.

		PIXELINDEX m_tnMotionX, m_tnMotionY;
			// The motion vector to be used for this flood-fill.
	};

	friend class PruningFloodFillControl;
		// Allow the zero-motion flood-fill-control class direct access.

	PruningFloodFillControl m_oPruningFloodFillControl;
		// Used to implement flood-filling a region to remove any
		// resolved pixels.
};



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
		PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
		REFERENCEFRAME>::MotionSearcher()
	: m_oRegionAllocator (1048576),
	m_oMovedRegionSetAllocator (262144),
	m_setRegions (typename MovedRegion::SortBySizeThenMotionVectorLength(),
		m_oMovedRegionSetAllocator),
	m_oMatchThrottleRegion (m_oRegionAllocator),
	#ifdef THROTTLE_PIXELSORTER_WITH_SAD
	m_oMatchAllocator (65536),
	m_setMatches (typename MatchedPixelGroup::SortBySAD(),
		m_oMatchAllocator),
	#endif // THROTTLE_PIXELSORTER_WITH_SAD
	m_oSearchBorder (m_setRegions, m_oRegionAllocator)
	#ifndef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	, m_oZeroMotionFloodFillControl (m_oRegionAllocator)
	#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	#ifndef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	, m_oMatchThrottleFloodFillControl (m_oRegionAllocator)
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	#ifndef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
	, m_oPruningFloodFillControl (m_oRegionAllocator)
	#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// No frames yet.
	m_nFrames = 0;
	m_ppFrames = NULL;
	m_nFirstFrame = m_nLastFrame = 0;
	m_tnWidth = m_tnHeight = PIXELINDEX (0);
	m_tnPixels = FRAMESIZE (0);

	// No information on the sort of search to do yet.
	m_tnSearchRadiusX = m_tnSearchRadiusY = PIXELINDEX (0);
	m_tnZeroTolerance = m_tnTolerance = m_tnTwiceTolerance
		= PIXEL_TOL (0);
	m_nMatchCountThrottle = 0;
	m_nMatchSizeThrottle = 0;

	// No active search yet.
	m_tnX = m_tnY = m_tnStepX = PIXELINDEX (0);
	m_pNewFrame = NULL;
	m_pReferenceFrame = NULL;
	m_pNewFramePixels = NULL;
}



// Destructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::~MotionSearcher()
{
	// Free up any remaining moved regions.  (Testing for a non-zero
	// size is defined to be safe even if the set hasn't been
	// initialized, i.e. if we get destroyed before our Init() has been
	// called.)
	if (m_setRegions.Size() > 0)
	{
		typename MovedRegionSet::Iterator itHere;
			// The location of the next region to destroy.

		// Loop through the moved-regions set, remove each item,
		// destroy it.
		while (itHere = m_setRegions.Begin(),
			itHere != m_setRegions.End())
		{
			// Get the moved-region to destroy.
			MovedRegion *pRegion = *itHere;

			// Remove it from the set.
			m_setRegions.Erase (itHere);

			// Destroy the region.
			m_oSearchBorder.DeleteRegion (pRegion);
		}
	}

	// Destroy the reference frames.
	for (int i = 0; i < m_nFrames; i++)
	{
		m_ppFrames[i]->Reset();
		delete m_ppFrames[i];
	}
	delete[] m_ppFrames;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,REFERENCEFRAME>::Init
	(Status_t &a_reStatus, int a_nFrames, PIXELINDEX a_tnWidth,
	PIXELINDEX a_tnHeight, PIXELINDEX a_tnSearchRadiusX,
	PIXELINDEX a_tnSearchRadiusY, PixelValue_t a_tnZeroTolerance,
	PixelValue_t a_tnTolerance, FRAMESIZE a_nMatchCountThrottle,
	FRAMESIZE a_nMatchSizeThrottle)
{
	int i;
		// Used to loop through things.
	FRAMESIZE tnPixels;
		// The number of pixels in each frame.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a reasonable number of frames.
	// (We need at least one for the new frame and one to
	// compare against the new frame.  We prefer more.)
	assert (a_nFrames >= 2);

	// Make sure the width & height are reasonable.
	assert (a_tnWidth > PIXELINDEX (0));
	assert (a_tnHeight > PIXELINDEX (0));

	// Make sure the search radius is reasonable.
	assert (a_tnSearchRadiusX > PIXELINDEX (0)
		&& a_tnSearchRadiusY > PIXELINDEX (0)
		&& a_tnSearchRadiusX <= a_tnWidth
		&& a_tnSearchRadiusY <= a_tnHeight);

	// Make sure the match throttles are reasonable.
	assert (a_nMatchCountThrottle >= 0
		&& a_nMatchCountThrottle
			<= a_tnSearchRadiusX * a_tnSearchRadiusY);
	assert (a_nMatchSizeThrottle > 0);

	// Calculate the number of pixels in each frame.
	tnPixels = FRAMESIZE (a_tnWidth) * FRAMESIZE (a_tnHeight);

	// Initialize our pixel pool.
	m_oPixelPool.Initialize (a_reStatus, tnPixels * FRAMESIZE (a_nFrames));
	if (a_reStatus != g_kNoError)
		return;

	// Allocate space for our pointers to frames.
	m_ppFrames = new ReferenceFrame_t * [a_nFrames];
	if (m_ppFrames == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}
	// (Initialize each one to NULL, in case we run out of memory while
	// trying to allocate frames -- if we don't do this, we'll end up
	// trying to delete garbage pointers.)
	for (i = 0; i < a_nFrames; ++i)
		m_ppFrames[i] = NULL;
	// (Save this parameter now, to make it possible to destroy an
	// incompletely-initialized object.)
	m_nFrames = a_nFrames;

	// Allocate our reference frames.
	for (i = 0; i < a_nFrames; ++i)
	{
		// Allocate the next reference frame.
		m_ppFrames[i] = new ReferenceFrame_t (a_reStatus, a_tnWidth,
			a_tnHeight);
		if (m_ppFrames[i] == NULL)
		{
			a_reStatus = g_kOutOfMemory;
			return;
		}
		if (a_reStatus != g_kNoError)
			return;
	}

	// Initialize the search-window.
	m_oSearchWindow.Init (a_reStatus, a_tnWidth, a_tnHeight,
		a_tnSearchRadiusX, a_tnSearchRadiusY, a_tnTolerance);
	if (a_reStatus != g_kNoError)
		return;

	// Initialize our set of matches.  (We'll use this to sort the
	// incoming matches by how closely it matches the current
	// pixel-group, and we'll throw away fuzzier matches.)
	#ifdef THROTTLE_PIXELSORTER_WITH_SAD
	typename MatchedPixelGroupSet::InitParams oInitSetMatches (rand(),
		true /* allocate internal nodes from allocator */);
	m_setMatches.Init (a_reStatus, true, oInitSetMatches);
	if (a_reStatus != g_kNoError)
		return;
	#endif // THROTTLE_PIXELSORTER_WITH_SAD

	// Initialize our moved-regions set.
	m_setRegions.Init (a_reStatus, true);
	if (a_reStatus != g_kNoError)
		return;

	// Initialize the moved-region extents allocator.
	#ifdef SET_REGION_IMPLEMENTED_WITH_VECTOR
	m_oRegionAllocator.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
	#endif // SET_REGION_IMPLEMENTED_WITH_VECTOR

	// Initialize our used reference-pixels container.
	#ifdef USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	m_oUsedReferencePixels.Init (a_reStatus, a_tnWidth, a_tnHeight);
	#else // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	m_oUsedReferencePixels.Init (a_reStatus, m_oRegionAllocator);
	#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	if (a_reStatus != g_kNoError)
		return;
	#if defined (USED_REFERENCE_PIXELS_REGION_IS_BITMAP)
	#if defined (DEBUG_REGION2D)
	// HACK: too expensive, test region class elsewhere.
	m_oUsedReferencePixels.SetDebug (false);
	#endif // DEBUG_REGION2D
	#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP

	// Initialize our tested zero-motion-pixels container.
	#ifdef USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	m_oTestedZeroMotionPixels.Init (a_reStatus, a_tnWidth, a_tnHeight);
	#else // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	m_oTestedZeroMotionPixels.Init (a_reStatus, m_oRegionAllocator);
	#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	if (a_reStatus != g_kNoError)
		return;
	#if defined (USED_REFERENCE_PIXELS_REGION_IS_BITMAP)
	#if defined (DEBUG_REGION2D)
	// HACK: too expensive, test region class elsewhere.
	m_oTestedZeroMotionPixels.SetDebug (false);
	#endif // DEBUG_REGION2D
	#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP

	// Initialize our match-throttle region.
	m_oMatchThrottleRegion.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;

	// Initialize the region that does the flood-filling for the
	// match-throttle-region.
	#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oFloodFillRegion.Init (a_reStatus, a_tnWidth, a_tnHeight);
	if (a_reStatus != g_kNoError)
		return;
	#if defined (DEBUG_REGION2D)
	// HACK: too expensive, test region class elsewhere.
	m_oFloodFillRegion.SetDebug (false);
	#endif // DEBUG_REGION2D
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Initialize the search-border.
	// Note that the search-border is not given the value of
	// the externally-set match-size-throttle.  That was done
	// back when throttling happened at the end of a pixel-group
	// search if any of the resulting regions exceeded the
	// match-size-throttle.  Now, those large regions are kept
	// in the search-border until all the new conditions for
	// throttling are met.
	m_oSearchBorder.Init (a_reStatus, a_tnWidth, a_tnHeight,
		a_tnSearchRadiusX, a_tnSearchRadiusY, PGW, PGH,
		/* a_nMatchSizeThrottle */ a_tnWidth * a_tnHeight);
	if (a_reStatus != g_kNoError)
		return;

	// Finally, store our parameters.  (Convert the tolerance value to
	// the format used internally.)
	//m_nFrames = a_nFrames;		(Stored above)
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;
	m_tnPixels = tnPixels;
	m_tnSearchRadiusX = a_tnSearchRadiusX;
	m_tnSearchRadiusY = a_tnSearchRadiusY;
	m_tnZeroTolerance = Pixel_t::MakeTolerance (a_tnZeroTolerance);
	m_tnTolerance = Pixel_t::MakeTolerance (a_tnTolerance);
	m_tnTwiceTolerance = Pixel_t::MakeTolerance (PixelValue_t (2)
		* a_tnTolerance);
	m_nMatchCountThrottle = a_nMatchCountThrottle;
	m_nMatchSizeThrottle = a_nMatchSizeThrottle;

	// Initialize our flood-fill controllers.  (This happens after we
	// store our parameters, because these methods may need those
	// values.)
	m_oZeroMotionFloodFillControl.Init (a_reStatus, this);
	if (a_reStatus != g_kNoError)
		return;
	m_oMatchThrottleFloodFillControl.Init (a_reStatus, this);
	if (a_reStatus != g_kNoError)
		return;
	m_oPruningFloodFillControl.Init (a_reStatus, this);
	if (a_reStatus != g_kNoError)
		return;
}



// If a frame is ready to be output, return it, otherwise return
// NULL.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
const typename MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,
	FRAMESIZE, PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ReferenceFrame_t *
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::GetFrameReadyForOutput (void)
{
	ReferenceFrame_t *pFrame;
		// The frame to return to the caller.
	int i;
		// Used to loop through things.

	// If we have space for the new frame in our reference frames, then
	// it's not time to emit a frame yet.  (We delay emitting frames for
	// as long as possible, in order to calculate the most accurate
	// pixel values.)
	if (m_nFirstFrame != 0 || m_nLastFrame != m_nFrames)
		return NULL;

	// Get the frame to return to the caller.
	pFrame = m_ppFrames[0];

	// Shift the remaining frames down.
	for (i = 1; i < m_nFrames; ++i)
		m_ppFrames[i - 1] = m_ppFrames[i];

	// Our caller will read the data in the frame.  By the time the
	// caller calls AddFrame(), we'll need to use this frame again.
	// So put it at the end of the list.
	--m_nLastFrame;
	m_ppFrames[m_nLastFrame] = pFrame;

	// Finally, return the frame to our caller.
	return pFrame;
}



// HACK: developer debugging output.
extern "C" { extern int frame, verbose; };

// Add another frame to be analyzed into the system.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,REFERENCEFRAME>::AddFrame
	(Status_t &a_reStatus, const Pixel_t *a_pPixels)
{
	FRAMESIZE i;
		// Used to loop through pixels.
	FRAMESIZE tnNotMovedZeroMotionPixels, tnNotMovedThrottledPixels,
			tnNotMovedPixels, tnMovedThrottledPixels, tnMovedPixels,
			tnNoMatchNewPixels, tnNewPixels;
		// Statistics on the result of our analysis -- the number of
		// pixels that didn't move, the number that moved, the number
		// found by flood-filling, and the number of new pixels.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we can accept a new frame.
	assert (m_nFirstFrame == 0 && m_nLastFrame < m_nFrames);

	// Get the reference frame that will become the new frame.
	m_pNewFrame = m_ppFrames[m_nLastFrame];
	m_pNewFramePixels = a_pPixels;

	// Reset the new frame, so that it doesn't refer to any pixels.
	// (This frame was previously returned by GetFrameReadyForOutput(),
	// so it wasn't safe to reset it until now.)
	m_pNewFrame->Reset();

	// Reset our statistics.
	tnNotMovedZeroMotionPixels = tnNotMovedThrottledPixels
		= tnNotMovedPixels = tnMovedThrottledPixels = tnMovedPixels
		= tnNoMatchNewPixels = tnNewPixels = FRAMESIZE (0);

	// If there is a previous frame, do motion-detection against it.
	if (m_nFirstFrame != m_nLastFrame)
	{
		PIXELINDEX tnLastX, tnLastY;
			// Used to zigzag through the frame.

		// Get the reference frame, i.e. the one that we'll do
		// motion-detection against.  (For now, that's the previous
		// frame.  Eventually, we'd like to do motion-detection against
		// several previous frames, but not yet.)
		m_pReferenceFrame = m_ppFrames[m_nLastFrame - 1];

		// Prepare to search within this frame.
		m_oSearchWindow.StartFrame (m_pReferenceFrame);
		m_oSearchBorder.StartFrame (a_reStatus);
		if (a_reStatus != g_kNoError)
			return;

		// Start by processing parts of the image that aren't moving.
		// Loop through pixel-group-sized chunks of the image, find
		// pixel-groups within the specified tolerance, and flood-fill
		// them.  If the resulting region exceeds the match-size-throttle,
		// apply it now.  Either way, remember the region that was found
		// (so that the region doesn't have to be found again).
		//
		// Skip if they specified a zero for the tolerance.
		m_oUsedReferencePixels.Clear();
		m_oTestedZeroMotionPixels.Clear();
		if (m_tnZeroTolerance != 0)
		{
			m_tnY = 0;
			for (;;)
			{
				PIXELINDEX tnPixelX, tnPixelY;
					// Used to loop through pixels in the pixel-group.

				m_tnX = 0;
				for (;;)
				{
					ReferencePixel_t *pPrevPixel;
						// The pixel from the previous frame.
					#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					typename BitmapRegion_t::ConstIterator itExtent;
					#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
						// Used to convert between region types.

					// Loop through the pixels to compare, see if they all
					// match within the tolerance.
					for (tnPixelY = m_tnY;
						 tnPixelY < m_tnY + PGH;
						 ++tnPixelY)
					{
						for (tnPixelX = m_tnX;
							 tnPixelX < m_tnX + PGW;
							 ++tnPixelX)
						{
							// If a pixel in this pixel-group has already
							// been tested in this pass, skip it.
							if (m_oTestedZeroMotionPixels.DoesContainPoint
									(tnPixelY, tnPixelX))
								goto noMatch;

							// Get the two pixels to compare.
							pPrevPixel = m_pReferenceFrame->GetPixel
								(tnPixelX, tnPixelY);
							assert (pPrevPixel != NULL);
							const Pixel_t &rPrevPixel
								= pPrevPixel->GetValue();
							const Pixel_t &rNewPixel
								= a_pPixels[tnPixelY * m_tnWidth
									+ tnPixelX];

							// Compare them.
							if (!rPrevPixel.IsWithinTolerance (rNewPixel,
								m_tnZeroTolerance))
							{
								// No match.
								goto noMatch;
							}
						}
					}

					// These pixels are within the zero-motion tolerance.
					// Set up a region describing the current pixel-group.
					#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					m_oFloodFillRegion.Clear();
					for (tnPixelY = m_tnY;
						 tnPixelY < m_tnY + PGH;
						 ++tnPixelY)
					{
						m_oFloodFillRegion.Merge (a_reStatus, tnPixelY,
							m_tnX, m_tnX + PGW);
						if (a_reStatus != g_kNoError)
							return;
					}
					#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					m_oMatchThrottleRegion.Clear();
					for (tnPixelY = m_tnY;
						 tnPixelY < m_tnY + PGH;
						 ++tnPixelY)
					{
						m_oMatchThrottleRegion.Merge (a_reStatus, tnPixelY,
							m_tnX, m_tnX + PGW);
						if (a_reStatus != g_kNoError)
							return;
					}
					#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

					// Set its motion vector.
					m_oMatchThrottleRegion.SetMotionVector (0, 0);

					// Set its location.  (Needed only for debugging
					// purposes.)
					#ifndef NDEBUG
					m_oMatchThrottleRegion.m_tnX = m_tnX;
					m_oMatchThrottleRegion.m_tnY = m_tnY;
					#endif // !NDEBUG

					// Flood-fill this match, so as to get its full extent.
					#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					m_oFloodFillRegion.FloodFill (a_reStatus,
						m_oZeroMotionFloodFillControl, false, true);
					#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					m_oMatchThrottleRegion.FloodFill (a_reStatus,
						m_oZeroMotionFloodFillControl, false, true);
					#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					if (a_reStatus != g_kNoError)
						return;

					// Now copy the results of the flood-fill to the
					// match-throttle region.
					#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
					m_oMatchThrottleRegion.Clear();
					for (itExtent = m_oFloodFillRegion.Begin();
						 itExtent != m_oFloodFillRegion.End();
						 ++itExtent)
					{
						// Get the current extent.
						const typename BitmapRegion_t::Extent &rExtent
							= *itExtent;

						// Copy it to the match-throttle region.
						m_oMatchThrottleRegion.Union
							(a_reStatus, rExtent.m_tnY,
								rExtent.m_tnXStart, rExtent.m_tnXEnd);
						if (a_reStatus != g_kNoError)
							return;
					}
					#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

					// All of these reference pixels have been tested.
					// There's no need to test them again.
					{
						typename MovedRegion::ConstIterator itExtent;
						for (itExtent = m_oMatchThrottleRegion.Begin();
							 itExtent != m_oMatchThrottleRegion.End();
							 ++itExtent)
						{
							// Get the current extent.
							const typename MovedRegion::Extent &rExtent
								= *itExtent;

							// Add it to our running total of tested
							// zero-motion reference-pixels.
							m_oTestedZeroMotionPixels.Union
								(a_reStatus, rExtent.m_tnY,
									rExtent.m_tnXStart, rExtent.m_tnXEnd);
							if (a_reStatus != g_kNoError)
								return;
						}
					}

					// If this match is larger than the
					// match-size-throttle, apply it now.
					if (m_oMatchThrottleRegion.NumberOfPoints()
						>= m_nMatchSizeThrottle)
					{
						// Apply this region to the new frame.
						ApplyRegionToNewFrame (a_reStatus,
							m_oMatchThrottleRegion);
						if (a_reStatus != g_kNoError)
							return;

						// That's more resolved pixels.
						tnNotMovedZeroMotionPixels
							+= m_oMatchThrottleRegion.NumberOfPoints();

						// Clean up after ourselves.
						m_oMatchThrottleRegion.Clear();
					}

					// Otherwise, just dispose of it.
					// m_oTestedZeroMotionPixels will prevent us from
					// finding it again.
					else
						m_oMatchThrottleRegion.Clear();

noMatch:
					// Now move X forward, but in a way that handles
					// frames whose dimensions are not even multiples of
					// the pixel-group dimension.
					if (m_tnX + PGW == m_tnWidth)
						break;
					m_tnX += PGW;
					if (m_tnX > m_tnWidth - PGW)
						m_tnX = m_tnWidth - PGW;
				}

				// Now move Y forward, but in a way that handles
				// frames whose dimensions are not even multiples of
				// the pixel-group dimension.
				if (m_tnY + PGH == m_tnHeight)
					break;
				m_tnY += PGH;
				if (m_tnY > m_tnHeight - PGH)
					m_tnY = m_tnHeight - PGH;
			}

			// Clean up after ourselves.
			m_oTestedZeroMotionPixels.Clear();
		}

		// Now do the motion-compensated denoising.  Start in the
		// upper-left corner of the frame, and zigzag down the frame
		// (i.e. move all the way right, then down one line, then all
		// the way left, then down one line, etc.).  Look for matches
		// for the current pixel-group in the reference frame, and
		// build regions of such matches.
		//
		// (Skip it if they turned motion-detection off.)
		if (m_tnTolerance > 0)
		{
			// Start searching in the upper-left corner, and prepare to
			// move right.
			m_tnX = m_tnY = 0;
			tnLastX = m_tnWidth - PGW;
			tnLastY = m_tnHeight - PGH;
			m_tnStepX = 1;

			// Loop until the entire frame has been searched.
			for (;;)
			{
				typename SearchWindow_t::PixelGroup oCurrentGroup;
					// The current pixel-group.
				typename SearchWindow_t::PixelSorterIterator itMatch;
					// Used to search for matches for the current
					// pixel-group.
				#ifdef THROTTLE_PIXELSORTER_WITH_SAD
				typename MatchedPixelGroupSet::ConstIterator
						itBestMatch;
					// One of the best matches found.
				Tolerance_t tnSAD;
					// The sum-of-absolute-differences between the
					// current pixel group and the match.
				#endif // THROTTLE_PIXELSORTER_WITH_SAD
				const typename SearchWindow_t::PixelGroup *pMatch;
					// A pixel-group that matches the current pixel,
					// within the configured tolerance.
				FRAMESIZE tnMatches;
					// The number of matches found.

				// Create the current pixel-group.  If any of its pixels
				// have been resolved, skip it.
				{
					PIXELINDEX x, y;
						// Used to loop through the current
						// pixel-group's pixels.

					for (y = 0; y < PGH; ++y)
					{
						for (x = 0; x < PGW; ++x)
						{
							PIXELINDEX tnPixelX, tnPixelY;
								// The index of the current pixel.

							// Calculate the index of the current pixel.
							tnPixelX = m_tnX + x;
							tnPixelY = m_tnY + y;

							// If this pixel has been resolved already,
							// skip this pixel-group.
							if (m_pNewFrame->GetPixel (tnPixelX,
									tnPixelY) != NULL)
								goto nextGroup;

							// Set the pixel value in the pixel-group.
							oCurrentGroup.m_atPixels[y][x]
								= a_pPixels[FRAMESIZE (tnPixelY)
									* FRAMESIZE (m_tnWidth)
									+ FRAMESIZE (tnPixelX)];
						}
					}
				}

				// Tell the pixel-group where it is.
				oCurrentGroup.m_tnX = m_tnX;
				oCurrentGroup.m_tnY = m_tnY;

				// HACK
				#if 0
				fprintf (stderr, "Now checking pixel-group, frame %d, "
					"x %d, y %d\n", frame, int (m_tnX), int (m_tnY));
				#endif

				// No matches yet.
				tnMatches = 0;

				// Find matches for the current pixel-group.  Search for
				// them in the pixel-sorter tree, make sure each wasn't
				// already found (i.e. isn't already in the search-border),
				// flood-fill each one, and add each to the search-border.
				//
				// If the number of matches in the area exceeds the
				// match-count-throttle, or if the largest region found
				// exceeds the match-size-throttle, then pick the largest
				// region that intersects the current pixel-group and apply
				// it to the frame now.

				// Set up the search-window in a radius around the
				// current pixel-group.
				m_oSearchWindow.PrepareForSearch (a_reStatus, true);
				if (a_reStatus != g_kNoError)
					return;

				// Search for matches for the current pixel-group
				// within the search radius.
				m_oSearchWindow.StartSearch (itMatch, oCurrentGroup);
				#ifdef THROTTLE_PIXELSORTER_WITH_SAD
				m_setMatches.Clear();
				while (pMatch = m_oSearchWindow.FoundNextMatch (itMatch,
					tnSAD), pMatch != NULL)
				#else // THROTTLE_PIXELSORTER_WITH_SAD
				while (pMatch = m_oSearchWindow.FoundNextMatch (itMatch),
					pMatch != NULL)
				#endif // THROTTLE_PIXELSORTER_WITH_SAD
				{
					// Calculate its motion vector.
					PIXELINDEX tnMotionX = pMatch->m_tnX - m_tnX;
					PIXELINDEX tnMotionY = pMatch->m_tnY - m_tnY;

					#ifdef PRINT_SEARCHBORDER
					//if (frame == 61 && DIM == 2)
					fprintf (stderr, "Found match at (%d,%d), "
						"motion vector (%d,%d)\n",
						int (m_tnX), int (m_tnY),
						int (tnMotionX), int (tnMotionY));
					#endif // PRINT_SEARCHBORDER

					// Make sure this match is within the search
					// radius.  (Sanity check.)
					assert (AbsoluteValue (tnMotionX)
						<= m_tnSearchRadiusX);
					assert (AbsoluteValue (tnMotionY)
						<= m_tnSearchRadiusY);

					// Make sure all matches refer to unused
					// reference-frame pixels.
					#ifndef NDEBUG
					#ifdef USE_REFERENCEFRAMEPIXELS_ONCE
					{
						PIXELINDEX x, y;
							// Used to loop through the current
							// pixel-group's pixels.

						for (y = 0; y < PGH; ++y)
						{
							for (x = 0; x < PGW; ++x)
							{
								PIXELINDEX tnPixelX, tnPixelY;
									// The index of the current pixel.

								// Calculate the index of the current
								// pixel.
								tnPixelX = pMatch->m_tnX + x;
								tnPixelY = pMatch->m_tnY + y;

								// Make sure this reference-frame pixel
								// hasn't been used already.
								assert (!m_oUsedReferencePixels
									.DoesContainPoint (tnPixelY,
										tnPixelX));
							}
						}
					}
					#endif // USE_REFERENCEFRAMEPIXELS_ONCE
					#endif // !NDEBUG

					// That's one more match.
					++tnMatches;

					// See if a region with this motion vector already
					// intersects/borders the current pixel-group, and
					// get its size.
					bool bExistingMatch = m_oSearchBorder.HasExistingMatch
						(tnMotionX, tnMotionY);

					// HACK
					#if 0
					fprintf (stderr, "\tFound match, "
						"motion vector (%d,%d)",
						int (tnMotionX), int (tnMotionY));
					if (!bExistingMatch)
						fprintf (stderr, " - new\n");
					else
						fprintf (stderr, " - existing\n");
					#endif

				#ifdef THROTTLE_PIXELSORTER_WITH_SAD

					// If a region with this motion vector already
					// intersects/borders the current pixel-group, then
					// this match can be skipped.
					if (!bExistingMatch)
					{
						// If this match is better than our worst so
						// far, get rid of our worst & use the new one
						// instead.
						if (m_setMatches.Size() == m_nMatchCountThrottle)
						{
							typename MatchedPixelGroupSet::Iterator
									itWorst;
								// The worst match found so far.

							// Get the worst match.  (It's at the end of
							// the list.)
							itWorst = m_setMatches.End();
							--itWorst;

							// If the new item is better than our worst,
							// get rid of our worst to make room for the
							// new item.
							if (tnSAD < (*itWorst).m_tnSAD)
								m_setMatches.Erase (itWorst);
						}

						// If this match is close enough to the current
						// pixel-group, make a note of it.
						if (m_setMatches.Size() < m_nMatchCountThrottle)
						{
							m_setMatches.Insert (a_reStatus,
								MatchedPixelGroup (tnSAD, pMatch));
							if (a_reStatus != g_kNoError)
								return;
						}
					}

				#else // THROTTLE_PIXELSORTER_WITH_SAD

					// If a region with this motion vector doesn't already
					// intersect/border the current pixel-group, then
					// flood-fill it and add it to the search-border.
					if (!bExistingMatch)
					{
						// Add this region to the search-border.
						(void) SearchBorder_AddNewRegion (a_reStatus,
							tnMotionX, tnMotionY);
						if (a_reStatus != g_kNoError)
							return;
					}

				#endif // THROTTLE_PIXELSORTER_WITH_SAD
				}

			#ifdef THROTTLE_PIXELSORTER_WITH_SAD

				// Now loop through all the good matches found, flood-fill
				// each one, and add them to the search-border.
				for (itBestMatch = m_setMatches.Begin();
					 itBestMatch != m_setMatches.End();
					 ++itBestMatch)
				{
					PIXELINDEX tnMotionX, tnMotionY;
						// The motion vector of this match.

					// Get the current match.
					const MatchedPixelGroup &rMatch = *itBestMatch;

					// Calculate its motion vector.
					tnMotionX = rMatch.m_pGroup->m_tnX - m_tnX;
					tnMotionY = rMatch.m_pGroup->m_tnY - m_tnY;

					// Add this region to the search-border.
					(void) SearchBorder_AddNewRegion (a_reStatus,
						tnMotionX, tnMotionY);
					if (a_reStatus != g_kNoError)
						return;
				}

				// All done with the matches.
				m_setMatches.Clear();

			#endif // THROTTLE_PIXELSORTER_WITH_SAD

				// If it's time to throttle matches, do so.
				//
				// The idea is that matches are only throttled when the
				// match-count-throttle is exceeded, and the largest
				// matches are applied until the match-size-throttle is
				// satisfied.  Previously, matches could be throttled if
				// only the match-size-throttle was exceeded.  But this
				// led to bad matches getting applied before a better match
				// could be found, which led to an "earthquaking" sort of
				// artifact in dark, cloudy areas of the picture.  This
				// ended up being the simple fix for that problem!
				//
				// Now, the challenge is to pick a match-size-throttle that
				// keeps the denoiser running quickly without missing many
				// good matches, and a match-count-throttle that keeps the
				// number of candidates high enough to avoid the shaking
				// artifact described above.
				if (tnMatches >= m_nMatchCountThrottle
					/* || m_oSearchBorder.GetSizeOfLargestActiveRegion()
						>= m_nMatchSizeThrottle */)
				{
					// Get the largest active-region from the search-border
					// and apply it now.  Then, keep doing that as long
					// as the largets active-region exceeds the match-size-
					// throttle.
					// This allows at least one region to be applied if the
					// match-count-throttle is exceeded, and allows the
					// search-border to avoid doing expensive work for
					// regions that exceed the match-size-throttle.
					do
					{
						FRAMESIZE tnThrottledPixels;
						PIXELINDEX tnMotionX, tnMotionY;

						// Take the largest region that the current
						// pixel-group matched, flood-fill it, and apply it
						// to the new frame now, eliminating any competing
						// moved-regions encountered along the way.
						tnThrottledPixels = SearchBorder_MatchThrottle
							(a_reStatus, tnMatches, tnMotionX, tnMotionY);
						if (a_reStatus != g_kNoError)
							return;

						// HACK
						#if 0
						fprintf (stderr, "Match throttle: frame %d, "
							"x %03d, y %03d, %03d matches, "
							"%04d flood     \r", frame,
							int (m_tnX), int (m_tnY), int (tnMatches),
							int (tnThrottledPixels));
						#endif

						// That's more pixels found by match-throttling.
						if (tnMotionX == 0 && tnMotionY == 0)
							tnNotMovedThrottledPixels += tnThrottledPixels;
						else
							tnMovedThrottledPixels += tnThrottledPixels;
					} while (m_oSearchBorder.GetSizeOfLargestActiveRegion()
						>= m_nMatchSizeThrottle);
				}

				// If no matches were found for the current pixel-group,
				// and there are no active border-regions in the area,
				// then we've found new information.
				//
				// NOTE: this dramatically speeds up the analysis of
				// frames that contain mostly new info, but probably
				// impacts quality.  Flood-fills are allowed to override
				// the result, though.
				//
				// NOTE: We no longer test whether there are any active
				// border-regions in the area.  The logic is, they've been
				// flood-filled, and if they don't extend into the current
				// pixel-group, then it's safe to consider the current
				// pixel-group to be new information.
				else if (tnMatches == 0)
				{
					PIXELINDEX tnX, tnY;

					for (tnY = m_tnY; tnY < m_tnY + PGH; ++tnY)
					{
						for (tnX = m_tnX; tnX < m_tnX + PGW; ++tnX)
						{
							// Allocate a new reference pixel.
							ReferencePixel_t *pNewPixel
								= m_oPixelPool.Allocate();

							// Store the new pixel in the reference frame.
							m_pNewFrame->SetPixel (tnX, tnY, pNewPixel);

							// Give it the value from the new frame.
							pNewPixel->AddSample (a_pPixels[tnY
								* m_tnWidth + tnX]);
						}
					}
				}

nextGroup:
				// Move to the next pixel-group.
				if ((m_tnStepX == 1 && m_tnX == tnLastX)
				|| (m_tnStepX == -1 && m_tnX == 0))
				{
					// We need to move down a line.  If we're already on
					// the last line, we're done with the frame.
					if (m_tnY == tnLastY)
						break;

					// We should have found enough matches during this line
					// to safely apply all regions that exceed the
					// match-size-throttle.
					if (m_oSearchBorder.GetSizeOfLargestActiveRegion()
						>= m_nMatchSizeThrottle)
					{
						// Get the largest active-region from the
						// search-border and apply it now.  Then, keep
						// doing that as long as the largets active-region
						// exceeds the match-size-throttle.
						// This allows at least one region to be applied if
						// the match-count-throttle is exceeded, and allows
						// the search-border to avoid doing expensive work
						// for regions that exceed the match-size-throttle.
						tnMatches = 0;
						do
						{
							FRAMESIZE tnThrottledPixels;
							PIXELINDEX tnMotionX, tnMotionY;
	
							// Take the largest region that the current
							// pixel-group matched, flood-fill it, and
							// apply it to the new frame now, eliminating
							// any competing moved-regions encountered
							// along the way.
							tnThrottledPixels = SearchBorder_MatchThrottle
								(a_reStatus, tnMatches,
									tnMotionX, tnMotionY);
							if (a_reStatus != g_kNoError)
								return;
	
							// HACK
							#if 0
							fprintf (stderr, "Match throttle: frame %d, "
								"x %03d, y %03d, %03d matches, "
								"%04d flood     \r", frame,
								int (m_tnX), int (m_tnY), int (tnMatches),
								int (tnThrottledPixels));
							#endif
	
							// That's more pixels found by
							// match-throttling.
							if (tnMotionX == 0 && tnMotionY == 0)
								tnNotMovedThrottledPixels
									+= tnThrottledPixels;
							else
								tnMovedThrottledPixels
									+= tnThrottledPixels;
						} while
							(m_oSearchBorder.GetSizeOfLargestActiveRegion()
								>= m_nMatchSizeThrottle);
					}

					// Move down a line.
					m_oSearchWindow.MoveDown();
					m_oSearchBorder.MoveDown (a_reStatus);
					if (a_reStatus != g_kNoError)
						return;
					++m_tnY;

					// HACK
					//fprintf (stderr, "Now on line %d\r", int (m_tnY));

					// Now move across the frame in the other direction,
					// i.e. zigzag.
					m_tnStepX = -m_tnStepX;
				}
				else if (m_tnStepX == 1)
				{
					// Move right one pixel.
					m_oSearchWindow.MoveRight();
					m_oSearchBorder.MoveRight (a_reStatus);
					if (a_reStatus != g_kNoError)
						return;
					++m_tnX;
				}
				else
				{
					// Move left one pixel.
					assert (m_tnStepX == -1);
					m_oSearchWindow.MoveLeft();
					m_oSearchBorder.MoveLeft (a_reStatus);
					if (a_reStatus != g_kNoError)
						return;
					--m_tnX;
				}
			}

			// Get all the remaining moved-regions from the search-border.
			m_oSearchBorder.FinishFrame (a_reStatus);
			if (a_reStatus != g_kNoError)
				return;

			// We've found all the possible moved regions between the
			// new frame and the reference frame.
			// Loop through the moved regions found by motion-detection,
			// prune them of already-resolved pixels, and apply each one
			// to the new frame.
			//fprintf (stderr, "Applied extents:");	// HACK
			while (m_setRegions.Size() > 0)
			{
				typename MovedRegionSet::Iterator itRegion;
				MovedRegion *pRegion;
					// The moved region to apply next.
				PIXELINDEX tnMotionX, tnMotionY;
					// The region's motion vector.

				// Get the moved region to apply next.
				itRegion = m_setRegions.Begin();
				pRegion = *itRegion;
				m_setRegions.Erase (itRegion);
				pRegion->GetMotionVector (tnMotionX, tnMotionY);

				// Flood-fill the candidate region, re-testing all the
				// extents.  This removes parts that have been resolved
				// already.
				m_oPruningFloodFillControl.SetupForFloodFill
					(tnMotionX, tnMotionY);
				pRegion->FloodFill (a_reStatus,
					m_oPruningFloodFillControl, true, false);
				if (a_reStatus != g_kNoError)
					return;

				// If that makes it smaller than the next highest-
				// priority region we found, put it back in & try again.
				itRegion = m_setRegions.Begin();
				if (itRegion != m_setRegions.End()
				&& pRegion->NumberOfPoints()
					< (*itRegion)->NumberOfPoints())
				{
					// Are there enough points left in this region for
					// us to bother with it?
					if (pRegion->NumberOfPoints() < PGW * PGH)
					{
						// No.  Just get rid of it.
						m_oSearchBorder.DeleteRegion (pRegion);
					}
					else
					{
						// Yes.  Put the region back into our set, to be
						// tried later.
						#ifndef NDEBUG
						typename MovedRegionSet::InsertResult
							oInsertResult =
						#endif // !NDEBUG
							m_setRegions.Insert (a_reStatus, pRegion);
						if (a_reStatus != g_kNoError)
						{
							m_oSearchBorder.DeleteRegion (pRegion);
							return;
						}
						assert (oInsertResult.m_bInserted);
					}

					// Try the region that's now the highest priority.
					continue;
				}

				// Apply this region to the new frame.
				ApplyRegionToNewFrame (a_reStatus, *pRegion);
				if (a_reStatus != g_kNoError)
				{
					m_oSearchBorder.DeleteRegion (pRegion);
					return;
				}

				// That's more resolved pixels.
				if (tnMotionX == 0 && tnMotionY == 0)
					tnNotMovedPixels += pRegion->NumberOfPoints();
				else
					tnMovedPixels += pRegion->NumberOfPoints();

				// We're done with this region.
				m_oSearchBorder.DeleteRegion (pRegion);
			}
		}

		// Prepare for searching the next frame.
		m_oSearchWindow.FinishFrame();
	}

	// Motion-searching is done.  Loop through the reference frame's
	// pixels, find any unresolved pixels, and create a new pixel for
	// them, using the data in the new frame.
	for (i = 0; i < m_tnPixels; ++i)
	{
		ReferencePixel_t *pNewPixel;

		// If this pixel is still unresolved, give it the value of
		// the corresponding pixel in the new frame.
		pNewPixel = m_pNewFrame->GetPixel (i);
		if (pNewPixel == NULL)
		{
			// Allocate a new reference pixel.
			ReferencePixel_t *pNewPixel = m_oPixelPool.Allocate();

			// Store the new pixel in the reference frame.
			m_pNewFrame->SetPixel (i, pNewPixel);

			// Give it the value from the new frame.
			pNewPixel->AddSample (a_pPixels[i]);

			// That's one more new pixel.
			++tnNewPixels;
		}
		else if (pNewPixel != NULL
			&& pNewPixel->GetFrameReferences() == 1)
		{
			// Count up the earlier-found new pixel.  (It wasn't safe to
			// count them until flood-filling had a chance to override
			// this decision.)
			++tnNoMatchNewPixels;
		}
	}

	// Make sure all pixels were accounted for.
	// (This is a big sanity check.)
	assert (tnNotMovedZeroMotionPixels + tnNotMovedThrottledPixels
		+ tnNotMovedPixels + tnMovedThrottledPixels + tnMovedPixels
		+ tnNoMatchNewPixels + tnNewPixels == m_tnPixels);

	// All done.  Remember that the data in the new frame is now valid.
	++m_nLastFrame;

	// Make sure none of the pixels have more references than we have
	// frames.  (Sanity check.)
	#ifndef NDEBUG
	#ifdef USE_REFERENCEFRAMEPIXELS_ONCE
	for (i = 0; i < m_tnPixels; ++i)
	{
		int16_t nRefs = m_pNewFrame->GetPixel (i)->GetFrameReferences();
		assert (nRefs > 0);
		assert (nRefs <= m_nFrames);
	}
	#endif // USE_REFERENCEFRAMEPIXELS_ONCE
	#endif // !NDEBUG

	// We'll have a new reference-frame and new-frame in the next pass.
	m_pReferenceFrame = m_pNewFrame = NULL;
	m_pNewFramePixels = NULL;

	// HACK: print the pixel statistics.
	if (verbose >= 1)
	{
		float fInversePixelsPercent = 100.0f / float (m_tnPixels);

		fprintf (stderr, "Frame %d: %.1f%%+%.1f%%+%.1f%% not-moved, "
				"%.1f%%+%.1f%% moved, %.1f%%+%.1f%% new\n",
			frame,
			(float (tnNotMovedZeroMotionPixels) * fInversePixelsPercent),
			(float (tnNotMovedThrottledPixels) * fInversePixelsPercent),
			(float (tnNotMovedPixels) * fInversePixelsPercent),
			(float (tnMovedThrottledPixels) * fInversePixelsPercent),
			(float (tnMovedPixels) * fInversePixelsPercent),
			(float (tnNoMatchNewPixels) * fInversePixelsPercent),
			(float (tnNewPixels) * fInversePixelsPercent));
	}

	// Print the allocation totals.
	#ifndef NDEBUG
	fprintf (stderr, "%lu moved-regions, %lu pixel-sorters\n",
		(unsigned long) m_oSearchBorder.GetMovedRegionCount(),
		(unsigned long) m_oSearchWindow.GetPixelSorterNodeCount());
	#endif // !NDEBUG

	// Purge all remaining temporary memory allocations.
	m_oMatchThrottleRegion.Purge();
	#ifndef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oZeroMotionFloodFillControl.Purge();
	#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	#ifndef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oMatchThrottleFloodFillControl.Purge();
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	#ifndef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oPruningFloodFillControl.Purge();
	#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Make sure our temporary memory allocations have been purged.
	assert (m_oRegionAllocator.GetNumAllocated() == 0);
	assert (m_oMovedRegionSetAllocator.GetNumAllocated() == 0);
}



// Once there is no more input, call this repeatedly to get the
// details of the remaining frames, until it returns NULL.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
const typename MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,
	FRAMESIZE,PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ReferenceFrame_t *
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::GetRemainingFrames (void)
{
	ReferenceFrame_t *pFrame;
		// The frame to return to the caller.

	// If there are no frames left, let our caller know.
	if (m_nFirstFrame == m_nLastFrame)
		return NULL;

	// Get the frame to return to the caller.
	pFrame = m_ppFrames[m_nFirstFrame];

	// Remember not to hand it to the caller ever again.
	++m_nFirstFrame;

	// Finally, return the frame to our caller.
	return pFrame;
}



// Purge ourselves of temporary structures.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::Purge (void)
{
	// Clear out the pixel-sorter.
	m_oSearchWindow.PurgePixelSorter();
}



#ifdef THROTTLE_PIXELSORTER_WITH_SAD

// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::MatchedPixelGroup()
{
	// No match yet.
	m_tnSAD = 0;
	m_pGroup = NULL;
}



// Initializing constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::MatchedPixelGroup
	(Tolerance_t a_tnSAD,
	const typename SearchWindow_t::PixelGroup *a_pGroup)
: m_tnSAD (a_tnSAD), m_pGroup (a_pGroup)
{
	// Nothing else to do.
}



// Destructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::~MatchedPixelGroup()
{
	// Nothing to do.
}



// A comparison operator, suitable for Set<>.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
inline bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::SortBySAD::operator()
	(const MatchedPixelGroup &a_rLeft,
	const MatchedPixelGroup &a_rRight) const
{
	// Easy enough.
	return (a_rLeft.m_tnSAD < a_rRight.m_tnSAD);
}

#endif // THROTTLE_PIXELSORTER_WITH_SAD



// Apply this region to the new frame.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,REFERENCEFRAME>
	::ApplyRegionToNewFrame (Status_t &a_reStatus,
	const MovedRegion &a_rRegion)
{
	PIXELINDEX tnMotionX, tnMotionY;
		// The region's motion vector, i.e. the offset between the
		// new-frame pixel and the corresponding reference-frame pixel.
	typename MovedRegion::ConstIterator itExtent;
		// An extent to apply to the new frame.
	PIXELINDEX x;
		// Used to loop through pixels.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Get this region's motion vector.
	a_rRegion.GetMotionVector (tnMotionX, tnMotionY);

	// Loop through the region's extents, locate every pixel it
	// describes, and set it to the corresponding pixel in the
	// reference-frame representation of the new frame.
	for (itExtent = a_rRegion.Begin();
		 itExtent != a_rRegion.End();
		 ++itExtent)
	{
		// Get the next extent.
		const typename MovedRegion::Extent &rExtent = *itExtent;

		// Loop through the pixels it represents, set each
		// new-frame pixel to the corresponding reference-frame
		// pixel.
		for (x = rExtent.m_tnXStart; x < rExtent.m_tnXEnd; ++x)
		{
			#ifdef PRINT_SEARCHBORDER
			if (m_pNewFrame->GetPixel (x, rExtent.m_tnY) != NULL
				&& m_pNewFrame->GetPixel (x, rExtent.m_tnY)
					->GetFrameReferences() != 1)
			{
				fprintf (stderr, "Pixel (%d,%d) already resolved\n",
					int (x), int (rExtent.m_tnY));
			}
			#endif // PRINT_SEARCHBORDER

			// Make sure this new-frame pixel hasn't been
			// resolved yet.
			assert (m_pNewFrame->GetPixel (x, rExtent.m_tnY)
				== NULL
			|| m_pNewFrame->GetPixel (x, rExtent.m_tnY)
				->GetFrameReferences() == 1);

			// Get the corresponding reference-frame pixel.
			ReferencePixel_t *pReferencePixel
				= m_pReferenceFrame->GetPixel (x + tnMotionX,
					rExtent.m_tnY + tnMotionY);
			assert (pReferencePixel != NULL);

			// Set the new-frame pixel to this reference pixel.
			m_pNewFrame->SetPixel (x, rExtent.m_tnY,
				pReferencePixel);

			// Accumulate the new-frame value of this pixel.
			pReferencePixel->AddSample
				(m_pNewFramePixels[FRAMESIZE (rExtent.m_tnY)
				 * FRAMESIZE (m_tnWidth) + FRAMESIZE (x)]);
		}
	}

#ifdef USE_REFERENCEFRAMEPIXELS_ONCE

	// All of these reference pixels have been used.
	for (itExtent = a_rRegion.Begin();
		 itExtent != a_rRegion.End();
		 ++itExtent)
	{
		// Get the current extent.
		typename MovedRegion::Extent oExtent = *itExtent;

		// Move it along the motion vector.
		oExtent.m_tnY += tnMotionY;
		oExtent.m_tnXStart += tnMotionX;
		oExtent.m_tnXEnd += tnMotionX;

		// Make sure it's already in the frame.
		assert (oExtent.m_tnY >= 0 && oExtent.m_tnY < m_tnHeight
			&& oExtent.m_tnXStart >= 0 && oExtent.m_tnXEnd <= m_tnWidth);

		// Add it to our running total of used reference-pixels.
		m_oUsedReferencePixels.Union (a_reStatus, oExtent.m_tnY,
			oExtent.m_tnXStart, oExtent.m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	}

#endif // USE_REFERENCEFRAMEPIXELS_ONCE

	// Remove all pixel-groups containing used reference pixels from
	// the search window.
	m_oSearchWindow.Prune (a_rRegion, tnMotionX, tnMotionY);
}



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
		PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
		REFERENCEFRAME>::SearchBorder_t::SearchBorder_t
		(MovedRegionSet &a_rsetRegions,
			typename MovedRegion::Allocator &a_rAlloc)
	: BaseClass (a_rAlloc), m_rsetRegions (a_rsetRegions)
{
	// Nothing else to do.
}



// Receive a completed region from the search-border.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::SearchBorder_t::OnCompletedRegion
	(Status_t &a_reStatus,
	typename SearchBorder_t::MovedRegion *a_pRegion)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a completed region.
	assert (a_pRegion != NULL);

	// Put it in our list.
	if (a_pRegion->NumberOfPoints() < PGW * PGH)
	{
		// This region is too small to be bothered with.
		// Just get rid of it.
		this->DeleteRegion (a_pRegion);
	}
	else
	{
		#ifndef NDEBUG
		typename MovedRegionSet::InsertResult oInsertResult =
		#endif // !NDEBUG
			m_rsetRegions.Insert (a_reStatus, a_pRegion);
		if (a_reStatus != g_kNoError)
			return;
		assert (oInsertResult.m_bInserted);
	}
}



// Add a new region, with the given motion vector, to the
// search border.  Flood-fills its area before adding.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
FRAMESIZE
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::SearchBorder_AddNewRegion (Status_t &a_reStatus,
	PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY)
{
	PIXELINDEX tnY;
		// Used to loop through the pixel-group's lines.

	// Set up a region describing the current pixel-group.
	#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oFloodFillRegion.Clear();
	for (tnY = m_tnY; tnY < m_tnY + PGH; ++tnY)
	{
		m_oFloodFillRegion.Merge (a_reStatus, tnY, m_tnX,
			m_tnX + PGW);
		if (a_reStatus != g_kNoError)
			return 0;
	}
	#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oMatchThrottleRegion.Clear();
	for (tnY = m_tnY; tnY < m_tnY + PGH; ++tnY)
	{
		m_oMatchThrottleRegion.Merge (a_reStatus, tnY, m_tnX,
			m_tnX + PGW);
		if (a_reStatus != g_kNoError)
			return 0;
	}
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Set its motion vector.
	m_oMatchThrottleRegion.SetMotionVector (a_tnMotionX, a_tnMotionY);

	// Set its location.  (Needed only for debugging purposes.)
	#ifndef NDEBUG
	m_oMatchThrottleRegion.m_tnX = m_tnX;
	m_oMatchThrottleRegion.m_tnY = m_tnY;
	#endif // !NDEBUG

	// Flood-fill this match, so as to get its full extent.
	m_oMatchThrottleFloodFillControl.SetupForFloodFill (a_tnMotionX,
		a_tnMotionY);
	#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oFloodFillRegion.FloodFill (a_reStatus,
		m_oMatchThrottleFloodFillControl, false, true);
	#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oMatchThrottleRegion.FloodFill (a_reStatus,
		m_oMatchThrottleFloodFillControl, false, true);
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	if (a_reStatus != g_kNoError)
		return 0;

	// Now copy the results of the flood-fill to the match-throttle
	// region, so that it can be added to the search-border.
	#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	m_oMatchThrottleRegion.Clear();
	typename BitmapRegion_t::ConstIterator itExtent;
	for (itExtent = m_oFloodFillRegion.Begin();
		 itExtent != m_oFloodFillRegion.End();
		 ++itExtent)
	{
		// Get the current extent.
		const typename BitmapRegion_t::Extent &rExtent = *itExtent;

		// Copy it to the match-throttle region.
		m_oMatchThrottleRegion.Merge
			(a_reStatus, rExtent.m_tnY,
				rExtent.m_tnXStart, rExtent.m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return 0;
	}
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Get the size of the flood-filled region.
	FRAMESIZE tnThisMatch = m_oMatchThrottleRegion.NumberOfPoints();

	// Add this match to our growing set of moved regions.
	m_oSearchBorder.AddNewRegion (a_reStatus,
		m_oMatchThrottleRegion);
	if (a_reStatus != g_kNoError)
		return 0;

	// Make sure that emptied the region.
	// (AddNewRegion() is supposed to do that; this will catch any
	// changes in that behavior.)
	assert (m_oMatchThrottleRegion.NumberOfPoints() == 0);

	// Return the size of the flood-filled region.
	return tnThisMatch;
}



// Get the best region that matched the current pixel-group
// (which is usually the largest active-region).  Expand it as far
// as it can go, i.e. flood-fill in its area.  If it's no longer
// the largest region, put it back and try again.  Otherwise, apply
// it to the new frame now.
//
// Pass the number of pixel-group matches as an argument.  If a
// best-region candidate is found to have no unresolved pixels,
// it's no longer considered a pixel-group match, and that may lead
// to the discovery that the match-count-throttle is no longer
// being exceeded.
//
// Returns the number of points flood-filled, and backpatches the
// motion vector associated with the best region.
// May return zero if analysis reveals that the match-throttles
// weren't really exceeded.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
FRAMESIZE
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::SearchBorder_MatchThrottle (Status_t &a_reStatus,
	FRAMESIZE a_nMatchCount,
	PIXELINDEX &a_rtnMotionX, PIXELINDEX &a_rtnMotionY)
{
	MovedRegion *pSurvivor;
		// The only region to survive pruning -- the largest one, with
		// the shortest motion vector.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Keep testing best-active-region candidates until one really is the
	// best, or until the match-throttles are no longer being exceeded.
	while (m_oSearchBorder.GetSizeOfLargestActiveRegion() > 0)
	{
		// Get the best active region to apply to the frame now.
		// This also gives us the size of the second-best active-region.
		FRAMESIZE tnSecondBestActiveRegionSize = 0;
		pSurvivor = m_oSearchBorder.ChooseBestActiveRegion (a_reStatus,
			tnSecondBestActiveRegionSize);
		if (a_reStatus != g_kNoError)
			return 0;

		// Get the region's motion vector.
		pSurvivor->GetMotionVector (a_rtnMotionX, a_rtnMotionY);

		// Remove all pixels that were already resolved.
		m_oPruningFloodFillControl.SetupForFloodFill
			(a_rtnMotionX, a_rtnMotionY);
		pSurvivor->FloodFill (a_reStatus,
			m_oPruningFloodFillControl, true, false);
		if (a_reStatus != g_kNoError)
		{
			m_oSearchBorder.DeleteRegion (pSurvivor);
			return 0;
		}

		#ifdef PRINT_SEARCHBORDER
		if (frame == 61 && DIM == 2)
		{
			fprintf (stderr, "Flood-filled region:\n");
			PrintRegion (*pSurvivor);
			fprintf (stderr, "\n");
		}
		#endif // PRINT_SEARCHBORDER

		// Remember the number of points in the region.
		FRAMESIZE tnSurvivorSize = FRAMESIZE (pSurvivor->NumberOfPoints());

		// If this region has no unresolved pixels (or too few), try again.
		if (tnSurvivorSize < PGW * PGH)
		{
			// This region is no longer needed.
			m_oSearchBorder.DeleteRegion (pSurvivor);

			// Go back and try again.
			continue;
		}

		// If this region is no longer the best choice, put it back
		// and try again.
		if (tnSurvivorSize < tnSecondBestActiveRegionSize)
		{
			// Put it back.
			m_oSearchBorder.AddNewRegion (a_reStatus, *pSurvivor);
			m_oSearchBorder.DeleteRegion (pSurvivor);
			if (a_reStatus != g_kNoError)
				return 0;

			// If the match-size-throttle is no longer exceeded,
			// then let our caller know that no throttling was needed.
			if (tnSecondBestActiveRegionSize < m_nMatchSizeThrottle)
				return 0;

			// Try again.
			continue;
		}

		// Apply this region to the new frame.
		ApplyRegionToNewFrame (a_reStatus, *pSurvivor);
		if (a_reStatus != g_kNoError)
		{
			m_oSearchBorder.DeleteRegion (pSurvivor);
			return 0;
		}

		// Clean up the region, return the number of points it had.
		m_oSearchBorder.DeleteRegion (pSurvivor);
		return tnSurvivorSize;
	}

	// No region was applied to the frame.
	return 0;
}



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl
	#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	::ZeroMotionFloodFillControl()
	#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	::ZeroMotionFloodFillControl
		(typename BaseClass::Allocator &a_rAllocator)
	: BaseClass (a_rAllocator)
	#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// We don't know who we're working for yet.
	m_pMotionSearcher = NULL;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl::Init
	(Status_t &a_reStatus, MotionSearcher *a_pMotionSearcher)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a motion-searcher.
	assert (a_pMotionSearcher != NULL);

	// Initialize our base class.
	BaseClass::Init (a_reStatus
		#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		, a_pMotionSearcher->m_tnWidth,
		a_pMotionSearcher->m_tnHeight
		#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		);
	if (a_reStatus != g_kNoError)
		return;

	// Remember which motion-searcher we're working for.
	m_pMotionSearcher = a_pMotionSearcher;
}



// Return true if the flood-fill should examine the given
// extent.  May modify the extent.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl::ShouldUseExtent
	(typename ZeroMotionFloodFillControl::BaseClass::Extent &a_rExtent)
{
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Make sure the extent doesn't need to be clipped.
	assert (a_rExtent.m_tnY >= 0
		&& a_rExtent.m_tnY < m_pMotionSearcher->m_tnHeight
		&& a_rExtent.m_tnXStart >= 0
		&& a_rExtent.m_tnXStart < m_pMotionSearcher->m_tnWidth
		&& a_rExtent.m_tnXEnd > 0
		&& a_rExtent.m_tnXEnd <= m_pMotionSearcher->m_tnWidth);

#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

	// If this extent is completely off the screen, skip it.
	if (a_rExtent.m_tnY < 0
	|| a_rExtent.m_tnY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart >= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd <= 0)
		return false;

	// If this extent is partially off the screen, clip it.
	if (a_rExtent.m_tnXStart < 0)
		a_rExtent.m_tnXStart = 0;
	if (a_rExtent.m_tnXEnd > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth;

#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Let our caller know to use this extent.
	return true;
}



// Returns true if the given point should be included in the
// flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl::IsPointInRegion
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// If this pixel has been resolved, skip it.
	if (m_pMotionSearcher->m_pNewFrame->GetPixel (a_tnX, a_tnY) != NULL)
		return false;

	// Get the pixels of interest.
	const Pixel_t &rNewPixel = m_pMotionSearcher->m_pNewFramePixels
		[a_tnY * m_pMotionSearcher->m_tnWidth + a_tnX];
	ReferencePixel_t *pRefPixel
		= m_pMotionSearcher->m_pReferenceFrame->GetPixel (a_tnX, a_tnY);

	// If the new pixel is close enough to the reference pixel, the
	// point is in the region.
	if (rNewPixel.IsWithinTolerance
		(pRefPixel->GetValue(), m_pMotionSearcher->m_tnZeroTolerance))
	{
		// The point is in the region.
		return true;
	}

	// The point is not in the region.
	return false;
}



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl
	#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	::MatchThrottleFloodFillControl()
	#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	::MatchThrottleFloodFillControl
		(typename BaseClass::Allocator &a_rAllocator)
	: BaseClass (a_rAllocator)
	#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// We don't know who we're working for yet.
	m_pMotionSearcher = NULL;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::Init
	(Status_t &a_reStatus, MotionSearcher *a_pMotionSearcher)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a motion-searcher.
	assert (a_pMotionSearcher != NULL);

	// Initialize our base class.
	BaseClass::Init (a_reStatus
		#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		, a_pMotionSearcher->m_tnWidth,
		a_pMotionSearcher->m_tnHeight
		#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		);
	if (a_reStatus != g_kNoError)
		return;

	// Remember which motion-searcher we're working for.
	m_pMotionSearcher = a_pMotionSearcher;
}



// Set up to to a flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::SetupForFloodFill
	(PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY)
{
	// Save the motion vector.
	m_tnMotionX = a_tnMotionX;
	m_tnMotionY = a_tnMotionY;
}



// Return true if the flood-fill should examine the given
// extent.  May modify the extent.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::ShouldUseExtent
	(typename Region_t::Extent &a_rExtent)
{
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Make sure the extent doesn't need to be clipped.
	assert (a_rExtent.m_tnY >= 0
		&& a_rExtent.m_tnY < m_pMotionSearcher->m_tnHeight
		&& a_rExtent.m_tnXStart >= 0
		&& a_rExtent.m_tnXStart < m_pMotionSearcher->m_tnWidth
		&& a_rExtent.m_tnXEnd > 0
		&& a_rExtent.m_tnXEnd <= m_pMotionSearcher->m_tnWidth);

	// If this extent (with its motion vector) is completely off the
	// screen, skip it.
	if (a_rExtent.m_tnY + m_tnMotionY < 0
	|| a_rExtent.m_tnY + m_tnMotionY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart + m_tnMotionX >= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd + m_tnMotionX <= 0)
		return false;

	// If this extent (with its motion vector) is partially off the
	// screen, clip it.
	if (a_rExtent.m_tnXStart + m_tnMotionX < 0)
		a_rExtent.m_tnXStart = -m_tnMotionX;
	if (a_rExtent.m_tnXEnd + m_tnMotionX > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth - m_tnMotionX;

#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// If this extent is completely off the screen, skip it.
	if (a_rExtent.m_tnY < 0
	|| a_rExtent.m_tnY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart >= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd <= 0
	|| a_rExtent.m_tnY + m_tnMotionY < 0
	|| a_rExtent.m_tnY + m_tnMotionY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart + m_tnMotionX >= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd + m_tnMotionX <= 0)
		return false;

	// If this extent is partially off the screen, clip it.
	if (a_rExtent.m_tnXStart + m_tnMotionX < 0)
		a_rExtent.m_tnXStart = -m_tnMotionX;
	if (a_rExtent.m_tnXEnd + m_tnMotionX > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth - m_tnMotionX;
	if (a_rExtent.m_tnXStart < 0)
		a_rExtent.m_tnXStart = 0;
	if (a_rExtent.m_tnXEnd > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth;

#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Let our caller know to use this extent.
	return true;
}



// Returns true if the given point should be included in the
// flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::IsPointInRegion
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// Get the new pixel, if any.
	ReferencePixel_t *pNewPixel = m_pMotionSearcher->m_pNewFrame
		->GetPixel (a_tnX, a_tnY);

	// We can potentially flood-fill this pixel if it's unresolved or
	// if it thinks it's new data.
	if (pNewPixel == NULL || pNewPixel->GetFrameReferences() == 1)
	{
		// Get the reference pixel's coordinates.
		PIXELINDEX tnRefX = a_tnX + m_tnMotionX;
		PIXELINDEX tnRefY = a_tnY + m_tnMotionY;

		// If the corresponding reference pixel hasn't been
		// used already, see if it matches our pixel.
		#ifdef USE_REFERENCEFRAMEPIXELS_ONCE
		if (!m_pMotionSearcher->m_oUsedReferencePixels.DoesContainPoint
			(tnRefY, tnRefX))
		#endif // USE_REFERENCEFRAMEPIXELS_ONCE
		{
			// If the new pixel is close enough to it, the point is in
			// the region.
			ReferencePixel_t *pRefPixel
				= m_pMotionSearcher->m_pReferenceFrame->GetPixel
					(tnRefX, tnRefY);
			const Pixel_t &rNewPixel = m_pMotionSearcher
				->m_pNewFramePixels[a_tnY
					* m_pMotionSearcher->m_tnWidth + a_tnX];
			if (rNewPixel.IsWithinTolerance (pRefPixel->GetValue(),
				 m_pMotionSearcher->m_tnTolerance))
			{
				// Let our caller know the point is in the region.
				return true;
			}
		}
	}

	// Let our caller know the point is not in the region.
	return false;
}



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::PruningFloodFillControl
	#ifdef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
	::PruningFloodFillControl()
	#else // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
	::PruningFloodFillControl
		(typename BaseClass::Allocator &a_rAllocator)
	: BaseClass (a_rAllocator)
	#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// We don't know who we're working for yet.
	m_pMotionSearcher = NULL;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::PruningFloodFillControl::Init
	(Status_t &a_reStatus, MotionSearcher *a_pMotionSearcher)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a motion-searcher.
	assert (a_pMotionSearcher != NULL);

	// Initialize our base class.
	BaseClass::Init (a_reStatus
		#ifdef PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		, a_pMotionSearcher->m_tnWidth,
		a_pMotionSearcher->m_tnHeight
		#endif // PRUNING_FLOOD_FILL_WITH_BITMAP_REGIONS
		);
	if (a_reStatus != g_kNoError)
		return;

	// Remember which motion-searcher we're working for.
	m_pMotionSearcher = a_pMotionSearcher;
}



// Set up to to a flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::PruningFloodFillControl::SetupForFloodFill
	(PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY)
{
	// Save the motion vector.
	m_tnMotionX = a_tnMotionX;
	m_tnMotionY = a_tnMotionY;
}



// Return true if the flood-fill should examine the given
// extent.  May modify the extent.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::PruningFloodFillControl::ShouldUseExtent
	(typename Region_t::Extent &a_rExtent)
{
	// Make sure the extent doesn't need to be clipped.
	// (This style of flood-filling doesn't try to generate new extents
	// that would have to be filtered.)
	assert (a_rExtent.m_tnY >= 0
		&& a_rExtent.m_tnY < m_pMotionSearcher->m_tnHeight
		&& a_rExtent.m_tnXStart >= 0
		&& a_rExtent.m_tnXStart < m_pMotionSearcher->m_tnWidth
		&& a_rExtent.m_tnXEnd > 0
		&& a_rExtent.m_tnXEnd <= m_pMotionSearcher->m_tnWidth);

	// Let our caller know to use this extent.
	return true;
}



// Returns true if the given point should be included in the
// flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::PruningFloodFillControl::IsPointInRegion
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// Get the new pixel, if any.
	ReferencePixel_t *pNewPixel = m_pMotionSearcher->m_pNewFrame
		->GetPixel (a_tnX, a_tnY);

	// We can potentially flood-fill this pixel if it's unresolved or
	// if it thinks it's new data.
	if (pNewPixel == NULL || pNewPixel->GetFrameReferences() == 1)
	{
		// Get the reference pixel's coordinates.
		PIXELINDEX tnRefX = a_tnX + m_tnMotionX;
		PIXELINDEX tnRefY = a_tnY + m_tnMotionY;

		// If the corresponding reference pixel hasn't been
		// used already, it matches our pixel.
		#ifdef USE_REFERENCEFRAMEPIXELS_ONCE
		if (!m_pMotionSearcher->m_oUsedReferencePixels.DoesContainPoint
			(tnRefY, tnRefX))
		#endif // USE_REFERENCEFRAMEPIXELS_ONCE
		{
			// Let our caller know the point is in the region.
			return true;
		}
	}

	// Let our caller know the point is not in the region.
	return false;
}



#endif // __MOTION_SEARCHER_H__
