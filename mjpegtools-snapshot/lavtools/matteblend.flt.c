/*
 *  matteblend - reads three frame-interlaced YUV4MPEG streams from stdin
 *               and blends the second over the first using the third's
 *               luminance channel as a matte
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

#include "yuv4mpeg.h"

static void usage (void) {

   fprintf (stderr, "usage:  matteblend.flt\n"
                    "no params at the moment - color saturation falloff or such has to be implemented\n");

}

static void blend (unsigned char *src0[3], unsigned char *src1[3], unsigned char *matte[3],
                   unsigned int width,     unsigned int height,
                   unsigned char *dst[3])
{
   register unsigned int i,j;
   register unsigned int len = width * height;

   for (i=0; i<len; i+=4) {
      dst[0][i]   = ((235 - matte[0][i]   )   * src0[0][i]   + (matte[0][i] - 16 )  * src1[0][i])   / 219;
      dst[0][i+1] = ((235 - matte[0][i+1] ) * src0[0][i+1] + (matte[0][i+1] - 16 )  * src1[0][i+1]) / 219;
      dst[0][i+2] = ((235 - matte[0][i+2] ) * src0[0][i+2] + (matte[0][i+2] - 16 )  * src1[0][i+2]) / 219;
      dst[0][i+3] = ((235 - matte[0][i+3] ) * src0[0][i+3] + (matte[0][i+3] - 16 )  * src1[0][i+3]) / 219;            
   }

   len>>=2; /* len = len / 4 */
   /* do we really have to "downscale" matte here? */
   for (i=0,j=0; i<len; i++, j+=2) {
      int m = (matte[0][j] + matte[0][j+1] + matte[0][j+width] + matte[0][j+width+1]) >> 2;
      if ((j % width) == (width - 2)) j += width;
      dst[1][i] = ((235-m) * src0[1][i] + (m-16) * src1[1][i]) / 219;
      dst[2][i] = ((235-m) * src0[2][i] + (m-16) * src1[2][i]) / 219;
   }
}

int main (int argc, char *argv[])
{
   int in_fd  = 0;         /* stdin */
   int out_fd = 1;         /* stdout */
   unsigned char *yuv0[3]; /* input 0 */
   unsigned char *yuv1[3]; /* input 1 */
   unsigned char *yuv2[3]; /* input 2 */
   unsigned char *yuv[3];  /* output */
   y4m_stream_info_t streaminfo;
   y4m_frame_info_t frameinfo;
   int i;
   int w, h;

   if (argc > 1) {
      usage ();
      exit (1);
   }

   y4m_init_stream_info (&streaminfo);
   y4m_init_frame_info (&frameinfo);

   i = y4m_read_stream_header (in_fd, &streaminfo);
   if (i != Y4M_OK) {
      fprintf (stderr, "%s: input stream error - %s\n", 
	       argv[0], y4m_strerr(i));
      exit (1);
   }
   w = y4m_si_get_width(&streaminfo);
   h = y4m_si_get_height(&streaminfo);

   yuv[0] =  malloc (w * h);
   yuv0[0] = malloc (w * h);
   yuv1[0] = malloc (w * h);
   yuv2[0] = malloc (w * h);
   yuv[1] =  malloc (w * h / 4);
   yuv0[1] = malloc (w * h / 4);
   yuv1[1] = malloc (w * h / 4);
   yuv2[1] = malloc (w * h / 4);
   yuv[2] =  malloc (w * h / 4);
   yuv0[2] = malloc (w * h / 4);
   yuv1[2] = malloc (w * h / 4);
   yuv2[2] = malloc (w * h / 4);

   y4m_write_stream_header (out_fd, &streaminfo);

   while (1) {
      i = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv0);
      if (i == Y4M_ERR_EOF)
	exit (0);
      else if (i != Y4M_OK)
         exit (1);
      i = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv1);
      if (i != Y4M_OK)
         exit (1);
      i = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv2);
      if (i != Y4M_OK)
         exit (1);
      /* constrain matte luma */
      for (i = 0; i < w*h; i++) {
	  if (yuv2[0][i] < 16) yuv2[0][i] = 16;
	  else
	      if (yuv2[0][i] > 235) yuv2[0][i] = 235;
      }

      blend (yuv0, yuv1, yuv2, w, h, yuv);

      y4m_write_frame (out_fd, &streaminfo, &frameinfo, yuv);

   }

}

