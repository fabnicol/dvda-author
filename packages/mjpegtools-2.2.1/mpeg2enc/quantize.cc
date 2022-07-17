/* quantize.c, quantization / inverse quantization                          */

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
/* Modifications and enhancements (C) 2000/2001 Andrew Stevens */

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
#include "encoderparams.hh"
#include "mpeg2syntaxcodes.h"
#include "picture.hh"
#include "macroblock.hh"
#include "quantize.hh"

#include <stdlib.h>

/********************
 * 
 * Unit coefficient elimination.
 * Zero DCT blocks in with 'only a few widely scattered' unit DCT coefficients.
 * The basis for this heuristic is that such blocks have a high coding cost
 * relative to the modest amount of picture information they carry.
 *
 * Original implementation: Copyright (c) 2000,2001 Fabrice Bellard.
 * Same GPL V2 as above.
 *
 * I can't be bothered to look up the research papers on this topic so
 * I can't give the original references...
 *
 * RETURN: 1 if block zero-ed, 0 otherwise
 * 
 *******************/

static int unit_coeff_elimination(DCTblock &block, 
								   const uint8_t *scan_pattern,
								   const int start_coeff,
								   const int threshold)
{
    static const char run_shortness_weight[64]=
        {3,2,2,1,1,1,1,1,
         1,1,1,1,1,1,1,1,
         1,1,1,1,1,1,1,1,
         0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0};

    int score=0;
    int run=0;
    int i;

	// A non-unit DC coefficent makes zero-ing a bad idea... always
	if( start_coeff > 0 && block[0] > 1 )
		return 0;
	
	// 
	// Compute a measure of the 'denseness' of the unit coefficients
	// give up on zero-ing if a non-unit coefficient found...
	//
    for(i=start_coeff; i<64; i++)
	{
	    
        const int j = scan_pattern[i];
        const int level = abs(block[j]);
        if(level==1)
		{
            score += run_shortness_weight[run];
            run=0;
        }
		else if(level>1)
		{
			// Uh-oh non-unit coefficient... zeroing not sensible...
            return 0;
        }
		else
		{
            ++run;
        }
    }

	//
	// The weighted score of the zero runs lengths seperating the unit
	// coefficients is high (unit coefficients are densely packed
	// numerous) zero-ing not a good trade-off, abort.
	//
    if(score >= threshold) 
 		return 0;

	//
	// Zero the DCT block... N.b. all scan patterns have the DC coefficient
	// first.
	//
    for(i=start_coeff; i<64; i++)
	{
        block[i]=0;
    }

    return (block[0] == 0);
}

//
// TODO for efficiency the qdctblocks should be an external buffer managed by the calling slice/picture
// coder.
//
void MacroBlock::Quantize( Quantizer &quant  )
{
    if (best_me->mb_type & MB_INTRA)
    {
        quant.QuantIntra( dctblocks[0],
                          qdctblocks[0],
                          picture->q_scale_type,
                          picture->dc_prec,
                          picture->encparams.dctsatlim,
                          &mquant );
		
        cbp = (1<<BLOCK_COUNT) - 1;
    }
    else
    {
        cbp = quant.QuantInter( dctblocks[0],
                                qdctblocks[0],
                                picture->q_scale_type,
                                picture->encparams.dctsatlim,
                                &mquant );
        int block;
		if( picture->unit_coeff_threshold )
        {
            for( block = 0; block < BLOCK_COUNT; ++block )
            {
                int zero = 
                    unit_coeff_elimination( qdctblocks[block],
                                            picture->scan_pattern,
                                            picture->unit_coeff_first,
                                            picture->unit_coeff_threshold);
                cbp &= ~(zero<<(BLOCK_COUNT-1-block));
            }
        }
    }
}

void MacroBlock::IQuantize( Quantizer &quant)
{
    int j;
    if (best_me->mb_type & MB_INTRA)
    {
        for (j=0; j<BLOCK_COUNT; j++)
            quant.IQuantIntra(qdctblocks[j], qdctblocks[j], picture->dc_prec, mquant);
    }
    else
    {
        for (j=0;j<BLOCK_COUNT;j++)
            quant.IQuantInter(qdctblocks[j], qdctblocks[j], mquant);
    }
}



Quantizer::Quantizer( EncoderParams &_encparams ) :
    encparams( _encparams )
{
}

void Quantizer::Init()
{
    init_quantizer( static_cast<QuantizerCalls *>(this),
                    &workspace,
                    static_cast<int>(encparams.mpeg1), 
                    encparams.intra_q, 
                    encparams.inter_q );
}

Quantizer::~Quantizer()
{
    shutdown_quantizer( workspace );
}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
