/*
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
// Version of the 25/05/2003
// Copyright X. Biquard xbiquard@free.fr
// Great speed Warning : function mjpeg_debug implies an implicit test => may slow down a lot the execution
// of the program.
// SIMD accelerated multiplications with csplineh not possible since value to be multiply do not stand
// in int16_t, but in int32_t
// Maybe possible in SSE since xmm registers of 128 bits available

// IL FAUT NETTOYER LE HEADER : PAS BESOIN DE TOUTES CES VARIABLES GLOBALES A LA CON

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
#include "mjpeg_types.h"
#include "yuvscaler.h"
#include "../utils/mmx.h"

extern unsigned int output_active_width;
extern unsigned int output_active_height;
extern unsigned int output_width;
extern unsigned int input_width;
extern unsigned int input_useful_width;
extern unsigned int input_useful_height;
extern unsigned int specific;


// Defines
#define FLOAT2INTEGERPOWER 11
#define FLOAT2INTOFFSET 1024
#define DBLEFLOAT2INT 22
#define DBLEFLOAT2INTOFFSET 2097152

// MMX test
#ifdef HAVE_ASM_MMX
extern int32_t *mmx_res;
extern int mmx;			// =1 for mmx activated
#endif
// MMX test

extern int32_t *intermediate;

// *************************************************************************************
int
cubic_scale (uint8_t * padded_input, uint8_t * output, 
	     unsigned int *in_col, unsigned int *in_line,
	     int16_t *cspline_w, uint16_t  width_neighbors, uint8_t zero_width_neighbors,
	     int16_t *cspline_h, uint16_t height_neighbors, uint8_t zero_height_neighbors,
	     unsigned int half)
{
  // Warning: because cubic-spline values may be <0 or >1.0, a range test on value is mandatory
  unsigned int local_output_active_width = output_active_width >> half;
  unsigned int local_output_active_height = output_active_height >> half;
  unsigned int local_output_width = output_width >> half;
  unsigned int local_input_useful_height = input_useful_height >> half;
  unsigned int local_input_useful_width = input_useful_width >> half;
  unsigned int local_padded_height = local_input_useful_height + height_neighbors -1;
  unsigned int local_padded_width = local_input_useful_width + width_neighbors -1;
  unsigned int out_line, out_col,w,h;
  unsigned int *in_line_p,*in_col_p;
  int16_t output_offset;

  uint8_t *output_p,*line,*line_begin,*line_0;
  int16_t *cspline_wp,*cspline_hp,*cspline_hp0;
  int32_t value=0,value1=0,*intermediate_p,*inter_begin;

//   mjpeg_debug ("Start of cubic_scale ");

   output_p = output;
   output_offset = local_output_width-local_output_active_width;
   
   /* *INDENT-OFF* */
   
   switch(specific) {
    case 0: 
	{
	   // First scale along the Width, not the height
	   width_neighbors-=zero_width_neighbors;
	   height_neighbors-=zero_height_neighbors;
	   intermediate_p=intermediate;
	   line_0 = padded_input;
	   for (out_line = 0; out_line < local_padded_height; out_line++)
	     {
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_0 + *(in_col_p++);
		     value1=*(line++)*(*(cspline_wp++));
		     for (w=1;w<width_neighbors-1;w++) 
		       value1+=*(line++)*(*(cspline_wp++));
		     value1+=*(line)*(*(cspline_wp++));
		     if (zero_width_neighbors)
		       cspline_wp++;
		     *(intermediate_p++)=value1;
		  }
		// a line of intermediate in now finished. Make line_0 points on the next line of padded_input
		line_0+=local_padded_width;
	     }
 
	   // Intermediate now contains an width-scaled frame
	   // we now scale it along the height, not the width
	   cspline_hp0=cspline_h;
	   in_line_p=in_line;
	   for (out_line = 0; out_line < local_output_active_height; out_line++)
	     {
		inter_begin=intermediate + *(in_line_p++) * local_output_active_width;
		for (out_col = 0; out_col < local_output_active_width-1; out_col++)
		  {
		     cspline_hp=cspline_hp0;
		     value1 = *(inter_begin)*(*(cspline_hp++));
		     for (h=1;h<height_neighbors-1;h++)
		       value1 += (*(inter_begin+h*local_output_active_width))*(*(cspline_hp++));
		     value1 += (*((inter_begin++)+h*local_output_active_width))*(*(cspline_hp));
		     
		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value =(value1 + DBLEFLOAT2INTOFFSET) >> DBLEFLOAT2INT;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// last out_col to be treated => cspline_hp0 incremented => we use the C "++" facility
		value1 = *(inter_begin)*(*(cspline_hp0++));
		for (h=1;h<height_neighbors;h++)
		  value1 += (*(inter_begin+h*local_output_active_width))*(*(cspline_hp0++));
		if (zero_height_neighbors)
		  cspline_hp0++;
		
		if (value1 < 0) *(output_p++) = 0;
		else {
		   value =(value1 + DBLEFLOAT2INTOFFSET) >> DBLEFLOAT2INT;
		   if (value > 255) *(output_p++) = 255;
		   else *(output_p++) = (uint8_t) value;
		}
		// a line on output is now finished. We jump to the beginning of the next line
		output_p+=output_offset;
	     }
	}
    break;
    
  case 1:
/*      
#ifdef HAVE_ASM_MMX
      if (mmx==1)
	{
	   // We only downscale on width, not height
	   emms();
	   pxor_r2r(mm7,mm7);
	   // only zeros in mm7
	   
	   line_0=padded_input;
	   for (out_line = 0; out_line < local_output_active_height; out_line++)
	     {
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_0 + *(in_col_p++);
		     switch(width_neighbors)
		       {
			  // NB : c'est l'opération movq_r2m qui coute le plus en temps, seulement pour la première !!!
			case 4:
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(line),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm0,mm6);      movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  break;
			case 6:
			  // cspline_w in mm0 and mm1
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(cspline_wp+4),mm1);
			  
			  // 4 pixels in mm6 and next 4 in mm5
			  movq_m2r(*(line),mm6);
//			  __m64 mm6= _mm_unpacklo_pi16(mm6, mm7);
			  punpcklbw_r2r(mm7,mm6);
			  movq_m2r(*(line+4),mm5);    
			  punpcklbw_r2r(mm7,mm5);

			  // multiply and add => these take more than one cycle and may be done in parallel
			  pmaddwd_r2r(mm0,mm6);
			  movq_r2m(mm6,*(mmx_t *)mmx_res);
//			  movntq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  pmaddwd_r2r(mm1,mm5);
			  movq_r2m(mm5,*(mmx_t *)mmx_res);
//			  movntq_r2m(mm5,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0];

			  break;
			case 8:
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(cspline_wp+4),mm1);
			  movq_m2r(*(line),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm0,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+4),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm1,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  break;
			case 10:
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(cspline_wp+4),mm1);
			  movq_m2r(*(cspline_wp+8),mm2);
			  movq_m2r(*(line),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm0,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+4),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm1,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+8),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm2,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0];
			  break;
			case 12:
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(cspline_wp+4),mm1);
			  movq_m2r(*(cspline_wp+8),mm2);
			  movq_m2r(*(line),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm0,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+4),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm1,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+8),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm2,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  break;
			case 14:
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(cspline_wp+4),mm1);
			  movq_m2r(*(cspline_wp+8),mm2);
			  movq_m2r(*(cspline_wp+12),mm3);
			  movq_m2r(*(line),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm0,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+4),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm1,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+8),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm2,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+12),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm3,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0];
			  break;
			case 16:
			  movq_m2r(*cspline_wp,mm0);
			  movq_m2r(*(cspline_wp+4),mm1);
			  movq_m2r(*(cspline_wp+8),mm2);
			  movq_m2r(*(cspline_wp+12),mm3);
			  movq_m2r(*(line),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm0,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+4),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm1,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+8),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm2,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  movq_m2r(*(line+12),mm6);     punpcklbw_r2r(mm7,mm6);
			  pmaddwd_r2r(mm3,mm6);     movq_r2m(mm6,*(mmx_t *)mmx_res);
			  value1+=mmx_res[0]+mmx_res[1];
			  break;
			default:
			  mjpeg_error_exit1("width neighbors = %d, is not supported inside cubic-scale function",width_neighbors);
			  break;
		       }
		     cspline_wp+=width_neighbors;

		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// a line on output is now finished. We jump to the beginning of the next line
		output_p+=output_offset;
		line_0+=local_padded_width;
	     }
	}
      else
#endif	
 */
	{
	   // We only scale on width, not height
	   width_neighbors-=zero_width_neighbors;
	   height_neighbors-=zero_height_neighbors;
	   line_0=padded_input;
	   for (out_line = 0; out_line < local_output_active_height; out_line++)
	     {
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_0 + *(in_col_p++);
		     value1=*(line++)*(*(cspline_wp++));
		     for (w=1;w<width_neighbors-1;w++) 
		       value1+=*(line++)*(*(cspline_wp++));
		     value1+=*(line)*(*(cspline_wp++));
		     if (zero_width_neighbors)
		       cspline_wp++;
		     
		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// a line on output is now finished. We jump to the beginning of the next line
		output_p+=output_offset;
		line_0+=local_padded_width;
	     }
	}
      break;
   
   

  case 5:
      // We only scale on height, not width
	{
	   width_neighbors-=zero_width_neighbors;
	   height_neighbors-=zero_height_neighbors;
	   cspline_hp0=cspline_h;
	   in_line_p=in_line;
	   for (out_line = 0; out_line < local_output_active_height; out_line++)
	     {
		line_begin=padded_input + *(in_line_p++) * local_padded_width;
		for (out_col = 0; out_col < local_output_active_width-1; out_col++)
		  {
		     cspline_hp=cspline_hp0;
		     value1 = *(line_begin)*(*(cspline_hp++));
		     for (h=1;h<height_neighbors-1;h++)
		       value1 += (*(line_begin+h*local_padded_width))*(*(cspline_hp++));
		     value1 += (*((line_begin++)+h*local_padded_width))*(*(cspline_hp));

		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// last out_col to be treated => cspline_hp0 incremented => we use the C "++" facility
		value1 = *(line_begin)*(*(cspline_hp0++));
		for (h=1;h<height_neighbors;h++)
		  value1 += (*(line_begin+h*local_padded_width))*(*(cspline_hp0++));
		if (zero_height_neighbors)
		  cspline_hp0++;
		
		if (value1 < 0) *(output_p++) = 0;
		else {
		   value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
		   if (value > 255) *(output_p++) = 255;
		   else *(output_p++) = (uint8_t) value;
		}
		// a line on output is now finished. We jump to the beginning of the next line
		output_p+=output_offset;
	     }
	}
      break;
   }

   /* *INDENT-ON* */
   //   mjpeg_debug ("End of cubic_scale");
   return (0);
}

// *************************************************************************************


// *************************************************************************************
int
cubic_scale_interlaced (uint8_t * padded_top, uint8_t * padded_bottom, uint8_t * output, 
			unsigned int *in_col, unsigned int *in_line,
			int16_t * cspline_w, uint16_t  width_neighbors, uint8_t zero_width_neighbors,
			int16_t * cspline_h, uint16_t height_neighbors, uint8_t zero_height_neighbors,
			unsigned int half)
{
  // Warning: because cubic-spline values may be <0 or >1.0, a range test on value is mandatory
  unsigned int local_output_active_width = output_active_width >> half;
  unsigned int local_output_active_height = output_active_height >> half;
  unsigned int local_output_width = output_width >> half;
  unsigned int local_input_useful_height = input_useful_height >> half;
  unsigned int local_input_useful_width = input_useful_width >> half;
  unsigned int local_padded_height = local_input_useful_height + height_neighbors -1;
  unsigned int local_padded_width = local_input_useful_width + width_neighbors -1;
  unsigned int out_line, out_col,w,h;
  unsigned int *in_line_p,*in_col_p;
  int16_t output_offset;

  uint8_t *output_p,*line,*line_begin,*line_top,*line_bot;
  int16_t *cspline_wp,*cspline_hp,*cspline_hp0;
  int32_t value=0,value1=0,*inter_begin,*intermediate_p,*intermediate_top_p,*intermediate_bot_p;;

//   mjpeg_debug ("Start of cubic_scale ");

   output_p = output;
   output_offset = local_output_width-local_output_active_width;
   width_neighbors-=zero_width_neighbors;
   height_neighbors-=zero_height_neighbors;
   
   /* *INDENT-OFF* */
   
   switch(specific) {
    case 0: 
	{
	   // First scale along the Width, not the height
	   // TOP, then BOTTOM
	   intermediate_p=intermediate;
	   line_top = padded_top;
	   for (out_line = 0; out_line < (local_padded_height>>1); out_line++)
	     {
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_top + *(in_col_p++);
		     value1=*(line++)*(*(cspline_wp++));
		     for (w=1;w<width_neighbors-1;w++) 
		       value1+=*(line++)*(*(cspline_wp++));
		     value1+=*(line)*(*(cspline_wp++));
		     if (zero_width_neighbors)
		       cspline_wp++;
		     *(intermediate_p++)=value1;
		  }
		// a line of intermediate in now finished. Make line_0 points on the next line of padded_input
		line_top+=local_padded_width;
	     }
	   line_bot = padded_bottom;
	   for (out_line = 0; out_line < (local_padded_height>>1); out_line++)
	     {
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_bot + *(in_col_p++);
		     value1=*(line++)*(*(cspline_wp++));
		     for (w=1;w<width_neighbors-1;w++) 
		       value1+=*(line++)*(*(cspline_wp++));
		     value1+=*(line)*(*(cspline_wp++));
		     if (zero_width_neighbors)
		       cspline_wp++;
		     *(intermediate_p++)=value1;
		  }
		// a line of intermediate in now finished. Make line_0 points on the next line of padded_input
		line_bot+=local_padded_width;
	     }

	   
	   // Intermediate now contains an width-scaled frame. top frame on top and bottom frame on bottom
	   // we now scale it along the height, not the width
	   
	   intermediate_top_p=intermediate;
	   intermediate_bot_p=intermediate+(local_padded_height>>1)*local_output_active_width;
	   cspline_hp0=cspline_h;
	   in_line_p=in_line;
	   for (out_line = 0; out_line < (local_output_active_height>>1); out_line++)
	     {
		// TOP line
		inter_begin=intermediate_top_p + *(in_line_p) * local_output_active_width;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     cspline_hp=cspline_hp0;
		     value1 = *(inter_begin)*(*(cspline_hp++));
		     for (h=1;h<height_neighbors-1;h++)
		       value1 += (*(inter_begin+h*local_output_active_width))*(*(cspline_hp++));
		     value1 += (*((inter_begin++)+h*local_output_active_width))*(*(cspline_hp));
		     
		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value =(value1 + DBLEFLOAT2INTOFFSET) >> DBLEFLOAT2INT;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// a top line on output is now finished. We jump to the beginning of the next bottom line
		output_p+=output_offset;
		
		// BOTTOM line
		inter_begin=intermediate_bot_p + *(in_line_p++) * local_output_active_width;
		for (out_col = 0; out_col < local_output_active_width-1; out_col++)
		  {
		     cspline_hp=cspline_hp0;
		     value1 = *(inter_begin)*(*(cspline_hp++));
		     for (h=1;h<height_neighbors-1;h++)
		       value1 += (*(inter_begin+h*local_output_active_width))*(*(cspline_hp++));
		     value1 += (*((inter_begin++)+h*local_output_active_width))*(*(cspline_hp));
		     
		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value =(value1 + DBLEFLOAT2INTOFFSET) >> DBLEFLOAT2INT;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// last out_col to be treated => cspline_hp0 incremented => we use the C "++" facility
		value1 = *(inter_begin)*(*(cspline_hp0++));
		for (h=1;h<height_neighbors;h++)
		  value1 += (*(inter_begin+h*local_output_active_width))*(*(cspline_hp0++));
		if (zero_height_neighbors)
		  cspline_hp0++;
		
		if (value1 < 0) *(output_p++) = 0;
		else {
		   value =(value1 + DBLEFLOAT2INTOFFSET) >> DBLEFLOAT2INT;
		   if (value > 255) *(output_p++) = 255;
		   else *(output_p++) = (uint8_t) value;
		}
		// a bottom line on output is now finished. We jump to the beginning of the next top line
		output_p+=output_offset;
	     }
	}
    break;

      
  case 1:
      // We only scale on width, not height
	{
	   line_top=padded_top;
	   line_bot=padded_bottom;
	   for (out_line = 0; out_line < (local_output_active_height>>1); out_line++)
	     {
		// TOP LINE
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_top + *(in_col_p++);
		     value1=*(line++)*(*(cspline_wp++));
		     for (w=1;w<width_neighbors-1;w++) 
		       value1+=*(line++)*(*(cspline_wp++));
		     value1+=*(line)*(*(cspline_wp++));
		     if (zero_width_neighbors)
		       cspline_wp++;
		     
		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// a top line on output is now finished. We jump to the beginning of the next bottom line
		output_p+=output_offset;
		line_top+=local_padded_width;

		// BOTTOM LINE
		cspline_wp=cspline_w;
		in_col_p=in_col;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     line = line_bot + *(in_col_p++);
		     value1=*(line++)*(*(cspline_wp++));
		     for (w=1;w<width_neighbors-1;w++) 
		       value1+=*(line++)*(*(cspline_wp++));
		     value1+=*(line)*(*(cspline_wp++));
		     if (zero_width_neighbors)
		       cspline_wp++;
		     
		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// a bottom line on output is now finished. We jump to the beginning of the next top line
		output_p+=output_offset;
		line_bot+=local_padded_width;
	     }
	}
      break;
   

  case 5:
      // We only scale on height, not width
	{
	   cspline_hp0=cspline_h;
	   in_line_p=in_line;
	   for (out_line = 0; out_line < (local_output_active_height>>1); out_line++)
	     {
		// TOP LINE
		line_begin=padded_top + *(in_line_p) * local_padded_width;
		for (out_col = 0; out_col < local_output_active_width; out_col++)
		  {
		     cspline_hp=cspline_hp0;
		     value1 = *(line_begin)*(*(cspline_hp++));
		     for (h=1;h<height_neighbors-1;h++)
		       value1 += (*(line_begin+h*local_padded_width))*(*(cspline_hp++));
		     value1 += (*((line_begin++)+h*local_padded_width))*(*(cspline_hp));

		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// a top line on output is now finished. We jump to the beginning of the next bottom line
		output_p+=output_offset;
		
		// BOTTTOM LINE
		line_begin=padded_bottom + *(in_line_p++) * local_padded_width;
		for (out_col = 0; out_col < local_output_active_width-1; out_col++)
		  {
		     cspline_hp=cspline_hp0;
		     value1 = *(line_begin)*(*(cspline_hp++));
		     for (h=1;h<height_neighbors-1;h++)
		       value1 += (*(line_begin+h*local_padded_width))*(*(cspline_hp++));
		     value1 += (*((line_begin++)+h*local_padded_width))*(*(cspline_hp));

		     if (value1 < 0) *(output_p++) = 0;
		     else {
			value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
			if (value > 255) *(output_p++) = 255;
			else *(output_p++) = (uint8_t) value;
		     }
		  }
		// last out_col to be treated => cspline_hp0 incremented => we use the C "++" facility
		value1 = *(line_begin)*(*(cspline_hp0++));
		for (h=1;h<height_neighbors;h++)
		  value1 += (*(line_begin+h*local_padded_width))*(*(cspline_hp0++));
		if (zero_height_neighbors)
		  cspline_hp0++;
		
		if (value1 < 0) *(output_p++) = 0;
		else {
		   value = (value1 + FLOAT2INTOFFSET) >> FLOAT2INTEGERPOWER;
		   if (value > 255) *(output_p++) = 255;
		   else *(output_p++) = (uint8_t) value;
		}
		// a bottom line on output is now finished. We jump to the beginning of the next top line
		output_p+=output_offset;
	     }
	}
      break;
   }

   /* *INDENT-ON* */
   //   mjpeg_debug ("End of cubic_scale");
   return (0);
   
}

// *************************************************************************************
// 

// *************************************************************************************
int16_t
cubic_spline (float x, unsigned int multiplicative)
{
  // Implementation of the Mitchell-Netravalli cubic spline, with recommended parameters B and C
  // [after Reconstruction filters in Computer Graphics by P. Mitchel and N. Netravali : Computer Graphics, Volume 22, Number 4, pp 221-228]
  // Normally, coefficiants are float, but they are transformed into integer with 1/FLOAT2INTEGER = 1/2"11 precision for speed reasons.
  // Please note that these coefficient may over and under shoot in the sense that they may be <0.0 and >1.0
  // Given out values of B and C, maximum value is (x=0) 8/9 and undeshoot is bigger than -0.04 (x#1.5)
  const float B = 1.0 / 3.0;
  const float C = 1.0 / 3.0;


  if (fabs (x) < 1)
    return ((int16_t)
	    floor (0.5 +
		   (((12.0 - 9.0 * B - 6.0 * C)  * fabs (x) * fabs (x) * fabs (x) 
		  + (-18.0 + 12.0 * B + 6.0 * C) * fabs (x) * fabs (x) 
		  + (6.0 - 2.0 * B)) / 6.0
		    ) * multiplicative));
  if (fabs (x) <= 2)
    return ((int16_t)
	    floor (0.5 +
		   (((-B - 6.0 * C) * fabs (x) * fabs (x) * fabs (x) +
		     (6.0 * B + 30.0 * C) * fabs (x) * fabs (x) +
		     (-12.0 * B - 48.0 * C) * fabs (x) + (8.0 * B +
							  24.0 * C)) /
		    6.0) * multiplicative));
  if (fabs (x) <= 3)
     return (0);
  mjpeg_info("In function cubic_spline: x=%f >3",x);
  return (0);
}

// *************************************************************************************


// *************************************************************************************
int
padding (uint8_t * padded_input, uint8_t * input, unsigned int half, 
	 uint16_t left_offset, uint16_t top_offset, uint16_t right_offset, uint16_t bottom_offset,
	 uint16_t width_pad)
{
  // In cubic interpolation, output pixel are evaluated from the 4*4 to 12*12 nearest neigbors. 
  // For border pixels, this requires that input datas along the edge to be padded. 
  // We choose to pad border pixel with black pixel, since border pixel along width are much of the time non-visible
  // (TV set for example) and along the height they are either non-visible or black borders are displayed 
  // This padding functions requires output_interlaced==0
  unsigned int local_input_useful_width = input_useful_width >> half;
  unsigned int local_input_useful_height = input_useful_height >> half;
  unsigned int local_padded_width = local_input_useful_width + width_pad;
  unsigned int local_input_width = input_width >> half;
  unsigned int line;
  uint8_t black,*uint8_pad,*uint8_inp;
  unsigned long int nb_top=top_offset*local_padded_width;

  // mjpeg_debug ("Start of padding, left_offset=%d,top_offset=%d,right_offset=%d,bottom_offset=%d,width_pad=%d",
//	       left_offset,top_offset,right_offset,bottom_offset,width_pad);
  if (half)
     black=128;
  else
     black=16;
  // PADDING
  // vertical offset of top_offset lines
  // Black pixel on the left_offset left pixels
  // Content Copy with left_offset pixels offset on the left and right_offset pixels of the right
  // Black pixel on the right_offset right pixels
  // vertical offset of the last bottom_offset lines
  
  memset(padded_input,black,nb_top);
  uint8_inp=input;
  uint8_pad=padded_input+nb_top;
  for (line = 0; line < local_input_useful_height; line++) 
     {
	memset(uint8_pad,black,left_offset);
	uint8_pad+=left_offset;
	memcpy (uint8_pad, uint8_inp, local_input_useful_width);
	uint8_pad+=local_input_useful_width;
	uint8_inp+=local_input_width; // it is local_input_width, not local_input_useful_width, see yuvscaler_implementation.txt
	memset(uint8_pad,black,right_offset);
	uint8_pad+=right_offset;
     }
  memset(uint8_pad,black,bottom_offset*local_padded_width);
  // mjpeg_debug ("End of padding");

  return (0);
}

// *************************************************************************************

// *************************************************************************************
int
padding_interlaced (uint8_t * padded_top, uint8_t * padded_bottom, uint8_t * input, unsigned int half,
		    uint16_t left_offset, uint16_t top_offset, uint16_t right_offset, uint16_t bottom_offset,
		    uint16_t width_pad)
{
  unsigned int local_input_useful_width = input_useful_width >> half;
  unsigned int local_input_useful_height = input_useful_height >> half;
  unsigned int local_padded_width = local_input_useful_width + width_pad;
  unsigned int local_input_width = input_width >> half;
  unsigned int line;
  uint8_t black, * uint8_ptop, * uint8_pbot, * uint8_inp;
  unsigned long int nb_top=top_offset*local_padded_width;
  unsigned long int nb_bot=bottom_offset*local_padded_width;

  // mjpeg_debug ("Start of padding_interlaced, left_offset=%d,top_offset=%d,right_offset=%d,bottom_offset=%d,width_pad=%d",
//	       left_offset,top_offset,right_offset,bottom_offset,width_pad);
  if (half)
     black=128;
  else
     black=16;
  // PADDING
  // vertical offset of top_offset lines
  // Black pixel on the left_offset left pixels
  // Content Copy with left_offset pixels offset on the left and right_offset pixels of the right
  // Black pixel on the right_offset right pixels
  // vertical offset of the last bottom_offset lines
  
  memset(padded_top,black,nb_top);
  memset(padded_bottom,black,nb_top);
  uint8_inp=input;
  uint8_ptop=padded_top+nb_top;
  uint8_pbot=padded_bottom+nb_top;
  for (line = 0; line < (local_input_useful_height >> 1); line++) 
     {
	memset(uint8_ptop,black,left_offset);
	uint8_ptop+=left_offset;
	memset(uint8_pbot,black,left_offset);
	uint8_pbot+=left_offset;
	memcpy (uint8_ptop, uint8_inp, local_input_useful_width);
	uint8_ptop+=local_input_useful_width;
	uint8_inp +=local_input_width; // it is local_input_width, not local_input_useful_width, see yuvscaler_implementation.txt
	memcpy (uint8_pbot, uint8_inp, local_input_useful_width);
	uint8_pbot+=local_input_useful_width;
	uint8_inp +=local_input_width; // it is local_input_width, not local_input_useful_width, see yuvscaler_implementation.txt
	memset(uint8_ptop,black,right_offset);
	uint8_ptop+=right_offset;
	memset(uint8_pbot,black,right_offset);
	uint8_pbot+=right_offset;
     }
  memset(uint8_ptop,black,nb_bot);
  memset(uint8_pbot,black,nb_bot);

  // mjpeg_debug ("End of padding_interlaced");
  return (0);

}

// *************************************************************************************

				  // THE FOLLOWING LINE "if (!mmx) mmx=0;" DOES NO USEFUL CALCULATION BUT
				  // it is necessary when gcc compilation with -O2 flag 
				  // to calculate correct values for value1+=(mmx_res[0]+mmx_res[1])*(*csplineh[h]);
				  // I know this sounds incredible, but believe me, it is true !
				  // On the other hand, using only gcc -O1, this line is no more necessary.
				  // And in both case, mmx_res[0],mmx_res[1] and *csplineh[h] have the same values
				  // Indeed, the corresponding machine code is totally different with or without
				  // this line with gcc -O2.
				  // The line value1+=(mmx_res[0]+mmx_res[1])*(*csplineh[h]) is compiled into
				  // (From DDD):
				  // Right calculation, that is including the "if (!mmx) mmx=0;" line
				  // --> 0x804f07f <cubic_scale+1615>:pmaddwd %mm0,%mm6
				  // --> 0x804f082 <cubic_scale+1618>:movq   %mm6,(%ecx)
				  //     0x804f085 <cubic_scale+1621>:mov    (%ecx),%edx
				  //     0x804f087 <cubic_scale+1623>:mov    0x4(%ecx),%eax
				  //     0x804f08a <cubic_scale+1626>:mov    0xffffffbc(%ebp),%ebx
				  //     0x804f08d <cubic_scale+1629>:add    %edx,%eax
				  //     0x804f08f <cubic_scale+1631>:mov    (%ebx,%edi,4),%edx
				  // --> 0x804f092 <cubic_scale+1634>:inc    %edi
				  // Wrong calculation, that is not including the "if (!mmx) mmx=0;" line
				  // --> 0x804f062 <cubic_scale+1586>:pmaddwd %mm0,%mm6
				  // --> 0x804f065 <cubic_scale+1589>:movq   %mm6,(%ecx)
				  //     0x804f068 <cubic_scale+1592>:mov    0xffffffbc(%ebp),%ebx
				  //     0x804f06b <cubic_scale+1595>:mov    (%ecx),%eax
				  //     0x804f06d <cubic_scale+1597>:mov    (%ebx,%edi,4),%edx
				  //     0x804f070 <cubic_scale+1600>:add    %esi,%eax
				  // --> 0x804f072 <cubic_scale+1602>:inc    %edi
				  
//				  if (!mmx)
//				    mmx=0;
