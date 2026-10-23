#ifndef __REFERENCE_FRAME_H__
#define __REFERENCE_FRAME_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#include "config.h"
#include <assert.h>
#include "mjpeg_types.h"
#include "Status_t.h"
#include "TemplateLib.hh"

// Definitions for pixels, reference pixels, pixel allocators, and
// reference frames.



// A generic pixel.
// Provide the numeric type used to store pixel values, the dimension of
// the pixel type, and the numeric type to use in tolerance
// calculations.
template <class NUM, int DIM, class TOL>
class Pixel
{
public:
	typedef NUM Num_t;
		// The numeric type used to store pixel values.

	Pixel() {}
		// Default constructor.

	Pixel (const NUM a_atnVal[DIM]);
		// Initializing constructor.

	Pixel (const Pixel<NUM,DIM,TOL> &a_rOther);
		// Copy constructor.

	NUM &operator[] (int a_nDim) { return m_atnVal[a_nDim]; }
	NUM operator[] (int a_nDim) const { return m_atnVal[a_nDim]; }
		// Get the value of this pixel.

	static TOL MakeTolerance (NUM a_tnTolerance);
		// Turn an integer tolerance value into what's appropriate for
		// the pixel type.
	
	bool IsWithinTolerance (const Pixel<NUM,DIM,TOL> &a_rOther,
			TOL a_tnTolerance) const;
		// Return true if the two pixels are within the specified
		// tolerance.
		// a_tnTolerance must have been previously retrieved from
		// MakeTolerance().
	
	bool IsWithinTolerance (const Pixel<NUM,DIM,TOL> &a_rOther,
			TOL a_tnTolerance, TOL &a_rtnSAD) const;
		// Return true if the two pixels are within the specified
		// tolerance, and backpatch the sample-array-difference.
		// a_tnTolerance must have been previously retrieved from
		// MakeTolerance().

private:
	NUM m_atnVal[DIM];
		// The pixel value.
};



// A reference pixel.  Used to accumulate values across several frames
// for any pixel deemed, via motion detection, to actually be the same
// pixel.
// Provide the numeric type for accumulated pixels (and tolerance
// calculations), the numeric type for pixels, the dimension of the
// pixel, and the class used to implement pixels.
template <class ACCUM_NUM, class PIXEL_NUM, int DIM,
	class PIXEL = Pixel<PIXEL_NUM,DIM,ACCUM_NUM> >
class ReferencePixel
{
public:
	typedef PIXEL Pixel_t;
		// Our pixel type.

	typedef ACCUM_NUM Tolerance_t;
		// Our tolerance value type.

	ReferencePixel();
		// Default constructor.

	~ReferencePixel();
		// Destructor.

	void Reset (void);
		// Reset ourselves, so that we may refer to a new pixel.

	void AddSample (const PIXEL &a_rPixel);
		// Incorporate another sample.

	const PIXEL &GetValue (void);
		// Return this pixel's value.

	void AddFrameReference (void) { ++m_nFrameReferences; }
		// Add another reference from a frame.

	void RemoveFrameReference (void) { --m_nFrameReferences; }
		// Remove an existing reference from a frame.
		// (Once all references are removed, the pixel is implicitly
		// unallocated.)

	int16_t GetFrameReferences (void) const
			{ return m_nFrameReferences; }
		// Return the number of frames that refer to this pixel.

private:
	ACCUM_NUM m_atSum[DIM];
		// The sum of the values of all pixels we've incorporated.

	ACCUM_NUM m_tCount;
		// The number of pixels we've incorporated.

	int16_t m_nFrameReferences;
		// The number of reference-frames that make use of this pixel.
		// A value of 0 means the pixel is not in use & therefore can
		// be allocated.

	PIXEL m_oPixel;
		// The value of this pixel, i.e. m_nSumY / m_nYCount.
		// Calculated on demand.
};



// A class to allocate reference pixels.
// Parameterized by the type of reference pixel, and a numeric type that
// can hold the largest number of reference pixels to be allocated (i.e.
// big enough to hold the product of the frame's width & height).
template <class REFERENCEPIXEL, class FRAMESIZE>
class PixelAllocator
{
public:
	PixelAllocator();
		// Default constructor.

	~PixelAllocator();
		// Destructor.

	void Initialize (Status_t &a_reStatus, FRAMESIZE a_tnCount);
		// Initialize the pool to contain the given number of pixels.
		// This must be greater than or equal to the number of pixels
		// that can ever be in use at one time.

	REFERENCEPIXEL *Allocate (void);
		// Allocate another pixel.
		// (Note that not having a free pixel to allocate indicates
		// that the a_nCount parameter to Initialize() needed to be
		// bigger.)

private:
	FRAMESIZE m_tnCount;
		// The number of reference pixels in our pool.

	FRAMESIZE m_tnNext;
		// The index of the next pixel to try to allocate.
		// A pixel is considered unallocated if it has a zero
		// reference count.

	REFERENCEPIXEL *m_pPixels;
		// The pool of pixels.
};



// A reference frame.  Points to reference pixels, which may be shared
// across several reference frames, depending on what motion-detection
// determines about the pixel's lifetime.  Parameterized by the type of
// reference pixels to use, the numeric type to use for pixel indices,
// and a numeric type big enough to hold the product of the largest
// expected width & height.
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
class ReferenceFrame
{
public:
	ReferenceFrame (Status_t &a_reStatus, PIXELINDEX a_tnWidth,
			PIXELINDEX a_tnHeight);
		// Initializing constructor.

	REFERENCEPIXEL *GetPixel (PIXELINDEX a_tnX, PIXELINDEX a_tnY) const;
		// Get the pixel at this index (which may be NULL).

	REFERENCEPIXEL *GetPixel (FRAMESIZE a_tnI) const;
		// Get the pixel at this offset (which may be NULL).

	void SetPixel (PIXELINDEX a_tnX, PIXELINDEX a_tnY,
			REFERENCEPIXEL *a_pPixel);
		// Set the pixel at this index.  a_pPixel may be NULL.

	void SetPixel (FRAMESIZE a_tnI, REFERENCEPIXEL *a_pPixel);
		// Set the pixel at this offset.  a_pPixel may be NULL.

	void Reset (void);
		// Reset the frame, i.e. set all the pixels to NULL.

private:
	PIXELINDEX m_tnWidth, m_tnHeight;
		// The dimensions of the frame.

	REFERENCEPIXEL **m_ppPixels;
		// The reference pixels that make up this frame.
		// The contained pointers may be NULL, to mark pixels that still
		// need to be resolved.
};



// Initializing constructor.
template <class NUM, int DIM, class TOL>
Pixel<NUM,DIM,TOL>::Pixel (const NUM a_atnVal[DIM])
{
	// Store the values.
	for (int i = 0; i < DIM; ++i)
		m_atnVal[i] = a_atnVal[i];
}



// Copy constructor.
template <class NUM, int DIM, class TOL>
Pixel<NUM,DIM,TOL>::Pixel (const Pixel<NUM,DIM,TOL> &a_rOther)
{
	// Copy the values.
	for (int i = 0; i < DIM; ++i)
		m_atnVal[i] = a_rOther.m_atnVal[i];
}



// Turn an integer tolerance value into what's appropriate for
// the pixel type.
template <class NUM, int DIM, class TOL>
TOL
Pixel<NUM,DIM,TOL>::MakeTolerance (NUM a_tnTolerance)
{
	// The default is to just use the given number.
	return TOL (a_tnTolerance);
}



#if 0

// This is what I mean, but C++ templates can't do this.

// Turn an integer tolerance value into what's appropriate for
// the pixel type.
template <class NUM, class TOL>
TOL
Pixel<NUM,1,TOL>::MakeTolerance (NUM a_tnTolerance)
{
	// For a one-dimensional pixel, just use the given number.
	return TOL (a_tnTolerance);
}

#endif



#if 0

// This is what I mean, but C++ templates can't do this.

// Turn an integer tolerance value into what's appropriate for
// the pixel type.
template <class NUM, class TOL>
TOL
Pixel<NUM,2,TOL>::MakeTolerance (NUM a_tnTolerance)
{
	// For a two-dimensional pixel, use the square of the given number.
	return TOL (a_tnTolerance) * TOL (a_tnTolerance);
}

#endif

	

#if 0

// This is what I mean, but C++ templates can't do this.

// Return true if the two pixels are within the specified tolerance.
template <class NUM, class TOL>
bool
Pixel<NUM,1,TOL>::IsWithinTolerance
	(const Pixel<NUM,1,TOL> &a_rOther, TOL a_tnTolerance) const
{
	// Check to see if the absolute value of the difference between
	// the two pixels is within our tolerance value.
	return AbsoluteValue (TOL (m_atnVal[0])
		- TOL (a_rOther.m_atnVal[0])) <= a_tnTolerance;
}

#endif



#if 0

// This is what I mean, but C++ templates can't do this.

// Return true if the two pixels are within the specified tolerance.
template <class NUM, class TOL>
bool
Pixel<NUM,2,TOL>::IsWithinTolerance
	(const Pixel<NUM,2,TOL> &a_rOther, TOL a_tnTolerance) const
{
	// Calculate the vector difference between the two pixels.
	TOL tnX = TOL (m_atnVal[0]) - TOL (a_rOther.m_atnVal[0]);
	TOL tnY = TOL (m_atnVal[1]) - TOL (a_rOther.m_atnVal[1]);

	// Check to see if the length of the vector difference is within
	// our tolerance value.  (Technically, we check the squares of
	// the values, but that's just as valid & much faster than
	// calculating a square root.)
	return tnX * tnX + tnY * tnY <= a_tnTolerance;
}

#endif



// Default constructor.
template <class ACCUM_NUM, class PIXEL_NUM, int DIM, class PIXEL>
ReferencePixel<ACCUM_NUM,PIXEL_NUM,DIM,PIXEL>::ReferencePixel()
{
	// Reset the pixel value.
	Reset();

	// No references from frames yet.
	m_nFrameReferences = 0;
}



// Destructor.
template <class ACCUM_NUM, class PIXEL_NUM, int DIM, class PIXEL>
ReferencePixel<ACCUM_NUM,PIXEL_NUM,DIM,PIXEL>::~ReferencePixel()
{
	// Make sure no frames refer to us.
	assert (m_nFrameReferences == 0);
}



// Reset ourselves, so that we may refer to a new pixel.
template <class ACCUM_NUM, class PIXEL_NUM, int DIM, class PIXEL>
void
ReferencePixel<ACCUM_NUM,PIXEL_NUM,DIM,PIXEL>::Reset (void)
{
	// Reset the accumulated sum, and get rid of any existing pixel value.
	for (int i = 0; i < DIM; i++)
	{
		m_atSum[i] = ACCUM_NUM (0);
		m_oPixel[i] = PIXEL_NUM (0);
	}
	m_tCount = ACCUM_NUM (0);
}



// Incorporate another sample.
template <class ACCUM_NUM, class PIXEL_NUM, int DIM, class PIXEL>
void
ReferencePixel<ACCUM_NUM,PIXEL_NUM,DIM,PIXEL>::AddSample
	(const PIXEL &a_rPixel)
{
	// Make sure this pixel is in use.
	assert (m_nFrameReferences > 0);

	// If the number of samples is getting close to causing overflow
	// problems, deal with it by losing a little resolution.
	// (These divides should turn into bit-shifts with integer types,
	// and therefore not be a performance issue.)
	if (m_tCount >= 10 /* HACK (Limits<ACCUM_NUM>::Max / ACCUM_NUM (2)) */)
	{
		for (int i = 0; i < DIM; i++)
			m_atSum[i] /= ACCUM_NUM (2);
		m_tCount /= ACCUM_NUM (2);
	}

	// Add the value to our accumulated sum.
	for (int i = 0; i < DIM; i++)
		m_atSum[i] += a_rPixel[i];
	++m_tCount;

	// Recalculate the pixel value.
	{
		static float afDivisors[] = { 0.0f, 1.0f, 1.0f / 2.0f,
				1.0f / 3.0f, 1.0f / 4.0f, 1.0f / 5.0f, 1.0f / 6.0f,
				1.0f / 7.0f, 1.0f / 8.0f, 1.0f / 9.0f, 1.0f / 10.0f };
			// HACK: try to speed this up.

		// Calculate the pixel's value.
		if (m_tCount <= 10)
		{
			for (int i = 0; i < DIM; i++)
				m_oPixel[i] = PIXEL_NUM ((float (m_atSum[i])
					* afDivisors[m_tCount]) + 0.5f);
		}
		else
		{
			for (int i = 0; i < DIM; i++)
				m_oPixel[i] = PIXEL_NUM ((float (m_atSum[i])
					/ float (m_tCount)) + 0.5f);
		}
	}
}



// Return the pixel's value.
template <class ACCUM_NUM, class PIXEL_NUM, int DIM, class PIXEL>
const PIXEL &
ReferencePixel<ACCUM_NUM,PIXEL_NUM,DIM,PIXEL>::GetValue (void)
{
	// Easy enough.
	return m_oPixel;
}



// Default constructor.
template <class REFERENCEPIXEL, class FRAMESIZE>
PixelAllocator<REFERENCEPIXEL,FRAMESIZE>::PixelAllocator()
{
	// No pixels yet.
	m_tnCount = m_tnNext = 0;
	m_pPixels = NULL;
}



// Destructor.
template <class REFERENCEPIXEL, class FRAMESIZE>
PixelAllocator<REFERENCEPIXEL,FRAMESIZE>::~PixelAllocator()
{
	// Free up the pixel pool.  (The pixels themselves will verify that
	// there are no more references to them.)
	delete[] m_pPixels;
}



// Initialize the pool to contain the given number of pixels.
template <class REFERENCEPIXEL, class FRAMESIZE>
void
PixelAllocator<REFERENCEPIXEL,FRAMESIZE>::Initialize
	(Status_t &a_reStatus, FRAMESIZE a_tnCount)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we haven't been initialized already.
	assert (m_pPixels == NULL);

	// Try to allocate a pool of the given number of pixels.
	m_pPixels = new REFERENCEPIXEL[a_tnCount];
	if (m_pPixels == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}

	// Remember that we allocated this many pixels.
	m_tnCount = a_tnCount;

	// Allocate the first pixel first.
	m_tnNext = 0;
}



// Allocate another pixel.
template <class REFERENCEPIXEL, class FRAMESIZE>
REFERENCEPIXEL *
PixelAllocator<REFERENCEPIXEL,FRAMESIZE>::Allocate (void)
{
	FRAMESIZE tnOrigNext;
		// The original value of the index of the next pixel to
		// allocate.  Used to detect when we've run through all of
		// them.
	REFERENCEPIXEL *pPixel;
		// The pixel we allocate.

	// Loop through the pixel pool, find an unallocated one, return it
	// to them.
	tnOrigNext = m_tnNext;
	for (;;)
	{
		// Get the next pixel.
		pPixel = m_pPixels + m_tnNext;
		//m_tnNext = (m_tnNext + 1) % m_tnCount;
		++m_tnNext;		// (faster...no divide)
		assert (m_tnNext <= m_tnCount);
		if (m_tnNext == m_tnCount)
			m_tnNext = 0;

		// If this pixel is unallocated, reset it & return it.
		if (pPixel->GetFrameReferences() == 0)
		{
			pPixel->Reset();
			return pPixel;
		}

		// Make sure we haven't run out of pixels.  (Returning NULL
		// may cause a segmentation fault in our client, but that's
		// slightly better than an infinite loop.)
		if (m_tnNext == tnOrigNext)
		{
			assert (false);
			return NULL;
		}
	}
}



// Initializing constructor.
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE>::ReferenceFrame
	(Status_t &a_reStatus, PIXELINDEX a_tnWidth, PIXELINDEX a_tnHeight)
{
	FRAMESIZE tnPixels;
		// The total number of pixels referred to by this frame.
	FRAMESIZE i;
		// Used to loop through pixel references.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the height and width are sane.
	assert (a_tnWidth > 0);
	assert (a_tnHeight > 0);

	// Allocate space for the frame's pixels.
	tnPixels = FRAMESIZE (a_tnWidth) * FRAMESIZE (a_tnHeight);
	m_ppPixels = new REFERENCEPIXEL * [tnPixels];
	if (m_ppPixels == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}

	// Remember our dimensions.
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;

	// Initially, no pixels are referred to by the frame.
	for (i = 0; i < tnPixels; i++)
		m_ppPixels[i] = NULL;
}



// Get the pixel at this index (which may be NULL).
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
REFERENCEPIXEL *
ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE>::GetPixel
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY) const
{
	// Make sure the indices are within limits.
	assert (a_tnX >= PIXELINDEX (0) && a_tnX < m_tnWidth);
	assert (a_tnY >= PIXELINDEX (0) && a_tnY < m_tnHeight);

	// Easy enough.
	return m_ppPixels[FRAMESIZE (a_tnY) * FRAMESIZE (m_tnWidth)
		+ FRAMESIZE (a_tnX)];
}



// Get the pixel at this offset (which may be NULL).
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
REFERENCEPIXEL *
ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE>::GetPixel
	(FRAMESIZE a_tnI) const
{
	// Make sure the offset is within limits.
	assert (a_tnI >= FRAMESIZE (0)
		&& a_tnI < FRAMESIZE (m_tnWidth) * FRAMESIZE (m_tnHeight));

	// Easy enough.
	return m_ppPixels[a_tnI];
}



// Set the pixel at this index.  a_pPixel may be NULL.
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
void
ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE>::SetPixel
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY, REFERENCEPIXEL *a_pPixel)
{
	// Make sure the indices are within limits.
	assert (a_tnX >= PIXELINDEX (0) && a_tnX < m_tnWidth);
	assert (a_tnY >= PIXELINDEX (0) && a_tnY < m_tnHeight);

	// Get the pixel of interest.
	REFERENCEPIXEL *&rpPixel = m_ppPixels[FRAMESIZE (a_tnY)
		* FRAMESIZE (m_tnWidth) + FRAMESIZE (a_tnX)];

	// If there's a pixel here already, remove our reference to it.
	if (rpPixel != NULL)
		rpPixel->RemoveFrameReference();

	// Store the new pixel here.
	rpPixel = a_pPixel;

	// If they stored a valid pixel, add our reference to it.
	if (rpPixel != NULL)
		rpPixel->AddFrameReference();
}



// Set the pixel at this offset.  a_pPixel may be NULL.
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
void
ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE>::SetPixel
	(FRAMESIZE a_tnI, REFERENCEPIXEL *a_pPixel)
{
	// Make sure the offset is within limits.
	assert (a_tnI >= FRAMESIZE (0)
		&& a_tnI < FRAMESIZE (m_tnWidth) * FRAMESIZE (m_tnHeight));

	// Get the pixel of interest.
	REFERENCEPIXEL *&rpPixel = m_ppPixels[a_tnI];

	// If there's a pixel here already, remove our reference to it.
	if (rpPixel != NULL)
		rpPixel->RemoveFrameReference();

	// Store the new pixel here.
	rpPixel = a_pPixel;

	// If they stored a valid pixel, add our reference to it.
	if (rpPixel != NULL)
		rpPixel->AddFrameReference();
}



// Reset the frame (i.e. set all the pixels to NULL).
template <class REFERENCEPIXEL, class PIXELINDEX, class FRAMESIZE>
void
ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE>::Reset (void)
{
	FRAMESIZE tnPixels;
		// The total number of pixels referred to by this frame.
	FRAMESIZE i;
		// Used to loop through pixels.

	// Loop through all pixels, set them to NULL.
	tnPixels = FRAMESIZE (m_tnWidth) * FRAMESIZE (m_tnHeight);
	for (i = 0; i < tnPixels; ++i)
	{
		// If there's a pixel here already, remove our reference to it,
		// then remove the pixel.
		if (m_ppPixels[i] != NULL)
		{
			m_ppPixels[i]->RemoveFrameReference();
			m_ppPixels[i] = NULL;
		}
	}
}



#endif // __REFERENCE_FRAME_H__
