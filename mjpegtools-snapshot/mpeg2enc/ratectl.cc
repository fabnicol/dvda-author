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
#include "simd.h"
#include "fastintfns.h"
#include "mpeg2encoder.hh"
#include "picture.hh"
#include "ratectl.hh"
#include "quantize.hh"


RateCtl::RateCtl( EncoderParams &_encparams, RateCtlState &_state ) :
	encparams( _encparams ),
    state( _state )
{
}

void RateCtl::PictSetup( Picture &picture)
{
    /* Handle splitting of output stream into sequences of desired size */
    if(  picture.new_seq )
    {
        InitSeq();
    }

    if( picture.gop_decode == 0)
    {
        InitGOP();
    }


    InitPict( picture );

}

double RateCtl::ClipQuant( int q_scale_type, double quant )
{
	double quantf = quant;
	if ( q_scale_type )
	{		/* clip to legal (non-linear) range */
		if (quantf<1)
		{
            quantf = 1;
		}
		
		if (quantf>111)
		{
			quantf = 112;

		}
    }
    else
    {   		/* clip mquant to legal (linear) range */

		if (quantf<2.0)
			quantf = 2.0;
		if (quantf>62.0)
			quantf = 62.0;
    }
    return quantf;
}

double RateCtl::ScaleQuantf( int q_scale_type, double quant )
{
	double quantf;

	if ( q_scale_type )
	{
		int iquantl, iquanth;
		double wl, wh;

		wh = quant-floor(quant);
		wl = 1.0 - wh;
		iquantl = (int) floor(quant);
		iquanth = iquantl+1;
		/* clip to legal (non-linear) range */
		if (iquantl<1)
		{
			iquantl = 1;
			iquanth = 1;
		}
		
		if (iquantl>111)
		{
			iquantl = 112;
			iquanth = 112;
		}
		
		quantf = (double)
		  wl * (double)non_linear_mquant_table[map_non_linear_mquant[iquantl]] 
			+ 
		  wh * (double)non_linear_mquant_table[map_non_linear_mquant[iquanth]]
			;
	}
	else
	{
		/* clip mquant to legal (linear) range */
		quantf = quant;
		if (quantf<2.0)
			quantf = 2;
		if (quantf>62.0)
			quantf = 62.0;
	}
	return quantf;
}


int RateCtl::ScaleQuant( int q_scale_type , double quant )
{
	double quantf = ClipQuant( q_scale_type, quant );
    int iquant = (int)floor(quantf+0.5);
	if ( q_scale_type  )
	{
		iquant =
			non_linear_mquant_table[map_non_linear_mquant[iquant]];
	}
	else
	{
		iquant = (iquant/2)*2; // Must be *even*
	}
	return iquant;
}

double RateCtl::InvScaleQuant( int q_scale_type, int raw_code )
{
	int i;
	if( q_scale_type )
	{
		i = 112;
		while( 0 < i && map_non_linear_mquant[i] != raw_code )
			--i;
		return ((double)i);
	}
	else
		return ((double)raw_code);
}

Pass1RateCtl::Pass1RateCtl( EncoderParams &_encparams, RateCtlState &_state ) :
        RateCtl( _encparams, _state )
{
}



Pass2RateCtl::Pass2RateCtl( EncoderParams &_encparams, RateCtlState &_state ) :
        RateCtl( _encparams, _state )
{
}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
