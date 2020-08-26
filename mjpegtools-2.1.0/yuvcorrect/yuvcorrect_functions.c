/*
  *  yuvcorrect_functions.c
  *  Common functions between yuvcorrect and yuvcorrect_tune
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

// *************************************************************************************

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

// For pointer adress alignement
const uint16_t ALIGNEMENT = 16 ;   // 16 bytes alignement for mmx registers in SIMD instructions for Pentium

const float PI = 3.141592654;

const char *legal_opt_flags = "I:F:M:T:Y:R:v:h";
const char LUMINANCE[] = "LUMINANCE_";
const char CHROMINANCE[] = "CHROMINANCE_";
const char Y[] = "Y_";
const char UV[] = "UV_";
const char CONFORM[] = "CONFORM";
const char R[] = "R_";
const char G[] = "G_";
const char B[] = "B_";

// Possible capacity over or under flow for the RGB to YUV conversion and vice-versa
const uint16_t OFFSET = 256;

// *************************************************************************************
void
handle_args_yuv_rgb (int argc, char *argv[], yuv_correction_t * yuv_correct, rgb_correction_t * rgb_correct)
{
  // This function handles argument passing on the command line
  int c;
  unsigned int ui1, ui2, ui3, ui4;
  int k_yuv, k_rgb;
  float f1, f2, f3;

  // Ne pas oublier de mettre la putain de ligne qui suit, sinon, plus d'argument à la ligne de commande, ils auront été bouffés par l'appel précédent à getopt!!
  optind = 1;
  while ((c = getopt (argc, argv, legal_opt_flags)) != -1)
    {
      switch (c)
	{

	  // **************            
	  // yuv KEYOWRD
	  // **************
	case 'Y':
	  k_yuv = 0;
	  if (strncmp (optarg, LUMINANCE, 10) == 0)
	    {
	      k_yuv = 1;
	      if (sscanf
		  (optarg, "LUMINANCE_%f_%u_%u_%u_%u", &f1, &ui1, &ui2, &ui3,
		   &ui4) == 5)
		{
		  // Coherence check:
		  if ((f1 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) || (ui1 > ui2) || (ui3 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent luminance correction (0<>255, small, large): Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u\n",
		       f1, ui1, ui2, ui3, ui4);
		  yuv_correct->luma = 1;
		  yuv_correct->Gamma = f1;
		  yuv_correct->InputYmin = (uint8_t) ui1;
		  yuv_correct->InputYmax = (uint8_t) ui2;
		  yuv_correct->OutputYmin = (uint8_t) ui3;
		  yuv_correct->OutputYmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to LUMINANCE keyword: %s\n",
		   optarg);
	    }
	  if (strncmp (optarg, Y, 2) == 0)
	    {
	      k_yuv = 1;
	      if (sscanf
		  (optarg, "Y_%f_%u_%u_%u_%u", &f1, &ui1, &ui2, &ui3,
		   &ui4) == 5)
		{
		  // Coherence check:
		  if ((f1 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) || (ui1 > ui2) || (ui3 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent luminance correction (0<>255, small, large): Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u\n",
		       f1, ui1, ui2, ui3, ui4);
		  yuv_correct->luma = 1;
		  yuv_correct->Gamma = f1;
		  yuv_correct->InputYmin = (uint8_t) ui1;
		  yuv_correct->InputYmax = (uint8_t) ui2;
		  yuv_correct->OutputYmin = (uint8_t) ui3;
		  yuv_correct->OutputYmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to Y keyword: %s\n", optarg);
	    }
	  if (strncmp (optarg, CHROMINANCE, 12) == 0)
	    {
	      k_yuv = 1;
	      if (sscanf
		  (optarg, "CHROMINANCE_%f_%f_%u_%f_%u_%u_%u", &f1, &f2, &ui1,
		   &f3, &ui2, &ui3, &ui4) == 7)
		{
		  // Coherence check:
		  if ((f2 <= 0.0) || (f3 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) ||
		      (ui3 > ui4) || (ui1 > ui4) || (ui2 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent chrominance correction (0<>255, small, large): UVrotation=%f, Ufactor=%f, Ucenter=%u, Vfactor=%f, Vcenter=%u, UVmin=%u, UVmax=%u, \n",
		       f1, f2, ui1, f3, ui2, ui3, ui4);
		  yuv_correct->chroma = 1;
		  yuv_correct->UVrotation = f1;
		  yuv_correct->Urotcenter = (uint8_t) ui1;
		  yuv_correct->Vrotcenter = (uint8_t) ui2;
		  yuv_correct->Ufactor = f2;
		  yuv_correct->Vfactor = f3;
		  yuv_correct->UVmin = (uint8_t) ui3;
		  yuv_correct->UVmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to CHROMINANCE keyword: %s\n",
		   optarg);
	    }
	  if (strncmp (optarg, UV, 3) == 0)
	    {
	      k_yuv = 1;
	      if (sscanf
		  (optarg, "UV_%f_%f_%u_%f_%u_%u_%u", &f1, &f2, &ui1,
		   &f3, &ui2, &ui3, &ui4) == 7)
		{
		  // Coherence check:
		  if ((f2 <= 0.0) || (f3 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) ||
		      (ui3 > ui4) || (ui1 > ui4) || (ui2 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent chrominance correction (0<>255, small, large): UVrotation=%f, Ufactor=%f, Ucenter=%u, Vfactor=%f, Vcenter=%u, UVmin=%u, UVmax=%u, \n",
		       f1, f2, ui1, f3, ui2, ui3, ui4);
		  yuv_correct->chroma = 1;
		  yuv_correct->UVrotation = f1;
		  yuv_correct->Urotcenter = (uint8_t) ui1;
		  yuv_correct->Vrotcenter = (uint8_t) ui2;
		  yuv_correct->Ufactor = f2;
		  yuv_correct->Vfactor = f3;
		  yuv_correct->UVmin = (uint8_t) ui3;
		  yuv_correct->UVmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to UV keyword: %s\n", optarg);
	    }
	  if (strncmp (optarg, CONFORM, 7) == 0)
	    {
	      k_yuv = 1;
	      yuv_correct->luma = 1;
	      yuv_correct->Gamma = 1.0;
	      yuv_correct->InputYmin = 16;
	      yuv_correct->InputYmax = 235;
	      yuv_correct->OutputYmin = 16;
	      yuv_correct->OutputYmax = 235;
	      yuv_correct->chroma = 1;
	      yuv_correct->UVrotation = 0.0;
	      yuv_correct->Urotcenter = 128;
	      yuv_correct->Vrotcenter = 128;
	      yuv_correct->Ufactor = 1.0;
	      yuv_correct->Vfactor = 1.0;
	      yuv_correct->UVmin = 16;
	      yuv_correct->UVmax = 240;
	    }
	  if (k_yuv == 0)
	    mjpeg_error_exit1 ("Unrecognized yuv keyword: %s", optarg);
	  break;
	  // *************



	  // **************            
	  // RGB KEYOWRD
	  // **************
	case 'R':
	  k_rgb = 0;
	  if (strncmp (optarg, R, 2) == 0)
	    {
	      k_rgb = 1;
	      if (sscanf
		  (optarg, "R_%f_%u_%u_%u_%u", &f1, &ui1, &ui2, &ui3,
		   &ui4) == 5)
		{
		  // Coherence check:
		  if ((f1 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) || (ui1 > ui2) || (ui3 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent RED correction (0<>255, small, large): Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u\n",
		       f1, ui1, ui2, ui3, ui4);
		  rgb_correct->rgb = 1;
		  rgb_correct->RGamma = f1;
		  rgb_correct->InputRmin = (uint8_t) ui1;
		  rgb_correct->InputRmax = (uint8_t) ui2;
		  rgb_correct->OutputRmin = (uint8_t) ui3;
		  rgb_correct->OutputRmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to R keyword: %s\n", optarg);
	    }
	  if (strncmp (optarg, G, 2) == 0)
	    {
	      k_rgb = 1;
	      if (sscanf
		  (optarg, "G_%f_%u_%u_%u_%u", &f1, &ui1, &ui2, &ui3,
		   &ui4) == 5)
		{
		  // Coherence check:
		  if ((f1 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) || (ui1 > ui2) || (ui3 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent GREEN correction (0<>255, small, large): Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u\n",
		       f1, ui1, ui2, ui3, ui4);
		  rgb_correct->rgb = 1;
		  rgb_correct->GGamma = f1;
		  rgb_correct->InputGmin = (uint8_t) ui1;
		  rgb_correct->InputGmax = (uint8_t) ui2;
		  rgb_correct->OutputGmin = (uint8_t) ui3;
		  rgb_correct->OutputGmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to G keyword: %s\n", optarg);
	    }
	  if (strncmp (optarg, B, 2) == 0)
	    {
	      k_rgb = 1;
	      if (sscanf
		  (optarg, "B_%f_%u_%u_%u_%u", &f1, &ui1, &ui2, &ui3,
		   &ui4) == 5)
		{
		  // Coherence check:
		  if ((f1 <= 0.0) ||
		      (ui1 < 0) || (ui1 > 255) ||
		      (ui2 < 0) || (ui2 > 255) ||
		      (ui3 < 0) || (ui3 > 255) ||
		      (ui4 < 0) || (ui4 > 255) || (ui1 > ui2) || (ui3 > ui4))
		    mjpeg_error_exit1
		      ("Uncoherent BLUE correction (0<>255, small, large): Gamma=%f, InputYmin=%u, InputYmax=%u, OutputYmin=%u, OutputYmax=%u\n",
		       f1, ui1, ui2, ui3, ui4);
		  rgb_correct->rgb = 1;
		  rgb_correct->BGamma = f1;
		  rgb_correct->InputBmin = (uint8_t) ui1;
		  rgb_correct->InputBmax = (uint8_t) ui2;
		  rgb_correct->OutputBmin = (uint8_t) ui3;
		  rgb_correct->OutputBmax = (uint8_t) ui4;
		}
	      else
		mjpeg_error_exit1
		  ("Wrong number of argument to B keyword: %s\n", optarg);
	    }
	  if (k_rgb == 0)
	    mjpeg_error_exit1 ("Unrecognized rgb keyword: %s", optarg);
	  break;
	  // *************

	default:
	  break;
	}
    }


}
// *************************************************************************************



// *************************************************************************************
void ref_frame_init(int fd,ref_frame_t *ref_frame)
{
   unsigned long int length;
   uint8_t *u_c_p;
   
   y4m_init_stream_info (&ref_frame->streaminfo);
   if (y4m_read_stream_header (fd,&ref_frame->streaminfo) != Y4M_OK)
     mjpeg_error_exit1 ("Could not read RefFrame yuv4mpeg header!");
   ref_frame->width = y4m_si_get_width(&ref_frame->streaminfo);
   ref_frame->height=y4m_si_get_height(&ref_frame->streaminfo);
   
   
   length=(ref_frame->width>>1)*ref_frame->height*3;
   if (!(u_c_p = malloc (length + ALIGNEMENT)))
     mjpeg_error_exit1 ("Could not allocate memory for ref frame. STOP!");
   mjpeg_debug ("before alignement: %p", u_c_p);
   if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
     u_c_p =
     (uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) * ALIGNEMENT);
   mjpeg_debug ("after alignement: %p", u_c_p);
   ref_frame->ref = u_c_p;
   y4m_init_frame_info (&ref_frame->info);
   return;
}

// *************************************************************************************



// *************************************************************************************
void initialisation1(int fd,frame_t * frame, general_correction_t * gen_correct,
		     yuv_correction_t * yuv_correct, rgb_correction_t * rgb_correct)
		    
{
   uint8_t *u_c_p;		//u_c_p = uint8_t pointer

  // gen_correct 
  gen_correct->no_header = gen_correct->line_switch =
  gen_correct->field_move = 0;

  y4m_init_stream_info (&gen_correct->streaminfo);
  if (y4m_read_stream_header (fd, &gen_correct->streaminfo) != Y4M_OK)
      mjpeg_error_exit1("Couldn't read yuv4mpeg header!");

  if (y4m_si_get_plane_count(&gen_correct->streaminfo) != 3)
      mjpeg_error_exit1("Only 3 plane formats supported");

   // frame
  frame->y_width = y4m_si_get_width (&gen_correct->streaminfo);
  frame->y_height = y4m_si_get_height (&gen_correct->streaminfo);
  frame->nb_y = frame->y_width * frame->y_height;
  frame->ss_h = y4m_chroma_ss_x_ratio(y4m_si_get_chroma(&gen_correct->streaminfo)).d;
  frame->ss_v = y4m_chroma_ss_y_ratio(y4m_si_get_chroma(&gen_correct->streaminfo)).d;
  frame->uv_width = y4m_si_get_plane_width(&gen_correct->streaminfo, 1);  /* planes 1&2 = U+V */
  frame->uv_height = y4m_si_get_plane_height(&gen_correct->streaminfo, 1);
  frame->nb_uv = frame->uv_width * frame->uv_height;
  frame->length = frame->nb_y + 2 * frame->nb_uv;
  if (!(u_c_p = malloc (frame->length + ALIGNEMENT)))
    mjpeg_error_exit1 ("Could not allocate memory for frame table. STOP!");
  mjpeg_debug ("before alignement: %p", u_c_p);
  if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
    u_c_p =
      (uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) * ALIGNEMENT);
  mjpeg_debug ("after alignement: %p", u_c_p);
  frame->y = u_c_p;
  frame->u = frame->y + frame->nb_y;
  frame->v = frame->u + frame->nb_uv;
  frame->field1 = frame->field2 = NULL;
  y4m_init_frame_info (&frame->info);


  // yuv_correct
  yuv_correct->luma = yuv_correct->chroma = 0;
  yuv_correct->luminance = yuv_correct->chrominance = NULL;
  yuv_correct->InputYmin = yuv_correct->OutputYmin = yuv_correct->UVmin = 0;
  yuv_correct->InputYmax = yuv_correct->OutputYmax = yuv_correct->UVmax = 255;
  yuv_correct->Gamma = yuv_correct->Ufactor = yuv_correct->Vfactor = 1.0;
  yuv_correct->UVrotation = 0.0;
  yuv_correct->Urotcenter = yuv_correct->Vrotcenter = 128;

  // rgb_correct 
  rgb_correct->rgb = 0;
  rgb_correct->new_red = rgb_correct->new_green = rgb_correct->new_blue =
    NULL;
  rgb_correct->RGamma = rgb_correct->GGamma = rgb_correct->BGamma = 1.0;
  rgb_correct->InputRmin = rgb_correct->InputGmin = rgb_correct->InputBmin =
    0;
  rgb_correct->InputRmax = rgb_correct->InputGmax = rgb_correct->InputBmax =
    255;
  rgb_correct->OutputRmin = rgb_correct->OutputGmin =
    rgb_correct->OutputBmin = 0;
  rgb_correct->OutputRmax = rgb_correct->OutputGmax =
    rgb_correct->OutputBmax = 255;
  rgb_correct->luma_r = rgb_correct->luma_g = rgb_correct->luma_b = NULL;
  rgb_correct->u_r = rgb_correct->u_g = rgb_correct->u_b = NULL;
  rgb_correct->v_r = rgb_correct->v_g = rgb_correct->v_b = NULL;
  rgb_correct->RUV_v = rgb_correct->GUV_v = rgb_correct->GUV_u =
    rgb_correct->BUV_u = NULL;
   return ;
}
// *************************************************************************************


// *************************************************************************************
void initialisation2(yuv_correction_t * yuv_correct, rgb_correction_t * rgb_correct)
{
   uint8_t *u_c_p;		//u_c_p = uint8_t pointer
   int8_t *si;			// si = int8_t pointer
   int16_t *sii;			// sii = int16_t pointer

  // Luminance correction initialisation
  if (yuv_correct->luma == 1)
    {
      // Memory allocation for the luminance vector
      if (!(u_c_p = (uint8_t *) malloc (256 * sizeof (uint8_t) + ALIGNEMENT)))
	mjpeg_error_exit1
	  ("Could not allocate memory for luminance table. STOP!");
      if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
	u_c_p =
	  (uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
      yuv_correct->luminance = u_c_p;
      // Filling in the luminance vectors
      yuvcorrect_luminance_init (yuv_correct);
    }
  // Chrominance correction initialisation
  if (yuv_correct->chroma == 1)
    {
      // Memory allocation for the UVchroma vector
      if (!(u_c_p =
	    (uint8_t *) malloc (2 * 256 * 256 * sizeof (uint8_t) +
				ALIGNEMENT)))
	mjpeg_error_exit1
	  ("Could not allocate memory for UVchroma vector. STOP!");
      // memory alignement of the 2 chroma vectors
      if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
	u_c_p =
	  (uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
      yuv_correct->chrominance = u_c_p;
      // Filling in the UVchroma vector
      yuvcorrect_chrominance_init (yuv_correct);
    }
  // RGB correction initialisation
  if (rgb_correct->rgb == 1)
    {
      // Memory allocation for the rgb vectors
      if (!
	  (u_c_p =
	   (uint8_t *) malloc (3 * (256 + (OFFSET << 1))* sizeof (uint8_t) + ALIGNEMENT)))
	mjpeg_error_exit1 ("Could not allocate memory for rgb table. STOP!");
      if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
	u_c_p =
	  (uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
      rgb_correct->new_red = u_c_p;
      u_c_p += (256 + (OFFSET<<1));
      rgb_correct->new_green = u_c_p;
      u_c_p += (256 + (OFFSET<<1));
      rgb_correct->new_blue = u_c_p;
      // Accélération
      if (!
	  (u_c_p =
	   (uint8_t *) malloc (3 * 256 * sizeof (uint8_t) + ALIGNEMENT)))
	mjpeg_error_exit1 ("Could not allocate memory for rgb table. STOP!");
      if (((unsigned long) u_c_p % ALIGNEMENT) != 0)
	u_c_p =
	  (uint8_t *) ((((unsigned long) u_c_p / ALIGNEMENT) + 1) *
		       ALIGNEMENT);
      rgb_correct->luma_r = u_c_p;
      u_c_p += 256;
      rgb_correct->luma_g = u_c_p;
      u_c_p += 256;
      rgb_correct->luma_b = u_c_p;
      if (!(si = (int8_t *) malloc (3 * 256 * sizeof (int8_t) + ALIGNEMENT)))
	mjpeg_error_exit1 ("Could not allocate memory for rgb table. STOP!");
      if (((unsigned long) si % ALIGNEMENT) != 0)
	si =
	  (int8_t *) ((((unsigned long) si / ALIGNEMENT) + 1) * ALIGNEMENT);
      rgb_correct->u_r = si;
      si += 256;
      rgb_correct->u_g = si;
      si += 256;
      rgb_correct->u_b = si;
      if (!(si = (int8_t *) malloc (3 * 256 * sizeof (int8_t) + ALIGNEMENT)))
	mjpeg_error_exit1 ("Could not allocate memory for rgb table. STOP!");
      if (((unsigned long) si % ALIGNEMENT) != 0)
	si =
	  (int8_t *) ((((unsigned long) si / ALIGNEMENT) + 1) * ALIGNEMENT);
      rgb_correct->v_r = si;
      si += 256;
      rgb_correct->v_g = si;
      si += 256;
      rgb_correct->v_b = si;
      if (!
	  (sii =
	   (int16_t *) malloc (4 * 256 * sizeof (int16_t) + ALIGNEMENT)))
	mjpeg_error_exit1 ("Could not allocate memory for rgb table. STOP!");
      if (((unsigned long) sii % ALIGNEMENT) != 0)
	sii =
	  (int16_t *) ((((unsigned long) sii / ALIGNEMENT) + 1) * ALIGNEMENT);
      rgb_correct->RUV_v = sii;
      sii += 256;
      rgb_correct->GUV_v = sii;
      sii += 256;
      rgb_correct->GUV_u = sii;
      sii += 256;
      rgb_correct->BUV_u = sii;
      // Filling in the RGB vectors
      yuvcorrect_RGB_init (rgb_correct);
    }
}
// *************************************************************************************



// *************************************************************************************
int
yuvcorrect_y4m_read_frame (int fd, y4m_stream_info_t *si, frame_t * frame, uint8_t line_switch)
{
  // This function reads a frame from input stream. 
  // It is the same as the y4m_read_frame function (from y4mpeg.c) except that line switching
  // is done during frame read
  static int err = Y4M_OK;
  unsigned int line;
  uint8_t *buf;
  buf = frame->y;

  if ((err = y4m_read_frame_header (fd, si, &frame->info)) == Y4M_OK)
    {
      if (!line_switch)
	{
	  if ((err = y4m_read (fd, buf, frame->length)) != Y4M_OK)
	    {
	      mjpeg_info ("Couldn't read FRAME content: %s!",
			  y4m_strerr (err));
	      return (err);
	    }
	}
      else
	{
	  // line switching during frame read 
	  // Y COMPONENT
	  for (line = 0; line < frame->y_height; line += 2)
	    {
	      buf += frame->y_width;	// buf points to next line on output, store input line there
	      if ((err = y4m_read (fd, buf, frame->y_width)) != Y4M_OK)
		{
		  mjpeg_info
		    ("Couldn't read FRAME content line %d : %s!",
		     line, y4m_strerr (err));
		  return (err);
		}
	      buf -= frame->y_width;	// buf points to former line on output, store input line there
	      if ((err = y4m_read (fd, buf, frame->y_width)) != Y4M_OK)
		{
		  mjpeg_info
		    ("Couldn't read FRAME content line %d : %s!",
		     line + 1, y4m_strerr (err));
		  return (err);
		}
	      buf += (frame->y_width << 1);	// 2 lines were read and stored
	    }
	  // U and V component
	  for (line = 0; line < (frame->uv_height << 1); line += 2)
	    {
	      buf += frame->uv_width;	// buf points to next line on output, store input line there
	      if ((err = y4m_read (fd, buf, frame->uv_width)) != Y4M_OK)
		{
		  mjpeg_info
		    ("Couldn't read FRAME content line %d : %s!",
		     line, y4m_strerr (err));
		  return (err);
		}
	      buf -= frame->uv_width;	// buf points to former line on output, store input line there
	      if ((err = y4m_read (fd, buf, frame->uv_width)) != Y4M_OK)
		{
		  mjpeg_info
		    ("Couldn't read FRAME content line %d : %s!",
		     line + 1, y4m_strerr (err));
		  return (err);
		}
	      buf += (frame->uv_width << 1);	// two line were read and stored
	    }
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
int
yuvcorrect_luminance_init (yuv_correction_t * yuv_correct)
{
  // This function initialises the luminance vector
  uint8_t *u_c_p;		//u_c_p = uint8_t pointer
  uint16_t i;

  // Filling in the luminance vector
  u_c_p = yuv_correct->luminance;
  for (i = 0; i < 256; i++)
    {
      if (i <= yuv_correct->InputYmin)
	*(u_c_p++) = yuv_correct->OutputYmin;
      else
	{
	  if (i >= yuv_correct->InputYmax)
	    *(u_c_p++) = yuv_correct->OutputYmax;
	  else
	    *(u_c_p++) = yuv_correct->OutputYmin +
	      floor (0.5 +
		     pow ((float) (i - yuv_correct->InputYmin) /
			  (float) (yuv_correct->InputYmax -
				   yuv_correct->InputYmin),
			  (float) 1 / yuv_correct->Gamma) *
		     (yuv_correct->OutputYmax - yuv_correct->OutputYmin));


	}
//      mjpeg_debug ("Luminance[%u]=%u", i, yuv_correct->luminance[i]);
    }

  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
yuvcorrect_chrominance_init (yuv_correction_t * yuv_correct)
{
  // This function initialises the UVchroma vector
  uint8_t *u_c_p;		//u_c_p = uint8_t pointer
  uint16_t u, v;		// do not use uint8_t, else you get infinite loop for u and v because 255+1=0 in unit8_t 
  float newU, newV, cosinus, sinus;

  mjpeg_debug ("chroma init");
  cosinus = cos (yuv_correct->UVrotation / 180.0 * PI);
  sinus = sin (yuv_correct->UVrotation / 180.0 * PI);
  // Filling in the chrominance vector
  u_c_p = yuv_correct->chrominance;
  for (u = 0; u <= 255; u++)
    {
      for (v = 0; v <= 255; v++)
	{
	  newU =
	    (((float) (u - yuv_correct->Urotcenter) * yuv_correct->Ufactor) *
	     cosinus -
	     ((float) (v - yuv_correct->Vrotcenter) * yuv_correct->Vfactor) *
	     sinus) + 128.0;
//          mjpeg_debug("u=%u, v=%u, newU=%f",u,v,newU);
	  newU = (float) floor (0.5 + newU);	// nearest integer in double format
	  if (newU < yuv_correct->UVmin)
	    newU = yuv_correct->UVmin;
	  if (newU > yuv_correct->UVmax)
	    newU = yuv_correct->UVmax;
	  newV =
	    (((float) (v - yuv_correct->Vrotcenter) * yuv_correct->Vfactor) *
	     cosinus +
	     ((float) (u - yuv_correct->Urotcenter) * yuv_correct->Ufactor) *
	     sinus) + 128.0;
//          mjpeg_debug("u=%u, v=%u, newV=%f",u,v,newV);
	  newV = (float) floor (0.5 + newV);	// nearest integer in double format
	  if (newV < yuv_correct->UVmin)
	    newV = yuv_correct->UVmin;
	  if (newV > yuv_correct->UVmax)
	    newV = yuv_correct->UVmax;
	  *(u_c_p++) = (uint8_t) newU;
	  *(u_c_p++) = (uint8_t) newV;
	}
    }
  mjpeg_debug ("end of chroma init");
  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
yuvcorrect_luminance_treatment (frame_t * frame,
				yuv_correction_t * yuv_correct)
{
  // This function corrects the luminance of input
  uint8_t *u_c_p;
  uint32_t i;

  u_c_p = frame->y;
  // Luminance (Y component)
  for (i = 0; i < frame->nb_y; i++)
    *(u_c_p+i) = yuv_correct->luminance[*(u_c_p+i)];

  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
yuvcorrect_chrominance_treatment (frame_t * frame,
				  yuv_correction_t * yuv_correct)
{
  // This function corrects the chrominance of input
  uint8_t *Uu_c_p, *Vu_c_p;
  uint32_t i, base;

//   mjpeg_debug("Start of yuvcorrect_chrominance_treatment(%p, %lu, %p)",input,size,UVchroma); 
  Uu_c_p = frame->u;
  Vu_c_p = frame->v;

  // Chroma
  for (i = 0; i < frame->nb_uv; i++)
    {
      base = ((((uint32_t) * Uu_c_p) << 8) + (*Vu_c_p)) << 1;	// base = ((((uint32_t)*Uu_c_p) * 256) + (*Vu_c_p)) * 2
      *(Uu_c_p++) = yuv_correct->chrominance[base++];
      *(Vu_c_p++) = yuv_correct->chrominance[base];
    }
//   mjpeg_debug("End of yuvcorrect_chrominance_treatment");
  return (0);
}

// *************************************************************************************



// *************************************************************************************
int
bottom_field_storage (frame_t * frame, uint8_t oddeven, uint8_t * field1,
		      uint8_t * field2)
{
  int ligne;
  uint8_t *u_c_p;
  // This function stores the current bottom field into tabular field[1 or 2] 
  u_c_p = frame->y;
  u_c_p += frame->y_width;	// first pixel of the bottom field
  if (oddeven)
    {
      // field1
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (field1, u_c_p, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field1 += frame->y_width;
	}
      u_c_p -= frame->y_width;
      u_c_p += frame->uv_width;
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (field1, u_c_p, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field1 += frame->uv_width;
	}
    }
  else
    {
      // field2
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (field2, u_c_p, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field2 += frame->y_width;
	}
      u_c_p -= frame->y_width;
      u_c_p += frame->uv_width;
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (field2, u_c_p, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field2 += frame->uv_width;
	}
    }
  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
top_field_storage (frame_t * frame, uint8_t oddeven, uint8_t * field1,
		   uint8_t * field2)
{
  int ligne;
  uint8_t *u_c_p;
  // This function stores the current bottom field into tabular field[1 or 2] 
  u_c_p = frame->y;
  if (oddeven)
    {
      // field1
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (field1, u_c_p, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field1 += frame->y_width;
	}
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (field1, u_c_p, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field1 += frame->uv_width;
	}
    }
  else
    {
      // field2
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (field2, u_c_p, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field2 += frame->y_width;
	}
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (field2, u_c_p, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field2 += frame->uv_width;
	}
    }
  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
bottom_field_replace (frame_t * frame, uint8_t oddeven, uint8_t * field1,
		      uint8_t * field2)
{
  int ligne;
  uint8_t *u_c_p;
  // This function replaces the current bottom field with tabular field[1 or 2] 
  u_c_p = frame->y;
  u_c_p += frame->y_width;

  if (oddeven)
    {
      // field2
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (u_c_p, field2, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field2 += frame->y_width;
	}
      u_c_p -= frame->y_width;
      u_c_p += frame->uv_width;
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (u_c_p, field2, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field2 += frame->uv_width;
	}
    }
  else
    {
      // field1
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (u_c_p, field1, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field1 += frame->y_width;
	}
      u_c_p -= frame->y_width;
      u_c_p += frame->uv_width;
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (u_c_p, field1, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field1 += frame->uv_width;
	}
    }
  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
top_field_replace (frame_t * frame, uint8_t oddeven, uint8_t * field1,
		   uint8_t * field2)
{
  int ligne;
  uint8_t *u_c_p;
  // This function replaces the current bottom field with tabular field[1 or 2] 
  u_c_p = frame->y;

  if (oddeven)
    {
      // field2
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (u_c_p, field2, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field2 += frame->y_width;
	}
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (u_c_p, field2, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field2 += frame->uv_width;
	}
    }
  else
    {
      // field1
      // Y Component
      for (ligne = 0; ligne < frame->y_height; ligne += 2)
	{
	  memcpy (u_c_p, field1, frame->y_width);
	  u_c_p += (frame->y_width << 1);
	  field1 += frame->y_width;
	}
      // U and V COMPONENTS
      for (ligne = 0; ligne < (frame->uv_height << 1); ligne += 2)
	{
	  memcpy (u_c_p, field1, frame->uv_width);
	  u_c_p += (frame->uv_width << 1);
	  field1 += frame->uv_width;
	}
    }
  return (0);
}

// *************************************************************************************


// *************************************************************************************
void
yuvstat (frame_t * frame)
{
  uint8_t y, u, v;
  uint8_t *input;
  int16_t r, g, b;
  unsigned long int somme_y = 0, somme_u = 0, somme_v = 0, moy_y = 0, moy_u =
    0, moy_v = 0;
  unsigned long int somme_r = 0, somme_g = 0, somme_b = 0, moy_r = 0, moy_g =
    0, moy_b = 0;
  uint16_t histo_y[256], histo_u[256], histo_v[256];
  uint16_t histo_r[256], histo_g[256], histo_b[256];
  unsigned long int i;
  unsigned long int decalage = frame->nb_uv;
  unsigned long int decalage_y_u = decalage << 2;
  unsigned long int decalage_y_v = decalage * 5;
  input = frame->y;
  for (i = 0; i < 256; i++)
    {
      histo_y[i] = histo_u[i] = histo_v[i] = 0;
      histo_r[i] = histo_g[i] = histo_b[i] = 0;
    }

  for (i = 0; i < decalage; i++)
    {
      y = input[i * 4];
      u = input[i + decalage_y_u];
      v = input[i + decalage_y_v];
      histo_y[y]++;
      histo_u[u]++;
      histo_v[v]++;
      r = (int) y + (int) floor (1.375 * (float) (v - 128));
      g = (int) y + (int) floor (-0.698 * (v - 128) - 0.336 * (u - 128));
      b = (int) y + (int) floor (1.732 * (float) (u - 128));
      histo_r[clip_0_255 (r)]++;
      histo_g[clip_0_255 (g)]++;
      histo_b[clip_0_255 (b)]++;
    }
  mjpeg_info ("Histogramme\ni Y U V");
  for (i = 0; i < 256; i++)
    {
      mjpeg_info ("%03lu %05u %05u %05u", i, histo_y[i], histo_u[i],
		  histo_v[i]);
      somme_y += histo_y[i];
      somme_u += histo_u[i];
      somme_v += histo_v[i];
    }
  i = 0;
  while (moy_y < somme_y / 2)
    moy_y += histo_y[i++];
  moy_y = i;
  i = 0;
  while (moy_u < somme_u / 2)
    moy_u += histo_u[i++];
  moy_u = i;
  i = 0;
  while (moy_v < somme_v / 2)
    moy_v += histo_v[i++];
  moy_v = i;

  mjpeg_info ("moyY=%03lu moyU=%03lu moyV=%03lu", moy_y, moy_u, moy_v);
  mjpeg_info ("sommes = %06lu %06lu %06lu", somme_y, somme_u, somme_v);


  mjpeg_info ("Histogramme\ni R G B");
  for (i = 0; i < 256; i++)
    {
      mjpeg_info ("%03lu %05u %05u %05u", i, histo_r[i], histo_g[i],
		  histo_b[i]);
      somme_r += histo_r[i];
      somme_g += histo_g[i];
      somme_b += histo_b[i];
    }
  mjpeg_info ("sommes = %06lu %06lu %06lu", somme_r, somme_g, somme_b);


  i = 0;
  while (moy_r < somme_r / 2)
    moy_r += histo_r[i++];
  moy_r = i;
  i = 0;
  while (moy_g < somme_g / 2)
    moy_g += histo_g[i++];
  moy_g = i;
  i = 0;
  while (moy_b < somme_b / 2)
    moy_b += histo_b[i++];
  moy_b = i;

  mjpeg_info ("moyR=%03lu moyG=%03lu moyB=%03lu", moy_r, moy_g, moy_b);

}

// *************************************************************************************


// *************************************************************************************
uint8_t
clip_0_255 (int16_t number)
{
  if (number <= 0)
    return (0);
  else
    {
      if (number >= 255)
	return (255);
      else
	return ((uint8_t) number);
    }
  mjpeg_error_exit1 ("function clip_0_255 failed!!!");
}

// *************************************************************************************


// *************************************************************************************
int8_t
clip_127_127 (int16_t number)
{
  if (number <= -127)
    return (-127);
  else
    {
      if (number >= 127)
	return (127);
      else
	return ((int8_t) number);
    }
  mjpeg_error_exit1 ("function clip_127_127 failed!!!");
}

// *************************************************************************************


// *************************************************************************************
int
yuvcorrect_RGB_init (rgb_correction_t * rgb_correct)
{
  int i;
  unsigned char *u_c_p;
  int8_t *si;
  int16_t *sii;

  // Filling in R vector
  u_c_p = rgb_correct->new_red;
  for (i = 0; i < 256+(OFFSET<<1); i++)
    {
      if ((i-OFFSET) <= rgb_correct->InputRmin)
	*(u_c_p++) = rgb_correct->OutputRmin;
      else
	{
	  if ((i-OFFSET) >= rgb_correct->InputRmax)
	    *(u_c_p++) = rgb_correct->OutputRmax;
	  else
	    *(u_c_p++) = rgb_correct->OutputRmin +
	      floor (0.5 +
		     pow ((float) ((i-OFFSET) - rgb_correct->InputRmin) /
			  (float) (rgb_correct->InputRmax -
				   rgb_correct->InputRmin),
			  (float) 1 / rgb_correct->RGamma) *
		     (rgb_correct->OutputRmax - rgb_correct->OutputRmin));
	}
      mjpeg_debug ("R[%u]=%u", i, rgb_correct->new_red[i]);
    }

  // Filling in G vector
  u_c_p = rgb_correct->new_green;
  for (i = 0; i < 256+(OFFSET<<1); i++)
    {
      if ((i-OFFSET) <= rgb_correct->InputGmin)
	*(u_c_p++) = rgb_correct->OutputGmin;
      else
	{
	  if ((i-OFFSET) >= rgb_correct->InputGmax)
	    *(u_c_p++) = rgb_correct->OutputGmax;
	  else
	    *(u_c_p++) = rgb_correct->OutputGmin +
	      floor (0.5 +
		     pow ((float) ((i-OFFSET) - rgb_correct->InputGmin) /
			  (float) (rgb_correct->InputGmax -
				   rgb_correct->InputGmin),
			  (float) 1 / rgb_correct->GGamma) *
		     (rgb_correct->OutputGmax - rgb_correct->OutputGmin));
	}
      mjpeg_debug ("G[%u]=%u", i, rgb_correct->new_green[i]);
    }

  // Filling in B vector
  u_c_p = rgb_correct->new_blue;
  for (i = 0; i < 256+(OFFSET<<1); i++)
    {
      if ((i-OFFSET) <= rgb_correct->InputBmin)
	*(u_c_p++) = rgb_correct->OutputBmin;
      else
	{
	  if ((i-OFFSET) >= rgb_correct->InputBmax)
	    *(u_c_p++) = rgb_correct->OutputBmax;
	  else
	    *(u_c_p++) = rgb_correct->OutputBmin +
	      floor (0.5 +
		     pow ((float) ((i-OFFSET) - rgb_correct->InputBmin) /
			  (float) (rgb_correct->InputBmax -
				   rgb_correct->InputBmin),
			  (float) 1 / rgb_correct->BGamma) *
		     (rgb_correct->OutputBmax - rgb_correct->OutputBmin));
	}
      mjpeg_debug ("B[%u]=%u", i, rgb_correct->new_blue[i]);
    }
   
  // Filling the luma_(r,g,b) vectors
  u_c_p = rgb_correct->luma_r;
  for (i = 0; i < 256; i++)
    *(u_c_p++) = clip_0_255 ((int) floor (0.5 + 0.3000 * i));
  u_c_p = rgb_correct->luma_g;
  for (i = 0; i < 256; i++)
    *(u_c_p++) = clip_0_255 ((int) floor (0.5 + 0.5859 * i));
  u_c_p = rgb_correct->luma_b;
  for (i = 0; i < 256; i++)
    *(u_c_p++) = clip_0_255 ((int) floor (0.5 + 0.1120 * i));
  // Filling the u_(r,g,b) vectors
  si = rgb_correct->u_r;
  for (i = 0; i < 256; i++)
    *(si++) = clip_127_127 ((int) floor (0.5 - 0.1719 * i));
  si = rgb_correct->u_g;
  for (i = 0; i < 256; i++)
    *(si++) = clip_127_127 ((int) floor (0.5 - 0.3398 * i));
  si = rgb_correct->u_b;
  for (i = 0; i < 256; i++)
    *(si++) = clip_127_127 ((int) floor (0.5 + 0.5117 * i));
  // Filling the v_(r,g,b) vectors
  si = rgb_correct->v_r;
  for (i = 0; i < 256; i++)
    *(si++) = clip_127_127 ((int) floor (0.5 + 0.5117 * i));
  si = rgb_correct->v_g;
  for (i = 0; i < 256; i++)
    *(si++) = clip_127_127 ((int) floor (0.5 - 0.4297 * i));
  si = rgb_correct->v_b;
  for (i = 0; i < 256; i++)
    *(si++) = clip_127_127 ((int) floor (0.5 - 0.0820 * i));
  // Filling the RUV_v,GUV_u,GUV_v,BUV_u
  sii = rgb_correct->RUV_v;
  for (i = 0; i < 256; i++)
    *(sii++) = clip_127_127 ((int) floor (0.5 + 1.375 * (i - 128)));
  sii = rgb_correct->GUV_v;
  for (i = 0; i < 256; i++)
    *(sii++) = clip_127_127 ((int) floor (0.5 - 0.698 * (i - 128)));
  sii = rgb_correct->GUV_u;
  for (i = 0; i < 256; i++)
    *(sii++) = clip_127_127 ((int) floor (0.5 - 0.336 * (i - 128)));
  sii = rgb_correct->BUV_u;
  for (i = 0; i < 256; i++)
    *(sii++) = clip_127_127 ((int) floor (0.5 + 1.732 * (i - 128)));

/*
 for (i = 0; i < 256; i++)
     {	
	mjpeg_info("acceleration : %u %u %u %d %d %d %d %d %d",
		   rgb_correct->luma_r[i],rgb_correct->luma_g[i],rgb_correct->luma_b[i],
		   rgb_correct->u_r[i],rgb_correct->u_g[i],rgb_correct->u_b[i],
		   rgb_correct->v_r[i],rgb_correct->v_g[i],rgb_correct->v_b[i]);
     }
*/
  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
yuvcorrect_RGB_treatment (frame_t * frame, rgb_correction_t * rgb_correct)
{
  // This function corrects the current frame based on RGB corrections
  // Optimisations : all possible multiplicative operation results are already stored in tables like luma_r,u_r,GUV_u, etc...
  // TODO Optimisatiion : suppress the necessity of the clip_0_255 function by enlarging concerned tables new_(red,green,blue): from [0:256] to [-256:512]
  uint8_t *u_p, *v_p, *line1, *line2;
  uint32_t i, j;
  int16_t R_UV, G_UV, B_UV;
  uint8_t moy_r, moy_g, moy_b;
  uint8_t R1, R2, R3, R4, G1, G2, G3, G4, B1, B2, B3, B4;

  line1 = frame->y;
  line2 = line1 + frame->y_width;
  u_p = frame->u;
  v_p = frame->v;

  if (frame->ss_h==2 && frame->ss_v==2) // 4:2:0
    {
      for (i = 0; i < frame->uv_height; i++)
        {
          for (j = 0; j < frame->uv_width; j++)
            {
              R_UV = rgb_correct->RUV_v[*v_p];
              G_UV = rgb_correct->GUV_v[*v_p] + rgb_correct->GUV_u[*u_p];
              B_UV = rgb_correct->BUV_u[*u_p];
              //         mjpeg_info("YUV = %u + %d %d %d = %d %d %d",*line1,R_UV,G_UV,B_UV,(int16_t)*line1+R_UV,(int16_t)*line1+G_UV,(int16_t)*line1+B_UV);
              // Calculate the value of the four pixels concerned by the single (u,v) values
              // Upper Left
              R1 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G1 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B1 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              // Compute new y value
              //         mjpeg_info("line1 = %u %u %u %d",rgb_correct->luma_r[R1],rgb_correct->luma_g[G1],rgb_correct->luma_b[B1],clip_0_255((uint16_t)rgb_correct->luma_r[R1]
              //                                            +rgb_correct->luma_g[G1]+rgb_correct->luma_b[B1]));
              *line1++ = clip_0_255 ((uint16_t) rgb_correct->luma_r[R1]
                                     + rgb_correct->luma_g[G1] +
                                     rgb_correct->luma_b[B1]);
              
              R2 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G2 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B2 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              
              // Compute new y value
              *line1++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R2]
                                     + (int16_t) rgb_correct->luma_g[G2] +
                                     (int16_t) rgb_correct->luma_b[B2]);
              
              R3 = rgb_correct->new_red  [OFFSET + *line2 + R_UV];
              G3 = rgb_correct->new_green[OFFSET + *line2 + G_UV];
              B3 = rgb_correct->new_blue [OFFSET + *line2 + B_UV];
              // Compute new y value
              *line2++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R3]
                                     + (int16_t) rgb_correct->luma_g[G3] +
                                     (int16_t) rgb_correct->luma_b[B3]);
              
              R4 = rgb_correct->new_red  [OFFSET + *line2 + R_UV];
              G4 = rgb_correct->new_green[OFFSET + *line2 + G_UV];
              B4 = rgb_correct->new_blue [OFFSET + *line2 + B_UV];
              // Compute new y value
              *line2++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R4]
                                     + (int16_t) rgb_correct->luma_g[G4] +
                                     (int16_t) rgb_correct->luma_b[B4]);
              
              moy_r = clip_0_255 (((int16_t) 2 + R1 + R2 + R3 + R4) >> 2);
              moy_g = clip_0_255 (((int16_t) 2 + G1 + G2 + G3 + G4) >> 2);
              moy_b = clip_0_255 (((int16_t) 2 + B1 + B2 + B3 + B4) >> 2);
              //        mjpeg_info("B : %u %u %u %u moyennes %u %u %u",B1,B2,B3,B4,moy_r,moy_g,moy_b);
              *u_p++ =
                clip_0_255 ((int16_t) 128 + rgb_correct->u_r[moy_r] +
                            rgb_correct->u_g[moy_g] + rgb_correct->u_b[moy_b]);
              *v_p++ =
                clip_0_255 ((int16_t) 128 + rgb_correct->v_r[moy_r] +
                            rgb_correct->v_g[moy_g] + rgb_correct->v_b[moy_b]);
            }
          line1 += frame->y_width;
          line2 += frame->y_width;
        }
    }
  else if (frame->ss_h==4 && frame->ss_v==1) // 4:1:1
    {
      for (i = 0; i < frame->uv_height; i++)
        {
          for (j = 0; j < frame->uv_width; j++)
            {
              R_UV = rgb_correct->RUV_v[*v_p];
              G_UV = rgb_correct->GUV_v[*v_p] + rgb_correct->GUV_u[*u_p];
              B_UV = rgb_correct->BUV_u[*u_p];
              //         mjpeg_info("YUV = %u + %d %d %d = %d %d %d",*line1,R_UV,G_UV,B_UV,(int16_t)*line1+R_UV,(int16_t)*line1+G_UV,(int16_t)*line1+B_UV);
              // Calculate the value of the four pixels concerned by the single (u,v) values
              // Upper Left
              R1 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G1 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B1 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              // Compute new y value
              //         mjpeg_info("line1 = %u %u %u %d",rgb_correct->luma_r[R1],rgb_correct->luma_g[G1],rgb_correct->luma_b[B1],clip_0_255((uint16_t)rgb_correct->luma_r[R1]
              //                                            +rgb_correct->luma_g[G1]+rgb_correct->luma_b[B1]));
              *line1++ = clip_0_255 ((uint16_t) rgb_correct->luma_r[R1]
                                     + rgb_correct->luma_g[G1] +
                                     rgb_correct->luma_b[B1]);
              
              R2 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G2 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B2 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              
              // Compute new y value
              *line1++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R2]
                                     + (int16_t) rgb_correct->luma_g[G2] +
                                     (int16_t) rgb_correct->luma_b[B2]);
              
              R3 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G3 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B3 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              // Compute new y value
              *line1++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R3]
                                     + (int16_t) rgb_correct->luma_g[G3] +
                                     (int16_t) rgb_correct->luma_b[B3]);
              
              R4 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G4 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B4 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              // Compute new y value
              *line1++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R4]
                                     + (int16_t) rgb_correct->luma_g[G4] +
                                     (int16_t) rgb_correct->luma_b[B4]);
              
              moy_r = clip_0_255 (((int16_t) 2 + R1 + R2 + R3 + R4) >> 2);
              moy_g = clip_0_255 (((int16_t) 2 + G1 + G2 + G3 + G4) >> 2);
              moy_b = clip_0_255 (((int16_t) 2 + B1 + B2 + B3 + B4) >> 2);
              //        mjpeg_info("B : %u %u %u %u moyennes %u %u %u",B1,B2,B3,B4,moy_r,moy_g,moy_b);
              *u_p++ =
                clip_0_255 ((int16_t) 128 + rgb_correct->u_r[moy_r] +
                            rgb_correct->u_g[moy_g] + rgb_correct->u_b[moy_b]);
              *v_p++ =
                clip_0_255 ((int16_t) 128 + rgb_correct->v_r[moy_r] +
                            rgb_correct->v_g[moy_g] + rgb_correct->v_b[moy_b]);
            }
        }
    }
  else if (frame->ss_h==2 && frame->ss_v==1) // 4:2:2
    {
      for (i = 0; i < frame->uv_height; i++)
        {
          for (j = 0; j < frame->uv_width; j++)
            {
              R_UV = rgb_correct->RUV_v[*v_p];
              G_UV = rgb_correct->GUV_v[*v_p] + rgb_correct->GUV_u[*u_p];
              B_UV = rgb_correct->BUV_u[*u_p];
              //         mjpeg_info("YUV = %u + %d %d %d = %d %d %d",*line1,R_UV,G_UV,B_UV,(int16_t)*line1+R_UV,(int16_t)*line1+G_UV,(int16_t)*line1+B_UV);
              // Calculate the value of the two pixels concerned by the single (u,v) values
              // Upper Left
              R1 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G1 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B1 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              // Compute new y value
              //         mjpeg_info("line1 = %u %u %u %d",rgb_correct->luma_r[R1],rgb_correct->luma_g[G1],rgb_correct->luma_b[B1],clip_0_255((uint16_t)rgb_correct->luma_r[R1]
              //                                            +rgb_correct->luma_g[G1]+rgb_correct->luma_b[B1]));
              *line1++ = clip_0_255 ((uint16_t) rgb_correct->luma_r[R1]
                                     + rgb_correct->luma_g[G1] +
                                     rgb_correct->luma_b[B1]);
              
              R2 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G2 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B2 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              
              // Compute new y value
              *line1++ = clip_0_255 ((int16_t) rgb_correct->luma_r[R2]
                                     + (int16_t) rgb_correct->luma_g[G2] +
                                     (int16_t) rgb_correct->luma_b[B2]);
              
              moy_r = clip_0_255 (((int16_t) 1 + R1 + R2) >> 1);
              moy_g = clip_0_255 (((int16_t) 1 + G1 + G2) >> 1);
              moy_b = clip_0_255 (((int16_t) 1 + B1 + B2) >> 1);
              //        mjpeg_info("B : %u %u %u %u moyennes %u %u %u",B1,B2,B3,B4,moy_r,moy_g,moy_b);
              *u_p++ =
                clip_0_255 ((int16_t) 128 + rgb_correct->u_r[moy_r] +
                            rgb_correct->u_g[moy_g] + rgb_correct->u_b[moy_b]);
              *v_p++ =
                clip_0_255 ((int16_t) 128 + rgb_correct->v_r[moy_r] +
                            rgb_correct->v_g[moy_g] + rgb_correct->v_b[moy_b]);
            }
        }
    }
  else if (frame->ss_h==1 && frame->ss_v==1) // 4:4:4
    {
      for (i = 0; i < frame->uv_height; i++)
        {
          for (j = 0; j < frame->uv_width; j++)
            {
              R_UV = rgb_correct->RUV_v[*v_p];
              G_UV = rgb_correct->GUV_v[*v_p] + rgb_correct->GUV_u[*u_p];
              B_UV = rgb_correct->BUV_u[*u_p];
              //         mjpeg_info("YUV = %u + %d %d %d = %d %d %d",*line1,R_UV,G_UV,B_UV,(int16_t)*line1+R_UV,(int16_t)*line1+G_UV,(int16_t)*line1+B_UV);
              // here we have 1-1 correspondence between RGB and YUV
              R1 = rgb_correct->new_red  [OFFSET + *line1 + R_UV];
              G1 = rgb_correct->new_green[OFFSET + *line1 + G_UV];
              B1 = rgb_correct->new_blue [OFFSET + *line1 + B_UV];
              // Compute new y value
              //         mjpeg_info("line1 = %u %u %u %d",rgb_correct->luma_r[R1],rgb_correct->luma_g[G1],rgb_correct->luma_b[B1],clip_0_255((uint16_t)rgb_correct->luma_r[R1]
              //                                            +rgb_correct->luma_g[G1]+rgb_correct->luma_b[B1]));
              *line1++ = clip_0_255 ((uint16_t) rgb_correct->luma_r[R1]
                                     + rgb_correct->luma_g[G1] +
                                     rgb_correct->luma_b[B1]);
              *u_p++ = clip_0_255 ((int16_t) 128 + rgb_correct->u_r[R1] +
                            rgb_correct->u_g[G1] + rgb_correct->u_b[B1]);
              *v_p++ = clip_0_255 ((int16_t) 128 + rgb_correct->v_r[R1] +
                            rgb_correct->v_g[G1] + rgb_correct->v_b[B1]);
            }
        }
    }
  else
      mjpeg_error_exit1 ("Sorry, RGB corrections not supported with that chroma subsampling");
  return (0);
}

// *************************************************************************************



/* 
 * Local variables:
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
