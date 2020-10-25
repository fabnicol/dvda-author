/*
 *  multiblend - reads two frame-interlaced YUV4MPEG streams from stdin
 *               and blends the second over the first.
 *
 *  derived from mattblend.flt.c. Added more blending stuff.
 *  Copyright (C) 2001, pHilipp Zabel <pzabel@gmx.de>
 *  Copyright (C) 2002 , Niels Elburg elburg@hio.hen.nl
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

/*
 * Note: for all _yuvdata methods, 
 *       I want to blend on Luminance only,  
 *       and keep the Cb and Cr channels of the original frame
 *       If you want to blend over Cb and Cr too, uncomment the lines of code.
 *
 */

static void usage (void) {

   fprintf (stderr, "usage:  multiblend.flt\n"
                    "params: -t num     type of overlay [1-10]\n"
                    "         1 = additive\n"
                    "         2 = substractive\n"
                    "         3 = multiply\n"
                    "         4 = divide\n"
                    "         5 = difference\n"
                    "         6 = difference negate\n"
                    "         7 = freeze\n"
                    "         8 = unfreeze\n"
                    "         9 = hardlight\n"
                    "        10 = lighten only\n"
                    "\n");

}

static void overlay_lighten_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a,b,c;
        
        for(i=0; i < len; i++) {
                a =  src1[0][i];
                b =  src2[0][i];
                if ( a > b ) c = a;
                else c = b;
                
                src1[0][i]=c;
        }
}

static void overlay_hardlight_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a,b,c;
        
        for(i=0; i < len; i++) {
                a = src1[0][i];
                b = src2[0][i]; 
                
                if ( b < 128 ) c = (a*b)>>7;
                else c = 235 - ( (235-b)*(235-a)>>7);
                if (c <16) c = 16;
                
                src1[0][i] = c;
        }
}

static void overlay_unfreeze_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a,b,c;
        
        for(i=0; i < len; i++) {
                a = src1[0][i];
                b = src2[0][i]; 
                
                if ( a < 16 ) c = 16;
                else c = 235 - ( (235-b)*(235-b))/a;
                if (c <16) c = 16;
                
                src1[0][i] = c;
        }
}

static void overlay_freeze_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a,b,c;
        
        for(i=0; i < len; i++) {
                a = src1[0][i];
                b = src2[0][i]; 
                
                if ( b < 16 ) c = 16;
                else c = 235 - ( (235-a)*(235-a))/b;
                if (c <16) c = 16;
                
                src1[0][i] = c;
        }
}

static void overlay_diffnegate_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        
        for(i=0; i < len; i++) {
                src1[0][i] = 235 - abs( 235 - src1[0][i] - src2[0][i] );
        }

}
static void overlay_difference_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        
        for(i=0; i < len; i++) {
                src1[0][i] = abs( src1[0][i] - src2[0][i] );
        }
        
}

static void overlay_divide_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a,b,c;
        
        for(i=0; i < len; i++) {
                b = src1[0][i]*src2[0][i];
                c = 255 - src2[0][i];
                if(c==0)c=16;
                a = b/c;
                if ( a > 235 ) a = 235;
                if ( a < 16 ) a = 16;
                src1[0][i] = a;
        }
        /* for Cb and Cr 
        len >>=2;

        for(i=0; i < len; i++) {
                b = src1[1][i]*src2[1][i];
                c = 255 - src2[1][i];
                if(c==0)c=16;
                a = b/c;
                if ( a > 235 ) a = 235;
                if ( a < 16 ) a = 16;
                src1[1][i] = a;
                b = src1[2][i]*src2[2][i];
                c = 255 - src2[2][i];
                if(c==0)c=16;
                a = b/c;
                if ( a > 235 ) a = 235;
                if ( a < 16 ) a = 16;
                src1[2][i] = a;
        }
        */
}
static void overlay_add_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a;
        
        for(i=0; i < len; i++) {
                a = src1[0][i] + (2*src2[0][i]) - 235;
                if (a < 16) a = 16;
                if (a > 235) a = 235;
                src1[0][i]=a;
        }
        /* for Cb and Cr 
        len>>=2;
        for(i=0; i < len; i++) {
                a = src1[1][i] + (2*src2[1][i]) - 235;
                if (a < 16) a = 16;
                if (a > 235) a = 235;
                src1[1][i]=a;
                a = src1[2][i] + (2*src2[2][i]) - 235;
                if (a < 16) a = 16;
                if (a > 235) a = 235;
                src1[2][i]=a;
        }
        */
}
static void overlay_sub_yuvdata(uint8_t *src1[3], uint8_t *src2[3], int width, int height) {
        register unsigned int i;        
        register unsigned int len = width * height;
        register int a;

        for(i=0; i < len; i++) {
                a = src1[0][i]+src2[0][i] - 235;
                if (a < 16) a = 16;
                src1[0][i]=a;
        }
}
static void overlay_multiply_yuvdata(uint8_t *src1[3], 
                              uint8_t *src2[3], int width, int height) {

        register unsigned int i;        
        register unsigned int len = width * height;
        for(i=0; i < len; i++) {
                src1[0][i]=(src1[0][i]*src2[0][i])>>8;
        }
}

static void multiblend(uint8_t *src1[3], uint8_t *src2[3], int width, int height, int type) {
        switch(type) {
                case 1:
                        overlay_add_yuvdata(src1,src2,width,height);
                        break;
                case 2:
                        overlay_sub_yuvdata(src1,src2,width,height);
                        break;
                case 3:
                        overlay_multiply_yuvdata(src1,src2,width,height);
                        break;
                case 4:
                        overlay_divide_yuvdata(src1,src2,width,height);
                        break;
                case 5:
			overlay_difference_yuvdata(src1,src2,width,height);
                        break;
                case 6:
			overlay_diffnegate_yuvdata(src1,src2,width,height);
                        break;
                case 7:
                        overlay_freeze_yuvdata(src1,src2,width,height);
                        break;
                case 8:
                        overlay_unfreeze_yuvdata(src1,src2,width,height);
                        break;
                case 9:
                        overlay_hardlight_yuvdata(src1,src2,width,height);
                        break;          
                case 10:
                        overlay_lighten_yuvdata(src1,src2,width,height);
                        break;
        }
}

int main (int argc, char *argv[])
{
   int in_fd  = 0;         /* stdin */
   int out_fd = 1;         /* stdout */
   unsigned char *yuv0[3]; /* input 0 */
   unsigned char *yuv1[3]; /* input 1 */
   
   y4m_stream_info_t streaminfo;
   y4m_frame_info_t frameinfo;
   int i;
   int w, h;
   int which_overlay=0;

   if (argc < 1) {
      usage ();
      exit (1);
   }

   y4m_init_stream_info (&streaminfo);
   y4m_init_frame_info (&frameinfo);

   while( ( i = getopt(argc,argv, "t:"))!=-1) {
        switch(i) {
                case 't':
                        which_overlay = atoi(optarg);
                        if (which_overlay < 1 || which_overlay > 10) {
                                usage();
                                exit(1);
                        }
                break;
        }
   }

   i = y4m_read_stream_header (in_fd, &streaminfo);
   if (i != Y4M_OK) {
      fprintf (stderr, "%s: input stream error - %s\n", 
               argv[0], y4m_strerr(i));
      exit (1);
   }
   w = y4m_si_get_width(&streaminfo);
   h = y4m_si_get_height(&streaminfo);

   yuv0[0] = malloc (w * h);
   yuv1[0] = malloc (w * h);
   yuv0[1] = malloc (w * h / 4);
   yuv1[1] = malloc (w * h / 4);
   yuv0[2] = malloc (w * h / 4);
   yuv1[2] = malloc (w * h / 4);
 
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
   
      multiblend (yuv0, yuv1,w, h, which_overlay);

      y4m_write_frame (out_fd, &streaminfo, &frameinfo, yuv0);

   }

}
