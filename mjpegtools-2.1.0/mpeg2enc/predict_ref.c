/* predict_ref.c, Reference implementations of motion compensated
 * prediction routines */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */
/* Modifications and enhancements (C) 2000/2001 Andrew Stevens */

/* These modifications are free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */


#include "config.h"
#include "mjpeg_types.h"
#include "mjpeg_logging.h"
#include "mpeg2syntaxcodes.h"
#include "predict_ref.h"
#include "cpu_accel.h"
#include "simd.h"


#if defined(HAVE_ASM_MMX)
extern void init_x86_predict(uint32_t cpucap);
#endif


#ifdef HAVE_ALTIVEC
#include "../utils/altivec/altivec_predict.h"
#endif

void (*ppred_comp)( uint8_t *src, uint8_t *dst,
					int lx, int w, int h, int x, int y, int dx, int dy,
					int addflag);



/* low level prediction routine (Reference implementation)
 *
 * src:     prediction source
 * dst:     prediction destination
 * lx:      line width (for both src and dst)
 * x,y:     destination coordinates
 * dx,dy:   half pel motion vector
 * w,h:     size of prediction block
 * addflag: store or add prediction
 *
 * There are also SIMD versions of this routine...
 */

void pred_comp(
	uint8_t *src,
	uint8_t *dst,
	int lx,
	int w, int h,
	int x, int y,
	int dx, int dy,
	int addflag)
{
	int xint, xh, yint, yh;
	int i, j;
	uint8_t *s, *d;

	/* half pel scaling */
	xint = dx>>1; /* integer part */
	xh = dx & 1;  /* half pel flag */
	yint = dy>>1;
	yh = dy & 1;

	/* origins */
	s = src + lx*(y+yint) + (x+xint); /* motion vector */
	d = dst + lx*y + x;

	if (!xh && !yh)
		if (addflag)
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (unsigned int)(d[i]+s[i]+1)>>1;
				s+= lx;
				d+= lx;
			}
		else
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = s[i];
				s+= lx;
				d+= lx;
			}
	else if (!xh && yh)
		if (addflag)
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (d[i] + ((unsigned int)(s[i]+s[i+lx]+1)>>1)+1)>>1;
				s+= lx;
				d+= lx;
			}
		else
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (unsigned int)(s[i]+s[i+lx]+1)>>1;
				s+= lx;
				d+= lx;
			}
	else if (xh && !yh)
		if (addflag)
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (d[i] + ((unsigned int)(s[i]+s[i+1]+1)>>1)+1)>>1;
				s+= lx;
				d+= lx;
			}
		else
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (unsigned int)(s[i]+s[i+1]+1)>>1;
				s+= lx;
				d+= lx;
			}
	else /* if (xh && yh) */
		if (addflag)
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (d[i] + ((unsigned int)(s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2)+1)>>1;
				s+= lx;
				d+= lx;
			}
		else
			for (j=0; j<h; j++)
			{
				for (i=0; i<w; i++)
					d[i] = (unsigned int)(s[i]+s[i+1]+s[i+lx]+s[i+lx+1]+2)>>2;
				s+= lx;
				d+= lx;
			}
}



void clearblock( uint8_t *cur[], int i0, int j0,
                 int field_off,
                 int stride
	           )
{
    int i, j;
    uint8_t *p;
    
    p = cur[0] + field_off + i0 + stride*j0;
    
    for (j=0; j<16; j++)
    {
        for (i=0; i<16; i++)
            p[i] = 128;
        p+= stride;
    }

    // 422 Video
    i0 >>= 1; j0 >>= 1; stride >>= 1; field_off >>= 1;


    p = cur[1] + field_off + i0 + stride*j0;
    
    for (j=0; j<8; j++)
    {
        for (i=0; i<8; i++)
            p[i] = 128;
        p+= stride;
    }
    
    p = cur[2] + field_off + i0 + stride*j0;
    
    for (j=0; j<8; j++)
    {
        for (i=0; i<8; i++)
            p[i] = 128;
        p+= stride;
    }
}


/*
  Initialise prediction - currently purely selection of which
  versions of the various low level computation routines to use
  
  */

void init_predict(void)
{
	int cpucap = cpu_accel();

    /* Default to reference implementation ... */
    ppred_comp = pred_comp;

    if ( cpucap != 0 )
    {
#if defined(HAVE_ASM_MMX) 
        init_x86_predict(cpucap);
#endif
#ifdef HAVE_ALTIVEC
#  if ALTIVEC_TEST_PREDICT
#    if defined(ALTIVEC_BENCHMARK)
	    mjpeg_info("SETTING AltiVec BENCHMARK for PREDICTION!");
#    elif defined(ALTIVEC_VERIFY)
	    mjpeg_info("SETTING AltiVec VERIFY for PREDICTION!");
#    endif
#  else
	    mjpeg_info("SETTING AltiVec for PREDICTION!");
#  endif

#  if ALTIVEC_TEST_FUNCTION(pred_comp)
	    ppred_comp = ALTIVEC_TEST_SUFFIX(pred_comp);
#  else
	    ppred_comp = ALTIVEC_SUFFIX(pred_comp);
#  endif
#endif /* HAVE_ALTIVEC */
    }

}



/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
