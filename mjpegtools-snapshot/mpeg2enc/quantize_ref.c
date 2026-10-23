/* quantize.c, Low-level quantization / inverse quantization
 * routines */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

/* Modifications and enhancements (C) 2000-2003 Andrew Stevens */

/* These modifications are free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */


#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "syntaxconsts.h"
#include "fastintfns.h"
#include "cpu_accel.h"
#include "tables.h"
#include "simd.h"
#include "quantize_ref.h"
#include "quantize_precomp.h"

#ifdef HAVE_ALTIVEC
void enable_altivec_quantization(struct QuantizerCalls *calls, int opt_mpeg1);
#endif
#if defined(HAVE_ASM_MMX)
void init_x86_quantization( struct QuantizerCalls *calls,
                            struct QuantizerWorkSpace *wsp,
                            int mpeg1 );
#endif

#define fabsshift ((8*sizeof(unsigned int))-1)
#define signmask(x) (((int)x)>>fabsshift)
static inline int intsamesign(int x, int y)
{
	return (y+(signmask(x) & -(y<<1)));
}
#undef signmask
#undef fabsshift

/*
 * Return the code for a quantisation level
 */

int quant_code(  int q_scale_type, int mquant )
{
	return q_scale_type ? map_non_linear_mquant[ mquant] : mquant>>1;
}

/*
 *
 * Computes the next quantisation up.  Used to avoid saturation
 * in macroblock coefficients - common in MPEG-1 - which causes
 * nasty artifacts.
 *
 * NOTE: Does no range checking...
 */

int next_larger_quant( int q_scale_type, int quant )
{
	if( q_scale_type )
	{
		if( map_non_linear_mquant[quant]+1 > 31 )
			return quant;
		else
			return non_linear_mquant_table[map_non_linear_mquant[quant]+1];
	}
	else 
	{
		return ( quant+2 > 31 ) ? quant : quant+2;
	}
}

/* 
 * Quantisation for intra blocks using Test Model 5 quantization
 *
 * this quantizer has a bias of 1/8 stepsize towards zero
 * (except for the DC coefficient)
 *
	PRECONDITION: src dst point to *disinct* memory buffers...
		          of block_count *adjact* int16_t[64] arrays... 
 *
 * RETURN: 1 If non-zero coefficients left after quantisaiont 0 otherwise
 */

void quant_intra( struct QuantizerWorkSpace *wsp,
                  int16_t *src, 
				  int16_t *dst,
				  int q_scale_type, 
                  int dc_prec,
                  int clipvalue,
				  int *nonsat_mquant
	)
{
  int16_t *psrc,*pbuf;
  int i,comp;
  int x, y, d;
  int clipping;
  int mquant = *nonsat_mquant;
  uint16_t *quant_mat = wsp->intra_q_tbl[mquant] /* intra_q_mat * mquant */;
  /* 
   * Complicate by handlin clipping by increasing quantisation.  This
   * seems to avoid nasty artifacts in some situations...
   */

  do
	{
	  clipping = 0;
	  pbuf = dst;
	  psrc = src;
	  for( comp = 0; comp<BLOCK_COUNT && !clipping; ++comp )
	  {
		x = psrc[0];
		d = 8>>dc_prec; /* intra_dc_mult */
		pbuf[0] = (x>=0) ? (x+(d>>1))/d : -((-x+(d>>1))/d); /* round(x/d) */


		for (i=1; i<64 ; i++)
		  {
			x = psrc[i];
#ifdef ORIGINAL_CODE
			d = wsp->intra_q_mat[i];
			y = (16*(x >= 0 ? x : -x) + (d>>1))/d; /* round(32*x/quant_mat) */
			d = (3*mquant+2)>>2;
			y = (y+d)/mquant; /* (y+0.75*mquant) / mquant) */
#else

			d = quant_mat[i];
			y = ((abs(x)<<5)  + d ) /(d<<1);
#endif
            if ( y > clipvalue )
            {
              clipping = 1;
              mquant = next_larger_quant(q_scale_type, mquant );
              quant_mat = wsp->intra_q_tbl[mquant];
              break;
            }

		  	pbuf[i] = intsamesign(x,y);
		  }
		pbuf += 64;
		psrc += 64;
	  }
			
	} while( clipping );
  *nonsat_mquant = mquant;
}


/*
 * Quantisation matrix weighted Coefficient sum fixed-point
 * integer with low 16 bits fractional...
 * To be used for rate control as a measure of dct block
 * complexity...
 *
 */

static int quant_weight_coeff_intra( struct QuantizerWorkSpace *wsp,
                                     int16_t *blk  )
{
    uint16_t * i_quant_mat = wsp->i_intra_q_mat;
    int i;
    int sum = 0;
    for( i = 0; i < 64; i+=2 )
	{
		sum += abs((int)blk[i]) * (i_quant_mat[i]) + abs((int)blk[i+1]) * (i_quant_mat[i+1]);
	}
    return sum;
	/* In case you're wondering typical average coeff_sum's for a rather
	 noisy video are around 20.0.  */
}

static int quant_weight_coeff_inter( struct QuantizerWorkSpace *wsp,
                                     int16_t *blk )
{
    uint16_t * i_quant_mat = wsp->i_inter_q_mat;
    int i;
    int sum = 0;
    for( i = 0; i < 64; i+=2 )
	{
		sum += abs((int)blk[i]) * (i_quant_mat[i]) + abs((int)blk[i+1]) * (i_quant_mat[i+1]);
	}
    return sum;
	/* In case you're wondering typical average coeff_sum's for a rather
       noisy video are around 20.0.  */
}

/* 
 * Quantisation for non-intra blocks using Test Model 5 quantization
 *
 * this quantizer has a bias of 1/8 stepsize towards zero
 * (except for the DC coefficient)
 *
 * A.Stevens 2000: The above comment is nonsense.  Only the intra quantiser does
 * this.  This one just truncates with a modest bias of 1/(4*quant_matrix_scale)
 * to 1.
 *
 *	PRECONDITION: src dst point to *disinct* memory buffers...
 *	              of block_count *adjacent* int16_t[64] arrays...
 *
 * RETURN: A bit-mask of block_count bits indicating non-zero blocks (a 1).
 *
 */

int quant_non_intra( struct QuantizerWorkSpace *wsp,
                     int16_t *src, int16_t *dst,
					 int q_scale_type,
                     int clipvalue,
					 int *nonsat_mquant)
{
	int i;
	int x, y, dmquant;
	int nzflag;
	int coeff_count;
	int flags = 0;
	int saturated = 0;
    int mquant = *nonsat_mquant;
	uint16_t *quant_mat = wsp->inter_q_tbl[mquant]; /* inter_q */
	
	coeff_count = 64*BLOCK_COUNT;
	flags = 0;
	nzflag = 0;
	for (i=0; i<coeff_count; ++i)
	{
restart:
		if( (i%64) == 0 )
		{
			nzflag = (nzflag<<1) | !!flags;
			flags = 0;
			  
		}
		/* RJ: save one divide operation */

		x = abs( ((int)src[i]) ) /*(src[i] >= 0 ? src[i] : -src[i])*/ ;
		dmquant = (int)quant_mat[(i&63)]; 
		/* A.Stevens 2003: Given the math of non-intra frame
		   quantisation / inverse quantisation I always thought the
		   funny little foudning factor was bogus.  The reconstruction
           formula for non-intra frames includes a factor that is exactly
           right for minimising error if quantisation is performed by a simple
           *truncating* divide!
		*/

		y = (x<<4) /  dmquant; /* NOW: 16*x / d*mquant  
                                  OLD: round(16*x/d)/mquant) 
                                  = (32*x+d>>1)/(d*2)/mquant*/ ;
		if ( y > clipvalue )
		{
			if( saturated )
			{
				y = clipvalue;
			}
			else
			{
				int new_mquant = next_larger_quant( q_scale_type, mquant );
				if( new_mquant != mquant )
				{
					mquant = new_mquant;
					quant_mat = wsp->inter_q_tbl[mquant];
				}
				else
				{
					saturated = 1;
				}
				i=0;
				nzflag =0;
				goto restart;
			}
		}
		dst[i] = intsamesign(src[i], y) /* (src[i] >= 0 ? y : -y) */;
		flags |= dst[i];
	}
	nzflag = (nzflag<<1) | !!flags;

    *nonsat_mquant = mquant;
    return nzflag;
}

/* MPEG-1 inverse quantization */
void iquant_intra_m1(struct QuantizerWorkSpace *wsp,
                     int16_t *src, int16_t *dst, int dc_prec, int mquant)
{
  int i, val;
  uint16_t *quant_mat = wsp->intra_q_mat;

  dst[0] = src[0] << (3-dc_prec);
  for (i=1; i<64; i++)
  {
    val = (int)(src[i]*quant_mat[i]*mquant)/16;

    /* mismatch control */
    if ((val&1)==0 && val!=0)
      val+= (val>0) ? -1 : 1;

    /* saturation */
    dst[i] = (val>2047) ? 2047 : ((val<-2048) ? -2048 : val);
  }
}

/* MPEG-2 inverse quantization */
void iquant_intra_m2(struct QuantizerWorkSpace *wsp,
                     int16_t *src, int16_t *dst, int dc_prec, int mquant)
{
  int i, val, sum;

  sum = dst[0] = src[0] << (3-dc_prec);
  for (i=1; i<64; i++)
  {
      val = (int)(src[i]*wsp->intra_q_mat[i]*mquant)/16;
      sum+= dst[i] = (val>2047) ? 2047 : ((val<-2048) ? -2048 : val);
  }
  
  /* mismatch control */
  if ((sum&1)==0)
      dst[63]^= 1;
}


void iquant_non_intra_m1(struct QuantizerWorkSpace *wsp,
                         int16_t *src, int16_t *dst, int mquant )
{
    uint16_t *quant_mat = wsp->inter_q_tbl[mquant];
    int i, val;

#ifndef ORIGINAL_CODE

  for (i=0; i<64; i++)
  {
    val = src[i];
    if (val!=0)
    {
      val = (int)((2*val+(val>0 ? 1 : -1))*quant_mat[i])/32;

      /* mismatch control */
      if ((val&1)==0 && val!=0)
        val+= (val>0) ? -1 : 1;
    }

    /* saturation */
     dst[i] = (val>2047) ? 2047 : ((val<-2048) ? -2048 : val);
 }
#else
  
  for (i=0; i<64; i++)
  {
    val = abs(src[i]);
    if (val!=0)
    {
       val = ((val+val+1)*quant_mat[i]) >> 5;
     /* mismatch control */
     val -= (~(val&1))&(val!=0);
        val = fastmin(val, 2047); /* Saturation */
    }
   dst[i] = intsamesign(src[i],val);
  
  }
  
#endif

}

void iquant_non_intra_m2(struct QuantizerWorkSpace *wsp,
                         int16_t *src, int16_t *dst, int mquant )
{
  int i, val, sum;
  uint16_t *quant_mat;
  
  sum = 0;
#ifdef ORIGINAL_CODE
  
  for (i=0; i<64; i++)
  {
      val = src[i];
      if (val!=0)

			  val = (int)((2*val+(val>0 ? 1 : -1))*inter_q[i]*mquant)/32;
      sum+= dst[i] = (val>2047) ? 2047 : ((val<-2048) ? -2048 : val);
  }
#else
  quant_mat = wsp->inter_q_tbl[mquant];
  for (i=0; i<64; i++)
  {
      val = src[i];
      if( val != 0 )
      {
          val = abs(val);
          val = (int)((val+val+1)*quant_mat[i])>>5;
          val = intmin( val, 2047);
          sum += val;
      }
      dst[i] = intsamesign(src[i],val);
  }
#endif
  
  /* mismatch control */
  if ((sum&1)==0)
      dst[63]^= 1;

}

/*
  Initialise quantization routines.  Currently just setting up MMX
  routines if available.  

  TODO: The initialisation of the quantisation tables should move
  here...
*/

void init_quantizer( struct QuantizerCalls *calls, 
                     struct QuantizerWorkSpace **workspace,
                     int mpeg1, 
                     uint16_t intra_q[64], 
                     uint16_t inter_q[64])
{
    int q, i;
    struct QuantizerWorkSpace *wsp =
        bufalloc(sizeof(struct QuantizerWorkSpace));
    if( ((int)wsp)%16 != 0 )
    {
        printf( "BANG!");
        abort();
    }
    *workspace = wsp;
    for (i = 0; i < 64; i++)
    {
        wsp->intra_q_mat[i] = intra_q[i];
        wsp->inter_q_mat[i] = inter_q[i];
        wsp->i_intra_q_mat[i] = 
            (int)(((double)IQUANT_SCALE) / ((double)intra_q[i]));
        wsp->i_inter_q_mat[i] = 
            (int)(((double)IQUANT_SCALE) / ((double)inter_q[i]));
    }

    for (q = 1; q <= 112; ++q)
    {
        for (i = 0; i < 64; i++)
        {
            wsp->intra_q_tbl[q][i] = intra_q[i] * q;
            wsp->inter_q_tbl[q][i] = inter_q[i] * q;

            wsp->intra_q_tblf[q][i] = (float)wsp->intra_q_tbl[q][i];
            wsp->inter_q_tblf[q][i] = (float)wsp->inter_q_tbl[q][i];
            wsp->i_intra_q_tblf[q][i] = (float)(1.0 / (wsp->intra_q_tblf[q][i]));
            wsp->i_intra_q_tbl[q][i] = (IQUANT_SCALE/ wsp->intra_q_tbl[q][i]);
            wsp->r_intra_q_tbl[q][i] = (IQUANT_SCALE % wsp->intra_q_tbl[q][i]);
            
            wsp->i_inter_q_tblf[q][i] =  (float)(1.0 / (wsp->inter_q_tblf[q][i]));
            wsp->i_inter_q_tbl[q][i] = (IQUANT_SCALE/wsp->inter_q_tbl[q][i]);
            wsp->r_inter_q_tbl[q][i] = (IQUANT_SCALE % wsp->inter_q_tbl[q][i]);
        }
    }
    if( mpeg1 )
    {
        calls->piquant_intra = iquant_intra_m1;
        calls->piquant_non_intra = iquant_non_intra_m1;
    }
    else
    {
        calls->piquant_intra = iquant_intra_m2;
        calls->piquant_non_intra = iquant_non_intra_m2;
    }
    calls->pquant_non_intra = quant_non_intra;	  
    calls->pquant_weight_coeff_intra = quant_weight_coeff_intra;
    calls->pquant_weight_coeff_inter = quant_weight_coeff_inter;
    
#if defined(HAVE_ASM_MMX)
    if( cpu_accel() )
    {
        init_x86_quantization( calls, wsp, mpeg1 );
    }
#endif
#ifdef HAVE_ALTIVEC
	if (cpu_accel())
	    enable_altivec_quantization(calls, mpeg1);
#endif
}

void shutdown_quantizer(struct QuantizerWorkSpace *workspace)
{
    free(workspace);
}

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
