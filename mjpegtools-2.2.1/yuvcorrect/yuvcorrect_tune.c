/*
  *  yuvcorrect_tune.c
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
// November: RGB corrections available as well as image splitting
// TODO:
// S, D and Q keystrokes

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include "yuv4mpeg.h"
#include "yuvcorrect.h"

#define yuvcorrect_tune_VERSION "26-11-2002"
// For pointer adress alignement
extern const uint16_t ALIGNEMENT;		// 16 bytes alignement for mmx registers in SIMD instructions for Pentium
extern const char *legal_opt_flags;

const float gamma_increment=0.01;

const char FULL[]           = "FULL";
const char HALF_LEFT[]      = "HALF_LEFT";
const char HALF_RIGHT[]     = "HALF_RIGHT";
const char QUARTER_LEFT[]   = "QUARTER_LEFT";
const char QUARTER_RIGHT[]  = "QUARTER_RIGHT";
const char TQUARTER_LEFT[]  = "3QUARTER_LEFT";
const char TQUARTER_RIGHT[] = "3QUARTER_RIGHT";
const char TOP[]            = "TOP";
const char BOTTOM[]         = "BOTTOM";
const char RGBFIRST[]       = "RGBFIRST";

void yuvcorrect_tune_print_usage (void);
void add_reference_part(frame_t *frame,uint8_t *final_ref,uint8_t type);
void yuvcorrect_tune_handle_args (int argc, char *argv[], overall_t *overall, 
				  general_correction_t *gen_correct);


// *************************************************************************************
void
yuvcorrect_tune_print_usage (void)
{
  fprintf (stderr,
	   "usage: yuvcorrect_tune -I <filename> -F <Ref Frame> -M [mode_keyword] -Y [yuv_keyword] -R [RGB_keyword]  [-v 0-2] [-h]\n"
	   "yuvcorrect_tune is an interactive tool enabling you to interactively tune different\n"
	   "corrections related to interlacing, luminance and color. <filename> defines the uncorrected\n"
	   "reference yuv frames (in yuv4MPEG 4:2:0 format)\n"
	   "Typical use is 'lav2yuv -f 1 <videofile> > frame ; cat fifo | yuvplay ; yuvcorrect_tune -I frame <keywords> > fifo' \n"
	   "\n"
	   "yuvcorrect_tune is keyword driven :\n"
	   "\t -I <filename>\n"
	   "\t -F <Ref Frame> (optional)\n"
	   "\t -M for keyword concerning the correction MODE of yuvcorrect_tune\n"
	   "\t -Y for keyword concerning color corrections in the YUV space\n"
	   "\t -R for keyword concerning color corrections in the RGB space\n"
	   "\n"
	   "\t Yuvcorrect_tune uses _only_ the first frame it reads from <filename> and all corrections defined on command\n"
	   "\t line will then be applied to this frame to generate the image frame.\n"
	   "\t It is to this image frame that color corrections will be applied, as defined by succesive keystrokes\n"
	   "\t on the keyboard. By default, the resulting corrected frame is then outputted to stdout.\n"
	   "\t But - using mode keywords - part of this corrected frame may be replaced by the corresponding part of a\n"
	   "\t Ref frame (-F keyword) like colorbars, thus enabling to visually adjust correction to match the reference frame.\n"
	   "\n"
	   "Possible mode keyword are:\n"
	   "\t FULL (default)\n"
	   "\t HALF_LEFT      The generated frame is now constituted of two different parts:\n"
	   "\t                the left half shows the final reference uncorrected while the right half shows it corrected\n"
	   "\t HALF_RIGHT     Same as HALF_LEFT except right and left are inversed\n"
	   "\t QUARTER_LEFT   Same as HALF_LEFT except the uncorrected part is only a quarter of the image\n"
	   "\t QUARTER_RIGHT  Same as HALF_RIGHT except the uncorrected part is only a quarter of the image\n"
	   "\t 3QUARTER_LEFT  The left three quarters of the image is uncorrected, the right quarter corrected\n"
	   "\t 3QUARTER_RIGHT The right three quarters of the image is uncorrected, the left quarter corrected\n"
	   "\t TOP            the top half is uncorrected, the bottom half is corrected\n"
	   "\t BOTTOM         the bottom half is uncorrected, the top half is corrected\n"
	   "\t RGBFIRST       to have yuvcorrect_tune apply RGB corrections first, then YUV corrections\n"
	   "\n"
	   "Possible yuv keywords are:\n"
	   "\t LUMINANCE_Gamma_InputYmin_InputYmax_OutputYmin_OutputYmax\n"
	   "\t Y_Gamma_InputYmin_InputYmax_OutputYmin_OutputYmax\n"
	   "\t    to correct the input frame luminance by clipping it inside range [InputYmin;InputYmax],\n"
	   "\t    scale with power (1/Gamma), and expand/shrink/shift it to [OutputYmin;OutputYmax]\n"
	   "\t CHROMINANCE_UVrotation_Ufactor_Urotcenter_Vfactor_Vrotcenter_UVmin_UVmax\n"
	   "\t UV_UVrotation_Ufactor_Urotcenter_Vfactor_Vrotcenter_UVmin_UVmax\n"
	   "\t    to rotate rescaled UV chroma components Ufactor*(U-Urotcenter) and Vfactor*(V-Vrotcenter)\n"
	   "\t    by (float) UVrotation degrees, recenter to the normalize (128,128) center,\n"
	   "\t    and _clip_ the result to range [UVmin;UVmax]\n"
	   "\t CONFORM to have yuvcorrect_tune generate frames conform to the Rec.601 specification for Y'CbCr frames\n"
	   "\t    that is LUMINANCE_1.0_16_235_16_235 and CHROMINANCE_0.0_1.0_128_1.0_128_16_240\n"
	   "\n"
	   "Possible RGB keywords are:\n"
	   "\t R_Gamma_InputRmin_InputRmax_OutputRmin_OutputRmax\n"
	   "\t G_Gamma_InputGmin_InputGmax_OutputGmin_OutputGmax\n"
	   "\t B_Gamma_InputBmin_InputBmax_OutputBmin_OutputBmax\n"
	   "\t    to correct the input frame RGB color by clipping it inside range [InputRGBmin;InputRGBmax],\n"
	   "\t    scale with power (1/Gamma), and expand/shrink/shift it to [OutputRGBmin;OutputRGBmax]\n"
	   "\n"
	   "How to use the keyboard:\n"
	   "Pressing Y, U, V, R, G, B or M will tell yuvcorrect_tune that all following keystrokes refer to corrections applied to\n"
	   "Y values, UV values, UV values, R values, G values, B values, Mode of yuvcorrect_tune ; until ESCAPE is pressed\n"
	   "To define corrections, use Capital letter to increase and minor letter to decrease\n"
	   "Gamma values are changed by amount of 0.01, Ufactor and Vfactor by 0.05; UVrotation by amount of 1 degree; Integer values by 1\n"
	   "To modify the 1st, 2nd, 3rd, 4th, 5th, 6th or 7th parameter, use respectively e/E, r/R, t/T, y/Y, u/U, i/I, o/O\n"
	   "To modify yuvcorrect_tune mode, use keypad values 0, 1, 2, 3, 4, 5, 6, 7, 8, 9\n"
	   "For status, press S. To go back to default, press D. To quit, press Q.\n"
	   "-v  Specifies the degree of verbosity: 0=quiet, 1=normal, 2=verbose/debug\n"
	   "-h : print this lot!\n");
  exit (1);
}
// *************************************************************************************



// *************************************************************************************
void
handle_args_overall (int argc, char *argv[], overall_t *overall)
{
  int c,verb;

   optind = 1;
   while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
     {
	switch (c)
	  {
	   case 'I':
	     if ((overall->ImgFrame=open(optarg,O_RDONLY))==-1)
	       mjpeg_error_exit1("Unable to open %s!!",optarg);
	     break;

	   case 'F':
	     if ((overall->RefFrame=open(optarg,O_RDONLY))==-1)
	       mjpeg_error_exit1("Unable to open %s!!",optarg);
	     break;

	   case 'v':
	     verb = atoi (optarg);
	     if (verb < 0 || verb > 2)
	       {
		  mjpeg_info ("Verbose level must be 0, 1 or 2 ! => resuming to default 1");
	       } 
	     else 
	       {
		  overall->verbose = verb;
	       }
	     break;
	     
	   case 'h':
	     yuvcorrect_tune_print_usage ();
	     break;
	     
	   default:
	     break;
	  }
     }
   if ((optind != argc)||(overall->ImgFrame==-1))
     yuvcorrect_tune_print_usage ();
   if (overall->RefFrame==-1) 
     overall->RefFrame=overall->ImgFrame;
}
// *************************************************************************************



// *************************************************************************************
void
yuvcorrect_tune_handle_args (int argc, char *argv[], overall_t *overall, general_correction_t *gen_correct)
{
  // This function handles argument passing on the command line
  int c;
  int k_mode;

   // Ne pas oublier de mettre la putain de ligne qui suit, sinon, plus d'argument à la lign de commande, ils auront été bouffés par l'appel précédnt à getopt!!
   optind=1;
   while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
    {
      switch (c)
	{
	  // **************            
	  // MODE KEYOWRD
	  // *************
	 case 'M':
	  k_mode = 0;
	  if (strcmp (optarg, RGBFIRST) == 0)
	    {
	      k_mode = 1;
	      overall->rgbfirst = 1;
	    }
	  if (strcmp (optarg, FULL) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 0;
	    }
	  if (strcmp (optarg, HALF_LEFT) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 1;
	    }
	  if (strcmp (optarg, HALF_RIGHT) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 2;
	    }
	  if (strcmp (optarg, QUARTER_LEFT) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 3;
	    }
	  if (strcmp (optarg, QUARTER_RIGHT) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 4;
	    }
	  if (strcmp (optarg, TQUARTER_LEFT) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 5;
	    }
	  if (strcmp (optarg, TQUARTER_RIGHT) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 6;
	    }
	   if (strcmp (optarg, TOP) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 7;
	    }
	  if (strcmp (optarg, BOTTOM) == 0)
	    {
	      k_mode = 1;
	      overall->mode = 8;
	    }
	  if (k_mode == 0)
	    mjpeg_error_exit1 ("Unrecognized MODE keyword: %s", optarg);
	  break;
	  // *************

	default:
	  break;
	}
    }


}
// *************************************************************************************


// *************************************************************************************
void add_reference_part(frame_t *frame,uint8_t *final_ref,uint8_t type)
{

   uint8_t *u_c_p_frame,*u_c_p_ref;
   unsigned long int length_y,length_uv,offset3,offset4;
   uint16_t i;
   u_c_p_frame=frame->y;
   u_c_p_ref  =final_ref;
   
   switch(type+'0')
     {
      case '1':
	// "HALF_LEFT";
	length_y=frame->y_width>>1;
	length_uv=length_y>>1;
	for (i=0;i<frame->y_height;i++,u_c_p_frame+=frame->y_width,u_c_p_ref+=frame->y_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_y);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '2':
	// "HALF_RIGHT";
	length_y=frame->y_width>>1;
	length_uv=length_y>>1;
	offset3=length_y>>1;
	u_c_p_frame+=length_y;
	u_c_p_ref  +=length_y;
	for (i=0;i<frame->y_height;i++,u_c_p_frame+=frame->y_width,u_c_p_ref+=frame->y_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_y);
	u_c_p_frame-=offset3;
	u_c_p_ref  -=offset3;
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '3':
	// "QUARTER_LEFT";
	length_y=frame->y_width>>2;
	length_uv=length_y>>1;
	for (i=0;i<frame->y_height;i++,u_c_p_frame+=frame->y_width,u_c_p_ref+=frame->y_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_y);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '4':
	// "QUARTER_RIGHT";
	length_y=frame->y_width>>2;
	length_uv=length_y>>1;
	offset3=3*length_y;
	offset4=offset3>>1;
	u_c_p_frame+=offset3;
	u_c_p_ref  +=offset3;
	for (i=0;i<frame->y_height;i++,u_c_p_frame+=frame->y_width,u_c_p_ref+=frame->y_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_y);
	u_c_p_frame-=offset4;
	u_c_p_ref  -=offset4;
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '5':
	// "3QUARTER_LEFT";
	length_y=(3*frame->y_width)>>2;
	length_uv=length_y>>1;
	for (i=0;i<frame->y_height;i++,u_c_p_frame+=frame->y_width,u_c_p_ref+=frame->y_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_y);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '6':
	// "3QUARTER_RIGHT";
	length_y=(3*frame->y_width)>>2;
	length_uv=length_y>>1;
	offset3=(frame->y_width>>2);
	offset4=offset3>>1; 
	u_c_p_frame+=offset3;
	u_c_p_ref  +=offset3;
	for (i=0;i<frame->y_height;i++,u_c_p_frame+=frame->y_width,u_c_p_ref+=frame->y_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_y);
	u_c_p_frame-=offset4;
	u_c_p_ref  -=offset4;
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	for (i=0;i<frame->uv_height;i++,u_c_p_frame+=frame->uv_width,u_c_p_ref+=frame->uv_width) 
	  memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '7':
	// "TOP";
	length_y =frame->nb_y>>1;
	length_uv=length_y>>2;
	memcpy(u_c_p_frame,u_c_p_ref,length_y);
	u_c_p_frame=frame->u;
	u_c_p_ref+=frame->nb_y;
	memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	u_c_p_frame=frame->v;
	u_c_p_ref+=frame->nb_uv;
	memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;
      case '8':
	// "BOTTOM";
	length_y =frame->nb_y>>1;
	length_uv=length_y>>2;
	offset3  =frame->nb_y>>1;
	offset4  =length_y+length_uv;
	u_c_p_frame+=offset3;
	u_c_p_ref  +=offset3;
	memcpy(u_c_p_frame,u_c_p_ref,length_y);
	u_c_p_frame+=offset4;
	u_c_p_ref  +=offset4;
	memcpy(u_c_p_frame,u_c_p_ref,length_uv);
	u_c_p_frame+=(length_uv<<1);
	u_c_p_ref  +=(length_uv<<1);
	memcpy(u_c_p_frame,u_c_p_ref,length_uv);
      break;

      default:
      break;
	
     }
   
}
// *************************************************************************************


// *************************************************************************************
// MAIN
// *************************************************************************************
int
main (int argc, char *argv[])
{
   
  // Defining yuvcorrect dedicated structures (see yuvcorrect.h) 
  overall_t *overall=NULL;
  frame_t *frame=NULL;
  general_correction_t *gen_correct=NULL;
  yuv_correction_t *yuv_correct=NULL;
  rgb_correction_t *rgb_correct=NULL;
  ref_frame_t *ref_frame=NULL;
   
  int c,err;
  uint8_t quit=0,current_correction=0;
  uint8_t *u_c_p, *image_frame;	//u_c_p = uint8_t pointer

  // Information output
  mjpeg_info("yuvcorrect_tune "VERSION" ("yuvcorrect_tune_VERSION") is an interactive image correction utility for yuv frames");
  mjpeg_info
    ("(C) 2002 Xavier Biquard <xbiquard@free.fr>, yuvcorrect_tune -h for usage, or man yuvcorrect_tune");

   // START OF INITIALISATION 
   // START OF INITIALISATION 
   // START OF INITIALISATION 
   // yuvcorrect overall structure initialisation
   if (!(overall = malloc(sizeof(overall_t))))
     mjpeg_error_exit1("Could not allocate memory for overall structure pointer");
   overall->verbose=1;
   overall->mode=overall->stat=0;
   overall->RefFrame=overall->ImgFrame=-1;
   handle_args_overall(argc, argv, overall);
   mjpeg_default_handler_verbosity (overall->verbose);
   
   mjpeg_debug("Start of initialisation");

     
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
   // ref_frame
   if (!(ref_frame = (ref_frame_t *) malloc (sizeof (ref_frame_t))))
     mjpeg_error_exit1
     ("Could not allocate memory for ref_frame structure pointer");


   initialisation1(overall->ImgFrame,frame,gen_correct,yuv_correct,rgb_correct);
   // Deal with args 
   handle_args_yuv_rgb (argc, argv, yuv_correct, rgb_correct);
   yuvcorrect_tune_handle_args (argc, argv, overall, gen_correct);
   // Specific to yuvcorrect_tune: automatically activate yuv and rgb corrections
   yuv_correct->luma = yuv_correct->chroma = rgb_correct->rgb = 1;
   initialisation2(yuv_correct,rgb_correct);
     
   mjpeg_debug ("End of Initialisation");
   // END OF INITIALISATION
   // END OF INITIALISATION
   // END OF INITIALISATION


  mjpeg_debug("overall: verbose=%u, mode=%d, stat=%u",overall->verbose,overall->mode,overall->stat);
  mjpeg_debug("frame: Y:%ux%u=>%lu UV:%ux%u=>%lu Size=%lu",frame->y_width,frame->y_height,frame->nb_y,frame->uv_width,frame->uv_height,frame->nb_uv,frame->length);
  mjpeg_debug("yuv: Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u",yuv_correct->Gamma,yuv_correct->InputYmin,yuv_correct->InputYmax,yuv_correct->OutputYmin,yuv_correct->OutputYmax);
   
   mjpeg_debug("yuv: Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u",yuv_correct->Gamma,yuv_correct->InputYmin,yuv_correct->InputYmax,yuv_correct->OutputYmin,yuv_correct->OutputYmax);
   if (!(u_c_p = malloc (frame->length + ALIGNEMENT)))
     mjpeg_error_exit1  ("Could not allocate memory for frame table. STOP!");
   mjpeg_debug ("before alignement: %p", u_c_p);
   if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
     u_c_p =(uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) * ALIGNEMENT);
   mjpeg_debug ("after alignement: %p", u_c_p);
   image_frame=u_c_p;
   // Read image frame and apply corrections eventually defined on command line
   if(yuvcorrect_y4m_read_frame (overall->ImgFrame, &gen_correct->streaminfo, frame, gen_correct->line_switch) != Y4M_OK)
     mjpeg_error_exit1("Unable to read image frame. Aborting now !!");
   close(overall->ImgFrame);
   
   if (overall->RefFrame!=overall->ImgFrame) 
     {
	// Specific to yuvcorrect_tune: ref frame
	// ref_frame
	ref_frame_init(overall->RefFrame,ref_frame);
	// Coherence check
	if ((ref_frame->width!=frame->y_width)||(ref_frame->height!=frame->y_height))
	  mjpeg_error_exit1("Width and Height of Image and Ref Frame differ. aborting now!!");
	// Read ref frame
	if (y4m_read_frame_header (overall->RefFrame, &gen_correct->streaminfo, &ref_frame->info) == Y4M_OK)
	  {
	     if ((err = y4m_read (overall->RefFrame, ref_frame->ref, frame->length)) != Y4M_OK)
	       mjpeg_error_exit1 ("Couldn't read FRAME content: %s!",y4m_strerr (err));
	  }
	else 
	  mjpeg_error_exit1("Unable to read ref frame. Aborting now !!");
	close(overall->RefFrame);
     }
   
   
   mjpeg_info("Y_%f_%u_%u_%u_%u",yuv_correct->Gamma,yuv_correct->InputYmin,yuv_correct->InputYmax,yuv_correct->OutputYmin,yuv_correct->OutputYmax);
   mjpeg_info("UV_%f_%f_%u_%f_%u_%u_%u",yuv_correct->UVrotation,yuv_correct->Ufactor,yuv_correct->Urotcenter,
	      yuv_correct->Vfactor,yuv_correct->Vrotcenter,yuv_correct->UVmin,yuv_correct->UVmax);
   mjpeg_info("R_%f_%u_%u_%u_%u",rgb_correct->RGamma,rgb_correct->InputRmin,rgb_correct->InputRmax,rgb_correct->OutputRmin,rgb_correct->OutputRmax);
   mjpeg_info("G_%f_%u_%u_%u_%u",rgb_correct->GGamma,rgb_correct->InputGmin,rgb_correct->InputGmax,rgb_correct->OutputGmin,rgb_correct->OutputGmax);
   mjpeg_info("B_%f_%u_%u_%u_%u",rgb_correct->BGamma,rgb_correct->InputBmin,rgb_correct->InputBmax,rgb_correct->OutputBmin,rgb_correct->OutputBmax);
   if (overall->rgbfirst == 1)
     yuvcorrect_RGB_treatment (frame, rgb_correct);
   // luminance correction
   yuvcorrect_luminance_treatment (frame, yuv_correct);
   // chrominance correction
   yuvcorrect_chrominance_treatment (frame, yuv_correct);
   if (overall->rgbfirst != 1)
     yuvcorrect_RGB_treatment (frame, rgb_correct);
   
   y4m_write_stream_header (1, &gen_correct->streaminfo);
   // Now, frame->y points on the corrected frame. Store it inside image_frame
   memcpy(image_frame,frame->y,frame->length);
   // Output Frame Header
   if (y4m_write_frame_header (1, &gen_correct->streaminfo, &frame->info) != Y4M_OK)
     goto out_error;
   // Output Frame content
   if (y4m_write (1, frame->y, frame->length) != Y4M_OK)
     goto out_error;
   
   // Master loop : continue until there is no more keystrokes
   while (((c=getc(stdin))!=EOF)&&(quit!=1)) 
     {
	if (c!='\n') 
	  {
	if (current_correction==0)
	    {
	     // this keystroke may defined the current_correction
	     switch(c) 
	       {
		case 'y' :
		case 'Y' :
		  // Activate Luminance corrections
		  current_correction=1;
		  mjpeg_info("Luminance corrections activated");
		  break;
		
		case 'u' :
		case 'U' :
		case 'v' :
		case 'V' :
		  // Activate Chrominance corrections
		  current_correction=2;
		  mjpeg_info("Chrominance corrections activated");
		  break;
		 
		case 'r' :
		case 'R' :
		  // Activate RED corrections
		  current_correction=3;
		  mjpeg_info("RED corrections activated");
		  break;
		  
		case 'g' :
		case 'G' :
		  // Activate GREEN corrections
		  current_correction=4;
		  mjpeg_info("GREEN corrections activated");
		  break;
		  
		case 'b' :
		case 'B' :
		  // Activate BLUE corrections
		  current_correction=5;
		  mjpeg_info("BLUE corrections activated");
		  break;
		
		case 'm' :
		case 'M' :
		  // Activate MODE corrections
		  current_correction=6;
		  mjpeg_info("MODE corrections activated");
		  break;
	       }
	       c = '\n';
	  }
	if (c=='\e') 
	  {
	     current_correction=0;
	     mjpeg_info("ESCAPE!!! => next (valid) keystroke defined which correction type will be activated");
	  }
	
	
	if ((c=='Q')||(c=='q'))
	  quit=1;
	
	if ((c=='D')||(c=='d'))
	  {
	     // Go back to default values
	     mjpeg_info("Not implemented yet. Sorry ... please relanch yuvcorrect_tune.\n");
	  }
	
	if ((c=='S')||(c=='s'))
	  {
	     mjpeg_info("Y_%f_%u_%u_%u_%u",yuv_correct->Gamma,yuv_correct->InputYmin,yuv_correct->InputYmax,yuv_correct->OutputYmin,yuv_correct->OutputYmax);
	     mjpeg_info("UV_%f_%f_%u_%f_%u_%u_%u",yuv_correct->UVrotation,yuv_correct->Ufactor,yuv_correct->Urotcenter,
			yuv_correct->Vfactor,yuv_correct->Vrotcenter,yuv_correct->UVmin,yuv_correct->UVmax);
	     mjpeg_info("R_%f_%u_%u_%u_%u",rgb_correct->RGamma,rgb_correct->InputRmin,rgb_correct->InputRmax,rgb_correct->OutputRmin,rgb_correct->OutputRmax);
	     mjpeg_info("G_%f_%u_%u_%u_%u",rgb_correct->GGamma,rgb_correct->InputGmin,rgb_correct->InputGmax,rgb_correct->OutputGmin,rgb_correct->OutputGmax);
	     mjpeg_info("B_%f_%u_%u_%u_%u",rgb_correct->BGamma,rgb_correct->InputBmin,rgb_correct->InputBmax,rgb_correct->OutputBmin,rgb_correct->OutputBmax);
	  }
	
	
	if (current_correction!=0) 
	  {
	     if (current_correction==6) 
	       {
		  // MODE output corrections
		  switch(c)
		    {
		     case '0' :
		       overall->mode=0;
		       break;
		     case '1' :
		       overall->mode=1;
		       break;
		     case '2' :
		       overall->mode=2;
		       break;
		     case '3' :
		       overall->mode=3;
		       break;
		     case '4' :
		       overall->mode=4;
		       break;
		     case '5' :
		       overall->mode=5;
		       break;
		     case '6' :
		       overall->mode=6;
		       break;
		     case '7' :
		       overall->mode=7;
		       break;
		     case '8' :
		       overall->mode=8;
		       break;
		     case '9' :
		       if (overall->rgbfirst==1)
			 overall->rgbfirst=0;
		       else
			 overall->rgbfirst=1;
		     break;
		     break;
		       
		     default:
		     break;
		    }
	       }
	     
	     if (current_correction==1) 
	       {
		  switch(c) 
		    {
		     case 'E':  // Increase first parameter
		       yuv_correct->Gamma+=gamma_increment;
		       break;
		  
		     case 'e':  // Decrease first parameter
		       if (yuv_correct->Gamma<=gamma_increment)
			 mjpeg_info("Gamma value would become negative!! Ignoring");
		       else
			 yuv_correct->Gamma-=gamma_increment;
		       break;
		  
		     case 'R':  // Increase Second parameter
		       if ((yuv_correct->InputYmin==255)||(yuv_correct->InputYmin==yuv_correct->InputYmax))
			 mjpeg_info("InputYmin would be greater than 255 or InputYmax!! Ignoring");
		       else
			 yuv_correct->InputYmin+=1;
		       break;
		       
		     case 'r':  // Decrease second parameter
		       if (yuv_correct->InputYmin==0)
			 mjpeg_info("InputYmin would be smaller than 0!! Ignoring");
		       else
			 yuv_correct->InputYmin-=1;
		       break;
		       
		     case 'T':  // Increase third parameter
		       if (yuv_correct->InputYmax==255)
			 mjpeg_info("InputYmax would be greater than 255!! Ignoring");
		       else
			 yuv_correct->InputYmax+=1;
		       break;
		  
		     case 't':  // Decrease third parameter
		       if ((yuv_correct->InputYmax==0)||(yuv_correct->InputYmax==yuv_correct->InputYmin))
			 mjpeg_info("InputYmax would be smaller than 0 or InputYmin!! Ignoring");
		       else
			 yuv_correct->InputYmax-=1;
		       break;
		       
		     case 'Y':  // Increase fourth parameter
		       if ((yuv_correct->OutputYmin==255)||(yuv_correct->OutputYmin==yuv_correct->OutputYmax))
			 mjpeg_info("OutputYmin would be greater than 255 or OutputYmax!! Ignoring");
		       else
			 yuv_correct->OutputYmin+=1;
		       break;
		  
		     case 'y':  // Decrease fourth parameter
		       if (yuv_correct->OutputYmin==0)
			 mjpeg_info("OutputYmin would be smaller than 0!! Ignoring");
		       else
			 yuv_correct->OutputYmin-=1;
		       break;
		  
		     case 'U':  // Increase fifth parameter
		       if (yuv_correct->OutputYmax==255)
			 mjpeg_info("OutputYmax would be greater than 255!! Ignoring");
		       else
			 yuv_correct->OutputYmax+=1;
		       break;
		  
		     case 'u':  // Decrease fifth parameter
		       if ((yuv_correct->OutputYmax==0)||(yuv_correct->OutputYmax==yuv_correct->OutputYmin))
			 mjpeg_info("OutputYmax would be smaller than 0 or OutputYmin!! Ignoring");
		       else
			 yuv_correct->OutputYmax-=1;
		       break;
		       
		     default :
		       break;
		    }
	       }
	     
	     if (current_correction==3)
	       {
		  switch(c) 
		    {
		     case 'E':  // Increase first parameter
		       rgb_correct->RGamma+=gamma_increment;
		       break;
		  
		     case 'e':  // Decrease first parameter
		       if (rgb_correct->RGamma<=gamma_increment)
			 mjpeg_info("RGamma value would become negative!! Ignoring");
		       else
			 rgb_correct->RGamma-=gamma_increment;
		       break;
		  
		     case 'R':  // Increase Second parameter
		       if ((rgb_correct->InputRmin==255)||(rgb_correct->InputRmin==rgb_correct->InputRmax))
			 mjpeg_info("InputRmin would be greater than 255 or InputRmax!! Ignoring");
		       else
			 rgb_correct->InputRmin+=1;
		       break;
		       
		     case 'r':  // Decrease second parameter
		       if (rgb_correct->InputRmin==0)
			 mjpeg_info("InputRmin would be smaller than 0!! Ignoring");
		       else
			 rgb_correct->InputRmin-=1;
		       break;
		       
		     case 'T':  // Increase third parameter
		       if (rgb_correct->InputRmax==255)
			 mjpeg_info("InputRmax would be greater than 255!! Ignoring");
		       else
			 rgb_correct->InputRmax+=1;
		       break;
		  
		     case 't':  // Decrease third parameter
		       if ((rgb_correct->InputRmax==0)||(rgb_correct->InputRmax==rgb_correct->InputRmin))
			 mjpeg_info("InputRmax would be smaller than 0 or InputRmin!! Ignoring");
		       else
			 rgb_correct->InputRmax-=1;
		       break;
		       
		     case 'Y':  // Increase fourth parameter
		       if ((rgb_correct->OutputRmin==255)||(rgb_correct->OutputRmin==rgb_correct->OutputRmax))
			 mjpeg_info("OutputRmin would be greater than 255 or OutputRmax!! Ignoring");
		       else
			 rgb_correct->OutputRmin+=1;
		       break;
		  
		     case 'y':  // Decrease fourth parameter
		       if (rgb_correct->OutputRmin==0)
			 mjpeg_info("OutputRmin would be smaller than 0!! Ignoring");
		       else
			 rgb_correct->OutputRmin-=1;
		       break;
		  
		     case 'U':  // Increase fifth parameter
		       if (rgb_correct->OutputRmax==255)
			 mjpeg_info("OutputRmax would be greater than 255!! Ignoring");
		       else
			 rgb_correct->OutputRmax+=1;
		       break;
		  
		     case 'u':  // Decrease fifth parameter
		       if ((rgb_correct->OutputRmax==0)||(rgb_correct->OutputRmax==rgb_correct->OutputRmin))
			 mjpeg_info("OutputRmax would be smaller than 0 or OutputRmin!! Ignoring");
		       else
			 rgb_correct->OutputRmax-=1;
		       break;
		       
		     default :
		       break;
		    }
	       }
	     
	     if (current_correction==4)
	       {
		  switch(c) 
		    {
		     case 'E':  // Increase first parameter
		       rgb_correct->GGamma+=gamma_increment;
		       break;
		  
		     case 'e':  // Decrease first parameter
		       if (rgb_correct->GGamma<=gamma_increment)
			 mjpeg_info("GGamma value would become negative!! Ignoring");
		       else
			 rgb_correct->GGamma-=gamma_increment;
		       break;
		  
		     case 'R':  // Increase Second parameter
		       if ((rgb_correct->InputGmin==255)||(rgb_correct->InputGmin==rgb_correct->InputGmax))
			 mjpeg_info("InputGmin would be greater than 255 or InputGmax!! Ignoring");
		       else
			 rgb_correct->InputGmin+=1;
		       break;
		       
		     case 'r':  // Decrease second parameter
		       if (rgb_correct->InputGmin==0)
			 mjpeg_info("InputGmin would be smaller than 0!! Ignoring");
		       else
			 rgb_correct->InputGmin-=1;
		       break;
		       
		     case 'T':  // Increase third parameter
		       if (rgb_correct->InputGmax==255)
			 mjpeg_info("InputGmax would be greater than 255!! Ignoring");
		       else
			 rgb_correct->InputGmax+=1;
		       break;
		  
		     case 't':  // Decrease third parameter
		       if ((rgb_correct->InputGmax==0)||(rgb_correct->InputGmax==rgb_correct->InputGmin))
			 mjpeg_info("InputGmax would be smaller than 0 or InputGmin!! Ignoring");
		       else
			 rgb_correct->InputGmax-=1;
		       break;
		       
		     case 'Y':  // Increase fourth parameter
		       if ((rgb_correct->OutputGmin==255)||(rgb_correct->OutputGmin==rgb_correct->OutputGmax))
			 mjpeg_info("OutputGmin would be greater than 255 or OutputGmax!! Ignoring");
		       else
			 rgb_correct->OutputGmin+=1;
		       break;
		  
		     case 'y':  // Decrease fourth parameter
		       if (rgb_correct->OutputGmin==0)
			 mjpeg_info("OutputGmin would be smaller than 0!! Ignoring");
		       else
			 rgb_correct->OutputGmin-=1;
		       break;
		  
		     case 'U':  // Increase fifth parameter
		       if (rgb_correct->OutputGmax==255)
			 mjpeg_info("OutputGmax would be greater than 255!! Ignoring");
		       else
			 rgb_correct->OutputGmax+=1;
		       break;
		  
		     case 'u':  // Decrease fifth parameter
		       if ((rgb_correct->OutputGmax==0)||(rgb_correct->OutputGmax==rgb_correct->OutputGmin))
			 mjpeg_info("OutputGmax would be smaller than 0 or OutputGmin!! Ignoring");
		       else
			 rgb_correct->OutputGmax-=1;
		       break;
		       
		     default :
		       break;
		    }
	       }
	     
	     if (current_correction==5)
	       {
		  switch(c) 
		    {
		     case 'E':  // Increase first parameter
		       rgb_correct->BGamma+=gamma_increment;
		       break;
		  
		     case 'e':  // Decrease first parameter
		       if (rgb_correct->BGamma<=gamma_increment)
			 mjpeg_info("BGamma value would become negative!! Ignoring");
		       else
			 rgb_correct->BGamma-=gamma_increment;
		       break;
		  
		     case 'R':  // Increase Second parameter
		       if ((rgb_correct->InputBmin==255)||(rgb_correct->InputBmin==rgb_correct->InputBmax))
			 mjpeg_info("InputBmin would be greater than 255 or InputBmax!! Ignoring");
		       else
			 rgb_correct->InputBmin+=1;
		       break;
		       
		     case 'r':  // Decrease second parameter
		       if (rgb_correct->InputBmin==0)
			 mjpeg_info("InputBmin would be smaller than 0!! Ignoring");
		       else
			 rgb_correct->InputBmin-=1;
		       break;
		       
		     case 'T':  // Increase third parameter
		       if (rgb_correct->InputBmax==255)
			 mjpeg_info("InputBmax would be greater than 255!! Ignoring");
		       else
			 rgb_correct->InputBmax+=1;
		       break;
		  
		     case 't':  // Decrease third parameter
		       if ((rgb_correct->InputBmax==0)||(rgb_correct->InputBmax==rgb_correct->InputBmin))
			 mjpeg_info("InputBmax would be smaller than 0 or InputBmin!! Ignoring");
		       else
			 rgb_correct->InputBmax-=1;
		       break;
		       
		     case 'Y':  // Increase fourth parameter
		       if ((rgb_correct->OutputBmin==255)||(rgb_correct->OutputBmin==rgb_correct->OutputBmax))
			 mjpeg_info("OutputBmin would be greater than 255 or OutputBmax!! Ignoring");
		       else
			 rgb_correct->OutputBmin+=1;
		       break;
		  
		     case 'y':  // Decrease fourth parameter
		       if (rgb_correct->OutputBmin==0)
			 mjpeg_info("OutputBmin would be smaller than 0!! Ignoring");
		       else
			 rgb_correct->OutputBmin-=1;
		       break;
		  
		     case 'U':  // Increase fifth parameter
		       if (rgb_correct->OutputBmax==255)
			 mjpeg_info("OutputBmax would be greater than 255!! Ignoring");
		       else
			 rgb_correct->OutputBmax+=1;
		       break;
		  
		     case 'u':  // Decrease fifth parameter
		       if ((rgb_correct->OutputBmax==0)||(rgb_correct->OutputBmax==rgb_correct->OutputBmin))
			 mjpeg_info("OutputBmax would be smaller than 0 or OutputBmin!! Ignoring");
		       else
			 rgb_correct->OutputBmax-=1;
		       break;
		       
		     default :
		       break;
		    }
	       }
	     
	     if (current_correction==2)
	       {
		  switch(c) 
		    {
		     case 'E':  // Increase UBrotation by 1
		       yuv_correct->UVrotation+=1.0;
		       break;
		       
		     case 'e':  // Decrease UBrotation by 1
		       yuv_correct->UVrotation-=1.0;
		       break;
		       
		     case 'R':  // Increase Ufactor
		       yuv_correct->Ufactor+=0.05;
		       break;
		       
		     case 'r':  // Decrease Ufactor value
		       if (yuv_correct->Ufactor<=0.05)
			 mjpeg_info("Ufactor value would become negative!! Ignoring");
		       else
			 yuv_correct->Ufactor-=0.05;
		       break;
		       
		     case 'T':  // Increase Urotcenter by 1
		       if (yuv_correct->Urotcenter==255)
			 mjpeg_info("Urotcenter would be greater than 255!! Ignoring");
		       else
			 yuv_correct->Urotcenter+=1;
		       break;
		       
		     case 't':  // Decrease Urotcenter by 1
		       if (yuv_correct->Urotcenter==0)
			 mjpeg_info("Urotcenter would be smaller than 0!! Ignoring");
		       else
			 yuv_correct->Urotcenter-=1;
		       break;
		       
		       
		     case 'Y':  // Increase Vfactor
		       yuv_correct->Vfactor+=0.05;
		       break;
		       
		     case 'y':  // Decrease Vfactor value
		       if (yuv_correct->Vfactor<=0.05)
			 mjpeg_info("Vfactor value would become negative!! Ignoring");
		       else
			 yuv_correct->Vfactor-=0.05;
		       break;
		       
		     case 'U':  // Increase Vrotcenter by 1
		       if (yuv_correct->Vrotcenter==255)
			 mjpeg_info("Vrotcenter would be greater than 255!! Ignoring");
		       else
			 yuv_correct->Vrotcenter+=1;
		       break;
		       
		     case 'u':  // Decrease Vrotcenter by 1
		       if (yuv_correct->Vrotcenter==0)
			 mjpeg_info("Vrotcenter would be smaller than 0!! Ignoring");
		       else
			 yuv_correct->Vrotcenter-=1;
		       break;
		       
		     case 'I':  // Increase UVmin by 1
		       if (yuv_correct->UVmin==255)
			 mjpeg_info("UVmin would be greater than 255!! Ignoring");
		       else
			 yuv_correct->UVmin+=1;
		       break;
		       
		     case 'i':  // Decrease UVmin by 1
		       if (yuv_correct->UVmin==0)
			 mjpeg_info("UVmin would be smaller than 0!! Ignoring");
		       else
			 yuv_correct->UVmin-=1;
		       break;
		       
		     case 'O':  // Increase UVmax by 1
		       if (yuv_correct->UVmax==255)
			 mjpeg_info("UVmax would be greater than 255!! Ignoring");
		       else
			 yuv_correct->UVmax+=1;
		       break;
		       
		     case 'o':  // Decrease UVmax by 1
		       if (yuv_correct->UVmax==0)
			 mjpeg_info("UVmax would be smaller than 0!! Ignoring");
		       else
			 yuv_correct->UVmax-=1;
		       break;
		    }
	       }
	  }
	memcpy(frame->y,image_frame,frame->length);
	// Apply correction
	if (overall->rgbfirst == 1)
	  {
	     // RGB correction
	     yuvcorrect_RGB_init(rgb_correct);
	     yuvcorrect_RGB_treatment (frame, rgb_correct);
	  }
	// luminance correction
	yuvcorrect_luminance_init(yuv_correct);
	yuvcorrect_luminance_treatment (frame, yuv_correct);
	// chrominance correction
	yuvcorrect_chrominance_init(yuv_correct);
	yuvcorrect_chrominance_treatment (frame, yuv_correct);
	if (overall->rgbfirst != 1)
	  {
	     yuvcorrect_RGB_init(rgb_correct);
	     yuvcorrect_RGB_treatment (frame, rgb_correct);
	  }
	
	// Output Frame Header
	if (y4m_write_frame_header (1, &gen_correct->streaminfo, &frame->info) != Y4M_OK)
	  goto out_error;
	// Output Frame content
	// Output may be in fact constituted of two parts => cover part of frame with final_ref
	if (overall->mode!=0) 
	  {
	     if (overall->RefFrame!=overall->ImgFrame) 
	       add_reference_part(frame,ref_frame->ref,overall->mode);
	     else
	       add_reference_part(frame,frame->y,overall->mode);
	  }
	     
	// Output full frame size
	if (y4m_write (1, frame->y, frame->length) != Y4M_OK)
	  goto out_error;
	  }  
     }
   
   y4m_fini_stream_info (&gen_correct->streaminfo);
   y4m_fini_frame_info (&frame->info);
   return 0;
   
	
   out_error:
   mjpeg_error_exit1 ("Unable to write to output - aborting!");
   return 1;
}
// *************************************************************************************
// *************************************************************************************
// *************************************************************************************
// *************************************************************************************

/* 
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
