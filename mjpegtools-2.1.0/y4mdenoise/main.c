/***********************************************************
 * A new denoiser for the mjpegtools project               *
 * ------------------------------------------------------- *
 * (C) 2004-2009 Steven Boswell.                                *
 * Based on yuvdenoise/main.c, (C) 2001,2002 Stefan Fendt  *
 *                                                         *
 * Licensed and protected by the GNU-General-Public-       *
 * License version 2. See the file COPYING for detailed    *
 * information.                                            *
 ***********************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "mjpeg_types.h"
#include "yuv4mpeg.h"
#include "mjpeg_logging.h"
#include "config.h"
#include "newdenoise.hh"

// HACK
#include <sys/stat.h>
#include <fcntl.h>

void process_commandline(int argc, char *argv[]);
void alloc_buffers (void);
void display_help (void);

DNSR_GLOBAL denoiser;
int frame = 0;
int verbose = 1;

/***********************************************************
 *                                                         *
 ***********************************************************/

int main(int argc, char *argv[])
{
  int fd_in  = 0;
  int fd_out = 1;
  int errno  = 0;
  
  y4m_frame_info_t frameinfo;
  y4m_stream_info_t streaminfo;
  int chroma_mode;
  int bInputStreamEnded;

  frame = 0;

  /* initialize stream-information */
  y4m_accept_extensions (1);
  y4m_init_stream_info (&streaminfo);
  y4m_init_frame_info (&frameinfo);
  
  /* setup default values for denoiser's parameters */
  denoiser.frames             = 10;
  denoiser.interlaced         = -1;
  denoiser.bwonly             = 0;
  denoiser.radiusY            = 16;
  denoiser.radiusCbCr         = -1;
  denoiser.zThresholdY        = 0; /* assume medium noise material */
  denoiser.zThresholdCbCr     = -1;
  denoiser.thresholdY         = 3; /* assume medium noise material */
  denoiser.thresholdCbCr      = -1;
  denoiser.matchCountThrottle = 16;
  denoiser.matchSizeThrottle  = 256;
  denoiser.threads            = 1;
  
  /* process commandline */
  process_commandline(argc, argv);

  /* If color-specific settings were not specified, copy them
     from the intensity settings. */
  if (denoiser.radiusCbCr == -1)
  	denoiser.radiusCbCr = denoiser.radiusY;
  if (denoiser.thresholdCbCr == -1)
  	denoiser.thresholdCbCr = denoiser.thresholdY;
  if (denoiser.zThresholdCbCr == -1)
  	denoiser.zThresholdCbCr = denoiser.zThresholdY;

	/* Make sure the parameters are consistent. */
	{
		int isConsistent = 1;

		/* The zero-motion error threshold has to be lower than the
		   motion-compensated error threshold.  The zero-motion
		   pass is just a way to speed up detection of unambiguous
		   zero-motion areas, so a higher error threshold would defeat
		   the purpose. */
		if (denoiser.zThresholdY > denoiser.thresholdY)
		{
			mjpeg_error ("-z setting (%d) cannot be larger than "
				"-t setting (%d)\n", denoiser.zThresholdY,
				denoiser.thresholdY);
			isConsistent = 0;
		}
		if (denoiser.zThresholdCbCr > denoiser.thresholdCbCr)
		{
			mjpeg_error ("-Z setting (%d) cannot be larger than "
				"-T setting (%d)\n", denoiser.zThresholdCbCr,
				denoiser.thresholdCbCr);
			isConsistent = 0;
		}

		/* If the parameters weren't consistent, quit with an error. */
		if (!isConsistent)
			exit (1);
	}

	/* open input stream */
	if ((errno = y4m_read_stream_header (fd_in, &streaminfo)) != Y4M_OK)
		mjpeg_error_exit1 ("Couldn't read YUV4MPEG header: %s!",
			y4m_strerr (errno));
	if (y4m_si_get_plane_count (&streaminfo) != 3)
		mjpeg_error_exit1 ("Only 3-plane formats supported.");
	denoiser.frame.w = y4m_si_get_width (&streaminfo);
	denoiser.frame.h = y4m_si_get_height (&streaminfo);
	chroma_mode = y4m_si_get_chroma (&streaminfo);
	denoiser.frame.ss_h = y4m_chroma_ss_x_ratio (chroma_mode).d;
	denoiser.frame.ss_v = y4m_chroma_ss_y_ratio (chroma_mode).d;
	denoiser.frame.Cw=denoiser.frame.w/denoiser.frame.ss_h;
	denoiser.frame.Ch=denoiser.frame.h/denoiser.frame.ss_v;

  /* write the outstream header */
  y4m_write_stream_header (fd_out, &streaminfo); 

  /* allocate memory for frames */
  if (!(denoiser.threads & 1))
  	alloc_buffers();
	
  /* set interlacing, if it hasn't been yet. */
  if (denoiser.interlaced == -1)
  {
    int n = y4m_si_get_interlace(&streaminfo);
    switch (n)
    {
    case Y4M_ILACE_NONE:
         denoiser.interlaced = 0;
         break;
    case Y4M_ILACE_TOP_FIRST:
         denoiser.interlaced = 1;
         break;
    case Y4M_ILACE_BOTTOM_FIRST:
         denoiser.interlaced = 2;
         break;
    default:
         mjpeg_warn("Unknown interlacing '%d', assuming non-interlaced", n);
         denoiser.interlaced = 0;
         break;
    }
  }

  /* if we're processing interlaced material, there is an additional
     requirement that the number of reference frames be a multiple of
	 2 (since there are 2 fields per frame). */
  if (denoiser.interlaced != 0 && (denoiser.frames & 1) != 0)
	mjpeg_error_exit1 ("When denoising interlaced material, -f must be"
		" a multiple of 2");

	/* initialize the denoiser */
	errno = newdenoise_init (denoiser.frames, denoiser.frame.w,
		denoiser.frame.h, (denoiser.bwonly) ? 0 : denoiser.frame.Cw,
		(denoiser.bwonly) ? 0 : denoiser.frame.Ch, fd_in, fd_out,
		&streaminfo, &frameinfo);
	if (errno == -1)
		mjpeg_error_exit1( "Could not initialize denoiser");

	/* get space for the first output frame, if necessary */
	if (denoiser.threads & 1)
	{
		errno = newdenoise_get_write_frame (denoiser.frame.out);
		if (errno)
			mjpeg_error_exit1 ("Could not get space for frame %d",
				frame);
	}

  /* read every single frame until the end of the input stream */
  bInputStreamEnded = 0;
  for (;;)
  {
	/* If there is another input frame, read it.  We may find there
	is no more input. */
	if (!bInputStreamEnded)
	{
		/* Read the next frame. */
		if (!(denoiser.threads & 1))
			errno = y4m_read_frame (fd_in, &streaminfo, &frameinfo, 
				denoiser.frame.in);
		else
			errno = newdenoise_read_frame (denoiser.frame.in);

		/* did stream end unexpectedly ? */
		if(errno != Y4M_ERR_EOF && errno != Y4M_OK )
			mjpeg_error_exit1( "%s", y4m_strerr( errno ) );

		/* Note if the stream ended. */
		if (errno == Y4M_ERR_EOF)
			bInputStreamEnded = 1;
	}

	  /* One more input frame. */
	  frame++;

	  //if (frame < 5395) continue;	// MAJOR HACK MAJOR HACK MAJOR HACK
	  //if (frame == 5455) break;	// MAJOR HACK MAJOR HACK MAJOR HACK
	  // fprintf (stderr, "Frame %d\r", frame);	// HACK

		/* denoise the current frame */
		errno = ((denoiser.interlaced == 0) ? newdenoise_frame
			: newdenoise_interlaced_frame)
			(((bInputStreamEnded) ? NULL : denoiser.frame.in[0]),
			((bInputStreamEnded) ? NULL : denoiser.frame.in[1]),
			((bInputStreamEnded) ? NULL : denoiser.frame.in[2]),
			denoiser.frame.out[0], denoiser.frame.out[1],
			denoiser.frame.out[2]);
		if (errno == -1)
			mjpeg_error_exit1( "Could not denoise frame %d", frame);

    	/* if there was no output from the denoiser, leave. */
		if (bInputStreamEnded && errno == 1)
			break;
	      
		/* if there was some output from the denoiser, write it. */
		if (errno == 0)
		{
			/* if b/w was selected, set the frame color to white */
			if (denoiser.bwonly)
			{
				int i, iExtent = denoiser.frame.Cw * denoiser.frame.Ch;
	
				for (i = 0; i < iExtent; ++i)
					denoiser.frame.out[1][i] = 128;
				for (i = 0; i < iExtent; ++i)
					denoiser.frame.out[2][i] = 128;
			}

			/* Write the frame. */
			if (!(denoiser.threads & 1))
	    		errno = y4m_write_frame ( fd_out, &streaminfo,
					&frameinfo, denoiser.frame.out);
			else
				errno = newdenoise_write_frame();
			if (errno)
				mjpeg_error_exit1 ("Could not write frame %d", frame);

			/* Get space for the next frame.*/
			if (denoiser.threads & 1)
			{
				errno = newdenoise_get_write_frame (denoiser.frame.out);
				if (errno)
					mjpeg_error_exit1 ("Could not get space for "
						"frame %d", frame);
			}
		}
  }

  /* shut down the denoiser */
  errno = newdenoise_shutdown();
  if (errno != 0)
	mjpeg_error_exit1 ("Could not shut down denoiser");
  
  /* Exit gently */
  return(0);
}



void
process_commandline(int argc, char *argv[])
{
  char c;

  while ((c = getopt (argc, argv, "h?z:Z:t:T:r:R:m:M:f:BI:p:v:")) != -1)
  {
    switch (c)
    {
      case 'h':
      {
        display_help();
        exit (0);
        break;
      }
      case '?':
      {
        display_help();
        exit (0);
        break;
      }
      case 'r':
      {
        denoiser.radiusY = atoi(optarg);
        if(denoiser.radiusY<4)
        {
          denoiser.radiusY=4;
  	      mjpeg_warn ("Minimum allowed search radius is 4 pixels.");
        }
        break;
	  }
      case 'R':
      {
        denoiser.radiusCbCr = atoi(optarg);
        if(denoiser.radiusCbCr<4)
        {
          denoiser.radiusCbCr=4;
  	      mjpeg_warn ("Minimum allowed color search radius is 4 pixel.");
        }
        break;
      }
      case 'z':
      {
        denoiser.zThresholdY = atoi(optarg);
        break;
      }
      case 'Z':
      {
        denoiser.zThresholdCbCr = atoi(optarg);
        break;
      }
      case 't':
      {
        denoiser.thresholdY = atoi(optarg);
        break;
      }
      case 'T':
      {
        denoiser.thresholdCbCr = atoi(optarg);
        break;
      }
      case 'm':
      {
        denoiser.matchCountThrottle = atoi(optarg);
        break;
      }
      case 'M':
      {
        denoiser.matchSizeThrottle = atoi(optarg);
        break;
      }
      case 'f':
      {
        denoiser.frames = atoi(optarg);
        break;
      }
      case 'B':
      {
        denoiser.bwonly = 1;
        break;
      }
      case 'I':
      {
	 	int interlaced = atoi (optarg);
		if (interlaced != 0 && interlaced != 1 && interlaced != 2)
		{
      		mjpeg_error_exit1 ("-I must be either 0, 1, or 2");
		}
        denoiser.interlaced = interlaced;
        break;
      }
      case 'p':
      {
	 	int threads = atoi (optarg);
		if (threads < 0 || threads > 3)
		{
      		mjpeg_error_exit1 ("-p must be either 0, 1, 2, or 3");
		}
        denoiser.threads = threads;
        break;
      }
      case 'v':
        verbose = atoi (optarg);
        if (verbose < 0 || verbose > 2)
        {
          mjpeg_error_exit1 ("Verbose level must be [0..2]");
        }
        break;
	default:
	  fprintf (stderr, "Unknown option '%c'\n", c);
	  display_help();
	  exit (1);
    }
  }
}


void alloc_buffers(void)
{
  int luma_buffsize; 
  int chroma_buffsize;

  luma_buffsize = denoiser.frame.w * denoiser.frame.h;
  chroma_buffsize = denoiser.frame.Cw * denoiser.frame.Ch;
  
  denoiser.frame.in[0] = (uint8_t *) malloc (luma_buffsize);
  denoiser.frame.in[1] = (uint8_t *) malloc (chroma_buffsize);
  denoiser.frame.in[2] = (uint8_t *) malloc (chroma_buffsize);
  
  denoiser.frame.out[0] = (uint8_t *) malloc (luma_buffsize);
  denoiser.frame.out[1] = (uint8_t *) malloc (chroma_buffsize);
  denoiser.frame.out[2] = (uint8_t *) malloc (chroma_buffsize);

	if ( denoiser.frame.in[0] == NULL
	|| denoiser.frame.in[1] == NULL
	|| denoiser.frame.in[2] == NULL
	|| denoiser.frame.out[0] == NULL
	|| denoiser.frame.out[1] == NULL
	|| denoiser.frame.out[2] == NULL)
		mjpeg_error_exit1( "Out of memory "
			"when allocating frame buffers");
}

void
display_help (void)
{
	fprintf (stderr,
	"y4mdenoise options\n"
	"------------------\n"
	"-p    parallelism: 0=no threads, 1=r/w thread only, 2=do color in\n"
	"      separate thread (default: 1)\n"
	"-r    Radius for motion-search (default: 16)\n"
	"-R    Radius for color motion-search (default: -r setting)\n"
	"-t    Error tolerance (default: 3)\n"
	"-T    Color error tolerance (default: -t setting)\n"
	"-z    Error tolerance for zero-motion pass (default: 2)\n"
	"-Z    Error tolerance for color's zero-motion pass (default: -z setting)\n"
	"-m    Match-count throttle (keep this many of the best pixel-group\n"
	"      matches found in a radius search) (default: 15)\n"
	"-M    Match-size throttle (apply first match whose flood-fill is the\n"
	"      size of this many pixel-groups or greater) (default: 3)\n"
	"-f    Number of reference frames (default: 10)\n"
	"-B    Black-and-white mode; denoise intensity, set color to white\n"
	"-I    Interlacing type: 0=frame, 1=top-field-first, 2=bottom-field-first\n"
	"      (default: taken from stream header)\n"
	"-v    Verbosity (0=none, 1=normal 2=debug)\n"
	"-h,-? Help\n"
	"Intensity pixel-groups are 4x2 (i.e. 4 across and 2 down).\n"
	"Color pixel-groups are 2x2.\n"
	);
}
