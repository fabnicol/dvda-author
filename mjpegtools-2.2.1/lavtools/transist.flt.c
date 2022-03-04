/*
 *  transist - reads two frame-interlaced YUV4MPEG streams from stdin
 *             and writes out a transistion from one to the other
 *
 *  Copyright (C) 2000, pHilipp Zabel <pzabel@gmx.de>
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

#include "mjpeg_logging.h"

#include "yuv4mpeg.h"

static void usage (void) 
{
  fprintf(stderr, 
	  "usage:  transist.flt [params]\n"
	  "params: -o num   opacity of second input at the beginning [0-255]\n"
	  "        -O num   opacity of second input at the end [0-255]\n"
	  "        -d num   duration of transistion in frames (REQUIRED!)\n"
	  "        -s num   skip first num frames of transistion\n"
	  "        -n num   only process num frames of the transistion\n"
	  "        -r num   repeat each frame 'num' times. (default = 1)\n"
	  "        -v num   verbosity [0..2]\n");
}

static void blend (unsigned char *src0[3], unsigned char *src1[3],
                   unsigned int opacity1,
                   unsigned int len,
                   unsigned char *dst[3])
{
   register unsigned int i = 0;
   register unsigned int j = 0;
   register unsigned int op1 = opacity1 & 0xFF;
   register unsigned int op0 = 0x100 - op1;

   do {
      dst[2][j] = (op0 * src0[2][j] + op1 * src1[2][j]) >> 8;
      dst[1][j] = (op0 * src0[1][j] + op1 * src1[1][j]) >> 8; j++;
      dst[0][i] = (op0 * src0[0][i] + op1 * src1[0][i]) >> 8; i++;
      dst[0][i] = (op0 * src0[0][i] + op1 * src1[0][i]) >> 8; i++;
      dst[0][i] = (op0 * src0[0][i] + op1 * src1[0][i]) >> 8; i++;
      dst[0][i] = (op0 * src0[0][i] + op1 * src1[0][i]) >> 8; i++;
   } while (i < len);
}

int main (int argc, char *argv[])
{
   int verbose = 1;
   int in_fd  = 0;         /* stdin */
   int out_fd = 1;         /* stdout */
   unsigned char *yuv0[3]; /* input 0 */
   unsigned char *yuv1[3]; /* input 1 */
   unsigned char *yuv[3];  /* output */
   int w, h, len, lensr2;
   int i, j, opacity, opacity_range, frame, numframes, r = 0;
   unsigned int param_opacity0   = 0;     /* opacity of input1 at the beginning */
   unsigned int param_opacity1   = 255;   /* opacity of input1 at the end */
   unsigned int param_duration   = 0;     /* duration of transistion effect */
   unsigned int param_skipframes = 0;     /* # of frames to skip */
   unsigned int param_numframes  = 0;     /* # of frames to (process - skip+num) * framerepeat <= duration */
   unsigned int param_framerep   = 1;    /* # of repititions per frame */
   y4m_stream_info_t streaminfo;
   y4m_frame_info_t frameinfo;

   y4m_init_stream_info (&streaminfo);
   y4m_init_frame_info (&frameinfo);

   while ((i = getopt(argc, argv, "v:o:O:d:s:n:r:")) != -1) {
      switch (i) {
      case 'v':
         verbose = atoi (optarg);
		 if( verbose < 0 || verbose >2 )
		 {
			 usage ();
			 exit (1);
		 }
         break;		  
      case 'o':
         param_opacity0 = atoi (optarg);
         if (param_opacity0 > 255) {
            mjpeg_warn( "start opacity > 255");
            param_opacity0 = 255;
         }
         break;
      case 'O':
         param_opacity1 = atoi (optarg);
         if (param_opacity1 > 255) {
            mjpeg_warn( "end opacity > 255");
            param_opacity1 = 255;
         }
         break;
      case 'd':
         param_duration = atoi (optarg);
         if (param_duration == 0) {
            mjpeg_error_exit1( "error: duration = 0 frames");
         }
         break;
      case 's':
         param_skipframes = atoi (optarg);
         break;
      case 'n':
         param_numframes = atoi (optarg);
         break;
      case 'r':
         param_framerep = atoi (optarg);
         break;
      }
   }
   if (param_numframes == 0)
      param_numframes = (param_duration - param_skipframes) / param_framerep;
   if (param_duration == 0) {
      usage ();
      exit (1);
   }
   numframes = (param_skipframes + param_numframes) * param_framerep;
   if (numframes > param_duration) {
      mjpeg_error_exit1( "skip + num > duration");
   }

   (void)mjpeg_default_handler_verbosity(verbose);


   i = y4m_read_stream_header (in_fd, &streaminfo);
   if (i != Y4M_OK) {
      fprintf (stderr, "%s: input stream error - %s\n", 
	       argv[0], y4m_strerr(i));
      exit (1);
   }
   w = y4m_si_get_width(&streaminfo);
   h = y4m_si_get_height(&streaminfo);
   
   len = w*h;
   lensr2 = len >> 2;
   yuv[0] = malloc (len);
   yuv0[0] = malloc (len);
   yuv1[0] = malloc (len);
   yuv[1] = malloc (lensr2);
   yuv0[1] = malloc (lensr2);
   yuv1[1] = malloc (lensr2);
   yuv[2] = malloc (lensr2); 
   yuv0[2] = malloc (lensr2); 
   yuv1[2] = malloc (lensr2);

   y4m_write_stream_header (out_fd, &streaminfo);

   frame = param_skipframes;
   param_duration--;
   opacity_range = param_opacity1 - param_opacity0;
   while (1) {

      if (!r) {
        r = param_framerep;

      i = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv0);
      if (i != Y4M_OK)
          exit (frame < numframes);

      j = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv1);
      if (j != Y4M_OK)
          exit (frame < numframes);
      }
      r--;

      opacity = param_opacity0 + ((frame * opacity_range) / param_duration);

      blend (yuv0, yuv1, opacity, len, yuv);
      y4m_write_frame (out_fd, &streaminfo, &frameinfo, yuv);
      if (++frame == numframes)
         exit (0);
   }

}

