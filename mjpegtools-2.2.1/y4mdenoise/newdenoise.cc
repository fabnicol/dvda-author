// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "mjpeg_types.h"
#include "mjpeg_logging.h"
#include "yuv4mpeg.h"
#include <stdio.h>
#include "newdenoise.hh"
#include "MotionSearcher.hh"

// Pixel types we use.
typedef Pixel<uint8_t,1,int32_t> PixelY;
typedef Pixel<uint8_t,2,int32_t> PixelCbCr;

// Reference-pixel types we use.
typedef ReferencePixel<uint16_t,uint8_t,1,PixelY> ReferencePixelY;
typedef ReferencePixel<uint16_t,uint8_t,2,PixelCbCr> ReferencePixelCbCr;

// Reference-frame types we use.
typedef ReferenceFrame<ReferencePixelY, int16_t, int32_t>
	ReferenceFrameY;
typedef ReferenceFrame<ReferencePixelCbCr, int16_t, int32_t>
	ReferenceFrameCbCr;

// The denoisers.  (We have to make these classes, in order to keep gdb
// from crashing.  I didn't even know one could crash gdb. ;-)
#if 0
typedef MotionSearcher<uint8_t, 1, int32_t, int16_t, int32_t, 4, 2,
		uint16_t, PixelY, ReferencePixelY, ReferenceFrameY>
	MotionSearcherY;
typedef MotionSearcher<uint8_t, 2, int32_t, int16_t, int32_t, 2, 2,
		uint16_t, PixelCbCr, ReferencePixelCbCr, ReferenceFrameCbCr>
	MotionSearcherCbCr;
#else
class MotionSearcherY
	: public MotionSearcher<uint8_t, 1, int32_t, int16_t, int32_t, 4, 2,
		uint16_t, PixelY, ReferencePixelY, ReferenceFrameY> {};
class MotionSearcherCbCr
	: public MotionSearcher<uint8_t, 2, int32_t, int16_t, int32_t, 2, 2,
		uint16_t, PixelCbCr, ReferencePixelCbCr, ReferenceFrameCbCr> {};
#endif
MotionSearcherY g_oMotionSearcherY;
MotionSearcherCbCr g_oMotionSearcherCbCr;

// Whether the denoisers should be used.
bool g_bMotionSearcherY;
bool g_bMotionSearcherCbCr;

// Pixel buffers, used to translate provided input into the form the
// denoiser needs.
int g_nPixelsY, g_nWidthY, g_nHeightY;
MotionSearcherY::Pixel_t *g_pPixelsY;
int g_nPixelsCbCr, g_nWidthCbCr, g_nHeightCbCr;
MotionSearcherCbCr::Pixel_t *g_pPixelsCbCr;

// Internal methods to output a frame.
static void output_frame
	(const MotionSearcherY::ReferenceFrame_t *a_pFrameY,
	const MotionSearcherCbCr::ReferenceFrame_t *a_pFrameCbCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr);
static void output_field
	(int a_nMask, const MotionSearcherY::ReferenceFrame_t *a_pFrameY,
	const MotionSearcherCbCr::ReferenceFrame_t *a_pFrameCbCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr);



// A class wrapper for mutexes.
class ThreadMutex
{
public:
	ThreadMutex();
		// Default constructor.
	
	~ThreadMutex();
		// Destructor.
	
	void Lock (void);
		// Lock the mutex.
	
	void Unlock (void);
		// Unlock the mutex.
	
	operator pthread_mutex_t * (void) { return &m_oMutex; }
	operator const pthread_mutex_t * (void) const { return &m_oMutex; }
		// Accessor.

private:
	pthread_mutex_t m_oMutex;
		// The wrapped mutex.

#ifndef NDEBUG
public:
	bool m_bLocked;
		// true if the mutex is locked.
#endif // NDEBUG
};



// A class to communicate conditions between threads.
class ThreadCondition
{
public:
	ThreadCondition (ThreadMutex &a_rMutex);
		// Default constructor.
	
	~ThreadCondition();
		// Destructor.
	
	void Signal (void);
		// Signal the condition.
		// The mutex must be locked to call this.
	
	void Wait (void);
		// Wait for the condition to be signaled.
		// The mutex must be locked to call this.

private:
	ThreadMutex &m_rMutex;
		// Provides mutual exclusion for the condition.
	pthread_cond_t m_oCondition;
		// The way the condition gets signaled if another thread is
		// waiting for it.  Protected by the mutex.
#ifndef NDEBUG
	bool m_bSignaled;
		// Whether or not a condition has been signaled.
#endif // NDEBUG
};

// A basic input/output-oriented thread.
class BasicThread
{
public:
	BasicThread();
		// Default constructor.
	
	virtual ~BasicThread();
		// Destructor.
	
	void ForceShutdown (void);
		// Force the thread to terminate.

protected:
	virtual int Work (void) = 0;
		// Do some work.
		// Must be implemented by the subclass.

	void Initialize (void);
		// Start the thread.
		// Called by the subclass' Initialize() method.
	
	void Lock (void);
		// Lock the mutex that guards the conditions.
	
	void Unlock (void);
		// Unlock the mutex that guards the conditions.

#ifndef NDEBUG

	bool IsLocked (void) const;
		// True if the mutex is locked.

#endif // !NDEBUG

	void SignalInput (void);
		// Signal that input has been provided to the thread.
	
	void WaitForInput (void);
		// Wait for input to be provided.
	
	void SignalOutput (void);
		// Signal that output has been provided to the thread.
	
	void WaitForOutput (void);
		// Wait for output to be provided.
	
	void Shutdown (void);
		// Stop the thread.

	pthread_t m_oThreadInfo;
		// OS-level thread state.
	
private:
	ThreadMutex m_oMutex;
		// A mutex to guard general shared access to the thread's
		// data, as well as the following conditions.

	ThreadCondition m_oInputCondition, m_oOutputCondition;
		// Conditions to signal input-ready and output-ready.
	
	static void *WorkLoop (void *a_pThread);
		// The thread function stub.  Calls the virtual WorkLoop().

public:
	void WorkLoopExit (void);
		// Clean up after WorkLoop() exits.
		// To be called only by the static WorkLoop().
	
protected:
	bool m_bWaitingForInput, m_bWaitingForOutput;
		// true if we're waiting for input/output (i.e. if they need
		// to be signaled).

	bool m_bWorkLoop;
		// true if WorkLoop() should continue running, false if it
		// should exit.
	
	virtual void WorkLoop (void);
		// The thread function.  Loop, calling Work(), until told to
		// stop.

	int m_nWorkRetval;
		// The value returned by Work().
};

// A class to run denoisers in a separate thread.
class DenoiserThread : public BasicThread
{
private:
	typedef BasicThread BaseClass;
		// Keep track of who our base class is.

public:
	DenoiserThread();
		// Default constructor.
	
	virtual ~DenoiserThread();
		// Destructor.

	// Prototypes for methods that subclasses must implement.

	//void Initialize (subclass parameters);
		// Initialize.  Set up all private thread data and start the
		// worker thread.
	
	//void AddFrame (subclass parameters);
		// Add a frame to the denoiser.
	
	//void WaitForAddFrame (subclass parameters);
		// Wait for the frame to be added & output possibly generated.
	
	//void Shutdown (subclass parameters);
		// Shut down the worker thread.

protected:

	virtual void WorkLoop (void);
		// The thread function.  Loop, calling Work(), until told to
		// stop.
	
	void AddFrame (void);
		// Add a frame to the denoiser.
		// Te be called by the subclass' AddFrame().
	
	void WaitForAddFrame (void);
		// Wait for the frame to be added & output possibly generated.
		// Te be called by the subclass' WaitForAddFrame().

private:
	enum { m_kWaitingForFrame, m_kGivenFrame, m_kFinishedFrame };
	int m_eWorkStatus;
		// Where we are in the process of denoising.
};

// A class to run the intensity denoiser in a separate thread.
class DenoiserThreadY : public DenoiserThread
{
private:
	typedef DenoiserThread BaseClass;
		// Keep track of who our base class is.

public:
	DenoiserThreadY();
		// Default constructor.
	
	virtual ~DenoiserThreadY();
		// Destructor.

	void Initialize (void);
		// Initialize.  Set up all private thread data and start the
		// worker thread.
	
	void AddFrame (const uint8_t *a_pInputY, uint8_t *a_pOutputY);
		// Add a frame to the denoiser.
	
	int WaitForAddFrame (void);
		// Get the next denoised frame, if any.
		// Returns the result of Work().

protected:
	virtual int Work (void);
		// Denoise the current frame.

private:
	const uint8_t *m_pInputY;
	uint8_t *m_pOutputY;
		// Input/output buffers.
};

// A class to run the color denoiser in a separate thread.
class DenoiserThreadCbCr : public DenoiserThread
{
private:
	typedef DenoiserThread BaseClass;
		// Keep track of who our base class is.

public:
	DenoiserThreadCbCr();
		// Default constructor.
	
	virtual ~DenoiserThreadCbCr();
		// Destructor.

	void Initialize (void);
		// Initialize.  Set up all private thread data and start the
		// worker thread.
	
	void AddFrame (const uint8_t *a_pInputCb,
			const uint8_t *a_pInputCr, uint8_t *a_pOutputCb,
			uint8_t *a_pOutputCr);
		// Add a frame to the denoiser.
	
	int WaitForAddFrame (void);
		// Get the next denoised frame, if any.
		// Returns the result of Work().

protected:
	virtual int Work (void);
		// Denoise the current frame.

private:
	const uint8_t *m_pInputCb;
	const uint8_t *m_pInputCr;
	uint8_t *m_pOutputCb;
	uint8_t *m_pOutputCr;
		// Input/output buffers.
};

// A class to read/write raw-video in a separate thread.
class ReadWriteThread : public BasicThread
{
private:
	typedef BasicThread BaseClass;
		// Keep track of who our base class is.

public:
	ReadWriteThread();
		// Default constructor.
	
	virtual ~ReadWriteThread();
		// Destructor.

	void Initialize (int a_nFD, const y4m_stream_info_t *a_pStreamInfo,
			y4m_frame_info_t *a_pFrameInfo, int a_nWidthY,
			int a_nHeightY, int a_nWidthCbCr, int a_nHeightCbCr);
		// Start the thread.

protected:
	int m_nFD;
		// The file descriptor from which to read/write raw video.
	const y4m_stream_info_t *m_pStreamInfo;
		// Information on the raw video stream.
	y4m_frame_info_t *m_pFrameInfo;
		// Information on the current frame in the raw video stream.
	struct Frame { uint8_t *planes[3]; Frame *next; };
		// The type of a frame.
	Frame *m_apFrames;
		// Space for frames being read from input.
	enum { m_kNumFrames = 4 };
		// The number of frames to allocate.
	Frame *m_pValidFramesHead, *m_pValidFramesTail, *m_pCurrentFrame,
			*m_pFreeFramesHead;
		// A list of frames containing data, the current frame, and
		// a list of free frames.
	
	Frame *GetFirstValidFrame (void);
		// Remove the first valid frame from the list and return it.

	void AddFrameToValidList (Frame *a_pFrame);
		// Add the given frame to the end of the valid-frame list.

	Frame *GetFreeFrame (void);
		// Remove a frame from the free-frames list and return it.

	void AddFrameToFreeList (Frame *a_pFrame);
		// Add the given frame to the free-frames list.

	void MoveValidFrameToCurrent (void);
		// Remove the first valid frame, and make it the current frame.
	
	void MoveCurrentFrameToValidList (void);
		// Move the current frame to the end of the valid-frame list.
	
	void MoveFreeFrameToCurrent (void);
		// Remove a frame from the free list, and make it the current
		// frame.
	
	void MoveCurrentFrameToFreeList (void);
		// Move the current frame to the free-frame list.
};

// A class to read raw-video in a separate thread.
class DenoiserThreadRead : public ReadWriteThread
{
private:
	typedef ReadWriteThread BaseClass;
		// Keep track of who our base class is.

public:
	DenoiserThreadRead();
		// Default constructor.
	
	virtual ~DenoiserThreadRead();
		// Destructor.

	int ReadFrame (uint8_t **a_apPlanes);
		// Read a frame from input.  a_apPlanes[] gets backpatched
		// with pointers to valid frame data, and they are valid until
		// the next call to ReadFrame().
		// Returns Y4M_OK if it succeeds, Y4M_ERR_EOF at the end of
		// the stream.  (Returns other errors too.)

protected:
	virtual int Work (void);
		// Read frames from the raw-video stream.
};

// A class to write raw-video in a separate thread.
class DenoiserThreadWrite : public ReadWriteThread
{
private:
	typedef ReadWriteThread BaseClass;
		// Keep track of who our base class is.

public:
	DenoiserThreadWrite();
		// Default constructor.
	
	virtual ~DenoiserThreadWrite();
		// Destructor.

	int GetSpaceToWriteFrame (uint8_t **a_apPlanes);
		// Get space for a frame to write to output.  a_apPlanes[] gets
		// backpatched with pointers to valid frame data.
		// Returns Y4M_OK if it succeeds, something else if it fails.
	
	void WriteFrame (void);
		// Write a frame to output.  The a_apPlanes[] previously set up
		// by GetSpaceToWriteFrame() must be filled with video data by
		// the client.

protected:
	virtual void WorkLoop (void);
		// The thread function.  Loop, calling Work(), until told to
		// stop.
		// Differs from BasicThread's implementation because it waits until
		// all pending frames are written before it stops.
	
	virtual int Work (void);
		// Write frames to the raw-video stream.
};

// Threads for denoising intensity & color.
DenoiserThreadY g_oDenoiserThreadY;
DenoiserThreadCbCr g_oDenoiserThreadCbCr;

// Threads for reading and writing raw video.
DenoiserThreadRead g_oDenoiserThreadRead;
DenoiserThreadWrite g_oDenoiserThreadWrite;



// Initialize the denoising system.
int newdenoise_init (int a_nFrames, int a_nWidthY, int a_nHeightY,
	int a_nWidthCbCr, int a_nHeightCbCr, int a_nInputFD,
	int a_nOutputFD, const y4m_stream_info_t *a_pStreamInfo,
	y4m_frame_info_t *a_pFrameInfo)
{
	Status_t eStatus;
		// An error that may occur.
	int nInterlace;
		// A factor to apply to frames/frame-height because of
		// interlacing.
	
	// No errors yet.
	eStatus = g_kNoError;

	// Save the width and height.
	g_nWidthY = a_nWidthY;
	g_nHeightY = a_nHeightY;
	g_nWidthCbCr = a_nWidthCbCr;
	g_nHeightCbCr = a_nHeightCbCr;

	// If the video is interlaced, that means the denoiser will see
	// twice as many frames, half their original height.
	nInterlace = (denoiser.interlaced) ? 2 : 1;
	
	// If input/output should be handled in separate threads, set that
	// up.
	if (denoiser.threads & 1)
	{
		g_oDenoiserThreadRead.Initialize (a_nInputFD,
			a_pStreamInfo, a_pFrameInfo, a_nWidthY, a_nHeightY,
			a_nWidthCbCr, a_nHeightCbCr);
		g_oDenoiserThreadWrite.Initialize (a_nOutputFD,
			a_pStreamInfo, a_pFrameInfo, a_nWidthY, a_nHeightY,
			a_nWidthCbCr, a_nHeightCbCr);
	}

	// If intensity should be denoised, set it up.
	if (a_nWidthY != 0 && a_nHeightY != 0)
	{
		g_bMotionSearcherY = true;
		g_nPixelsY = (a_nWidthY * a_nHeightY) / nInterlace;
		g_pPixelsY = new MotionSearcherY::Pixel_t [g_nPixelsY];
		if (g_pPixelsY == NULL)
			return -1;
		g_oMotionSearcherY.Init (eStatus, nInterlace * a_nFrames,
			a_nWidthY, a_nHeightY / nInterlace,
			denoiser.radiusY, denoiser.radiusY,
			denoiser.zThresholdY, denoiser.thresholdY,
			denoiser.matchCountThrottle, denoiser.matchSizeThrottle);
		if (eStatus != g_kNoError)
		{
			delete[] g_pPixelsY;
			return -1;
		}
	}
	else
		g_bMotionSearcherY = false;

	// If color should be denoised, set it up.
	if (a_nWidthCbCr != 0 && a_nHeightCbCr != 0)
	{
		g_bMotionSearcherCbCr = true;
		g_nPixelsCbCr = (a_nWidthCbCr * a_nHeightCbCr) / nInterlace;
		g_pPixelsCbCr = new MotionSearcherCbCr::Pixel_t [g_nPixelsCbCr];
		if (g_pPixelsCbCr == NULL)
		{
			delete[] g_pPixelsY;
			return -1;
		}
		g_oMotionSearcherCbCr.Init (eStatus, nInterlace * a_nFrames,
			a_nWidthCbCr, a_nHeightCbCr / nInterlace,
			denoiser.radiusCbCr / denoiser.frame.ss_h,
			denoiser.radiusCbCr / denoiser.frame.ss_v,
			denoiser.zThresholdCbCr, denoiser.thresholdCbCr,
			denoiser.matchCountThrottle, denoiser.matchSizeThrottle);
		if (eStatus != g_kNoError)
		{
			delete[] g_pPixelsCbCr;
			delete[] g_pPixelsY;
			return -1;
		}

		// If color should be denoised in a separate thread, set that
		// up.
		if (denoiser.threads & 2)
			g_oDenoiserThreadCbCr.Initialize();
	}
	else
		g_bMotionSearcherCbCr = false;

	// Initialization was successful.
	return 0;
}

// Shut down the denoising system.
int
newdenoise_shutdown (void)
{
	// If color was denoised in a separate thread, shut that down.
	if (g_bMotionSearcherCbCr && (denoiser.threads & 2))
		g_oDenoiserThreadCbCr.ForceShutdown();
	
	// If reading/writing is being done in separate threads, shut
	// them down.
	if (denoiser.threads & 1)
	{
		g_oDenoiserThreadRead.ForceShutdown();
		g_oDenoiserThreadWrite.ForceShutdown();
	}
	
	// No errors.
	return 0;
}

/* Read another frame.  Usable only in multi-threaded situations. */
int
newdenoise_read_frame (uint8_t **a_apPlanes)
{
	// Make sure the read/write threads are being used.
	assert (denoiser.threads & 1);

	// Easy enough.
	return g_oDenoiserThreadRead.ReadFrame (a_apPlanes);
}

/* Get space to write another frame.  Usable only in multi-threaded
   situations. */
int
newdenoise_get_write_frame (uint8_t **a_apPlanes)
{
	// Make sure the read/write threads are being used.
	assert (denoiser.threads & 1);

	// Easy enough.
	return g_oDenoiserThreadWrite.GetSpaceToWriteFrame (a_apPlanes);
}

/* Write another frame.  Usable only in multi-threaded situations. */
int
newdenoise_write_frame (void)
{
	// Make sure the read/write threads are being used.
	assert (denoiser.threads & 1);

	// Easy enough.
	g_oDenoiserThreadWrite.WriteFrame();
	return Y4M_OK;
}

// (This routine isn't used any more, but I think it should be kept around
// as a reference.)
int
newdenoise_frame0 (const uint8_t *a_pInputY, const uint8_t *a_pInputCb,
	const uint8_t *a_pInputCr, uint8_t *a_pOutputY,
	uint8_t *a_pOutputCb, uint8_t *a_pOutputCr)
{
	Status_t eStatus;
		// An error that may occur.
	const MotionSearcherY::ReferenceFrame_t *pFrameY;
	const MotionSearcherCbCr::ReferenceFrame_t *pFrameCbCr;
		// Denoised frame data, ready for output.
	int i;
		// Used to loop through pixels.

	// No errors yet.
	eStatus = g_kNoError;

	// No output frames have been received yet.
	pFrameY = NULL;
	pFrameCbCr = NULL;

	// If it's time to purge, do so.
	{
		extern int frame;
		if (frame % denoiser.frames == 0)
		{
			g_oMotionSearcherY.Purge();
			g_oMotionSearcherCbCr.Purge();
		}
	}

	// If the end of input has been reached, then return the next
	// remaining denoised frame, if available.
	if ((g_bMotionSearcherY && a_pInputY == NULL)
		|| (g_bMotionSearcherCbCr && a_pInputCr == NULL))
	{
		// Get any remaining frame.
		if (g_bMotionSearcherY)
			pFrameY = g_oMotionSearcherY.GetRemainingFrames();
		if (g_bMotionSearcherCbCr)
			pFrameCbCr = g_oMotionSearcherCbCr.GetRemainingFrames();
	
		// Output it.
		output_frame (pFrameY, pFrameCbCr, a_pOutputY, a_pOutputCb,
			a_pOutputCr);
	}

	// Otherwise, if there is more input, feed the frame into the
	// denoiser & possibly get a frame back.
	else
	{
		// Get any frame that's ready for output.
		if (g_bMotionSearcherY)
			pFrameY = g_oMotionSearcherY.GetFrameReadyForOutput();
		if (g_bMotionSearcherCbCr)
			pFrameCbCr = g_oMotionSearcherCbCr.GetFrameReadyForOutput();
	
		// Output it.
		output_frame (pFrameY, pFrameCbCr, a_pOutputY, a_pOutputCb,
			a_pOutputCr);
	
		// Pass the input frame to the denoiser.
		if (g_bMotionSearcherY)
		{
			// Convert the input frame into the format needed by the
			// denoiser.  (This step is a big waste of time & cache.
			// I wish there was another way.)
			for (i = 0; i < g_nPixelsY; ++i)
				g_pPixelsY[i] = PixelY (a_pInputY + i);

			// Pass the frame to the denoiser.
			g_oMotionSearcherY.AddFrame (eStatus, g_pPixelsY);
			if (eStatus != g_kNoError)
				return -1;
		}
		if (g_bMotionSearcherCbCr)
		{
			PixelCbCr::Num_t aCbCr[2];

			// Convert the input frame into the format needed by the
			// denoiser.  (This step is a big waste of time & cache.
			// I wish there was another way.)
			for (i = 0; i < g_nPixelsCbCr; ++i)
			{
				aCbCr[0] = a_pInputCb[i];
				aCbCr[1] = a_pInputCr[i];
				g_pPixelsCbCr[i] = PixelCbCr (aCbCr);
			}

			// Pass the frame to the denoiser.
			g_oMotionSearcherCbCr.AddFrame (eStatus, g_pPixelsCbCr);
			if (eStatus != g_kNoError)
				return -1;
		}
	}

	// If we're denoising both color & intensity, make sure we
	// either got two reference frames or none at all.
	assert (!g_bMotionSearcherY || !g_bMotionSearcherCbCr
		|| (pFrameY == NULL && pFrameCbCr == NULL)
		|| (pFrameY != NULL && pFrameCbCr != NULL));

	// Return whether there was an output frame this time.
	return ((g_bMotionSearcherY && pFrameY != NULL)
		|| (g_bMotionSearcherCbCr && pFrameCbCr != NULL)) ? 0 : 1;
}

int
newdenoise_frame_intensity (const uint8_t *a_pInputY,
	uint8_t *a_pOutputY)
{
	Status_t eStatus;
		// An error that may occur.
	const MotionSearcherY::ReferenceFrame_t *pFrameY;
		// Denoised frame data, ready for output.
	int i;
		// Used to loop through pixels.
	
	// Make sure intensity is being denoised.
	assert (g_bMotionSearcherY);

	// No errors yet.
	eStatus = g_kNoError;

	// No output frames have been received yet.
	pFrameY = NULL;

	// If it's time to purge, do so.
	{
		extern int frame;
		if (frame % denoiser.frames == 0)
			g_oMotionSearcherY.Purge();
	}

	// If the end of input has been reached, then return the next
	// remaining denoised frame, if available.
	if (a_pInputY == NULL)
	{
		// Get any remaining frame.
		pFrameY = g_oMotionSearcherY.GetRemainingFrames();
	
		// Output it.
		output_frame (pFrameY, NULL, a_pOutputY, NULL, NULL);
	}

	// Otherwise, if there is more input, feed the frame into the
	// denoiser & possibly get a frame back.
	else
	{
		// Get any frame that's ready for output.
		pFrameY = g_oMotionSearcherY.GetFrameReadyForOutput();
	
		// Output it.
		output_frame (pFrameY, NULL, a_pOutputY, NULL, NULL);
	
		// Convert the input frame into the format needed by the
		// denoiser.  (This step is a big waste of time & cache.
		// I wish there was another way.)
		for (i = 0; i < g_nPixelsY; ++i)
			g_pPixelsY[i] = PixelY (a_pInputY + i);

		// Pass the frame to the denoiser.
		g_oMotionSearcherY.AddFrame (eStatus, g_pPixelsY);
		if (eStatus != g_kNoError)
			return -1;
	}

	// Return whether there was an output frame this time.
	return (pFrameY != NULL) ? 0 : 1;
}

int
newdenoise_frame_color (const uint8_t *a_pInputCb,
	const uint8_t *a_pInputCr, uint8_t *a_pOutputCb,
	uint8_t *a_pOutputCr)
{
	Status_t eStatus;
		// An error that may occur.
	const MotionSearcherCbCr::ReferenceFrame_t *pFrameCbCr;
		// Denoised frame data, ready for output.
	int i;
		// Used to loop through pixels.
	
	// Make sure color is being denoised.
	assert (g_bMotionSearcherCbCr);

	// No errors yet.
	eStatus = g_kNoError;

	// No output frames have been received yet.
	pFrameCbCr = NULL;

	// If it's time to purge, do so.
	{
		extern int frame;
		if (frame % denoiser.frames == 0)
			g_oMotionSearcherCbCr.Purge();
	}

	// If the end of input has been reached, then return the next
	// remaining denoised frame, if available.
	if (a_pInputCr == NULL)
	{
		// Get any remaining frame.
		pFrameCbCr = g_oMotionSearcherCbCr.GetRemainingFrames();
	
		// Output it.
		output_frame (NULL, pFrameCbCr, NULL, a_pOutputCb, a_pOutputCr);
	}

	// Otherwise, if there is more input, feed the frame into the
	// denoiser & possibly get a frame back.
	else
	{
		// Get any frame that's ready for output.
		pFrameCbCr = g_oMotionSearcherCbCr.GetFrameReadyForOutput();
	
		// Output it.
		output_frame (NULL, pFrameCbCr, NULL, a_pOutputCb, a_pOutputCr);
	
		// Pass the input frame to the denoiser.
		{
			PixelCbCr::Num_t aCbCr[2];

			// Convert the input frame into the format needed by the
			// denoiser.  (This step is a big waste of time & cache.
			// I wish there was another way.)
			for (i = 0; i < g_nPixelsCbCr; ++i)
			{
				aCbCr[0] = a_pInputCb[i];
				aCbCr[1] = a_pInputCr[i];
				g_pPixelsCbCr[i] = PixelCbCr (aCbCr);
			}

			// Pass the frame to the denoiser.
			g_oMotionSearcherCbCr.AddFrame (eStatus, g_pPixelsCbCr);
			if (eStatus != g_kNoError)
				return -1;
		}
	}

	// Return whether there was an output frame this time.
	return (pFrameCbCr != NULL) ? 0 : 1;
}

int
newdenoise_frame (const uint8_t *a_pInputY, const uint8_t *a_pInputCb,
	const uint8_t *a_pInputCr, uint8_t *a_pOutputY,
	uint8_t *a_pOutputCb, uint8_t *a_pOutputCr)
{
	int bY, bCbCr;

	// Make the compiler shut up.
	bY = bCbCr = 0;

	// Denoise intensity & color.  (Do intensity in the current
	// thread.)
	if (g_bMotionSearcherCbCr && (denoiser.threads & 2))
		g_oDenoiserThreadCbCr.AddFrame (a_pInputCb, a_pInputCr,
			a_pOutputCb, a_pOutputCr);
	if (g_bMotionSearcherY)
		bY = newdenoise_frame_intensity (a_pInputY, a_pOutputY);
	if (g_bMotionSearcherCbCr && !(denoiser.threads & 2))
		bCbCr = newdenoise_frame_color (a_pInputCb, a_pInputCr,
			a_pOutputCb, a_pOutputCr);
	if (g_bMotionSearcherCbCr && (denoiser.threads & 2))
		bCbCr = g_oDenoiserThreadCbCr.WaitForAddFrame();

	// If we're denoising both color & intensity, make sure we
	// either got two reference frames or none at all.  (This is
	// a sanity check, to make sure there are as many intensity
	// frames as color frames.)
	assert (!g_bMotionSearcherY || !g_bMotionSearcherCbCr
		|| (bY != 0 && bCbCr != 0)
		|| (bY == 0 && bCbCr == 0));
	
	// Return 0 if there are no errors.
	return (bY) ? bY : bCbCr;
}

static void output_frame
	(const MotionSearcherY::ReferenceFrame_t *a_pFrameY,
	const MotionSearcherCbCr::ReferenceFrame_t *a_pFrameCbCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr)
{
	int i;
		// Used to loop through pixels.

	// Convert any denoised intensity frame into the format expected
	// by our caller.
	if (a_pFrameY != NULL)
	{
		ReferencePixelY *pY;
			// The pixel, as it's being converted to the output format.

		// Make sure our caller gave us somewhere to write output.
		assert (a_pOutputY != NULL);

		// Loop through all the pixels, convert them to the output
		// format.
		for (i = 0; i < g_nPixelsY; ++i)
		{
			pY = a_pFrameY->GetPixel (i);
			assert (pY != NULL);
			const PixelY &rY = pY->GetValue();
			a_pOutputY[i] = rY[0];
		}
	}
	if (a_pFrameCbCr != NULL)
	{
		ReferencePixelCbCr *pCbCr;
			// The pixel, as it's being converted to the output format.

		// Make sure our caller gave us somewhere to write output.
		assert (a_pOutputCb != NULL && a_pOutputCr != NULL);

		// Loop through all the pixels, convert them to the output
		// format.
		for (i = 0; i < g_nPixelsCbCr; ++i)
		{
			pCbCr = a_pFrameCbCr->GetPixel (i);
			assert (pCbCr != NULL);
			const PixelCbCr &rCbCr = pCbCr->GetValue();
			a_pOutputCb[i] = rCbCr[0];
			a_pOutputCr[i] = rCbCr[1];
		}
	}
}

// (This routine isn't used any more, but I think it should be kept around
// as a reference.)
int
newdenoise_interlaced_frame0 (const uint8_t *a_pInputY,
	const uint8_t *a_pInputCb, const uint8_t *a_pInputCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr)
{
	Status_t eStatus;
		// An error that may occur.
	const MotionSearcherY::ReferenceFrame_t *pFrameY;
	const MotionSearcherCbCr::ReferenceFrame_t *pFrameCbCr;
		// Denoised frame data, ready for output.
	int i, x, y;
		// Used to loop through pixels.
	int nMask;
		// Used to switch between top-field interlacing and bottom-field
		// interlacing.

	// No errors yet.
	eStatus = g_kNoError;

	// No output frames have been received yet.
	pFrameY = NULL;
	pFrameCbCr = NULL;

	// If it's time to purge, do so.
	{
		extern int frame;
		if (frame % denoiser.frames == 0)
		{
			g_oMotionSearcherY.Purge();
			g_oMotionSearcherCbCr.Purge();
		}
	}

	// Set up for the type of interlacing.
	nMask = (denoiser.interlaced == 2) ? 1 : 0;

	// If the end of input has been reached, then return the next
	// remaining denoised frame, if available.
	if ((g_bMotionSearcherY && a_pInputY == NULL)
		|| (g_bMotionSearcherCbCr && a_pInputCr == NULL))
	{
		// Get 1/2 any remaining frame.
		if (g_bMotionSearcherY)
			pFrameY = g_oMotionSearcherY.GetRemainingFrames();
		if (g_bMotionSearcherCbCr)
			pFrameCbCr = g_oMotionSearcherCbCr.GetRemainingFrames();
	
		// Output it.
		output_field (nMask ^ 0, pFrameY, pFrameCbCr, a_pOutputY,
			a_pOutputCb, a_pOutputCr);

		// Get 1/2 any remaining frame.
		if (g_bMotionSearcherY)
			pFrameY = g_oMotionSearcherY.GetRemainingFrames();
		if (g_bMotionSearcherCbCr)
			pFrameCbCr = g_oMotionSearcherCbCr.GetRemainingFrames();
	
		// Output it.
		output_field (nMask ^ 1, pFrameY, pFrameCbCr, a_pOutputY,
			a_pOutputCb, a_pOutputCr);
	}

	// Otherwise, if there is more input, feed the frame into the
	// denoiser & possibly get a frame back.
	else
	{
		// Get 1/2 any frame that's ready for output.
		if (g_bMotionSearcherY)
			pFrameY = g_oMotionSearcherY.GetFrameReadyForOutput();
		if (g_bMotionSearcherCbCr)
			pFrameCbCr = g_oMotionSearcherCbCr.GetFrameReadyForOutput();
	
		// Output it.
		output_field (nMask ^ 0, pFrameY, pFrameCbCr, a_pOutputY,
			a_pOutputCb, a_pOutputCr);
	
		// Pass the input frame to the denoiser.
		if (g_bMotionSearcherY)
		{
			// Convert the input frame into the format needed by the
			// denoiser.
			for (i = 0, y = (nMask ^ 0); y < g_nHeightY; y += 2)
				for (x = 0; x < g_nWidthY; ++x, ++i)
					g_pPixelsY[i]
						= PixelY (a_pInputY + (y * g_nWidthY + x));
			assert (i == g_nPixelsY);

			// Pass the frame to the denoiser.
			g_oMotionSearcherY.AddFrame (eStatus, g_pPixelsY);
			if (eStatus != g_kNoError)
				return -1;
		}
		if (g_bMotionSearcherCbCr)
		{
			PixelCbCr::Num_t aCbCr[2];

			// Convert the input frame into the format needed by the
			// denoiser.
			for (i = 0, y = (nMask ^ 0); y < g_nHeightCbCr; y += 2)
			{
				for (x = 0; x < g_nWidthCbCr; ++x, ++i)
				{
					aCbCr[0] = a_pInputCb[y * g_nWidthCbCr + x];
					aCbCr[1] = a_pInputCr[y * g_nWidthCbCr + x];
					g_pPixelsCbCr[i] = PixelCbCr (aCbCr);
				}
			}
			assert (i == g_nPixelsCbCr);

			// Pass the frame to the denoiser.
			g_oMotionSearcherCbCr.AddFrame (eStatus, g_pPixelsCbCr);
			if (eStatus != g_kNoError)
				return -1;
		}
	
		// Get 1/2 any frame that's ready for output.
		if (g_bMotionSearcherY)
			pFrameY = g_oMotionSearcherY.GetFrameReadyForOutput();
		if (g_bMotionSearcherCbCr)
			pFrameCbCr = g_oMotionSearcherCbCr.GetFrameReadyForOutput();
	
		// Output it.
		output_field (nMask ^ 1, pFrameY, pFrameCbCr, a_pOutputY,
			a_pOutputCb, a_pOutputCr);
	
		// Pass the input frame to the denoiser.
		if (g_bMotionSearcherY)
		{
			// Convert the input frame into the format needed by the
			// denoiser.
			for (i = 0, y = (nMask ^ 1); y < g_nHeightY; y += 2)
				for (x = 0; x < g_nWidthY; ++x, ++i)
					g_pPixelsY[i]
						= PixelY (a_pInputY + (y * g_nWidthY + x));
			assert (i == g_nPixelsY);

			// Pass the frame to the denoiser.
			g_oMotionSearcherY.AddFrame (eStatus, g_pPixelsY);
			if (eStatus != g_kNoError)
				return -1;
		}
		if (g_bMotionSearcherCbCr)
		{
			PixelCbCr::Num_t aCbCr[2];

			// Convert the input frame into the format needed by the
			// denoiser.
			for (i = 0, y = (nMask ^ 1); y < g_nHeightCbCr; y += 2)
			{
				for (x = 0; x < g_nWidthCbCr; ++x, ++i)
				{
					aCbCr[0] = a_pInputCb[y * g_nWidthCbCr + x];
					aCbCr[1] = a_pInputCr[y * g_nWidthCbCr + x];
					g_pPixelsCbCr[i] = PixelCbCr (aCbCr);
				}
			}
			assert (i == g_nPixelsCbCr);

			// Pass the frame to the denoiser.
			g_oMotionSearcherCbCr.AddFrame (eStatus, g_pPixelsCbCr);
			if (eStatus != g_kNoError)
				return -1;
		}
	}

	// If we're denoising both color & intensity, make sure we
	// either got two reference frames or none at all.
	assert (!g_bMotionSearcherY || !g_bMotionSearcherCbCr
		|| (pFrameY == NULL && pFrameCbCr == NULL)
		|| (pFrameY != NULL && pFrameCbCr != NULL));

	// Return whether there was an output frame this time.
	return ((g_bMotionSearcherY && pFrameY != NULL)
		|| (g_bMotionSearcherCbCr && pFrameCbCr != NULL)) ? 0 : 1;
}

int
newdenoise_interlaced_frame_intensity (const uint8_t *a_pInputY,
	uint8_t *a_pOutputY)
{
	Status_t eStatus;
		// An error that may occur.
	const MotionSearcherY::ReferenceFrame_t *pFrameY;
		// Denoised frame data, ready for output.
	int i, x, y;
		// Used to loop through pixels.
	int nMask;
		// Used to switch between top-field interlacing and bottom-field
		// interlacing.
	
	// Make sure intensity is being denoised.
	assert (g_bMotionSearcherY);

	// No errors yet.
	eStatus = g_kNoError;

	// No output frames have been received yet.
	pFrameY = NULL;

	// If it's time to purge, do so.
	{
		extern int frame;
		if (frame % denoiser.frames == 0)
			g_oMotionSearcherY.Purge();
	}

	// Set up for the type of interlacing.
	nMask = (denoiser.interlaced == 2) ? 1 : 0;

	// If the end of input has been reached, then return the next
	// remaining denoised frame, if available.
	if (a_pInputY == NULL)
	{
		// Get 1/2 any remaining frame.
		pFrameY = g_oMotionSearcherY.GetRemainingFrames();
	
		// Output it.
		output_field (nMask ^ 0, pFrameY, NULL, a_pOutputY, NULL, NULL);

		// Get 1/2 any remaining frame.
		pFrameY = g_oMotionSearcherY.GetRemainingFrames();
	
		// Output it.
		output_field (nMask ^ 1, pFrameY, NULL, a_pOutputY, NULL, NULL);
	}

	// Otherwise, if there is more input, feed the frame into the
	// denoiser & possibly get a frame back.
	else
	{
		// Get 1/2 any frame that's ready for output.
		pFrameY = g_oMotionSearcherY.GetFrameReadyForOutput();
	
		// Output it.
		output_field (nMask ^ 0, pFrameY, NULL, a_pOutputY, NULL, NULL);
	
		// Convert the input frame into the format needed by the
		// denoiser.
		for (i = 0, y = (nMask ^ 0); y < g_nHeightY; y += 2)
			for (x = 0; x < g_nWidthY; ++x, ++i)
				g_pPixelsY[i]
					= PixelY (a_pInputY + (y * g_nWidthY + x));
		assert (i == g_nPixelsY);

		// Pass the frame to the denoiser.
		g_oMotionSearcherY.AddFrame (eStatus, g_pPixelsY);
		if (eStatus != g_kNoError)
			return -1;
	
		// Get 1/2 any frame that's ready for output.
		pFrameY = g_oMotionSearcherY.GetFrameReadyForOutput();
	
		// Output it.
		output_field (nMask ^ 1, pFrameY, NULL, a_pOutputY, NULL, NULL);
	
		// Convert the input frame into the format needed by the
		// denoiser.
		for (i = 0, y = (nMask ^ 1); y < g_nHeightY; y += 2)
			for (x = 0; x < g_nWidthY; ++x, ++i)
				g_pPixelsY[i]
					= PixelY (a_pInputY + (y * g_nWidthY + x));
		assert (i == g_nPixelsY);

		// Pass the frame to the denoiser.
		g_oMotionSearcherY.AddFrame (eStatus, g_pPixelsY);
		if (eStatus != g_kNoError)
			return -1;
	}

	// Return whether there was an output frame this time.
	return (pFrameY != NULL) ? 0 : 1;
}

int
newdenoise_interlaced_frame_color (const uint8_t *a_pInputCb,
	const uint8_t *a_pInputCr, uint8_t *a_pOutputCb,
	uint8_t *a_pOutputCr)
{
	Status_t eStatus;
		// An error that may occur.
	const MotionSearcherCbCr::ReferenceFrame_t *pFrameCbCr;
		// Denoised frame data, ready for output.
	int i, x, y;
		// Used to loop through pixels.
	int nMask;
		// Used to switch between top-field interlacing and bottom-field
		// interlacing.
	
	// Make sure color is being denoised.
	assert (g_bMotionSearcherCbCr);

	// No errors yet.
	eStatus = g_kNoError;

	// No output frames have been received yet.
	pFrameCbCr = NULL;

	// If it's time to purge, do so.
	{
		extern int frame;
		if (frame % denoiser.frames == 0)
			g_oMotionSearcherCbCr.Purge();
	}

	// Set up for the type of interlacing.
	nMask = (denoiser.interlaced == 2) ? 1 : 0;

	// If the end of input has been reached, then return the next
	// remaining denoised frame, if available.
	if (a_pInputCr == NULL)
	{
		// Get 1/2 any remaining frame.
		pFrameCbCr = g_oMotionSearcherCbCr.GetRemainingFrames();
	
		// Output it.
		output_field (nMask ^ 0, NULL, pFrameCbCr, NULL, a_pOutputCb,
			a_pOutputCr);

		// Get 1/2 any remaining frame.
		pFrameCbCr = g_oMotionSearcherCbCr.GetRemainingFrames();
	
		// Output it.
		output_field (nMask ^ 1, NULL, pFrameCbCr, NULL, a_pOutputCb,
			a_pOutputCr);
	}

	// Otherwise, if there is more input, feed the frame into the
	// denoiser & possibly get a frame back.
	else
	{
		// Get 1/2 any frame that's ready for output.
		pFrameCbCr = g_oMotionSearcherCbCr.GetFrameReadyForOutput();
	
		// Output it.
		output_field (nMask ^ 0, NULL, pFrameCbCr, NULL, a_pOutputCb,
			a_pOutputCr);
	
		// Pass the input frame to the denoiser.
		{
			PixelCbCr::Num_t aCbCr[2];

			// Convert the input frame into the format needed by the
			// denoiser.
			for (i = 0, y = (nMask ^ 0); y < g_nHeightCbCr; y += 2)
			{
				for (x = 0; x < g_nWidthCbCr; ++x, ++i)
				{
					aCbCr[0] = a_pInputCb[y * g_nWidthCbCr + x];
					aCbCr[1] = a_pInputCr[y * g_nWidthCbCr + x];
					g_pPixelsCbCr[i] = PixelCbCr (aCbCr);
				}
			}
			assert (i == g_nPixelsCbCr);

			// Pass the frame to the denoiser.
			g_oMotionSearcherCbCr.AddFrame (eStatus, g_pPixelsCbCr);
			if (eStatus != g_kNoError)
				return -1;
		}
	
		// Get 1/2 any frame that's ready for output.
		pFrameCbCr = g_oMotionSearcherCbCr.GetFrameReadyForOutput();
	
		// Output it.
		output_field (nMask ^ 1, NULL, pFrameCbCr, NULL, a_pOutputCb,
			a_pOutputCr);
	
		// Pass the input frame to the denoiser.
		{
			PixelCbCr::Num_t aCbCr[2];

			// Convert the input frame into the format needed by the
			// denoiser.
			for (i = 0, y = (nMask ^ 1); y < g_nHeightCbCr; y += 2)
			{
				for (x = 0; x < g_nWidthCbCr; ++x, ++i)
				{
					aCbCr[0] = a_pInputCb[y * g_nWidthCbCr + x];
					aCbCr[1] = a_pInputCr[y * g_nWidthCbCr + x];
					g_pPixelsCbCr[i] = PixelCbCr (aCbCr);
				}
			}
			assert (i == g_nPixelsCbCr);

			// Pass the frame to the denoiser.
			g_oMotionSearcherCbCr.AddFrame (eStatus, g_pPixelsCbCr);
			if (eStatus != g_kNoError)
				return -1;
		}
	}

	// Return whether there was an output frame this time.
	return (pFrameCbCr != NULL) ? 0 : 1;
}

int
newdenoise_interlaced_frame (const uint8_t *a_pInputY,
	const uint8_t *a_pInputCb, const uint8_t *a_pInputCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr)
{
	int bY, bCbCr;

	// Make the compiler shut up.
	bY = bCbCr = 0;

	// Denoise intensity & color.  (Intensity is denoised in the
	// current thread.)
	if (g_bMotionSearcherCbCr && (denoiser.threads & 2))
		g_oDenoiserThreadCbCr.AddFrame (a_pInputCb, a_pInputCr,
			a_pOutputCb, a_pOutputCr);
	if (g_bMotionSearcherY)
		bY = newdenoise_interlaced_frame_intensity (a_pInputY,
			a_pOutputY);
	if (g_bMotionSearcherCbCr && !(denoiser.threads & 2))
		bCbCr = newdenoise_interlaced_frame_color (a_pInputCb,
			a_pInputCr, a_pOutputCb, a_pOutputCr);
	if (g_bMotionSearcherCbCr && (denoiser.threads & 2))
		bCbCr = g_oDenoiserThreadCbCr.WaitForAddFrame();

	// If we're denoising both color & intensity, make sure we
	// either got two reference frames or none at all.
	assert (!g_bMotionSearcherY || !g_bMotionSearcherCbCr
		|| (bY != 0 && bCbCr != 0)
		|| (bY == 0 && bCbCr == 0));
	
	// Return 0 if there are no errors.
	return (bY) ? bY : bCbCr;
}

static void output_field
	(int a_nMask, const MotionSearcherY::ReferenceFrame_t *a_pFrameY,
	const MotionSearcherCbCr::ReferenceFrame_t *a_pFrameCbCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr)
{
	int i, x, y;
		// Used to loop through pixels.

	// Convert any denoised intensity frame into the format expected
	// by our caller.
	if (a_pFrameY != NULL)
	{
		ReferencePixelY *pY;
			// The pixel, as it's being converted to the output format.

		// Make sure our caller gave us somewhere to write output.
		assert (a_pOutputY != NULL);

		// Loop through all the pixels, convert them to the output
		// format.
		for (i = 0, y = a_nMask; y < g_nHeightY; y += 2)
		{
			for (x = 0; x < g_nWidthY; ++x, ++i)
			{
				pY = a_pFrameY->GetPixel (i);
				assert (pY != NULL);
				const PixelY &rY = pY->GetValue();
				a_pOutputY[y * g_nWidthY + x] = rY[0];
			}
		}
	}
	if (a_pFrameCbCr != NULL)
	{
		ReferencePixelCbCr *pCbCr;
			// The pixel, as it's being converted to the output format.

		// Make sure our caller gave us somewhere to write output.
		assert (a_pOutputCb != NULL && a_pOutputCr != NULL);

		// Loop through all the pixels, convert them to the output
		// format.
		for (i = 0, y = a_nMask; y < g_nHeightCbCr; y += 2)
		{
			for (x = 0; x < g_nWidthCbCr; ++x, ++i)
			{
				pCbCr = a_pFrameCbCr->GetPixel (i);
				assert (pCbCr != NULL);
				const PixelCbCr &rCbCr = pCbCr->GetValue();
				a_pOutputCb[y * g_nWidthCbCr + x] = rCbCr[0];
				a_pOutputCr[y * g_nWidthCbCr + x] = rCbCr[1];
			}
		}
	}
}



// Pixel methods.



// Turn an integer tolerance value into what's appropriate for
// the pixel type.
template <>
int32_t
PixelY::MakeTolerance (uint8_t a_tnTolerance)
{
	// For a one-dimensional pixel, just use the given number.
	return int32_t (a_tnTolerance);
}



// Turn an integer tolerance value into what's appropriate for
// the pixel type.
template <>
int32_t
PixelCbCr::MakeTolerance (uint8_t a_tnTolerance)
{
	// For a two-dimensional pixel, use the square of the given number.
	return int32_t (a_tnTolerance) * int32_t (a_tnTolerance);
}



// Return true if the two pixels are within the specified tolerance.
template <>
bool
PixelY::IsWithinTolerance (const PixelY &a_rOther,
	int32_t a_tnTolerance) const
{
	// Check to see if the absolute value of the difference between
	// the two pixels is within our tolerance value.
	return AbsoluteValue (int32_t (m_atnVal[0])
		- int32_t (a_rOther.m_atnVal[0])) <= a_tnTolerance;
}



// Return true if the two pixels are within the specified tolerance.
template <>
bool
PixelY::IsWithinTolerance (const PixelY &a_rOther,
	int32_t a_tnTolerance, int32_t &a_rtnSAD) const
{
	// Check to see if the absolute value of the difference between
	// the two pixels is within our tolerance value.
	a_rtnSAD = AbsoluteValue (int32_t (m_atnVal[0])
		- int32_t (a_rOther.m_atnVal[0]));
	return a_rtnSAD <= a_tnTolerance;
}



// Return true if the two pixels are within the specified tolerance.
template <>
bool
PixelCbCr::IsWithinTolerance
	(const PixelCbCr &a_rOther, int32_t a_tnTolerance) const
{
	// Calculate the vector difference between the two pixels.
	int32_t tnX = int32_t (m_atnVal[0])
		- int32_t (a_rOther.m_atnVal[0]);
	int32_t tnY = int32_t (m_atnVal[1])
		- int32_t (a_rOther.m_atnVal[1]);

	// Check to see if the length of the vector difference is within
	// our tolerance value.  (Technically, we check the squares of
	// the values, but that's just as valid & much faster than
	// calculating a square root.)
	return tnX * tnX + tnY * tnY <= a_tnTolerance;
}



// Return true if the two pixels are within the specified tolerance.
template <>
bool
PixelCbCr::IsWithinTolerance
	(const PixelCbCr &a_rOther, int32_t a_tnTolerance,
	int32_t &a_rtnSAD) const
{
	// Calculate the vector difference between the two pixels.
	int32_t tnX = int32_t (m_atnVal[0])
		- int32_t (a_rOther.m_atnVal[0]);
	int32_t tnY = int32_t (m_atnVal[1])
		- int32_t (a_rOther.m_atnVal[1]);

	// Calculate the sample-array-difference.
	a_rtnSAD = tnX * tnX + tnY * tnY;

	// Check to see if the length of the vector difference is within
	// our tolerance value.  (Technically, we check the squares of
	// the values, but that's just as valid & much faster than
	// calculating a square root.)
	return a_rtnSAD <= a_tnTolerance;
}



// The ThreadMutex class.



// Default constructor.
ThreadMutex::ThreadMutex()
{
	int nErr;
		// An error that may occur.
	pthread_mutexattr_t sMutexAttr;
		// Attributes for the mutex.

#ifndef NDEBUG
	// Not locked upon creation.
	m_bLocked = false;
#endif // NDEBUG

	// Initialize the mutex.
	nErr = pthread_mutexattr_init (&sMutexAttr);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_mutexattr_init() failed: %s",
			strerror (nErr));
	nErr = pthread_mutex_init (&m_oMutex, &sMutexAttr);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_mutex_init() failed: %s",
			strerror (nErr));
	nErr = pthread_mutexattr_destroy (&sMutexAttr);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_mutexattr_destroy() failed: %s",
			strerror (nErr));
}



// Destructor.
ThreadMutex::~ThreadMutex()
{
	int nErr;
		// An error that may occur.

	// Make sure it's not locked.
	assert (!m_bLocked);
	
	// Destroy the mutex.
	nErr = pthread_mutex_destroy (&m_oMutex);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_mutex_destroy() failed: %s",
			strerror (nErr));
}



// Lock the mutex.
void
ThreadMutex::Lock (void)
{
	int nErr;
		// An error that may occur.

	// Note that the mutex could either be locked or unlocked when this
	// method is called.  If it's locked, we'll have to wait our turn.

	// Get exclusive access.
	nErr = pthread_mutex_lock (&m_oMutex);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_mutex_lock() failed: %s",
			strerror (nErr));

#ifndef NDEBUG

	// Make sure it wasn't locked already.
	assert (!m_bLocked);

	// Now it's locked.
	m_bLocked = true;

#endif // NDEBUG
}



// Unlock the mutex
void
ThreadMutex::Unlock (void)
{
	int nErr;
		// An error that may occur.

	// Make sure it's locked.
	assert (m_bLocked);

#ifndef NDEBUG

	// Now it's unlocked.
	m_bLocked = false;

#endif // NDEBUG

	// Release exclusive access.
	nErr = pthread_mutex_unlock (&m_oMutex);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_mutex_unlock() failed: %s",
			strerror (nErr));
}



// The ThreadCondition class.



// Default constructor.
ThreadCondition::ThreadCondition (ThreadMutex &a_rMutex)
	: m_rMutex (a_rMutex)
{
	int nErr;
		// An error that may occur.
	pthread_condattr_t sCondAttr;
		// Attributes for the condition.
	
	// Initialize the condition.
	nErr = pthread_condattr_init (&sCondAttr);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_condattr_init() failed: %s",
			strerror (nErr));
	nErr = pthread_cond_init (&m_oCondition, &sCondAttr);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_cond_init() failed: %s",
			strerror (nErr));
	nErr = pthread_condattr_destroy (&sCondAttr);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_condattr_destroy() failed: %s",
			strerror (nErr));
	
#ifndef NDEBUG

	// No signal yet.
	m_bSignaled = false;

#endif // NDEBUG
}



// Destructor.
ThreadCondition::~ThreadCondition()
{
	int nErr;
		// An error that may occur.

	// Destroy the condition.
	nErr = pthread_cond_destroy (&m_oCondition);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_cond_destroy() failed: %s",
			strerror (nErr));
}



// Signal the condition.
void
ThreadCondition::Signal (void)
{
	int nErr;
		// An error that may occur.

	// Make sure our mutex is locked.
	assert (m_rMutex.m_bLocked);

#ifndef NDEBUG

	// Make sure we haven't been signaled already.
	assert (!m_bSignaled);

	// Remember we've been signaled.
	m_bSignaled = true;

#endif // NDEBUG

	// Signal the condition.
	nErr = pthread_cond_signal (&m_oCondition);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_cond_signal() failed: %s",
			strerror (nErr));
}



// Wait for the condition to be signaled.
void
ThreadCondition::Wait (void)
{
	int nErr;
		// An error that may occur.

	// Make sure our mutex is locked.
	assert (m_rMutex.m_bLocked);

	// Make sure a condition hasn't been signaled.  (We have to wait
	// for it first, in order for signaling to mean anything.)
	assert (!m_bSignaled);

#ifndef NDEBUG

	// Waiting on the condition will unlock the mutex.
	m_rMutex.m_bLocked = false;

#endif // NDEBUG

	// Wait for the condition to be signaled.
	nErr = pthread_cond_wait (&m_oCondition, m_rMutex);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_cond_wait() failed: %s",
			strerror (nErr));

	// Make sure the condition was signaled properly.
	assert (m_bSignaled);

	// Make sure the mutex thinks it's still unlocked.
	// (It's actually locked...the mutex was re-acquired
	// when the wait succeeded.)
	assert (!m_rMutex.m_bLocked);

#ifndef NDEBUG

	// Remember that the mutex is actually locked.
	m_rMutex.m_bLocked = true;

	// The signal has been delivered.
	m_bSignaled = false;

#endif // NDEBUG
}



// The BasicThread class.



// Default constructor.
BasicThread::BasicThread()
	: m_oInputCondition (m_oMutex), m_oOutputCondition (m_oMutex)
{
	// No waiting yet.
	m_bWaitingForInput = m_bWaitingForOutput = false;

	// No work yet.
	m_bWorkLoop = false;

	// No return value yet.
	m_nWorkRetval = Y4M_OK;

	// Nothing additional to do.  m_oInputCondition and
	// m_oOutputCondition are already constructed, and m_oThreadInfo is
	// initialized by pthread_create(), called in Initialize().
}



// Destructor.
BasicThread::~BasicThread()
{
	// Nothing additional to do.  m_oInputCondition and
	// m_oOutputCondition are destroyed by default, and m_oThreadInfo
	// apparently does not need to be destroyed.
}



// Force the thread to terminate.
void
BasicThread::ForceShutdown (void)
{
	// Get exclusive access.
	Lock();

	// Shut down the thread.  (No need to unlock; Shutdown() will do that.)
	Shutdown();
}



// Start the thread.
void
BasicThread::Initialize (void)
{
	int nErr;
		// An error that may occur.
	pthread_attr_t sThreadAttributes;
		// Where we set up thread attributes.
	
	// Set up to work.
	m_bWorkLoop = true;

	// Set up the initial, default set of thread attributes.
	nErr = pthread_attr_init (&sThreadAttributes);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_attr_init() failed: %s",
			strerror (nErr));

	// Inherit scheduling parameters from the main thread.
	nErr = pthread_attr_setinheritsched (&sThreadAttributes,
		PTHREAD_INHERIT_SCHED);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_attr_setinheritsched() failed: %s",
			strerror (nErr));
	
	// Create the thread.
	nErr = pthread_create (&m_oThreadInfo,
		&sThreadAttributes, WorkLoop, (void *)this);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_create() failed: %s",
			strerror (nErr));
}


	
// Lock the mutex that guards the conditions.
void
BasicThread::Lock (void)
{
	// Easy enough.
	m_oMutex.Lock();
}



// Unlock the mutex that guards the conditions.
void
BasicThread::Unlock (void)
{
	// Easy enough.
	m_oMutex.Unlock();
}



#ifndef NDEBUG

// Returns true if the mutex is locked.
bool
BasicThread::IsLocked (void) const
{
	// Easy enough.
	return m_oMutex.m_bLocked;
}

#endif // !NDEBUG



// Signal that input has been provided to the thread.
void
BasicThread::SignalInput (void)
{
	// Make sure the mutex is locked.
	assert (IsLocked());

	// Easy enough.
	m_oInputCondition.Signal();

	// We're no longer waiting for output.
	m_bWaitingForInput = false;
}



// Wait for input to be provided.
// Called by the subclass' Work() method.
void
BasicThread::WaitForInput (void)
{
	// Make sure the mutex is locked.
	assert (IsLocked());

	// Make sure we're not already waiting for input.
	assert (!m_bWaitingForInput);

	// Easy enough.
	m_bWaitingForInput = true;
	m_oInputCondition.Wait();
	assert (!m_bWaitingForInput);
}



// Signal that output has been provided to the thread.
// Called by the subclass' Work() method.
void
BasicThread::SignalOutput (void)
{
	// Make sure the mutex is locked.
	assert (IsLocked());

	// Easy enough.
	m_oOutputCondition.Signal();

	// We're no longer waiting for output.
	m_bWaitingForOutput = false;
}



// Wait for output to be provided.
void
BasicThread::WaitForOutput (void)
{
	// Make sure the mutex is locked.
	assert (IsLocked());

	// Make sure we're not already waiting for output.
	assert (!m_bWaitingForOutput);

	// Easy enough.
	m_bWaitingForOutput = true;
	m_oOutputCondition.Wait();
	assert (!m_bWaitingForOutput);
}



// Stop the thread.
void
BasicThread::Shutdown (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Tell the thread to stop looping.
	m_bWorkLoop = false;

	// If anyone is waiting, stop them.  They should notice
	// that m_bWorkLoop is now false (or that m_nWorkRetval
	// is not Y4M_OK) and gracefully exit.
	if (m_bWaitingForInput)
		SignalInput();
	if (m_bWaitingForOutput)
		SignalOutput();

	// Release exclusive access.
	Unlock();

	// Wait for the loop to stop.
	int nErr = pthread_join (m_oThreadInfo, NULL);
	if (nErr != 0)
		mjpeg_error_exit1 ("pthread_join() failed: %s", strerror (nErr));
}



// The thread function.
void *
BasicThread::WorkLoop (void *a_pThread)
{
	// Make sure they gave us a thread object.
	assert (a_pThread != NULL);

	// Call the real work loop.
	(static_cast<BasicThread *>(a_pThread))->WorkLoop();

	// Clean up after WorkLoop() exits.
	(static_cast<BasicThread *>(a_pThread))->WorkLoopExit();

	// Mandatory return value.
	return NULL;
}



// Clean up after WorkLoop() exits.
// To be called only by the static WorkLoop().
void
BasicThread::WorkLoopExit (void)
{
	// If the work loop stopped due of an error, clean up.
	// This is similar to Shutdown(), but without the pthread_join().
	if (m_nWorkRetval != Y4M_OK)
	{
		// Get exclusive access.
		Lock();

		// Tell the thread to stop looping.
		m_bWorkLoop = false;

		// If anyone is waiting, stop them.  They should notice
		// that m_bWorkLoop is now false (or that m_nWorkRetval
		// is not Y4M_OK) and gracefully exit.
		if (m_bWaitingForInput)
			SignalInput();
		if (m_bWaitingForOutput)
			SignalOutput();

		// Release exclusive access.
		Unlock();
	}
}



// The thread function.  Loop, calling Work(), until told to stop.
void
BasicThread::WorkLoop (void)
{
	// Loop and do work until told to quit.
	while (m_bWorkLoop)
	{
		// Do work.
		m_nWorkRetval = Work();

		// If it returned non-OK, stop.
		if (m_nWorkRetval != Y4M_OK)
			break;
	}
}



// The DenoiserThread class.



// Default constructor.
DenoiserThread::DenoiserThread()
{
	// No work done yet.
	m_eWorkStatus = m_kWaitingForFrame;
}



// Destructor.
DenoiserThread::~DenoiserThread()
{
	// Nothing additional to do.
}



// The thread function.  Loop, calling Work(), until told to stop.
void
DenoiserThread::WorkLoop (void)
{
	// Loop and do work until told to quit.
	while (m_bWorkLoop)
	{
		// Wait for an input frame.
		Lock();
		if (m_eWorkStatus != m_kGivenFrame)
			WaitForInput();
		bool bWorkLoop = m_bWorkLoop;
		Unlock();

		// If there's no more work to do, stop.
		if (!bWorkLoop)
			break;

		// Do work.
		assert (m_eWorkStatus == m_kGivenFrame);
		m_nWorkRetval = Work();

		// Signal that there's output.
		Lock();
		m_eWorkStatus = m_kFinishedFrame;
		if (m_bWaitingForOutput)
			SignalOutput();
		Unlock();
	}
}



// Add a frame to the denoiser.
void
DenoiserThread::AddFrame (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure they're waiting for a frame.
	assert (m_eWorkStatus == m_kWaitingForFrame);

	// Now they've been given a frame.
	m_eWorkStatus = m_kGivenFrame;

	// Signal the availability of input.
	if (m_bWaitingForInput)
		SignalInput();
}



// Get the next denoised frame, if any.
void
DenoiserThread::WaitForAddFrame (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure that we know we're working.
	assert (m_eWorkStatus != m_kWaitingForFrame);

	// Wait for the current frame to finish denoising.
	if (m_eWorkStatus != m_kFinishedFrame)
		WaitForOutput();
	assert (m_eWorkStatus == m_kFinishedFrame);

	// Now we're waiting for another frame.
	m_eWorkStatus = m_kWaitingForFrame;
}



// The DenoiserThreadY class.



// Default constructor.
DenoiserThreadY::DenoiserThreadY()
{
	// No input/output buffers yet.
	m_pInputY = NULL;
	m_pOutputY = NULL;
}



// Destructor.
DenoiserThreadY::~DenoiserThreadY()
{
	// Nothing to do.
}



// Initialize.  Set up all private thread data and start the
// worker thread.
void
DenoiserThreadY::Initialize (void)
{
	// Let the base class initialize itself.
	BaseClass::Initialize();
}



// Add a frame to the denoiser.
void
DenoiserThreadY::AddFrame (const uint8_t *a_pInputY, uint8_t *a_pOutputY)
{
	// Make sure they gave us a new frame to denoise.
	assert (a_pInputY != NULL && a_pOutputY != NULL);

	// Get exclusive access.
	Lock();

	// Make sure there's not a current frame already.
	assert (m_pInputY == NULL && m_pOutputY == NULL);

	// Store the parameters.
	m_pInputY = a_pInputY;
	m_pOutputY = a_pOutputY;

	// Signal the availability of input.
	BaseClass::AddFrame();

	// Release exclusive access.
	Unlock();
}



// Get the next denoised frame, if any.
int
DenoiserThreadY::WaitForAddFrame (void)
{
	// Get exclusive access.
	Lock();

	// Make sure we're denoising a frame.
	assert (m_pInputY != NULL && m_pOutputY != NULL);

	// Wait for the current frame to finish denoising.
	BaseClass::WaitForAddFrame();

	// We're done denoising this frame.
	m_pInputY = NULL;
	m_pOutputY = NULL;

	// Release exclusive access.
	Unlock();

	// Let our caller know if there's another frame.
	return m_nWorkRetval;
}



// Denoise the current frame.
int
DenoiserThreadY::Work (void)
{
	// Make sure we're denoising a frame.
	assert (m_pInputY != NULL && m_pOutputY != NULL);

	// Denoise the current frame.
	return ((denoiser.interlaced != 0)
			? newdenoise_interlaced_frame_intensity
			: newdenoise_frame_intensity)
		(m_pInputY, m_pOutputY);
}



// The DenoiserThreadCbCr class.



// Default constructor.
DenoiserThreadCbCr::DenoiserThreadCbCr()
{
	// No input/output buffers yet.
	m_pInputCb = NULL;
	m_pInputCr = NULL;
	m_pOutputCb = NULL;
	m_pOutputCr = NULL;
}



// Destructor.
DenoiserThreadCbCr::~DenoiserThreadCbCr()
{
	// Nothing to do.
}



// Initialize.  Set up all private thread data and start the
// worker thread.
void
DenoiserThreadCbCr::Initialize (void)
{
	// Let the base class initialize itself.
	BaseClass::Initialize();
}



// Add a frame to the denoiser.
void
DenoiserThreadCbCr::AddFrame (const uint8_t *a_pInputCb,
	const uint8_t *a_pInputCr, uint8_t *a_pOutputCb,
	uint8_t *a_pOutputCr)
{
	// Make sure they gave us a frame to denoise.  (Actually,
	// a null input frame means that the end of input has
	// been reached, which is OK, but there always needs to
	// be space for an output frame.)
	assert ((a_pInputCb == NULL && a_pInputCr == NULL)
		|| (a_pInputCb != NULL && a_pInputCr != NULL));
	assert (a_pOutputCb != NULL && a_pOutputCr != NULL);

	// Get exclusive access.
	Lock();

	// Make sure there isn't already a current frame.
	assert (m_pInputCb == NULL && m_pInputCr == NULL
		&& m_pOutputCb == NULL && m_pOutputCr == NULL);
	
	// Store the parameters.
	m_pInputCb = a_pInputCb;
	m_pInputCr = a_pInputCr;
	m_pOutputCb = a_pOutputCb;
	m_pOutputCr = a_pOutputCr;

	// Signal the availability of input.
	BaseClass::AddFrame();

	// Release exclusive access.
	Unlock();
}



// Get the next denoised frame, if any.
int
DenoiserThreadCbCr::WaitForAddFrame (void)
{
	// Get exclusive access.
	Lock();

	// Make sure there's a current frame.  (Actually,
	// a null input frame means that the end of input has
	// been reached, which is OK, but there always needs to
	// be space for an output frame.)
	assert ((m_pInputCb == NULL && m_pInputCr == NULL)
		|| (m_pInputCb != NULL && m_pInputCr != NULL));
	assert (m_pOutputCb != NULL && m_pOutputCr != NULL);

	// Wait for the frame to finish denoising.
	BaseClass::WaitForAddFrame();

	// We're done denoising this frame.
	m_pInputCb = NULL;
	m_pInputCr = NULL;
	m_pOutputCb = NULL;
	m_pOutputCr = NULL;

	// Release exclusive access.
	Unlock();

	// Let our caller know if there's a new frame.
	return m_nWorkRetval;
}



// Denoise the current frame.
int
DenoiserThreadCbCr::Work (void)
{
	// Make sure there's a current frame.  (Actually,
	// a null input frame means that the end of input has
	// been reached, which is OK, but there always needs to
	// be space for an output frame.)
	assert ((m_pInputCb == NULL && m_pInputCr == NULL)
		|| (m_pInputCb != NULL && m_pInputCr != NULL));
	assert (m_pOutputCb != NULL && m_pOutputCr != NULL);

	// Denoise the current frame.
	return ((denoiser.interlaced != 0)
			? newdenoise_interlaced_frame_color
			: newdenoise_frame_color)
		(m_pInputCb, m_pInputCr, m_pOutputCb, m_pOutputCr);
}



// The ReadWriteThread class.



// Default constructor.
ReadWriteThread::ReadWriteThread()
{
	// No input stream yet.
	m_nFD = -1;
	m_pStreamInfo = NULL;
	m_pFrameInfo = NULL;

	// No space for frames yet.
	m_apFrames = NULL;
	m_pValidFramesHead = m_pValidFramesTail = m_pCurrentFrame
		= m_pFreeFramesHead = NULL;
}



// Destructor.
ReadWriteThread::~ReadWriteThread()
{
	// Free up all the frames.
	if (m_apFrames != NULL)
	{
		for (int i = 0; i < m_kNumFrames; ++i)
		{
			delete[] m_apFrames[i].planes[0];
			delete[] m_apFrames[i].planes[1];
			delete[] m_apFrames[i].planes[2];
		}
		delete[] m_apFrames;
	}
}



// Initialize.  Set up all private thread data and start the
// worker thread.
void
ReadWriteThread::Initialize (int a_nFD,
	const y4m_stream_info_t *a_pStreamInfo,
	y4m_frame_info_t *a_pFrameInfo, int a_nWidthY, int a_nHeightY,
	int a_nWidthCbCr, int a_nHeightCbCr)
{
	int i, nSizeY, nSizeCbCr;

	// Make sure we got stream/frame info.
	assert (a_pStreamInfo != NULL);
	assert (a_pFrameInfo != NULL);

	// Fill in the blanks.
	m_nFD = a_nFD;
	m_pStreamInfo = a_pStreamInfo;
	m_pFrameInfo = a_pFrameInfo;

	// Allocate space for frames.
	m_apFrames = new Frame[m_kNumFrames];
	nSizeY = a_nWidthY * a_nHeightY;
	assert (nSizeY > 0);
	nSizeCbCr = a_nWidthCbCr * a_nHeightCbCr;
	for (i = 0; i < m_kNumFrames; ++i)
	{
		// Allocate space for each frame.  Don't allocate space for
		// color unless we're denoising color.
		m_apFrames[i].planes[0] = new uint8_t[nSizeY];
		if (nSizeCbCr > 0)
		{
			m_apFrames[i].planes[1] = new uint8_t[nSizeCbCr];
			m_apFrames[i].planes[2] = new uint8_t[nSizeCbCr];
		}
		else
		{
			// Space must be allocated anyway, since the incoming
			// frames have color information that needs to be read
			// in (even though it's not used).
			m_apFrames[i].planes[1] = new uint8_t[denoiser.frame.Cw
				* denoiser.frame.Ch];
			m_apFrames[i].planes[2] = new uint8_t[denoiser.frame.Cw
				* denoiser.frame.Ch];
		}

		// Put each new frame into the free list.
		m_apFrames[i].next = m_pFreeFramesHead;
		m_pFreeFramesHead = &(m_apFrames[i]);
	}

	// Let the base class initialize.  The thread will start.
	BaseClass::Initialize();
}



// Remove the first valid frame from the list and return it.
ReadWriteThread::Frame *
ReadWriteThread::GetFirstValidFrame (void)
{
	Frame *pFrame;
		// The frame we get.

	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure there's a valid frame.
	assert (m_pValidFramesHead != NULL);

	// Make the first valid frame the current frame.
	pFrame = m_pValidFramesHead;

	// Remove that frame from the valid-frame list.
	m_pValidFramesHead = m_pValidFramesHead->next;
	if (m_pValidFramesHead == NULL)
		m_pValidFramesTail = NULL;
	
	// Return the next valid frame.
	return pFrame;
}



// Add the given frame to the end of the valid-frame list.
void
ReadWriteThread::AddFrameToValidList (Frame *a_pFrame)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure they gave us a frame to add.
	assert (a_pFrame != NULL);

	// This will be at the end of the list.
	a_pFrame->next = NULL;

	// If the valid-frame list is empty, set it up.
	if (m_pValidFramesHead == NULL)
	{
		m_pValidFramesHead = m_pValidFramesTail = a_pFrame;
	}

	// Otherwise, append this frame to the end of the list.
	else
	{
		assert (m_pValidFramesTail->next == NULL);
		m_pValidFramesTail->next = a_pFrame;
		m_pValidFramesTail = m_pValidFramesTail->next;
	}
}



// Remove a frame from the free-frames list and return it.
ReadWriteThread::Frame *
ReadWriteThread::GetFreeFrame (void)
{
	Frame *pFrame;
		// The frame we get.

	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure there's a free frame.
	assert (m_pFreeFramesHead != NULL);

	// Make the first free frame the current frame.
	pFrame = m_pFreeFramesHead;

	// Remove that frame from the free-frames list.
	m_pFreeFramesHead = m_pFreeFramesHead->next;

	// Return the free frame.
	return pFrame;
}



// Add the given frame to the free-frames list.
void
ReadWriteThread::AddFrameToFreeList (Frame *a_pFrame)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure they gave us a frame.
	assert (a_pFrame != NULL);

	// Add this frame to the list.
	a_pFrame->next = m_pFreeFramesHead;
	m_pFreeFramesHead = a_pFrame;
}



// Remove the first valid frame, and make it the current frame.
void
ReadWriteThread::MoveValidFrameToCurrent (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure there's a valid frame.
	assert (m_pValidFramesHead != NULL);

	// Make sure there's no current frame.
	assert (m_pCurrentFrame == NULL);

	// Make the first valid frame the current frame.
	m_pCurrentFrame = GetFirstValidFrame();
}



// Move the current frame to the end of the valid-frame list.
void
ReadWriteThread::MoveCurrentFrameToValidList (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure there's a current frame.
	assert (m_pCurrentFrame != NULL);

	// Add the frame to the end of the valid-frame list.
	AddFrameToValidList (m_pCurrentFrame);

	// Now there's no current frame.
	m_pCurrentFrame = NULL;
}



// Remove a frame from the free list, and make it the current frame.
void
ReadWriteThread::MoveFreeFrameToCurrent (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure there's a free frame.
	assert (m_pFreeFramesHead != NULL);

	// Make sure there's no current frame.
	assert (m_pCurrentFrame == NULL);

	// Make the first free frame the current frame.
	m_pCurrentFrame = GetFreeFrame();
}



// Move the current frame to the free-frame list.
void
ReadWriteThread::MoveCurrentFrameToFreeList (void)
{
	// Make sure we have exclusive access.
	assert (IsLocked());

	// Make sure there's a current frame.
	assert (m_pCurrentFrame != NULL);

	// Add this frame to the list.
	AddFrameToFreeList (m_pCurrentFrame);

	// Now there's no current frame.
	m_pCurrentFrame = NULL;
}



// The DenoiserThreadRead class.



// Default constructor.
DenoiserThreadRead::DenoiserThreadRead()
{
	// Nothing additional to do.
}



// Destructor.
DenoiserThreadRead::~DenoiserThreadRead()
{
	// Nothing additional to do.
}



// Read a frame from input.  a_apPlanes[] gets backpatched
// with pointers to valid frame data, and they are valid until
// the next call to ReadFrame().
// Returns Y4M_OK if it succeeds, Y4M_ERR_EOF at the end of
// the stream.  (Returns other errors too.)
int
DenoiserThreadRead::ReadFrame (uint8_t **a_apPlanes)
{
	// Get exclusive access.
	Lock();

	// Any previous current frame can be reused now.
	if (m_pCurrentFrame != NULL)
	{
		// Now recycle the previous current frame.
		MoveCurrentFrameToFreeList();

		// If there were no free frames before, then signal that input
		// can continue.
		if (m_bWaitingForInput)
			SignalInput();
	}
	
	// If there are no valid frames, and the thread is still reading
	// frames, then wait for some valid output.
	if (m_pValidFramesHead == NULL && m_bWorkLoop)
		WaitForOutput();

	// Make the next valid frame the current frame.  If there are no
	// valid frames at this point, then we're at the end of the stream.
	if (m_pValidFramesHead != NULL)
		MoveValidFrameToCurrent();
	
	// Remember the current frame.  (It could conceivably get changed
	// once we release exclusive access.  The code, as presently written,
	// doesn't do that, but I like to plan for random future extensions.)
	Frame *pCurrentFrame = m_pCurrentFrame;

	// Release exclusive access.
	Unlock();

	// Backpatch the frame info.
	if (pCurrentFrame != NULL)
	{
		a_apPlanes[0] = pCurrentFrame->planes[0];
		a_apPlanes[1] = pCurrentFrame->planes[1];
		a_apPlanes[2] = pCurrentFrame->planes[2];
		return Y4M_OK;
	}

	// Make sure we got an error at the end of stream (hopefully
	// Y4M_ERR_EOF).
	assert (m_nWorkRetval != Y4M_OK);

	// Return whatever error we got at the end of the stream.
	return m_nWorkRetval;
}



// Read frames from the raw-video stream.
int
DenoiserThreadRead::Work (void)
{
	int nErr;
		// An error that may occur.
	Frame *pFrame;
		// Space for reading a frame into memory.

	// No errors yet.
	nErr = Y4M_OK;

	// No frame yet.
	pFrame = NULL;

	// Get exclusive access.
	Lock();

	// Are there no free buffers?
	if (m_pFreeFramesHead == NULL)
	{
		// If we've been asked to quit, do so.
		if (!m_bWorkLoop)
			nErr = Y4M_ERR_EOF;
		
		// Otherwise, wait for space to read in frames.
		else
			WaitForInput();
	}

	// If there is still no space to read in frames, we're done.
	if (nErr == Y4M_OK && m_pFreeFramesHead == NULL)
		nErr = Y4M_ERR_EOF;

	// Otherwise, get a free buffer.
	if (nErr == Y4M_OK)
		pFrame = GetFreeFrame();

	// Release exclusive access.
	Unlock();

	// If there's space to read in another frame, do so.
	if (nErr == Y4M_OK && pFrame != NULL)
	{
		// Read the next frame into the buffer.
		nErr = y4m_read_frame (m_nFD, m_pStreamInfo, m_pFrameInfo,
		    pFrame->planes);
	
		// Get exclusive access.
		Lock();
	
		// Did we successfully read a frame?
		if (nErr == Y4M_OK)
		{
			// Yes.  Put the frame into the valid-frames list.
			AddFrameToValidList (pFrame);
	
			// If there are no other valid frames, then signal
			// that there's some output now.
			if (m_bWaitingForOutput)
				SignalOutput();
			
		}
		else
		{
			// No.  Put the frame back into the free-frames list.
			AddFrameToFreeList (pFrame);
		}
	
		// Release exclusive access.
		Unlock();
	}

	// Return whether we successfully read a frame.
	return nErr;
}



// The DenoiserThreadWrite class.



// Default constructor.
DenoiserThreadWrite::DenoiserThreadWrite()
{
	// Nothing additional to do.
}



// Destructor.
DenoiserThreadWrite::~DenoiserThreadWrite()
{
	// Nothing additional to do.
}



// Get space for a frame to write to output.  a_apPlanes[] gets
// backpatched with pointers to valid frame data.
// Returns Y4M_OK if it succeeds, something else if it fails.
int
DenoiserThreadWrite::GetSpaceToWriteFrame (uint8_t **a_apPlanes)
{
	// Get exclusive access.
	Lock();

	// Make sure there's no current frame.
	assert (m_pCurrentFrame == NULL);

	// If there are no free frames, and the thread is still writing
	// frames, then wait for it to free up a frame.
	if (m_pFreeFramesHead == NULL && m_bWorkLoop)
		WaitForInput();
	
	// Make a free frame the current frame.
	if (m_pFreeFramesHead != NULL)
		MoveFreeFrameToCurrent();
	
	// Remember the current frame.  (It could conceivably get changed
	// once we release exclusive access.  The code, as presently written,
	// doesn't do that, but I like to plan for random future extensions.)
	Frame *pCurrentFrame = m_pCurrentFrame;

	// Release exclusive access.
	Unlock();

	// Backpatch the frame info, for all the info we're denoising.
	if (pCurrentFrame != NULL)
	{
		a_apPlanes[0] = pCurrentFrame->planes[0];
		a_apPlanes[1] = pCurrentFrame->planes[1];
		a_apPlanes[2] = pCurrentFrame->planes[2];
		return Y4M_OK;
	}

	// Make sure we got an error at the end of stream (hopefully
	// Y4M_ERR_EOF).
	assert (m_nWorkRetval != Y4M_OK);

	// Return whatever error we got at the end of the stream.
	return m_nWorkRetval;
}



// Write a frame to output.  The a_apPlanes[] previously set up by
// GetSpaceToWriteFrame() must be filled with video data by the client.
void
DenoiserThreadWrite::WriteFrame (void)
{
	// Get exclusive access.
	Lock();

	// Make sure there's a current frame.
	assert (m_pCurrentFrame != NULL);

	// Move it to the end of the valid-frames list.
	MoveCurrentFrameToValidList();

	// If that's the only valid frame, then signal that we're ready
	// for output again.
	if (m_bWaitingForOutput)
		SignalOutput();

	// Release exclusive access.
	Unlock();
}



// The thread function.  Loop, calling Work(), until told to stop.
// Differs from BasicThread's implementation because it waits until
// all frames are written before it stops.
void
DenoiserThreadWrite::WorkLoop (void)
{
	// Loop and do work until told to quit.
	for (;;)
	{
		// Determine if we should continue looping.
		// This requires exclusive access because of the test
		// for remaining frames to write.
		bool bContinue;
		Lock();
		bContinue = (m_bWorkLoop || m_pValidFramesHead != NULL);
		Unlock();
		if (!bContinue)
			break;

		// Do work.
		m_nWorkRetval = Work();

		// If it returned non-OK, stop.
		if (m_nWorkRetval != Y4M_OK)
			break;
	}
}



// Write frames to the raw-video stream.
int
DenoiserThreadWrite::Work (void)
{
	int nErr;
		// An error that may occur.
	Frame *pFrame;
		// A frame that gets written out.

	// No errors yet.
	nErr = Y4M_OK;

	// No frame yet.
	pFrame = NULL;

	// Get exclusive access.
	Lock();

	// Are there no frames to write out?
	if (m_pValidFramesHead == NULL)
	{
		// If we've been asked to quit, do so.
		if (!m_bWorkLoop)
			nErr = Y4M_ERR_EOF;

		// Otherwise, wait for some frames to write out.
		else
			WaitForOutput();
	}
	
	// If there are still no frames to write out, we're done.
	if (nErr == Y4M_OK && m_pValidFramesHead == NULL)
		nErr = Y4M_ERR_EOF;
	
	// Otherwise, fetch a frame to write to output.
	if (nErr == Y4M_OK)
		pFrame = GetFirstValidFrame();
	
	// Release exclusive access.
	Unlock();

	// If there's a frame to write to output, do so.
	if (nErr == Y4M_OK && pFrame != NULL)
	{
		// Write the frame to output.
		nErr = y4m_write_frame (m_nFD, m_pStreamInfo, m_pFrameInfo,
			pFrame->planes);

		// Whether or not that succeeds, put the frame into the
		// free-frames list.
		Lock();
		AddFrameToFreeList (pFrame);
		if (m_bWaitingForInput)
			SignalInput();
		Unlock();
	}

	// Let our caller know what happened.
	return nErr;
}
