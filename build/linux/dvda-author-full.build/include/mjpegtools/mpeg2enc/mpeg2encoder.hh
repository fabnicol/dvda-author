#ifndef _MPEG2ENCODER_HH
#define _MPEG2ENCODER_HH

/* mpeg2encoder.hh Top-level class for an instance of mpeg2enc++
 * MPEG-1/2 encoder.   */
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

#include <stdio.h>
#include "mpeg2encoptions.hh"
#include "encoderparams.hh"

 
class EncoderParams;
class PictureReader;
class Pass1RateCtl;
class Pass2RateCtl;
class SeqEncoder;
class Quantizer;
class Transformer;
class MPEG2CodingBuf;
class BitStreamWriter;
class ElemStrmWriter;

class MPEG2Encoder
{
public:
    MPEG2Encoder( MPEG2EncOptions &options );
    ~MPEG2Encoder();

    static void SIMDInitOnce();
    static bool simd_init;
    MPEG2EncOptions &options;
    EncoderParams parms;
    PictureReader  *reader;
    ElemStrmWriter *writer;
    Quantizer      *quantizer;
    MPEG2CodingBuf *coder;
    Pass1RateCtl   *pass1ratectl;
    Pass2RateCtl   *pass2ratectl;
    SeqEncoder     *seqencoder;
};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif

