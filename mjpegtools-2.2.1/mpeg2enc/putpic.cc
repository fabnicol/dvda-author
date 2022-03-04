/* putpic.c, block and motion vector encoding routines                      */

 
/*  (C) 2000-2005 Andrew Stevens */

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
 
/* Original reference encoder from which this derived:
 * Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

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



#include <config.h>
#include <stdio.h>
#include <cassert>
#include "mpeg2syntaxcodes.h"
#include "tables.h"
#include "simd.h"
#include "mpeg2encoder.hh"
#include "mpeg2coder.hh"
#include "ratectl.hh"
#include "macroblock.hh"
#include "picture.hh"

    
/* output motion vectors (6.2.5.2, 6.3.16.2)
 *
 * this routine also updates the predictions for motion vectors (PMV)
 */
void Picture::PutMVs( MotionEst &me, bool back )

{
	int hor_f_code;
	int vert_f_code;

	if( back )
	{
		hor_f_code = back_hor_f_code;
		vert_f_code = back_vert_f_code;
	}
	else
	{
		hor_f_code = forw_hor_f_code;
		vert_f_code = forw_vert_f_code;
	}

	if (pict_struct==FRAME_PICTURE)
	{
		if (me.motion_type==MC_FRAME)
		{
			/* frame prediction */
			coding->PutMV(me.MV[0][back][0]-PMV[0][back][0],hor_f_code);
			coding->PutMV(me.MV[0][back][1]-PMV[0][back][1],vert_f_code);
			PMV[0][back][0]=PMV[1][back][0]=me.MV[0][back][0];
			PMV[0][back][1]=PMV[1][back][1]=me.MV[0][back][1];
		}
		else if (me.motion_type==MC_FIELD)
		{
			/* field prediction */

			coding->PutBits(me.field_sel[0][back],1);
			coding->PutMV(me.MV[0][back][0]-PMV[0][back][0],hor_f_code);
			coding->PutMV((me.MV[0][back][1]>>1)-(PMV[0][back][1]>>1),vert_f_code);
			coding->PutBits(me.field_sel[1][back],1);
			coding->PutMV(me.MV[1][back][0]-PMV[1][back][0],hor_f_code);
			coding->PutMV((me.MV[1][back][1]>>1)-(PMV[1][back][1]>>1),vert_f_code);
			PMV[0][back][0]=me.MV[0][back][0];
			PMV[0][back][1]=me.MV[0][back][1];
			PMV[1][back][0]=me.MV[1][back][0];
			PMV[1][back][1]=me.MV[1][back][1];

		}
		else
		{
#ifdef DEBUG_DPME
            MotionVector DMV[Parity::dim /*pred*/];
                        calc_DMV(*this,
                         DMV,
                         me.dualprimeMV,
                         me.MV[0][0][0],
                         me.MV[0][0][1]>>1);
                         
            printf( "PR%06d: %03d %03d %03d %03d %03d %03d\n", dp_mv,
                me.MV[0][0][0], (me.MV[0][0][1]>>1), DMV[0][0], DMV[0][1], DMV[1][0], DMV[1][1] );
            ++dp_mv;
            if( dp_mv == 45000 )
                exit(0);
#endif
			/* dual prime prediction */
			coding->PutMV(me.MV[0][back][0]-PMV[0][back][0],hor_f_code);
			coding->PutDMV(me.dualprimeMV[0]);
			coding->PutMV((me.MV[0][back][1]>>1)-(PMV[0][back][1]>>1),vert_f_code);
			coding->PutDMV(me.dualprimeMV[1]);
			PMV[0][back][0]=PMV[1][back][0]=me.MV[0][back][0];
			PMV[0][back][1]=PMV[1][back][1]=me.MV[0][back][1];
		}
	}
	else
	{
		/* field picture */
		if (me.motion_type==MC_FIELD)
		{
			/* field prediction */
			coding->PutBits(me.field_sel[0][back],1);
			coding->PutMV(me.MV[0][back][0]-PMV[0][back][0],hor_f_code);
			coding->PutMV(me.MV[0][back][1]-PMV[0][back][1],vert_f_code);
			PMV[0][back][0]=PMV[1][back][0]=me.MV[0][back][0];
			PMV[0][back][1]=PMV[1][back][1]=me.MV[0][back][1];
		}
		else if (me.motion_type==MC_16X8)
		{
			/* 16x8 prediction */
			coding->PutBits(me.field_sel[0][back],1);
			coding->PutMV(me.MV[0][back][0]-PMV[0][back][0],hor_f_code);
			coding->PutMV(me.MV[0][back][1]-PMV[0][back][1],vert_f_code);
			coding->PutBits(me.field_sel[1][back],1);
			coding->PutMV(me.MV[1][back][0]-PMV[1][back][0],hor_f_code);
			coding->PutMV(me.MV[1][back][1]-PMV[1][back][1],vert_f_code);
			PMV[0][back][0]=me.MV[0][back][0];
			PMV[0][back][1]=me.MV[0][back][1];
			PMV[1][back][0]=me.MV[1][back][0];
			PMV[1][back][1]=me.MV[1][back][1];
		}
		else
		{
			/* dual prime prediction */
			coding->PutMV(me.MV[0][back][0]-PMV[0][back][0],hor_f_code);
			coding->PutDMV(me.dualprimeMV[0]);
			coding->PutMV(me.MV[0][back][1]-PMV[0][back][1],vert_f_code);
			coding->PutDMV(me.dualprimeMV[1]);
			PMV[0][back][0]=PMV[1][back][0]=me.MV[0][back][0];
			PMV[0][back][1]=PMV[1][back][1]=me.MV[0][back][1];
		}
	}
}

void Picture::PutDCTBlocks( MacroBlock &mb, int mb_type )
{
    int comp;
    int cc;
    for (comp=0; comp<BLOCK_COUNT; comp++)
    {
        /* block loop */
        if( mb.cbp & (1<<(BLOCK_COUNT-1-comp)))
        {
            if (mb_type & MB_INTRA)
            {
                // TODO: 420 Only?
                cc = (comp<4) ? 0 : (comp&1)+1;
                coding->PutIntraBlk(this, mb.QuantDCTblocks()[comp],cc);
            }
            else
            {
                coding->PutNonIntraBlk(this,mb.QuantDCTblocks()[comp]);
            }
        }
    }
}


/* generate picture header (6.2.3, 6.3.10) */
void Picture::PutHeader()
{
	assert( coding->Aligned() );
	coding->PutBits(PICTURE_START_CODE,32); /* picture_start_code */
	coding->PutBits(temp_ref,10); /* temporal_reference */
	coding->PutBits(pict_type,3); /* picture_coding_type */
	coding->PutBits(FFFF_VBV_DELAY,16); /* vbv_delay */

	if (pict_type==P_TYPE || pict_type==B_TYPE)
	{
		coding->PutBits(0,1); /* full_pel_forward_vector */
		if (encparams.mpeg1)
			coding->PutBits(forw_hor_f_code,3);
		else
			coding->PutBits(7,3); /* forward_f_code */
	}

	if (pict_type==B_TYPE)
	{
		coding->PutBits(0,1); /* full_pel_backward_vector */
		if (encparams.mpeg1)
			coding->PutBits(back_hor_f_code,3);
		else
			coding->PutBits(7,3); /* backward_f_code */
	}
	coding->PutBits(0,1); /* extra_bit_picture */
    coding->AlignBits();
	if ( !encparams.mpeg1 )
	{
		PutCodingExt();
	}

}

/* generate picture coding extension (6.2.3.1, 6.3.11)
 *
 * composite display information (v_axis etc.) not implemented
 */
void Picture::PutCodingExt()
{
	assert( coding->Aligned() );
	coding->PutBits(EXT_START_CODE,32); /* extension_start_code */
	coding->PutBits(CODING_ID,4); /* extension_start_code_identifier */
	coding->PutBits(forw_hor_f_code,4); /* forward_horizontal_f_code */
	coding->PutBits(forw_vert_f_code,4); /* forward_vertical_f_code */
	coding->PutBits(back_hor_f_code,4); /* backward_horizontal_f_code */
	coding->PutBits(back_vert_f_code,4); /* backward_vertical_f_code */
	coding->PutBits(dc_prec,2); /* intra_dc_precision */
	coding->PutBits(pict_struct,2); /* picture_structure */
	coding->PutBits((pict_struct==FRAME_PICTURE)?topfirst : 0, 1); /* top_field_first */
	coding->PutBits(frame_pred_dct,1); /* frame_pred_frame_dct */
	coding->PutBits(0,1); /* concealment_motion_vectors  -- currently not implemented */
	coding->PutBits(q_scale_type,1); /* q_scale_type */
	coding->PutBits(intravlc,1); /* intra_vlc_format */
	coding->PutBits(altscan,1); /* alternate_scan */
	coding->PutBits(repeatfirst,1); /* repeat_first_field */

	coding->PutBits(prog_frame,1); /* chroma_420_type */
	coding->PutBits(prog_frame,1); /* progressive_frame */
	coding->PutBits(0,1); /* composite_display_flag */
    coding->AlignBits();
}


void Picture::PutSliceHdr( int slice_mb_y, int mquant )
{
    /* slice header (6.2.4) */
    coding->AlignBits();
    
    if (encparams.mpeg1 || encparams.vertical_size<=2800)
        coding->PutBits(SLICE_MIN_START+slice_mb_y,32); /* slice_start_code */
    else
    {
        coding->PutBits(SLICE_MIN_START+(slice_mb_y&127),32); /* slice_start_code */
        coding->PutBits(slice_mb_y>>7,3); /* slice_vertical_position_extension */
    }
    
    /* quantiser_scale_code */
    coding->PutBits(q_scale_type 
            ? map_non_linear_mquant[mquant] 
            : mquant >> 1, 5);
    
    coding->PutBits(0,1); /* extra_bit_slice */
    
} 


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
