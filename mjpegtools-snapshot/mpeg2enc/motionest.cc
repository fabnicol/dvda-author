/* motionest.cc, motion estimation  */

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
 * */

/* Modifications and enhancements (C) 2000-2004 Andrew Stevens */

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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <cassert>
#include <math.h>
#include <mpeg2syntaxcodes.h>
#include "cpu_accel.h"
#include "simd.h"
#include "fastintfns.h"
#include "motionsearch.h"
#include "mjpeg_logging.h"
#include "encoderparams.hh"
#include "picture.hh"
#include "imageplanes.hh"


/* See ISO13818-2 7.6.3.5 */
int 
dualprime_m[FieldOrder::dim][Parity::dim /*ref*/][Parity::dim /*pred*/] =
{
    { { 2, 1 }, { 3, 2 } }, // Botfield first
    { { 2, 3 }, { 1, 2 } }  // Topfield first
};

int dualprime_e[Parity::dim /*ref*/ ][Parity::dim /*pred*/ ] =
{ 
    { 0, +1 }, 
    { -1, 0 }
};
        

/* Macro-block Motion estimation results record */


struct MotionCand
{
	Coord pos;        // Half-pel co-ormv[1]dinates of source block
	int sad;			// Sum of absolute difference
	int var;
	uint8_t *blk ;		// Source block data (in luminace data array)
	int hx, hy;			// Half-pel offsets
	int fieldsel;		// 0 = top 1 = bottom
	int fieldoff;       // Offset from start of frame data to first line
						// of field.	(top = 0, bottom = width );
};


struct SubSampledImg
{
	uint8_t *mb;		// One pel
	uint8_t *fmb;		// Two-pel subsampled
	uint8_t *qmb;		// Four-pel subsampled
	uint8_t *umb; 		// U compoenent one-pel
	uint8_t *vmb;       // V component  one-pel
};



/*
  Main field and frame based motion estimation entry points.
*/



static void field_estimate (const Picture &picture,
							uint8_t *toporg,
							uint8_t *topref, 
							uint8_t *botorg, 
							uint8_t *botref,
							SubSampledImg *ssmb,
							int i, int j, int sx, int sy,
							MotionCand *bestfr,
							MotionCand *best8u,
							MotionCand *best8l,
							MotionCand *bestsp);

static void mb_me_search (
    const EncoderParams &eparams,
	uint8_t *org, uint8_t *ref,
    int fieldoff,
	SubSampledImg *ssblk,
	int lx, int i0, int j0, 
	int sx, int sy, int h, 
	int xmax, int ymax,
	MotionCand *motion );


inline int mv_coding_penalty( int mv_x, int mv_y )
{
    return (abs(mv_x) + abs(mv_y))<<3;
}


/* 
 *  Compute subsampled images for fast motion compensation search
 *  N.b. Sub-sampling works correctly if we treat interlaced images
 *  as two half-height images side-by-side.
 */



/*
 * Compute the variance of the residual of uni-directionally motion
 * compensated block.
 *
 */

static inline int unidir_pred_var( const MotionCand *motion,
							uint8_t *mb,  
							int lx, 
							int h)
{
	return psumsq(motion->blk, mb, lx, motion->hx, motion->hy, h);
}


/*
 * Compute the variance of the residual of bi-directionally motion
 * compensated block.
 */

static inline int bidir_pred_var( const MotionCand *motion_f, 
									  const MotionCand *motion_b,
									  uint8_t *mb,  
									  int lx, int h)
{
	return pbsumsq( motion_f->blk, motion_b->blk,
					   mb, lx, 
					   motion_f->hx, motion_f->hy,
					   motion_b->hx, motion_b->hy,
					   h);
}


/* unidir_var_sum
 *
 * Compute the combined variance of luminance and
 * chrominance information for a particular non-intra macro block
 * after unidirectional motion estimation...
 *
 *  Note: results are scaled to give chrominance equal weight to
 *  chrominance.  The variance of the luminance portion is computed
 *  at the time the motion estimation is computed.
 *
 *  TODO: Perhaps we should compute the whole thing in mb_mc_search
 *  not seperate it.  However, that would involve a lot of fiddling
 *  with field_* and until its thoroughly debugged and tested I think
 *  I'll leave that alone. Furthermore, it is unclear if its really
 *  worth * doing these computations for B *and* P frames.
 *
 *  TODO: BUG: ONLY works for 420 video...
 *  
 */

static int unidir_var_sum( MotionCand *lum_mc, 
						  ImagePlanes &ref, 
						  SubSampledImg *ssblk,
						  int lx, int h )
{
	int uvlx = (lx>>1);
	int uvh = (h>>1);
	/* N.b. MC co-ordinates are computed in half-pel units! */
	int cblkoffset = (lum_mc->fieldoff>>1) +
		(lum_mc->pos.x>>2) + (lum_mc->pos.y>>2)*uvlx;
	
	return 	lum_mc->var +
         (psumsq_sub22( ref.Plane(1) + cblkoffset, ssblk->umb, uvlx, uvh) +
         psumsq_sub22( ref.Plane(2) + cblkoffset, ssblk->vmb, uvlx, uvh));
}

/*
 *  bidir_var_sum
 *  Compute the combined variance of luminance and chrominance information
 *  for a particular non-intra macro block after bidirectional
 *  motion estimation...  
 *
 *  Note: results are scaled to give chrominance equal weight to
 *  chrominance.  The variance of the luminance portion is computed
 *  at the time the motion estimation is computed.
 *
 *  Note: results scaled to give chrominance equal weight to chrominance.
 * 
 *  TODO: BUG: ONLY works for 420 video...
 *
 *  NOTE: Currently unused but may be required if it turns out that taking
 *  chrominance into account in B frames is needed.
 *
 */

static int bidir_var_sum( MotionCand *lum_mc_f, 
						  MotionCand *lum_mc_b, 
						  ImagePlanes &ref_f, 
						  ImagePlanes &ref_b,
						  SubSampledImg *ssblk,
						  int lx, int h )
{
	int uvlx = (lx>>1);
	int uvh = (h>>1);
	/* N.b. MC co-ordinates are computed in half-pel units! */
	int cblkoffset_f = (lum_mc_f->fieldoff>>1) + 
		(lum_mc_f->pos.x>>2) + (lum_mc_f->pos.y>>2)*uvlx;
	int cblkoffset_b = (lum_mc_b->fieldoff>>1) + 
		(lum_mc_b->pos.x>>2) + (lum_mc_b->pos.y>>2)*uvlx;
	
	return 	(
		pbsumsq( lum_mc_f->blk, lum_mc_b->blk,
					ssblk->mb, lx, 
					lum_mc_f->hx, lum_mc_f->hy,
					lum_mc_b->hx, lum_mc_b->hy,
					h) +
         pbsumsq_sub22( ref_f.Plane(1) + cblkoffset_f, ref_b.Plane(1) + cblkoffset_b,
					   ssblk->umb, uvlx, uvh ) +
         pbsumsq_sub22( ref_f.Plane(2) + cblkoffset_f, ref_b.Plane(2) + cblkoffset_b,
					   ssblk->vmb, uvlx, uvh ));

}

/*
 * Sum of chrominance variance of a block.
 */

static inline int chrom_var_sum( SubSampledImg *ssblk, int h, int lx )
{
    uint32_t var1, var2, dummy_mean;
    assert( (h>>1) == 8 || (h>>1) == 16 );
    pvariance(ssblk->umb,(h>>1),(lx>>1), &var1, &dummy_mean);
    pvariance(ssblk->vmb,(h>>1),(lx>>1), &var2, &dummy_mean);
    
	return (var1+var2)*2;
}



/*
 * Compute SAD for bi-directionally motion compensated blocks...
 */

static inline int bidir_pred_sad( const MotionCand *motion_f, 
									  const MotionCand *motion_b,
									  uint8_t *mb,  
									  int lx, int h)
{
	return pbsad(motion_f->blk, motion_b->blk, 
					 mb, lx, 
					 motion_f->hx, motion_f->hy,
					 motion_b->hx, motion_b->hy,
					 h);
}

#ifdef DEBUG_MC
static void display_mb(MotionCand *lum_mc, 
				  uint8_t **ref, 
				  SubSampledImg *ssblk,
				  int lx, int h )

{
	int x,y;
	int sum = 0;
	int uvlx = (lx>>1);
	int uvh = (h>>1);
	/* N.b. MC co-ordinates are computed in half-pel units! */
	int lblkoffset = lum_mc->fieldoff +
		(lum_mc->pos.x>>1) + (lum_mc->pos.y>>1)*lx;
	/*int cblkoffset = (lum_mc->fieldoff>>1) +
	  (lum_mc->pos.x>>2) + (lum_mc->pos.y>>2)*uvlx;*/
	fprintf(stderr, "ref[0] = %08x (%d,%d) width=%d reconbase = %08x blk =%08x\n", 
			(int)ref[0], (lum_mc->pos.x>>1), (lum_mc->pos.y>>1),
			lx,
			(int)(ref[0]+lblkoffset), (int)(lum_mc->blk) );
		for( y = 0; y < 16; ++y )
		{
			for( x = 0; x< 16; ++x )
			{
				int diff = *(ref[0]+lblkoffset+x+y*lx)-*(ssblk->mb+x+y*lx);
				sum += diff*diff;
				fprintf( stderr,"%04d ", diff );
			}
			fprintf(stderr,"\n");
		}
		fprintf(stderr, "sumsq = %d [%d] (%d)\n", 
				sum, 
				lum_mc->var,
				unidir_pred_var(lum_mc, ssblk->mb, lx,  h)
			);
		
}
#endif

void FieldMotionCands(
    const EncoderParams &eparams,
	uint8_t *org,
	uint8_t *ref,
	SubSampledImg *topssmb,
	SubSampledImg *botssmb,
	int i, int j, int sx, int sy,
	MotionCand *besttop,
	MotionCand *bestbot,
    MotionCand (&fieldmcs)[2][2]
	)
{
	/* predict top field from top field */
	mb_me_search( eparams,
                  org,ref,0,topssmb,
                  eparams.phy_width<<1,i,j>>1,sx,sy>>1,8,
                  eparams.enc_width,eparams.enc_height>>1,
                  &fieldmcs[Parity::top][Parity::top]);

	/* predict top field from bottom field */
	mb_me_search( eparams,
                  org,ref,eparams.phy_width,topssmb, 
                  eparams.phy_width<<1,i,j>>1,sx,sy>>1,8,
                  eparams.enc_width,eparams.enc_height>>1, 
                  &fieldmcs[Parity::bot][Parity::top]);
    
	/* set correct field selectors... */
    // TODO fieldset and fieldoff are redundant.  Use only fieldoff
    // and derive fieldset as fieldoff != 0
	fieldmcs[Parity::top][Parity::top].fieldsel = 0;
	fieldmcs[Parity::bot][Parity::top].fieldsel = 1;
	fieldmcs[Parity::top][Parity::top].fieldoff = 0;
	fieldmcs[Parity::bot][Parity::top].fieldoff = eparams.phy_width;

	/* select prediction for top field */
	if (fieldmcs[Parity::top][Parity::top].sad<=fieldmcs[Parity::bot][Parity::top].sad)
	{
		*besttop = fieldmcs[Parity::top][Parity::top];
	}
	else
	{
		*besttop = fieldmcs[Parity::bot][Parity::top];
	}

	/* predict bottom field from top field */
	mb_me_search( eparams,
                  org,ref,0,botssmb,
                  eparams.phy_width<<1,i,j>>1,sx,sy>>1,8,
                  eparams.enc_width,eparams.enc_height>>1,
                  &fieldmcs[Parity::top][Parity::bot]);

	/* predict bottom field from bottom field */
	mb_me_search( eparams,
                  org,ref,eparams.phy_width,botssmb,
                  eparams.phy_width<<1,i,j>>1,sx,sy>>1,8,
                  eparams.enc_width,eparams.enc_height>>1,
                  &fieldmcs[Parity::bot][Parity::bot]);
    
	/* set correct field selectors... */
	fieldmcs[Parity::top][Parity::bot].fieldsel = 0;
	fieldmcs[Parity::bot][Parity::bot].fieldsel = 1;
	fieldmcs[Parity::top][Parity::bot].fieldoff = 0;
	fieldmcs[Parity::bot][Parity::bot].fieldoff = eparams.phy_width;

	/* select prediction for bottom field */
	if (fieldmcs[Parity::bot][Parity::bot].sad<=fieldmcs[Parity::top][Parity::bot].sad)
	{
		*bestbot = fieldmcs[Parity::bot][Parity::bot];
	}
	else
	{
		*bestbot = fieldmcs[Parity::top][Parity::bot];
	}
}


typedef int (*DualPrimeMeasure)( uint8_t *sameref,
                                 uint8_t *crossref,
                                 uint8_t *pred_mb,
                                 int field_stride,
                                 int same_fx, int same_fy,
                                 int cross_fx, int cross_fy,
                                 int h
    );


bool DualPrimeMetric( const Picture &picture,
                      DualPrimeMeasure meas,
                      const Coord &same,
                      const Coord (&crossblks)[Parity::dim], // Cross-field ref
                      const MotionVector &dmv,
                      uint8_t *ref,
                      uint8_t *pred_mb,
                      int stride,
                      int &measure
    )
{
    Coord cross;
    int part_meas = 0;

    // Base MV is out-of-range
    if( ! picture.InRangeFieldMVRef( same ) )
        return false;

    for( int ppred = Parity::top; ppred <= Parity::bot; ++ppred )
    {
        int same_fieldoff = (ppred == Parity::top ? 0 : stride);
        Coord cross(crossblks[Parity::Invert(ppred)],dmv);
        
        int cross_fieldoff = stride - same_fieldoff;
        if( ! picture.InRangeFieldMVRef( cross ) )
            return false;
        uint8_t *sameref = ref + same_fieldoff
            + (same.x>>1) + (stride<<1)*(same.y>>1);
        uint8_t *crossref = ref + cross_fieldoff
            + (cross.x>>1) + (stride<<1)*(cross.y>>1);
        /* compute prediction error */
        part_meas += meas( sameref, crossref,
                           pred_mb,
                           stride<<1,
                           same.x&1,
                           same.y&1,
                           cross.x&1,
                           cross.y&1,
                           8
            );
    }
    measure = part_meas;
    return true; // All vectors legal...
}


bool 
MacroBlock::FrameDualPrimeCand (uint8_t *ref,
                                const SubSampledImg &ssmb,
                                const MotionCand (&best_fieldmcs)[Parity::dim][Parity::dim], 
                                MotionCand &best_mc,
                                MotionVector &min_dpmv)
{
	int local_dist;
    int stride = picture->encparams.phy_width;
    bool dpmvfound = false;
    // Tricky here we need half-pel field co-ordinates so hpel won't do!
    const Coord mb( Coord::HalfPel(Coord::Field(pel)) );

	/* Calculate Dual Prime distortions for 9 delta candidates
	 * for each of the four minimum field vectors
	 * Note: only for P pictures!
	 */

	/* initialize minimum dual prime distortion to maximum
     * macroblockvariance value. Ensure estimate won't be used if
     * no legal one can be found...
     */
    int best_sad = 256*16*16;
    
    int (&m)[Parity::dim /*ref*/][Parity::dim /*pred*/] =
        dualprime_m[picture->topfirst];

    Coord min_cross[Parity::dim];
    Coord min_same;
    // Iterate over the different suggestions corresponding to the
    // different possible combinations of reference/prediction
    // field parity 
	for ( int psugref=Parity::top; psugref<=Parity::bot; psugref++) 
	{
        // Iterate of over parity predicted MB suggestion
		for ( int psugpred=Parity::top; psugpred<=Parity::top; psugpred++)
		{
			/* convert Cartesian absolute to relative motion vector
			 * values (wrt current macroblock address (i,j)
			 */
            MotionVector suggestion = 
                MotionVector::Frame(best_fieldmcs[psugref][psugpred].pos, mb );

            /* If the candidate base vector is between opposite polarity
               fields we have to convert it to a corresponding dual-prime
               base-vector.  N.b. this may *increase* its magnitude outside the
               legal range so we have to check for this...
            */
            MotionVector base;
            base[Dim::X] = suggestion[Dim::X]*2/m[psugref][psugpred];
            base[Dim::Y] = (suggestion[Dim::Y] - dualprime_e[psugref][psugpred])*2/m[psugref][psugpred];
            if( ! picture->Legal( base ) )
                continue;

            /* We use (legal) base motion vectors for the same
               polarity prediction and to derive the base motion
               vectors for the cross-polarity predictions */
            
            Coord same( mb, base );
            Coord cross[Parity::dim /* ref polarity */];
            for( int pref = Parity::top; pref < Parity::dim; ++pref )
            {
                int ppred = Parity::Invert(pref);
                cross[pref].x = rnddiv2(base[Dim::X] * m[pref][ppred]) 
                    + mb.x;
                cross[pref].y = rnddiv2(base[Dim::Y] * m[pref][ppred])
                    +dualprime_e[pref][ppred]+mb.y;
            }
                              
			/* Now find the best differential motion vector for the
               cross-polarity predictions 
            */

            MotionVector dmv;
            for (dmv[Dim::Y]=-1; dmv[Dim::Y]<=1; ++dmv[Dim::Y])
            {
                for (dmv[Dim::X]=-1; dmv[Dim::X]<=1; ++dmv[Dim::X])
                {
                    local_dist = 0;
                    bool legal =
                        DualPrimeMetric( *picture, pbsad,
                                         same, cross,
                                         dmv,
                                         ref,
                                         ssmb.mb,
                                         stride,
                                         local_dist );

                    /* update best legal MV with smallest distortion
                     * distortion */
                    
                    if ( local_dist < best_sad && legal )
                    {
                        dpmvfound = true;
                        min_dpmv = dmv;
                        best_sad = local_dist;
                        min_same = same;
                        min_cross[Parity::top] = cross[Parity::top];
                        min_cross[Parity::bot] = cross[Parity::bot];
                    }
                }  /* end delta x loop */
            } /* end delta y loop */
		}
	}
    
    if( dpmvfound )
    {
        DualPrimeMetric( *picture, pbsumsq,
                         min_same, min_cross, min_dpmv,
                         ref,
                         ssmb.mb,
                         stride,
                         best_mc.var );
        best_mc.sad = best_sad + mv_coding_penalty( min_same.x-mb.x, 
                                                    min_same.y-mb.y );
        best_mc.pos = min_same;
    }
    return dpmvfound;
}

static void dpfield_estimate(
	const Picture &picture,
	uint8_t *topref,
	uint8_t *botref, 
	uint8_t *mb,
	int i, int j, 
	MotionCand *bestsp_mc,
	MotionCand *bestdp_mc,
	int *vmcp
	)

{
    const EncoderParams &eparams = picture.encparams;
	uint8_t *sameref, *oppref;
	int io0,jo0,io,jo,delta_x,delta_y,mvxs,mvys,mvxo0,mvyo0;
	int imino = 0;
	int jmino = 0;
	int imindmv = 0;
	int jmindmv = 0;
	int vmc_dp,local_dist;
	int imins = 0;
	int jmins = 0;

	/* Calculate Dual Prime distortions for 9 delta candidates */
	/* Note: only for P pictures! */

	/* Assign opposite and same reference pointer */
	if (picture.pict_struct==TOP_FIELD)
	{
		sameref = topref;    
		oppref = botref;
	}
	else 
	{
		sameref = botref;
		oppref = topref;
	}

	/* convert Cartesian absolute to relative motion vector
	 * values (wrt current macroblock address (i,j)
	 */
	mvxs = imins - (i<<1);
	mvys = jmins - (j<<1);

	/* vector for prediction from field of opposite 'parity' */
	mvxo0 = (mvxs+(mvxs>0)) >> 1;  /* mvxs / / */
	mvyo0 = (mvys+(mvys>0)) >> 1;  /* mvys / / 2*/

			/* vertical field shift correction */
	if (picture.pict_struct==TOP_FIELD)
		mvyo0--;
	else
		mvyo0++;

			/* convert back to absolute coordinates */
	io0 = mvxo0 + (i<<1);
	jo0 = mvyo0 + (j<<1);

			/* initialize minimum dual prime distortion to maximum
             * macroblock variance value */
	vmc_dp = 256*256*16*16;

	for (delta_y = -1; delta_y <= 1; delta_y++)
	{
		for (delta_x = -1; delta_x <=1; delta_x++)
		{
			/* opposite field coordinates */
			io = io0 + delta_x;
			jo = jo0 + delta_y;

			if (io >= 0 && io <= (eparams.enc_width-16)<<1 &&
				jo >= 0 && jo <= (eparams.enc_height2-16)<<1)
			{
				/* compute prediction error */
				local_dist = pbsumsq(
					sameref + (imins>>1) + eparams.phy_width2*(jmins>>1),
					oppref  + (io>>1)    + eparams.phy_width2*(jo>>1),
					mb,             /* current mb location */
					eparams.phy_width2,         /* adjacent line distance */
					imins&1, jmins&1, io&1, jo&1, /* half-pel flags */
					16);            /* block height */

				/* update delta with least distortion vector */
				if (local_dist < vmc_dp)
				{
					imino = io;
					jmino = jo;
					imindmv = delta_x;
					jmindmv = delta_y;
					vmc_dp = local_dist;
				}
			}
		}  /* end delta x loop */
	} /* end delta y loop */

	/* Compute L1 error for decision purposes */
	bestdp_mc->sad =
		pbsad(
			sameref + (imins>>1) + eparams.phy_width2*(jmins>>1),
			oppref  + (imino>>1) + eparams.phy_width2*(jmino>>1),
			mb,             /* current mb location */
			eparams.phy_width2,         /* adjacent line distance */
			imins&1, jmins&1, imino&1, jmino&1, /* half-pel flags */
			16);            /* block height */

	bestdp_mc->pos.x = imindmv;
	bestdp_mc->pos.y = jmindmv;
	*vmcp = vmc_dp;
}


/*
 * Collection motion estimates for the different modes for frame pictures
 * picture: picture object for which MC is to be computed.
 *
 * results: a vector of 'plausible' motion estimates.  This function
 * which is a refined version of the original MSSE reference encoder
 * only every puts a single, heuristically 'best', estimate into the vector
 *
 * TODO: MC_DMV should trigger on the current (dynamically selected)
 * bigroup length and not the fixed Maximum bi-group length M.
 */

#ifdef DEBUG_MOTION_EST
const static bool trace_me = false;
#endif

void MacroBlock::FrameMEs()
{
    const Picture &picture = ParentPicture();
    const EncoderParams &eparams = picture.encparams;
    int i = TopleftX();
    int j = TopleftY();

    //
    // Motion info for the various possible motion modes...
    //
	MotionCand framef_mc;
	MotionCand frameb_mc;
	MotionCand dualpf_mc;
	MotionCand topfldf_mc;
	MotionCand botfldf_mc;
	MotionCand topfldb_mc;
	MotionCand botfldb_mc;
	MotionCand zeromot_mc;

    // Pointers to macroblock's contents in YUV and half and quad
    // subsampled planes, and for FIELD modes, the bottom field
    // variants...
	SubSampledImg ssmb;
	SubSampledImg  botssmb;

	MotionCand best_fieldmcs[2][2];
    MotionVector min_dpmv;

    int mb_row_start = j*eparams.phy_width;
	
    MotionEst me;
    best_of_kind_me.clear();

	/* A.Stevens fast motion estimation data is appended to actual
	   luminance information. 
	   TODO: The append thing made sense before we had
	   a nice tidy compression record for each picture but now 
	   it should really be replaced by additional pointers to
	   seperate buffers.
	*/
    ssmb.mb = picture.org_img->Plane(0) + mb_row_start + i;
    ssmb.umb = (uint8_t*)(picture.org_img->Plane(1) + (i>>1) + (mb_row_start>>2));
    ssmb.vmb = (uint8_t*)(picture.org_img->Plane(2) + (i>>1) + (mb_row_start>>2));
    ssmb.fmb = (uint8_t*)(picture.org_img->Plane(0) + eparams.fsubsample_offset + 
						  ((i>>1) + (mb_row_start>>2)));
    ssmb.qmb = (uint8_t*)(picture.org_img->Plane(0) + eparams.qsubsample_offset + 
						  (i>>2) + (mb_row_start>>4));


	/* Zero motion vector - useful for some optimisations
	 */
	zeromot_mc.pos.x = (i<<1);	/* Damn but its messy carrying doubled */
	zeromot_mc.pos.y = (j<<1);	/* absolute Co-ordinates for M/C */
	zeromot_mc.fieldsel = 0;
	zeromot_mc.fieldoff = 0;
    zeromot_mc.blk = picture.fwd_rec->Plane(0)+mb_row_start+i;
    zeromot_mc.hx = zeromot_mc.hy = 0;

	/* Compute variance MB as a measure of Intra-coding complexity
	   We include chrominance information here, scaled to compensate
	   for sub-sampling.  Silly MPEG forcing chrom/lum to have same
	   quantisations... ;-)
	 */

    pvariance(ssmb.mb,16,eparams.phy_width, &lum_variance, &lum_mean );
	int intravar = lum_variance + chrom_var_sum(&ssmb,16,eparams.phy_width);


    // INTRA coding is always an option and always comes first in the list of
    // motion estimates for the available coding options

    me.mb_type = MB_INTRA;
    me.motion_type = 0;
    me.var = intravar;
    me.MV[0][0].Zero();
    best_of_kind_me.push_back( me );

	if (picture.pict_type==P_TYPE)
	{

        /* FRAME mode non-intra coding 0 MV and estimated MVs is
           always possible */
        zeromot_mc.var = unidir_pred_var(&zeromot_mc, ssmb.mb, 
                                         eparams.phy_width, 16 );
        me.mb_type = 0;
        me.motion_type = MC_FRAME;
        me.var =  unidir_var_sum(&zeromot_mc, *picture.fwd_rec, &ssmb,
                                 eparams.phy_width, 16 );
        best_of_kind_me.push_back( me );

        mb_me_search( eparams,
                      picture.fwd_org->Plane(0),picture.fwd_rec->Plane(0),
                      0,
                      &ssmb, eparams.phy_width,
                      i,j,picture.sxf,picture.syf,16,
                      eparams.enc_width,eparams.enc_height, &framef_mc);
        framef_mc.fieldoff = 0;

        me.mb_type = MB_FORWARD;
        me.motion_type=MC_FRAME;
        me.var = unidir_var_sum( &framef_mc, *picture.fwd_rec, &ssmb,
                                              eparams.phy_width, 16 );
        me.MV[0][0] = MotionVector::Frame( framef_mc.pos, hpel );
        best_of_kind_me.push_back( me );
        

        /* FIELD modes only possible if appropriate picture type is legal */
		if (!picture.frame_pred_dct )
		{
			botssmb.mb = ssmb.mb+eparams.phy_width;
			botssmb.fmb = ssmb.fmb+(eparams.phy_width>>1);
			botssmb.qmb = ssmb.qmb+(eparams.phy_width>>2);
			botssmb.umb = ssmb.umb+(eparams.phy_width>>1);
			botssmb.vmb = ssmb.vmb+(eparams.phy_width>>1);

			FieldMotionCands( eparams,
                                        picture.fwd_org->Plane(0), picture.fwd_rec->Plane(0),
                                        &ssmb, &botssmb,
                                        i,j,picture.sxf,picture.syf,
                                        &topfldf_mc,
                                        &botfldf_mc,
                                        best_fieldmcs);

            me.mb_type = MB_FORWARD;
            me.motion_type = MC_FIELD;
            me.var = 
                unidir_var_sum( &topfldf_mc, *picture.fwd_rec, &ssmb,
                                (eparams.phy_width<<1), 8 )
				+ unidir_var_sum( &botfldf_mc, *picture.fwd_rec, &botssmb,
                                  (eparams.phy_width<<1), 8 );
            me.MV[0][0] = MotionVector::Field(topfldf_mc.pos, hpel);
            me.MV[1][0] = MotionVector::Field(botfldf_mc.pos, hpel);
            me.field_sel[0][0] = topfldf_mc.fieldsel;
            me.field_sel[1][0] = botfldf_mc.fieldsel;

            best_of_kind_me.push_back( me );

			if ( eparams.dualprime 
        && FrameDualPrimeCand( picture.fwd_rec->Plane(0), ssmb,
                                       best_fieldmcs, dualpf_mc, min_dpmv ) 
                )
            {
                    me.mb_type = MB_FORWARD;
                    me.motion_type = MC_DMV;
                    me.MV[0][0] = MotionVector::Field(dualpf_mc.pos, hpel);
                    me.dualprimeMV = min_dpmv;
                    // TODO: No actual calculation of chroma variances
                    // just assumed identical per block as luma.
                    me.var = dualpf_mc.var + dualpf_mc.var/2;
                    best_of_kind_me.push_back( me );
            }
        }
	}
	else if (picture.pict_type==B_TYPE)
	{

        /*  FRAME modes: always possible */

        // Forward motion estimates
        mb_me_search( eparams,
                      picture.fwd_org->Plane(0),picture.fwd_rec->Plane(0),0,&ssmb,
                                eparams.phy_width,i,j,picture.sxf,picture.syf,
                                16,eparams.enc_width,eparams.enc_height,
                                &framef_mc
					   );
        framef_mc.fieldoff = 0;
        
        // Backword motion estimates...
        mb_me_search( eparams,
                      picture.bwd_org->Plane(0),picture.bwd_rec->Plane(0),0,&ssmb,
                      eparams.phy_width, i,j,picture.sxb,picture.syb,
                      16, eparams.enc_width, eparams.enc_height,
                      &frameb_mc);
        frameb_mc.fieldoff = 0;

        me.motion_type = MC_FRAME;
        me.MV[0][0][0] = framef_mc.pos.x - (i<<1);
        me.MV[0][0][1] = framef_mc.pos.y - (j<<1);
        me.MV[0][1][0] = frameb_mc.pos.x - (i<<1);
        me.MV[0][1][1] = frameb_mc.pos.y - (j<<1);

        me.mb_type = MB_FORWARD;
        me.var = unidir_var_sum( &framef_mc, *picture.fwd_rec, &ssmb,
                                              eparams.phy_width, 16 );
        best_of_kind_me.push_back( me );
       
        me.mb_type = MB_BACKWARD;
        me.var = unidir_var_sum( &frameb_mc, *picture.bwd_rec, &ssmb,
                                                eparams.phy_width, 16 );
        best_of_kind_me.push_back( me );

        me.mb_type = MB_FORWARD|MB_BACKWARD;
        me.var = bidir_var_sum( &framef_mc, &frameb_mc,  
                                *picture.fwd_rec, *picture.bwd_rec,
                                &ssmb, eparams.phy_width, 16 );
        best_of_kind_me.push_back( me );
	    
        /* FIELD modes only possible if appropriate picture type is legal */

		if (!picture.frame_pred_dct )
		{
			botssmb.mb = ssmb.mb+eparams.phy_width;
			botssmb.fmb = ssmb.fmb+(eparams.phy_width>>1);
			botssmb.qmb = ssmb.qmb+(eparams.phy_width>>2);
			botssmb.umb = ssmb.umb+(eparams.phy_width>>1);
			botssmb.vmb = ssmb.vmb+(eparams.phy_width>>1);

            // Forward motion estimates...
			FieldMotionCands( eparams,
                                        picture.fwd_org->Plane(0),picture.fwd_rec->Plane(0),
                                        &ssmb, &botssmb,
                                        i,j,picture.sxf,picture.syf,
                                        &topfldf_mc,
                                        &botfldf_mc,
                                        best_fieldmcs);
            

			// Backward motion estimates...
			FieldMotionCands( eparams,
                                        picture.bwd_org->Plane(0),picture.bwd_rec->Plane(0),
                                        &ssmb, &botssmb,
                                        i,j,picture.sxb,picture.syb,
                                        &topfldb_mc,
                                        &botfldb_mc,
                                        best_fieldmcs);


            me.motion_type = MC_FIELD;
            me.MV[0][0][0] = topfldf_mc.pos.x - (i<<1);
            me.MV[0][0][1] = (topfldf_mc.pos.y<<1) - (j<<1);
            me.MV[1][0][0] = botfldf_mc.pos.x - (i<<1);
            me.MV[1][0][1] = (botfldf_mc.pos.y<<1) - (j<<1);
            me.field_sel[0][0] = topfldf_mc.fieldsel;
            me.field_sel[1][0] = botfldf_mc.fieldsel;
            me.MV[0][1][0] = topfldb_mc.pos.x - (i<<1);
            me.MV[0][1][1] = (topfldb_mc.pos.y<<1) - (j<<1);
            me.MV[1][1][0] = botfldb_mc.pos.x - (i<<1);
            me.MV[1][1][1] = (botfldb_mc.pos.y<<1) - (j<<1);
            me.field_sel[0][1] = topfldb_mc.fieldsel;
            me.field_sel[1][1] = botfldb_mc.fieldsel;


            me.mb_type = MB_FORWARD|MB_BACKWARD;
            me.var = 
                bidir_var_sum( &topfldf_mc, &topfldb_mc, 
                                        * picture.fwd_rec,*picture.bwd_rec,
                                        &ssmb, eparams.phy_width<<1, 8) 
                +  bidir_var_sum( &botfldf_mc, &botfldb_mc, 
                                        *picture.fwd_rec,*picture.bwd_rec,
                                        &botssmb, eparams.phy_width<<1, 8);
            best_of_kind_me.push_back( me );

            me.mb_type = MB_FORWARD;
            me.var = 
                unidir_var_sum( &topfldf_mc, *picture.fwd_rec, &ssmb,
                                (eparams.phy_width<<1), 8 )
                + unidir_var_sum( &botfldf_mc, *picture.fwd_rec, &botssmb,
                                  (eparams.phy_width<<1), 8 );
            best_of_kind_me.push_back( me );

            me.mb_type = MB_BACKWARD;
            me.var =  
                unidir_var_sum( &topfldb_mc, *picture.bwd_rec, &ssmb,
                                (eparams.phy_width<<1), 8 )
                + unidir_var_sum( &botfldb_mc, *picture.bwd_rec, &botssmb,
                                  (eparams.phy_width<<1), 8 );
            best_of_kind_me.push_back( me );
            
		}
	}

}



/*
 * motion estimation for field pictures
 * picture: picture object for which MC is to be computed.
 * mbi:    pointer to macroblock info of picture object
 * mb_row_start: offset in chrominance block of start of this MB's row
 *
 * results:
 * mbi->
 *  me.mb_type: 0, MB_INTRA, MB_FORWARD, MB_BACKWARD, MB_FORWARD|MB_BACKWARD
 *  me.MV[][][]: motion vectors (field format)
 *  me.field_sel: top/bottom field
 *  me.motion_type: MC_FIELD, MC_16X8
 *
 */
void MacroBlock::FieldME()
{
    const Picture &picture = ParentPicture();
    const EncoderParams &eparams = picture.encparams;
    int i = TopleftX();
    int j = TopleftY();
	int w2;
	uint8_t *toporg, *topref, *botorg, *botref;
	SubSampledImg ssmb;
	MotionCand fieldsp_mc, dualp_mc;
	MotionCand fieldf_mc, fieldb_mc;
	MotionCand field8uf_mc, field8lf_mc;
	MotionCand field8ub_mc, field8lb_mc;
	int intravar, vmc,v0,dmc,dmcfieldi,dmcfield,dmcfieldf,dmcfieldr,dmc8i;
	int dmc8f,dmc8r;
	int vmc_dp,dctl_dp;

    MotionEst me;

	w2 = eparams.phy_width<<1;

	/* Fast motion data sits at the end of the luminance buffer */
    ssmb.mb = picture.org_img->Plane(0) + i + w2*j;
    ssmb.umb = picture.org_img->Plane(1) + ((i>>1)+(w2>>1)*(j>>1));
    ssmb.vmb = picture.org_img->Plane(2) + ((i>>1)+(w2>>1)*(j>>1));
    ssmb.fmb = picture.org_img->Plane(0) + eparams.fsubsample_offset+((i>>1)+(w2>>1)*(j>>1));
    ssmb.qmb = picture.org_img->Plane(0) + eparams.qsubsample_offset+ (i>>2)+(w2>>2)*(j>>2);

	if (picture.pict_struct==BOTTOM_FIELD)
	{
		ssmb.mb += eparams.phy_width;
		ssmb.umb += (eparams.phy_width >> 1);
		ssmb.vmb += (eparams.phy_width >> 1);
		ssmb.fmb += (eparams.phy_width >> 1);
		ssmb.qmb += (eparams.phy_width >> 2);
	}

    pvariance( ssmb.mb, 16, w2, &lum_variance, &lum_mean );
	intravar = lum_variance + chrom_var_sum(&ssmb,16,w2);

    // INTRA coding is always an option and always comes first in the list of
    // motion estimates for the available coding options
    me.mb_type = MB_INTRA;
    me.motion_type = 0;
    me.var = intravar;
    me.MV[0][0].Zero();
    best_of_kind_me.clear();
    best_of_kind_me.push_back( me );

	if(picture.pict_type==I_TYPE)
    {
		me.mb_type = MB_INTRA;
        me.var = intravar;
    }
	else if (picture.pict_type==P_TYPE)
	{
        toporg = picture.fwd_org->Plane(0);
        topref = picture.fwd_rec->Plane(0);
        botorg = picture.fwd_org->Plane(0);
        botref = picture.fwd_rec->Plane(0);
                                                        
		if (picture.secondfield)
		{
			/* opposite parity field is in same frame */
			if (picture.pict_struct==TOP_FIELD)
			{
				/* current is top field */
                botorg = picture.org_img->Plane(0) ;
                botref = picture.rec_img->Plane(0);
			}
			else
			{
				/* current is bottom field */
                toporg = picture.org_img->Plane(0);
                topref = picture.rec_img->Plane(0);
			}
		}
		field_estimate(picture,
					   toporg,topref,botorg,botref,&ssmb,
					   i,j,picture.sxf,picture.syf,
					   &fieldf_mc,
					   &field8uf_mc,
					   &field8lf_mc,
					   &fieldsp_mc);
		dmcfield = fieldf_mc.sad;
		dmc8f = field8uf_mc.sad + field8lf_mc.sad;
		dctl_dp = 100000000;		/* Suppress compiler warning */

		if ( eparams.dualprime && !picture.ipflag)  /* generic condition which permits Dual Prime */
		{
			dpfield_estimate(picture,
							 topref,botref,ssmb.mb,i,j,
							 &fieldsp_mc,
							 &dualp_mc,
							 &vmc_dp);
			dctl_dp = dualp_mc.sad;
		}
		/* select between dual prime, field and 16x8 prediction */
		if ( eparams.M==1 && !picture.ipflag && dctl_dp<dmc8f && dctl_dp<dmcfield)
		{
			/* Dual Prime prediction */
			me.motion_type = MC_DMV;
			dmc = dualp_mc.sad;
			vmc = vmc_dp;

		}
		else if (dmc8f<dmcfield)
		{
			/* 16x8 prediction */
			me.motion_type = MC_16X8;
			/* upper and lower half blocks */
			vmc =  field8uf_mc.var + unidir_pred_var( &field8uf_mc, ssmb.mb, w2, 8);
			vmc += field8lf_mc.var + unidir_pred_var( &field8lf_mc, ssmb.mb, w2, 8);
		}
		else
		{
			/* field prediction */
			me.motion_type = MC_FIELD;
			vmc = fieldf_mc.var + unidir_pred_var( &fieldf_mc, ssmb.mb, w2, 16 );
		}

		/* select between intra and non-intra coding */
        
		if ( vmc>intravar && vmc > 12*256)
        {
			me.mb_type = MB_INTRA;
            me.var = intravar;
        }
		else
		{
			/* zero MV field prediction from same parity ref. field
			 * (not allowed if ipflag is set)
			 */
			if (!picture.ipflag)
				v0 = psumsq(((picture.pict_struct==BOTTOM_FIELD) ? botref : topref) + i + w2*j,
							   ssmb.mb,w2,0,0,16);
			else
				v0 = 1234;			/* Keep Compiler happy... */

			if (picture.ipflag  || (4*v0>5*vmc ))
			{
				me.mb_type = MB_FORWARD;
                me.var = vmc;
				if (me.motion_type==MC_FIELD)
				{
					me.MV[0][0][0] = fieldf_mc.pos.x - (i<<1);
					me.MV[0][0][1] = fieldf_mc.pos.y - (j<<1);
					me.field_sel[0][0] = fieldf_mc.fieldsel;
				}
				else if (me.motion_type==MC_DMV)
				{
					/* same parity vector */
					me.MV[0][0][0] = fieldsp_mc.pos.x - (i<<1);
					me.MV[0][0][1] = fieldsp_mc.pos.y - (j<<1);

					/* opposite parity vector */
					me.dualprimeMV[0] = dualp_mc.pos.x;
					me.dualprimeMV[1] = dualp_mc.pos.y;
				}
				else
				{
					me.MV[0][0][0] = field8uf_mc.pos.x - (i<<1);
					me.MV[0][0][1] = field8uf_mc.pos.y - (j<<1);
					me.MV[1][0][0] = field8lf_mc.pos.x - (i<<1);
					me.MV[1][0][1] = field8lf_mc.pos.y - ((j+8)<<1);
					me.field_sel[0][0] = field8uf_mc.fieldsel;
					me.field_sel[1][0] = field8lf_mc.fieldsel;
				}
			}
			else
			{
				/* No MC */
				me.mb_type = 0;
                me.var = v0;
				me.motion_type = MC_FIELD;
				me.MV[0][0][0] = 0;
				me.MV[0][0][1] = 0;
				me.field_sel[0][0] = (picture.pict_struct==BOTTOM_FIELD);
			}
		}
	}
	else /* if (pict_type==B_TYPE) */
	{
		/* forward prediction */
		field_estimate( picture,
                                picture.fwd_org->Plane(0),
                                picture.fwd_rec->Plane(0),
                                picture.fwd_org->Plane(0),
                                picture.fwd_rec->Plane(0),
                                &ssmb,i,j,picture.sxf,picture.syf,
                                &fieldf_mc,
                                &field8uf_mc,
                                &field8lf_mc,
                                &fieldsp_mc);
		dmcfieldf = fieldf_mc.sad;
		dmc8f = field8uf_mc.sad + field8lf_mc.sad;

		/* backward prediction */
		field_estimate( picture,
                                picture.bwd_org->Plane(0),
                                picture.bwd_rec->Plane(0),
                                picture.bwd_org->Plane(0),
                                picture.bwd_rec->Plane(0),
                                &ssmb,i,j,picture.sxb,picture.syb,
                                &fieldb_mc,
                                &field8ub_mc,
                                &field8lb_mc,
                                &fieldsp_mc);
		dmcfieldr = fieldb_mc.sad;
		dmc8r = field8ub_mc.sad + field8lb_mc.sad;

		/* calculate distances for bidirectional prediction */
		/* field */
		dmcfieldi = bidir_pred_sad( &fieldf_mc, &fieldb_mc, ssmb.mb, w2, 16);

		/* 16x8 upper and lower half blocks */
		dmc8i =  bidir_pred_sad( &field8uf_mc, &field8ub_mc, ssmb.mb, w2, 16 );
		dmc8i += bidir_pred_sad( &field8lf_mc, &field8lb_mc, ssmb.mb, w2, 16 );

		/* select prediction type of minimum distance */
		if (dmcfieldi<dmc8i && dmcfieldi<dmcfieldf && dmcfieldi<dmc8f
			&& dmcfieldi<dmcfieldr && dmcfieldi<dmc8r)
		{
			/* field, interpolated */
			me.mb_type = MB_FORWARD|MB_BACKWARD;
			me.motion_type = MC_FIELD;
			vmc = fieldf_mc.var + bidir_pred_var( &fieldf_mc, &fieldb_mc, ssmb.mb, w2, 16);
		}
		else if (dmc8i<dmcfieldf && dmc8i<dmc8f
				 && dmc8i<dmcfieldr && dmc8i<dmc8r)
		{
			/* 16x8, interpolated */
			me.mb_type = MB_FORWARD|MB_BACKWARD;
			me.motion_type = MC_16X8;

			/* upper and lower half blocks */
			vmc =  field8uf_mc.var + bidir_pred_var( &field8uf_mc, &field8ub_mc, ssmb.mb, w2, 8);
			vmc += field8lf_mc.var + bidir_pred_var( &field8lf_mc, &field8lb_mc, ssmb.mb, w2, 8);
		}
		else if (dmcfieldf<dmc8f && dmcfieldf<dmcfieldr && dmcfieldf<dmc8r)
		{
			/* field, forward */
			me.mb_type = MB_FORWARD;
			me.motion_type = MC_FIELD;
			vmc = fieldf_mc.var + unidir_pred_var( &fieldf_mc, ssmb.mb, w2, 16);
		}
		else if (dmc8f<dmcfieldr && dmc8f<dmc8r)
		{
			/* 16x8, forward */
			me.mb_type = MB_FORWARD;
			me.motion_type = MC_16X8;

			/* upper and lower half blocks */
			vmc = field8uf_mc.var +  unidir_pred_var( &field8uf_mc, ssmb.mb, w2, 8);
			vmc += field8lf_mc.var + unidir_pred_var( &field8lf_mc, ssmb.mb, w2, 8);
		}
		else if (dmcfieldr<dmc8r)
		{
			/* field, backward */
			me.mb_type = MB_BACKWARD;
			me.motion_type = MC_FIELD;
			vmc = fieldb_mc.var + unidir_pred_var( &fieldb_mc, ssmb.mb, w2, 16 );
		}
		else
		{
			/* 16x8, backward */
			me.mb_type = MB_BACKWARD;
			me.motion_type = MC_16X8;

			/* upper and lower half blocks */
			vmc =  field8ub_mc.var + unidir_pred_var( &field8ub_mc, ssmb.mb, w2, 8);
			vmc += field8lb_mc.var + unidir_pred_var( &field8lb_mc, ssmb.mb, w2, 8);

		}


		/* select between intra and non-intra coding */
		if (vmc>intravar && vmc > 12*256)
        {
			me.mb_type = MB_INTRA;
            me.var = intravar;
        }
		else
		{
            me.var = vmc;
			if (me.motion_type==MC_FIELD)
			{
				/* forward */
				me.MV[0][0][0] = fieldf_mc.pos.x - (i<<1);
				me.MV[0][0][1] = fieldf_mc.pos.y - (j<<1);
				me.field_sel[0][0] = fieldf_mc.fieldsel;
				/* backward */
				me.MV[0][1][0] = fieldb_mc.pos.x - (i<<1);
				me.MV[0][1][1] = fieldb_mc.pos.y - (j<<1);
				me.field_sel[0][1] = fieldb_mc.fieldsel;
			}
			else /* MC_16X8 */
			{
				/* forward */
				me.MV[0][0][0] = field8uf_mc.pos.x - (i<<1);
				me.MV[0][0][1] = field8uf_mc.pos.y - (j<<1);
				me.field_sel[0][0] = field8uf_mc.fieldsel;
				me.MV[1][0][0] = field8lf_mc.pos.x - (i<<1);
				me.MV[1][0][1] = field8lf_mc.pos.y - ((j+8)<<1);
				me.field_sel[1][0] = field8lf_mc.fieldsel;
				/* backward */
				me.MV[0][1][0] = field8ub_mc.pos.x - (i<<1);
				me.MV[0][1][1] = field8ub_mc.pos.y - (j<<1);
				me.field_sel[0][1] = field8ub_mc.fieldsel;
				me.MV[1][1][0] = field8lb_mc.pos.x - (i<<1);
				me.MV[1][1][1] = field8lb_mc.pos.y - ((j+8)<<1);
				me.field_sel[1][1] = field8lb_mc.fieldsel;
			}
		}
	}

    if( me.mb_type != MB_INTRA )
    {
        best_of_kind_me.push_back( me );
    }

}

/*
 * frame picture field mode motion estimates...
 *
 * org: top left pel of source reference frame
 * ref: top left pel of reconstructed reference frame
 * ssmb:  macroblock to be matched
 * i,j: location of mb relative to ref (=center of search window)
 * sx,sy: half widths of search window
 * besttop: location of best field pred. for top field of mb
 * bestbo : location of best field pred. for bottom field of mb
 */


/*
 * field picture motion estimation subroutine
 *
 * toporg: address of frame holding original top reference field
 * topref: address of frame holding reconstructed top reference field
 * botorg: address of frame holding original bottom reference field
 * botref: address of frame holding reconstructed bottom reference field
 * ssmmb:  macroblock to be matched
 * i,j: location of mb (=center of search window)
 * sx,sy: half width/height of search window
 *
 * bestfld: location and distance of best field prediction
 * best8u: location of best 16x8 pred. for upper half of mb
 * best8lp: location of best 16x8 pred. for lower half of mb
 * bdestsp: location and distance of best same parity field
 *                    prediction (needed for dual prime, only valid if
 *                    ipflag==0)
 */

static void field_estimate (
	const Picture &picture,
	uint8_t *toporg,
	uint8_t *topref, 
	uint8_t *botorg, 
	uint8_t *botref,
	SubSampledImg *ssmb,
	int i, int j, int sx, int sy,
	MotionCand *bestfld,
	MotionCand *best8u,
	MotionCand *best8l,
	MotionCand *bestsp)

{
    const EncoderParams &eparams = picture.encparams;
	MotionCand topfld_mc;
	MotionCand botfld_mc;
	int dt, db;
	int notop, nobot;
	SubSampledImg botssmb;

	botssmb.mb = ssmb->mb+eparams.phy_width;
	botssmb.umb = ssmb->umb+(eparams.phy_width>>1);
	botssmb.vmb = ssmb->vmb+(eparams.phy_width>>1);
	botssmb.fmb = ssmb->fmb+(eparams.phy_width>>1);
	botssmb.qmb = ssmb->qmb+(eparams.phy_width>>2);

	/* if ipflag is set, predict from field of opposite parity only */
	notop = picture.ipflag && (picture.pict_struct==TOP_FIELD);
	nobot = picture.ipflag && (picture.pict_struct==BOTTOM_FIELD);

	/* field prediction */

	/* predict current field from top field */
	if (notop)
		topfld_mc.sad = dt = 65536; /* infinity */
	else
		mb_me_search(eparams,
                     toporg,topref,0,ssmb,
                     eparams.phy_width<<1, i,j,sx,sy>>1,16,
                     eparams.enc_width,eparams.enc_height>>1, &topfld_mc);
	dt = topfld_mc.sad;
	/* predict current field from bottom field */
	if (nobot)
		botfld_mc.sad = db = 65536; /* infinity */
	else
		mb_me_search(eparams,
                     botorg,botref,eparams.phy_width,ssmb,
                     eparams.phy_width<<1, i,j,sx,sy>>1,16,
                     eparams.enc_width,eparams.enc_height>>1, &botfld_mc);
	db = botfld_mc.sad;
	/* Set correct field selectors */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = eparams.phy_width;

	/* same parity prediction (only valid if ipflag==0) */
	if (picture.pict_struct==TOP_FIELD)
	{
		*bestsp = topfld_mc;
	}
	else
	{
		*bestsp = botfld_mc;
	}

	/* select field prediction */
	if (dt<=db)
	{
		*bestfld = topfld_mc;
	}
	else
	{
		*bestfld = botfld_mc;
	}


	/* 16x8 motion estimation */

	/* predict upper half field from top field */

	if (notop)
		topfld_mc.sad = dt = 65536;
	else
		mb_me_search(eparams,
                     toporg,topref,0,ssmb,
                     eparams.phy_width<<1, i,j,sx,sy>>1,8,
                     eparams.enc_width,eparams.enc_height>>1,&topfld_mc);
	dt = topfld_mc.sad;
	/* predict upper half field from bottom field */
	if (nobot)
		botfld_mc.sad = db = 65536;
	else
		mb_me_search(eparams,
                     botorg,botref,eparams.phy_width,ssmb,
                     eparams.phy_width<<1, i,j,sx,sy>>1,8,
                     eparams.enc_width,
                     eparams.enc_height>>1,&botfld_mc);
	db = botfld_mc.sad;

	/* Set correct field selectors */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = eparams.phy_width;

	/* select prediction for upper half field */
	if (dt<=db)
	{
		*best8u = topfld_mc;
	}
	else
	{
		*best8u = botfld_mc;
	}

	/* predict lower half field from top field */
	/*
	  N.b. For interlaced data width<<4 (width*16) takes us 8 rows
	  down in the same field.  
	  Thus for the fast motion data (2*2
	  sub-sampled) we need to go 4 rows down in the same field.
	  This requires adding width*4 = (width<<2).
	  For the 4*4 sub-sampled motion data we need to go down 2 rows.
	  This requires adding width = width
	 
	*/
	if (notop)
		topfld_mc.sad = dt = 65536;
	else
		mb_me_search(eparams,
                     toporg,topref,0,&botssmb,
                     eparams.phy_width<<1, i,j+8,sx,sy>>1,8,
                     eparams.enc_width,eparams.enc_height>>1, &topfld_mc);
	dt = topfld_mc.sad;
	/* predict lower half field from bottom field */
	if (nobot)
		botfld_mc.sad = db = 65536;
	else
		mb_me_search(eparams,
                     botorg,botref,eparams.phy_width,&botssmb,
                     eparams.phy_width<<1,i,j+8,sx,sy>>1,8,
                     eparams.enc_width,eparams.enc_height>>1, &botfld_mc);
	db = botfld_mc.sad;
	/* Set correct field selectors */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = eparams.phy_width;

	/* select prediction for lower half field */
	if (dt<=db)
	{
		*best8l = topfld_mc;
	}
	else
	{
		*best8l = botfld_mc;
	}
}





 
/* Hierarchical block matching motion estimation search
 *
 * A.Stevens 2000: This is now a big misnomer.  The search is now a
 * hierarchical/sub-sampling search not a full search.  However,
 * experiments have shown it is always close to optimal and almost
 * always very close or optimal.
 *
 * org: top left pel of source reference frame
 * ref: top left pel of reconstructed reference frame
 * fieldoff - Offset to top left pel relevant field in org and ref
 *          if we're doing by-field matching 
 * ssblk: top-left element of macro block to be motion compensated
 *        at 1*1,2*2 and 4*4 subsampling
 * lx: distance (in bytes) of vertically adjacent pels in ref,blk
 *     This is twice the physical line length if we're doing by-field
 *     matching otherwise the physical line length
 * i0,j0: center of search window
 * sx,sy: half widths of search window
 * h: height of macro block
 * xmax,ymax: right/bottom limits of search area for macro block
 * res: pointers to where the result is stored
 *      N.b. as in the original code result is given as
 *      half pel offset from ref(0,0) not the position relative to i0 j0
 *      as will actually be used.
 *
 * TODO: SHould use half-pel co-ordinates relative to i0,j0 for motion vectors
 * throughout the motion estimation code but this would be damn fiddly to
 * do without introducing lots of tricky-to-find bugs.
 *
 */
 
#ifdef DEBUG_MOTION_EST
static void log_result_set( me_result_set *rs )
{
    int i;
    for( i = 0; i < rs->len; ++i )
        printf( "%03d: %6d %3d %3d\n", 
                i, rs->mests[i].weight, 
                2*rs->mests[i].x, 2*rs->mests[i].y );
}

static int hash( uint8_t *blk, int stride )
{
    int i,j;
    int sum = 0;
    for( j= 0; j <16; ++j )
        for( i = 0; i <16 ; ++i)
            sum += *(blk+i+j*stride);
    return sum;
}

static int dump( uint8_t *blk, int stride )
{
    int i,j;
    int sum = 0;
    for( j= 0; j <16; ++j )
    {
        for( i = 0; i <16 ; ++i)
            printf( "%02x ", *(blk+i+j*stride) );
        if( j & 1 )
            printf( "\n" );
        
    }
    return sum;
}
#endif

static void mb_me_search(
    const EncoderParams &eparams,
	uint8_t *org,
	uint8_t *ref,
    int fieldoff,
	SubSampledImg *ssblk,
	int lx, int i0, int j0, 
	int sx, int sy, int h,
	int xmax, int ymax,
	MotionCand *res
	)
{
	me_result_s best;
	int i,j,ilow,ihigh,jlow,jhigh;
	int x,y;
	int d;

	/* NOTE: Surprisingly, the initial motion estimation search
	   works better when the original image not the reference (reconstructed)
	   image is used. 
	*/
	uint8_t *s22org = (uint8_t*)(org+eparams.fsubsample_offset+(fieldoff>>1));
	uint8_t *s44org = (uint8_t*)(org+eparams.qsubsample_offset+(fieldoff>>2));
	uint8_t *orgblk;

    uint8_t *reffld = ref+fieldoff;

	int flx = lx >> 1;
	int qlx = lx >> 2;
	int fh = h >> 1;
	int qh = h >> 2;

	me_result_set sub44set;
	me_result_set sub22set;

	/* xmax and ymax into more useful form... */
	xmax -= 16;
	ymax -= h;
  
  
  	/* The search radii are *always* multiples of 4 to avoid messiness
	   in the initial 4*4 pel search.  This is handled by the
	   parameter checking/processing code in readparmfile() */
  
	/* Create a distance-order mests of possible motion estimations
	  based on the fast estimation data - 4*4 pel sums (4*4
	  sub-sampled) rather than actual pel's.  1/16 the size...  */
	jlow = j0-sy;
	jlow = jlow < 0 ? 0 : jlow;
	jhigh =  j0+(sy-1);
	jhigh = jhigh > ymax ? ymax : jhigh;
	ilow = i0-sx;
	ilow = ilow < 0 ? 0 : ilow;
	ihigh =  i0+(sx-1);
	ihigh = ihigh > xmax ? xmax : ihigh;

	/*
 	   Very rarely this may fail to find matchs due to all the good
	   looking ones being over threshold. hence we make sure we
	   fall back to a 0 motion estimation in this case.
	   
		 The sad for the 0 motion estimation is also very useful as
		 a basis for setting thresholds for rejecting really dud 4*4
		 and 2*2 sub-sampled matches.
	*/
	best.weight = psad_00(reffld+i0+j0*lx,ssblk->mb,lx,h,INT_MAX);
	best.x = 0;
	best.y = 0;

	/* Generate the best matches at 4*4 sub-sampling. 
	   The precise fraction of the matches included is
	   controlled by eparams.44_red
	   Note: we use the original picture here for the match...
	 */


	pbuild_sub44_mests( &sub44set,
                        ilow, jlow, ihigh, jhigh,
                        i0, j0,
                        best.weight,
                        s44org, 
                        ssblk->qmb, qlx, qh,
                        eparams.me44_red); 
#ifdef DEBUG_MOTION_EST
    if( trace_me )
        log_result_set( &sub44set );
#endif	
	/* Generate the best 2*2 sub-sampling matches from the
	   immediate 2*2 neighbourhoods of the 4*4 sub-sampling matches.
	   The precise fraction of the matches included is controlled
	   by eparams.22_red.
	   Note: we use the original picture here for the match...

	*/

	pbuild_sub22_mests( &sub44set, &sub22set,
                        i0, j0, 
                        ihigh,  jhigh, 
                        best.weight,
                        s22org, ssblk->fmb, flx, fh,
                        eparams.me22_red);

#ifdef DEBUG_MOTION_EST
    if( trace_me )
        log_result_set( &sub22set );
#endif
		
    /* Now choose best 1-pel match from the 2*2 neighbourhoods
	   of the best 2*2 sub-sampled matches.
	   Note that here we start using the reference picture not the
	   original.
	*/
	

	pfind_best_one_pel( &sub22set,
                        reffld, ssblk->mb, 
                        i0, j0,
                        ihigh, jhigh, 
                        lx, h, &best );

#ifdef DEBUG_MOTION_EST
    if( trace_me )
    {
        printf( "BST: %6d %3d %3d @ %6d %7d (%7d)\n", 
                best.weight, 
                2*best.x, 2*best.y,
                (i0+best.x)+lx*(j0+best.y),
                hash(reffld+(i0+best.x)+lx*(j0+best.y), lx),
                hash(ssblk->mb, lx)                
            );
        dump( reffld+(i0+best.x)+lx*(j0+best.y), lx );
    };
#endif
	/* Final polish: half-pel search of best 1*1 against
	   reconstructed image. 
	*/
	res->sad = INT_MAX;
	x = (i0+best.x)<<1;
	y = (j0+best.y)<<1;

	/* Narrow search box to half-pel's around best 1-pel match - half-pel
	   units....*/
	ilow = x - (x>(ilow<<1));
	ihigh = x + (x<((ihigh)<<1));
	jlow = y - (y>(jlow<<1));
	jhigh =  y+ (y<((jhigh)<<1));

	for (j=jlow; j<=jhigh; j++)
	{
		for (i=ilow; i<=ihigh; i++)
		{
			orgblk = reffld+(i>>1)+((j>>1)*lx);
			if( i&1 )
			{
				if( j & 1 )
					d = psad_11(orgblk,ssblk->mb,lx,h);
				else
					d = psad_01(orgblk,ssblk->mb,lx,h);
			}
			else
			{
				if( j & 1 )
					d = psad_10(orgblk,ssblk->mb,lx,h);
				else
					d = psad_00(orgblk,ssblk->mb,lx,h,res->sad);
			}
            // TODO: Mismatches motionsearch...
#ifdef DEBUG_MOTION_EST
            if( trace_me )
                printf( "BSS: %6d %3d %3d @ %6d %7d\n", 
                        d, 
                        i, j,
                        orgblk-reffld,
                        hash(orgblk, lx) );
#endif
			d += mv_coding_penalty(i-(i0<<1),j-(j0<<1));
			if (d<res->sad)
			{

				res->sad = d;
				res->pos.x = i;
				res->pos.y = j;
				res->blk = orgblk;
				res->hx = i&1;
				res->hy = j&1;
			}
		}
	}
	res->var = psumsq(res->blk, ssblk->mb, lx, res->hx, res->hy, h);

}




/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
