/* macroblock.hh macroblock class... */

/*  (C) 2003 Andrew Stevens */

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

#include <stdio.h>
#include <cassert>
#include <limits.h>

#include "macroblock.hh"
#include "mpeg2syntaxcodes.h"
#include "picture.hh"

void MacroBlock::Encode()
{ 
    Predict();
    Transform();
}


void MacroBlock::MotionEstimateAndModeSelect()
{ 
    MotionEstimate();
    SelectCodingModeOnVariance();
}

void MacroBlock::MotionEstimate()
{
	if (picture->pict_struct==FRAME_PICTURE)
	{			
		FrameMEs();
	}
	else
	{		
		FieldME();
	}
}

void MacroBlock::SelectCodingModeOnVariance()
{
    vector<MotionEst>::iterator i;
    vector<MotionEst>::iterator min_me;
    int best_score = INT_MAX;
    int best_fwd_score = INT_MAX;
    int cur_score;


    assert( best_of_kind_me.begin()->mb_type == MB_INTRA );
    //
    // Select motion estimate with lowest variance
    // Penalise the INTRA motion type slightly because it can't be
    // skip coded and the DC coefficient is usually large...
    for( i = best_of_kind_me.begin(); i < best_of_kind_me.end(); ++ i)
    {
        cur_score = i->var + (i->mb_type == MB_INTRA ? 3*3*256 : 0);
        if( cur_score < best_score )
        {
            best_score = cur_score;
            best_me = &*i;
        }
        if( i->mb_type & MB_BACKWARD == 0 && cur_score < best_fwd_score)
        {   
            best_fwd_score = cur_score;
            best_fwd_me = &*i;
        }
    }

}

/**********************************************
 *
 * ForceIFrame - Force selection of intra-coding so that that macroblock
 *             can be correctly coded in an I Frame.
 *
 *********************************************/

void MacroBlock::ForceIFrame()
{
    vector<MotionEst>::iterator i = best_of_kind_me.begin();
    assert( i->mb_type == MB_INTRA );
    best_me = &*i;
}

/**********************************************
 *
 * ForcePFrame - Force selection of motion-estimation so that that macroblock
 *             can be correctly coded in an P Frame.  I.e. use only forward
 *             motion estimatino.
 *
 *********************************************/

void MacroBlock::ForcePFrame()
{
    best_me = best_fwd_me;
}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */

