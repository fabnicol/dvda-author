#ifndef _STREAMSTATE_H
#define _STREAMSTATE_H


/*  (C) 2005Andrew Stevens */

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
 
 #include "mjpeg_types.h"
 
/************************************************
 *
 * StreamState - MPEG-1/2 streams have a fairly complex structure. The context of each picture
 *               in this structure determines how it is encoded. This is the class for maintaining
 *               iterating through such stream encoding contexts.
 *
 **********************************************/

class EncoderParams;
class PictureReader;

class StreamState 
{
public:
    StreamState( EncoderParams &encparams, PictureReader &reader );
    void Init( );
    void Next( uint64_t bits_after_mux );
    void ForceIFrame();
    void SuppressBFrames();

    // Handle seperately - need to know input frame number (set by Next)

    inline bool EndOfStream() const { return end_stream; }
    inline int BGroupLength() const { return bigrp_length; }
    inline int TemporalReference() const { return temp_ref; }
    inline int PresentationNum() const { return frame_num+temp_ref-g_idx; }
    inline int DecodeNum() const { return frame_num; }
    
    bool NextGopClosed() const;
    bool CanSplitHere(int offset=0) const;

protected:    
    void GopStart();

    void SetTempRef();

    int GetNextChapter() const;

public:
    // Conext of current frame in hierarchy of structures: sequence, GOP, B-group */
    int frame_num;                  /* Index in total video stream */
    int s_idx;                          /* Index in current sequence */
    int g_idx;                          /* Index in current GOP */
    int b_idx;                          /* Index in current B frame group */
    int frame_type;                 /* Type of indexed frame */
    int temp_ref;                   /* Temporal reference in GOP == presentation order in GOP */
    
    // Context of current sequence and GOP in the input image stream

    int seq_start_frame;        /* Index start current sequence in  input stream */
    int gop_start_frame;        /* Index start current gop in input stream */

    // GOP state
    int gop_length;             /* Length of current gop */
    int bigrp_length;           /* Length of current B-frame group */
    int bs_short;               /* Number of B frame GOP is short of
                                        having M-1 B's for each I/P frame
                                    */
    bool suppress_b_frames; /* Disable B frames for rest of current GOP */
    int np;                        /* P frames in current GOP */
    int nb;                        /* B frames in current GOP */
    double next_b_drop;         /* When next B frame drop is due in GOP */
    bool closed_gop;            /* Current GOP is closed */

    // Sequence splitting state
    bool gop_end_seq;      /* Current GOP is last in sequence */
    bool end_seq;             /* Current frame is last in sequence */
    bool new_seq;            /* Current GOP/frame starts new sequence */
    bool end_stream;       /* End of video stream reached - no further coding possible */

private:
    uint64_t next_split_point;      // Keep track of size-based points to split individual sequences
    uint64_t seq_split_length;
    EncoderParams &encparams;
    PictureReader &reader;
};

#endif
