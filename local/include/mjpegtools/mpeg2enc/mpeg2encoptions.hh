#ifndef _MPEG2ENCOPTIONS_HH
#define _MPEG2ENCOPTIONS_HH

/* mpeg2encoptions.h - Encoding options for mpeg2enc++ MPEG-1/2
 * encoder library */
/*  (C) 2000/2001 Andrew Stevens */

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

#include "mpeg2encparams.h"
#include "mpegconsts.h"

#include <deque>

using std::deque;


struct MPEG2EncInVidParams
{
    int horizontal_size;
    int vertical_size;
    mpeg_aspect_code_t aspect_ratio_code;
    mpeg_framerate_code_t frame_rate_code;
    int interlacing_code;
};


class MPEG2EncOptions : public MPEG2EncParams
{
public:
    MPEG2EncOptions();
    bool SetFormatPresets(  const MPEG2EncInVidParams &strm );  // True iff fail
    int InferStreamDataParams( const MPEG2EncInVidParams &strm );
    int CheckBasicConstraints();

    uint16_t custom_intra_quantizer_matrix[64];
    uint16_t custom_nonintra_quantizer_matrix[64];

    deque<int> chapter_points;
};
#endif

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
