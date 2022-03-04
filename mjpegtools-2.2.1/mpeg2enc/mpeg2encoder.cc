/* mpeg2encoder.hh Top-level class for an instance of the mpeg2enc++
 * MPEG-1/2 encoder. That evolved out of the MSSG mpeg2enc reference
 * encoder  */

/*  (C) 2003 Andrew Stevens */

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


#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif

#include "mpeg2encoder.hh"
#include "picturereader.hh"
#include "elemstrmwriter.hh"
#include "quantize.hh"
#include "ratectl.hh"
#include "seqencoder.hh"
#include "mpeg2coder.hh"

#include "simd.h"
#include "motionsearch.h"

MPEG2Encoder::MPEG2Encoder( MPEG2EncOptions &_options) :
    options( _options ),
    parms( options ),
    reader(0),
    writer(0),
    quantizer(0),
    coder(0),
    pass1ratectl(0),
    pass2ratectl(0)
{
    if( !simd_init )
        SIMDInitOnce();
    simd_init = true;
}



MPEG2Encoder::~MPEG2Encoder()
{
    delete seqencoder;
    delete pass1ratectl;
    delete pass2ratectl;
    delete coder;
    delete quantizer;
    delete writer;
    delete reader;
}


bool MPEG2Encoder::simd_init = false;
    
void MPEG2Encoder::SIMDInitOnce()
{
	init_motion_search();
	init_transform();
	init_predict();
}    




/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
