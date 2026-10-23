#ifndef _ONTHEFLYRATECTLPASS1_HH
#define _ONTHEFLYRATECTLPASS1_HH

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

class OnTheFlyPass1State :  public RateCtlState
{
public:
	virtual ~OnTheFlyPass1State() {}
	virtual RateCtlState *New() const { return new OnTheFlyPass1State; }
	virtual void Set( const RateCtlState &state ) { *this = static_cast<const OnTheFlyPass1State &>(state); }
	virtual const RateCtlState &Get() const { return *this; }
	int target_bits; // target_bits
	int vbuf_fullness;
    int ratectl_vbuf[NUM_PICT_TYPES];

	int per_pict_bits;
	int     fields_in_gop;
	double  field_rate;
	int     fields_per_pict;

	int buffer_variation;
	int64_t bits_transported;
	int64_t bits_used;
	int gop_buffer_correction;


	int frame_overshoot_margin;
	int undershoot_carry;

	/*!
	 * If our buffer is running down we need to reduce available bits
	 * for encoding to fill it up again.   This is the gain expressed
	 * as the fraction of the undershoot to recover over the next second.
	 */
	double overshoot_gain;

    /*
	  actsum - Total activity (sum block lum variances) in frame
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


    int N[NUM_PICT_TYPES];


	int min_d, max_d;
	int min_q, max_q;

	double bits_per_mb;
	bool fast_tune;
	bool first_gop;
	

    /* X's measure global complexity (Chi! not X!) of frame types.
	* Actually: X = average quantisation * bits allocated in *previous* frame
	* N.b. the choice of measure is *not* arbitrary.  The feedback bit
	* rate control gets horribly messed up if it is *not* proportionate
	* to bit demand i.e. bits used scaled for quantisation.  
	* d's are virtual reciever buffer fullness 
	* r is Rate control feedback gain (in* bits/frame) 
	*/
    
    double Xhi[NUM_PICT_TYPES];

	/* The average complexity of frames of the different types is used
     * to predict a reasonable bit-allocation for these types.
	 * The AVG_WINDOW set the size of the sliding window for these
     * averages.  Basically I Frames respond very quickly.
     * B / P frames more or less quickly depending on the target number
     * of B frames per P frame.
	 */
    double K_AVG_WINDOW[NUM_PICT_TYPES];

	/*
     * 'Typical' sizes of the different types of picture in a GOP - these
     * sizes are needed so that buffer management can compensate for the
     * 'normal' ebb and flow of buffer space in a GOP (low after a big I frame)
     * nearly full at the end after lots of smaller B/P frames.
     *
     */
    int32_t pict_base_bits[NUM_PICT_TYPES];
    bool first_encountered[NUM_PICT_TYPES];


    // Some statistics for measuring if things are going well.
    double sum_size[NUM_PICT_TYPES];
    int pict_count[NUM_PICT_TYPES];

};


class OnTheFlyPass1 :  public Pass1RateCtl,  public OnTheFlyPass1State
{
public:
	OnTheFlyPass1( EncoderParams &encoder );
    virtual void Init() ;

    virtual void GopSetup( int nb, int np );
    virtual void PictUpdate (Picture &picture, int &padding_needed );


	virtual int  MacroBlockQuant( const MacroBlock &mb);
	virtual int  InitialMacroBlockQuant();


    double SumAvgActivity()  { return sum_avg_act; }
protected:
    virtual int  TargetPictureEncodingSize();

    virtual void InitSeq( );
    virtual void InitGOP( ) ;
    virtual void InitPict( Picture &picture );

protected:
    /*!
     * Mean bitrate to aim for...
     */
    double  ctrl_bitrate;
private:



    double  cur_base_Q;       // Current base quantisation (before adjustments
                              // for relative macroblock activity
    int     cur_mquant;       // Current macroblock quantisation
    int     mquant_change_ctr;


    double  sum_base_Q;       // Accumulates base quantisations encoding
    int     sum_actual_Q;     // Accumulates actual quantisation

	// inverse feedback gain: its in weird units 
	// The quantisation is porportionate to the
	// buffer bit overshoot (virtual buffer fullness)
	// *divided* by fb_gain  A 
	int32_t fb_gain;		
	


	// VBV calculation data
	double picture_delay;
	double next_ip_delay; /* due to frame reordering delay */
	double decoding_time;

};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
