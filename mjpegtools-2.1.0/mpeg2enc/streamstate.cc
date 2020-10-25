/* (C) 2005 Andrew Stevens 
 *  This file is free software; you can redistribute it
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

#include "streamstate.h"
#include <cassert>
#include <algorithm>
#include "mjpeg_types.h"
#include "mjpeg_logging.h"
#include "encoderparams.hh"
#include "mpeg2syntaxcodes.h"
#include "picturereader.hh"




// --------------------------------------------------------------------------------
//  Stream state maintenance class


StreamState::StreamState( EncoderParams &_encparams, PictureReader &_reader ) :
    encparams(_encparams),
    reader(_reader)
{
}

void StreamState::Init(  )
{
    seq_split_length = ((int64_t)encparams.seq_length_limit)*(8*1024*1024);
    next_split_point = BITCOUNT_OFFSET + seq_split_length;
    mjpeg_debug( "Split len = %lld", seq_split_length );

    frame_num = 0;          // All these counters in *encoding* order !!
    s_idx = 0;
    g_idx = 0;
    b_idx = 0;
    gop_length = 0;             // Forces new GOP start 1st sequence.
    seq_start_frame = 0;
    gop_start_frame = 0;
    gop_end_seq = true;         // At start we act as after sequence split...
    GopStart(  );
    SetTempRef();
}

/*
    Could the following GOP be closed?
*/

bool StreamState::NextGopClosed() const 
{ 
    return gop_end_seq || encparams.closed_GOPs || gop_start_frame + gop_length == GetNextChapter();
}

int StreamState::GetNextChapter() const
{
    // advance return value when gop_start_frame exceeds or equals current chapter point
    while( !encparams.chapter_points.empty() ) {
        int next_chapter=encparams.chapter_points.front();
        if( next_chapter > gop_start_frame )
            return next_chapter;
        encparams.chapter_points.pop_front();
    }
    return -1;
}

bool StreamState::CanSplitHere(int offset) const
{
    int nc=GetNextChapter();

    // Enforce GOP dimensioning.
    if( g_idx+offset < encparams.N_min )
        return false;

    // Enforce chapter splits...
    if( nc < 0 )
        return true;
    int nc_distance = nc - (frame_num+offset);
    if( nc_distance < 0 || g_idx+offset < encparams.N_min)
        return false;

    // Check that legal GOP sizes allow the the next chapter point to
    // be hit exactly.  If it is between x and x+1 minimum GOPs away
    // it must be x or less maximum gops away as only then can a mixture
    // of gops sized somewhere between minimum and maximum sizes reach it
    // exactly.
    // Division must occur first; rounding down is relied upon!
    return nc_distance <= (nc_distance/encparams.N_min) * encparams.N_max;

}


/*
  Update ss to the next sequence state.
*/

void StreamState::Next(  uint64_t bits_after_mux )   // Estimate of how much output would have been produced
{
    ++frame_num;    
    ++s_idx;
    ++g_idx;
    ++b_idx;  
    
    new_seq = false;
    /* Are we starting a new B group */
    if( b_idx >= bigrp_length )
    {
        b_idx = 0;
        /* Does this need to be a short B group to make the GOP length
           come out right ? */
        if( bs_short != 0 && g_idx > (int)next_b_drop )
        {
            if( bs_short )
                next_b_drop += ((double)gop_length) / (double)(bs_short+1) ;
            bigrp_length = encparams.M - 1;
        }
        else if( !suppress_b_frames )
            bigrp_length = encparams.M;
        else
            bigrp_length = 1;

        // Do we need to start next GOP?
        if( g_idx == gop_length )
        {
            GopStart(  );            // Sets frame_type == I_TYPE
        }
        else
        {
            frame_type = P_TYPE;
        }
    }
    else
    {
        frame_type = B_TYPE;
    }

    // Figure out if a sequence split is due...
    if( (next_split_point != 0ULL && bits_after_mux > next_split_point)
        || (s_idx != 0 && encparams.seq_end_every_gop)
        )
    {
        mjpeg_info( "Splitting sequence next GOP start" );
        next_split_point += seq_split_length;
        gop_end_seq = true;
    }
    SetTempRef();
}

/*
	Switch stream state to force an I-Frame where a P-frame would be expected.
    Basically: start a new GOP and do the usual house-keeping.
*/

void StreamState::ForceIFrame()
{
    assert( frame_type != B_TYPE );
    GopStart();
    SetTempRef();
}

/*
    Switch stream state so that no B frames are used from the current
    frame and onwards.
*/

void StreamState::SuppressBFrames()
{
    assert( b_idx == 0 && encparams.M_min == 1);
    frame_type = P_TYPE;
    if( encparams.M_min == 1 )
    {
        np += nb;
        nb = 0;
        bigrp_length = encparams.M_min;
        bs_short = 0;
        suppress_b_frames = true;
        SetTempRef();
    }
}


void StreamState::SetTempRef()
{
    // Ensure we have read up to the input frame we might need if the next
    // frame in decode order is an I or P.   Make sure we don't go 'of the end'
    // once we've reached EOS

    int read_ahead_frame = frame_num+encparams.M;
    reader.FillBufferUpto( read_ahead_frame  );
    int last_frame = reader.NumberOfFrames()-1;
    if( frame_type == B_TYPE )
        temp_ref =  g_idx - 1;
    else if( g_idx == 0 && closed_gop)
        temp_ref =   0;
    else // I or P frame open GOP
        temp_ref = g_idx+(bigrp_length-1);

    // At end of Sequence may need to truncate bigrp!
    if (temp_ref > (last_frame-gop_start_frame))
        temp_ref = (last_frame-gop_start_frame);
    
    // DEBUG remove when validated
    assert( frame_num + temp_ref - g_idx == gop_start_frame + temp_ref );
    end_stream = frame_num > last_frame;
    end_seq =  frame_num == last_frame || ( g_idx == gop_length-1 && gop_end_seq);
}


void StreamState::GopStart(  )
{
    /* If   we're starting a GOP and have gone past the current
       sequence splitting point split the sequence and
       set the next splitting point.
    */

    suppress_b_frames = false;
    g_idx = 0;
    b_idx = 0;
    frame_type = I_TYPE;

    /* Normally set closed_GOP in first GOP only...   */

    closed_gop = NextGopClosed(); // must call this before the gop_end_seq code
    gop_start_frame = frame_num;

    /* Sequence ended at end previous GOP so this one starts a new sequence */
    if( gop_end_seq )
    {
        /* We split sequence last frame.This is the input stream display 
         * order sequence number of the frame that will become frame 0 in display
         * order in  the new sequence 
         */
        seq_start_frame =frame_num;
        s_idx = 0;
        gop_end_seq = false;
        new_seq = true;
    }
    

    // 
    // GOPs initially always start out maximum length - short GOPs occur when we notice
    // a P frame occurring after the minimum GOP lengthhas been reached
    //
    // also shorten a GOP if we have an upcoming chapter point we are aiming for
    //

    for( gop_length = encparams.N_max; gop_length > encparams.N_min; gop_length-- )
        if( CanSplitHere(gop_length) )
            break;

    mjpeg_info( "NEW GOP INIT length %d", gop_length );
    /* First figure out how many B frames we're short from
       being able to achieve an even M-1 B's per I/P frame.
       
       To avoid peaks in necessary data-rate we try to
       lose the B's in the middle of the GOP. We always
       *start* with M-1 B's (makes choosing I-frame breaks simpler).
       A complication is the extra I-frame in the initial
       closed GOP of a sequence.
    */
    if( encparams.M-1 > 0 )
    {
        int pics_in_bigrps = 
            closed_gop ? gop_length - 1 : gop_length;
        bs_short = (encparams.M - pics_in_bigrps % encparams.M)%encparams.M;
        next_b_drop = ((double)gop_length) / (double)(bs_short+1)-1.0 ;
    }
    else
    {
        bs_short = 0;
        next_b_drop = 0.0;
    }
    
    /* We aim to spread the dropped B's evenly across the GOP */
    bigrp_length = (encparams.M-1);
    
    if (closed_gop )
    {
        bigrp_length = 1;
        np = (gop_length + 2*(encparams.M-1))/encparams.M - 1; /* Closed GOP */
    }
    else
    {
        bigrp_length = encparams.M;
        np = (gop_length + (encparams.M-1))/encparams.M - 1;
    }
    /* number of B frames */
    nb = gop_length - np - 1;

    //np = np;
    //nb = nb;
    if( np+nb+1 != gop_length )
    {
        mjpeg_error_exit1( "****INTERNAL: inconsistent GOP %d %d %d", 
                           gop_length, np, nb);
    }

}


