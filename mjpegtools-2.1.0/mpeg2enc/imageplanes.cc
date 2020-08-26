#include "imageplanes.hh"


/*********************
 *
 * Construct ImagePlanes Raw video data container.
*  Constructs a high-patterned border around each video plane so that 
 *  'blocks' overlapping the border won't match proper blocks inside.
 * This is useful for
 * motion estimation routines that, for speed, are a little
 * sloppy about some of the candidates they consider.
 *
 ********************/
ImagePlanes::ImagePlanes( EncoderParams &encparams )
{
    for( int c = 0; c < NUM_PLANES; ++c )
    { 
        // Allocate storage for image data
        switch( c )
        {
            case 0 : // Y plane
                planes[c] = new uint8_t[encparams.lum_buffer_size];
                BorderMark( planes[c] ,
                            encparams.enc_width,encparams.enc_height,
                            encparams.phy_width,encparams.phy_height);
                break;
            case 1 : // U plane
            case 2 :  // V plane
                planes[c] = new uint8_t[encparams.chrom_buffer_size];
                BorderMark( planes[c],
                            encparams.enc_chrom_width, encparams.enc_chrom_height,
                            encparams.phy_chrom_width,encparams.phy_chrom_height);
                break;
            default : // TODO: shift Y subsampled data from appended in Y buffer to seperate planes
                planes[c] = 0;
                break;
        }
    }
}


 ImagePlanes::~ImagePlanes()
{
    for( int c = 0; c < NUM_PLANES; ++c )
    { 
        if( planes[c] != 0 )
            delete [] planes[c];
    }
}


void ImagePlanes::BorderMark( uint8_t *frame,  
                              int total_width, int total_height,
                              int image_data_width, int image_data_height)
{
    int i, j;
    uint8_t *fp;
    uint8_t mask = 0xff;
    /* horizontal pixel replication (right border) */
 
    for (j=0; j<total_height; j++)
    {
        fp = frame + j*image_data_width;
        for (i=total_width; i<image_data_width; i++)
        {
            fp[i] = mask;
            mask ^= 0xff;
        }
    }
 
    /* vertical pixel replication (bottom border) */

    for (j=total_height; j<image_data_height; j++)
    {
        fp = frame + j*image_data_width;
        for (i=0; i<image_data_width; i++)
        {
            fp[i] = mask;
            mask ^= 0xff;
        }
    }
}
