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
#include "ontheflyratectlpass2.hh"
#include "cpu_accel.h"



/*****************************
 *
 * On the fly rate controller for encoding pass2.
 * A simple virtual buffer controller that exploits
 * limited look-ahead knowledge of the complexity of upcoming frames.
 *
 ****************************/

OnTheFlyPass2::OnTheFlyPass2(EncoderParams &encparams ) :
        Pass2RateCtl(encparams, *this)
{
	    buffer_variation = 0;
        bits_transported = 0;
        seq_bits_used = 0;
        total_bits_used = 0;
        sum_avg_act = 0.0;
        sum_avg_quant = 0.0;
        m_encoded_frames = 0;
        m_control_undershoot = 0;
        m_picture_xhi_bitrate = 0;
        m_strm_Xhi = 0.0;
        m_seq_ctrl_bitrate = encparams.bit_rate;
}


void OnTheFlyPass2::Init()
{

  /*
     Gain is set so that feedback is set to recover buffer variation in 0.5
     seconds for a typical DVD stream.  Gain is reduce in proportion to
     buffer size and increased in proportion to bit-rate.
   */

  double per_frame_bits = encparams.bit_rate / encparams.decode_frame_rate;
  int buffer_danger = 3 * per_frame_bits ;
  buffer_variation_danger = (encparams.video_buffer_size-buffer_danger);
  overshoot_gain =
	  (2.0 * (230.0*8.0/11000.0)) * encparams.bit_rate / encparams.video_buffer_size;
}
/*********************
 *
 * Initialise rate control parameters for start of new sequence
 *
 ********************/

void OnTheFlyPass2::InitSeq()
{
  /* If its stills with a size we have to hit then make the
     guesstimates of for initial quantisation pessimistic...
  */
  bits_transported = seq_bits_used = 0;
  field_rate = 2*encparams.decode_frame_rate;
  fields_per_pict = encparams.fieldpic ? 1 : 2;

                        
  if( encparams.still_size > 0 )
  {
    per_pict_bits = encparams.still_size * 8;
  }
  else
  {
    per_pict_bits =
      static_cast<int32_t>(encparams.fieldpic
                           ? encparams.bit_rate / field_rate
                           : encparams.bit_rate / encparams.decode_frame_rate
                          );
  }

  mean_reencode_A_T_ratio = 1.0;
}


void OnTheFlyPass2::GopSetup( std::deque<Picture *>::iterator gop_begin,
                              std::deque<Picture *>::iterator gop_end )
{

  /*
    At the start of a GOP before any frames have gone the
    actual buffer state represents a long term average. Any
    undershoot due to the I_frame of the previous GOP
    should by now have been caught up.
  */
  gop_buffer_correction = 0;
  mjpeg_debug( "PASS2 GOP Rate Lookead" );

  std::deque<Picture *>::iterator i;
  double sum_Xhi = 0.0;
  for( i = gop_begin; i != gop_end; ++i )
  {
    //mjpeg_info( "P2RC: %d xhi = %.0f", (*i)->decode, (*i)->ABQ * (*i)->EncodedSize() );
	double frame_Xhi = (*i)->ABQ * (*i)->EncodedSize();
    sum_Xhi += frame_Xhi;
  }

  GopStats gop_stats;
  gop_stats.pictures = static_cast<int>(gop_end - gop_begin);
  gop_stats.Xhi = sum_Xhi;

  m_gop_stats_Q.push_back( gop_stats );
}

/* ****************************
*
* Reinitialize rate control parameters for start of new GOP
*
* ****************************/

void OnTheFlyPass2::InitGOP(  )
{
  mjpeg_debug( "PASS2 GOP Rate Init" );

  GopStats gop_stats = m_gop_stats_Q.front();
  m_gop_stats_Q.pop_front();
  fields_in_gop = fields_per_pict * gop_stats.pictures;
  gop_Xhi = gop_stats.Xhi;

  //mjpeg_info( "P2RC: GOP actual size %.0f total xhi = %0.f",total_size, gop_Xhi);
  // Sanity check complexity based allocation to ensure it doesn't cause
  // buffer underflow


  unsigned int rep_sample_frames;
  if(  encparams.stream_frames == 0 )
  {
	  // No stream length given ... 5 minutes of video ought to be representative
	  rep_sample_frames = 25*5*60; // 5 min of video
  }
  else
  {
	  // 10% of stream or 10 GOPs...
	  rep_sample_frames = std::max( encparams.N_max * 10, encparams.stream_frames / 10 );
  }
  double undershoot = 0.0;
  if( encparams.target_bitrate > 0 )
  {

	  if( m_strm_Xhi < encparams.stream_Xhi && m_encoded_frames < encparams.stream_frames )
	  {
		  // Two pass encoding we - we know exact # bits under/overshoot from
		  // target.
		  undershoot = m_control_undershoot;
		  m_seq_ctrl_weight = 1.0;
		  double stream_bits =
			  encparams.stream_frames * encparams.target_bitrate /  encparams.frame_rate ;
		  m_picture_xhi_bitrate =
			  (field_rate/fields_per_pict) * stream_bits / encparams.stream_frames;
	  }
	  else
	  {
		  // Rate feedback... aim to recover under/overshoot over the
		  // period of a representative sample of frames
		  undershoot =
			  encparams.target_bitrate *  m_encoded_frames / encparams.decode_frame_rate -
			  total_bits_used;
		  m_seq_ctrl_weight =
			  std::min( 1.0,
						static_cast<double>(m_encoded_frames) / rep_sample_frames );
		  m_picture_xhi_bitrate = 0.0;
	  }
	  double rate_feedback =
		  undershoot * encparams.decode_frame_rate / rep_sample_frames;
	  //
	  // We never set control bitrate higher than peak bitrate as otherwise
	  // we risk under-run...
	  m_seq_ctrl_bitrate =
		  std::max( encparams.bit_rate, encparams.target_bitrate + rate_feedback );
  }
  m_mean_gop_Xhi =  gop_Xhi / gop_stats.pictures;


  // if we don't have 2-pass data we wrogressively shift from per-gop to complete
  // stream rate-control until rep_sample_frames seen...

  m_mean_strm_Xhi = m_encoded_frames > 0
					? m_strm_Xhi / m_encoded_frames
					: m_mean_gop_Xhi;
  mjpeg_info( "Mean strm Xhi = %.0f mean gop Xhi = %.0f pXhibr=%.4f cbr/abr=%d/%.0f under=%.0f",
			  m_mean_strm_Xhi, m_mean_gop_Xhi,
			  m_picture_xhi_bitrate,
			  m_seq_ctrl_bitrate,
			  encparams.target_bitrate *  m_encoded_frames / encparams.decode_frame_rate,
			  undershoot );
}


    /* ****************************
    *
    * Reinitialize rate control parameters for start of new Picture
    *
    * @return (re)encoding of picture necessary to achieve rate-control
    *
    *
    * TODO: Fix VBV violations here?
    *
    * ****************************/

void OnTheFlyPass2::InitPict(Picture &picture)
{

  actsum = picture.VarSumBestMotionComp();
  avg_act = actsum/(double)(encparams.mb_per_pict);
  sum_avg_act += avg_act;
  actcovered = 0.0;
  sum_base_Q = 0.0;
  sum_actual_Q = 0;
  mquant_change_ctr = encparams.mb_width/4;

  // Bitrate model:  bits_picture(i) =  K(i) / quantisation
  // Hence use Complexity metric = bits * quantisation

  // TODO Currently we're just using a dumb reactive feedback correction
  // to avoid buffer under-run... longer term we should look-ahead and
  // correct looking-ahead ...

  double buffer_state_feedback =  overshoot_gain * buffer_variation;
  double rel_overshoot = std::max( 0.0, -buffer_variation/buffer_variation_danger );
  int actual_bits = picture.EncodedSize();
  double Xhi = picture.ABQ * actual_bits;
  double ctrl_bitrate;
  if( encparams.still_size > 0 )
  {
	  target_bits = per_pict_bits;
	  ctrl_bitrate = encparams.bit_rate;
  }
  else if( encparams.target_bitrate > 0  )
  {
	  double seq_ctrl_bitrate;
	  if( m_picture_xhi_bitrate != 0.0 )
	  {
		  seq_ctrl_bitrate = Xhi*m_picture_xhi_bitrate;
	  }
	  else
	  {
		  seq_ctrl_bitrate =
		      Xhi * (m_seq_ctrl_bitrate+buffer_state_feedback) / m_mean_strm_Xhi;
	  }

	  // As the video buffer empties from 1/4 empty bring the control bitrate progressively
	  // dwn so that at 3/4's empty it never exceeds 3/4's peak rate. This ensures
	  // we don't continue to empty the video buffer due to super-active
	  // frames when its already near-empty...

	  double buffer_danger = std::min(1.0,std::max(0.0,(4.0/3.0)*(rel_overshoot-0.25)));
	  seq_ctrl_bitrate =  encparams.bit_rate * 3.0/4.0 * buffer_danger
						+ seq_ctrl_bitrate * (1.0-buffer_danger);

	  if( m_picture_xhi_bitrate != 0.0 )
	  {

		  ctrl_bitrate = seq_ctrl_bitrate+buffer_state_feedback;
	  }
	  else
	  {

		  // Weighted combination of whole-stream rate-control
		  // and simple rate-control maintaining target gop by gop
		  double gop_ctrl_bitrate =
			  Xhi * (encparams.target_bitrate+buffer_state_feedback) / m_mean_gop_Xhi;
		  ctrl_bitrate = m_seq_ctrl_weight * seq_ctrl_bitrate  +
			             (1.0-m_seq_ctrl_weight) * gop_ctrl_bitrate;
	  }




	  // Heuristic
	  // We don't set control bit-rate ludicrously low to avoid
	  // quantisation artefacts even on super-low activity frames.
	  double ctrl_bitrate_floor =
		  std::min( encparams.target_bitrate/3.0, encparams.bit_rate/5.0 );
	  ctrl_bitrate = std::max( ctrl_bitrate, ctrl_bitrate_floor);

	  // N.b. no multiplication by fields_per_pict as Xhi is actually
	  // sum of field complexities == fields_in_pict * mean_field_Xhi...
	  target_bits = fields_per_pict * ctrl_bitrate  / field_rate;
  }
  else
  {
	  ctrl_bitrate = (encparams.bit_rate+buffer_state_feedback);
	  // Heuristic
	  // We don't set control bit-rate more below half target bit rate or
	  // more below target bit-rate than peak rate target rate
	  double ctrl_bitrate_floor = encparams.bit_rate/5.0;
	  ctrl_bitrate = std::max( ctrl_bitrate, ctrl_bitrate_floor);

	  double available_bits = ctrl_bitrate * fields_in_gop/field_rate;
	  target_bits = static_cast<int32_t>(available_bits*Xhi/gop_Xhi);

  }
  target_bits = min( target_bits, encparams.video_buffer_size*3/4 );

  picture.avg_act = avg_act;
  picture.sum_avg_act = sum_avg_act;

  double rel_error = (actual_bits-target_bits) / static_cast<double>(target_bits);
  double scale_quant_floor = std::max(1.0, encparams.quant_floor);
  //
  // Tolerance of overshooting target bit allocation
  // drops to zero from encparams.coding_tolerance
  // as buffer_variation gets closer to danger level...

  double overshoot_tolerance =  encparams.coding_tolerance * (1.0-rel_overshoot);
  double undershoot_tolerance = -encparams.coding_tolerance;

  // Re-encode if we overshot too much or undershot and weren't yet at the specified
  // quantization floor.
  reencode =
        rel_error  > overshoot_tolerance
    || (rel_error < undershoot_tolerance && picture.ABQ > scale_quant_floor  );

  //fprintf( stderr, "RE = %.2f OT=%.2f UT=%.02f RENC=%d\n", rel_error, overshoot_tolerance, undershoot_tolerance, reencode );
  // If re-encoding to hit a target we adjust *relative* to previous (off-target) base quantisation
  // Since there is often a tendency for systematically under or over correct we maintain a moving
  // average of the ratio target_bits/actual_bits after re-encoding ansd use this to correct our
  // correction 
  
  double target_ABQ = picture.ABQ * actual_bits / target_bits;
  // If the correction of the correction looks reasonable... use it...
  double debiased_target_ABQ = target_ABQ * mean_reencode_A_T_ratio;
  if( actual_bits > target_bits &&  debiased_target_ABQ > picture.ABQ ||
      actual_bits < target_bits && debiased_target_ABQ < picture.ABQ )
  {
     target_ABQ = debiased_target_ABQ;
  }

  double raw_base_Q;
  if( scale_quant_floor < target_ABQ )
  {
      sample_T_A = reencode;
      raw_base_Q = target_ABQ;
  }
  else
  {
    // If we've hit the quantisation floor we *expect* 
    // (A)ctual bits < (T)arget bits so its not
    // useful to use the T/A ratio to update our estimate
    // of bias in our T/A ratio when we're trying to actually
    // hit the target bits.
    raw_base_Q = scale_quant_floor;
    sample_T_A = false;
  }
  base_Q = ClipQuant( picture.q_scale_type,
                      fmax( encparams.quant_floor, raw_base_Q ) );

  cur_int_base_Q = floor( base_Q + 0.5 );
  rnd_error = 0.0;
  cur_mquant = ScaleQuant( picture.q_scale_type, cur_int_base_Q );


  mjpeg_info( "%s: %d - reencode actual %d (%.1f) target %d Q=%.1f BV  = %.2f cbr=%.0f",
                reencode ? "RENC" : "SKIP",
                picture.decode, actual_bits, picture.ABQ, target_bits, base_Q,
                buffer_variation/((double)encparams.video_buffer_size), ctrl_bitrate );
}



/*
 * Update rate-controls statistics after pictures has ended..
 *
 * RETURN: The amount of padding necessary for picture to meet syntax or
 * rate constraints...
 */

void OnTheFlyPass2::PictUpdate( Picture &picture, int &padding_needed)
{
  ++m_encoded_frames;
  int actual_bits = picture.EncodedSize();
  m_control_undershoot += target_bits-actual_bits;

  if( sample_T_A )
  {
      double A_T_ratio = actual_bits / static_cast<double>(target_bits);
      mean_reencode_A_T_ratio = 
        ( RENC_A_T_RATIO_WINDOW * mean_reencode_A_T_ratio + A_T_ratio ) / (RENC_A_T_RATIO_WINDOW+1);
  }

  /*
    Compute the estimate of the current decoder buffer state.  We
    use this to feedback-correct the available bit-pool with a
    fraction of the current buffer state estimate.
    
    Note that since we cannot hold more than a buffer-full!
    Excess would be padded away / transmission would be
    halted when the buffer was full.

    TODO for CBR we should assume padding -> update seq_bits_bits_used
    and total_bits_used to bring buffer_varation = 0.
    TODO Really we need to use a VBV model to get this precise....
  */

  seq_bits_used += actual_bits;
  total_bits_used += actual_bits;
  bits_transported += per_pict_bits;
  buffer_variation  = static_cast<int32_t>(bits_transported - seq_bits_used);


  if( buffer_variation > 0 )
  {
    bits_transported = seq_bits_used;
    buffer_variation = 0;
  }

  /* Rate-control
    ABQ is the average 'base' quantisation (before adjustments for relative
    macro-block complexity) of the block.  This is what is used as a base-line
    for adjusting quantisation to meet a bit allocation target.
    
  */
  if( sum_base_Q != 0.0 ) // 0.0 if old encoding retained...
  {
    picture.ABQ = sum_base_Q / encparams.mb_per_pict;
    picture.AQ = static_cast<double>(sum_actual_Q ) / encparams.mb_per_pict;
  }

  double Xhi = picture.ABQ * actual_bits;
  m_strm_Xhi += Xhi;

  /* Stats and logging.
     AQ is the average Quantisation of the block.
    Its only used for stats display as the integerisation
    of the quantisation value makes it rather coarse for use in
    estimating bit-demand
  */
  
  sum_avg_quant += picture.AQ;
  picture.SQ = sum_avg_quant;
  mjpeg_debug( "Frame %c A=%6.0f %.2f", 
               pict_type_char[picture.pict_type],
               actual_bits/8.0,
               actual_bits/picture.AQ
             );

  // TODO TODO TODO We generate padding only for still frames....
  // TODO TODO TODO the relevant code should be moved from pass-1 which should NOT
  // pad (to allow a decent estimate of required quantisation)
  padding_needed = 0;
}

int OnTheFlyPass2::InitialMacroBlockQuant()
{
  return cur_mquant;
}

int OnTheFlyPass2::TargetPictureEncodingSize()
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

int OnTheFlyPass2::MacroBlockQuant( const MacroBlock &mb )
{
  int lum_variance = mb.BaseLumVariance();

  const Picture &picture = mb.ParentPicture();


  /* We 'dither' the rounded base quantisation so that average base quantisation
    is close to the target value.  This should help achieve smaller adjustments in
    coding size reasonably accurately.
  */

  --mquant_change_ctr;
  if( mquant_change_ctr == 0 )
  {
      mquant_change_ctr = encparams.mb_width/4; 
      rnd_error += (cur_int_base_Q - base_Q);
      if( rnd_error > 0.5 )
        cur_int_base_Q -= 1;
      else if( rnd_error <= -0.5 )
        cur_int_base_Q += 1;
  }

  double act_boost;
  if( lum_variance < encparams.boost_var_ceil )
  {
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
  sum_base_Q += cur_int_base_Q;
  cur_mquant = ScaleQuant(picture.q_scale_type,cur_int_base_Q/act_boost) ;
  sum_actual_Q += cur_mquant;


  return cur_mquant;
}

#if 0
/* VBV calculations
 *
 * generates warnings if underflow or overflow occurs
 */

/* vbv_end_of_picture
 *
 * - has to be called directly after writing picture_data()
 * - needed for accurate VBV buffer overflow calculation
 * - assumes there is no byte stuffing prior to the next start code
 *
 * Note correction for bytes that will be stuffed away in the eventual CBR
 * bit-stream.
 */

void OnTheFlyPass2::VbvEndOfPict(Picture &picture)
{
}

/* calc_vbv_delay
 *
 * has to be called directly after writing the picture start code, the
 * reference point for vbv_delay
 *
 * A.Stevens 2000: 
 * Actually we call it just before the start code is written, but anyone
 * who thinks 32 bits +/- in all these other approximations matters is fooling
 * themselves.
 */

void OnTheFlyPass2::CalcVbvDelay(Picture &picture)
{

        /* VBV checks would go here...*/


        if( !encparams.mpeg1 || encparams.quant_floor != 0 || encparams.still_size > 0)
                picture.vbv_delay =  0xffff;
        else if( encparams.still_size > 0 )
                picture.vbv_delay =  static_cast<int>(90000.0/encparams.frame_rate/4);
}

#endif

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
