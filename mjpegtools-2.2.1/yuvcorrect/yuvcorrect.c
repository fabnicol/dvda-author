/*
  *  yuvcorrect.c
  *  Copyright (C) 2002 Xavier Biquard <xbiquard@free.fr>
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
// September/October 2002: First version 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include "yuv4mpeg.h"
#include "yuvcorrect.h"

extern const uint16_t OFFSET;
extern const uint16_t ALIGNENEMENT;
extern const char *legal_opt_flags;
  

const char PIPE[] = "PIPE";
const char STAT[] = "STAT";
const char FULL[] = "FULL";
const char HALF[] = "HALF";
const char TOP_FIRST[] = "INTERLACED_TOP_FIRST";
const char BOT_FIRST[] = "INTERLACED_BOTTOM_FIRST";
const char NOT_INTER[] = "NOT_INTERLACED";
const char PROGRESSIVE[] = "PROGRESSIVE";
const char LINESWITCH[] = "LINE_SWITCH";
const char NO_HEADER[] = "NO_HEADER";
const char TOP_FORWARD[] = "TOP_FORWARD";
const char BOTT_FORWARD[] = "BOTT_FORWARD";
const char RGBFIRST[] = "RGBFIRST";

// Prototypes specific to yuvcorrect
void yuvcorrect_print_usage (void);
void yuvcorrect_print_information (general_correction_t * gen_correct,
				   yuv_correction_t * yuv_correct,
				   rgb_correction_t * rgb_correct);
void yuvcorrect_handle_args (int argc, char *argv[], overall_t * overall,
			     general_correction_t * gen_correct);

void
yuvcorrect_print_usage (void)
{
  fprintf (stderr,
	   "usage: yuvcorrect -M [mode_keyword] -T [general_keyword] -Y [yuv_keyword] -R [RGB_keyword]  [-v 0-2] [-h]\n"
	   "yuvcorrect applies different corrections related to interlacing and color\n"
	   "to yuv frames coming from stdin (in yuv4MPEG 4:2:0 format) to stdout.\n"
	   "In contrast to yuvscaler, frame size is kept constant.\n"
	   "\n"
	   "yuvcorrect is keyword driven :\n"
	   "\t -M for keyword concerning the correction MODE of yuvcorrect\n"
	   "\t -T for keyword concerning spatial, temporal or header corrections to be applied\n"
	   "\t -Y for keyword concerning color corrections in the yuv space\n"
	   "\t -R for keyword concerning color corrections in the RGB space\n"
	   "By default, yuvcorrect will not modify frames and simply act as a pass-through. Also, it will apply\n"
	   "YUV corrections first and then RGB corrections\n"
	   "\n" "Possible mode keyword are:\n"
	   "\t STAT to have yuvcorrect print statistical information on your frames _before_ corrections\n"
	   "\t RGBFIRST to have yuvcorrect apply RGB corrections first, then YUV corrections\n"
	   "\n"
	   "Possible general keyword are:\n"
	   "\t If you suspect that your video capture was given a wrong interlacing type,\n"
	   "\t and/or was spatially or temporarly missed up, please use and combine:\n"
	   "\t INTERLACED_TOP_FIRST    to correct file header by specifying top_field_first as interlacing type\n"
	   "\t INTERLACED_BOTTOM_FIRST to correct file header by specifying bottom_field_first as interlacing\n"
	   "\t NOT_INTERLACED          to correct file header by specifying not-interlaced/progressive as interlacing type\n"
	   "\t PROGRESSIVE             to correct file header by specifying not-interlaced/progressive as interlacing type\n"
	   "\t NO_HEADER    to suppress stream header generation (apply different corrections to different part of an input file)\n"
	   "\t LINE_SWITCH  to switch lines two by two\n"
	   "\t BOTT_FORWARD to move the bottom field one frame forward\n"
	   "\t TOP_FORWARD  to move the top    field one frame forward\n"
	   "\n"
	   "Possible yuv keywords are:\n"
	   "\t LUMINANCE_Gamma_InputYmin_InputYmax_OutputYmin_OutputYmax or\n"
	   "\t Y_Gamma_InputYmin_InputYmax_OutputYmin_OutputYmax\n"
	   "\t    to correct the input frame luminance by clipping it inside range [InputYmin;InputYmax],\n"
	   "\t    scale with power (1/Gamma), and expand/shrink/shift it to [OutputYmin;OutputYmax]\n"
	   "\t CHROMINANCE_UVrotation_Ufactor_Urotcenter_Vfactor_Vrotcenter_UVmin_UVmax or\n"
	   "\t UV_UVrotation_Ufactor_Urotcenter_Vfactor_Vrotcenter_UVmin_UVmax\n"
	   "\t    to rotate rescaled UV chroma components Ufactor*(U-Urotcenter) and Vfactor*(V-Vrotcenter)\n"
	   "\t    by (float) UVrotation degrees, recenter to the normalize (128,128) center,\n"
	   "\t    and _clip_ the result to range [UVmin;UVmax]\n"
	   "\t CONFORM to have yuvcorrect generate frames conform to the Rec.601 specification for Y'CbCr frames\n"
	   "\t    that is LUMINANCE_1.0_16_235_16_235 and CHROMINANCE_0.0_1.0_128_1.0_128_16_240\n"
	   "\n"
	   "\t Possible RGB keywords are:\n"
	   "\t R_Gamma_InputRmin_InputRmax_OutputRmin_OutputRmax\n"
	   "\t G_Gamma_InputGmin_InputGmax_OutputGmin_OutputGmax\n"
	   "\t B_Gamma_InputBmin_InputBmax_OutputBmin_OutputBmax\n"
	   "\t    to correct the input frame RGB color by clipping it inside range [InputRGBmin;InputRGBmax],\n"
	   "\t    scale with power (1/Gamma), and expand/shrink/shift it to [OutputRGBmin;OutputRGBmax]\n"
	   "\n"
	   "-v  Specifies the degree of verbosity: 0=quiet, 1=normal, 2=verbose/debug\n"
	   "-h : print this lot!\n");
  exit (1);
}

void
handle_args_overall (int argc, char *argv[], overall_t * overall)
{
  int c, verb;

  while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
    {
      switch (c)
	{
	case 'v':
	  verb = atoi (optarg);
	  if (verb < 0 || verb > 2)
	    {
	      mjpeg_info
		("Verbose level must be 0, 1 or 2 ! => resuming to default 1");
	    }
	  else
	    {
	      overall->verbose = verb;
	    }
	  break;

	case 'h':
	  yuvcorrect_print_usage ();
	  break;

	default:
	  break;
	}
    }
  if (optind != argc)
    yuvcorrect_print_usage ();
}

void
yuvcorrect_handle_args (int argc, char *argv[], overall_t * overall,
	     general_correction_t * gen_correct)
{
  // This function handles argument passing on the command line
  int c;
  int k_mode, k_general;

  // Ne pas oublier de mettre la putain de ligne qui suit, sinon, plus d'argument à la lign de commande, ils auront été bouffés par l'appel précédnt à getopt!!
  optind = 1;
  while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
    {
      switch (c)
	{
	  // **************            
	  // MODE KEYOWRD
	  // *************
	case 'M':
	  k_mode = 0;
	   if (strcmp (optarg, STAT) == 0)
	    {
	      k_mode = 1;
	      overall->stat = 1;
	    }
	  if (strcmp (optarg, RGBFIRST) == 0)
	    {
	      k_mode = 1;
	      overall->rgbfirst = 1;
	    }
	   if (k_mode == 0)
	    mjpeg_error_exit1 ("Unrecognized MODE keyword: %s", optarg);
	  break;
	  // *************


	  // **************            
	  // GENERAL KEYOWRD
	  // *************
	case 'T':
	  k_general = 0;
	  if (strcmp (optarg, TOP_FIRST) == 0)
	    {
	      k_general = 1;
	      y4m_si_set_interlace (&gen_correct->streaminfo,
				    Y4M_ILACE_TOP_FIRST);
	    }
	  if (strcmp (optarg, BOT_FIRST) == 0)
	    {
	      k_general = 1;
	      y4m_si_set_interlace (&gen_correct->streaminfo,
				    Y4M_ILACE_BOTTOM_FIRST);
	    }
	  if ((strcmp (optarg, NOT_INTER) == 0)
	      || (strcmp (optarg, PROGRESSIVE) == 0))
	    {
	      k_general = 1;
	      y4m_si_set_interlace (&gen_correct->streaminfo, Y4M_ILACE_NONE);
	    }
	  if (strcmp (optarg, NO_HEADER) == 0)
	    {
	      k_general = 1;
	      gen_correct->no_header = 1;
	    }
	  if (strcmp (optarg, LINESWITCH) == 0)
	    {
	      k_general = 1;
	      gen_correct->line_switch = 1;
	    }
	  if (strcmp (optarg, BOTT_FORWARD) == 0)
	    {
	      k_general = 1;
	      gen_correct->field_move = 1;
	    }
	  if (strcmp (optarg, TOP_FORWARD) == 0)
	    {
	      k_general = 1;
	      gen_correct->field_move = -1;
	    }
	  if (k_general == 0)
	    mjpeg_error_exit1 ("Unrecognized GENERAL keyword: %s", optarg);
	  break;
	  // *************

	default:
	  break;
	}
    }


}

void
yuvcorrect_print_information (general_correction_t * gen_correct,
			      yuv_correction_t * yuv_correct,
			      rgb_correction_t * rgb_correct)
{
  // This function print USER'S INFORMATION

  y4m_log_stream_info (mjpeg_loglev_t("info"), "input: ", &gen_correct->streaminfo);

  switch (gen_correct->line_switch)
    {
    case 0:
      mjpeg_info ("no line switching");
      break;
    case 1:
      mjpeg_info ("with line switching");
      break;
    default:
      mjpeg_error_exit1 ("Unknown line switching status: %d",
			 gen_correct->line_switch);
    }

  switch (gen_correct->field_move)
    {
    case 0:
      mjpeg_info ("no time forwarding");
      break;
    case 1:
      mjpeg_info ("with bottom field one frame forward");
      break;
    case -1:
      mjpeg_info ("with top field one frame forward");
      break;
    default:
      mjpeg_error_exit1 ("Unknown time reordering status: %d",
			 gen_correct->field_move);
    }

  switch (yuv_correct->luma)
    {
    case 0:
      mjpeg_info ("Without luminance correction");
      break;
    case 1:
      mjpeg_info ("With luminance correction");
      break;
    default:
      mjpeg_error_exit1 ("Unknown luminance correction status %u",
			 yuv_correct->luma);
    }

  switch (yuv_correct->chroma)
    {
    case 0:
      mjpeg_info ("Without chrominance correction");
      break;
    case 1:
      mjpeg_info ("With chrominance correction");
      break;
    default:
      mjpeg_error_exit1 ("Unknown chrominance correction status %u",
			 yuv_correct->chroma);
    }

  switch (rgb_correct->rgb)
    {
    case 0:
      mjpeg_info ("Without rgb correction");
      break;
    case 1:
      mjpeg_info ("With rgb correction");
      break;
    default:
      mjpeg_error_exit1 ("Unknown rgb correction status %u",
			 rgb_correct->rgb);
    }

}

int
main (int argc, char *argv[])
{

  // Defining yuvcorrect dedicated structures (see yuvcorrect.h) 
  overall_t *overall=NULL;
  frame_t *frame=NULL;
  general_correction_t *gen_correct=NULL;
  yuv_correction_t *yuv_correct=NULL;
  rgb_correction_t *rgb_correct=NULL;

  int err = Y4M_OK;
  uint8_t oddeven;

  unsigned long int frame_num = 0;

  y4m_accept_extensions(1);

  // START OF INITIALISATION 
  // yuvcorrect overall structure initialisation
  if (!(overall = (overall_t *) malloc (sizeof (overall_t))))
    mjpeg_error_exit1
      ("Could not allocate memory for overall structure pointer");
  overall->verbose = 1;
  overall->mode = overall->stat = overall->rgbfirst = 0;
  handle_args_overall (argc, argv, overall);
  mjpeg_default_handler_verbosity (overall->verbose);
   
  mjpeg_debug ("Start of initialisation");

   // yuvcorrect general_correction_t structure initialisations
   if (!(gen_correct = (general_correction_t *) malloc (sizeof (general_correction_t))))
     mjpeg_error_exit1
     ("Could not allocate memory for gen_correct structure pointer");
   // yuvcorrect frame_t structure initialisations
   if (!(frame = (frame_t *) malloc (sizeof (frame_t))))
     mjpeg_error_exit1
     ("Could not allocate memory for frame structure pointer");
   // yuvcorrect yuv_correction_t structure initialisation  
   if (!(yuv_correct = (yuv_correction_t *) malloc (sizeof (yuv_correction_t))))
     mjpeg_error_exit1
     ("Could not allocate memory for yuv_correct structure pointer");
   // rgbcorrect rgb_correction_t structure initialisation
   if (!(rgb_correct = (rgb_correction_t *) malloc (sizeof (rgb_correction_t))))
     mjpeg_error_exit1
     ("Could not allocate memory for rgb_correct structure pointer");

   initialisation1(0,frame,gen_correct,yuv_correct,rgb_correct);
   // Deal with args 
   handle_args_yuv_rgb (argc, argv, yuv_correct, rgb_correct);
   yuvcorrect_handle_args (argc, argv, overall, gen_correct);
  // Further initialisations depending on the stream itself 
  // General correction initialisations
  if (gen_correct->field_move != 0)
    {
      if (!(frame->field1 = (uint8_t *) malloc ((frame->length >> 1)*sizeof(uint8_t))) ||
	  !(frame->field2 = (uint8_t *) malloc ((frame->length >> 1)*sizeof(uint8_t))))
	 mjpeg_error_exit1
	  ("Could not allocate memory for field1 or field2 tables. STOP!");
    }
   initialisation2(yuv_correct,rgb_correct);

  // USER'S INFORMATION OUTPUT
  yuvcorrect_print_information (gen_correct, yuv_correct, rgb_correct);

  mjpeg_debug ("End of Initialisation");
  // END OF INITIALISATION

  // Eventually output file header
  if (gen_correct->no_header == 0)
    y4m_write_stream_header (1, &gen_correct->streaminfo);

  mjpeg_debug ("overall: verbose=%u, mode=%d, stat=%u", overall->verbose,
	       overall->mode, overall->stat);
  mjpeg_debug ("frame: Y:%ux%u=>%lu UV:%ux%u=>%lu Size=%lu", frame->y_width,
	       frame->y_height, frame->nb_y, frame->uv_width,
	       frame->uv_height, frame->nb_uv, frame->length);
  mjpeg_debug
    ("yuv: Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u",
     yuv_correct->Gamma, yuv_correct->InputYmin, yuv_correct->InputYmax,
     yuv_correct->OutputYmin, yuv_correct->OutputYmax);
  // Master loop : continue until there is no next frame in stdin
  while ((err = yuvcorrect_y4m_read_frame (0, &gen_correct->streaminfo, frame, gen_correct->line_switch)) == Y4M_OK)
    {
      if (overall->stat == 1)
	yuvstat (frame);

      mjpeg_info ("Frame number %ld", frame_num);

      // Time reordering
      if (gen_correct->field_move != 0)
	{
	  oddeven = frame_num & (unsigned long int) 1;	// oddeven = frame_num % 2, fast implementation
	  if (gen_correct->field_move == 1)
	    {
	      // Bottom field one frame forward
	      if (frame_num == 0)
		{
		  bottom_field_storage (frame, oddeven, frame->field1, frame->field2);
		  if (yuvcorrect_y4m_read_frame (0, &gen_correct->streaminfo, frame, gen_correct->line_switch) !=
		      Y4M_OK)
		    mjpeg_error_exit1 ("Can't read frame %ld", frame_num);
		  frame_num++;
		  oddeven = frame_num & (unsigned long int) 1;
		  mjpeg_info ("Frame number %ld", frame_num);
		}
	      bottom_field_storage (frame, oddeven, frame->field1, frame->field2);
	      bottom_field_replace (frame, oddeven, frame->field1, frame->field2);
	    }
	  else
	    {
	      // Top field one frame forward
	      if (frame_num == 0)
		{
		  top_field_storage (frame, oddeven, frame->field1, frame->field2);
		  if (yuvcorrect_y4m_read_frame (0, &gen_correct->streaminfo, frame, gen_correct->line_switch) !=
		      Y4M_OK)
		    mjpeg_error_exit1 ("Can't read frame %ld", frame_num);
		  frame_num++;
		  oddeven = frame_num & (unsigned long int) 1;
		  mjpeg_info ("Frame number %ld", frame_num);
		}
	      top_field_storage (frame, oddeven, frame->field1, frame->field2);
	      top_field_replace (frame, oddeven, frame->field1, frame->field2);
	    }
	}


      if (overall->rgbfirst == 1)
	{
	  // RGB correction
	  if (rgb_correct->rgb == 1)
	    yuvcorrect_RGB_treatment (frame, rgb_correct);
	}
      // luminance correction
      if (yuv_correct->luma == 1)
	yuvcorrect_luminance_treatment (frame, yuv_correct);
      // chrominance correction
      if (yuv_correct->chroma == 1)
	yuvcorrect_chrominance_treatment (frame, yuv_correct);
      if (overall->rgbfirst != 1)
	{
	  // RGB correction
	  if (rgb_correct->rgb == 1)
	    yuvcorrect_RGB_treatment (frame, rgb_correct);
	}

      // Output Frame Header
      if (y4m_write_frame_header (1, &gen_correct->streaminfo, &frame->info) != Y4M_OK)
	goto out_error;

      // Output Frame content
      if (y4m_write (1, frame->y, frame->length) != Y4M_OK)
	goto out_error;

      frame_num++;

    }
  // End of master loop => no more frame in stdin

  if (err != Y4M_ERR_EOF)
    mjpeg_error_exit1 ("Couldn't read frame number %ld!", frame_num);
  else
    mjpeg_info ("Normal exit: end of stream with frame number %ld!",
		frame_num);
  y4m_fini_stream_info (&gen_correct->streaminfo);
  y4m_fini_frame_info (&frame->info);
  return 0;


out_error:
  mjpeg_error_exit1 ("Unable to write to output - aborting!");
  return 1;
}

/* 
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
