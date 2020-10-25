/* 
 * $Id: y4munsharp.c,v 1.11 2009/09/19 19:01:43 sms00 Exp $
 *
 * Constructed using:
 * unsharp.c 0.10 -- This is a plug-in for the GIMP 1.0
 * Copyright (C) 1999 Winston Chang <winstonc@cs.wisc.edu>/<winston@stdout.org>
 * 
 *
 * Rewritten/modified from the GIMP unsharp plugin into y4munsharp by
 * Steven Schultz
 *
 * 2004/11/10 - used the core functions of the unsharp plugin and wrote a 
 *       y4m filter program.  The original plugin was allocated/deallocated
 *       memory for each picture/frame and of course that had to be done in a
 *       more efficient manner.  Additional work involved handling interlaced
 *       input for the column blur function.  
 *
 *       By default only the LUMA (Y') is processed.  Processing the CHROMA 
 *       (CbCr) can, in some cases, result in color shifts where the edge
 *       enhancement is done.  If it is desired to process the CHROMA planes 
 *       -C option must be given explicitly.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <yuv4mpeg.h>

#define MAX(a,b) ((a) >= (b) ? (a) : (b))
#define ROUND(x) ((int) ((x) + 0.5))

void usage(char *);
void y4munsharp(void);
static void get_column(u_char *, u_char *, int, int);
static void put_column(u_char *, u_char *, int, int);
static void blur_line (double *, double *, int, u_char *, u_char *, int );
static double *gen_lookup_table (double *, int);
static int gen_convolve_matrix(double, double **);

	u_char	*i_yuv[3], *o_yuv[3], *cur_col, *dest_col, *cur_row, *dest_row;

	double	y_radius = 2.0, y_amount = 0.30;
	double	uv_radius = -1.0, uv_amount;
	int	y_threshold = 4, uv_threshold, frameno;

	int	interlaced, ywidth, uvwidth, yheight, uvheight, ylen, uvlen;
	int	cmatrix_y_len, cmatrix_uv_len;
	double	*cmatrix_y, *cmatrix_uv, *ctable_y, *ctable_uv;

	int	lowy = 16, highy = 235, lowuv = 16, highuv = 240;

int
main(int argc, char **argv)
	{
	int	fdin, fdout, err, c, i, verbose = 1;
	y4m_stream_info_t istream, ostream;
	y4m_frame_info_t iframe;

	fdin = fileno(stdin);
	fdout = fileno(stdout);

	y4m_accept_extensions(1);
	y4m_init_stream_info(&istream);
	y4m_init_frame_info(&iframe);

	while	((c = getopt(argc, argv, "L:C:hv:N")) != EOF)
		{
		switch	(c)
			{
			case	'N':
				lowuv = lowy = 0;
				lowuv = highy = 255;
				break;
			case	'L':
				i = sscanf(optarg, "%lf,%lf,%d", &y_radius, 
						&y_amount, &y_threshold);
				if	(i != 3)
					{
					mjpeg_error("-L r,a,t");
					usage(argv[0]);
					}
				break;
			case	'C':
				i = sscanf(optarg, "%lf,%lf,%d", &uv_radius,
						&uv_amount, &uv_threshold);
				if	(i != 3)
					{
					mjpeg_error("-C r,a,t");
					usage(argv[0]);
					}
				break;
			case	'v':
				verbose = atoi(optarg);
				if	(verbose < 0 || verbose > 2)
					mjpeg_error_exit1("-v 0|1|2");
				break;
			case	'h':
			default:
				usage(argv[0]);
				break;
			}
		}

	if	(isatty(fdout))
		mjpeg_error_exit1("stdout must not be a terminal");

	mjpeg_default_handler_verbosity(verbose);

	err = y4m_read_stream_header(fdin, &istream);
	if	(err != Y4M_OK)
		mjpeg_error_exit1("Couldn't read input stream header");

	switch	(y4m_si_get_interlace(&istream))
		{
		case	Y4M_ILACE_NONE:
			interlaced = 0;
			break;
		case	Y4M_ILACE_BOTTOM_FIRST:
		case	Y4M_ILACE_TOP_FIRST:
			interlaced = 1;
			break;
		default:
			mjpeg_error_exit1("Unsupported/unknown interlacing");
		}

	if	(y4m_si_get_plane_count(&istream) != 3)
		mjpeg_error_exit1("Only 3 plane formats supported");

	yheight = y4m_si_get_plane_height(&istream, 0);
	uvheight = y4m_si_get_plane_height(&istream, 1);
	ywidth = y4m_si_get_plane_width(&istream, 0);
	uvwidth = y4m_si_get_plane_width(&istream, 1);
	ylen = y4m_si_get_plane_length(&istream, 0);
	uvlen = y4m_si_get_plane_length(&istream, 1);

/* Input and output frame buffers */
	i_yuv[0] = (u_char *)malloc(ylen);
	i_yuv[1] = (u_char *)malloc(uvlen);
	i_yuv[2] = (u_char *)malloc(uvlen);
	o_yuv[0] = (u_char *)malloc(ylen);
	o_yuv[1] = (u_char *)malloc(uvlen);
	o_yuv[2] = (u_char *)malloc(uvlen);

/*
 * general purpose row/column scratch buffers.  Slightly over allocated to
 * simplify life.
*/
	cur_col = (u_char *)malloc(MAX(ywidth, yheight));
	dest_col = (u_char *)malloc(MAX(ywidth, yheight));
	cur_row = (u_char *)malloc(MAX(ywidth, yheight));
	dest_row = (u_char *)malloc(MAX(ywidth, yheight));

/*
 * Generate the convolution matrices.  The generation routine allocates the
 * memory and returns the length.
*/
	cmatrix_y_len = gen_convolve_matrix(y_radius, &cmatrix_y);
	cmatrix_uv_len = gen_convolve_matrix(uv_radius, &cmatrix_uv);
	ctable_y = gen_lookup_table(cmatrix_y, cmatrix_y_len);
	ctable_uv = gen_lookup_table(cmatrix_uv, cmatrix_uv_len);

	y4m_init_stream_info(&ostream);
	y4m_copy_stream_info(&ostream, &istream);
	y4m_write_stream_header(fileno(stdout), &ostream);

	mjpeg_info("Luma radius: %f", y_radius);
	mjpeg_info("Luma amount: %f", y_amount);
	mjpeg_info("Luma threshold: %d", y_threshold);
	if	(uv_radius != -1.0)
		{
		mjpeg_info("Chroma radius: %f", uv_radius);
		mjpeg_info("Chroma amount: %f", uv_amount);
		mjpeg_info("Chroma threshold: %d", uv_threshold);
		}

	for	(frameno = 0; y4m_read_frame(fdin, &istream, &iframe, i_yuv) == Y4M_OK; frameno++)
		{
		y4munsharp();
		err = y4m_write_frame(fdout, &ostream, &iframe, o_yuv);
		if	(err != Y4M_OK)
			{
			mjpeg_error("y4m_write_frame err at frame %d", frameno);
			break;
			}
		}
	y4m_fini_frame_info(&iframe);
	y4m_fini_stream_info(&istream);
	y4m_fini_stream_info(&ostream);
	exit(0);
	}

/*
 * Uses the globals defined above - probably not the best practice in the world
 * but beats passing a jillion arguments or going to the effort of 
 * encapsulation.
*/

void y4munsharp(void)
	{
	int	i, row, col, diff, value;
	u_char	*i_ptr, *o_ptr;

	mjpeg_debug("Blurring Luma rows frame %d", frameno);

	for	(row = 0; row < yheight; row++)
		{
		blur_line(ctable_y, cmatrix_y, cmatrix_y_len, 
			&i_yuv[0][row * ywidth],
			&o_yuv[0][row * ywidth],
			ywidth);
		}

	if	(uv_radius != -1.0)
		{
		mjpeg_debug("Blurring Chroma rows frame %d", frameno);
		for	(row = 0; row < uvheight; row++)
			{
			blur_line(ctable_uv, cmatrix_uv, cmatrix_uv_len,
				&i_yuv[1][row * uvwidth],
				&o_yuv[1][row * uvwidth],
				uvwidth);
			blur_line(ctable_uv, cmatrix_uv, cmatrix_uv_len,
				&i_yuv[2][row * uvwidth],
				&o_yuv[2][row * uvwidth],
				uvwidth);
			}
		}
	else
		{
		memcpy(o_yuv[1], i_yuv[1], uvlen);
		memcpy(o_yuv[2], i_yuv[2], uvlen);
		}

	mjpeg_debug("Blurring Luma columns frame %d", frameno);
	for	(col = 0; col < ywidth; col++)
		{
/*
 * Do the entire frame if progressive, otherwise this does the only
 * the first field.
*/
		get_column(&o_yuv[0][col], cur_col,
			interlaced ? 2 * ywidth : ywidth,
			interlaced ? yheight / 2 : yheight);
		blur_line(ctable_y, cmatrix_y, cmatrix_y_len,
			cur_col,
			dest_col,
			interlaced ? yheight / 2 : yheight);
		put_column(dest_col, &o_yuv[0][col],
			interlaced ? 2 * ywidth : ywidth,
			interlaced ? yheight / 2 : yheight);

/*
 * If interlaced now process the second field (data source is offset 
 * by 'ywidth').
*/
		if	(interlaced)
			{
			get_column(&o_yuv[0][col + ywidth], cur_col,
				2 * ywidth,
				yheight / 2);
			blur_line(ctable_y, cmatrix_y, cmatrix_y_len,
				cur_col,
				dest_col,
				interlaced ? yheight / 2 : yheight);
			put_column(dest_col, &o_yuv[0][col + ywidth],
				2 * ywidth,
				yheight / 2);
			}
		}

	if	(uv_radius == -1)
		goto merging;

	mjpeg_debug("Blurring chroma columns frame %d", frameno);
	for	(col = 0; col < uvwidth; col++)
		{
/* U */
		get_column(&o_yuv[1][col], cur_col,
			interlaced ? 2 * uvwidth : uvwidth,
			interlaced ? uvheight / 2 : uvheight);
		blur_line(ctable_uv, cmatrix_uv, cmatrix_uv_len,
			cur_col,
			dest_col,
			interlaced ? uvheight / 2 : uvheight);
		put_column(dest_col, &o_yuv[1][col],
			interlaced ? 2 * uvwidth : uvwidth,
			interlaced ? uvheight / 2 : uvheight);
		if	(interlaced)
			{
			get_column(&o_yuv[1][col + uvwidth], cur_col,
				2 * uvwidth,
				uvheight / 2);
			blur_line(ctable_uv, cmatrix_uv, cmatrix_uv_len,
				cur_col,
				dest_col,
				interlaced ? uvheight / 2 : uvheight);
			put_column(dest_col, &o_yuv[1][col + uvwidth],
				2 * uvwidth,
				uvheight / 2);
			}
/* V */
		get_column(&o_yuv[2][col], cur_col,
			interlaced ? 2 * uvwidth : uvwidth,
			interlaced ? uvheight / 2 : uvheight);
		blur_line(ctable_uv, cmatrix_uv, cmatrix_uv_len,
			cur_col,
			dest_col,
			interlaced ? uvheight / 2 : uvheight);
		put_column(dest_col, &o_yuv[2][col],
			interlaced ? 2 * uvwidth : uvwidth,
			interlaced ? uvheight / 2 : uvheight);
		if	(interlaced)
			{
			get_column(&o_yuv[2][col + uvwidth], cur_col,
				2 * uvwidth,
				uvheight / 2);
			blur_line(ctable_uv, cmatrix_uv, cmatrix_uv_len,
				cur_col,
				dest_col,
				interlaced ? uvheight / 2 : uvheight);
			put_column(dest_col, &o_yuv[2][col + uvwidth],
				2 * uvwidth,
				uvheight / 2);
			}
		}
merging:
	mjpeg_debug("Merging luma frame %d", frameno);
	for	(row = 0, i_ptr = i_yuv[0], o_ptr = o_yuv[0]; row < yheight; row++)
		{
		for	(i = 0; i < ywidth; i++, i_ptr++, o_ptr++)
			{
			diff = *i_ptr - *o_ptr;
			if	(abs(2 * diff) < y_threshold)
				diff = 0;
			value = *i_ptr + (y_amount * diff);
/*
 * For video the limits are 16 and 235 for the luma rather than 0 and 255!
*/
			if	(value < lowy)
				value = lowy;
			else if	(value > highy)
				value = highy;
			*o_ptr = value;
			}
		}

	if	(uv_radius == -1.0)
		goto done;

	mjpeg_debug("Merging chroma frame %d", frameno);
	for	(row = 0, i_ptr = i_yuv[1], o_ptr = o_yuv[1]; row < uvheight; row++)
		{
		for	(i = 0; i < uvwidth; i++, i_ptr++, o_ptr++)
			{
			diff = *i_ptr - *o_ptr;
			if	(abs(2 * diff) < uv_threshold)
				diff = 0;
			value = *i_ptr + (uv_amount * diff);
/*
 * For video the limits are 16 and 240 for the chroma rather than 0 and 255!
*/
			if	(value < lowuv)
				value = lowuv;
			else if	(value > highuv)
				value = highuv;
			*o_ptr = value;
			}
		}
	for	(row = 0, i_ptr = i_yuv[2], o_ptr = o_yuv[2]; row < uvheight; row++)
		{
		for	(i = 0; i < uvwidth; i++, i_ptr++, o_ptr++)
			{
			diff = *i_ptr - *o_ptr;
			if	(abs(2 * diff) < uv_threshold)
				diff = 0;
			value = *i_ptr + (uv_amount * diff);
/*
 * For video the limits are 16 and 240 for the chroma rather than 0 and 255!
*/
			if	(value < 16)
				value = 16;
			else if	(value > highuv)
				value = highuv;
			*o_ptr = value;
			}
		}
done:
	return;
	}

void
get_column(u_char *in, u_char *out, int stride, int numrows)
	{
	int	i;

	for	(i = 0; i < numrows; i++)
		{
		*out++ = *in;
		in += stride;
		}
	}

void
put_column(u_char *in, u_char *out, int stride, int numrows)
	{
	int	i;

	for	(i = 0; i < numrows; i++)
		{
		*out = *in++;
		out += stride;
		}
	}

/* 
 * The blur_line(), gen_convolve_matrix() and gen_lookup_table() functions 
 * were lifted almost intact from the GIMP unsharp plugin.  malloc was used 
 * instead of g_new() and the style was cleaned up a little but the logic 
 * was left untouched.
*/

/* this function is written as if it is blurring a column at a time,
 * even though it can operate on rows, too.  There is no difference
 * in the processing of the lines, at least to the blur_line function.
*/
static void
blur_line (double *ctable, double *cmatrix, int cmatrix_length,
	   u_char  *cur_col, u_char  *dest_col, int y)
  {
  double scale, sum, *cmatrix_p, *ctable_p;
  int i=0, j=0, row, cmatrix_middle = cmatrix_length/2;
  u_char  *cur_col_p, *cur_col_p1, *dest_col_p;

  /* this first block is the same as the non-optimized version --
   * it is only used for very small pictures, so speed isn't a
   * big concern.
   */
  if (cmatrix_length > y)
     {
     for (row = 0; row < y ; row++)
	 {
	 scale=0;
	  /* find the scale factor */
	 for  (j = 0; j < y ; j++)
	      {
	      /* if the index is in bounds, add it to the scale counter */
	      if  ((j + cmatrix_length/2 - row >= 0) &&
		   (j + cmatrix_length/2 - row < cmatrix_length))
		  scale += cmatrix[j + cmatrix_length/2 - row];
	      }
	  for  (i = 0; i< 1; i++)
	       {
	       sum = 0;
	       for  (j = 0; j < y; j++)
                    {
		    if  ((j >= row - cmatrix_length/2) &&
		         (j <= row + cmatrix_length/2))
		    	sum += cur_col[j + i] * cmatrix[j];
		    }
	        dest_col[row + i] = (u_char) ROUND (sum / scale);
	        }
	   }
      }
  else
      {
      /* for the edge condition, we only use available info and scale to one */
      for (row = 0; row < cmatrix_middle; row++)
	  {
	  /* find scale factor */
	  scale=0;
	  for  (j = cmatrix_middle - row; j<cmatrix_length; j++)
	       scale += cmatrix[j];
	  for  (i = 0; i < 1; i++)
	       {
	       sum = 0;
	       for  (j = cmatrix_middle - row; j<cmatrix_length; j++)
		    sum += cur_col[(row + j-cmatrix_middle) + i] * cmatrix[j];
	        dest_col[row + i] = (u_char) ROUND (sum / scale);
	       }
	   }
      /* go through each pixel in each col */
      dest_col_p = dest_col + row;
      for  (; row < y-cmatrix_middle; row++)
	   {
	   cur_col_p = (row - cmatrix_middle) + cur_col;
	   for  (i = 0; i < 1; i++)
	        {
	        sum = 0;
	        cmatrix_p = cmatrix;
	        cur_col_p1 = cur_col_p;
	        ctable_p = ctable;
	        for  (j = cmatrix_length; j>0; j--)
		     {
		     sum += *(ctable_p + *cur_col_p1);
		     cur_col_p1 += 1;
		     ctable_p += 256;
		     }
	        cur_col_p++;
	        *(dest_col_p++) = ROUND (sum);
	        }
	    }
	
      /* for the edge condition, we only use available info, and scale to one */
       for  (; row < y; row++)
	    {
	    /* find scale factor */
	    scale=0;
	    for  (j = 0; j< y-row + cmatrix_middle; j++)
	         scale += cmatrix[j];
	    for  (i = 0; i < 1; i++)
	         {
	         sum = 0;
	         for  (j = 0; j<y-row + cmatrix_middle; j++)
		      sum += cur_col[(row + j-cmatrix_middle) + i] * cmatrix[j];
	         dest_col[row + i] = (u_char) ROUND (sum / scale);
	         }
	    }
      }
  }

/*
 * generates a 1-D convolution matrix to be used for each pass of 
 * a two-pass gaussian blur.  Returns the length of the matrix.
*/
static int
gen_convolve_matrix(double radius, double **cmatrix_p)
	{
	int matrix_length, matrix_midpoint, i, j;
	double *cmatrix, std_dev, sum, base_x;
	
  /* we want to generate a matrix that goes out a certain radius
   * from the center, so we have to go out ceil(rad-0.5) pixels,
   * inlcuding the center pixel.  Of course, that's only in one direction,
   * so we have to go the same amount in the other direction, but not count
   * the center pixel again.  So we double the previous result and subtract
   * one.
   * The radius parameter that is passed to this function is used as
   * the standard deviation, and the radius of effect is the
   * standard deviation * 2.  It's a little confusing.
   */
	radius = fabs(radius) + 1.0;
	
	std_dev = radius;
	radius = std_dev * 2;

	/* go out 'radius' in each direction */
	matrix_length = 2 * ceil(radius-0.5) + 1;
	if (matrix_length <= 0) matrix_length = 1;
	matrix_midpoint = matrix_length/2 + 1;
	*cmatrix_p = (double *)malloc(sizeof (double) * matrix_length);
	cmatrix = *cmatrix_p;

  /*  Now we fill the matrix by doing a numeric integration approximation
   * from -2*std_dev to 2*std_dev, sampling 50 points per pixel.
   * We do the bottom half, mirror it to the top half, then compute the
   * center point.  Otherwise asymmetric quantization errors will occur.
   *  The formula to integrate is e^-(x^2/2s^2).
   */

  /* first we do the top (right) half of matrix */
	for	(i = matrix_length/2 + 1; i < matrix_length; i++)
		{
		base_x = i - floor(matrix_length/2) - 0.5;
		sum = 0;
		for	(j = 1; j <= 50; j++)
			{
			if	(base_x+0.02*j <= radius)
				sum += exp (-(base_x+0.02*j)*(base_x+0.02*j) / 
					(2*std_dev*std_dev));
			}
		cmatrix[i] = sum/50;
		}

  /* mirror the thing to the bottom half */
	for	(i=0; i<=matrix_length/2; i++)
		cmatrix[i] = cmatrix[matrix_length-1-i];
	
  /* find center val -- calculate an odd number of quanta to make it symmetric,
   * even if the center point is weighted slightly higher than others. */
	sum = 0;
	for	(j=0; j<=50; j++)
  		sum += exp (-(0.5+0.02*j)*(0.5+0.02*j) / (2*std_dev*std_dev));
	cmatrix[matrix_length/2] = sum/51;
	
  /* normalize the distribution by scaling the total sum to one */
	sum=0;
	for	(i = 0; i < matrix_length; i++)
		sum += cmatrix[i];
	for	(i=0; i<matrix_length; i++)
		cmatrix[i] = cmatrix[i] / sum;
	return(matrix_length);
	}

/* generates a lookup table for every possible product of 0-255 and
   each value in the convolution matrix.  The returned array is
   indexed first by matrix position, then by input multiplicand (?) value.
*/
static double *
gen_lookup_table(double *cmatrix, int cmatrix_length)
	{
	int i, j;
	double* lookup_table = (double *)malloc(sizeof (double) * cmatrix_length * 256);
	double* lookup_table_p = lookup_table, *cmatrix_p = cmatrix;

	for	(i=0; i<cmatrix_length; i++)
		{
		for	(j=0; j<256; j++)
	  		*(lookup_table_p++) = *cmatrix_p * (double)j;
		cmatrix_p++;
		}
	return(lookup_table);
	}

void usage(char *pgm)
	{
	fprintf(stderr, "%s: usage: [-v 0|1|2] [-N] [-L radius,amount,threshold] [-C radius,amount,threshold]\n", pgm);
	fprintf(stderr, "%s:\tradius and amount are floating point numbers\n",
		pgm);
	fprintf(stderr, "%s:\tthreshold is integer.\n", pgm);
	fprintf(stderr, "%s:\tdefault for -L is 2.0,0.3,4\n", pgm);
	fprintf(stderr, "%s:\tchroma not filtered UNLESS -C used, no default\n",
		pgm);
	fprintf(stderr, "%s:-v verbose 0=quiet 1=normal 2=debug (default: 1)\n", pgm);
	fprintf(stderr, "%s:-N disables the Y' 16-235 clip/core\n", pgm);
	fprintf(stderr, "%s:   disables the CbCr 16-240 clip/core\n", pgm);
	fprintf(stderr, "%s:   (allow the entire 0 to 255 range).\n", pgm);
	exit(1);
	}
