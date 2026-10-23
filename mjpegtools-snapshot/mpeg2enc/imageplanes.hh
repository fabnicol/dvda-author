#ifndef _IMAGEPLANES_HH
#define _IMAGEPLANES_HH

/*  (C) 2005 Andrew Stevens */

/*  This is free software; you can redistribute it
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

#include <vector>
#include "picture.hh"
#include "mjpeg_types.h"
#include "encoderparams.hh"

/************************************************
*
* ImagePlanes - Raw image data with 'margins' around 
* actual data to allow motion estimation routines etc
* to be sloppy about cropping at image edges to save
* time. 
*
*
************************************************/

class ImagePlanes
{
    public:
        enum Planes_Enum { YPLANE=0, UPLANE=1, VPLANE=2, Y22=3, Y44=4, NUM_PLANES };
        
        ImagePlanes( EncoderParams &encoder );
        ~ImagePlanes();

        inline uint8_t *Plane( unsigned int plane) { return planes[plane]; }
        inline uint8_t **Planes() { return planes; }
        void AddUserData(PictureUserData *pdata);
        void DiscardUserData();
        std::vector<PictureUserData *> user_data;
    
    protected:
        static void BorderMark( uint8_t *frame,  
                                int total_width, int total_height,
                                int image_data_width, int image_data_height);
    protected:
        uint8_t *planes[NUM_PLANES];
};



#endif // _IMAGEPLANES_HH
