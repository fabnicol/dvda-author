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

// From outide MAIN : global variables

extern unsigned int input_width;
extern unsigned int output_width;
// Downscaling ratios
extern unsigned int input_height_slice;
extern unsigned int output_height_slice;
extern unsigned int input_width_slice;
extern unsigned int output_width_slice;

extern int interlaced; 

extern unsigned int output_active_width;
extern unsigned int output_active_height;

extern int line_switching;
extern unsigned int specific;

extern unsigned int out_nb_col_slice, out_nb_line_slice;
extern uint8_t *divide;

// From inside MAIN function



// *************************************************************************************
int
average_coeff (unsigned int input_length, unsigned int output_length,
	       unsigned int *coeff)
{
  // This function calculates multiplicative coeeficients to average an input (vector of) length
  // input_length into an output (vector of) length output_length;
  // We sequentially store the number-of-non-zero-coefficients, followed by the coefficients
  // themselvesc, and that, output_length time
  int last_coeff = 0, remaining_coeff, still_to_go = 0, in, out, non_zero =
    0, nb;
  unsigned int *non_zero_p = NULL;
  unsigned int *pointer;

  if ((output_length > input_length) || (input_length == 0)
      || (output_length == 0) || (coeff == NULL))
    {
      mjpeg_error ("Function average_coeff : arguments are wrong");
      mjpeg_error ("input length = %d, output length = %d, input = %p",
		   input_length, output_length, coeff);
      exit (1);
    }
#ifdef DEBUG
  mjpeg_debug
    ("Function average_coeff : input length = %d, output length = %d, input = %p",
     input_length, output_length, coeff);
#endif

  pointer = coeff;

  if (output_length == 1)
    {
      *pointer = input_length;
      pointer++;
      for (in = 0; in < input_length; in++)
	{
	  *pointer = 1;
	  pointer++;
	}
    }
  else
    {
      for (in = 0; in < output_length; in++)
	{
	  non_zero = 0;
	  non_zero_p = pointer;
	  pointer++;
	  still_to_go = input_length;
	  if (last_coeff > 0)
	    {
	      remaining_coeff = output_length - last_coeff;
	      *pointer = remaining_coeff;
	      pointer++;
	      non_zero++;
	      still_to_go -= remaining_coeff;
	    }
	  nb = (still_to_go / output_length);
#ifdef DEBUG
	  mjpeg_debug ("in=%d,nb=%d,stgo=%d ol=%d", in, nb, still_to_go,
		       output_length);
#endif
	  for (out = 0; out < nb; out++)
	    {
	      *pointer = output_length;
	      pointer++;
	    }
	  still_to_go -= nb * output_length;
	  non_zero += nb;

	  if ((last_coeff = still_to_go) != 0)
	    {
	      *pointer = last_coeff;
#ifdef DEBUG
	      mjpeg_debug ("non_zero=%d,last_coeff=%d", non_zero,
			   last_coeff);
#endif
	      pointer++;	// now pointer points onto the next number-of-non_zero-coefficients
	      non_zero++;
	      *non_zero_p = non_zero;
	    }
	  else
	    {
	      if (in != output_length - 1)
		{
		  mjpeg_error
		    ("There is a common divider between %d and %d\n This should not be the case",
		     input_length, output_length);
		  exit (1);
		}
	    }

	}
      *non_zero_p = non_zero;

      if (still_to_go != 0)
	{
	  mjpeg_error
	    ("Function average_coeff : calculus doesn't stop right : %d",
	     still_to_go);
	}
    }
#ifdef DEBUG
  if (verbose == 2)
    {
      int i, j;
      for (i = 0; i < output_length; i++)
	{
	  mjpeg_debug ("line=%d", i);
	  non_zero = *coeff;
	  coeff++;
	  mjpeg_debug (" ");
	  for (j = 0; j < non_zero; j++)
	    {
	      fprintf (stderr, "%d : %d ", j, *coeff);
	      coeff++;
	    }
	  fprintf (stderr, "\n");
	}
    }
#endif
   return (0);
}

// *************************************************************************************



// *************************************************************************************
int
average (uint8_t * input, uint8_t * output, unsigned int *height_coeff,
	 unsigned int *width_coeff, unsigned int half)
{
  // This function average an input matrix of name input and of size local_input_width*(local_out_nb_line_slice*input_height_slice)
  // into an output matrix of name output and of size local_output_width*(local_out_nb_line_slice+output_height_slice)
  // input and output images are interleaved
  // if half==1 => we are dealing with an U or V component => height and width are / 2 => for speed sake, we use >>half
  unsigned int local_input_width = input_width >> half;
  unsigned int local_output_width = output_width >> half;
  unsigned int local_out_nb_col_slice = out_nb_col_slice >> half;
  unsigned int local_out_nb_line_slice = out_nb_line_slice >> half;
  uint8_t *input_line_p[input_height_slice];
  uint8_t *output_line_p[output_height_slice];
  unsigned int *H_var, *W_var, *H, *W;
  uint8_t *u_c_p;
  int j, nb_H, nb_W, in_line, first_line, out_line;
  int out_col_slice, out_col;
  int out_line_slice;
  int current_line, last_line;
  unsigned long int value = 0;

  //Init
  mjpeg_debug ("Start of average");
  //End of INIT

  if (interlaced == Y4M_ILACE_NONE)
    {
      mjpeg_debug ("Non-interlaced downscaling");
      // output frames are not interlaced => averaging will generate output lines is growing order, 
      // output_height_slice lines per output_height_slice lines. 

      // More important is the following question :
      // is input frames CONTENT interlaced or not (input frames are then said progressives). If content is interlaced (odd lines corresponds to time t 
      // and even lines to another time t+dt with dt=1/(2*frame_rate)), then input frames should be DEINTERLACED prior to averaging
      // So, if input frames are interlaced, we will suppose they are progressives
      // TO BE PROGRAMMED, cf. FlaskMPEG
      for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	   out_line_slice++)
	{
	  u_c_p = input +
	    out_line_slice * input_height_slice * local_input_width;
	  for (in_line = 0; in_line < input_height_slice; in_line++)
	    {
	      input_line_p[in_line] = u_c_p;
	      u_c_p += local_input_width;
	    }
	  u_c_p =
	    output +
	    out_line_slice * output_height_slice * local_output_width;
	  for (out_line = 0; out_line < output_height_slice; out_line++)
	    {
	      output_line_p[out_line] = u_c_p;
	      u_c_p += local_output_width;
	    }
	  for (out_col_slice = 0; out_col_slice < local_out_nb_col_slice;
	       out_col_slice++)
	    {
	      H = height_coeff;
	      first_line = 0;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  nb_H = *H;
		  W = width_coeff;
		  for (out_col = 0; out_col < output_width_slice; out_col++)
		    {
		      H_var = H + 1;
		      nb_W = *W;
		      value = 0;
		      last_line = first_line + nb_H;
		      for (current_line = first_line;
			   current_line < last_line; current_line++)
			{
			  W_var = W + 1;
			  // we average nb_W columns of input : we increment input_line_p[current_line] and W_var each time, except for the last value where 
			  // input_line_p[current_line] and W_var do not need to be incremented, but H_var does
			  for (j = 0; j < nb_W - 1; j++)
			    value +=
			      (*H_var) * (*W_var++) *
			      (*input_line_p[current_line]++);
			  value +=
			    (*H_var++) * (*W_var) *
			    (*input_line_p[current_line]);
			}
		      //                Straiforward implementation is 
		      //                *(output_line_p[out_line]++)=value/diviseur;
		      //                round_off_error=value%diviseur;
		      //                Here, we speed up things but using the pretabulated nearest integral parts
		      *(output_line_p[out_line]++) = divide[value];
		      W += nb_W + 1;
		    }
		  H += nb_H + 1;
		  first_line += nb_H - 1;
		  input_line_p[first_line] -= input_width_slice - 1;
		  // If last line of input is to be reused in next loop, 
		  // make the pointer points at the correct place
		}
	      input_line_p[first_line] += input_width_slice - 1;
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		input_line_p[in_line]++;
	    }
	}
    }
  else
    {
      // output frames are interlaced, line numbers gioes from 0 to n-1. 
      // Therefore, downscaling is done between odd lines, then between even lines, but we do not mix odd and even lines.
      // So, we have to calculate the even and odd part of out_line_slice. 
      // If the odd part is naturally out_line_slice % 2, the even part is (out_line_slice/2)*2. For speed reason, 
      // the even part will be xritten as out_line_slice & ~(unsigned int) 1
      mjpeg_debug ("Interlaced downscaling");
      for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	   out_line_slice++)
	{
	  u_c_p =
	    input +
	    ((out_line_slice & ~(unsigned int) 1) * input_height_slice +
	     out_line_slice % 2) * local_input_width;
	  for (in_line = 0; in_line < input_height_slice; in_line++)
	    {
	      input_line_p[in_line] = u_c_p;
	      u_c_p += 2 * local_input_width;
	    }
	  u_c_p =
	    output +
	    ((out_line_slice & ~(unsigned int) 1) * output_height_slice +
	     out_line_slice % 2) * local_output_width;
	  for (out_line = 0; out_line < output_height_slice; out_line++)
	    {
	      output_line_p[out_line] = u_c_p;
	      u_c_p += 2 * local_output_width;
	    }

	  for (out_col_slice = 0; out_col_slice < local_out_nb_col_slice;
	       out_col_slice++)
	    {
	      H = height_coeff;
	      first_line = 0;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  nb_H = *H;
		  W = width_coeff;
		  for (out_col = 0; out_col < output_width_slice; out_col++)
		    {
		      H_var = H + 1;
		      nb_W = *W;
		      value = 0;
		      last_line = first_line + nb_H;
		      for (current_line = first_line;
			   current_line < last_line; current_line++)
			{
			  W_var = W + 1;
			  // we average nb_W columns of input : we increment input_line_p[current_line] and W_var each time, except for the last value where 
			  // input_line_p[current_line] and W_var do not need to be incremented, but H_var does
			  for (j = 0; j < nb_W - 1; j++)
			    value +=
			      (*H_var) * (*W_var++) *
			      (*input_line_p[current_line]++);
			  value +=
			    (*H_var++) * (*W_var) *
			    (*input_line_p[current_line]);
			}
		      //                Straiforward implementation is 
		      //                *(output_line_p[out_line]++)=value/diviseur;
		      //                round_off_error=value%diviseur;
		      //                Here, we speed up things but using the pretabulated integral parts
		      *(output_line_p[out_line]++) = divide[value];
		      W += nb_W + 1;
		    }
		  H += nb_H + 1;
		  first_line += nb_H - 1;
		  input_line_p[first_line] -= input_width_slice - 1;
		  // If last line of input is to be reused in next loop, 
		  // make the pointer points at the correct place
		}
	      input_line_p[first_line] += input_width_slice - 1;
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		input_line_p[in_line]++;
	    }
	}
    }
  mjpeg_debug ("End of average");
  return (0);
}

// *************************************************************************************



// *************************************************************************************
int
average_specific (uint8_t * input, uint8_t * output,
		  unsigned int *height_coeff, unsigned int *width_coeff,
		  unsigned int half)
{
  // This function gathers code that are speed enhanced due to specific downscaling ratios     
  unsigned int line_index;
  unsigned int local_output_active_height = output_active_height >> half;
  unsigned int local_input_width = input_width >> half;
  unsigned int local_output_width = output_width >> half;
  unsigned int local_output_active_width = output_active_width >> half;
  unsigned int local_out_nb_col_slice = out_nb_col_slice >> half;
  unsigned int local_out_nb_line_slice = out_nb_line_slice >> half;
  unsigned int number,in_line_offset,out_line_offset;
  // specific==1, 4
  uint8_t temp_uint8_t;
  uint8_t *in_line_p;
  uint8_t *out_line_p;
  unsigned int *W_var, *W;
  int j, nb_W;
  unsigned int out_col_slice, out_col;
  int treatment = 0;
  unsigned long int value = 0, value1 = 0, value2 = 0, value3 = 0;
  // Specific==2
  uint8_t *in_first_line_p, *in_second_line_p;
  unsigned int out_line;
  // specific=3
  unsigned char *u_c_p;
  unsigned int in_line;
  uint8_t *input_line_p[input_height_slice];
  // specific=5
  unsigned int *H_var, *H;
  unsigned int nb_H, first_line, last_line, current_line;
  unsigned int out_line_slice;
  uint8_t *output_line_p[output_height_slice];

  //Init
  mjpeg_debug ("Start of average_specific %u", specific);
  //End of INIT

  if (specific == 1)
    {
      treatment = 1;
      mjpeg_debug ("Non interlaced and/or interlaced treatment");
      // We just take the average along the width, not the height, line per line
      // Infered from average, with input_height_slice=output_height_slice=1;
      for (line_index = 0; line_index < local_output_active_height;
	   line_index++)
	{
	  in_line_p = input + line_index * local_input_width;
	  out_line_p = output + line_index * local_output_width;
	  for (out_col_slice = 0; out_col_slice < local_out_nb_col_slice;
	       out_col_slice++)
	    {
	      W = width_coeff;
	      for (out_col = 0; out_col < output_width_slice; out_col++)
		{
		  nb_W = *W;
		  value = 0;
		  W_var = W + 1;
		  for (j = 0; j < nb_W - 1; j++)
		    value += (*W_var++) * (*in_line_p++);
		  value += (*W_var) * (*in_line_p);
		  *(out_line_p++) = divide[value];
		  W += nb_W + 1;
		}
	      in_line_p++;
	    }
	}
    }



  if (specific == 2)
    {
      treatment = 2;
      // SPECIAL FAST Full_size to VCD downscaling : 2to1 for width and height
      // Since 2 to 1 height dowscaling, no need for line switching
      // Drawback: slight distortion on width
      if (interlaced == Y4M_ILACE_NONE)
	{
	  mjpeg_debug ("Non-interlaced downscaling");
	  for (out_line = 0; out_line < local_output_active_height;
	       out_line++)
	    {
	      in_first_line_p = input + out_line * (local_input_width << 1);
	      in_second_line_p = in_first_line_p + local_input_width;
	      out_line_p = output + out_line * local_output_width;
	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  // Division of integers is always made by default. This results in a systematic drift towards smaller values. 
		  // What we really need,
		  // is a division that takes the nearest integer. 
		  // So, we add 1/2 of the divider to the value to be divided
//                *(out_line_p++) =
//                  (2 + *(in_first_line_p) + *(in_first_line_p + 1) +
//                   *(in_second_line_p) + *(in_second_line_p + 1)) >> 2;
		  *(out_line_p++) =
		    divide[*(in_first_line_p) + *(in_first_line_p + 1) +
			   *(in_second_line_p) + *(in_second_line_p + 1)];
		  in_first_line_p += 2;
		  in_second_line_p += 2;
		}
	    }
	}
      else
	{
	  mjpeg_debug ("Interlaced downscaling");
	  for (line_index = 0; line_index < local_output_active_height;
	       line_index++)
	    {
	      in_first_line_p =
		input + (((line_index & ~(unsigned int) 1) << 1) +
			 (line_index % 2)) * local_input_width;
	      in_second_line_p = in_first_line_p + (local_input_width << 1);
	      out_line_p = output + line_index * local_output_width;
	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
/*		  *(out_line_p++) =
		    (2 + *(in_first_line_p) + *(in_first_line_p + 1) +
		     *(in_second_line_p) + *(in_second_line_p + 1)) >> 2;
*/ *(out_line_p++) = divide[*(in_first_line_p) + *(in_first_line_p + 1) +
											*
											(in_second_line_p)
											+
											*
											(in_second_line_p
											 +
											 1)];
		  in_first_line_p += 2;
		  in_second_line_p += 2;
		}
	    }
	}
    }


  if (specific == 3)
    {
      treatment = 3;
      // input_height_slice=2, output_height_slice=1 => input lines will be summed together.
      // infered from average with output_height_slice=1 and explicity writting of the for(in_line=0;in_line<input_height_slice;in_line++)
      // Special VCD downscaling without width distortion
      if (interlaced == Y4M_ILACE_NONE)
	{
	  mjpeg_debug ("Non-interlaced downscaling");
	  for (out_line = 0; out_line < local_output_active_height;
	       out_line++)
	    {
	      input_line_p[0] =
		input + out_line * input_height_slice * local_input_width;
	      input_line_p[1] = input_line_p[0] + local_input_width;
	      out_line_p = output + out_line * local_output_width;
	      for (out_col_slice = 0;
		   out_col_slice < local_out_nb_col_slice; out_col_slice++)
		{
		  W = width_coeff;
		  for (out_col = 0; out_col < output_width_slice; out_col++)
		    {
		      nb_W = *W;
		      value = 0;
		      W_var = W + 1;
		      for (j = 0; j < nb_W - 1; j++)
			value +=
			  (*W_var++) * ((*input_line_p[0]++) +
					(*input_line_p[1]++));
		      value +=
			(*W_var) * (*input_line_p[0] + *input_line_p[1]);
		      *(out_line_p++) = divide[value];
		      W += nb_W + 1;
		    }
		  input_line_p[0]++;
		  input_line_p[1]++;
		}
	    }
	}
      else
	{
	  mjpeg_debug ("Interlaced downscaling");
	  for (line_index = 0; line_index < local_output_active_height;
	       line_index++)
	    {
	      input_line_p[0] =
		input +
		(input_height_slice * (line_index & ~(unsigned int) 1) +
		 line_index % 2) * local_input_width;
	      input_line_p[1] = input_line_p[0] + 2 * local_input_width;
	      out_line_p = output + line_index * local_output_width;
	      for (out_col_slice = 0;
		   out_col_slice < (out_nb_col_slice >> half);
		   out_col_slice++)
		{
		  W = width_coeff;
		  for (out_col = 0; out_col < output_width_slice; out_col++)
		    {
		      nb_W = *W;
		      value = 0;
		      W_var = W + 1;
		      for (j = 0; j < nb_W - 1; j++)
			value +=
			  (*W_var++) * ((*input_line_p[0]++) +
					(*input_line_p[1]++));
		      value +=
			(*W_var) * (*input_line_p[0] + *input_line_p[1]);
		      *(out_line_p++) = divide[value];
		      W += nb_W + 1;
		    }
		  input_line_p[0]++;
		  input_line_p[1]++;
		}
	    }
	}

    }

  if (specific == 4)
    {
      // just a copy: we copy line per line (warning! these lines are output_width long BUT we only copy output_active_width length of them)
      treatment = 4;
      mjpeg_debug ("Non-interlaced or interlaced downscaling");
      for (line_index = 0; line_index < local_output_active_height;
	   line_index++)
//       ;
	memcpy (output + line_index * local_output_width,
		input + line_index * local_input_width,
		local_output_active_width);
    }

  if (specific == 5)
    {
      // We downscale only lines along the height, not the width
      treatment = 5;
      if (interlaced == Y4M_ILACE_NONE)
	{
	  mjpeg_debug ("Non-interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		{
		  number = out_line_slice * input_height_slice + in_line;
		  input_line_p[in_line] = input + number * local_input_width;
		}
	      u_c_p =
		output +
		out_line_slice * output_height_slice * local_output_width;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  output_line_p[out_line] = u_c_p;
		  u_c_p += local_output_width;
		}
	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  H = height_coeff;
		  first_line = 0;
		  for (out_line = 0; out_line < output_height_slice;
		       out_line++)
		    {
		      nb_H = *H;
		      H_var = H + 1;
		      value = 0;
		      last_line = first_line + nb_H;
		      for (current_line = first_line;
			   current_line < last_line; current_line++)
			value += (*H_var++) * (*input_line_p[current_line]);
		      *(output_line_p[out_line]++) = divide[value];
		      H += nb_H + 1;
		      first_line += nb_H - 1;
		    }
		  for (in_line = 0; in_line < input_height_slice; in_line++)
		    input_line_p[in_line]++;
		}
	    }
	}
      else
	{
	  mjpeg_debug ("Interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      u_c_p =
		input +
		((out_line_slice & ~(unsigned int) 1) * input_height_slice +
		 out_line_slice % 2) * local_input_width;
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		{
		  input_line_p[in_line] = u_c_p;
		  u_c_p += 2 * local_input_width;
		}
	      u_c_p =
		output +
		((out_line_slice & ~(unsigned int) 1) *
		 output_height_slice +
		 out_line_slice % 2) * local_output_width;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  output_line_p[out_line] = u_c_p;
		  u_c_p += 2 * local_output_width;
		}

	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  H = height_coeff;
		  first_line = 0;
		  for (out_line = 0; out_line < output_height_slice;
		       out_line++)
		    {
		      nb_H = *H;
		      H_var = H + 1;
		      value = 0;
		      last_line = first_line + nb_H;
		      for (current_line = first_line;
			   current_line < last_line; current_line++)
			value += (*H_var++) * (*input_line_p[current_line]);
		      *(output_line_p[out_line]++) = divide[value];
		      H += nb_H + 1;
		      first_line += nb_H - 1;
		    }
		  for (in_line = 0; in_line < input_height_slice; in_line++)
		    input_line_p[in_line]++;
		}
	    }
	}
    }

  if (specific == 6)
    {
      // Dedicated SVCD: we downscale 3 for 2 on width, and 1 to 1 on height. Infered from specific=1
      // For width, W points are "2 2 1 2 1 2" => we can explicitely write down the calculs of value
      treatment = 6;
      mjpeg_debug ("Non interlaced and/or interlaced treatment");
      in_line_offset  = local_input_width  - local_out_nb_col_slice * 3;
      out_line_offset = local_output_width - local_out_nb_col_slice * 2;
      in_line_p  = input;
      out_line_p = output;
      for (line_index = 0; line_index < local_output_active_height;
	   line_index++)
	{
//	  in_line_p = input + line_index * local_input_width;
//	  out_line_p = output + line_index * local_output_width;
	  for (out_col_slice = 0; out_col_slice < local_out_nb_col_slice;
	       out_col_slice++)
	    {
	      temp_uint8_t = in_line_p[1];
//	      *(out_line_p++) = divide[((*in_line_p) << 1) + temp_uint8_t];
	       *(out_line_p++) = (((*in_line_p) << 1) + temp_uint8_t + 1) / 3;
	      in_line_p += 2;
//	      *(out_line_p++) = divide[temp_uint8_t + ((*in_line_p++) << 1)];
	      *(out_line_p++) = (temp_uint8_t + ((*in_line_p++) << 1) + 1) / 3;	      
	    }
	   in_line_p  += in_line_offset;
	   out_line_p += out_line_offset;
	}
    }

  if (specific == 7)
    {
      // Dedicated to WIDE2STD alone downscaling: 4 to 3 on height, width not downscaled
      // For the height, H is equal to 2 3 1 2 2 2 2 1 3
      // Infered from specific=5
      treatment = 7;
      if (interlaced == Y4M_ILACE_NONE)
	{
	  mjpeg_debug ("Non-interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      input_line_p[0] =
		input + (4 * out_line_slice + 0) * local_input_width;
	      input_line_p[1] =
		input + (4 * out_line_slice + 1) * local_input_width;
	      input_line_p[2] =
		input + (4 * out_line_slice + 2) * local_input_width;
	      input_line_p[3] =
		input + (4 * out_line_slice + 3) * local_input_width;
	      output_line_p[0] =
		output + 3 * out_line_slice * local_output_width;
	      output_line_p[1] = output_line_p[0] + local_output_width;
	      output_line_p[2] = output_line_p[1] + local_output_width;
	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  *(output_line_p[0]++) =
		    divide[3 * (*input_line_p[0]++) + (*input_line_p[1])];
		  *(output_line_p[1]++) =
		    divide[2 * (*input_line_p[1]++) + 2 * (*input_line_p[2])];
		  *(output_line_p[2]++) =
		    divide[(*input_line_p[2]++) + 3 * (*input_line_p[3]++)];
		}
	    }
	}
      else
	{
	  mjpeg_debug ("Interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      u_c_p =
		input +
		((out_line_slice & ~(unsigned int) 1) * input_height_slice +
		 (out_line_slice % 2)) * local_input_width;
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		{
		  input_line_p[in_line] = u_c_p;
		  u_c_p += 2 * local_input_width;
		}
	      u_c_p =
		output +
		((out_line_slice & ~(unsigned int) 1) *
		 output_height_slice +
		 (out_line_slice % 2)) * local_output_width;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  output_line_p[out_line] = u_c_p;
		  u_c_p += 2 * local_output_width;
		}

	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  *(output_line_p[0]++) =
		    divide[3 * (*input_line_p[0]++) + (*input_line_p[1])];
		  *(output_line_p[1]++) =
		    divide[2 * (*input_line_p[1]++) + 2 * (*input_line_p[2])];
		  *(output_line_p[2]++) =
		    divide[(*input_line_p[2]++) + 3 * (*input_line_p[3]++)];
		}
	    }
	}
    }


  if (specific == 8)
    {
      // Special FASTWIDE2VCD mode: 2 to 1 for width, and 8 to 3 for height
      // *8 is replaced by <<3 and 2* by <<1
      // Drawback: slight distortion on width
      // Coefficient for horizontal downscaling : (3,3,2), (1,3,3,1), (2,3,3)
      treatment = 8;
      if (interlaced == Y4M_ILACE_NONE)
	{
	  mjpeg_debug ("Non-interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      input_line_p[0] =
		input + (8 * out_line_slice + 0) * local_input_width;
	      input_line_p[1] =
		input + (8 * out_line_slice + 1) * local_input_width;
	      input_line_p[2] =
		input + (8 * out_line_slice + 2) * local_input_width;
	      input_line_p[3] =
		input + (8 * out_line_slice + 3) * local_input_width;
	      input_line_p[4] =
		input + (8 * out_line_slice + 4) * local_input_width;
	      input_line_p[5] =
		input + (8 * out_line_slice + 5) * local_input_width;
	      input_line_p[6] =
		input + (8 * out_line_slice + 6) * local_input_width;
	      input_line_p[7] =
		input + (8 * out_line_slice + 7) * local_input_width;
	      output_line_p[0] =
		output + out_line_slice * 3 * local_output_width;
	      output_line_p[1] = output_line_p[0] + local_output_width;
	      output_line_p[2] = output_line_p[1] + local_output_width;
	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  *(output_line_p[0]++) =
		    divide[3 * (*input_line_p[0] + (*input_line_p[0] + 1)) +
			   3 * (*input_line_p[1] + (*input_line_p[1] + 1)) +
			   2 * (*input_line_p[2] + (*input_line_p[2] + 1))];
		  input_line_p[0] += 2;
		  input_line_p[1] += 2;
		  *(output_line_p[1]++) = divide[(*input_line_p[2] +
						  (*input_line_p[2] + 1)) +
						 3 * (*input_line_p[3] +
						      (*input_line_p[3] +
						       1)) +
						 3 * (*input_line_p[4] +
						      (*input_line_p[4] +
						       1)) +
						 (*input_line_p[5] +
						  (*input_line_p[5] + 1))];
		  input_line_p[2] += 2;
		  input_line_p[3] += 2;
		  input_line_p[4] += 2;
		  *(output_line_p[2]++) =
		    divide[2 * (*input_line_p[5] + (*input_line_p[5] + 1)) +
			   3 * (*input_line_p[6] + (*input_line_p[6] + 1)) +
			   3 * (*input_line_p[7] + (*input_line_p[7] + 1))];
		  input_line_p[5] += 2;
		  input_line_p[6] += 2;
		  input_line_p[7] += 2;
		}
	    }
	}
      else
	{
	  mjpeg_debug ("Interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      input_line_p[0] =
		input + (((out_line_slice & ~(unsigned int) 1) << 3) +
			 (out_line_slice % 2)) * local_input_width;
	      input_line_p[1] = input_line_p[0] + (local_input_width << 1);
	      input_line_p[2] = input_line_p[1] + (local_input_width << 1);
	      input_line_p[3] = input_line_p[2] + (local_input_width << 1);
	      input_line_p[4] = input_line_p[3] + (local_input_width << 1);
	      input_line_p[5] = input_line_p[4] + (local_input_width << 1);
	      input_line_p[6] = input_line_p[5] + (local_input_width << 1);
	      input_line_p[7] = input_line_p[6] + (local_input_width << 1);
	      output_line_p[0] =
		output + ((out_line_slice & ~(unsigned int) 1) * 3 +
			  (out_line_slice % 2)) * local_output_width;
	      output_line_p[1] = output_line_p[0] + (local_output_width << 1);
	      output_line_p[2] = output_line_p[1] + (local_output_width << 1);
	      for (out_col = 0; out_col < local_output_active_width;
		   out_col++)
		{
		  *(output_line_p[0]++) =
		    divide[3 * (*input_line_p[0] + (*input_line_p[0] + 1)) +
			   3 * (*input_line_p[1] + (*input_line_p[1] + 1)) +
			   2 * (*input_line_p[2] + (*input_line_p[2] + 1))];
		  input_line_p[0] += 2;
		  input_line_p[1] += 2;
		  *(output_line_p[1]++) = divide[(*input_line_p[2] +
						  (*input_line_p[2] + 1)) +
						 3 * (*input_line_p[3] +
						      (*input_line_p[3] +
						       1)) +
						 3 * (*input_line_p[4] +
						      (*input_line_p[4] +
						       1)) +
						 (*input_line_p[5] +
						  (*input_line_p[5] + 1))];
		  input_line_p[2] += 2;
		  input_line_p[3] += 2;
		  input_line_p[4] += 2;
		  *(output_line_p[2]++) =
		    divide[2 * (*input_line_p[5] + (*input_line_p[5] + 1)) +
			   3 * (*input_line_p[6] + (*input_line_p[6] + 1)) +
			   3 * (*input_line_p[7] + (*input_line_p[7] + 1))];
		  input_line_p[5] += 2;
		  input_line_p[6] += 2;
		  input_line_p[7] += 2;
		}
	    }
	}
    }

  if (specific == 9)
    {
      // Special WIDE2VCD, on height : 8->3
      treatment = 9;
      if (interlaced == Y4M_ILACE_NONE)
	{
	  mjpeg_debug ("Non-interlaced downscaling");
	  // input frames are not interlaced, as are output frames.
	  // So, we average input_height_slice following lines into output_height_slice lines
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		{
		  number = out_line_slice * input_height_slice + in_line;
		  input_line_p[in_line] = input + number * local_input_width;
		}
	      u_c_p =
		output +
		out_line_slice * output_height_slice * local_output_width;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  output_line_p[out_line] = u_c_p;
		  u_c_p += local_output_width;
		}
	      for (out_col_slice = 0;
		   out_col_slice < local_out_nb_col_slice; out_col_slice++)
		{
		  W = width_coeff;
		  for (out_col = 0; out_col < output_width_slice; out_col++)
		    {
		      nb_W = *W;
		      value1 = value2 = value3 = 0;
		      W_var = W + 1;
		      for (j = 0; j < nb_W - 1; j++)
			{
			  value1 +=
			    (*W_var) * (3 * (*input_line_p[0]++) +
					3 * (*input_line_p[1]++) +
					2 * (*input_line_p[2]));
			  value2 +=
			    (*W_var) * ((*input_line_p[2]++) +
					3 * (*input_line_p[3]++) +
					3 * (*input_line_p[4]++) +
					(*input_line_p[5]));
			  value3 +=
			    (*W_var++) * (2 * (*input_line_p[5]++) +
					  3 * (*input_line_p[6]++) +
					  3 * (*input_line_p[7]++));
			}
		      value1 +=
			(*W_var) * (3 * (*input_line_p[0]) +
				    3 * (*input_line_p[1]) +
				    2 * (*input_line_p[2]));
		      value2 +=
			(*W_var) * ((*input_line_p[2]) +
				    3 * (*input_line_p[3]) +
				    3 * (*input_line_p[4]) +
				    (*input_line_p[5]));
		      value3 +=
			(*W_var) * (2 * (*input_line_p[5]) +
				    3 * (*input_line_p[6]) +
				    3 * (*input_line_p[7]));
		      *(output_line_p[0]++) = divide[value1];
		      *(output_line_p[1]++) = divide[value2];
		      *(output_line_p[2]++) = divide[value3];
		      W += nb_W + 1;
		    }
		  input_line_p[0]++;
		  input_line_p[1]++;
		  input_line_p[2]++;
		  input_line_p[3]++;
		  input_line_p[4]++;
		  input_line_p[5]++;
		  input_line_p[6]++;
		  input_line_p[7]++;
		}
	    }

	}
      else
	{
	  mjpeg_debug ("Interlaced downscaling");
	  for (out_line_slice = 0; out_line_slice < local_out_nb_line_slice;
	       out_line_slice++)
	    {
	      u_c_p =
		input +
		((out_line_slice & ~(unsigned int) 1) * input_height_slice +
		 (out_line_slice % 2)) * local_input_width;
	      for (in_line = 0; in_line < input_height_slice; in_line++)
		{
		  input_line_p[in_line] = u_c_p;
		  u_c_p += 2 * local_input_width;
		}
	      u_c_p =
		output +
		((out_line_slice & ~(unsigned int) 1) *
		 output_height_slice +
		 (out_line_slice % 2)) * local_output_width;
	      for (out_line = 0; out_line < output_height_slice; out_line++)
		{
		  output_line_p[out_line] = u_c_p;
		  u_c_p += 2 * local_output_width;
		}

	      for (out_col_slice = 0;
		   out_col_slice < local_out_nb_col_slice; out_col_slice++)
		{
		  H = height_coeff;
		  first_line = 0;
		  for (out_line = 0; out_line < output_height_slice;
		       out_line++)
		    {
		      nb_H = *H;
		      W = width_coeff;
		      for (out_col = 0; out_col < output_width_slice;
			   out_col++)
			{
			  H_var = H + 1;
			  nb_W = *W;
			  value = 0;
			  last_line = first_line + nb_H;
			  for (current_line = first_line;
			       current_line < last_line; current_line++)
			    {
			      W_var = W + 1;
			      // we average nb_W columns of input : we increment input_line_p[current_line] and W_var each time, except for the last value where 
			      // input_line_p[current_line] and W_var do not need to be incremented, but H_var does
			      for (j = 0; j < nb_W - 1; j++)
				value +=
				  (*H_var) * (*W_var++) *
				  (*input_line_p[current_line]++);
			      value +=
				(*H_var++) * (*W_var) *
				(*input_line_p[current_line]);
			    }
			  //                Straiforward implementation is 
			  //                *(output_line_p[out_line]++)=value/diviseur;
			  //                round_off_error=value%diviseur;
			  //                Here, we speed up things but using the pretabulated integral parts
			  *(output_line_p[out_line]++) = divide[value];
			  W += nb_W + 1;
			}
		      H += nb_H + 1;
		      first_line += nb_H - 1;
		      input_line_p[first_line] -= input_width_slice - 1;
		      // If last line of input is to be reused in next loop, 
		      // make the pointer points at the correct place
		    }
		  input_line_p[first_line] += input_width_slice - 1;
		  for (in_line = 0; in_line < input_height_slice; in_line++)
		    input_line_p[in_line]++;
		}
	    }
	}
    }



  if (treatment == 0)
    mjpeg_error_exit1 ("Unknown specific downscaling treatment %u",
		       specific);

  mjpeg_debug ("End of average_specific");
  return (0);
}

