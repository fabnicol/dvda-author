#ifndef _PICTUREREADER_HH
#define _PICTUREREADER_HH

/* readpic.h Picture reader base class and basic file I/O based reader */
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

#include <pthread.h>
#include <deque>
#include "mjpeg_types.h"
#include "picture.hh"

class EncoderParams;
class ImagePlanes;
struct MPEG2EncInVidParams;

class PictureReader
{
public:
	PictureReader(EncoderParams &encoder );
    virtual ~PictureReader();
    void Init();
    void ReadPictureData( int num_frame, ImagePlanes &frame);
    virtual void StreamPictureParams( MPEG2EncInVidParams &strm ) = 0;
    ImagePlanes *ReadFrame( int num_frame );
    void ReleaseFrame( int num_frame );
    void FillBufferUpto( int num_frame );
    inline int NumberOfFrames() { return istrm_nframes; }
protected:
    void ReadChunkSequential( int num_frame );
    void AllocateBufferUpto( int buffer_slot );
    virtual bool LoadFrame( ImagePlanes &image ) = 0;
    
protected:
    EncoderParams &encparams;

	int frames_read; 
    int frames_released;
    std::deque<ImagePlanes *> input_imgs_buf;
    std::deque<ImagePlanes *>  unused;
    int istrm_nframes;      // Number of frames in stream once EOS known,
                                     // Otherwise INT_MAX
};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif

