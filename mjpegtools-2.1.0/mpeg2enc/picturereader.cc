/* picturereader.cc   */

/* (C) 2000/2001/2003 Andrew Stevens, Rainer Johanni */

/* This software is free software; you can redistribute it
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


#include "picturereader.hh"
#include "mpeg2encoder.hh"
#include "imageplanes.hh"
#include <limits.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <string.h>
//#include <errno.h>
//#include "simd.h"


PictureReader::PictureReader( EncoderParams &_encparams ) :
    encparams( _encparams )
{
    frames_read = 0;
    frames_released = 0;
    istrm_nframes = INT_MAX;
}


void PictureReader::Init()
{

}

PictureReader::~PictureReader()
{
    for( unsigned int i = 0; i < input_imgs_buf.size(); ++i )
        delete input_imgs_buf[i];

}

void PictureReader::AllocateBufferUpto( int buffer_slot )
{
    for( int i = input_imgs_buf.size(); i <= buffer_slot; ++i )
    {
        input_imgs_buf.push_back(new ImagePlanes( encparams ));
    }
}

void PictureReader::ReleaseFrame( int num_frame)
{
    while( frames_released <= num_frame )
    {
        input_imgs_buf.push_back( input_imgs_buf.front() );
        input_imgs_buf.pop_front();
        ++frames_released;
    }
}



void PictureReader::FillBufferUpto( int num_frame )
{
    while(frames_read <= num_frame  &&   frames_read < istrm_nframes ) 
    {
        AllocateBufferUpto( frames_read-frames_released );
        if( LoadFrame( *input_imgs_buf[frames_read-frames_released] ) )
        {
            istrm_nframes = frames_read;
            mjpeg_info( "Signaling last frame = %d", istrm_nframes-1 );
            return;
        }
        ++frames_read; 
    }
}

ImagePlanes *PictureReader::ReadFrame( int num_frame )
{
    if(istrm_nframes!=INT_MAX && num_frame>=istrm_nframes )
    {
        mjpeg_error("Internal error: PictureReader::ReadFrame: attempt to reading beyond known EOS");
        abort();
    }
   FillBufferUpto( num_frame );
   return input_imgs_buf[num_frame-frames_released];
}




/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
