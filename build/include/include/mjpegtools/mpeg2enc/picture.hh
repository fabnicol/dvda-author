
/*  (C) 2000/2001/2002 Andrew Stevens */

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


#ifndef _PICTURE_HH
#define _PICTURE_HH

#include "mjpeg_types.h"
#include "encoderparams.hh"
#include "synchrolib.h"
#include "macroblock.hh"
#include <vector>
#include "mpeg2syntaxcodes.h"

using namespace std;


/* Transformed per-picture data  */

typedef int MotionVecPred[2][2][2];
typedef int DC_DctPred[3];

class CodingPredictors
{
public:

    void Reset_DC_DCT_Pred()
        {
            int cc;
            for (cc=0; cc<3; cc++)
                dc_dct_pred[cc] = 0;
                
        }
    void Reset_MV_Pred()
        {
            int *base=&PMV[0][0][0];
            int v;
            for( v = 0; v < 2*2*2; ++v)
                base[v]=0;
        }

    DC_DctPred dc_dct_pred;
    MotionVecPred PMV;
    MacroBlock *prev_mb;
    
};

class RateCtl;
class Quantizer;
class StreamState;
class ElemStrmWriter;
class MPEG2CodingBuf;
class ImagePlanes;

class Picture : public CodingPredictors
{
public:
    
    
    Picture( EncoderParams &_encparams, 
             ElemStrmWriter &writer,
             Quantizer &_quantizer );
    ~Picture();

    void QuantiseAndCode(RateCtl &ratecontrol);

    void MotionSubSampledLum();
    void ITransform();
    void IQuantize();
    void CalcSNR();
    void Stats();
    void Reconstruct();
    void CommitCoding();
    void DiscardCoding();
    
    void SetFrameParams( const StreamState &ss, int field );

    
    // Metrics used for stearing the encoding
    inline double Complexity() const { return Xhi; }
    void SetComplexity( double _Xhi ) { Xhi = _Xhi; }
    int EncodedSize() const;
    double IntraCodedBlocks() const;   // Proportion of Macroblocks coded Intra


    void PutHeaders();                // Picture/Gop/Sequence headers
    void PutTrailers( int padding );  // Stuff after picture but before next picture

    void PutHeader(); 

    double ActivityBestMotionComp();
    double VarSumBestMotionComp();
    double VarSumBestFwdMotionComp();
    double MinVarBestMotionComp();
    //
    //
    inline bool Legal( const MotionVector &mv ) const
        {
            return mv[Dim::X] >= -sxf && mv[Dim::X] < sxf 
                && mv[Dim::Y] >= -syf && mv[Dim::Y] < syf;
        }


    inline bool InRangeFrameMVRef( const Coord &crd ) const
        {
            return  crd.x >= 0 && crd.x <= (encparams.enc_width-16)*2
                && crd.y >= 0 && crd.y <= (encparams.enc_height-16)*2;
            
        }

    inline bool InRangeFieldMVRef( const Coord &crd ) const
        {
            return  crd.x >= 0 && crd.x <= (encparams.enc_width-16)*2
                && crd.y >= 0 && crd.y <= (encparams.enc_height/2-16)*2;
            
        }

    inline bool FinalFieldOfRefFrame() const
        {
            return pict_type != B_TYPE && finalfield;
        }
protected:
    
    void SetFieldParams(int field);
    void PutSliceHdr( int slice_mb_y, int mquant );
    void PutMVs( MotionEst &me, bool back );
    void PutDCTBlocks( MacroBlock &mb, int mb_type );
    void PutCodingExt(); 
    bool SkippableMotionMode( MotionEst &cur_mb_mm, MotionEst &prev_mb_mm);

public:

    /***************
     *
     * Data areas allocated at construction
     *
     **************/

    EncoderParams &encparams;
    Quantizer &quantizer;
    MPEG2CodingBuf *coding;
    
	/* 8*8 block data, raw (unquantised) and quantised, and (eventually but
	   not yet inverse quantised */
	DCTblock *blocks;
	DCTblock *qblocks;

	/* Macroblocks of picture */
	vector<MacroBlock> mbinfo;


    /***************
     *
     * Data update as picture is re-used for different images in streams
     *
     **************/
 
	int decode;				// Number of frame in stream 
	int present;			// Number of frame in playback order
                            // == Number of frame in input stream

    bool last_picture;         // Last Picture of a stream
    bool finalfield;        // Last field of a frame

	/* multiple-reader/single-writer channels Synchronisation  
	   sync only: no data is "read"/"written"
	 */
    Picture *fwd_ref_frame;
    Picture *bwd_ref_frame;     // 0 if Not B_TYPE
    
	/* picture encoding source data  */
	ImagePlanes *fwd_org, *bwd_org;	// Original Images of fwd and bwd
                                    // reference pictures
	ImagePlanes *fwd_rec, *bwd_rec;	// Reconstructed  Images for fwd and 
                                    // bwd references pictures
	ImagePlanes *org_img, *rec_img;	// Images for current pict: orginal
                                                        // and reconstructed.  Reconstructed
                                                        // 0 for B planes except when debugging
	ImagePlanes *pred;
	int sxf, syf, sxb, syb;		/* MC search limits. */
	bool secondfield;			/* Second field of field frame */
	bool ipflag;				/* P pict in IP frame (FIELD pics only)*/

	/* picture structure (header) data */

	int temp_ref; /* temporal reference */
    PICTURE_CODING frame_type;  /* picture coding type for frame  (I, P or B) */
    int gop_decode;              /* Frame number in gop (decode order) */
    int bgrp_decode;            /* Frame number in 'B group' (decode order) */
    PICTURE_CODING pict_type;   /* picture coding type for current field (I, P or B) */
	int vbv_delay;              /* video buffering verifier delay (1/90000 seconds) */
	int forw_hor_f_code, forw_vert_f_code;
	int back_hor_f_code, back_vert_f_code; /* motion vector ranges */
	int dc_prec;				/* DC coefficient prec for intra blocks */
    PICTURE_STRUCT pict_struct;	/* picture structure (frame, top / bottom) */
	int topfirst;				/* display top field first */
	bool frame_pred_dct;			/* Use only frame prediction... */
	int intravlc;				/* Intra VLC format */
	int q_scale_type;			/* Quantiser scale... */
	bool altscan;				/* Alternate scan  pattern selected */
    const uint8_t *scan_pattern; /* The scan pattern itself */
	bool repeatfirst;			/* repeat first field after second field */
	bool prog_frame;				/* progressive frame */

    int unit_coeff_threshold;   // Unit coefficient density weight
                                // below which zeroing should be applied.
    int unit_coeff_first;       // First coefficient for zeroing purposes...
                                // either 1 or 0.
	/* Information for GOP start frames */
	bool gop_start;             /* GOP Start picture */
    bool closed_gop;            /* GOP is closed   */
	int nb;						/* B frames in GOP */
	int np;						/* P frames in GOP */
	bool new_seq;				/* GOP starts new sequence */
    bool end_seq;               /* Frame ends sequence */
    
    double Xhi;                 /* Complexity ... product of bits needed to code and
                                   quantisation */

    /* Rate control statistics  */
    double AQ;       // Mean actual quantisation of encoding (if any)
    double ABQ;      // Mean base quantisation of encoding (if any)


	/* Statistics... */
	int pad;
	//int split;

	double SQ;
	double avg_act;
	double sum_avg_act;

    
};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
