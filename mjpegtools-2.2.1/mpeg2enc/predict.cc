/* predict.cc, motion compensated prediction                                 */

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
#include <assert.h>
#include "mjpeg_logging.h"
#include "mpeg2encoder.hh"
#include "mpeg2syntaxcodes.h"
#include "picture.hh"
#include "macroblock.hh"
#include "imageplanes.hh"
#include "predict_ref.h"


/* predict a rectangular block (all three components)
 *
 * src:     source frame (Y,U,V)
 * sfield:  source field select (0: frame or top field, 1: bottom field)
 * dst:     destination frame (Y,U,V)
 * dfield:  destination field select (0: frame or top field, 1: bottom field)
 *
 * the following values are in luminance picture (frame or field) dimensions
 * lx:      distance of vertically adjacent pels (selects frame or field pred.)
 * w,h:     width and height of block (only 16x16 or 16x8 are used)
 * x,y:     coordinates of destination block
 * dx,dy:   half pel motion vector
 * addflag: store or add (= average) prediction
 */

void pred (	uint8_t *src[], int sfield,
			uint8_t *dst[], int dfield,
			int lx, int w, int h, int x, int y, 
			int dx, int dy, bool addflag
	)
{
	int cc;

	for (cc=0; cc<3; cc++)
	{
		if (cc==1)
		{
			/* scale for color components */
			/* vertical */
			h >>= 1; y >>= 1; dy /= 2;
			/* horizontal */
			w >>= 1; x >>= 1; dx /= 2;
			lx >>= 1;
		}
		ppred_comp(	src[cc]+(sfield?lx>>1:0),dst[cc]+(dfield?lx>>1:0),
					lx,w,h,x,y,dx,dy, (int)addflag);
	}
}



/* calculate derived motion vectors (DMV) for dual prime prediction
 * dmvector[2]: differential motion vectors (-1,0,+1)
 * mvx,mvy: motion vector (for same parity)
 *
 * DMV[2][2]: derived motion vectors (for opposite parity)
 *
 * uses global variables pict_struct and topfirst
 *
 * Notes:
 *  - all vectors are in field coordinates (even for frame pictures)
 *
 */

#ifndef DEBUG_DPME
static
#endif
void calc_DMV( const Picture &picture, /*int pict_struct,  int topfirst,*/
					  MotionVector DMV[Parity::dim],
					  MotionVector &dmvector, 
					  int mvx, int mvy
	)
{
	if (picture.pict_struct==FRAME_PICTURE)
	{
		if (picture.topfirst)
		{
			/* vector for prediction of top field from bottom field */
			DMV[0][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
			DMV[0][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] - 1;
			
			/* vector for prediction of bottom field from top field */
			DMV[1][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
			DMV[1][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] + 1;
		}
		else
		{
			/* vector for prediction of top field from bottom field */
			DMV[0][0] = ((3*mvx+(mvx>0))>>1) + dmvector[0];
			DMV[0][1] = ((3*mvy+(mvy>0))>>1) + dmvector[1] - 1;
			
			/* vector for prediction of bottom field from top field */
			DMV[1][0] = ((mvx  +(mvx>0))>>1) + dmvector[0];
			DMV[1][1] = ((mvy  +(mvy>0))>>1) + dmvector[1] + 1;
		}
	}
	else
	{
		/* vector for prediction from field of opposite 'parity' */
		DMV[0][0] = ((mvx+(mvx>0))>>1) + dmvector[0];
		DMV[0][1] = ((mvy+(mvy>0))>>1) + dmvector[1];
		
		/* correct for vertical field shift */
		if (picture.pict_struct==TOP_FIELD)
			DMV[0][1]--;
		else
			DMV[0][1]++;
	}
}


/* form prediction for one macroblock
 *
 * lx:     frame width (identical to global var `width')
 *
 * Notes:
 * - when predicting a P type picture which is the second field of
 *   a frame, the same parity reference field is in oldref, while the
 *   opposite parity reference field is assumed to be in newref!
 * - intra macroblocks are modelled to have a constant prediction of 128
 *   for all pels; this results in a DC DCT coefficient symmetric to 0
 * - vectors for field prediction in frame pictures are in half pel frame
 *   coordinates (vertical component is twice the field value and always
 *   even)
 *
 * already covers dual prime (not yet used)
 */

void MacroBlock::Predict()
{
    const Picture &picture = ParentPicture();
    const int bx = TopleftX();
    const int by = TopleftY();
    uint8_t **fwd_rec = picture.fwd_rec->Planes();   // Forward prediction
    uint8_t **bwd_rec = picture.bwd_rec->Planes();   // Backward prediction
    uint8_t **cur = picture.pred->Planes();      // Frame to predict
    int lx = picture.encparams.phy_width;
    int lx2 = picture.encparams.phy_width;
    
    bool addflag;
    int currentfield;
    uint8_t **predframe;
    MotionVector DMV[Parity::dim /*pred*/];
    
    if (best_me->mb_type&MB_INTRA)
    {
        clearblock( cur,bx,by,
                    picture.pict_struct==BOTTOM_FIELD ? lx : 0,
                    lx2);
        return;
    }
    
    addflag = false; /* first prediction is stored, second is added and averaged */
    
    if ((best_me->mb_type & MB_FORWARD) || (picture.pict_type==P_TYPE))
    {
        /* forward prediction, including zero MV in P pictures */
    
        if (picture.pict_struct==FRAME_PICTURE)
        {
            /* frame picture */
    
            if ( (best_me->motion_type==MC_FRAME)
                    || !(best_me->mb_type & MB_FORWARD))
            {
                /* frame-based prediction in frame picture */
                pred( fwd_rec,0,cur,0,
                    lx,16,16,bx,by,best_me->MV[0][0][0],best_me->MV[0][0][1],false);
            }
            else if (best_me->motion_type==MC_FIELD)
            {
                /* field-based prediction in frame picture
                *
                * note scaling of the vertical coordinates (by, MV[][0][1])
                * from frame to field!
                */
    
                /* top field prediction */
                pred(fwd_rec,best_me->field_sel[0][0],cur,0,
                    lx<<1,16,8,bx,by>>1,
                    best_me->MV[0][0][0],best_me->MV[0][0][1]>>1,false);
    
                /* bottom field prediction */
                pred(fwd_rec,best_me->field_sel[1][0],cur,1,
                    lx<<1,16,8,bx,by>>1,
                    best_me->MV[1][0][0],best_me->MV[1][0][1]>>1,false);
            }
            else if (best_me->motion_type==MC_DMV)
            {
                /* dual prime prediction calculate derived motion vectors */
                calc_DMV(picture,
                        DMV,
                        best_me->dualprimeMV,
                        best_me->MV[0][0][0],
                        best_me->MV[0][0][1]>>1);
    
    
                /* predict top field from top field */
                pred(fwd_rec,0,cur,0,
                    lx<<1,16,8,bx,by>>1,
                    best_me->MV[0][0][0],best_me->MV[0][0][1]>>1,false);
    
                /* predict bottom field from bottom field */
                pred(fwd_rec,1,cur,1,
                    lx<<1,16,8,bx,by>>1,
                    best_me->MV[0][0][0],best_me->MV[0][0][1]>>1,false);
    
                /* predict and add to top field from bottom field */
                pred(fwd_rec,1,cur,0,
                    lx<<1,16,8,bx,by>>1,
                    DMV[0][0],DMV[0][1],true);
    
    
                /* predict and add to bottom field from top field */
                pred(fwd_rec,0,cur,1,
                    lx<<1,16,8,bx,by>>1,
                    DMV[1][0],DMV[1][1],true);
            }
            else
            {
                /* invalid motion_type in frame picture */
                mjpeg_error_exit1("Internal: invalid motion_type");
            }
        }
        else /* TOP_FIELD or BOTTOM_FIELD */
        {
            /* field picture */
    
            currentfield = (picture.pict_struct==BOTTOM_FIELD);
    
            /* determine which frame to use for prediction */
            if ((picture.pict_type==P_TYPE) && picture.secondfield
                    && (currentfield!=best_me->field_sel[0][0]))
                predframe = bwd_rec; /* same frame */
            else
                predframe = fwd_rec; /* previous frame */
    
            if ( best_me->motion_type==MC_FIELD
                    || !(best_me->mb_type & MB_FORWARD))
            {
                /* field-based prediction in field picture */
                pred(predframe,best_me->field_sel[0][0],cur,currentfield,
                    lx<<1,16,16,bx,by,
                    best_me->MV[0][0][0],best_me->MV[0][0][1],false);
            }
            else if (best_me->motion_type==MC_16X8)
            {
                /* 16 x 8 motion compensation in field picture */
    
                /* upper half */
                pred(predframe,best_me->field_sel[0][0],cur,currentfield,
                    lx<<1,16,8,bx,by,
                    best_me->MV[0][0][0],best_me->MV[0][0][1],false);
    
                /* determine which frame to use for lower half prediction */
                if ((picture.pict_type==P_TYPE) && picture.secondfield
                        && (currentfield!=best_me->field_sel[1][0]))
                    predframe = bwd_rec; /* same frame */
                else
                    predframe = fwd_rec; /* previous frame */
    
                /* lower half */
                pred(predframe,best_me->field_sel[1][0],cur,currentfield,
                    lx<<1,16,8,bx,by+8,
                    best_me->MV[1][0][0],best_me->MV[1][0][1],false);
            }
            else if (best_me->motion_type==MC_DMV)
            {
                /* dual prime prediction */
    
                /* determine which frame to use for prediction */
                if (picture.secondfield)
                    predframe = bwd_rec; /* same frame */
                else
                    predframe = fwd_rec; /* previous frame */
    
                /* calculate derived motion vectors */
                calc_DMV(picture,
                        DMV,best_me->dualprimeMV,
                        best_me->MV[0][0][0],
                        best_me->MV[0][0][1]);
    
                /* predict from field of same parity */
                pred(fwd_rec,currentfield,cur,currentfield,
                    lx<<1,16,16,bx,by,
                    best_me->MV[0][0][0],best_me->MV[0][0][1],false);
    
                /* predict from field of opposite parity */
                pred(predframe,!currentfield,cur,currentfield,
                    lx<<1,16,16,bx,by,
                    DMV[0][0],DMV[0][1],true);
            }
            else
            {
                /* invalid motion_type in field picture */
                mjpeg_error_exit1("Internal: invalid motion_type");
            }
        }
        addflag = true; /* next prediction (if any) will be averaged with this one */
    }
    
    if (best_me->mb_type & MB_BACKWARD)
    {
        /* backward prediction */
    
        if (picture.pict_struct==FRAME_PICTURE)
        {
            /* frame picture */
    
            if (best_me->motion_type==MC_FRAME)
            {
                /* frame-based prediction in frame picture */
                pred(bwd_rec,0,cur,0,
                    lx,16,16,bx,by,
                    best_me->MV[0][1][0],best_me->MV[0][1][1],addflag);
            }
            else
            {
                /* field-based prediction in frame picture
                *
                * note scaling of the vertical coordinates (by, MV[][1][1])
                * from frame to field!
                */
    
                /* top field prediction */
                pred(bwd_rec,best_me->field_sel[0][1],cur,0,
                    lx<<1,16,8,bx,by>>1,
                    best_me->MV[0][1][0],best_me->MV[0][1][1]>>1,addflag);
    
                /* bottom field prediction */
                pred(bwd_rec,best_me->field_sel[1][1],cur,1,
                    lx<<1,16,8,bx,by>>1,
                    best_me->MV[1][1][0],best_me->MV[1][1][1]>>1,addflag);
            }
        }
        else /* TOP_FIELD or BOTTOM_FIELD */
        {
            /* field picture */
    
            currentfield = (picture.pict_struct==BOTTOM_FIELD);
    
            if (best_me->motion_type==MC_FIELD)
            {
                /* field-based prediction in field picture */
                pred(bwd_rec,best_me->field_sel[0][1],cur,currentfield,
                    lx<<1,16,16,bx,by,
                    best_me->MV[0][1][0],best_me->MV[0][1][1],addflag);
            }
            else if (best_me->motion_type==MC_16X8)
            {
                /* 16 x 8 motion compensation in field picture */
    
                /* upper half */
                pred(bwd_rec,best_me->field_sel[0][1],cur,currentfield,
                    lx<<1,16,8,bx,by,
                    best_me->MV[0][1][0],best_me->MV[0][1][1],addflag);
    
                /* lower half */
                pred(bwd_rec,best_me->field_sel[1][1],cur,currentfield,
                    lx<<1,16,8,bx,by+8,
                    best_me->MV[1][1][0],best_me->MV[1][1][1],addflag);
            }
            else
            {
                /* invalid motion_type in field picture */
                mjpeg_error_exit1("Internal: invalid motion_type");
            }
        }
    }
}
