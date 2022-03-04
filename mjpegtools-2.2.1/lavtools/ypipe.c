/*
 *  ypipe - simple test program that interlaces the output of two
 *          YUV4MPEG-emitting programs together into one output stream
 *          frame-wise - better description urgently needed :)
 *
 *  Copyright (C) 2001, pHilipp Zabel <pzabel@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include "mjpeg_logging.h"

#include "yuv4mpeg.h"

static void usage( char *name )
{
	fprintf( stderr, "Usage: %s [-v num] <input1> <input2>\n", name);
	fprintf( stderr, "-v - verbosity  num in [0..2]\n");
	fprintf( stderr, "example: ypipe \"lav2yuv test1.el\" \"lav2yuv -n1 test2.avi\"\n");
	exit (1);
}


int main (int argc, char *argv[])
{
  FILE *sfp0, *sfp1;
  int stream0;   /* read from input 1 */
  int stream1;   /* read from input 2 */
  int outstream = 1;  /* output to stdout */
  int fstarg;
  unsigned char *yuv0[3];
  unsigned char *yuv1[3];
  y4m_stream_info_t sinfo0;
  y4m_stream_info_t sinfo1;
  y4m_stream_info_t osinfo;
  y4m_frame_info_t finfo0;
  y4m_frame_info_t finfo1;
  int err;
  char *command0, *command1;
  
  y4m_init_stream_info(&sinfo0);
  y4m_init_stream_info(&sinfo1);
  y4m_init_stream_info(&osinfo);
  y4m_init_frame_info(&finfo0);
  y4m_init_frame_info(&finfo1);
  
  y4m_accept_extensions(1);

  if ((argc != 3) && (argc != 5)) 
    usage(argv[0]);
  
  if (argc == 5) {
     int verbose;
     if (strcmp(argv[1],"-v") != 0) 
       usage(argv[0]);
     verbose = atoi(argv[2]);
     if (verbose < 0 || verbose > 2)
       usage(argv[0]);
     (void)mjpeg_default_handler_verbosity(verbose);
     fstarg = 3;
   } else
     fstarg = 1;

  command0 = strdup(argv[fstarg]);
  command1 = strdup(argv[fstarg + 1]);
   
  sfp0 = popen(command0, "r");
  if (!sfp0)
     mjpeg_error_exit1("popen(%s) failed", command0);
  sfp1 = popen(command1, "r");
  if (!sfp1)
     mjpeg_error_exit1("popen(%s) failed", command1);
  stream0 = fileno(sfp0);
  stream1 = fileno(sfp1);

  if ((err = y4m_read_stream_header(stream0, &sinfo0)) != Y4M_OK)
    mjpeg_error_exit1("Failed to read first stream header:  %s errno=%d",
		      y4m_strerr(err), errno);
  if ((err = y4m_read_stream_header(stream1, &sinfo1)) != Y4M_OK)
    mjpeg_error_exit1("Failed to read second stream header:  %s",
		      y4m_strerr(err));
  
  mjpeg_info("First stream parameters:");
  y4m_log_stream_info(mjpeg_loglev_t("info"), "1> ", &sinfo0);
  mjpeg_info("Second stream parameters:");
  y4m_log_stream_info(mjpeg_loglev_t("info"), "2> ", &sinfo1);
  
  if (y4m_si_get_width(&sinfo0) != y4m_si_get_width(&sinfo1))
    mjpeg_error_exit1("Width mismatch");
  if (y4m_si_get_height(&sinfo0) != y4m_si_get_height(&sinfo1))
    mjpeg_error_exit1("Height mismatch");
  if (!(Y4M_RATIO_EQL(y4m_si_get_framerate(&sinfo0),
		      y4m_si_get_framerate(&sinfo1))))
    mjpeg_error_exit1("Framerate mismatch");
  if (y4m_si_get_interlace(&sinfo0) != y4m_si_get_interlace(&sinfo1))
    mjpeg_error_exit1("Interlace mismatch");
  
  y4m_copy_stream_info(&osinfo, &sinfo0);
  
  yuv0[0] = malloc(y4m_si_get_plane_length(&sinfo0, 0));
  yuv1[0] = malloc(y4m_si_get_plane_length(&sinfo0, 0));
  yuv0[1] = malloc(y4m_si_get_plane_length(&sinfo0, 1));
  yuv1[1] = malloc(y4m_si_get_plane_length(&sinfo0, 1));
  yuv0[2] = malloc(y4m_si_get_plane_length(&sinfo0, 2));
  yuv1[2] = malloc(y4m_si_get_plane_length(&sinfo0, 2));
  
  if ((err = y4m_write_stream_header(outstream, &sinfo0)) != Y4M_OK)
    mjpeg_error_exit1("Failed to write output header:  %s",
		      y4m_strerr(err));
  
  while ( ((err = y4m_read_frame(stream0, &sinfo0, &finfo0, yuv0)) 
	   == Y4M_OK) &&
	  ((err = y4m_write_frame(outstream, &osinfo, &finfo0, yuv0))
	   == Y4M_OK) &&
	  ((err = y4m_read_frame(stream1, &sinfo1, &finfo1, yuv1)) 
	   == Y4M_OK) &&
	  ((err = y4m_write_frame(outstream, &osinfo, &finfo1, yuv1))
	   == Y4M_OK) ) {}
  
  if (err != Y4M_ERR_EOF)
    mjpeg_error_exit1("Some error somewhere:  %s", y4m_strerr(err));
  
  y4m_fini_stream_info(&sinfo0);
  y4m_fini_stream_info(&sinfo1);
  y4m_fini_stream_info(&osinfo);
  y4m_fini_frame_info(&finfo0);
  y4m_fini_frame_info(&finfo1);
  return 0;
}
