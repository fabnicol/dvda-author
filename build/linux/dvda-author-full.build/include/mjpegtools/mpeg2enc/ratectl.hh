#ifndef _RATECTL_HH
#define _RATECTL_HH

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

#include <deque>
#include "mjpeg_types.h"
#include "mpeg2syntaxcodes.h"

class MacroBlock;
class EncoderParams;
class Picture;



/*
	Base class for state of rate-controller.
	Factory allows us to generate appropriate state
	objects for a given rate-controller object.

	Set allow us to set set of a rate-controller from
	a previously copied state...
*/
class RateCtlState
{
public:
	virtual ~RateCtlState() {}
	virtual RateCtlState *New() const = 0;
	virtual void Set( const RateCtlState &state) = 0;
	virtual const RateCtlState &Get() const = 0;
};

class RateCtl 
{
public:
    RateCtl( EncoderParams &_encparams, RateCtlState &_state );

    virtual ~RateCtl() {}

    /*********************
    *
    * Initialise rate control parameters for start of encoding
    * based on encoding parameters
    *
    ********************/

    virtual void Init() = 0;

    /*********************
    *
    * Setup rate control for a new picture
    * IF it is first of sequence new seq. is initialized first
    * IF it is first of GOP new GOP is initialized 2nd
    * Finally Picture specific initialization is done.
    *
    ********************/

    virtual void PictSetup (Picture &picture);

    /*********************
    *
    * Update rate control after coding of new picture (minus padding)
    *
    ********************/
    virtual void PictUpdate (Picture &picture, int &padding_needed ) = 0;


    virtual int MacroBlockQuant(  const MacroBlock &mb) = 0;
    virtual int  InitialMacroBlockQuant() = 0;

    inline RateCtlState *NewState() const { return state.New(); }
    inline void SetState( const RateCtlState &toset) { state.Set( toset ); }
    inline const RateCtlState &GetState() const { return state.Get(); }

    // TODO DEBUG
    virtual double SumAvgActivity() = 0;

    static double ClipQuant( int q_scale_type, double quant );
    static double InvScaleQuant(  int q_scale_type, int raw_code );
    static int ScaleQuant( int q_scale_type, double quant );
protected:

    /*********************
    *
    * Reinitialize rate control parameters for start of new sequence
    *
    ********************/

    virtual void InitSeq( ) = 0;

    /*********************
    *
    * Reinitialize rate control parameters for start of new GOP
    *
    ********************/

    virtual void InitGOP( ) = 0;

    /* ****************************
    *
    * Reinitialize rate control parameters for start of new Picture
    *
    * @return (re)encoding of picture to achieve rate-control mandatory
    *
    * ****************************/

    virtual void InitPict( Picture &picture ) = 0;


    double ScaleQuantf( int q_scale_type, double quant );
    EncoderParams &encparams;
    RateCtlState &state;
};

class Pass1RateCtl : public RateCtl
{    
public:
    Pass1RateCtl( EncoderParams &encoder, RateCtlState &state );

    /*********************
    *
    * Setup GOP structure for coding from nominal GOP size parameters
    *
    ********************/

    virtual void GopSetup( int nb, int np ) = 0;

};

class Pass2RateCtl : public RateCtl
{
public:
    Pass2RateCtl( EncoderParams &encoder, RateCtlState &state );

    /*********************
    *
    * Setup GOP structure for coding based on look-ahead data from pass-1
    *
    ********************/
    virtual void GopSetup( std::deque<Picture *>::iterator gop_begin,
                           std::deque<Picture *>::iterator gop_end ) = 0;

    /*********************
    * @pre PictureSetup called...
    * @return Re-encoding of picture to achieve rate control mandatory
    *
    *********************/
 
    virtual bool ReencodeRequired() const = 0;


    virtual unsigned int getEncodedFrames() const = 0;

    virtual double getStreamComplexity() const = 0;
};

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
