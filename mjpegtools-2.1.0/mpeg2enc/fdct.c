/* fdct.c, forward discrete cosine transform, double precision           */


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
/* Modifications and enhancements (C) 2003 Andrew Stevens */

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


#include <config.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "mjpeg_types.h"

static double aanscales[64];
static float  aanscalesf[64];
#define FDCTTEST

#define NC_COS6      0.382683432365089771728459984030399//cos(6*pi/16)

#define NC_R_SQRT2   0.707106781186547524400844362104849// 1/sqrt(2)

#define NC_COS0SQRT2 1.0                                //cos(0*pi/16)*sqrt(2)
#define NC_COS1SQRT2 1.38703984532214746182161919156644 //cos(1*pi/16)*sqrt(2)
#define NC_COS2SQRT2 1.30656296487637652785664317342719 //cos(2*pi/16)*sqrt(2)
#define NC_COS3SQRT2 1.17587560241935871697446710461126 //cos(3*pi/16)*sqrt(2)
#define NC_COS4SQRT2 1.0                                //cos(4*pi/16)*sqrt(2)
#define NC_COS5SQRT2 0.785694958387102181277897367657217//cos(5*pi/16)*sqrt(2)
#define NC_COS6SQRT2 0.541196100146196984399723205366389//cos(6*pi/16)*sqrt(2)
#define NC_COS7SQRT2 0.275899379282943012335957563669373//cos(7*pi/16)*sqrt(2)

void init_fdct_daan( void );
void fdct_daan(int16_t *block);
void fdct_daanf(int16_t *block);
void init_fdct (void);
void fdct (int16_t *block);


#ifdef FDCTTEST

void init_fdct_daan( void )
{
	int i, j;
	static const double aansf[8] = {
		1.0,           // sqrt(2) factor left out here...
		NC_COS1SQRT2,
		NC_COS2SQRT2,
		NC_COS3SQRT2,
		NC_COS4SQRT2,  // cos(4*pi/16) * sqrt(2) = 1.0 exactly
		NC_COS5SQRT2,
		NC_COS6SQRT2,
		NC_COS7SQRT2
	};

	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
			aanscalesf[ i*8 + j] =
			aanscales[i*8 + j] = 1.0 / (aansf[i] * aansf[j] * 8.0);
}

/*
 * Perform a floating point forward DCT on one block of samples.
 */

void fdct_daan(int16_t *block)
{
	double tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	double tmp10, tmp11, tmp12, tmp13;
	double z1, z2, z3, z4, z5, z11, z13;
	double *dataptr;
	double data[64];
	int16_t *blkptr;
	int i;

	/* Pass 1: process rows. */

	blkptr = block;
	dataptr = data;
	for (i = 0; i < 8; i++)
	{
		tmp0 = blkptr[0] + blkptr[7];
		tmp7 = blkptr[0] - blkptr[7];
		tmp1 = blkptr[1] + blkptr[6];
		tmp6 = blkptr[1] - blkptr[6];
		tmp2 = blkptr[2] + blkptr[5];
		tmp5 = blkptr[2] - blkptr[5];
		tmp3 = blkptr[3] + blkptr[4];
		tmp4 = blkptr[3] - blkptr[4];

		/* Even part */

		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = tmp10 + tmp11; /* phase 3 */
		dataptr[4] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * ((double) NC_R_SQRT2); /* c4 */
		dataptr[2] = tmp13 + z1;	/* phase 5 */
		dataptr[6] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * ((double) NC_COS6); /* c6 */
		z2 = ((double) NC_COS6SQRT2) * tmp10 + z5; /* c2-c6 */
		z4 = ((double) NC_COS2SQRT2) * tmp12 + z5; /* c2+c6 */
		z3 = tmp11 * ((double) NC_R_SQRT2); /* c4 */

		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;

		dataptr[5] = z13 + z2;	/* phase 6 */
		dataptr[3] = z13 - z2;
		dataptr[1] = z11 + z4;
		dataptr[7] = z11 - z4;

		dataptr += 8;		/* advance pointer to next row */
		blkptr += 8;
	}

	/* Pass 2: process columns. */

	dataptr = data;
	for (i = 0; i < 8; i++)
	{
		tmp0 = dataptr[0] + dataptr[56];
		tmp7 = dataptr[0] - dataptr[56];
		tmp1 = dataptr[8] + dataptr[48];
		tmp6 = dataptr[8] - dataptr[48];
		tmp2 = dataptr[16] + dataptr[40];
		tmp5 = dataptr[16] - dataptr[40];
		tmp3 = dataptr[24] + dataptr[32];
		tmp4 = dataptr[24] - dataptr[32];

		/* Even part */

		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = tmp10 + tmp11; /* phase 3 */
		dataptr[32] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * ((double) NC_R_SQRT2); /* c4 */
		dataptr[16] = tmp13 + z1; /* phase 5 */
		dataptr[48] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * ((double) NC_COS6); /* c6 */
		z2 = ((double) NC_COS6SQRT2) * tmp10 + z5; /* c2-c6 */
		z4 = ((double) NC_COS2SQRT2) * tmp12 + z5; /* c2+c6 */
		z3 = tmp11 * ((double) NC_R_SQRT2); /* c4 */

		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;

		dataptr[40] = z13 + z2; /* phase 6 */
		dataptr[24] = z13 - z2;
		dataptr[8] = z11 + z4;
		dataptr[56] = z11 - z4;

		dataptr++;			/* advance pointer to next column */
	}
	/* descale */
	for (i = 0; i < 64; i++)
		//block[i] = (int16_t) floor(data[i] * aanscales[i] + 0.499999);
		block[i] = (int16_t) floor(data[i] * aanscales[i] + 0.5);
}


/*
 * Perform a floating point forward DCT on one block of samples.
 */

void fdct_daanf(int16_t *block)
{
	float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	float tmp10, tmp11, tmp12, tmp13;
	float z1, z2, z3, z4, z5, z11, z13;
	float *dataptr;
	float data[64];
	int16_t *blkptr;
	int i;

	/* Pass 1: process rows. */

	blkptr = block;
	dataptr = data;
	for (i = 0; i < 8; i++)
	{
		tmp0 = blkptr[0] + blkptr[7];
		tmp7 = blkptr[0] - blkptr[7];
		tmp1 = blkptr[1] + blkptr[6];
		tmp6 = blkptr[1] - blkptr[6];
		tmp2 = blkptr[2] + blkptr[5];
		tmp5 = blkptr[2] - blkptr[5];
		tmp3 = blkptr[3] + blkptr[4];
		tmp4 = blkptr[3] - blkptr[4];

		/* Even part */

		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = tmp10 + tmp11; /* phase 3 */
		dataptr[4] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * ((double) NC_R_SQRT2); /* c4 */
		dataptr[2] = tmp13 + z1;	/* phase 5 */
		dataptr[6] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * ((float) NC_COS6); /* c6 */
		z2 = ((float) NC_COS6SQRT2) * tmp10 + z5; /* c2-c6 */
		z4 = ((float) NC_COS2SQRT2) * tmp12 + z5; /* c2+c6 */
		z3 = tmp11 * ((float) NC_R_SQRT2); /* c4 */

		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;

		dataptr[5] = z13 + z2;	/* phase 6 */
		dataptr[3] = z13 - z2;
		dataptr[1] = z11 + z4;
		dataptr[7] = z11 - z4;

		dataptr += 8;		/* advance pointer to next row */
		blkptr += 8;
	}

	/* Pass 2: process columns. */

	dataptr = data;
	for (i = 0; i < 8; i++)
	{
		tmp0 = dataptr[0] + dataptr[56];
		tmp7 = dataptr[0] - dataptr[56];
		tmp1 = dataptr[8] + dataptr[48];
		tmp6 = dataptr[8] - dataptr[48];
		tmp2 = dataptr[16] + dataptr[40];
		tmp5 = dataptr[16] - dataptr[40];
		tmp3 = dataptr[24] + dataptr[32];
		tmp4 = dataptr[24] - dataptr[32];

		/* Even part */

		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = tmp10 + tmp11; /* phase 3 */
		dataptr[32] = tmp10 - tmp11;

		z1 = (tmp12 + tmp13) * ((float) NC_R_SQRT2); /* c4 */
		dataptr[16] = tmp13 + z1; /* phase 5 */
		dataptr[48] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * ((float) NC_COS6); /* c6 */
		z2 = ((float) NC_COS6SQRT2) * tmp10 + z5; /* c2-c6 */
		z4 = ((float) NC_COS2SQRT2) * tmp12 + z5; /* c2+c6 */
		z3 = tmp11 * ((float) NC_R_SQRT2); /* c4 */

		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;

		dataptr[40] = z13 + z2; /* phase 6 */
		dataptr[24] = z13 - z2;
		dataptr[8] = z11 + z4;
		dataptr[56] = z11 - z4;

		dataptr++;			/* advance pointer to next column */
	}
	/* descale */
	for (i = 0; i < 64; i++)
		//block[i] = (int16_t) floor(data[i] * aanscales[i] + 0.499999);
		block[i] = (int16_t) floor(data[i] * aanscalesf[i] + 0.5f);
}

struct dct_test {
    int bounds,maxerr,iter;
    int me[64],mse[64];
};

extern void dct_test_and_print(struct dct_test *dt,int range,int16_t *origblock,int16_t *block);

static struct dct_test fdct_res;

/* reference fdct taken from "ieeetest.c"
 * Written by Tom Lane (tgl@cs.cmu.edu).
 * Released to public domain 11/22/93.
 */

/* The cosine lookup table */
/* coslu[a][b] = C(b)/2 * cos[(2a+1)b*pi/16] */
static double coslu[8][8];

void init_fdct_ref(void)
{
    int a,b;
    double tmp;

    for(a=0;a<8;a++)
        for(b=0;b<8;b++) {
            tmp = cos((double)((a+a+1)*b) * (3.14159265358979323846 / 16.0));
            if(b==0)
                tmp /= sqrt(2.0);
            coslu[a][b] = tmp * 0.5;
        }
}

void fdct_ref (int16_t *block)
{
    int x,y,u,v;
    double tmp, tmp2;
    double res[8][8];

    for (v=0; v<8; v++) {
        for (u=0; u<8; u++) {
            tmp = 0.0;
            for (y=0; y<8; y++) {
                tmp2 = 0.0;
                for (x=0; x<8; x++) {
                    tmp2 += (double) block[y*8+x] * coslu[x][u];
                }
                tmp += coslu[y][v] * tmp2;
            }
            res[v][u] = tmp;
        }
    }

    for (v=0; v<8; v++) {
        for (u=0; u<8; u++) {
            tmp = res[v][u];
            if (tmp < 0.0) {
                x = - ((int) (0.5 - tmp));
            } else {
                x = (int) (tmp + 0.5);
            }
            block[v*8+u] = (int16_t) x;
        }
    }
}

void fdct_test(int16_t *block)
{
    int16_t origblock[64];

    memcpy(origblock,block,64*sizeof(int16_t));

    fdct_ref(origblock);
    // fdct_daan(origblock);
    // fdct_sse(origblock);
    // fdct(origblock);

    fdct(block);
    // fdct_daan(block);
    // fdct_mmx(block);
    // fdct_mmx(block);

    dct_test_and_print(&fdct_res,2048,origblock,block);
}


#endif

#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI 3.14159265358979323846
# endif
#endif

/* private data */
static int c[8][8]; /* transform coefficients */


/*!
 * Warning this DCT is actually too imprecise... it really
 * needs 11-bit precise coefficiencts (FDCT_COS_BITS=11)
 * to avoid nasty mismatch errors at low q.  Unfortunately
 * it would then need > 32 bits.   It is also suspect
 * that rounding is towards +infinity rather than away from 0.
 */


#define FDCT_COS_BITS 9
#define FDCT_COS_MUL (1<<FDCT_COS_BITS)
#define FDCT_RES_SHIFT (2*FDCT_COS_BITS)
#define FDCT_RES_ROUND (1<<(FDCT_RES_SHIFT-1))

void init_fdct(void)
{
	int i, j;
	double s;

	for (i=0; i<8; i++)
	{
		s = (i==0) ? sqrt(0.125) : 0.5;
		for (j=0; j<8; j++)
			c[i][j] = (s * cos((PI/8.0)*i*(j+0.5))*FDCT_COS_MUL + 0.5);
	}
#ifdef FDCTTEST
    init_fdct_daan();
    init_fdct_ref();
    memset(&fdct_res,0,sizeof(fdct_res));
#endif
}


void fdct(int16_t *block)
{

	int i, j;
	int tmp[64];
	for (i=0; i<8; i++)
		for (j=0; j<8; j++)
		{
			int s =
				  c[j][0] * block[8*i+0]
				+ c[j][1] * block[8*i+1]
				+ c[j][2] * block[8*i+2]
				+ c[j][3] * block[8*i+3]
				+ c[j][4] * block[8*i+4]
				+ c[j][5] * block[8*i+5]
				+ c[j][6] * block[8*i+6]
				+ c[j][7] * block[8*i+7];

			tmp[8*i+j] = s;
		}

	for (j=0; j<8; j++)
		for (i=0; i<8; i++)
		{
			int s =
				  c[i][0] * tmp[8*0+j]
				+ c[i][1] * tmp[8*1+j]
				+ c[i][2] * tmp[8*2+j]
				+ c[i][3] * tmp[8*3+j]
				+ c[i][4] * tmp[8*4+j]
				+ c[i][5] * tmp[8*5+j]
				+ c[i][6] * tmp[8*6+j]
				+ c[i][7] * tmp[8*7+j];

			block[8*i+j] = (s+FDCT_RES_ROUND)>>FDCT_RES_SHIFT;
		}

}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
