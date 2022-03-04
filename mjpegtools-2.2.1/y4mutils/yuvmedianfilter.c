/*
 *    Copyright (C) 2001 Mike Bernson <mike@mlb.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Filter Based on code from Jim Cassburi filter: 2dclean
 *
 *    This filter look around the current point for a radius and averages
 *    this values that fall inside a threshold.
 */
#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "yuv4mpeg.h"
#include "mjpeg_logging.h"
#include "cpu_accel.h"

#ifdef HAVE_ASM_MMX
#include "mmx.h"
#endif


// must be less than 24
#define DIVISORBITS 20

int verbose = 1, domean8 = 0;
uint8_t	*input_frame[3];
uint8_t	*output_frame[3];

void	filter(int width, int height, uint8_t *const input[], uint8_t *output[]);
void	filter_buffer(int width, int height, int stride, int radius, int threshold, uint8_t * const input, uint8_t * const output);
void	filter_buffer_fast(int width, int height, int stride, int radius, int threshold, uint8_t * const input, uint8_t * const output);

int	threshold_luma = 2;
int	threshold_chroma = 2;

int	radius_luma = 2;
int	radius_chroma = 2;
int	interlace = -1;
int param_skip = 0;
int param_fast = 0;
int param_weight_type = 0;	/* 0 = use param_weight, 1 = 8, 2 = 2.667,
							   3 = 13.333, 4 = 24 */
double param_weight = -1.0;
double cutoff = 0.3333333;
#define	NUMAVG	1024
unsigned long avg_replace[NUMAVG];
int divisor[NUMAVG],divoffset[NUMAVG];

int	chroma_mode;
int     SS_H = 2;
int     SS_V = 2;

static void Usage(char *name )
{
	fprintf(stderr,
		"Usage: %s: [-h] [-r num] [-R num] [-t num] [-T num] [-c cutoff] [-v num]\n"
                "-h   - Print out this help\n"
		"-r   - Radius for luma median (default: 2 pixels)\n"
		"-R   - Radius for chroma median (default: 2 pixels)\n"
		"-t   - Trigger luma threshold (default: 2 [0=disable])\n"
		"-T   - Trigger chroma threshold (default: 2 [0=disable])\n"
		"-I   - Interlacing 0=off 1=on (default: taken from yuv stream)\n"
		"-f   - Fast mode (i.e. no trigger threshold, just simple mean)\n"
		"-w   - Weight given to current pixel vs. pixel in radius (default: 8)\n"
                "-c   - Fraction of pixels that must be within threshold (default: 0.333)\n"
		"-v   - Verbosity [0..2]\n", name);
}
			
int
main(int argc, char *argv[])
{
	int	i;
	long long avg, total;
	int	input_fd = 0;
	int	output_fd = 1;
	int	horz;
	int	vert;
	int	c;
	int	frame_count;

	y4m_stream_info_t istream, ostream;
	y4m_frame_info_t iframe;

	y4m_accept_extensions(1);

	while((c = getopt(argc, argv, "r:R:t:T:v:S:hI:w:fc:")) != EOF) {
		switch(c) {
		case 'r':
			radius_luma = atoi(optarg);
			break;
		case 'R':
			radius_chroma = atoi(optarg);
			break;
		case 't':
			threshold_luma = atoi(optarg);
			break;
		case 'T':
			threshold_chroma = atoi(optarg);
			break;
		case 'I':
			interlace = atoi (optarg);
			if (interlace != 0 && interlace != 1)
			{
				Usage (argv[0]);
				exit (1);
			}
			break;
		case 'S':
			param_skip = atoi (optarg);
			break;
		case 'f':
			param_fast = 1;
			break;
		case 'w':
			if (strcmp (optarg, "8") == 0)
				param_weight_type = 1;
			else if (strcmp (optarg, "2.667") == 0)
				param_weight_type = 2;
			else if (strcmp (optarg, "13.333") == 0)
				param_weight_type = 3;
			else if (strcmp (optarg, "24") == 0)
				param_weight_type = 4;
			else
				param_weight_type = 0;
			param_weight = atof (optarg);
			break;
                case 'c':
                        cutoff = atof(optarg);
                        break;
		case 'v':
			verbose = atoi (optarg);
			if (verbose < 0 || verbose >2)
			{
				Usage (argv[0]);
				exit (1);
			}
			break;		  
			
		case 'h':
                        Usage (argv[0]);
		default:
			exit(0);
		}
	}

        if( param_weight < 0 ) {
            if( param_fast )
                param_weight = 8.0;
            else
                param_weight = 1.0;
        }

        for( i=1; i<NUMAVG; i++ ) {
            avg_replace[i]=0;
            divisor[i]=((1<<DIVISORBITS)+(i>>1))/i;
            divoffset[i]=divisor[i]*(i>>1)+(divisor[i]>>1);
        }

#ifdef HAVE_ASM_MMX
        if( cpu_accel() & ACCEL_X86_MMXEXT )
            domean8=1;
#endif

	mjpeg_info ("fast %d, weight type %d\n", param_fast,
		param_weight_type);

	if (radius_luma <= 0 || radius_chroma <= 0)
	   mjpeg_error_exit1("radius values must be > 0!");

	if (threshold_luma < 0 || threshold_chroma < 0)
	   mjpeg_error_exit1("threshold values must be >= 0!");

   (void)mjpeg_default_handler_verbosity(verbose);

	y4m_init_stream_info(&istream);
	y4m_init_stream_info(&ostream);
	y4m_init_frame_info(&iframe);

	i = y4m_read_stream_header(input_fd, &istream);
	if (i != Y4M_OK)
	  mjpeg_error_exit1("Input stream error: %s", y4m_strerr(i));

	if (y4m_si_get_plane_count(&istream) != 3)
	   mjpeg_error_exit1("Only 3 plane formats supported");

	chroma_mode = y4m_si_get_chroma(&istream);
	SS_H = y4m_chroma_ss_x_ratio(chroma_mode).d;
	SS_V = y4m_chroma_ss_y_ratio(chroma_mode).d;

	mjpeg_debug("chroma subsampling: %dH %dV\n",SS_H,SS_V);

	if (interlace == -1)
	{
	  i = y4m_si_get_interlace(&istream);
	  switch (i)
	  {
	  case Y4M_ILACE_NONE:
	       interlace = 0;
	       break;
	  case Y4M_ILACE_BOTTOM_FIRST:
	  case Y4M_ILACE_TOP_FIRST:
	       interlace = 1;
	       break;
	  default:
	       mjpeg_warn("Unknown interlacing '%d', assuming non-interlaced", i);
	       interlace = 0;
	       break;
	  }
	}

	if( interlace && y4m_si_get_height(&istream) % 2 != 0 )
		mjpeg_error_exit1("Input images have odd number of lines - can't treats as interlaced!" );

	horz = y4m_si_get_width(&istream);
	vert = y4m_si_get_height(&istream);
	mjpeg_debug("width=%d height=%d luma_r=%d chroma_r=%d luma_t=%d chroma_t=%d", horz, vert, radius_luma, radius_chroma, threshold_luma, threshold_chroma);

	y4m_copy_stream_info(&ostream, &istream);

	input_frame[0] = malloc(horz * vert);
	input_frame[1] = malloc((horz / SS_H) * (vert / SS_V));
	input_frame[2] = malloc((horz / SS_H) * (vert / SS_V));

	output_frame[0] = malloc(horz * vert);
	output_frame[1] = malloc((horz / SS_H) * (vert / SS_V));
	output_frame[2] = malloc((horz / SS_H) * (vert / SS_V));


	y4m_write_stream_header(output_fd, &ostream);

	frame_count = 0;
	while (y4m_read_frame(input_fd, &istream, &iframe, input_frame) == Y4M_OK)
	{ 
		frame_count++;
		if (frame_count > param_skip)
		{
		  filter(horz, vert,  input_frame, output_frame);
		  y4m_write_frame(output_fd, &ostream, &iframe, output_frame);
		}
		else
		  y4m_write_frame(output_fd, &ostream, &iframe, input_frame);
	}

	for (total=0, avg=0, i=0; i < NUMAVG; i++) {
		total += avg_replace[i];
                avg   += avg_replace[i] * i; 
        }
	mjpeg_info("frames=%d avg=%3.1f", frame_count, ((double)avg)/((double)total));

	for (i=0; i < NUMAVG; i++) {
		mjpeg_debug( "%02d: %6.2f", i,
			(((double)avg_replace[i]) * 100.0)/(double)(total));
	}

	y4m_fini_stream_info(&istream);
	y4m_fini_stream_info(&ostream);
	y4m_fini_frame_info(&iframe);
	exit(0);
}

void
filter(int width, int height, uint8_t * const input[], uint8_t *output[])
{
	if (param_fast)
	{
		if( interlace )
		{
			filter_buffer_fast(width, height/2, width*2, 
						  radius_luma, threshold_luma, 
						  input[0], output[0]);
			filter_buffer_fast(width, height/2, width*2, 
						  radius_luma, threshold_luma, 
						  input[0]+width, output[0]+width);
	
			filter_buffer_fast(width/SS_H, (height/SS_V)/2, (width/SS_H)*2, 
						  radius_chroma, threshold_chroma, 
						  input[1], output[1]);
			filter_buffer_fast(width/SS_H, (height/SS_V)/2, (width/SS_H)*2, 
						  radius_chroma, threshold_chroma, 
						  input[1]+width/SS_H, output[1]+width/SS_H);
	
			filter_buffer_fast(width/SS_H, (height/SS_V)/2, (width/SS_H)*2, 
						  radius_chroma, threshold_chroma, 
						  input[2], output[2]);
			filter_buffer_fast(width/SS_H, (height/SS_V)/2, (width/SS_H)*2,
						  radius_chroma, threshold_chroma, 
						  input[2]+width/SS_H, output[2]+width/SS_H);
		}
		else
		{
			filter_buffer_fast(width, height, width, 
							  radius_luma, threshold_luma, 
							  input[0], output[0]);
			filter_buffer_fast(width/SS_H, height/SS_V, width/SS_H, 
						  radius_chroma, threshold_chroma, 
						  input[1], output[1]);
			filter_buffer_fast(width/SS_H, height/SS_V, width/SS_H, 
						  radius_chroma, threshold_chroma, 
						  input[2], output[2]);
		}
	}
	else
	{
		if( interlace )
		{
			filter_buffer(width, height/2, width*2, 
						  radius_luma, threshold_luma, 
						  input[0], output[0]);
			filter_buffer(width, height/2, width*2, 
						  radius_luma, threshold_luma, 
						  input[0]+width, output[0]+width);
	
			filter_buffer(width/SS_H, (height/SS_V)/2, (width/SS_H)*2, 
						  radius_chroma, threshold_chroma, 
						  input[1], output[1]);
			filter_buffer(width/SS_H, (height/SS_V)/2, (width/SS_H)*2, 
						  radius_chroma, threshold_chroma, 
						  input[1]+width/SS_H, output[1]+width/SS_H);
	
			filter_buffer(width/SS_H, (height/SS_V)/2, (width/SS_H)*2, 
						  radius_chroma, threshold_chroma, 
						  input[2], output[2]);
			filter_buffer(width/SS_H, (height/SS_V)/2, (width/SS_H)*2,
						  radius_chroma, threshold_chroma, 
						  input[2]+width/SS_H, output[2]+width/SS_H);
		}
		else
		{
			filter_buffer(width, height, width, 
							  radius_luma, threshold_luma, 
							  input[0], output[0]);
			filter_buffer(width/SS_H, height/SS_V, width/SS_H, 
						  radius_chroma, threshold_chroma, 
						  input[1], output[1]);
			filter_buffer(width/SS_H, height/SS_V, width/SS_H, 
						  radius_chroma, threshold_chroma, 
						  input[2], output[2]);
		}
	}
}

#ifdef HAVE_ASM_MMX
static inline void mean8(unsigned char *refpix,unsigned char *pixel,int radius_count,int row_stride,int threshold,int8_t *diff,unsigned char *count)
{
    int a,b;

    pxor_r2r(mm6,mm6); // mm6 (aka count) = 0
    pxor_r2r(mm7,mm7); // mm7 (aka diff) = 0
    movq_m2r(*refpix,mm3); // mm3 = refpix[0]

    movd_g2r(0x80808080,mm4); // mm4 = 128
    punpcklbw_r2r(mm4,mm4);

    pxor_r2r(mm4,mm3); // mm3 = refpix[0]-128

    movd_g2r(threshold,mm5); // mm5 = threshold
    punpcklbw_r2r(mm5,mm5);
    punpcklbw_r2r(mm5,mm5);
    punpcklbw_r2r(mm5,mm5);

    for( b=0; b<radius_count; b++ ) {
        for( a=0; a<radius_count; a++ ) {
            movq_m2r(*pixel,mm0); // mm0  = pixel[0]
            pxor_r2r(mm4,mm0);    // mm0  = pixel[0]-128
            movq_r2r(mm3,mm2);    // mm2  = refpix[0]-128
            psubsb_r2r(mm0,mm2);  // mm2  = refpix[0]-pixel[0]
            psubsb_r2r(mm3,mm0);  // mm0  = pixel[0]-refpix[0]
            pminub_r2r(mm0,mm2);  // mm2  = abs(pixel[0]-refpix[0])
            movq_r2r(mm5,mm1);    // mm1  = threshold
            pcmpgtb_r2r(mm2,mm1); // mm1  = (threshold > abs(pixel[0]-refpix[0])) ? -1 : 0
            psubb_r2r(mm1,mm6);   // mm6 += (threshold > abs(pixel[0]-refpix[0]))
            pand_r2r(mm1,mm0);    // mm0  = (threshold > abs(pixel[0]-refpix[0])) ? pixel[0]-refpix[0] : 0
            paddb_r2r(mm0,mm7);   // mm7 += (threshold > abs(pixel[0]-refpix[0])) ? pixel[0]-refpix[0] : 0

            ++pixel;
        }
        pixel += row_stride - radius_count;
    }

    movq_r2m(mm6,*count);
    movq_r2m(mm7,*diff);

    emms();
 
}
#endif

static inline void mean1(unsigned char *refpix,unsigned char *pixel,int radius_count,int row_stride,int threshold,int8_t *diff,unsigned char *count)
{
    int reference = *refpix;
    int total = 0;
    int cnt = 0;
    int a,b;
    for( b = radius_count; b > 0; --b ) {
        for( a = radius_count; a > 0; --a ) {
            int d = *pixel - reference;
            if (d < threshold && d > -threshold)
            {
                total += d;
                cnt++;
            }
            ++pixel;
        }
        pixel += (row_stride - radius_count);
    }
    *diff = total;
    *count = cnt;
}

static inline void tally(unsigned char *outpix,unsigned char *refpix,int total,int count,int min_count,int row_stride)
{
    ++avg_replace[count];

    /*
     * If we don't have enough samples to make a decent
     * pseudo-median use a simple mean
     */
    if (count <= min_count)
    {
        *outpix =  
            ( ( (refpix[-row_stride-1] + refpix[-row_stride]) + 
                (refpix[-row_stride+1] +  refpix[-1]) 
                ) 
              + 
              ( ((refpix[0]<<3) + 8 + refpix[1]) +
                (refpix[row_stride-1] + refpix[row_stride]) + 
                refpix[row_stride+1]
                  )
                ) >> 4;
    } else {
        count += param_weight - 1;
        //*outpix = (refpix[0]*count + total + count/2) / count;
        *outpix = refpix[0] + ((total * divisor[count] + divoffset[count])>>DIVISORBITS);
    }
}

void
filter_buffer(int width, int height, int row_stride,
			  int radius, int threshold, uint8_t * const input, uint8_t * const output)
{
	int	radius_count;
	int	x;
	int	y;
	int	offset;
	int	min_count;
	uint8_t *inpix, *refpix;
	uint8_t *outpix;
        static uint8_t count[8];
        static int8_t total[8];
	radius_count = radius + radius + 1;
	min_count = ceil((radius_count * radius_count) * cutoff);
	

	if (threshold == 0)
	   {
           for (y = 0; y < height; y++)
	       memcpy(&output[y * row_stride], &input[y * row_stride], width);
	   return;
           }

	for(y=0; y < radius; y++)
		memcpy(&output[y * row_stride], &input[y * row_stride], width);

	for(y=height - radius; y < height; y++)
		memcpy(&output[y* row_stride], &input[y*row_stride], width);
	
	inpix = &input[0];
	outpix = &output[0];
	for(y=0; y < height; ++y)
	{
		x = 0;
		while(x<radius)
		{
			outpix[x] = inpix[x];
			++x;
			outpix[width-x] = inpix[width-x];
		}
		inpix += row_stride;
		outpix += row_stride;
	}

	offset = radius*row_stride+radius;	/* Offset top-left of processing */
	                                /* Window to its centre */
	refpix = &input[offset];
	outpix = &output[offset];
	for(y=radius; y < height-radius; y++)
	{
            x=radius;
#ifdef HAVE_ASM_MMX
            if( domean8 ) {
                for(; x < width - radius - 7; x+=8)
                {
                    int i;

                    mean8(refpix,refpix-offset,radius_count,row_stride,threshold,total,count);
                    for( i=0; i<8; i++ ) {
                        tally(outpix,refpix,total[i],count[i],min_count,row_stride);
                        ++refpix;
                        ++outpix;
                    }
                }
            }
#endif
            for(; x < width - radius; x++)
            {
                mean1(refpix,refpix-offset,radius_count,row_stride,threshold,total,count);
                tally(outpix,refpix,total[0],count[0],min_count,row_stride);
                ++refpix;
                ++outpix;
            }
            refpix += (row_stride-width+(radius*2));
            outpix += (row_stride-width+(radius*2));
	}
}

void
filter_buffer_fast(int width, int height, int row_stride,
			  int radius, int threshold, uint8_t * const input, uint8_t * const output)
{
	int	a;
	int	b;
	uint8_t *pixel;
	int	total;
	int	count;
	int	radius_count;
	int	x;
	int	y;
	int	offset;
	uint8_t *refpix;
	uint8_t *outpix;
	int fasttype;

	/* If no filtering should be done, just copy data and leave. */
	if (threshold == 0)
	   {
           for (y = 0; y < height; y++)
	       memcpy(&output[y * row_stride], &input[y * row_stride], width);
	   return;
           }

	/* Calculate the number of pixels from one extreme of the radius
	   to the other extreme. */
	radius_count = radius + radius + 1;

	/* Figure out which optimized filtering algorithm to use, if any. */
	if (radius == 1 && param_weight_type == 2)
		fasttype = 1;
	else if (radius == 1 && param_weight_type == 1)
		fasttype = 2;
	else if (radius == 2 && param_weight_type == 1)
		fasttype = 3;
	else if (radius == 1 && param_weight_type == 3)
		fasttype = 4;
	else if (radius == 1 && param_weight_type == 4)
		fasttype = 5;
	else
		fasttype = 0;

	/* Copy top rows of picture, without filtering. */
	for(y=0; y < radius; y++)
		memcpy(&output[y * row_stride], &input[y * row_stride], width);

	/* Copy bottom rows of picture, without filtering. */
	for(y=height - radius; y < height; y++)
		memcpy(&output[y* row_stride], &input[y*row_stride], width);
	
	/* Copy leftmost/righmost columns of picture, without filtering. */
	refpix = &input[0];
	outpix = &output[0];
	for(y=0; y < height; ++y)
	{
		a = 0;
		while(a<radius)
		{
			outpix[a] = refpix[a];
			++a;
			outpix[width-a] = refpix[width-a];
		}
		refpix += row_stride;
		outpix += row_stride;
	}

	/* Find the top-left corner of the portion of the image that
	   will be filtered. */
	offset = radius*row_stride+radius;	
	refpix = &input[offset];
	outpix = &output[offset];

	/* Loop through the portion of the image to be filtered.
	   Use a simple mean, but process certain combinations of
	   radius/weight pairs more efficiently.  Note that adding
	   half the pixel count to the pixel value accomplishes
	   rounding the pixel value to the nearest whole value. */
	switch (fasttype)
	{
	case 1:
		for(y=radius; y < height-radius; y++)
		{
			for(x=radius; x < width - radius; x++)
			{
				/* Radius 1, weight 2.667.  The 8 pixels surrounding
				   the current one have a weight of 3, the current
				   pixel has a weight of 8, for a total weight of
				   8*3+8 = 32. */
				*outpix = ((3 * ((int)refpix[-row_stride-1]
					+ (int)refpix[-row_stride]
					+ ((int)refpix[-row_stride+1]
					+  (int)refpix[-1]) 
				  + (int)refpix[1]
				  + (int)refpix[row_stride-1]
				  + (int)refpix[row_stride]
				  + (int)refpix[row_stride+1]))
					+ (((int)refpix[0])<<3) + 16) >> 5;
				++avg_replace[9];
				++refpix;
				++outpix;
			}
			refpix += (row_stride-width+(radius*2));
			outpix += (row_stride-width+(radius*2));
		}
		break;

	case 2:
		for(y=radius; y < height-radius; y++)
		{
			for(x=radius; x < width - radius; x++)
			{
				/* Radius 1, weight 8.  The 8 pixels surrounding the
				   current one have a weight of 1, the current pixel has
				   a weight of 8, for a total weight of 8*1+8 = 16. */
				*outpix = (((int)refpix[-row_stride-1]
					+ (int)refpix[-row_stride]
					+ (int)refpix[-row_stride+1]
					+ (int)refpix[-1]
					+ (int)refpix[1]
					+ (int)refpix[row_stride-1]
					+ (int)refpix[row_stride]
					+ (int)refpix[row_stride+1])
					+ (((int)refpix[0])<<3) + 8) >> 4;
				++avg_replace[9];
				++refpix;
				++outpix;
			}
			refpix += (row_stride-width+(radius*2));
			outpix += (row_stride-width+(radius*2));
		}
		break;

	case 3:
		for(y=radius; y < height-radius; y++)
		{
			for(x=radius; x < width - radius; x++)
			{
				/* Radius 2, weight 8.  The 24 pixels surrounding the
				   current one have a weight of 1, the current pixel
				   has a weight of 8, for a total weight of
				   24*1+8 = 32. */
				*outpix = (((int)refpix[-row_stride-row_stride-2]
					+ (int)refpix[-row_stride-row_stride-1]
					+ (int)refpix[-row_stride-row_stride]
					+ (int)refpix[-row_stride-row_stride+1]
					+ (int)refpix[-row_stride-row_stride+2]
					+ (int)refpix[-row_stride-2]
					+ (int)refpix[-row_stride-1]
					+ (int)refpix[-row_stride]
					+ (int)refpix[-row_stride+1]
					+ (int)refpix[-row_stride+2]
					+ (int)refpix[-2] 
					+ (int)refpix[-1] 
					+ (int)refpix[1] 
					+ (int)refpix[2] 
					+ (int)refpix[row_stride-2]
					+ (int)refpix[row_stride-1]
					+ (int)refpix[row_stride]
					+ (int)refpix[row_stride+1]
					+ (int)refpix[row_stride+2]
					+ (int)refpix[row_stride+row_stride-2]
					+ (int)refpix[row_stride+row_stride-1]
					+ (int)refpix[row_stride+row_stride]
					+ (int)refpix[row_stride+row_stride+1]
					+ (int)refpix[row_stride+row_stride+2])
					+ (((int)refpix[0])<<3) + 16) >> 5;
				++avg_replace[25];
				++refpix;
				++outpix;
			}
			refpix += (row_stride-width+(radius*2));
			outpix += (row_stride-width+(radius*2));
		}
		break;

	case 4:
		for(y=radius; y < height-radius; y++)
		{
			for(x=radius; x < width - radius; x++)
			{
				/* Radius 1, weight 13.333.  The 8 pixels surrounding
				   the current one have a weight of 3, the current
				   pixel has a weight of 40, for a total weight of
				   8*3+40 = 64. */
				*outpix = ((3 * ((int)refpix[-row_stride-1]
					+ (int)refpix[-row_stride]
					+ ((int)refpix[-row_stride+1]
					+  (int)refpix[-1]) 
				  + (int)refpix[1]
				  + (int)refpix[row_stride-1]
				  + (int)refpix[row_stride]
				  + (int)refpix[row_stride+1]))
					+ (((int)refpix[0])*40) + 32) >> 6;
				++avg_replace[9];
				++refpix;
				++outpix;
			}
			refpix += (row_stride-width+(radius*2));
			outpix += (row_stride-width+(radius*2));
		}
		break;

	case 5:
		for(y=radius; y < height-radius; y++)
		{
			for(x=radius; x < width - radius; x++)
			{
				/* Radius 1, weight 24.  The 8 pixels surrounding the
				   current one have a weight of 1, the current pixel has
				   a weight of 24, for a total weight of 8*1+24 = 32. */
				*outpix = (((int)refpix[-row_stride-1]
					+ (int)refpix[-row_stride]
					+ (int)refpix[-row_stride+1]
					+ (int)refpix[-1]
					+ (int)refpix[1]
					+ (int)refpix[row_stride-1]
					+ (int)refpix[row_stride]
					+ (int)refpix[row_stride+1])
					+ (((int)refpix[0])*24) + 16) >> 5;
				++avg_replace[9];
				++refpix;
				++outpix;
			}
			refpix += (row_stride-width+(radius*2));
			outpix += (row_stride-width+(radius*2));
		}
		break;

	default:
		for(y=radius; y < height-radius; y++)
		{
			for(x=radius; x < width - radius; x++)
			{
				/* Reset the total pixel value and the count of pixels
				   found in the given radius. */
				total = 0;
				count = 0;
	
				/* Now loop through all pixels in a radius around the
				   current one, and add up their values, skipping the
				   current pixel. */
				pixel = refpix-offset;
				b = radius_count;
				while( b > 0 )
				{
					a = radius_count;
					--b;
					while( a > 0 )
					{
						--a;
						if (pixel != refpix)
						{
							total += *pixel;
							count++;
						}
						++pixel;
					}
					pixel += (row_stride - radius_count);
				}
				++avg_replace[count];

				/* Finally, calculate the pixel's new value. */
				*outpix = (total + (refpix[0] * param_weight)
					+ (count >> 1)) / (count + param_weight);

				++refpix;
				++outpix;
			}
			refpix += (row_stride-width+(radius*2));
			outpix += (row_stride-width+(radius*2));
		}
		break;
	}
}
