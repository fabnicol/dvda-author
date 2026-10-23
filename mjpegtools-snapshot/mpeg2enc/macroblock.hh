#ifndef _MACROBLOCK_HH
#define _MACROBLOCK_HH 
/* macroblock.hh macroblock class... */

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

/*  (C) 2000-2004 Andrew Stevens */

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

#include <vector>
#include "mjpeg_types.h"
#include "encodertypes.h"

using namespace std;

class Picture;


typedef int16_t DCTblock[64];


class MotionEst
{
public:
    enum Direction { fwd = 0, bwd =1 };

	int mb_type; /* intra/forward/backward/interpolated */
	int motion_type; /* frame/field/16x8/dual_prime */
	MotionVector MV[2][2]; /* motion vectors */
	int field_sel[2][2]; /* motion vertical field select */
	MotionVector dualprimeMV; /* dual prime vectors */
	int var; 	/* luminance variance after motion compensation 
                   (measure of activity) */
};

class Quantizer;
class MotionCand;

/* macroblock information */
class SubSampledImg;

class MacroBlock
{
public:
    MacroBlock(Picture &_picture,
               const unsigned int _i,
               const unsigned int _j,
               DCTblock *_dctblocks,
               DCTblock *_qdctblocks
               ) :
        picture(&_picture),
        i(_i),
        j(_j),
        pel( _i, _j ),
        hpel( _i<<1, _j<<1 ),
        dctblocks(_dctblocks),
        qdctblocks(_qdctblocks)
        {
        }

    inline Picture &ParentPicture() const { return *picture; }
    inline int BaseLumVariance() const { return best_me->var; }
    inline double Activity() const { return act; }
    inline const int TopleftX() const { return i; }
    inline const int TopleftY() const { return j; }
    inline DCTblock *RawDCTblocks() const { return dctblocks; }
    inline DCTblock *QuantDCTblocks() const { return qdctblocks; }


    void Encode();
    void MotionEstimateAndModeSelect();
    void ForceIFrame();
    void ForcePFrame();
    void Quantize( Quantizer &quant);             // In quantize.cc
    void IQuantize( Quantizer &quant);
    void Transform();          // In transfrm.cc
    void ITransform();

protected:
    void MotionEstimate();
    void SelectCodingModeOnVariance();
    void FrameME();            // In motionest.cc
    void FrameMEs();
    void FieldME();
    void Predict();            // In predict.cc



private:
    bool FrameDualPrimeCand(uint8_t *ref,
                            const SubSampledImg &ssmb,
                            const MotionCand (&best_fieldmcs)[2][2], 
                            MotionCand &best_mc,
                            MotionVector &min_dpmv);

private:

    Picture *picture;   
    unsigned int i,j;           // Co-ordinates top-left in picture DEBUG
    Coord pel;                  // Co-ordinates top-left in picture (pels)
    Coord hpel;                 // Co-ordindates top-left in picture (half-pel)
    int row_start;              // Offset from frame top to start of MB's row


    DCTblock *dctblocks;
    DCTblock *qdctblocks;

    uint32_t lum_mean;
    uint32_t lum_variance;

    /* Old public struct information...
       TODO: This will gradually disappear as C++-ification continues
    */

public:
	bool field_dct;             // Field DCT encoded rather than frame DCT
	int mquant; /* quantization parameter */
	int cbp; /* coded block pattern */
	bool skipped; /* skipped macroblock */
	double act; /* activity measure */
	int i_act;  /* Activity measure if intra coded (I/P-frame) */
	int p_act;  /* Activity measure for *forward* prediction (P-frame) */
	int b_act;	/* Activity measure if bi-directionally coded (B-frame) */
    vector<MotionEst> best_of_kind_me; 
                                 // The best predicting motion compensation
                                // of each possible kind.
    MotionEst *best_me;      // Best predicting motion estimate overall
    MotionEst *best_fwd_me; // Best predicting motion compensation requiring only
                                            // forward motion compensation
#ifdef OUTPUT_STAT
  double N_act;
#endif

};

 

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
