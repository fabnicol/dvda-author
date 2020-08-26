/*
  *  yuvfps.c
  *  Copyright (C) 2002 Alfonso Garcia-Patiño Barbolani <barbolani@jazzfree.com>
  *
  *  Weighted average resampling
  *  Copyright (C) 2006 Johannes Lehtinen <johannes.lehtinen@iki.fi>
  *
  *  Upsamples or downsamples a yuv stream to a specified frame rate
  *
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation; either version 2 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "yuv4mpeg.h"
#include "mpegconsts.h"

#define YUVFPS_VERSION "0.2"

static void print_usage() 
{
  fprintf (stderr,
	   "usage: yuvfps -r [NewFpsNum:NewFpsDen] [-s [InputFpsNum:InputFpsDen]] [-i NewInterlacing] [-I InputInterlacing] [-c] [-n] [-w] [-v -h]\n"
	   "yuvfps resamples a yuv video stream read from stdin to a new stream, identical\n"
           "to the source with frames repeated/copied/removed written to stdout.\n"
           "Alternatively each output frame/field can be produced as a weighted average\n"
           "of two temporally closest input frames/fields.\n"
           "\n"
           "\t -r Frame rate for the resulting stream (in X:Y fractional form)\n"
           "\t -w Use weighted average of two temporally closest input frames/fields\n"
           "\t -c Change only the output header, does not modify stream\n"
           "\t -s Assume this source frame rate ignoring source YUV header\n"
           "\t -i Interlacing for the resulting stream ('p', 't' or 'b')\n"
           "\t -I Assume this source interlacing ignoring source YUV header\n"
           "\t -n don't try to normalize the input framerate\n"
	   "\t -v Verbosity degree : 0=quiet, 1=normal, 2=verbose/debug\n"
	   "\t -h print this help\n"
         );
}

// Does malloc and checks that the allocation is successful before returning
// the pointer. If allocation fails, prints an error message and exits.
static void *checked_malloc(size_t size)
{
  void		*p;
  
  if ((p = malloc(size)) == NULL)
    mjpeg_error_exit1("Memory allocation failed (%lu bytes needed)",
                      (unsigned long) size);
  return p;
}


/* -----------------------------------------------------------------------
 * Drop/duplicate resampling code
 * ---------------------------------------------------------------------*/

//
// Resamples the video stream coming from fdIn and writes it
// to fdOut.
// There are two variations of the same theme:
//
//   Upsampling: frames are duplicated when needed
//   Downsampling: frames from the original are skipped
//
//   Parameters:
//
//      fdIn,fdOut : File descriptors for reading and writing the stream
//      inStream, outStream: stream handlers for the source and destination streams.
//      It is assumed that they are correctly initialized from/to
//      their respective file I/O streams with the desired sample rates.
//
//      src_frame_rate, frame_rate: ratios for source and destination frame rate
//            (note that this may not match the information contained
//             in the stream itself due to command line options) 
// 
//  In both cases a Bresenham - style algorithm is used.
//
static void resample(  int fdIn 
                      , y4m_stream_info_t  *inStrInfo
                      , y4m_ratio_t src_frame_rate
                      , int fdOut
                      , y4m_stream_info_t  *outStrInfo
                      , y4m_ratio_t frame_rate
                    )
{
  y4m_frame_info_t   in_frame ;
  uint8_t            *yuv_data[Y4M_MAX_NUM_PLANES] ;
  int                read_error_code ;
  int                write_error_code ;
  int                src_frame_counter ;
  int                dest_frame_counter ;
  // To perform bresenham resampling
  long long          srcInc ;
  long long          dstInc ;
  long long          currCount ;
  int i;

  // Allocate memory for the YUV channels
  for (i = 0; i < y4m_si_get_plane_count(inStrInfo); i++)
    yuv_data[i] = (uint8_t *) checked_malloc(y4m_si_get_plane_length(inStrInfo, i));
 
  /* Initialize counters */
  srcInc = (long long)src_frame_rate.n * (long long)frame_rate.d ;
  dstInc = (long long)frame_rate.n * (long long)src_frame_rate.d ;

  write_error_code = Y4M_OK ;

  src_frame_counter = 0 ;
  dest_frame_counter = 0 ;
  y4m_init_frame_info( &in_frame );
  read_error_code = y4m_read_frame(fdIn,inStrInfo,&in_frame,yuv_data );
  ++src_frame_counter ;
  currCount = 0 ;

  while( Y4M_ERR_EOF != read_error_code && write_error_code == Y4M_OK ) {
    write_error_code = y4m_write_frame( fdOut, outStrInfo, &in_frame, yuv_data );
    mjpeg_info( "Writing source frame %d at dest frame %d", src_frame_counter,++dest_frame_counter );
    currCount += srcInc ;
    while( currCount >= dstInc && Y4M_ERR_EOF != read_error_code ) {
      currCount -= dstInc ;
      ++src_frame_counter ;
      y4m_fini_frame_info( &in_frame );
      y4m_init_frame_info( &in_frame );
      read_error_code = y4m_read_frame(fdIn, inStrInfo,&in_frame,yuv_data );
    }
  }
  
  // Clean-up regardless an error happened or not
  y4m_fini_frame_info( &in_frame );
  for (i = 0; i < y4m_si_get_plane_count(inStrInfo); i++) {
    free( yuv_data[i] );
  }

  if( read_error_code != Y4M_ERR_EOF )
    mjpeg_error_exit1 ("Error reading from input stream!");
  if( write_error_code != Y4M_OK )
    mjpeg_error_exit1 ("Error writing output stream!");

}


/* -----------------------------------------------------------------------
 * Weighted average resampling code
 * ---------------------------------------------------------------------*/
//
// Returns the greatest common divisor (GCD) of the input parameters.
//
static int gcd(int a, int b)
{
  if (b == 0)
    return a;
  else
    return gcd(b, a % b);
}

//
// Resamples a video stream by producing each output frame/field as the
// weighted average of the two temporally closest input frames/fields.
// If the stream is interlaced and the input field does not match the output
// field then the input lines are produced as average of the surrounding lines.
//
// Parameters:
//   in_fd, out_fd:   File descriptors for reading and writing the stream
//   in_si, out_si:   stream info for the source and destination streams.
//             It is assumed that they are correctly initialized from/to
//             their respective file I/O streams with the desired sample rates.
//   in_frame_rate, out_frame_rate: ratios for input and output frame rate
//                      (note that this may not match the information contained
//                       in the stream itself due to command line options)
//   in_interlacing, out_interlacing: interlacing modes for input and output
//                      (note that this may not match the information contained
//                       in the stream itself due to command line options)
//
static void resample_wa(  int in_fd
                        , y4m_stream_info_t  *in_si
                        , y4m_ratio_t in_frame_rate
                        , int in_interlacing
                        , int out_fd
                        , y4m_stream_info_t  *out_si
                        , y4m_ratio_t out_frame_rate
                        , int out_interlacing
                    )
{
  y4m_frame_info_t fi;
  int		num_planes;
  int		plane_width[Y4M_MAX_NUM_PLANES];
  int           plane_height[Y4M_MAX_NUM_PLANES];
  int		plane_length[Y4M_MAX_NUM_PLANES];
  int		max_plane_width;
  uint8_t	*in_planes[2][Y4M_MAX_NUM_PLANES];
  uint8_t	*out_planes[Y4M_MAX_NUM_PLANES];
  uint8_t	*intermediate_data[2];
  int		in_frame_time, out_frame_time;
  int		in_pos = 0, out_pos = 0;
  int		num_buffered_frames = 0, buffer_head = 1, buffer_tail = 0;
  int		in_frame_count = 0, out_frame_count = 0;
  int		out_field = 0;
  int		eos = 0;
  int		i, j;

  /* Initialize frame info structure */
  y4m_init_frame_info(&fi);

  /* Read and check plane information */
  num_planes = y4m_si_get_plane_count(in_si);
  max_plane_width = 0;
  for (i = 0; i < num_planes; i++)
  {
    plane_width[i] = y4m_si_get_plane_width(in_si, i);
    plane_height[i] = y4m_si_get_plane_height(in_si, i);
    plane_length[i] = y4m_si_get_plane_length(in_si, i);
    if (plane_width[i] * plane_height[i] != plane_length[i])
      mjpeg_error_exit1("Only 8 bits per plane chroma modes are supported");
    if ((in_interlacing != Y4M_ILACE_NONE || out_interlacing != Y4M_ILACE_NONE)
        && (plane_height[i] & 1))
      mjpeg_error_exit1("Interlaced planes must have even height");
    if (plane_width[i] > max_plane_width)
      max_plane_width = plane_width[i];
  }
  
  /* Allocate memory buffers for processing */
  for (j = 0; j < 2; j++)
    for (i = 0; i < num_planes; i++)
      in_planes[j][i] = checked_malloc(plane_length[i]);
  for (i = 0; i < num_planes; i++)
    out_planes[i] = checked_malloc(plane_length[i]);
  for (i = 0; i < 2; i++)
    intermediate_data[i] = checked_malloc(max_plane_width);
  
  /* Determine exact relative input and output frame (and field) times
   * 
   *   in_time_sec = 1 / in_rate = in_rate_D / in_rate_N
   *   out_time_sec = 1 / out_rate = out_rate_D / out_rate_N
   * 
   *   Let us scale the time by in_rate_N * out_rate_N to get integer times
   * 
   *   in_time_int = in_rate_D * out_rate_N
   *   out_time_int = out_rate_D * in_rate_N
   * 
   *   Finally divide by the greatest common divisor. If necessary, make sure
   *   that field time (= frame time / 2) is an integer as well.
   */
  in_frame_time = in_frame_rate.d * out_frame_rate.n;
  out_frame_time = out_frame_rate.d * in_frame_rate.n;
  i = gcd(in_frame_time, out_frame_time);
  in_frame_time /= i;
  out_frame_time /= i;
  if ((in_interlacing != Y4M_ILACE_NONE && (in_frame_time & 1))
      || (out_interlacing != Y4M_ILACE_NONE && (out_frame_time & 1)))
  {
    in_frame_time *= 2;
    out_frame_time *= 2;
  }
  mjpeg_debug("Relative input frame time is %d and output frame time is %d.",
              in_frame_time, out_frame_time);

  /* Resample until end-of-stream, one output frame/field at a time */
  while (!eos) {
  
    /* Buffer input frames surrounding the current output position */
    while (num_buffered_frames == 0
           || in_pos + in_frame_time <= out_pos
           || (num_buffered_frames < 2
               && (in_interlacing != Y4M_ILACE_NONE
                   ? in_pos + in_frame_time / 2 < out_pos
                   : in_pos != out_pos)))
    {
    	
      /* Try to buffer next frame, if available */
      if ((i = y4m_read_frame(in_fd, in_si, &fi, in_planes[1 - buffer_head])) == Y4M_OK)
      {
      	y4m_clear_frame_info(&fi);
      	if (num_buffered_frames < 2)
          num_buffered_frames++;
        else
        {
          buffer_tail = 1 - buffer_tail;
          in_pos += in_frame_time;
        }
        buffer_head = 1 - buffer_head;
      	in_frame_count++;
      }
      else if (i != Y4M_ERR_EOF)
      	  mjpeg_error_exit1("Error reading from input stream!");
      else {

          /* End of input reached, try with one frame (might do) or give up */
          if (num_buffered_frames > 1)
          {
            buffer_tail = 1 - buffer_tail;
            num_buffered_frames--;
            in_pos += in_frame_time;
          }
          else
          {
            eos = 1;
            break;
          }
      }
    }
    
    /* Produce the output field at the current position if input available */
    if (!eos)
    {
      int src_frame[2] = { -1, -1 };
      int src_field[2] = { -1, -1 };
      int src_time[2] = { 0, 0 };

      /* Choose one or two source frames/fields for this output position */
      src_frame[0] = buffer_tail;
      if (in_interlacing == Y4M_ILACE_NONE)
      {
        src_time[0] = in_pos;
        if (out_pos != src_time[0])
        {
          src_frame[1] = buffer_head;
          src_time[1] = in_pos + in_frame_time;
        }
      }
      else /* interlaced input */
      {
        if (in_pos + in_frame_time / 2 > out_pos)
        {
          src_field[0] = 0;
          src_time[0] = in_pos;
          if (out_pos != src_time[0])
          {
            src_frame[1] = buffer_tail;
            src_field[1] = 1;
            src_time[1] = in_pos + in_frame_time / 2;
          }
        }
        else
        {
          src_field[0] = 1;
          src_time[0] = in_pos + in_frame_time / 2;
          if (out_pos != src_time[0])
          {
            src_frame[1] = buffer_head;
            src_field[1] = 0;
            src_time[1] = in_pos + in_frame_time;
          }
        }
      }
      mjpeg_debug("Producing output frame %d%s",
                  out_frame_count + 1,
                  (out_interlacing == Y4M_ILACE_NONE ? ""
                   : (out_field == 0 ? ", 1st field" : ", 2nd field")));
      mjpeg_debug("  %s source is input frame %d%s",
                  (src_frame[1] == -1 ? "only" : "1st"),
                  in_frame_count - num_buffered_frames + 1,
                  (src_field[0] == -1 ? ""
                   : (src_field[0] == 0 ? ", 1st field" : ", 2nd field")));
      if (src_frame[1] != -1)
        mjpeg_debug("  2nd source is input frame %d%s",
                    in_frame_count -
                      (src_frame[1] == buffer_tail
                       ? num_buffered_frames - 1
                       : 0),
                    (src_field[1] == -1 ? ""
                     : (src_field[1] == 0 ? ", 1st field" : ", 2nd field")));

      /* Produce the frame/field */
      for (i = 0; i < num_planes; i++)
      {
        int y;
        
        for (y = (out_interlacing == Y4M_ILACE_NONE
                  || (out_interlacing == Y4M_ILACE_TOP_FIRST
                      ? out_field == 0
                      : out_field == 1) ? 0 : 1);
             y < plane_height[i];
             y += (out_interlacing == Y4M_ILACE_NONE ? 1 : 2))
        {
          uint8_t *dst_line = out_planes[i] + y * plane_width[i];
          uint8_t *src_line[2] = { NULL, NULL };

          /* Determine or produce the source line(s) */
          for (j = 0; j < (src_frame[1] == -1 ? 1 : 2); j++)
          {
            if (in_interlacing == Y4M_ILACE_NONE
                || ((in_interlacing == Y4M_ILACE_TOP_FIRST
                     ? !src_field[j]
                     : src_field[j]) ? !(y & 1) : (y & 1)))
              src_line[j] = in_planes[src_frame[j]][i] + y * plane_width[i];
            else if (y == 0)
              src_line[j] = in_planes[src_frame[j]][i] + plane_width[i];
            else if (y == plane_height[i] - 1)
              src_line[j] = in_planes[src_frame[j]][i] + (y - 1) * plane_width[i];
            else
            {
              uint8_t *l1, *l2, *sl;
              int k;
              
              /* Produce source line by averaging the surrounding lines */
              sl = src_line[j] = intermediate_data[j];
              l1 = in_planes[src_frame[j]][i] + (y - 1) * plane_width[i];
              l2 = l1 + 2 * plane_width[i];
              for (k = plane_width[i]; k != 0; k--)
                *(sl++) = (uint8_t) (((int) *(l1++) + *(l2++)) / 2);
            }
          }
          
          /* If only one source line, just copy it */
          if (src_line[1] == NULL)
            memcpy(dst_line, src_line[0], plane_width[i]);
            
          /* Otherwise calculate weighted average of the source lines */
          else
          {
            int w1 = src_time[1] - out_pos;
            int w2 = out_pos - src_time[0];
            int wsum = w1 + w2;
            
            for (j = plane_width[i]; j != 0; j--)
              *(dst_line++)
                = (w1 * *(src_line[0]++) + w2 * *(src_line[1]++)) / wsum;
          }            
	}
      }

      /* Write the current frame if ready (progressive or both fields done) */
      if (out_interlacing == Y4M_ILACE_NONE || out_field == 1)
      {
      	mjpeg_info("Writing output frame %d", ++out_frame_count);
      	if (y4m_write_frame(out_fd, out_si, &fi, out_planes) != Y4M_OK)
      	  mjpeg_error_exit1("Error writing output stream!");
      }
    
      /* Advance to the next output frame/field position */
      if (out_interlacing == Y4M_ILACE_NONE)
        out_pos += out_frame_time;
      else
      {
        out_field = 1 - out_field;
        out_pos += out_frame_time / 2;
      }
    }
  }
  if (out_field == 1)
    mjpeg_debug("Discarding unfinished output frame %d",
                out_frame_count + 1);

  /* Finalize frame info structure */
  y4m_fini_frame_info(&fi);
}


/* -----------------------------------------------------------------------
 * Common initialization and user interface code
 * ---------------------------------------------------------------------*/

//
// Parse interlacing mode keyword into interlacing mode value.
//   "p" -> progressive, Y4M_ILACE_NONE
//   "t" -> top-field first, Y4M_ILACE_TOP_FIRST
//   "b" -> bottom-field first, Y4M_ILACE_BOTTOM_FIRST
//
// Exits with an error message if the specified argument is invalid.
//
static int parse_interlacing(char *str)
{
  if (str[0] != '\0' && str[1] == '\0')
  {
    switch (str[0])
    {
      case 'p':
        return Y4M_ILACE_NONE;
      case 't':
        return Y4M_ILACE_TOP_FIRST;
      case 'b':
        return Y4M_ILACE_BOTTOM_FIRST;
    }
  }
  mjpeg_error_exit1("Valid interlacing modes are: p - progressive, t - top-field first, b - bottom-field first");
  return Y4M_UNKNOWN; /* to avoid compiler warnings */
}

// ***************************************************************************
// MAIN
// ***************************************************************************
int main (int argc, char *argv[])
{

  int verbose = mjpeg_loglev_t("error");
  int change_header_only = 0 ;
  int not_normalize = 0;
  int use_weighted_average = 0;
  int fdIn = 0 ;
  int fdOut = 1 ;
  y4m_stream_info_t in_streaminfo, out_streaminfo ;
  y4m_ratio_t frame_rate, src_frame_rate, normalized_ratio ;
  int src_interlacing = Y4M_UNKNOWN;
  int interlacing = Y4M_UNKNOWN;
  const static char *legal_flags = "r:s:i:I:cnwv:h";
  int c ;
  
  src_frame_rate.d = 0;
  frame_rate.d = 0;

  while ((c = getopt (argc, argv, legal_flags)) != -1) {
        switch (c)
        {
        /* New frame rate */
        case 'r':
          if( Y4M_OK != y4m_parse_ratio(&frame_rate, optarg) )
              mjpeg_error_exit1 ("Syntax for frame rate should be Numerator:Denominator");
          break;
        /* Assumed frame rate for source (useful when the header contains an
           invalid frame rate) */
        case 's':
          if( Y4M_OK != y4m_parse_ratio(&src_frame_rate,optarg) )
              mjpeg_error_exit1 ("Syntax for frame rate should be Numerator:Denominator");
          break ;
        /* New interlacing */
        case 'i':
          interlacing = parse_interlacing(optarg);
          break;
        /* Assumed interlacing for source */
        case 'I':
          src_interlacing = parse_interlacing(optarg);
          break;
        /* Only change header frame-rate, not the stream itself */
        case 'c':
          change_header_only = 1 ;
        case 'n':
          not_normalize = 1;
        break;
        case 'w':
          use_weighted_average = 1;
          break;
        case 'v':
          verbose = atoi (optarg);
          if (verbose < 0 || verbose > 2)
            mjpeg_error_exit1 ("Verbose level must be [0..2]");
          break;
        
        case 'h':
        case '?':
          print_usage (argv);
          return 0 ;
          break;
        }
  }
  
  /* Check that frame rate was specified */
  if (frame_rate.d == 0)
    mjpeg_error_exit1("Output frame rate must be specified");

  y4m_accept_extensions(1);

  /* mjpeg tools global initialisations */
  mjpeg_default_handler_verbosity (verbose);

  /* Initialize input streams */
  y4m_init_stream_info (&in_streaminfo);
  y4m_init_stream_info (&out_streaminfo);

  // ***************************************************************
  // Get video stream informations (size, framerate, interlacing, aspect ratio).
  // The streaminfo structure is filled in
  // ***************************************************************
  // INPUT comes from stdin, we check for a correct file header
  if (y4m_read_stream_header (fdIn, &in_streaminfo) != Y4M_OK)
    mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");

  /* Determine input/output interlacing and check for conflicts */
  if (src_interlacing == Y4M_UNKNOWN)
    src_interlacing = y4m_si_get_interlace(&in_streaminfo);
  if (interlacing == Y4M_UNKNOWN)
    interlacing = src_interlacing;
  if (interlacing != src_interlacing
      && !use_weighted_average
      && !change_header_only)
    mjpeg_error_exit1("Interlacing mode can be changed only when using weighted average resampling (-w)");
  if (src_interlacing != Y4M_ILACE_NONE
      && src_interlacing != Y4M_ILACE_TOP_FIRST
      && src_interlacing != Y4M_ILACE_BOTTOM_FIRST)
    mjpeg_error_exit1("Unsupported interlacing mode");

  /* Prepare output stream */
  if (src_frame_rate.d == 0)
     src_frame_rate = y4m_si_get_framerate( &in_streaminfo );
  y4m_copy_stream_info( &out_streaminfo, &in_streaminfo );
  
  /* Information output */
  mjpeg_info ("yuv2fps (version " YUVFPS_VERSION
              ") is a general frame resampling utility for yuv streams");
  mjpeg_info ("(C) 2002 Alfonso Garcia-Patino Barbolani <barbolani@jazzfree.com>");
  mjpeg_info ("yuvfps -h for help, or man yuvfps");

  y4m_si_set_framerate( &out_streaminfo, frame_rate );
  y4m_si_set_interlace( &out_streaminfo, interlacing );
  y4m_write_stream_header(fdOut,&out_streaminfo);

  if (not_normalize == 0)
    { /* Trying to normalize the values */
      normalized_ratio = mpeg_conform_framerate( 
                          (double)src_frame_rate.n/(double)src_frame_rate.d );
      mjpeg_warn( "Original framerate: %d:%d, Normalized framerate: %d:%d", 
 src_frame_rate.n, src_frame_rate.d, normalized_ratio.n, normalized_ratio.d );
      src_frame_rate.n = normalized_ratio.n;
      src_frame_rate.d = normalized_ratio.d;
    }

/*
 * Do this BEFORE clobbering frame_rate below!!  Otherwise the wrong
 * value is printed because the src_rate is copied over to the -r value 
*/
  mjpeg_warn( "Converting from %d:%d to %d:%d", 
               src_frame_rate.n,src_frame_rate.d,frame_rate.n,frame_rate.d  );

  if( change_header_only )
  {
    frame_rate = src_frame_rate;
    interlacing = src_interlacing;
    use_weighted_average = 0;
  }
    
  /* in that function we do all the important work */
  if (use_weighted_average)
    resample_wa( fdIn, &in_streaminfo, src_frame_rate, src_interlacing,
                 fdOut, &out_streaminfo, frame_rate, interlacing );
  else
    resample( fdIn, &in_streaminfo, src_frame_rate,
              fdOut, &out_streaminfo, frame_rate );

  y4m_fini_stream_info (&in_streaminfo);
  y4m_fini_stream_info (&out_streaminfo);

  return 0;
}
/*
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
