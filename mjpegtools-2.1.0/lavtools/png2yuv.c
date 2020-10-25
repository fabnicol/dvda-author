/*
png2yuv
========

  Converts a collection of PNG images to a YUV4MPEG stream.
  (see png2yuv -h for help (or have a look at the function "usage"))
  
  PNG (Portable Network Graphics) is a lossless 2D image format. 
  See www.libpng.org for more information. 

  Based on rpf2yuv.c.

  Copyright (C) 1999, 2002 Gernot Ziegler (gz@lysator.liu.se)
  Copyright (C) 2001, 2004 Matthew Marjanovic (maddog@mir.com)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>

#include <png.h>

#include "mjpeg_logging.h"
#include "mjpeg_types.h"

#include "yuv4mpeg.h"
#include "mpegconsts.h"

#include "subsample.h"
#include "colorspace.h"

#define DEFAULT_CHROMA_MODE Y4M_CHROMA_420JPEG

typedef struct _parameters 
{
  char *pngformatstr;
  uint32_t begin;       /**< the video frame start */
  int32_t numframes;   /**< -1 means: take all frames */
  y4m_ratio_t framerate;
  int interlace;   /**< will the YUV4MPEG stream be interlaced? */
  int interleave;  /**< are the PNG frames field-interleaved? */
  int verbose; /**< the verbosity of the program (see mjpeg_logging.h) */

  png_uint_32 width;
  png_uint_32 height;
  int ss_mode; /**< subsampling mode (based on ssm_id from subsample.h) */

  int new_width; /// new MPEG2 width, in case the original one is uneven
  int new_height; /// new MPEG2 width, in case the original one is uneven
} parameters_t;


/*
 * The User Interface parts 
 */

/* usage
 * Prints a short description of the program, including default values 
 * in: prog: The name of the program 
 */
static void usage(char *prog)
{
  char *h;
  
  if (NULL != (h = (char *)strrchr(prog,'/')))
    prog = h+1;
  
  fprintf(stderr, 
	  "usage: %s [ options ]\n"
	  "\n"
	  "where options are ([] shows the defaults):\n"
	  "  -v num        verbosity (0,1,2)                  [1]\n"
	  "  -b framenum   starting frame number              [0]\n"
	  "  -f framerate  framerate for output stream (fps)     \n"
	  "  -n numframes  number of frames to process        [-1 = all]\n"
	  "  -j {1}%%{2}d{3} Read PNG frames with the name components as follows:\n"
	  "               {1} PNG filename prefix (e g rendered_ )\n"
	  "               {2} Counting placeholder (like in C, printf, eg 06 ))\n"
	  "  -I x  interlacing mode:  p = none/progressive\n"
	  "                           t = top-field-first\n"
	  "                           b = bottom-field-first\n"
	  "  -L x  interleaving mode:  0 = non-interleaved (two successive\n"
	  "                                 fields per PNG file)\n"
	  "                            1 = interleaved fields\n"
          "  -S mode  chroma subsampling mode [%s]\n"
	  "\n"
	  "%s pipes a sequence of PNG files to stdout,\n"
	  "making the direct encoding of MPEG files possible under mpeg2enc.\n"
	  "Any 8bit PNG format supported by libpng can be read.\n"
	  "stdout will be filled with the YUV4MPEG movie data stream,\n"
	  "so be prepared to pipe it on to mpeg2enc or to write it into a file.\n"
	  "\n"
	  "\n"
	  "examples:\n"
	  "  %s -j in_%%06d.png -f 25 -I p -b 100000 > result.yuv\n"
	  "  | combines all the available PNGs that match \n"
	  "    in_??????.png, starting with 100000 (in_100000.png, \n"
	  "    in_100001.png, etc...) into the uncompressed YUV4MPEG videofile result.yuv\n"
	  "    The videofile has 25 frames per second and does not use any interlacing.\n"
	  "  %s -Ip -L0 -j abc_%%04d.png | mpeg2enc -f3 -o out.m2v\n"
	  "  | combines all the available PNGs that match \n"
	  "    abc_??????.png, starting with 0000 (abc_0000.png, \n"
	  "    abc_0001.png, etc...) and pipes it to mpeg2enc which encodes\n"
	  "    an MPEG2-file called out.m2v out of it\n"
	  "\n",
	  prog, y4m_chroma_keyword(DEFAULT_CHROMA_MODE), prog, prog, prog);
}



/** parse_commandline
 * Parses the commandline for the supplied parameters.
 * in: argc, argv: the classic commandline parameters
 */
static void parse_commandline(int argc, char ** argv, parameters_t *param)
{
  int c;
  
  param->pngformatstr = NULL;
  param->begin = 0;
  param->numframes = -1;
  param->framerate = y4m_fps_UNKNOWN;
  param->interlace = Y4M_UNKNOWN;
  param->interleave = -1;
  param->verbose = 1;
  param->ss_mode = DEFAULT_CHROMA_MODE;

  /* parse options */
  for (;;) {
    if (-1 == (c = getopt(argc, argv, "I:hv:L:b:j:n:f:z:S:")))
      break;
    switch (c) {
    case 'j':
      param->pngformatstr = strdup(optarg);
      break;
#if 0 
    case 'z':
      param->mza_filename = strdup(optarg);
      param->make_z_alpha = 1;
      break;
#else
    case 'z':
      mjpeg_error("Z encoding currently unsupported !\n");
      exit(-1);
      break;
#endif
    case 'S':
      param->ss_mode = y4m_chroma_parse_keyword(optarg);
      if (param->ss_mode == Y4M_UNKNOWN) {
	mjpeg_error_exit1("Unknown subsampling mode option:  %s", optarg);
      } else if (!chroma_sub_implemented(param->ss_mode)) {
	mjpeg_error_exit1("Unsupported subsampling mode option:  %s", optarg);
      }
      break;
    case 'b':
      param->begin = atol(optarg);
      break;
    case 'n':
      param->numframes = atol(optarg);
      break;
    case 'f':
      param->framerate = mpeg_conform_framerate(atof(optarg));
      break;
    case 'I':
      switch (optarg[0]) {
      case 'p':
	param->interlace = Y4M_ILACE_NONE;
	break;
      case 't':
	param->interlace = Y4M_ILACE_TOP_FIRST;
	break;
      case 'b':
	param->interlace = Y4M_ILACE_BOTTOM_FIRST;
	break;
      default:
	mjpeg_error_exit1 ("-I option requires arg p, t, or b");
      }
      break;
    case 'L':
      param->interleave = atoi(optarg);
      if ((param->interleave != 0) &&
	  (param->interleave != 1)) 
	mjpeg_error_exit1 ("-L option requires arg 0 or 1");
      break;
    case 'v':
      param->verbose = atoi(optarg);
      if (param->verbose < 0 || param->verbose > 2) 
	mjpeg_error_exit1( "-v option requires arg 0, 1, or 2");    
      break;     
    case 'h':
    default:
      usage(argv[0]);
      exit(1);
    }
  }
  if (param->pngformatstr == NULL) 
    { 
      mjpeg_error("%s:  input format string not specified. (Use -j option.)",
		  argv[0]); 
      usage(argv[0]); 
      exit(1);
    }

  if (Y4M_RATIO_EQL(param->framerate, y4m_fps_UNKNOWN)) 
    {
      mjpeg_error("%s:  framerate not specified.  (Use -f option)",
		  argv[0]); 
      usage(argv[0]); 
      exit(1);
    }
}


/*
 * The file handling parts 
 */
/** 
Reads one PNG file. 
@param process Process the image data (NULL for initial parameter determination)
@returns -1 on failure, 1 on sucess
@on success returns RGB data in the second, yuv, parameter
*/
int decode_png(const char *pngname, uint8_t *yuv[], parameters_t *param)
{
  png_structp png_ptr;
  png_infop info_ptr;
  FILE *pngfile;

  /* libpng needs two structs - a png_struct and a png_info, there is no
   * need to make the third, another png_info, because that is only used
   * to store data (such as textual information) that can come after the
   * PNG image.  This code only cares about the image.
   */
  info_ptr = NULL;
  pngfile = NULL;
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    mjpeg_error_exit1("%s: Could not allocate PNG read struct !", pngname);

  /* This needs to be done immediately after creation of the png_struct
   * because storage allocation failures will longjmp back to here:
   */
  if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, 0);
      if (pngfile) (void)fclose(pngfile);
      mjpeg_error("%s: Corrupted PNG file !", pngname);
      return -1;
    }
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct(&png_ptr,
			      (png_infopp)NULL, (png_infopp)NULL);
      mjpeg_error_exit1("%s: Could not allocate PNG info struct !", pngname);
    }
  
/* Now open this PNG file, and examine its header to retrieve the 
 * YUV4MPEG info that shall be written */
  pngfile = fopen(pngname, "rb");
  if (!pngfile)
    {
      perror(pngname);
      png_error(png_ptr, "PNG file open failed");
    }

  png_init_io(png_ptr, pngfile);

  if (yuv)
    {
      png_uint_32 nr, input_height, input_width, output_height, output_width;
      uint8_t *r, *g, *b;
      png_bytepp rows;

      /* The code uses png_read_png to obtain a complete buffered copy of the
       * PNG file reduced (or expanded) to 8 bit RGB.  This is a little wasteful
       * in the case of a non-interlaced image - the code could work row by
       * row without buffering the whole image - but the interlaced case is
       * almost impossible to handle this way so it is better to be simple and
       * correct.
       */
#     if PNG_LIBPNG_VER >= 10500 && PNG_LIBPNG_VER < 10502
         /* There is a bug in 1.5 before 1.5.2 which causes png_read_png to
          * whine most terribly on interlaced images, this stops it:
          */
          (void)png_set_interlace_handling(png_ptr);
#     endif
      png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 |
       PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_EXPAND |
       PNG_TRANSFORM_GRAY_TO_RGB /* requires libpng 1.4 or later */, 0);

      /* And return the separated data to the parameters. */
      rows = png_get_rows(png_ptr, info_ptr);

      /* Since the PNG files for the frames are separate the actual PNG file 
       * that was read could be unrelated - a random width and height.  Because
       * the output may be interleaved the output height may be twice the input
       * PNG height.  Because the MPEG code requires an even width the output
       * width may be one more than the original frame width.
       *
       * For the interleaving the PNG data is smashed into the lower half of
       * the yuv rows.  For the other cases the input data is cropped or
       * top-lefted as appropriate.
       */
      output_height = param->new_height;

      input_height = png_get_image_height(png_ptr, info_ptr);
      if (input_height > output_height)
          input_height = output_height;

      output_width = param->new_width;

      input_width = png_get_image_width(png_ptr, info_ptr);
      if (input_width > output_width)
        input_width = output_width;

      /* Breaking up the RGB data is not hard to do, the separated channels are
       * simply packed into the three raw yuv arrays with new_width values per
       * row.
       */
      r = yuv[0];
      g = yuv[1];
      b = yuv[2];
      for (nr=0; nr<input_height; ++nr)
        {
         png_uint_32 nc;
         png_bytep row = *rows++;

         for (nc=0; nc<input_width; ++nc)
           {
               *r++ = *row++;
               *g++ = *row++;
               *b++ = *row++;
           }
       
         /* Pad the output: */
         for (; nc<output_width; ++nc)
           *r++ = *g++ = *b++ = 0;
       }
    }
  
  else
    {
      /* Just return the image width and height in *param */
      png_read_info(png_ptr, info_ptr);

      param->width = png_get_image_width(png_ptr, info_ptr);
      param->height = png_get_image_height(png_ptr, info_ptr);

    }

/* Successful exit: */
  png_destroy_read_struct(&png_ptr, &info_ptr, 0);

  fclose(pngfile);
  return 1;
}


/** init_parse_files
 * Verifies the PNG input files and prepares YUV4MPEG header information.
 * @returns 0 on success
 */
static int init_parse_files(parameters_t *param)
{ 
  char pngname[PATH_MAX+1]; /* See POSIX 1003.1 section 2.9.5 */

  snprintf(pngname, sizeof(pngname), 
	   param->pngformatstr, param->begin);
  mjpeg_debug("Analyzing %s to get the right pic params", pngname);
  
  /* The first frame (the param->begin frame) determines the height and
   * width of the output.  Passing NULL instead of yuv (see below) causes
   * decode_png to fill in param with the image dimensions.
   */
  if (decode_png(pngname, NULL, param) == -1)
    mjpeg_error_exit1("Reading of %s failed.\n", pngname);

  mjpeg_info("Image dimensions are %ux%u",
	     param->width, param->height);
  
  mjpeg_info("Movie frame rate is:  %f frames/second",
	     Y4M_RATIO_DBL(param->framerate));

  switch (param->interlace) 
    {
    case Y4M_ILACE_NONE:
      mjpeg_info("Non-interlaced/progressive frames.");
      break;
    case Y4M_ILACE_BOTTOM_FIRST:
      mjpeg_info("Interlaced frames, bottom field first.");      
      break;
    case Y4M_ILACE_TOP_FIRST:
      mjpeg_info("Interlaced frames, top field first.");      
      break;
    default:
      mjpeg_error_exit1("Interlace has not been specified (use -I option)");
      break;
    }

  if ((param->interlace != Y4M_ILACE_NONE) && (param->interleave == -1))
    mjpeg_error_exit1("Interleave has not been specified (use -L option)");

  if (!(param->interleave) && (param->interlace != Y4M_ILACE_NONE)) 
    {
      /* So the height in 'param' might be twice the PNG input height:*/
      param->height *= 2;
      mjpeg_info("Non-interleaved fields (image height doubled)");
    }
  mjpeg_info("Frame size:  %u x %u", param->width, param->height);

  return 0;
}

static int generate_YUV4MPEG(parameters_t *param)
{
  uint32_t frame;
  uint8_t *yuv[3];  /* Buffers, initially for R,G,B then Y,Cb,Cr */
  y4m_stream_info_t streaminfo;
  y4m_frame_info_t frameinfo;

  /* Make the output even, so the output may be one larger than the
   * original PNG image width.
   */
  param->new_width = param->width + (param->width & 1);
  param->new_height = param->height + (param->height & 1);

  mjpeg_info("Now generating YUV4MPEG stream.");
  y4m_init_stream_info(&streaminfo);
  y4m_init_frame_info(&frameinfo);

  y4m_si_set_width(&streaminfo, param->new_width);
  y4m_si_set_height(&streaminfo, param->new_height);
  y4m_si_set_interlace(&streaminfo, param->interlace);
  y4m_si_set_framerate(&streaminfo, param->framerate);
  y4m_si_set_chroma(&streaminfo, param->ss_mode);

  yuv[0] = (uint8_t *)malloc(param->new_width * param->new_height * sizeof(yuv[0][0]));
  yuv[1] = (uint8_t *)malloc(param->new_width * param->new_height * sizeof(yuv[1][0]));
  yuv[2] = (uint8_t *)malloc(param->new_width * param->new_height * sizeof(yuv[2][0]));

  y4m_write_stream_header(STDOUT_FILENO, &streaminfo);

  for (frame = param->begin;
       (frame < param->numframes + param->begin) || (param->numframes == -1);
       frame++) 
    {
      char pngname[PATH_MAX+1];
      snprintf(pngname, sizeof(pngname), param->pngformatstr, frame);
            
      /* decode_png reads the PNG into the yuv buffers as r,g,b [0..255]
       * values.
       */
      if (decode_png(pngname, yuv, param) == -1)
	{
	  mjpeg_info("Read from '%s' failed:  %s", pngname, strerror(errno));
	  if (param->numframes == -1) 
	    {
	      mjpeg_info("No more frames.  Stopping.");
	      break;  /* we are done; leave 'while' loop */
	    } 
	  else 
	    {
	      mjpeg_info("Rewriting latest frame instead.");
	    }
	} 
      else 
	{
	  mjpeg_debug("Converting frame to YUV format.");
	  /* Transform colorspace, then subsample (in place) */
	  convert_RGB_to_YCbCr(yuv, param->new_height *  param->new_width);
	  chroma_subsample(param->ss_mode, yuv, param->new_width, param->new_height);

	  mjpeg_debug("Frame decoded, now writing to output stream.");
	}

      mjpeg_debug("Frame decoded, now writing to output stream.");
      y4m_write_frame(STDOUT_FILENO, &streaminfo, &frameinfo, yuv);
    }

  y4m_fini_stream_info(&streaminfo);
  y4m_fini_frame_info(&frameinfo);
  free(yuv[0]);
  free(yuv[1]);
  free(yuv[2]);

  return 0;
}



/* main
 * in: argc, argv:  Classic commandline parameters. 
 * returns: int: 0: success, !0: !success :-)
 */
int main(int argc, char ** argv)
{ 
  parameters_t param;

  y4m_accept_extensions(1);

  parse_commandline(argc, argv, &param);
  mjpeg_default_handler_verbosity(param.verbose);

  mjpeg_info("Parsing & checking input files.");
  if (init_parse_files(&param)) {
    mjpeg_error_exit1("* Error processing the PNG input.");
  }

  if (generate_YUV4MPEG(&param)) { 
    mjpeg_error_exit1("* Error processing the input files.");
  }

  return 0;
}
