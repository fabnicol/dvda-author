#ifndef _SEQENCODER_HH
#define _SEQENCODER_HH


/*  (C) 2000/2001/2005 Andrew Stevens */

/*  This Software is free software; you can redistribute it
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
#include "picture.hh"
#include "streamstate.h"

class MPEG2Encoder;
class EncoderParams;
class MPEG2CodingBuf;
class PictureReader;
class Despatcher;
class RateCtlState;
class Pass1RateCtl;
class Pass2RateCtl;

class SeqEncoder
{
public:
	SeqEncoder( EncoderParams &encparams,
                PictureReader &reader,
                Quantizer &quantizer,
                ElemStrmWriter &writer,
                Pass1RateCtl   &pass1ratectl,
                Pass2RateCtl   &pass2ratectl
        );
	~SeqEncoder();


    /**********************************
     *
     * Setup ready to start encoding once parameter objects have
     * (where relevant) been Init-ed.
     *
     *********************************/

    void Init();
    

     /**********************************
     *
     * Encode Stream setup during Init
     *
     *********************************/
    void EncodeStream();

	/**********************************
	*
	* Do a unit of encocoding: Pass1 encode
    * a buffered frame (if available)
    * Pass2 encode a buffered Pass1 encoded
    * frame if enough are buffer.
	*
	*********************************/
	void EncodeStreamOneStep();

	/**********************************
	*
	* There are still Pass1-coded frames
    * buffered remaining to be committed for Pass2
    * or pass-2 coded.
    *
    * NOTE: Result is only correct
    * when called AFTER
    * EncodeStreamOneStep().
	*
	*********************************/
	bool EncodeStreamWhile();

	/**********************************
	*
	* Perform Epilogue to encoding video stream
	* Ensure all encoding work still being worked on
	* is completed and flushed.  Collects statistics etc etc.
	*
	*********************************/
	void StreamEnd();

        
private:

     /*********************************
     *
     * Manage Picture's - they're expensive-ish to allocate so
     * we maintained a stack already allocated but unused objects.
     *
     *********************************/
     
    Picture *GetFreshPicture();
    void ReleasePicture( Picture *);
      
    /**********************************
     *
     * Pass1Process - Unit of pass-1 processing work
     * generates a pass-1 coded frame ready for pass2
     * processing.
     *
     *********************************/
    void Pass1Process();
    
     /**********************************
     *
     * Pass2Process - Unit of pass-2 processing work
     * Consumes any available pass-1 coded frames ready
     * for pass2 coding.
     * If possible generates a frame of coded output.
     *
     *********************************/
    void Pass2Process();

    void Pass1RateCtlSetup( Picture &picture );
    void Pass2RateCtlSetup( Picture &picture );

    Picture *NextFramePicture0();
    Picture *NextFramePicture1(Picture *picture0);
    void EncodePicture( Picture &picture, RateCtl &ratectl);
    void RetainPicture( Picture &picture, RateCtl &ratectl);

    void Pass1GopSplitting( Picture &picture);
    void Pass1EncodePicture( Picture &picture, int field );
    void Pass1ReEncodePicture0( Picture &picture, void (MacroBlock::*modeMotionAdjustFunc)() );
    bool Pass2EncodePicture( Picture &picture, bool force_reencode );
    
    uint64_t BitsAfterMux() const;
    
    EncoderParams &encparams;
    PictureReader &reader;
    Quantizer &quantizer;
    ElemStrmWriter &writer;
    Pass1RateCtl    &pass1ratectl;
    Pass2RateCtl    &pass2ratectl;

    // Worker thread despatchers for the two passes
    //

    Despatcher &p1_despatcher;
    //Despatcher &p2_despatcher;
	
    // The state of the pass 1 rate controller before encoding.
    // We need to restore this if we decide to re-encode it in
    // pass-1
    RateCtlState *pass1_rcstate;

    // Picture's (in decode order) that have been pass1 coded
    // but are not yet ready to be committed for pass 2 coding...
    std::deque<Picture *> pass1coded;

    // Queue of Picture's (in decode order) committed for pass2 encoding
    std::deque<Picture*> pass2queue;

    // Picture objects no longer being encoded (signalled by
    // a called to 'ReleasePicture') but potentially still
    // referenced by other Picture's and hence not (yet) free
    // for re-use or destruction.

    std::deque<Picture*> released_pictures;

    // Reference frames all of whose field Picture's have been released
    // ... needed to maintain released_pictures queue.
    int released_ref_frames;

    // Picture objects free for re-use.
    std::vector<Picture *> free_pictures;
    
    
	// Internal state of encoding...
	StreamState pass1_ss;

    // Reference pictures in pass-1
	Picture *new_ref_picture, *old_ref_picture;

};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
