#ifndef _ONTHEFLYRATECTLPASS2_HH
#define _ONTHELFYRATECTLPASS2_HH

/*  (C) 2003 Andrew Stevens */

/*  This is free software; you can redistribute it
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

#include "ratectl.hh"

/*
        The parts of of the rate-controller's state neededfor save/restore if backing off
        a partial encoding
*/

class OnTheFlyPass2State :  public RateCtlState
{
public:
    virtual ~OnTheFlyPass2State() {}
    virtual RateCtlState *New() const { return new OnTheFlyPass2State; }
    virtual void Set( const RateCtlState &state ) { *this = static_cast<const OnTheFlyPass2State &>(state); }
    virtual const RateCtlState &Get() const { return *this; }


protected:
    /*!
     * Number of frames encoded so far in entire stream
     */
    unsigned int m_encoded_frames;

    /*!
     * Sum of differences target_size - actual_size for frames encoded so far
     * (used for 2-pass encoding rate control).
     */

    int64_t	m_control_undershoot;
    /*
     * Current feedback-control bitrate for whole-sequence rate control
     */
    unsigned int m_seq_ctrl_bitrate;

    /*
     * Weighting 0.0 .. 1.0 to give to whole-sequence rate control
     * (per-gop rate control gets 1.0-m_seq_ctrl_weight)
     */
    double m_seq_ctrl_weight;

    /*
     * Bit-rate for unit of picture complexity (Set to != 0.0 when two-pass encoding
     * rate control)
     */
    double m_picture_xhi_bitrate;

    /*
     * Mean stream picture complexity so far in pass 2
     */

    double m_mean_strm_Xhi;

    /*
     * Mean pass-1 picture complexity in pass 1
     */

    double m_mean_gop_Xhi;
    int32_t per_pict_bits;
    int     fields_in_gop;
    double  field_rate;
    int     fields_per_pict;

    /*!
     * Gain factor to feedback correction to bitrate used to control
     * bit allocation to recover decoder buffer fullness
     */
    double overshoot_gain;

    /*!
     * Estimated amount of  space in nominal decoder buffer at
     * current point in encoding.
     */
    int32_t buffer_variation;
    /*!
     * Bits assumed to be transported to decoder
     * at current point in current sequence
     */
    int64_t bits_transported;

    /*!
     * Total bits decoded at current point in entire stream
     */
    int64_t total_bits_used;

    /*!
     * Total bits decoded at current point in current sequence
     */
    int64_t seq_bits_used;

    int32_t gop_buffer_correction;

    int target_bits;    // target bits for current frame

    double base_quant;

    /*!
     * Sum of picture complexities for current gop
     */
    double gop_Xhi;

    /*!
     * Sum of pictures complexities up to current point
     * in entire stream.
     */
    double m_strm_Xhi;

    /*
      Moving average of the final ration actual_bits/target_bits after
      re-encoding of pictures.  Used to avoid a bias to under / over correction
     */
    double mean_reencode_A_T_ratio;

    /*
      actsum - Total activity (sum block variances) in frame
      actcovered - Activity macroblocks so far quantised (used to
      fine tune quantisation to avoid starving highly
      active blocks appearing late in frame...) UNUSED
      avg_act - Current average activity...
    */
    double actsum;
    double actcovered;
    double sum_avg_act;
    double avg_act;
    double sum_avg_quant;

};


class OnTheFlyPass2 :  public Pass2RateCtl,  public OnTheFlyPass2State
{
public:
    OnTheFlyPass2( EncoderParams &encoder );
    virtual void Init() ;

    virtual void GopSetup( std::deque<Picture *>::iterator gop_begin,
                           std::deque<Picture *>::iterator gop_end );
    virtual void PictUpdate (Picture &picture, int &padding_needed );

    virtual int  MacroBlockQuant( const MacroBlock &mb);
    virtual int  InitialMacroBlockQuant();

    double SumAvgActivity()  { return sum_avg_act; }

    bool ReencodeRequired() const { return reencode; }

    unsigned int getEncodedFrames() const { return m_encoded_frames; }

    double getStreamComplexity() const { return m_strm_Xhi; }

protected:
    virtual int  TargetPictureEncodingSize();

    virtual void InitSeq( );
    virtual void InitGOP( ) ;
    virtual void InitPict( Picture &picture );

    struct GopStats
    {
    	double Xhi;				// total complexity for GOP
    	unsigned int pictures;	// number of pictures
    };

    /*
     * Queue of GOP statistics updated as frames are queued (in encoding order)
     * for pass-2 encoding.  Stats are taken off queue
     *  when first picture of GOP is reached.
     */
    std::deque<GopStats>	m_gop_stats_Q;
private:

#if 0   // TODO: Do we need VBV checking? currently left to muxer
    virtual void CalcVbvDelay (Picture &picture);
    virtual void VbvEndOfPict (Picture &picture);
#endif

    double  base_Q;           // Base quantisation (before adjustments
                              // for relative macroblock activity
    double  cur_int_base_Q;   // Current rounded base quantisation
    double  rnd_error;        // Cumulative rounding error from base
                              // quantisation rounding

    int     cur_mquant;       // Current macroblock quantisation
    int     mquant_change_ctr;

                            // Window used for moving average of
                            // post-correction actual / target bits ratio
                            // for re-encoded frames.
    static const int RENC_A_T_RATIO_WINDOW = 4;
    bool   reencode;  // Current Picture flagged for re-encode to better hit
                      // target bitrate.
    bool   sample_T_A;      // Finaly (T)arget and (A)ctual bit ratio should be sampled
                            // to maintain an estimate of current systematic mean T/A ratio after
                            // pass 2 re-encoding.

    double sum_base_Q;        // Accumulates base quantisations encoding
    int sum_actual_Q;         // Accumulates actual quantisation
    double buffer_variation_danger; // Buffer variation level below full
                                 // at which serious risk of data under-run in muxed stream
};



/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
