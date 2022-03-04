/* ratectl.c, bitrate control routines (linear quantization only currently) */

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

/* Modifications and enhancements (C) 2000,2001,2002,2003 Andrew Stevens */

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
#include <math.h>
#include <limits.h>
#include "mjpeg_types.h"
#include "mjpeg_logging.h"
#include "mpeg2syntaxcodes.h"
#include "tables.h"
#include "mpeg2encoder.hh"
#include "picture.hh"
#include "ontheflyratectlpass1.hh"
#include "cpu_accel.h"



/*****************************
 *
 * On-the-fly rate controller for coding pass1.  The constructor sets up the initial
 * control and estimator parameter values to values that experience
 * suggest make sense.  All the important ones are dynamically 
 * tuned anyway so these values are not too critical.
 *
 ****************************/

OnTheFlyPass1::OnTheFlyPass1(EncoderParams &encparams ) :
	Pass1RateCtl(encparams, *this)
{
	buffer_variation = 0;
	bits_transported = 0;
	bits_used = 0;
	frame_overshoot_margin = 0;
	sum_avg_act = 0.0;
	sum_avg_quant = 0.0;

}


/*********************
 *
 * Initialise rate control parameters for start of encoding
 * based on encoding parameters
 *
 ********************/

void OnTheFlyPass1::Init()
{
    /*
      Reaction paramer - i.e. quantisation feedback gain relative
      to bit over/undershoot.
      For normal frames it is fairly modest as we can compensate
      over multiple frames and can average out variations in image
      complexity.

      For stills we set it a higher so corrections take place
      more rapidly *within* a single frame.
    */

	if( encparams.target_bitrate > 0 )
		ctrl_bitrate = encparams.target_bitrate;
	else
		ctrl_bitrate = encparams.bit_rate;
    if( encparams.still_size > 0 )
        fb_gain = (int)floor(2.0*ctrl_bitrate/encparams.decode_frame_rate);
    else
        fb_gain = (int)floor(4.0*ctrl_bitrate/encparams.decode_frame_rate);

    /* Set the virtual buffers for per-frame rate control feedback to
       values corresponding to the quantisation floor (if specified)
       or a "reasonable" quantisation (6.0) if not.
    */

    double init_quant = (encparams.quant_floor > 0.0 ? encparams.quant_floor : 6.0);
    int i;
    for( i = FIRST_PICT_TYPE; i <= LAST_PICT_TYPE; ++i )
    {
        ratectl_vbuf[i] = static_cast<int>(init_quant * fb_gain / 62.0);
        Xhi[i] = 1.0; // Not used in first frame, set so init
                      // for valgrind, debug messages etc.
        sum_size[i] = 0.0;
        pict_count[i] = 0;
    }

    first_gop = true;
    K_AVG_WINDOW[I_TYPE] = 2.0;
    switch( encparams.M )
    {
    case 1 : // P
        K_AVG_WINDOW[P_TYPE] = 8.0;
        K_AVG_WINDOW[B_TYPE] = 1.0; // dummy
        break;
    case 2 : // BP
        K_AVG_WINDOW[P_TYPE] = 4.0;
        K_AVG_WINDOW[B_TYPE] = 4.0;
        break;
    default: // BBP
        K_AVG_WINDOW[P_TYPE] = 3.0;
        K_AVG_WINDOW[B_TYPE] = 7.0;
        break;
    }        



  /*
     We assume that having less than 4 frame intervals
     worth buffered is cutting it fine for avoiding under-runs.
     This is the buffer_safe margin.

     The gain values represent the fraction of the under/over shoot
     to be recovered during one second.  Gain is lower if the
     buffer space above the buffer safe margin is large.

    Gain is currently set to so that if the buffer is down to the buffer_safe
    margin the deficit should be recovered in one second.

  */

    if( encparams.still_size > 0 )
    {
        per_pict_bits = encparams.still_size * 8;
        undershoot_carry = 0;
        overshoot_gain = 1.0;
    }
    else
    {
        per_pict_bits =
            static_cast<int32_t>(encparams.fieldpic
                                 ? ctrl_bitrate / field_rate
                                 : ctrl_bitrate / encparams.decode_frame_rate
                );
       
        int buffer_danger = 4 * per_pict_bits ;
        int safe_buffer_variation =  (encparams.video_buffer_size - buffer_danger);
        if( safe_buffer_variation < 0 )
            mjpeg_error_exit1("Rate control can't cope with a video buffer smaller 4 frame intervals");

        undershoot_carry = safe_buffer_variation/6;

        /*
           Gain is set so that feedback is set to recover buffer variation in 4
           seconds for a typical DVD stream.  Gain is reduced in proportion to
           buffer size and increased in proportion to bit-rate.

           Note this a  very low gain and may well produce buffer variations above
           the actual buffer size.  However, we don't care as the second pass will clean
           things up taking advantage of look-ahead.
         */

        overshoot_gain =
      	  (0.25 * (230.0*8.0/11000.0)) * encparams.bit_rate / encparams.video_buffer_size;

        //fprintf( stderr, "VBS=%d BD=%d Overshoot gain = %.2f", encparams.video_buffer_size, buffer_danger, overshoot_gain );
    }


    next_ip_delay = 0.0;
    decoding_time = 0.0;
}

/*********************
 *
 * Setup GOP structure for coding.
 *
 ********************/


void OnTheFlyPass1::GopSetup( int np, int nb )
{
    N[P_TYPE] = encparams.fieldpic ? 2*np+1 : 2*np;
    N[B_TYPE] = encparams.fieldpic ? 2*nb : 2*nb;
    N[I_TYPE] = encparams.fieldpic ? 1 : 2;
    fields_in_gop = N[I_TYPE] + N[P_TYPE] + N[B_TYPE];
}


/*********************
 *
 * Update rate control parameters for start of new sequence
 *
 ********************/

void OnTheFlyPass1::InitSeq()
{
    /* If its stills with a size we have to hit then make the
       guesstimates of for initial quantisation pessimistic...
    */
    bits_transported = bits_used = 0;
    field_rate = 2*encparams.decode_frame_rate;
    fields_per_pict = encparams.fieldpic ? 1 : 2;
}


/*********************
 *
 * Update rate control parameters for start of new GOP
 *
 ********************/

void OnTheFlyPass1::InitGOP(  )
{
	/*
	  At the start of a GOP before any frames have gone the
	  actual buffer state represents a long term average. Any
	  undershoot due to the I_frame of the previous GOP
	  should by now have been caught up.
	*/

	gop_buffer_correction = 0;

	/* Each still is encoded independently so we reset rate control
	   for each one.  They're all I-frames so each stills is a GOP too.
	*/

    int i;
	if( first_gop || encparams.still_size > 0)
	{
		mjpeg_debug( "FIRST GOP INIT");
		fast_tune = true;
		first_gop = false;
        for( i = FIRST_PICT_TYPE; i <= LAST_PICT_TYPE; ++i )
        {
            first_encountered[i] = true;
            pict_base_bits[i] = per_pict_bits;
        }
	}
	else
	{
		mjpeg_debug( "REST GOP INIT" );
		int available_bits = 
			static_cast<int>( (ctrl_bitrate+buffer_variation*overshoot_gain)
							  * fields_in_gop/field_rate);
        double Xsum = 0.0;
        for( i = FIRST_PICT_TYPE; i <= LAST_PICT_TYPE; ++i )
            Xsum += N[i]*Xhi[i];
        for( i = FIRST_PICT_TYPE; i <= LAST_PICT_TYPE; ++i )
            pict_base_bits[i] =  
                static_cast<int32_t>(fields_per_pict*available_bits*Xhi[i]/Xsum);
		fast_tune = false;

	}

}

  /* ****************************
  *
  * Update rate control parameters for start of new Picture
  *
  * ****************************/

void OnTheFlyPass1::InitPict(Picture &picture)
{
	int available_bits;
	double Xsum;

	/* TODO: A.Stevens  Nov 2000 - This modification needs testing visually.

	   Weird.  The original code used the average activity of the
	   *previous* frame as the basis for quantisation calculations for
	   rather than the activity in the *current* frame.  That *has* to
	   be a bad idea..., surely, here we try to be smarter by using the
	   current values and keeping track of how much of the frames
	   activitity has been covered as we go along.

	   We also guesstimate the relationship between  (sum
	   of DCT coefficients) and actual quantisation weighted activty.
	   We use this to try to predict the activity of each frame.
	*/

    actsum = picture.VarSumBestMotionComp();
	avg_act = actsum/(double)(encparams.mb_per_pict);
	sum_avg_act += avg_act;
	actcovered = 0.0;
	sum_base_Q = 0.0;
    sum_actual_Q = 0;

	/* Allocate target bits for frame based on frames numbers in GOP
	   weighted by:
	   - global complexity averages
	   - predicted activity measures
	   
	   T = available_bits * (Nx * Xx) / Sigma_j (Nj * Xj)

	   N.b. B frames are an exception as there is *no* predictive
	   element in their bit-allocations.  The reason this is done is
	   that highly active B frames are inevitably the result of
	   transients and/or scene changes.  Psycho-visual considerations
	   suggest there's no point rendering sudden transients
	   terribly well as they're not percieved accurately anyway.  

	   In the case of scene changes similar considerations apply.  In
	   this case also we want to save bits for the next I or P frame
	   where they will help improve other frames too.  

	   Note that we have to calulate per-frame bits by scaling the one-second
	   bit-pool to a one-GOP bit-pool.
	*/

	if( encparams.still_size > 0 )
		available_bits = per_pict_bits;
	else
	{
		int feedback_correction =
			static_cast<int>( fast_tune 
							  ?	buffer_variation * overshoot_gain
							  : (buffer_variation+gop_buffer_correction) 
							    * overshoot_gain
				);

		// Sanity limit - don't want negative available bits or
		// ludicrously small available bits.  "Bad guesses" will be fixed by
		// 2nd pass anyway...
		available_bits = std::max( ctrl_bitrate * 0.2,
					               ctrl_bitrate+feedback_correction
					             ) * fields_in_gop/field_rate;
	}



    Xsum = 0.0;
    int i;
    for( i = FIRST_PICT_TYPE; i <= LAST_PICT_TYPE; ++i )
        Xsum += N[i]*Xhi[i];
    vbuf_fullness = ratectl_vbuf[picture.pict_type];
    double first_weight[NUM_PICT_TYPES];
    first_weight[I_TYPE] = 1.0;
    first_weight[P_TYPE] = 1.7;
    first_weight[B_TYPE] = 1.7*2.0;

    double gop_frac = 0.0;
    if( first_encountered[picture.pict_type] )
    {
        for( i = FIRST_PICT_TYPE; i <= LAST_PICT_TYPE; ++i )
            gop_frac += N[i]/first_weight[i];
        target_bits = 
            static_cast<int32_t>(fields_per_pict*available_bits /
                                 (gop_frac * first_weight[picture.pict_type]));
    }
    else
        target_bits =
            static_cast<int32_t>(fields_per_pict*available_bits*Xhi[picture.pict_type]/Xsum);

	/* 
	   If we're fed a sequences of identical or near-identical images
	   we can get actually get allocations for frames that exceed
	   the video buffer size!  This of course won't work so we arbitrarily
	   limit any individual frame to 3/4's of the buffer.
	*/

	target_bits = min( target_bits, encparams.video_buffer_size*3/4 );

 	mjpeg_debug( "Frame %c T=%05d A=%06d  Xi=%.2f Xp=%.2f Xb=%.2f", 
                 pict_type_char[picture.pict_type],
                 (int)target_bits/8, (int)available_bits/8, 
                 Xhi[I_TYPE], Xhi[P_TYPE],Xhi[B_TYPE] );


	/* 
	   To account for the wildly different sizes of frames
	   we compute a correction to the current instantaneous
	   buffer state that accounts for the fact that all other
	   thing being equal buffer will go down a lot after the I-frame
	   decode but fill up again through the B and P frames.

	   For this we use the base bit allocations of the picture's
	   "pict_base_bits" which will pretty accurately add up to a
	   GOP-length's of bits not the more dynamic predictive T target
	   bit-allocation (which *won't* add up very well).
	*/

	gop_buffer_correction += (pict_base_bits[picture.pict_type]-per_pict_bits);


	/* Undershot bits have been "returned"  */
    vbuf_fullness = max( vbuf_fullness, 0 );

    //fprintf( stderr, "T=%05d A=%06d GBC = %d VBF=%d", (int)target_bits/8, (int)available_bits/8, (int)gop_buffer_correction/8,
    //										 (int)vbuf_fullness/8);

	/* We don't let the target volume get absurdly low as it makes some
	   of the prediction maths ill-condtioned.  At these levels quantisation
	   is always minimum anyway
	*/
	target_bits = max( target_bits, 4000 );

	if( encparams.still_size > 0 && encparams.vbv_buffer_still_size )
	{
		/* If stills size must match then target low to ensure no
		   overshoot.
		*/
		mjpeg_debug( "Setting VCD HR still overshoot margin to %d bytes", target_bits/(16*8) );
		frame_overshoot_margin = target_bits/16;
		target_bits -= frame_overshoot_margin;
	}


	picture.avg_act = avg_act;
	picture.sum_avg_act = sum_avg_act;
    cur_base_Q = fmax( vbuf_fullness*62.0/fb_gain, encparams.quant_floor);
	cur_mquant = ScaleQuant( picture.q_scale_type,  cur_base_Q );

    mquant_change_ctr = encparams.mb_width/2-1;
    //fprintf( stderr, " CBQ = %.1f CMQ=%03d\n", cur_base_Q, cur_mquant);
}




/*
 * Update rate-controls statistics after pictures has ended..
 *
 * RETURN: The amount of padding necessary for picture to meet syntax or
 * rate constraints...
 */

void OnTheFlyPass1::PictUpdate( Picture &picture, int &padding_needed)
{
	int32_t actual_bits;		/* Actual (inc. padding) picture bit counts */
	int frame_overshoot;
	actual_bits = picture.EncodedSize();
	frame_overshoot = (int)actual_bits-(int)target_bits;
	/* For the virtual buffers for quantisation feedback it is the
	   actual under/overshoot *including* padding.  Otherwise the
	   buffers go zero.
	*/
	vbuf_fullness += frame_overshoot;

	/* Warn if it looks like we've busted the safety margins in stills
	   size specification.  Adjust padding to account for safety
	   margin if we're padding to suit stills whose size has to be
	   specified in advance in vbv_buffer_size.
	*/
	picture.pad = 0;
   	int padding_bits = 0;
	if( encparams.still_size > 0 && encparams.vbv_buffer_still_size)
	{
		if( frame_overshoot > frame_overshoot_margin )
		{
			mjpeg_warn( "Rate overshoot: VCD hi-res still %d bytes too large! ", 
						((int)actual_bits)/8-encparams.still_size);
		}
		
		//
		// Aim for an actual size squarely in the middle of the 2048
		// byte granuality of the still_size coding.  This gives a 
		// safety margin for headers etc.
		//
		frame_overshoot = frame_overshoot - frame_overshoot_margin;
		if( frame_overshoot < -2048*8 )
			frame_overshoot += 1024*8;
        
		// Make sure we pad nicely to byte alignment
		if( frame_overshoot < 0 )
		{
		padding_bits = (((actual_bits-frame_overshoot)>>3)<<3)-actual_bits;
		picture.pad = 1;
		}
	}

    /* Adjust the various bit counting  parameters for the padding bytes that
     * will be added */
    actual_bits += padding_bits ;
    frame_overshoot += padding_bits;

	/*
	  Compute the estimate of the current decoder buffer state.  We
	  use this to feedback-correct the available bit-pool with a
	  fraction of the current buffer state estimate.  If we're ahead
	  of the game we allow a small increase in the pool.  If we
	  dropping towards a dangerously low buffer we decrease the pool
	  (rather more vigorously).
	  
	  Note that since we cannot hold more than a buffer-full if we have
	  a positive buffer_variation in VBR we assume we only sent until the buffer was full
	  in CBR we assume it was padded away.
	*/

	
	bits_used += actual_bits;
 
	bits_transported += per_pict_bits;
	//mjpeg_debug( "TR=%" PRId64 " USD=%" PRId64 "", bits_transported/8, bits_used/8);
	buffer_variation  = static_cast<int32_t>(bits_transported - bits_used);

	if( buffer_variation > 0 )
	{
			bits_transported = bits_used;
			buffer_variation = 0;	
	}

	//fprintf( stderr, "BV=%d A=%d T=%d\n", (int)buffer_variation/8, actual_bits/8, target_bits/8 );

  /* Rate-control
    ABQ is the average 'base' quantisation (before adjustments for relative
    macro-block complexity) of the block.  This is what is used as a base-line
    for adjusting quantisation to meet a 
  */

    picture.ABQ = sum_base_Q / encparams.mb_per_pict;
    picture.AQ = static_cast<double>(sum_actual_Q ) / encparams.mb_per_pict;


	

	sum_avg_quant += picture.AQ;

	
	/* X (Chi - Complexity!) is an estimate of "bit-demand" for the
	frame.  I.e. how many bits it would need to be encoded without
	quantisation.  It is used in adaptively allocating bits to busy
	frames. It is simply calculated as bits actually used times
	average target (not rounded!) quantisation.

	K is a running estimate of how bit-demand relates to frame
	activity - bits demand per activity it is used to allow
	prediction of quantisation needed to hit a bit-allocation.
	*/

	double actual_Xhi = actual_bits * picture.AQ;
    picture.SetComplexity( actual_Xhi );

    /* To handle longer sequences with little picture content
       where I, B and P frames are of unusually similar size we
       insist I frames assumed to be at least one and a half times
       as complex as typical P frames
    */
    if( picture.pict_type == I_TYPE )
        actual_Xhi = fmax(actual_Xhi, 1.5*Xhi[P_TYPE]);


    /* Stats and logging
       AQ is the average Quantisation of the block.
       Its only used for stats display as the integerisation
       of the quantisation value makes it rather coarse for use in
       estimating bit-demand */
    picture.SQ = sum_avg_quant;

	/* Xhi are used as a guesstimate of *typical* frame activities
	   based on the past.  Thus we don't want anomalous outliers due
	   to scene changes swinging things too much (this is handled by
	   the predictive complexity measure stuff) so we use moving
	   averages.  The weightings are intended so all 3 averages have
	   similar real-time decay periods based on an assumption of
	   20-30Hz frame rates.
	*/
    
    ratectl_vbuf[picture.pict_type] = vbuf_fullness;
    sum_size[picture.pict_type] += actual_bits/8.0;
    pict_count[picture.pict_type] += 1;

    if( first_encountered[picture.pict_type] )
    {
        Xhi[picture.pict_type] = actual_Xhi;
        first_encountered[picture.pict_type] = false;
    }
    else
    {
        double win = fast_tune 
            ? K_AVG_WINDOW[picture.pict_type] / 1.7
            : K_AVG_WINDOW[picture.pict_type];
        Xhi[picture.pict_type] = 
            (actual_Xhi + win*Xhi[picture.pict_type])/(win+1.0);
    }

    mjpeg_debug( "Frame %c A=%6.0f %.2f: I = %6.0f P = %5.0f B = %5.0f",
                 pict_type_char[picture.pict_type],
                 actual_bits/8.0,
                 actual_Xhi,
                 sum_size[I_TYPE]/pict_count[I_TYPE],
                 sum_size[P_TYPE]/pict_count[P_TYPE],
                 sum_size[B_TYPE]/pict_count[B_TYPE]
        );

    padding_needed = padding_bits/8;
}


int OnTheFlyPass1::InitialMacroBlockQuant()
{
    return cur_mquant;
}

int OnTheFlyPass1::TargetPictureEncodingSize()
{
    return target_bits;
}

/*************
 *
 * SelectQuantization - select a quantisation for the current
 * macroblock based on the fullness of the virtual decoder buffer.
 *
 * NOTE: *Must* be called for all Macroblocks as content-based quantisation tuning is
 * supported.
 ************/

int OnTheFlyPass1::MacroBlockQuant( const MacroBlock &mb )
{
    int lum_variance = mb.BaseLumVariance();
    const Picture &picture = mb.ParentPicture();

    if( mquant_change_ctr == 0 || lum_variance < encparams.boost_var_ceil )
    {

        /* A.Stevens 2000 : we measure how much *information* (total activity)
           has been covered and aim to release bits in proportion.

           We keep track of a virtual buffer that catches the difference
           between the bits allocated and the bits we actually used.  The
           fullness of this buffer controls quantisation.

        */

        /* Guesstimate a virtual buffer fullness based on
           bits used vs. bits in proportion to activity encoded
        */


        double dj = static_cast<double>(vbuf_fullness) 
            + static_cast<double>(picture.EncodedSize())
            - actcovered * target_bits / actsum;


        /* scale against dynamic range of mquant and the bits/picture
           count.  encparams.quant_floor != 0.0 is the VBR case where we set a
           bitrate as a (high) maximum and then put a floor on
           quantisation to achieve a reasonable overall size.  Not that
           this *is* baseline quantisation.  Not adjust for local
           activity.  Otherwise we end up blurring active
           macroblocks. Silly in a VBR context.
        */


        /*  Heuristic: We decrease quantisation for macroblocks
            with markedly low luminace variance.  This helps make
            gentle gradients (e.g. smooth backgrounds) look better at
            (hopefully) small additonal cost  in coding bits
        */

        double act_boost;
        if( lum_variance < encparams.boost_var_ceil )
        {
            // Force update for *next* macroblock as this may need a 'normal'
            // quantisation without any boosying
            mquant_change_ctr = 0;
            // Calculate quantisation reduction....
            if( lum_variance < encparams.boost_var_ceil/2)
                act_boost = encparams.act_boost;
            else
            {
                double max_boost_var = encparams.boost_var_ceil/2;
                double above_max_boost = 
                    (static_cast<double>(lum_variance)-max_boost_var)
                    / max_boost_var;
                act_boost = 1.0 + (encparams.act_boost-1.0) * (1.0-above_max_boost);
            }
        }
        else
            act_boost = 1.0;

        cur_base_Q =  ClipQuant( picture.q_scale_type,
                                 fmax(dj*62.0/fb_gain,encparams.quant_floor) );
        cur_mquant = ScaleQuant(picture.q_scale_type,cur_base_Q/act_boost) ;
    }
    --mquant_change_ctr;
    if( mquant_change_ctr < 0)
    {

        mquant_change_ctr =  encparams.mb_width/2-1;
    }

    sum_base_Q += cur_base_Q;
    sum_actual_Q += cur_mquant;
	/* Update activity covered */
	actcovered += lum_variance;

	return cur_mquant;
}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
