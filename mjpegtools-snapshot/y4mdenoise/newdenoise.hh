#ifndef __NEWDENOISE_H__
#define __NEWDENOISE_H__

// This file (C) 2004-2009 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License v2.
// See the file COPYING for more information.

#ifdef  __cplusplus
extern "C"
{
#endif /* _cplusplus */

/* Initialize the new denoiser.  a_nFrames is the number of frames over
   which to calculate pixel values.  a_nWidthY/a_nHeightY is the
   dimension of the intensity portion of the image; if intensity is not
   to be denoised, then these values must be 0.
   a_nWidthCbCr/a_nHeightCbCr is the dimension of the color portion of
   the image; if color is not to be denoised, then these values must be
   0.
   Returns 0 if successful, -1 if there was some problem (usually,
   running out of memory). */
int newdenoise_init (int a_nFrames, int a_nWidthY, int a_nHeightY,
	int a_nWidthCbCr, int a_nHeightCbCr, int a_nInputFD,
	int a_nOutputFD, const y4m_stream_info_t *a_pStreamInfo,
	y4m_frame_info_t *a_pFrameInfo);

/* Shutdown the new denoiser.
   Returns 0 if successful, -1 if there was some problem. */
int newdenoise_shutdown (void);

/* Read another frame.  Usable only in multi-threaded situations. */
int newdenoise_read_frame (uint8_t **a_apPlanes);

/* Get space to write another frame.  Usable only in multi-threaded
   situations. */
int newdenoise_get_write_frame (uint8_t **a_apPlanes);

/* Write another frame.  Usable only in multi-threaded situations. */
int newdenoise_write_frame (void);

/* Denoise another frame.  a_pInputY/a_pInputCb/a_pInputCr point to the
   incoming undenoised frame.
   Any components that are not to be denoised, i.e. because their
   dimension was set to 0x0 by newdenoise_init(), will not be used, and
   may be any value, e.g. NULL.
   If a component that is to be denoised is NULL, then that indicates
   that the end of input has been reached, and the next denoised frame
   should be returned.
   Returns 0 if a_pOutputY/a_pOutputCb/a_pOutputCr contains the next
   denoised frame, 1 if there was no output written, -1 if there was
   some error (usually, running out of memory). */
int newdenoise_frame (const uint8_t *a_pInputY,
	const uint8_t *a_pInputCb, const uint8_t *a_pInputCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr);
int newdenoise_interlaced_frame (const uint8_t *a_pInputY,
	const uint8_t *a_pInputCb, const uint8_t *a_pInputCr,
	uint8_t *a_pOutputY, uint8_t *a_pOutputCb, uint8_t *a_pOutputCr);

/* Denoiser configuration. */
typedef struct DNSR_GLOBAL
{
	int frames;				/* # of frames over which to average */
	int interlaced;			/* 0 == not interlaced, 1 == interlaced */
	int bwonly;				/* 1 if we're to denoise intensity only */
	int radiusY;			/* search radius for intensity */
	int radiusCbCr;			/* search radius for color */
	int zThresholdY;		/* zero-motion intensity error threshold */
	int zThresholdCbCr;		/* zero-motion color error threshold */
	int thresholdY;			/* intensity error threshold */
	int thresholdCbCr;		/* color error threshold */
	int matchCountThrottle;	/* match throttle on count */
	int matchSizeThrottle;	/* match throttle on size */
	int threads;			/* bit 0=rw only, bit 1=color in parallel */
	struct
	{
		int w, h;			/* width/height of intensity frame */
		int Cw, Ch;			/* width/height of color frame */
		int ss_h, ss_v;		/* ratio between intensity/color sizes */
		uint8_t *in[3];		/* frame data read from stdin */
		uint8_t *out[3];	/* frame data written to stdout */
	} frame;
} DNSR_GLOBAL;

extern DNSR_GLOBAL denoiser;

#ifdef  __cplusplus
};
#endif /* _cplusplus */

#endif // __NEWDENOISE_H__
