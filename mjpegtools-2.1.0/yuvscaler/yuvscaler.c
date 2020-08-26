/*
  *  yuvscaler.c
  *  Copyright (C) 2001-2004 Xavier Biquard <xbiquard@free.fr>
  * 
  *  
  *  Scales arbitrary sized yuv frame to yuv frames suitable for VCD, SVCD or specified
  * 
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation; either version 2 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */
// Implementation: there are two scaling methods: one for not_interlaced output and one for interlaced output. 
// 
// First version doing only downscaling with no interlacing
// June 2001: interlacing capable version
// July 2001: upscaling capable version
// September 2001: line switching
// September/October 2001: new yuv4mpeg header
// October 2001: first MMX part for bicubic
// September/November 2001: what a mess this code! => cleaning and splitting
// December 2001: implementation of time reordering of frames
// January 2002: sample aspect ratio calculation by Matto
// February 2002: interlacing specification now possible. Replaced alloca with malloc
// Mars 2002: sample aspect ratio calculations are back!
// May/June 2002: remove file reading capabilities (do not duplicate lav2yuv), add -O DVD, add color chrominance correction 
// as well as luminance linear reequilibrium. Lots of code cleaning, function renaming, etc...
// Keywords concerning interlacing/preprocessing now under INPUT case
// October 2002: yuvscaler functionnalities not related to image rescaling now part of yuvcorrect
// January 2003: reimplementation of the bicubic algorithm => goes faster
// December-January 2004: First MMX subroutine for bicubic calculus => speed x2
// January-February 2004: make it go even faster
// This first MMX version showed the limits of the use of cspline_w and cspline_h pointers => second
// MMX version will implement dedicated cspline_w and cspline_h pointers for MMX treatment
// 
// 
// TODO:
// no more global variables for librarification

// Remove file reading/writing
// treat the interlace case + specific cases

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include "mjpeg_logging.h"
#include "yuv4mpeg.h"
#include "mjpeg_types.h"
#include "yuvscaler.h"
#include "mpegconsts.h"

#ifdef HAVE_ASM_MMX
#include <fcntl.h>
#include "../utils/mmx.h"
#endif

#define yuvscaler_VERSION "11-Dec-2007"
// For pointer address alignement
#define ALIGNEMENT 16		// 16 bytes alignement for mmx registers in SIMD instructions for Pentium
#define MAXWIDTHNEIGHBORS 16

float PI = 3.141592654;


// For input
unsigned int input_width;
unsigned int input_height;
y4m_ratio_t input_sar;		// see yuv4mpeg.h and yuv4mpeg_intern.h for possible values
unsigned int input_useful = 0;	// =1 if specified
unsigned int input_useful_width = 0;
unsigned int input_useful_height = 0;
unsigned int input_discard_col_left = 0;
unsigned int input_discard_col_right = 0;
unsigned int input_discard_line_above = 0;
unsigned int input_discard_line_under = 0;
unsigned int input_black = 0;	//=1 if black borders on input frames
unsigned int input_black_line_above = 0;
unsigned int input_black_line_under = 0;
unsigned int input_black_col_right = 0;
unsigned int input_black_col_left = 0;
unsigned int input_active_height = 0;
unsigned int input_active_width = 0;
uint8_t input_y_min, input_y_max;
float Ufactor, Vfactor, Gamma;

// Downscaling ratios
unsigned int input_height_slice;
unsigned int output_height_slice;
unsigned int input_width_slice;
unsigned int output_width_slice;


// For padded_input
unsigned int padded_width = 0;
unsigned int padded_height = 0;


// For output
unsigned int display_width;
unsigned int display_height;
unsigned int output_width;
unsigned int output_height;
unsigned int output_active_width;
unsigned int output_active_height;
unsigned int output_black_line_above = 0;
unsigned int output_black_line_under = 0;
unsigned int output_black_col_right = 0;
unsigned int output_black_col_left = 0;
unsigned int output_skip_line_above = 0;
unsigned int output_skip_line_under = 0;
unsigned int output_skip_col_right = 0;
unsigned int output_skip_col_left = 0;
unsigned int black = 0, black_line = 0, black_col = 0;	// =1 if black lines must be generated on output
unsigned int skip = 0, skip_line = 0, skip_col = 0;	// =1 if lines or columns from the active output will not be displayed on output frames
// NB: as these number may not be multiple of output_[height,width]_slice, it is not possible to remove the corresponding pixels in
// the input frame, a solution that could speed up things. 
unsigned int vcd = 0;		//=1 if vcd output was selected
unsigned int svcd = 0;		//=1 if svcd output was selected
unsigned int dvd = 0;		//=1 if dvd output was selected

// Global variables
int interlaced = -1;            //=Y4M_ILACE_NONE for not-interlaced scaling, =Y4M_ILACE_TOP_FIRST or Y4M_ILACE_BOT_FIRST for interlaced scaling
int norm = -1;			// =0 for PAL and =1 for NTSC
int wide = 0;			// =1 for wide (16:9) input to standard (4:3) output conversion
int ratio = 0;
int size_keyword = 0;		// =1 is the SIZE keyword has been used
int infile = 0;			// =0 for stdin (default) =1 for file
int algorithm = -1;		// =0 for resample, and =1 for bicubic
unsigned int specific = 0;	// is >0 if a specific downscaling speed enhanced treatment of data is possible
unsigned int mono = 0;		// is =1 for monochrome output

// Keywords for argument passing 
const char VCD_KEYWORD[] = "VCD";
const char HIRESSTILL[]   = "HIRESSTILL";
const char LOSVCDSTILL[]  = "LOSVCDSTILL";
const char LOVCDSTILL[]   = "LOVCDSTILL";
const char SVCD_KEYWORD[] = "SVCD";
const char DVD_KEYWORD[] = "DVD";
const char SIZE_KEYWORD[] = "SIZE_";
const char USE_KEYWORD[] = "USE_";
const char WIDE2STD_KEYWORD[] = "WIDE2STD";
const char INFILE_KEYWORD[] = "INFILE_";
const char RATIO_KEYWORD[] = "RATIO_";
const char MONO_KEYWORD[] = "MONOCHROME";
const char FASTVCD[] = "FASTVCD";
const char FAST_WIDE2VCD[] = "FAST_WIDE2VCD";
const char WIDE2VCD[] = "WIDE2VCD";
const char RESAMPLE[] = "RESAMPLE";
const char BICUBIC[] = "BICUBIC";
const char ACTIVE[] = "ACTIVE";
const char NO_HEADER[] = "NO_HEADER";
const char NOMMX[] = "NOMMX";

// Specific to BICUBIC algorithm
// 2048=2^11
#define FLOAT2INTEGER 2048
#define FLOAT2INTEGERPOWER 11
unsigned int bicubic_div_width = FLOAT2INTEGER, bicubic_div_height =
  FLOAT2INTEGER;
unsigned int multiplicative;


// Unclassified
unsigned long int diviseur;
uint8_t *divide;
unsigned short int *u_i_p;
unsigned int out_nb_col_slice, out_nb_line_slice;
const static char *legal_opt_flags = "k:I:d:n:v:M:m:O:whtg";
int verbose = 1;
#define PARAM_LINE_MAX 256

uint8_t blacky = 16;
uint8_t blackuv = 128;
uint8_t no_header = 0;		// =1 for no stream header output 

#ifdef HAVE_ASM_MMX
int16_t *mmx_padded, *mmx_cubic;
int32_t *mmx_res;
int mmx = 1;			// =1 for mmx activated, =0 for deactivated/not available
#endif

int32_t *intermediate,*intermediate_p,*inter_begin;


// *************************************************************************************
void
yuvscaler_print_usage (char *argv[])
{
  fprintf (stderr,
	   "usage: yuvscaler -I [input_keyword] -M [mode_keyword] -O [output_keyword] [-S 0|1] [-n p|s|n] [-v 0-2] [-h]\n"
	   "yuvscaler UPscales or DOWNscales arbitrary-sized YUV frames coming from stdin (in YUV4MPEG 4:2:2 format)\n"
	   "to a specified YUV frame sizes to stdout. Please use yuvcorrect for interlacing or color corrections\n"
	   "\n"
	   "yuvscaler is keyword driven :\n"
	   "\t -I for keyword concerning INPUT  frame characteristics\n"
	   "\t -M for keyword concerning the scaling MODE of yuvscaler\n"
	   "\t -O for keyword concerning OUTPUT frame characteristics\n"
	   "\n"
	   "Possible input keyword are:\n"
	   "\t USE_WidthxHeight+WidthOffset+HeightOffset to select a useful area of the input frame (all multiple of 2,\n"
	   "\t    Height and HeightOffset multiple of 4 if interlaced), the rest of the image being discarded\n"
	   "\t ACTIVE_WidthxHeight+WidthOffset+HeightOffset to select an active area of the input frame (all multiple of 2,\n"
	   "\t    Height and HeightOffset multiple of 4 if interlaced), the rest of the image being made black\n"
	   "\n"
	   "Possible mode keyword are:\n"
	   "\t BICUBIC       to use the (Mitchell-Netravalli) high-quality bicubic upscaling and/or downscaling algorithm\n"
	   "\t RESAMPLE      to use a classical resampling algorithm -only for downscaling- that goes much faster than bicubic\n"
	   "\t For coherence reason, yuvscaler will use RESAMPLE if only downscaling is necessary, BICUBIC otherwise\n"
	   "\t WIDE2STD      to converts widescreen (16:9) input frames to standard output (4:3), generating necessary black lines\n"
	   "\t RATIO_WidthIn_WidthOut_HeightIn_HeightOut to specified conversion ratios of\n"
	   "\t     WidthIn/WidthOut for width and HeightIN/HeightOut for height to be applied to the useful area.\n"
	   "\t     The output active area that results from scaling the input useful area might be different\n"
	   "\t     from the display area specified thereafter using the -O KEYWORD syntax.\n"
	   "\t     In that case, yuvscaler will automatically generate necessary black lines and columns and/or skip necessary\n"
	   "\t     lines and columns to get an active output centered within the display size.\n"
	   "\t WIDE2VCD      to transcode wide (16:9) frames  to VCD (equivalent to -M WIDE2STD -O VCD)\n"
	   "\t FASTVCD       to transcode full sized frames to VCD (equivalent to -M RATIO_2_1_2_1 -O VCD)\n"
	   "\t FAST_WIDE2VCD to transcode full sized wide (16:9) frames to VCD (-M WIDE2STD -M RATIO_2_1_2_1 -O VCD)\n"
	   "\t NO_HEADER     to suppress stream header generation on output (chaining scaling with different ratios)\n"
	   "\t By default, yuvscaler will use either interlaced or not-interlaced scaling according to the input header interlace information.\n"
	   "\t If this information is missing in the header (cf. mpeg2dec), yuvscaler will use interlaced acaling\n"
	   "\n"
	   "Possible output keywords are:\n"
	   "\t MONOCHROME to generate monochrome frames on output\n"
	   "\t  VCD to generate  VCD compliant frames, taking care of PAL and NTSC standards, not-interlaced/progressive frames\n"
	   "\t SVCD to generate SVCD compliant frames, taking care of PAL and NTSC standards, any interlacing types\n"
	   "\t  DVD to generate  DVD compliant frames, taking care of PAL and NTSC standards, any interlacing types\n"
	   "\t      (SVCD and DVD: if input is not-interlaced/progressive, output interlacing will be taken as top_first)\n"
	   "\t HIRESSTILL to generate HIgh-RESolution STILL images: not-interlaced/progressive frames of size 704x(PAL-576,NTSC-480)\n"
	   "\t LOSVCDSTILL to generate LOw-resolution SVCD still images, not-interlaced/progressive frames, size 480x(PAL-576,NTSC-480)\n"
	   "\t LOVCDSTILL  to generate LOw-resolution  VCD still images, not-interlaced/progressive frames, size 352x(PAL-288,NTSC-240)\n"
	   "\t SIZE_WidthxHeight to generate frames of size WidthxHeight on output (multiple of 2, Height of 4 if interlaced)\n"
	   "\n"
	   "-n  (usually not necessary) if norm could not be determined from data flux, specifies the OUTPUT norm for VCD/SVCD p=pal,s=secam,n=ntsc\n"
	   "-v  Specifies the degree of verbosity: 0=quiet, 1=normal, 2=verbose/debug\n"
	   "-h : print this lot!\n");
  exit (1);
}

// *************************************************************************************


// *************************************************************************************
void
yuvscaler_print_information (y4m_stream_info_t in_streaminfo,
			     y4m_ratio_t frame_rate)
{
  // This function print USER'S INFORMATION
   const char TOP_FIRST[] = "INTERLACED_TOP_FIRST";
   const char BOT_FIRST[] = "INTERLACED_BOTTOM_FIRST";
   const char NOT_INTER[] = "NOT_INTERLACED";
   const char PROGRESSIVE[] = "PROGRESSIVE";

  y4m_log_stream_info (mjpeg_loglev_t("info"), "input: ", &in_streaminfo);

  switch (interlaced)
    {
    case Y4M_ILACE_NONE:
      mjpeg_info ("from %ux%u, take %ux%u+%u+%u, %s/%s",
		  input_width, input_height,
		  input_useful_width, input_useful_height,
		  input_discard_col_left, input_discard_line_above,
		  NOT_INTER, PROGRESSIVE);
      break;
    case Y4M_ILACE_TOP_FIRST:
      mjpeg_info ("from %ux%u, take %ux%u+%u+%u, %s",
		  input_width, input_height,
		  input_useful_width, input_useful_height,
		  input_discard_col_left, input_discard_line_above,
		  TOP_FIRST);
      break;
    case Y4M_ILACE_BOTTOM_FIRST:
      mjpeg_info ("from %ux%u, take %ux%u+%u+%u, %s",
		  input_width, input_height,
		  input_useful_width, input_useful_height,
		  input_discard_col_left, input_discard_line_above,
		  BOT_FIRST);
      break;
    default:
      mjpeg_info ("from %ux%u, take %ux%u+%u+%u",
		  input_width, input_height,
		  input_useful_width, input_useful_height,
		  input_discard_col_left, input_discard_line_above);

    }
  if (input_black == 1)
    {
      mjpeg_info ("with %u and %u black line above and under",
		  input_black_line_above, input_black_line_under);
      mjpeg_info ("and %u and %u black col left and right",
		  input_black_col_left, input_black_col_right);
      mjpeg_info ("%u %u", input_active_width, input_active_height);
    }


   mjpeg_info ("scale to %ux%u, %ux%u being displayed",
	       output_active_width, output_active_height, display_width,
	       display_height);
   
  switch (algorithm)
    {
    case 0:
      mjpeg_info ("Scaling uses the %s algorithm, ", RESAMPLE);
      break;
    case 1:
      mjpeg_info ("Scaling uses the %s algorithm, ", BICUBIC);
      break;
    default:
      mjpeg_error_exit1 ("Unknown algorithm %d", algorithm);
    }


  if (black == 1)
    {
      mjpeg_info ("black lines: %u above and %u under",
		  output_black_line_above, output_black_line_under);
      mjpeg_info ("black columns: %u left and %u right",
		  output_black_col_left, output_black_col_right);
    }
  if (skip == 1)
    {
      mjpeg_info ("skipped lines: %u above and %u under",
		  output_skip_line_above, output_skip_line_under);
      mjpeg_info ("skipped columns: %u left and %u right",
		  output_skip_col_left, output_skip_col_right);
    }
  mjpeg_info ("frame rate: %.3f fps", Y4M_RATIO_DBL (frame_rate));

}

// *************************************************************************************


// *************************************************************************************
uint8_t
yuvscaler_nearest_integer_division (unsigned long int p, unsigned long int q)
{
  // This function returns the nearest integer of the ratio p/q. 
  // As this ratio in yuvscaler corresponds to a pixel value, it should be between 0 and 255
  unsigned long int ratio = p / q;
  unsigned long int reste = p % q;
  unsigned long int frontiere = q - q / 2;	// Do **not** change this into q/2 => it is not the same for odd q numbers

  if (reste >= frontiere)
    ratio++;

  if ((ratio < 0) || (ratio > 255))
    mjpeg_error_exit1 ("Division error: %lu/%lu not in [0;255] range !!\n", p,
		       q);
  return ((uint8_t) ratio);
}

// *************************************************************************************


// *************************************************************************************
static y4m_ratio_t
yuvscaler_calculate_output_sar (int out_w, int out_h,
				int in_w, int in_h, y4m_ratio_t in_sar)
{
// This function calculates the sample aspect ratio (SAR) for the output stream,
//    given the input->output scale factors, and the input SAR.
  if (Y4M_RATIO_EQL (in_sar, y4m_sar_UNKNOWN))
    {
      return y4m_sar_UNKNOWN;
    }
  else
    {
      y4m_ratio_t out_sar;
      /*
         out_SAR_w     in_SAR_w    input_W   output_H
         ---------  =  -------- *  ------- * --------
         out_SAR_h     in_SAR_h    input_H   output_W
       */
      out_sar.n = in_sar.n * in_w * out_h;
      out_sar.d = in_sar.d * in_h * out_w;
      y4m_ratio_reduce (&out_sar);
      return out_sar;
    }
}

// *************************************************************************************


// *************************************************************************************
int
yuvscaler_y4m_read_frame (int fd, y4m_stream_info_t *si, 
			y4m_frame_info_t * frameinfo,
			unsigned long int buflen, uint8_t * buf)
{
  // This function reads a frame from input stream. It does the same thing as the y4m_read_frame function (from yuv4mpeg.c)
  // May be replaced directly by it in the near future
  static int err = Y4M_OK;
   if ((err = y4m_read_frame_header (fd, si, frameinfo)) == Y4M_OK)
     {
	if ((err = y4m_read (fd, buf, buflen)) != Y4M_OK)
	  {
	     mjpeg_info ("Couldn't read FRAME content: %s!",
			 y4m_strerr (err));
	     return (err);
	  }
     }
   else
     {
	if (err != Y4M_ERR_EOF)
	  mjpeg_info ("Couldn't read FRAME header: %s!", y4m_strerr (err));
	else
	  mjpeg_info ("End of stream!");
	return (err);
     }
   return Y4M_OK;
}

// *************************************************************************************


// *************************************************************************************
// PREPROCESSING
// *************************************************************************************
int
blackout (uint8_t * input_y, uint8_t * input_u, uint8_t * input_v)
{
  // The blackout function makes input borders pixels become black
  unsigned int line;
  uint8_t *right;
  // Y COMPONENT

  for (line = 0; line < input_black_line_above; line++)
    {
      memset (input_y, blacky, input_useful_width);
      input_y += input_width;
    }
  right = input_y + input_black_col_left + input_active_width;
  for (line = 0; line < input_active_height; line++)
    {
      memset (input_y, blacky, input_black_col_left);
      memset (right, blacky, input_black_col_right);
      input_y += input_width;
      right += input_width;
    }
  for (line = 0; line < input_black_line_under; line++)
    {
      memset (input_y, blacky, input_useful_width);
      input_y += input_width;
    }
  // U COMPONENT
  for (line = 0; line < (input_black_line_above >> 1); line++)
    {
      memset (input_u, blackuv, input_useful_width >> 1);
      input_u += input_width >> 1;
    }
  right = input_u + ((input_black_col_left + input_active_width) >> 1);
  for (line = 0; line < (input_active_height >> 1); line++)
    {
      memset (input_u, blackuv, input_black_col_left >> 1);
      memset (right, blackuv, input_black_col_right >> 1);
      input_u += input_width >> 1;
      right += input_width >> 1;
    }
  for (line = 0; line < (input_black_line_under >> 1); line++)
    {
      memset (input_u, blackuv, input_useful_width >> 1);
      input_u += input_width >> 1;
    }
  // V COMPONENT
  for (line = 0; line < (input_black_line_above >> 1); line++)
    {
      memset (input_v, blackuv, input_useful_width >> 1);
      input_v += input_width >> 1;
    }
  right = input_v + ((input_black_col_left + input_active_width) >> 1);
  for (line = 0; line < (input_active_height >> 1); line++)
    {
      memset (input_v, blackuv, input_black_col_left >> 1);
      memset (right, blackuv, input_black_col_right >> 1);
      input_v += input_width >> 1;
      right += input_width >> 1;
    }
  for (line = 0; line < (input_black_line_under >> 1); line++)
    {
      memset (input_v, blackuv, input_useful_width >> 1);
      input_v += input_width >> 1;
    }
  return (0);
}

// *************************************************************************************


// *************************************************************************************
void
handle_args_global (int argc, char *argv[])
{
  // This function takes care of the global variables 
  // initialisation that are independent of the input stream
  // The main goal is to know whether input frames originate from file or stdin
  int c;

  while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
    {
      switch (c)
	{
	case 'v':
	  verbose = atoi (optarg);
	  if (verbose < 0 || verbose > 2)
	    {
	      mjpeg_error_exit1 ("Verbose level must be [0..2]");
	    }
	  break;


	case 'n':		// TV norm for SVCD/VCD output
	  switch (*optarg)
	    {
	    case 'p':
	    case 's':
	      norm = 0;
	      break;
	    case 'n':
	      norm = 1;
	      break;
	    default:
	      mjpeg_error_exit1 ("Illegal norm letter specified: %c",
				 *optarg);
	    }
	  break;


	case 'h':
//      case '?':
	  yuvscaler_print_usage (argv);
	  break;

	default:
	  break;
	}

    }
  if (optind != argc)
    yuvscaler_print_usage (argv);

}


// *************************************************************************************


// *************************************************************************************
void
handle_args_dependent (int argc, char *argv[])
{
  // This function takes care of the global variables 
  // initialisation that may depend on the input stream
  // It does also coherence check on input, useful_input, display, output_active sizes and ratio sizes
  int c;
  unsigned int ui1, ui2, ui3, ui4;
  int output, input, mode;

  // By default, display sizes is the same as input size
  display_width = input_width;
  display_height = input_height;

  optind = 1;
  while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
    {
      switch (c)
	{


	  // **************               
	  // OUTPUT KEYWORD
	  // **************               
	case 'O':
	  output = 0;
	  if (strcmp (optarg, VCD_KEYWORD) == 0)
	    {
	      output = 1;
	      vcd = 1;
	      svcd = 0;		// if user gives VCD, SVCD and DVD keywords, take last one only into account
	      dvd = 0;
	      display_width = 352;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("VCD output format requested in PAL/SECAM norm");
		  display_height = 288;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("VCD output format requested in NTSC norm");
		  display_height = 240;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine VCD output size. Please use the -n option!");
	    }
	  if (strcmp (optarg, SVCD_KEYWORD) == 0)
	    {
	      output = 1;
	      svcd = 1;
	      vcd = 0;		// if user gives VCD, SVCD and DVD keywords, take last one only into account
	      dvd = 0;
	      display_width = 480;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("SVCD output format requested in PAL/SECAM norm");
		  display_height = 576;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("SVCD output format requested in NTSC norm");
		  display_height = 480;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine SVCD output size. Please use the -n option!");
	    }
	  if (strcmp (optarg, DVD_KEYWORD) == 0)
	    {
	      output = 1;
	      vcd = 0;
	      svcd = 0;		// if user gives VCD, SVCD and DVD keywords, take last one only into account
	      dvd = 1;
	      display_width = 720;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("DVD output format requested in PAL/SECAM norm");
		  display_height = 576;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("DVD output format requested in NTSC norm");
		  display_height = 480;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine DVD output size. Please use the -n option!");
	    }
	  if (strncmp (optarg, SIZE_KEYWORD, 5) == 0)
	    {
	      output = 1;
	      if (sscanf (optarg, "SIZE_%ux%u", &ui1, &ui2) == 2)
		{
		  // Coherence check: sizes must be multiple of 2
		  if ((ui1 % 2 == 0) && (ui2 % 2 == 0))
		    {
		      display_width = ui1;
		      display_height = ui2;
		      size_keyword = 1;
		    }
		  else
		    mjpeg_error_exit1
		      ("Unconsistent SIZE keyword, not multiple of 2: %s",
		       optarg);
		  // A second check will eventually be done when output interlacing is finally known
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to SIZE keyword: %s", optarg);
	    }
	  // Theoritically, this should go into yuvcorrect, but I hesitate to do so
	  if (strcmp (optarg, HIRESSTILL) == 0)
	    {
	      output = 1;
	      interlaced = Y4M_ILACE_NONE;
	      display_width = 704;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("HIRESSTILL output format requested in PAL/SECAM norm");
		  display_height = 576;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("HIRESSTILL output format requested in NTSC norm");
		  display_height = 480;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine HIRESSTILL output size. Please use the -n option!");
	    }
	   if (strcmp (optarg, LOSVCDSTILL) == 0)
	    {
	      output = 1;
	      interlaced = Y4M_ILACE_NONE;
	      display_width = 480;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("LOSVCDSTILL output format requested in PAL/SECAM norm");
		  display_height = 576;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("LOSVCDSTILL output format requested in NTSC norm");
		  display_height = 480;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine LOSVCDSTILL output size. Please use the -n option!");
	    }
	  if (strcmp (optarg, LOVCDSTILL) == 0)
	    {
	      output = 1;
	      interlaced = Y4M_ILACE_NONE;
	      display_width = 352;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("LOVCDSTILL output format requested in PAL/SECAM norm");
		  display_height = 288;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("LOVCDSTILL output format requested in NTSC norm");
		  display_height = 240;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine LOVCDSTILL output size. Please use the -n option!");
	    }
	  if (strcmp (optarg, MONO_KEYWORD) == 0)
	    {
	      output = 1;
	      mono = 1;
	    }

	  if (output == 0)
	    mjpeg_error_exit1 ("Uncorrect output keyword: %s", optarg);
	  break;



	  // **************            
	  // MODE KEYOWRD
	  // *************
	case 'M':
	  mode = 0;

	   
	   if (strcmp (optarg, WIDE2STD_KEYWORD) == 0)
	    {
	      wide = 1;
	      mode = 1;
	    }
	   // developper's Testing purpose only
	   if (strcmp (optarg, NOMMX) == 0)
	    {
#ifdef HAVE_ASM_MMX	       
	      mmx = 0;
#endif	       
	      mode = 1;
	    }
	  if (strcmp (optarg, RESAMPLE) == 0)
	    {
	      mode = 1;
	      algorithm = 0;
	    }
	  if (strcmp (optarg, BICUBIC) == 0)
	    {
	      mode = 1;
	      algorithm = 1;
	    }
	  if (strcmp (optarg, NO_HEADER) == 0)
	    {
	      mode = 1;
	      no_header = 1;
	    }
	  if (strncmp (optarg, RATIO_KEYWORD, 6) == 0)
	    {
	      if (sscanf (optarg, "RATIO_%u_%u_%u_%u", &ui1, &ui2, &ui3, &ui4)
		  == 4)
		{
		  // A coherence check will be done when the useful input sizes are known
		  ratio = 1;
		  mode = 1;
		  input_width_slice = ui1;
		  output_width_slice = ui2;
		  input_height_slice = ui3;
		  output_height_slice = ui4;
		}
	      if (ratio == 0)
		mjpeg_error_exit1 ("Unconsistent RATIO keyword: %s", optarg);
	    }

	  if (strcmp (optarg, FAST_WIDE2VCD) == 0)
	    {
	      wide = 1;
	      mode = 1;
	      ratio = 1;
	      input_width_slice = 2;
	      output_width_slice = 1;
	      input_height_slice = 2;
	      output_height_slice = 1;
	      vcd = 1;
	      svcd = 0;		// if user gives VCD and SVCD keyword, take last one only into account
	      display_width = 352;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("VCD output format requested in PAL/SECAM norm");
		  display_height = 288;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("VCD output format requested in NTSC norm");
		  display_height = 240;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine VCD output size. Please use the -n option!");
	    }

	  if (strcmp (optarg, WIDE2VCD) == 0)
	    {
	      wide = 1;
	      mode = 1;
	      vcd = 1;
	      svcd = 0;		// if user gives VCD and SVCD keyword, take last one only into account
	      display_width = 352;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("VCD output format requested in PAL/SECAM norm");
		  display_height = 288;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("VCD output format requested in NTSC norm");
		  display_height = 240;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine VCD output size. Please use the -n option!");
	    }

	  if (strcmp (optarg, FASTVCD) == 0)
	    {
	      mode = 1;
	      vcd = 1;
	      svcd = 0;		// if user gives VCD and SVCD keyword, take last one only into account
	      ratio = 1;
	      input_width_slice = 2;
	      output_width_slice = 1;
	      input_height_slice = 2;
	      output_height_slice = 1;
	      display_width = 352;
	      if (norm == 0)
		{
		  mjpeg_info
		    ("VCD output format requested in PAL/SECAM norm");
		  display_height = 288;
		}
	      else if (norm == 1)
		{
		  mjpeg_info ("VCD output format requested in NTSC norm");
		  display_height = 240;
		}
	      else
		mjpeg_error_exit1
		  ("No norm specified, cannot determine VCD output size. Please use the -n option!");
	    }

	  if (mode == 0)
	    mjpeg_error_exit1 ("Uncorrect mode keyword: %s", optarg);
	  break;



	  // **************            
	  // INPUT KEYOWRD
	  // *************
	case 'I':
	  input = 0;
	  if (strncmp (optarg, USE_KEYWORD, 4) == 0)
	    {
	      input = 1;
	      if (sscanf (optarg, "USE_%ux%u+%u+%u", &ui1, &ui2, &ui3, &ui4)
		  == 4)
		{
		  // Coherence check: 
		  // every values must be multiple of 2
		  // and if input is interlaced, height offsets must be multiple of 4
		  // since U and V have half Y resolution and are interlaced
		  // and the required zone must be inside the input size
		  if ((ui1 % 2 == 0) && (ui2 % 2 == 0) && (ui3 % 2 == 0)
		      && (ui4 % 2 == 0) && (ui1 + ui3 <= input_width)
		      && (ui2 + ui4 <= input_height))
		    {
		      input_useful_width = ui1;
		      input_useful_height = ui2;
		      input_discard_col_left = ui3;
		      input_discard_line_above = ui4;
		      input_discard_col_right =
			input_width - input_useful_width -
			input_discard_col_left;
		      input_discard_line_under =
			input_height - input_useful_height -
			input_discard_line_above;
		      input_useful = 1;
		    }
		  else
		    mjpeg_error_exit1
		      ("Unconsistent USE keyword: %s, offsets/sizes not multiple of 2 or offset+size>input size",
		       optarg);
		  if (interlaced != Y4M_ILACE_NONE)
		    {
		      if ((input_useful_height % 4 != 0)
			  || (input_discard_line_above % 4 != 0))
			mjpeg_error_exit1
			  ("Unconsistent USE keyword: %s, height offset or size not multiple of 4 but input is interlaced!!",
			   optarg);
		    }

		}
	       else
		 mjpeg_error_exit1
		 ("Uncorrect USE input flag argument: %s", optarg);
	    }
	  if (strncmp (optarg, ACTIVE, 6) == 0)
	    {
	      input = 1;
	      if (sscanf
		  (optarg, "ACTIVE_%ux%u+%u+%u", &ui1, &ui2, &ui3, &ui4) == 4)
		{
		  // Coherence check : offsets must be multiple of 2 since U and V have half Y resolution 
		  // if interlaced, height must be multiple of 4
		  // and the required zone must be inside the input size
		  if ((ui1 % 2 == 0) && (ui2 % 2 == 0) && (ui3 % 2 == 0)
		      && (ui4 % 2 == 0) && (ui1 + ui3 <= input_width)
		      && (ui2 + ui4 <= input_height))
		    {
		      input_active_width = ui1;
		      input_active_height = ui2;
		      input_black_col_left = ui3;
		      input_black_line_above = ui4;
		      input_black_col_right =
			input_width - input_active_width -
			input_black_col_left;
		      input_black_line_under =
			input_height - input_active_height -
			input_black_line_above;
		      input_black = 1;
		    }
		  else
		    mjpeg_error_exit1
		      ("Unconsistent ACTIVE keyword: %s, offsets/sizes not multiple of 2 or offset+size>input size",
		       optarg);
		  if (interlaced != Y4M_ILACE_NONE)
		    {
		      if ((input_active_height % 4 != 0)
			  || (input_black_line_above % 4 != 0))
			mjpeg_error_exit1
			  ("Unconsistent ACTIVE keyword: %s, height offset or size not multiple of 4 but input is interlaced!!",
			   optarg);
		    }

		}
	      else
		mjpeg_error_exit1
		  ("Uncorrect ACTIVE input flag argument: %s", optarg);
	    }
	  if (input == 0)
	    mjpeg_error_exit1 ("Uncorrect input keyword: %s", optarg);
	  break;

	default:
	   break;
	}
    }

// Interlacing warnings
  if (vcd == 1)
    {
       if ((interlaced == Y4M_ILACE_TOP_FIRST)
	   || (interlaced == Y4M_ILACE_BOTTOM_FIRST))
	 mjpeg_warn
	 ("Interlaced input frames will be downscaled to non-interlaced VCD frames\nIf quality is an issue, please consider deinterlacing input frames with yuvdeinterlace");
       interlaced = Y4M_ILACE_NONE;
    }


  // Size Keyword final coherence check
  if ((interlaced != Y4M_ILACE_NONE) && (size_keyword == 1))
    {
      if (display_height % 4 != 0)
	mjpeg_error_exit1
	  ("Unconsistent SIZE keyword, Height is not multiple of 4 but output interlaced!!");
    }

  // Unspecified input variables specification
  if (input_useful_width == 0)
    input_useful_width = input_width;
  if (input_useful_height == 0)
    input_useful_height = input_height;


  // Ratio coherence check against input_useful size
  if (ratio == 1)
    {
      if ((input_useful_width % input_width_slice == 0)
	  && (input_useful_height % input_height_slice == 0))
	{
	  output_active_width =
	    (input_useful_width / input_width_slice) * output_width_slice;
	  output_active_height =
	    (input_useful_height / input_height_slice) * output_height_slice;
	}
      else
	mjpeg_error_exit1
	  ("Specified input ratios (%u and %u) does not divide input useful size (%u and %u)!",
	   input_width_slice, input_height_slice, input_useful_width,
	   input_useful_height);
    }

  // if USE and ACTIVE keywords were used, redefined input ACTIVE size relative to USEFUL zone
  if ((input_black == 1) && (input_useful == 1))
    {
      input_black_line_above =
	input_black_line_above >
	input_discard_line_above ? input_black_line_above -
	input_discard_line_above : 0;
      input_black_line_under =
	input_black_line_under >
	input_discard_line_under ? input_black_line_under -
	input_discard_line_under : 0;
      input_black_col_left =
	input_black_col_left >
	input_discard_col_left ? input_black_col_left -
	input_discard_col_left : 0;
      input_black_col_right =
	input_black_col_right >
	input_discard_col_right ? input_black_col_right -
	input_discard_col_right : 0;
      input_active_width =
	input_useful_width - input_black_col_left - input_black_col_right;
      input_active_height =
	input_useful_height - input_black_line_above - input_black_line_under;
      if ((input_active_width == input_useful_width)
	  && (input_active_height == input_useful_height))
	input_black = 0;	// black zone is not enterely inside useful zone
    }


  // Unspecified output variables specification
  if (output_active_width == 0)
    output_active_width = display_width;
  if (output_active_height == 0)
    output_active_height = display_height;
//  if (display_width == 0)
//    display_width = output_active_width;
//  if (display_height == 0)
//    display_height = output_active_height;

  if (wide == 1)
    output_active_height = (output_active_height * 3) / 4;
  // Common pitfall! it is 3/4 not 9/16!
  // Indeed, Standard ratio is 4:3, so 16:9 has an height that is 3/4 smaller than the display_height

  // At this point, input size, input_useful size, output_active size and display size are specified
  // Time for the final coherence check and black and skip initialisations
  // Final check
  output_width =
    output_active_width > display_width ? output_active_width : display_width;
  output_height =
    output_active_height >
    display_height ? output_active_height : display_height;
  if (interlaced == Y4M_ILACE_NONE) 
     {
	if ((output_active_width % 2 !=0) || (output_active_height % 2 != 0)
	    || (display_width % 2 != 0) || (display_height % 2 != 0))
	  mjpeg_error_exit1
	  ("Output sizes are not multiple of 2 !!! %ux%u, %ux%u being displayed",
	   output_active_width, output_active_height, display_width,
	   display_height);
     }
   else
     {
	if ((output_active_width % 2 != 0) || (output_active_height % 4 != 0)
	    || (display_width % 2 != 0) || (display_height % 4 != 0))
	  mjpeg_error_exit1
	  ("Output sizes are not multiple of 2 on width and 4 on height (interlaced)! %ux%u, %ux%u being displayed",
	   output_active_width, output_active_height, display_width,
	   display_height);
     }
   
	// Skip and black initialisations
  // 
  if (output_active_width > display_width)
    {
      skip = 1;
      skip_col = 1;
      // output_skip_col_right and output_skip_col_left must be even numbers
      output_skip_col_right = ((output_active_width - display_width) / 4)*2;
      output_skip_col_left =
	output_active_width - display_width - output_skip_col_right;
    }
  if (output_active_width < display_width)
    {
      black = 1;
      black_col = 1;
      // output_black_col_right and output_black_col_left must be even numbers
      output_black_col_right = ((display_width - output_active_width) / 4)*2;
      output_black_col_left =
	display_width - output_active_width - output_black_col_right;
    }
  if (output_active_height > display_height)
    {
      skip = 1;
      skip_line = 1;
      // output_skip_line_above and output_skip_line_under must be even numbers
      output_skip_line_above = ((output_active_height - display_height) / 4)*2;
      output_skip_line_under =
	output_active_height - display_height - output_skip_line_above;
    }
  if (output_active_height < display_height)
    {
      black = 1;
      black_line = 1;
      // output_black_line_above and output_black_line_under must be even numbers
      output_black_line_above = ((display_height - output_active_height) / 4)*2;
      output_black_line_under =
	display_height - output_active_height - output_black_line_above;
    }
}



// *************************************************************************************
// MAIN
// *************************************************************************************
int
main (int argc, char *argv[])
{
  int input_fd = 0;
  int output_fd = 1;

//  DDD and time use
//   int input_fd  = open("./yuvscaler.input",O_RDONLY);
//   int output_fd = open("./yuvscaler.output",O_WRONLY);
// DDD use

  int err = Y4M_OK, nb;
  unsigned long int i, j, h, w;

  long int frame_num = 0;
  unsigned int *height_coeff = NULL, *width_coeff = NULL;
  uint8_t *input = NULL, *output = NULL,
    *padded_input = NULL, *padded_bottom = NULL, *padded_top = NULL;
  uint8_t *input_y, *input_u, *input_v;
  uint8_t *output_y, *output_u, *output_v;
  uint8_t *frame_y, *frame_u, *frame_v;
  uint8_t **frame_y_p = NULL, **frame_u_p = NULL, **frame_v_p = NULL;	// size is not yet known => pointer of pointer
  uint8_t *u_c_p;		//u_c_p = uint8_t pointer
  unsigned int divider;

  // SPECIFIC TO BICUBIC
  unsigned int *in_line = NULL, *in_col = NULL, out_line, out_col;
  unsigned long int somme;
  float *a = NULL, *b = NULL;
  int16_t *cspline_w=NULL,*cspline_h=NULL;
  uint16_t width_offset=0,height_offset=0,left_offset=0,top_offset=0,right_offset=0,bottom_offset=0;
  uint16_t height_pad=0,width_pad=0,width_neighbors=0,height_neighbors=0;
   // On constate que souvent, le dernier coeff cspline est nul => 
   // pas la peine de le prendre en compte dans les calculs
   // Attention ! optimisation vitesse yuvscaler_bicubic.c suppose que zero_width_neighbors=0 ou 1 seulement
  uint8_t zero_width_neighbors=1,zero_height_neighbors=1;
  float width_scale,height_scale;
  int16_t cspline_value = 0;
  int16_t *pointer;
     

  // SPECIFIC TO YUV4MPEG 
  unsigned long int nb_pixels;
  y4m_frame_info_t frameinfo;
  y4m_stream_info_t in_streaminfo;
  y4m_stream_info_t out_streaminfo;
  y4m_ratio_t frame_rate = y4m_fps_UNKNOWN;

  // Initialisation of global variables that are independent of the input stream, input_file in particular
  handle_args_global (argc, argv);

  // Information output
  if (verbose)
     {
     mjpeg_info ("yuvscaler %s %s", PACKAGE_VERSION, yuvscaler_VERSION);
     mjpeg_info ("(C) 2001-2004 Xavier Biquard <xbiquard@free.fr>, yuvscaler -h for help, or man yuvscaler");
     }

  // mjpeg tools global initialisations
  mjpeg_default_handler_verbosity (verbose);
  y4m_init_stream_info (&in_streaminfo);
  y4m_init_stream_info (&out_streaminfo);
  y4m_init_frame_info (&frameinfo);


  // ***************************************************************
  // Get video stream informations (size, framerate, interlacing, sample aspect ratio).
  // The in_streaminfo structure is filled in accordingly 
  // ***************************************************************
  if (y4m_read_stream_header (input_fd, &in_streaminfo) != Y4M_OK)
    mjpeg_error_exit1 ("Could'nt read YUV4MPEG header!");
  input_width = y4m_si_get_width (&in_streaminfo);
  input_height = y4m_si_get_height (&in_streaminfo);
  frame_rate = y4m_si_get_framerate (&in_streaminfo);
  interlaced = y4m_si_get_interlace (&in_streaminfo);
  // ***************************************************************


  // INITIALISATIONS
  // Norm determination from header (this has precedence over user's specification through the -n flag)
  if (Y4M_RATIO_EQL (frame_rate, y4m_fps_PAL))
    norm = 0;
  if (Y4M_RATIO_EQL (frame_rate, y4m_fps_NTSC))
    norm = 1;
  if (norm < 0)
    {
      mjpeg_warn
	("Could not infer norm (PAL/SECAM or NTSC) from input data (frame size=%dx%d, frame rate=%d:%d fps)!!",
	 input_width, input_height, frame_rate.n, frame_rate.d);
    }



  // Deal with args that depend on input stream
  handle_args_dependent (argc, argv);

  // Scaling algorithm determination
  if ((algorithm == 0) || (algorithm == -1))
    {
      // Coherences check: resample can only downscale not upscale
      if ((input_useful_width < output_active_width)
	  || (input_useful_height < output_active_height))
	{
	  if (algorithm == 0)
	    mjpeg_info
	      ("Resampling algorithm can only downscale, not upscale => switching to bicubic algorithm");
	  algorithm = 1;
	}
      else
	algorithm = 0;
    }

  // USER'S INFORMATION OUTPUT
  yuvscaler_print_information (in_streaminfo, frame_rate);

  divider = pgcd (input_useful_width, output_active_width);
  input_width_slice = input_useful_width / divider;
  output_width_slice = output_active_width / divider;
  mjpeg_debug ("divider,i_w_s,o_w_s = %d,%d,%d",
	       divider, input_width_slice, output_width_slice);

  divider = pgcd (input_useful_height, output_active_height);
  input_height_slice = input_useful_height / divider;
  output_height_slice = output_active_height / divider;
  mjpeg_debug ("divider,i_w_s,o_w_s = %d,%d,%d",
	       divider, input_height_slice, output_height_slice);

  diviseur = input_height_slice * input_width_slice;
  mjpeg_debug ("Diviseur=%ld", diviseur);

  mjpeg_info ("Scaling ratio for width is %u to %u",
	      input_width_slice, output_width_slice);
  mjpeg_info ("and is %u to %u for height", input_height_slice,
	      output_height_slice);


  // Now that we know about scaling ratios, we can optimize treatment of an active input zone:
  // we must also check final new size is multiple of 2 on width and 2 or 4 on height
  if (input_black == 1)
    {
      if (((nb = input_black_line_above / input_height_slice) > 0)
	  && ((nb * input_height_slice) % 2 == 0))
	{
	  if (interlaced == Y4M_ILACE_NONE)
	    {
	      input_useful = 1;
	      black = 1;
	      black_line = 1;
	      output_black_line_above += nb * output_height_slice;
	      input_black_line_above -= nb * input_height_slice;
	      input_discard_line_above += nb * input_height_slice;
	    }
	  if ((interlaced != Y4M_ILACE_NONE)
	      && ((nb * input_height_slice) % 4 == 0))
	    {
	      input_useful = 1;
	      black = 1;
	      black_line = 1;
	      output_black_line_above += nb * output_height_slice;
	      input_black_line_above -= nb * input_height_slice;
	      input_discard_line_above += nb * input_height_slice;
	    }
	}
      if (((nb = input_black_line_under / input_height_slice) > 0)
	  && ((nb * input_height_slice) % 2 == 0))
	{
	  if (interlaced == Y4M_ILACE_NONE)
	    {
	      input_useful = 1;
	      black = 1;
	      black_line = 1;
	      output_black_line_under += nb * output_height_slice;
	      input_black_line_under -= nb * input_height_slice;
	      input_discard_line_under += nb * input_height_slice;
	    }
	  if ((interlaced != Y4M_ILACE_NONE)
	      && ((nb * input_height_slice) % 4 == 0))
	    {
	      input_useful = 1;
	      black = 1;
	      black_line = 1;
	      output_black_line_under += nb * output_height_slice;
	      input_black_line_under -= nb * input_height_slice;
	      input_discard_line_under += nb * input_height_slice;
	    }
	}
      if (((nb = input_black_col_left / input_width_slice) > 0)
	  && ((nb * input_height_slice) % 2 == 0))
	{
	  input_useful = 1;
	  black = 1;
	  black_col = 1;
	  output_black_col_left += nb * output_width_slice;
	  input_black_col_left -= nb * input_width_slice;
	  input_discard_col_left += nb * input_width_slice;
	}
      if (((nb = input_black_col_right / input_width_slice) > 0)
	  && ((nb * input_height_slice) % 2 == 0))
	{
	  input_useful = 1;
	  black = 1;
	  black_col = 1;
	  output_black_col_right += nb * output_width_slice;
	  input_black_col_right -= nb * input_width_slice;
	  input_discard_col_right += nb * input_width_slice;
	}
      input_useful_height =
	input_height - input_discard_line_above - input_discard_line_under;
      input_useful_width =
	input_width - input_discard_col_left - input_discard_col_right;
      input_active_width =
	input_useful_width - input_black_col_left - input_black_col_right;
      input_active_height =
	input_useful_height - input_black_line_above - input_black_line_under;
      if ((input_active_width == input_useful_width)
	  && (input_active_height == input_useful_height))
	input_black = 0;	// black zone doesn't go beyong useful zone
      output_active_width =
	(input_useful_width / input_width_slice) * output_width_slice;
      output_active_height =
	(input_useful_height / input_height_slice) * output_height_slice;

      // USER'S INFORMATION OUTPUT
      mjpeg_info (" --- Newly speed optimized parameters ---");
      yuvscaler_print_information (in_streaminfo, frame_rate);
    }

   
   // Take care of the case where ratios are 1:1 and 1:1 => in the resample algorithm category by convention
   if ((output_height_slice == 1) && (input_height_slice == 1)
	  && (output_width_slice == 1) && (input_width_slice == 1)) 
     algorithm =0;


  // RESAMPLE RESAMPLE RESAMPLE   
  if (algorithm == 0)
    {
      // SPECIFIC
      // Is a specific downscaling speed enhanced treatment available?
      if ((output_width_slice == 1) && (input_width_slice == 1))
	specific = 5;
      if ((output_width_slice == 1) && (input_width_slice == 1)
	  && (input_height_slice == 4) && (output_height_slice == 3))
	specific = 7;
      if ((input_height_slice == 2) && (output_height_slice == 1))
	specific = 3;
      if ((output_height_slice == 1) && (input_height_slice == 1))
	specific = 1;
      if ((output_height_slice == 1) && (input_height_slice == 1)
	  && (output_width_slice == 2) && (input_width_slice == 3))
	specific = 6;
      if ((output_height_slice == 1) && (input_height_slice == 1)
	  && (output_width_slice == 1) && (input_width_slice == 1))
	specific = 4;
      if ((input_height_slice == 2) && (output_height_slice == 1)
	  && (input_width_slice == 2) && (output_width_slice == 1))
	specific = 2;
      if ((input_height_slice == 8) && (output_height_slice == 3))
	specific = 9;
      if ((input_height_slice == 8) && (output_height_slice == 3)
	  && (input_width_slice == 2) && (output_width_slice == 1))
	specific = 8;
      if (specific)
	mjpeg_info ("Specific downscaling routing number %u", specific);

      // To determine scaled value of pixels in the case of the resample algorithm, we have to divide a long int by 
      // the long int "diviseur". So, to speed up downscaling, we tabulate all possible results of this division 
      // using the divide vector and the function yuvscaler_nearest_integer_division. 
      if (!
	  (divide =
	   (uint8_t *) malloc ((1 + 255 * diviseur) * sizeof (uint8_t) +
			       ALIGNEMENT)))
	mjpeg_error_exit1
	  ("Could not allocate memory for divide table. STOP!");
      mjpeg_debug ("before alignement: divide=%p", divide);
      // alignement instructions
      if (((unsigned long) divide % ALIGNEMENT) != 0)
	divide =
	  (uint8_t *) ((((unsigned long) divide / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
      mjpeg_debug ("after alignement: divide=%p", divide);

      u_c_p = divide;
      for (i = 0; i <= 255 * diviseur; i++)
	*(u_c_p++) = yuvscaler_nearest_integer_division (i, diviseur);

      // Calculate averaging coefficient
      // For the height
      height_coeff =
	malloc ((input_height_slice + 1) * output_height_slice *
		sizeof (unsigned int));
      average_coeff (input_height_slice, output_height_slice, height_coeff);

      // For the width
      width_coeff =
	malloc ((input_width_slice + 1) * output_width_slice *
		sizeof (unsigned int));
      average_coeff (input_width_slice, output_width_slice, width_coeff);

    }
  // END OF RESAMPLE RESAMPLE RESAMPLE      

   
   

  // BICUBIC BICUBIC BICUBIC  
  if (algorithm == 1)
    {
       
       

      // SPECIFIC
      // Is a specific downscaling speed enhanced treatment available?
       
       // We only downscale on height, not width.
       // Ex: 16/9 to 4/3 conversion
      if ((output_width_slice == 1) && (input_width_slice == 1))
	specific = 5;

       // We only downscale on width, not height
       // Ex: Full size to SVCD 
      if ((output_height_slice == 1) && (input_height_slice == 1))
	specific = 1;

       if (specific)
	 mjpeg_info ("Specific downscaling routing number %u", specific);
       
//       specific=0;
       // Let us tabulate several values which are explained below:
       // 
       // Given the scaling factor "height_scale" on height and "width_scale" on width, to an output pixel of coordinates (out_col,out_line)
       // corresponds an input pixel of coordinates (in_col,in_line), where in_col = out_col/width_scale and in_line = out_line/height_scale.
       // As pixel coordinates are integer values, we take for in_col and out_col the nearest smaller integer
       // value: in_col = floor(out_col/width_scale) and in_line = floor(out_line/height_scale).
       // The input pixel of coordinates (in_col,in_line) is called the base input pixel
       // Thus, we make an error conventionnally named "b" for columns and "a" for lines
       // with b = out_col/width_scale - floor(out_col/width_scale) and a = out_line/height_scale - floor(out_line/height_scale). 
       // Please note that a and b are inside range [0,1[. 
       // 
       // For upscaling along w (resp. h), we need to take into account the 4 nearest neighbors along w (resp. h) of the base input pixel.
       // For downscaling along w (resp. h), we need to take into account AT LEAST 6 nearest neighbors of the base input pîxel. 
       // And the real number of neighbors pixel from the base input pixel to be taken into account depends on the scaling ratios. 
       // We have to take into account "width_neighbors" neighbors on the width and "height_neighbors" on the height; 
       // with width_neighbors = 2*nearest_higher_integer(2/width_scale),  or 4 if upscaling (output_width_slice>input_width_slice)
       // and width_offset=(width_neighbors/2)-1;
       // with height_neighbors = 2*nearest_higher_integer(2/height_scale), or 4 if upscaling (output_height_slice>input_height_slice)
       // and height_offset=(height_neighbors/2)-1;
       // 
       // *****************
       // The general formula giving the value of the output pixel as a function of the input pixels is:
       // OutputPixel(out_col,out_line)=
       // Sum(h=-height_offset,...(height_neigbors-height_offset-1))Sum(w=-width_offset,...(width_neigbors-width_offset-1))
       // InputPixel(in_col+w,in_line+h)*BicubicCoefficient((b-w)*scale_width)*BicubicCoefficient((a-h)*scale_height)*scale_height*scale_width
       // *****************
       // Please note that [theoretically] (a-h)*scale_height is [-2:2], as well as (b-w)*scale_width. 
       // But, as height_neigbors is the nearest higher integer, the practical range for "(a-h)*scale_height" and "(b-w)*scale_width" is
       // extended from theorital [-2:2] to [-3:3] => "(a-h)*scale_height" and "(b-w)*scale_width" are considered 0 outside of [-2:2].
       // Please note also that for upscaling only, scale_height and scale_width are artifially taken as 1.0 in the formula.
       // 
       // For an easier implementation, it is preferable that h and w start from 0. Therefore, in the general formula, we will replace
       // "h" by "h-height_offset" and "w" by "w-width_offset".
       // 
       // Moreover, the output pixel value depends on at least the 4x4 nearest neighbors from the base input pixel in the input image. 
       // As a consequence, if the base input pixel is at on the border of the image, the bicubic algorithm will try to find values
       // outside the input image => to avoid this, we will pad the input image height_offset pixel on the top, width_offset pixels on the left,
       // (and right_offset pixels on the right and bottom_offset pixels at the bottom). 
       // Therefore, in the general formula, we will replace InputPixel(x,y) by PaddedInputPixel(x+width_offset,y+height_offset).
       // 
       // *****************
       // Finally, the general formula may be rewritten as:
       // OutputPixel(out_col,out_line)=
       // Sum(h=0,...(height_neigbors-1))Sum(w=0...(width_neigbors-1))
       // PaddedInputPixel(in_col+w,in_line+h)*BicubicCoefficient((b-w+width_offset)*scale_width)*BicubicCoefficient((a-h+height_offset)*scale_height)*scale_height*scale_width
       // *****************
       // 
       // 
       // *****************
       // IMPLEMENTATION
       // *****************
       // To insure a fast implementation of the general formula, we will pre-calculate all possible values of 
       // BicubicCoefficient((b-w+width_offset)*scale_width)*scale_width, and tabulate them as cspline_w:
       // cspline_w[w,out_col]=BicubicCoefficient((b[out_col]-w+width_offset)*scale_width)*scale_width. 
       // And the same stands for height:
       // cspline_h[h,out_line]=BicubicCoefficient((a[out_line]-h+height_offset)*scale_height)*scale_height
       // 
       // To be continued ...
      
       height_scale=(float)output_height_slice/(float)input_height_slice;
       if (height_scale>1.0)
	 height_scale=1.0;
       
       width_scale=(float)output_width_slice/(float)input_width_slice;
       if (width_scale>1.0)
	 width_scale=1.0;

       width_neighbors = (2 * input_width_slice ) / output_width_slice; 
       if (((2 * input_width_slice ) % output_width_slice)!=0)
	 width_neighbors++;
       width_neighbors*=2;
       if (width_neighbors < 4)
	 width_neighbors = 4;
       width_offset = left_offset = width_neighbors/2-1;
       width_pad=width_neighbors - 1;
       right_offset=width_neighbors/2;

       height_neighbors = (2 * input_height_slice ) / output_height_slice; 
       if (((2 * input_height_slice ) % output_height_slice)!=0)
	 height_neighbors++;
       height_neighbors*=2;
       if (height_neighbors < 4)
	 height_neighbors = 4;
       height_offset = top_offset = height_neighbors/2-1;
       height_pad=height_neighbors - 1;
       bottom_offset=height_neighbors/2;

       mjpeg_debug("height_scale=%f, width_scale=%f, width_neighbors=%d, height_neighbors=%d",height_scale,width_scale,width_neighbors,height_neighbors);
      // Memory allocations

       
#ifdef HAVE_ASM_MMX
      if (!(mmx_res =
	       (int32_t *) malloc (2 * sizeof (int32_t) + ALIGNEMENT)))
	mjpeg_error_exit1
	  ("Could not allocate memory for mmx registers. STOP!");
      // alignement instructions
      if (((unsigned long int) mmx_res % ALIGNEMENT) != 0)
	mmx_res =
	  (int32_t *) ((((unsigned long int) mmx_res / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
       
       if (mmx==1)
	 {
	    if (width_neighbors <= MAXWIDTHNEIGHBORS)
	      {
		 mjpeg_info("MMX accelerated treatment activated");
		 mmx = 1;
	      }
	    else 
	      {
		 mmx=0;
		 mjpeg_warn("MMX accelerated treatment not available for downscaling ratio larger than 4 to 1");
		 mjpeg_warn("If you still want to use an MMX treatment (not really useful for such a large downscaling ratio");
		 mjpeg_warn("please use multiple yuvscaler downscaling to achieve the desired downscaling ratio");
	      }
	 }
#endif


// Il faudrait peut-être aligner correctement tous ces pointeurs, en particulier cspline_w_neighbors et cspline_h_neighbors
// qui sont amplement utilisés dans les routines de scaling => il faut aussi aligner cspline_w et cspline_h
      if (
	  !(cspline_w =            (int16_t *) malloc ( width_neighbors  * output_active_width  * sizeof (int16_t))) ||
	  !(cspline_h =            (int16_t *) malloc ( height_neighbors * output_active_height * sizeof (int16_t))) ||
	  !(in_col =          (unsigned int *) malloc ( output_active_width  * sizeof (unsigned int))) ||
	  !(b =                      (float *) malloc ( output_active_width  * sizeof (float))) ||
	  !(in_line =         (unsigned int *) malloc ( output_active_height * sizeof (unsigned int))) ||
	  !(a =                      (float *) malloc ( output_active_height * sizeof (float)))
	  )
	mjpeg_error_exit1
	  ("Could not allocate memory for bicubic tables. STOP!");

       // Initialisation of bicubic tables
       pointer=cspline_h;
       for (out_line = 0; out_line < output_active_height; out_line++)
	 {
	    in_line[out_line] = (out_line * input_height_slice) / output_height_slice;
	    //	    mjpeg_debug("in_line[%u]=%u",out_line,in_line[out_line]);
	    a[out_line] = 
	      (float) ((out_line * input_height_slice) % output_height_slice) /
	      (float) output_height_slice;
	    somme=0;
	    for (h=0;h<height_neighbors;h++)
	      {
		 cspline_value=cubic_spline ((a[out_line] + height_offset -h)*height_scale, bicubic_div_height)*height_scale;
		 mjpeg_debug("cspline_value=%d,cspline=%d,a[%u]=%g,height_offset=%d,height_scale=%g,h=%lu",cspline_value,cubic_spline ((a[out_line] + height_offset -h)*height_scale, bicubic_div_height),out_line,a[out_line],height_offset,height_scale,h);
		 somme+=cspline_value;
		 *(pointer++)=cspline_value;
	      }
	    if (cspline_value!=0)
	      zero_height_neighbors=0;
	    // Normalisation test and normalisation of cspline 
	    if (somme != bicubic_div_height) 
	      *(pointer-2) += bicubic_div_height-somme; 
	 }
       
       pointer=cspline_w;
       for (out_col = 0; out_col < output_active_width; out_col++)
	 {
	    in_col[out_col] = (out_col * input_width_slice) / output_width_slice;
	    b[out_col] = 
	      (float) ((out_col * input_width_slice) % output_width_slice) /
	      (float) output_width_slice;
	    somme=0;
	    for (w=0;w<width_neighbors;w++)
	      {
//		 mjpeg_debug("b[%u]=%g,width_offset=%d,width_scale=%g,w=%lu",out_col,b[out_col],width_offset,width_scale,w);
		 cspline_value=cubic_spline ((b[out_col] + width_offset -w)*width_scale, bicubic_div_width)*width_scale;
		 mjpeg_debug("cspline_value=%d,b[%u]=%g,height_offset=%d,height_scale=%g,w=%lu",cspline_value,out_col,b[out_col],height_offset,height_scale,w);
		 somme+=cspline_value;
		 *(pointer++)=cspline_value;
	      }
	    if (cspline_value!=0)
	      zero_width_neighbors=0;
	    // Normalisation test and normalisation of cspline 
	    if (somme != bicubic_div_width) 
	      *(pointer-2) += bicubic_div_width-somme; 
	 }
       
// Added +2*ALIGNEMENT for MMX scaling routines that loads a higher number of pixels than necessary (memory overflow) 
      if (interlaced == Y4M_ILACE_NONE)
	{
	  if (!(padded_input =
		(uint8_t *) malloc ((input_useful_width + width_neighbors) *
				    (input_useful_height + height_neighbors)+2*ALIGNEMENT)))
	    mjpeg_error_exit1
	      ("Could not allocate memory for padded_input table. STOP!");
	}
      else
	{
	  if (!(padded_top =
		(uint8_t *) malloc ((input_useful_width + width_neighbors) *
				    (input_useful_height / 2 + height_neighbors)+2*ALIGNEMENT)) ||
	      !(padded_bottom =
		(uint8_t *) malloc ((input_useful_width + width_neighbors) *
				    (input_useful_height / 2 + height_neighbors)+2*ALIGNEMENT)))
	    mjpeg_error_exit1
	      ("Could not allocate memory for padded_top|bottom tables. STOP!");
	}
       if (!(intermediate = (int32_t *) malloc(output_active_width*(input_useful_height + height_neighbors+1)*sizeof(int32_t)))) 
	     mjpeg_error_exit1
	     ("Could not allocate memory for intermediate. STOP!");
       
    }
   
   // END OF BICUBIC BICUBIC BICUBIC     


  // Pointers allocations
  if (!(input = malloc (((input_width * input_height * 3) / 2) + ALIGNEMENT)) ||
      !(output = malloc (((output_width * output_height * 3) / 2) + ALIGNEMENT))
      )
    mjpeg_error_exit1
      ("Could not allocate memory for input or output tables. STOP!");

  // input and output pointers alignement
  mjpeg_debug ("before alignement: input=%p output=%p", input, output);
  if (((unsigned long) input % ALIGNEMENT) != 0)
    input =
      (uint8_t *) ((((unsigned long) input / ALIGNEMENT) + 1) * ALIGNEMENT);
  if (((unsigned long) output % ALIGNEMENT) != 0)
    output =
      (uint8_t *) ((((unsigned long) output / ALIGNEMENT) + 1) * ALIGNEMENT);
  mjpeg_debug ("after alignement: input=%p output=%p", input, output);


  // if skip_col==1
  if (!(frame_y_p = (uint8_t **) malloc (display_height * sizeof (uint8_t *)))
      || !(frame_u_p =
	   (uint8_t **) malloc (display_height / 2 * sizeof (uint8_t *)))
      || !(frame_v_p =
	   (uint8_t **) malloc (display_height / 2 * sizeof (uint8_t *))))
    mjpeg_error_exit1
      ("Could not allocate memory for frame_y_p, frame_u_p or frame_v_p tables. STOP!");

  // Incorporate blacks lines and columns directly into output matrix since this will never change. 
  // BLACK pixel in YUV = (16,128,128)
  if (black == 1)
    {
      u_c_p = output;
      // Y component
      for (i = 0; i < output_black_line_above * output_width; i++)
	*(u_c_p++) = blacky;
      if (black_col == 0)
	u_c_p += output_active_height * output_width;
      else
	{
	  for (i = 0; i < output_active_height; i++)
	    {
	      for (j = 0; j < output_black_col_left; j++)
		*(u_c_p++) = blacky;
	      u_c_p += output_active_width;
	      for (j = 0; j < output_black_col_right; j++)
		*(u_c_p++) = blacky;
	    }
	}
      for (i = 0; i < output_black_line_under * output_width; i++)
	*(u_c_p++) = blacky;

      // U component
      //   u_c_p=output+output_width*output_height;
      for (i = 0; i < output_black_line_above / 2 * output_width / 2; i++)
	*(u_c_p++) = blackuv;
      if (black_col == 0)
	u_c_p += output_active_height / 2 * output_width / 2;
      else
	{
	  for (i = 0; i < output_active_height / 2; i++)
	    {
	      for (j = 0; j < output_black_col_left / 2; j++)
		*(u_c_p++) = blackuv;
	      u_c_p += output_active_width / 2;
	      for (j = 0; j < output_black_col_right / 2; j++)
		*(u_c_p++) = blackuv;
	    }
	}
      for (i = 0; i < output_black_line_under / 2 * output_width / 2; i++)
	*(u_c_p++) = blackuv;

      // V component
      //   u_c_p=output+(output_width*output_height*5)/4;
      for (i = 0; i < output_black_line_above / 2 * output_width / 2; i++)
	*(u_c_p++) = blackuv;
      if (black_col == 0)
	u_c_p += output_active_height / 2 * output_width / 2;
      else
	{
	  for (i = 0; i < output_active_height / 2; i++)
	    {
	      for (j = 0; j < output_black_col_left / 2; j++)
		*(u_c_p++) = blackuv;
	      u_c_p += output_active_width / 2;
	      for (j = 0; j < output_black_col_right / 2; j++)
		*(u_c_p++) = blackuv;
	    }
	}
      for (i = 0; i < output_black_line_under / 2 * output_width / 2; i++)
	*(u_c_p++) = blackuv;
    }

  // MONOCHROME FRAMES
  if (mono == 1)
    {
      // the U and V components of output frame will always be 128
      u_c_p = output + output_width * output_height;
      for (i = 0; i < 2 * output_width / 2 * output_height / 2; i++)
	*(u_c_p++) = blackuv;
    }


  // Various initialisatiosn for variables concerning input and output   
  out_nb_col_slice = output_active_width / output_width_slice;
  out_nb_line_slice = output_active_height / output_height_slice;
  input_y =
    input + input_discard_line_above * input_width + input_discard_col_left;
  input_u =
    input + input_width * input_height +
    input_discard_line_above / 2 * input_width / 2 +
    input_discard_col_left / 2;
  input_v =
    input + (input_height * input_width * 5) / 4 +
    input_discard_line_above / 2 * input_width / 2 +
    input_discard_col_left / 2;
  output_y =
    output + output_black_line_above * output_width + output_black_col_left;
  output_u =
    output + output_width * output_height +
    output_black_line_above / 2 * output_width / 2 +
    output_black_col_left / 2;
  output_v =
    output + (output_width * output_height * 5) / 4 +
    output_black_line_above / 2 * output_width / 2 +
    output_black_col_left / 2;

  // Other initialisations for frame output
  frame_y =
    output + output_skip_line_above * output_width + output_skip_col_left;
  frame_u =
    output + output_width * output_height +
    output_skip_line_above / 2 * output_width / 2 + output_skip_col_left / 2;
  frame_v =
    output + (output_width * output_height * 5) / 4 +
    output_skip_line_above / 2 * output_width / 2 + output_skip_col_left / 2;
  if (skip_col == 1)
    {
      for (i = 0; i < display_height; i++)
	frame_y_p[i] = frame_y + i * output_width;
      for (i = 0; i < display_height / 2; i++)
	{
	  frame_u_p[i] = frame_u + i * output_width / 2;
	  frame_v_p[i] = frame_v + i * output_width / 2;
	}
    }

  nb_pixels = (input_width * input_height * 3) / 2;

  mjpeg_debug ("End of Initialisation");
  // END OF INITIALISATION
  // END OF INITIALISATION
  // END OF INITIALISATION


  // SCALE AND OUTPUT FRAMES 
  // Output file header
  y4m_copy_stream_info (&out_streaminfo, &in_streaminfo);
  y4m_si_set_width (&out_streaminfo, display_width);
  y4m_si_set_height (&out_streaminfo, display_height);
  y4m_si_set_interlace (&out_streaminfo, interlaced);
  y4m_si_set_sampleaspect (&out_streaminfo,
			   yuvscaler_calculate_output_sar (output_width_slice,
							   output_height_slice,
							   input_width_slice,
							   input_height_slice,
							   y4m_si_get_sampleaspect
							   (&in_streaminfo)));
  if (no_header == 0)
    y4m_write_stream_header (output_fd, &out_streaminfo);
  y4m_log_stream_info (mjpeg_loglev_t("info"), "output: ", &out_streaminfo);
   
  
   

  // Master loop : continue until there is no next frame in stdin
  while ((err = yuvscaler_y4m_read_frame
	  (input_fd, &in_streaminfo, &frameinfo, nb_pixels, input)) == Y4M_OK)
    {
      mjpeg_info ("Frame number %ld", frame_num);

      // Blackout if necessary
      if (input_black == 1)
	blackout (input_y, input_u, input_v);
      frame_num++;

      // Output Frame Header
      if (y4m_write_frame_header (output_fd, &out_streaminfo, &frameinfo) != Y4M_OK)
	goto out_error;


      // ***************
      // SCALE THE FRAME
      // ***************
      // RESAMPLE ALGORITHM       
      // ***************
      if (algorithm == 0)
	{
	  if (specific) 
	     {
		average_specific (input_y, output_y, height_coeff,width_coeff, 0);
		if (!mono) 
		  {
		     average_specific (input_u, output_u, height_coeff,
				       width_coeff, 1);
		     average_specific (input_v, output_v, height_coeff,
				       width_coeff, 1);
		  }
	     }
	   else 
	     {
		average (input_y, output_y, height_coeff, width_coeff, 0);
		if (!mono) 
		  {
		  average (input_u, output_u, height_coeff, width_coeff, 1);
		  average (input_v, output_v, height_coeff, width_coeff, 1);
		  }
	     }
	}
      // ***************
      // RESAMPLE ALGO
      // ***************
      // BICIBIC ALGO
      // ***************
      if (algorithm == 1)
	{
	   // INPUT FRAME PADDING BEFORE BICUBIC INTERPOLATION
	   // PADDING IS DONE SEPARATELY FOR EACH COMPONENT
	   // 
	   if (interlaced != Y4M_ILACE_NONE)
	     {
		padding_interlaced (padded_top, padded_bottom, input_y, 0,left_offset,top_offset,right_offset,bottom_offset,width_pad);
		cubic_scale_interlaced (padded_top, padded_bottom, output_y, 
					in_col, in_line,
					cspline_w, width_neighbors, zero_width_neighbors,
					cspline_h, height_neighbors, zero_height_neighbors,
					0);
		if (!mono) 
		  {
		     padding_interlaced (padded_top, padded_bottom, input_u, 1,left_offset,top_offset,right_offset,bottom_offset,width_pad);
		     cubic_scale_interlaced (padded_top, padded_bottom, output_u, 
					     in_col, in_line,
					     cspline_w, width_neighbors,zero_width_neighbors,
					     cspline_h, height_neighbors,zero_height_neighbors,
					     1);
		     padding_interlaced (padded_top, padded_bottom, input_v, 1,left_offset,top_offset,right_offset,bottom_offset,width_pad);
		     cubic_scale_interlaced (padded_top, padded_bottom, output_v, 
					     in_col, in_line,
					     cspline_w, width_neighbors,zero_width_neighbors,
					     cspline_h, height_neighbors,zero_height_neighbors,
					     1);
		  }
	     }
	   else
	     {
		padding (padded_input, input_y, 0,left_offset,top_offset,right_offset,bottom_offset,width_pad);
		cubic_scale (padded_input, output_y, 
			     in_col, in_line,
			     cspline_w, width_neighbors,  zero_width_neighbors,
			     cspline_h, height_neighbors, zero_height_neighbors,
			     0);
		if (!mono) 
		  {
		     padding (padded_input, input_u, 1,left_offset,top_offset,right_offset,bottom_offset,width_pad);
		     cubic_scale (padded_input, output_u, 
				  in_col, in_line,
				  cspline_w, width_neighbors, zero_width_neighbors,
				  cspline_h, height_neighbors, zero_height_neighbors,
				  1);
		     padding (padded_input, input_v, 1,left_offset,top_offset,right_offset,bottom_offset,width_pad);
		     cubic_scale (padded_input, output_v, 
				  in_col, in_line,
				  cspline_w, width_neighbors, zero_width_neighbors,
				  cspline_h, height_neighbors, zero_height_neighbors,
				  1);
		  }
	     }
	}
      // ***************
      // BICIBIC ALGO
      // ***************
      // END OF SCALE THE FRAME
      // **********************

      // OUTPUT FRAME CONTENTS
      if (skip == 0)
	{
	  // Here, display=output_active 
	  if (y4m_write
	      (output_fd, output,
	       (display_width * display_height * 3) / 2) != Y4M_OK)
	    goto out_error;
	}
      else
	{
	  // skip == 1
	  if (skip_col == 0)
	    {
	      // output_active_width==display_width, component per component frame output
	      if (y4m_write
		  (output_fd, frame_y,
		   display_width * display_height) != Y4M_OK)
		goto out_error;
	      if (y4m_write
		  (output_fd, frame_u,
		   display_width / 2 * display_height / 2) != Y4M_OK)
		goto out_error;
	      if (y4m_write
		  (output_fd, frame_v,
		   display_width / 2 * display_height / 2) != Y4M_OK)
		goto out_error;
	    }
	  else
	    {
	      // output_active_width > display_width, line per line frame output for each component
	      for (i = 0; i < display_height; i++)
		{
		  if (y4m_write (output_fd, frame_y_p[i], display_width)
		      != Y4M_OK)
		    goto out_error;
		}
	      for (i = 0; i < display_height / 2; i++)
		{
		  if (y4m_write
		      (output_fd, frame_u_p[i], display_width / 2) != Y4M_OK)
		    goto out_error;
		}
	      for (i = 0; i < display_height / 2; i++)
		{
		  if (y4m_write
		      (output_fd, frame_v_p[i], display_width / 2) != Y4M_OK)
		    goto out_error;

		}
	    }
	}
    }
  // End of master loop => no more frame in stdin

  if (err != Y4M_ERR_EOF)
    mjpeg_error_exit1 ("Couldn't read frame number %ld!", frame_num);
  else
    mjpeg_info ("Normal exit: end of stream with frame number %ld!",
		frame_num);
  y4m_fini_stream_info (&in_streaminfo);
  y4m_fini_stream_info (&out_streaminfo);
  y4m_fini_frame_info (&frameinfo);
  return 0;

out_error:
  mjpeg_error_exit1 ("Unable to write to output - aborting!");
  return 1;
}


// *************************************************************************************
unsigned int
pgcd (unsigned int num1, unsigned int num2)
{
  // Calculates the biggest common divider between num1 and num2, after Euclid's
  // pgcd(a,b)=pgcd(b,a%b)
  // My thanks to Chris Atenasio <chris@crud.net>
  unsigned int c, bigger, smaller;

  if (num2 < num1)
    {
      smaller = num2;
      bigger = num1;
    }
  else
    {
      smaller = num1;
      bigger = num2;
    }

  while (smaller)
    {
      c = bigger % smaller;
      bigger = smaller;
      smaller = c;
    }
  return (bigger);
}

// *************************************************************************************

//      if ((output_width_slice == 1) && (input_width_slice == 1)
//	  && (input_height_slice == 4) && (output_height_slice == 3))
//	specific = 7;

//      if ((output_height_slice == 1) && (input_height_slice == 1)
//	  && (output_width_slice == 2) && (input_width_slice == 3))
//	specific = 6;
       
       
//       if ((input_height_slice == 2) && (output_height_slice == 1))
//	 specific = 3;
       // Full size to VCD
//      if ((input_height_slice == 2) && (output_height_slice == 1)
//	  && (input_width_slice == 2) && (output_width_slice == 1))
//	specific = 2;
//      if ((input_height_slice == 8) && (output_height_slice == 3))
//	specific = 9;
//      if ((input_height_slice == 8) && (output_height_slice == 3)
//	  && (input_width_slice == 2) && (output_width_slice == 1))
//	specific = 8;

     
      // alignement instructions
/*       if (((unsigned int) cspline_w % ALIGNEMENT) != 0)
	 cspline_w =
	 (int16_t *) ((((unsigned int) cspline_w / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
       if (((unsigned int) cspline_h % ALIGNEMENT) != 0)
	 cspline_h =
	 (int16_t *) ((((unsigned int) cspline_h / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
       if (((unsigned int) cspline_w_neighbors % ALIGNEMENT) != 0)
	 cspline_w_neighbors =
	 (int16_t **) ((((unsigned int) cspline_w_neighbors / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
       if (((unsigned int) cspline_h_neighbors % ALIGNEMENT) != 0)
	 cspline_h_neighbors =
	 (int16_t **) ((((unsigned int) cspline_h_neighbors / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
*/


/* 
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
