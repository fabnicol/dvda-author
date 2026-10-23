/* mpeg2coder.hh - MPEG2 packed bit / VLC syntax coding engine */

/*  (C) 2003 Andrew Stevens */

/*  This Software and modifications to existing software are free
 *  software; you can redistribute them and/or modify it under the terms
 *  of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
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


#include <config.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include "mpeg2syntaxcodes.h"
#include "tables.h"
#include "mpeg2coder.hh"
#include "elemstrmwriter.hh"
#include "mpeg2encoder.hh"
#include "picture.hh"

MPEG2CodingBuf::MPEG2CodingBuf( EncoderParams &_encparams, ElemStrmWriter &_writer ) :
    encparams( _encparams ),
	frag_buf( new ElemStrmFragBuf( _writer ) )
{
}

MPEG2CodingBuf::~MPEG2CodingBuf()
{
	delete frag_buf;
}

/* convert frame number to time_code
 *
 * drop_frame not implemented
 */
int MPEG2CodingBuf::FrameToTimeCode(int gop_timecode0_frame)
{
	int frame = gop_timecode0_frame;
	int fps, pict, sec, minute, hour, tc;

	/* Note: no drop_frame_flag support here, so we're simply rounding
	   the frame rate as per 6.3.8 13818-2
	*/
	fps = (int)(encparams.decode_frame_rate+0.5);
	pict = frame%fps;
	frame = (frame-pict)/fps;
	sec = frame%60;
	frame = (frame-sec)/60;
	minute = frame%60;
	frame = (frame-minute)/60;
	hour = frame%24;
	tc = (hour<<19) | (minute<<13) | (1<<12) | (sec<<6) | pict;
	return tc;
}


/****************
 *
 * generate sequence header (6.2.2.1, 6.3.3)
 *
 ***************/

void MPEG2CodingBuf::PutSeqHdr()
{
	int i;
    assert( frag_buf->Aligned() );
	frag_buf->PutBits(SEQ_START_CODE,32); /* sequence_header_code */
	frag_buf->PutBits(encparams.horizontal_size,12); /* horizontal_size_value */
	frag_buf->PutBits(encparams.vertical_size,12); /* vertical_size_value */
	frag_buf->PutBits(encparams.aspectratio,4); /* aspect_ratio_information */
	frag_buf->PutBits(encparams.frame_rate_code,4); /* frame_rate_code */

	/* MPEG-1 VBR is FFFF rate code. 
	   MPEG-2 VBR is a matter of mux-ing.  The ceiling bit_rate is always
	   sent 
	*/
	if(encparams.mpeg1 && (encparams.quant_floor != 0 || encparams.still_size > 0) ) {
		frag_buf->PutBits(0xfffff,18);
	} else {
		frag_buf->PutBits((int)ceil(encparams.bit_rate/400.0),18); /* bit_rate_value */
	}
	frag_buf->PutBits(1,1); /* marker_bit */
	frag_buf->PutBits(encparams.vbv_buffer_code,10); /* vbv_buffer_size_value */
	frag_buf->PutBits(encparams.constrparms,1); /* constrained_parameters_flag */

	frag_buf->PutBits(encparams.load_iquant,1); /* load_intra_quantizer_matrix */
	if (encparams.load_iquant)
		for (i=0; i<64; i++)  /* matrices are always downloaded in zig-zag order */
			frag_buf->PutBits(encparams.intra_q[zig_zag_scan[i]],8); /* intra_quantizer_matrix */

	frag_buf->PutBits(encparams.load_niquant,1); /* load_non_intra_quantizer_matrix */
	if (encparams.load_niquant)
		for (i=0; i<64; i++)
			frag_buf->PutBits(encparams.inter_q[zig_zag_scan[i]],8); /* non_intra_quantizer_matrix */
	if (!encparams.mpeg1)
	{
		PutSeqExt();
		PutSeqDispExt();
	}
	frag_buf->AlignBits();

}

/**************************
 *
 * generate sequence extension (6.2.2.3, 6.3.5) header (MPEG-2 only)
 *
 *************************/

void MPEG2CodingBuf::PutSeqExt()
{
	assert( frag_buf->Aligned() );
	frag_buf->PutBits(EXT_START_CODE,32); /* extension_start_code */
	frag_buf->PutBits(SEQ_ID,4); /* extension_start_code_identifier */
	frag_buf->PutBits((encparams.profile<<4)|encparams.level,8); /* profile_and_level_indication */
	frag_buf->PutBits(encparams.prog_seq,1); /* progressive sequence */
	frag_buf->PutBits(CHROMA420,2); /* chroma_format */
	frag_buf->PutBits(encparams.horizontal_size>>12,2); /* horizontal_size_extension */
	frag_buf->PutBits(encparams.vertical_size>>12,2); /* vertical_size_extension */
	frag_buf->PutBits(((int)ceil(encparams.bit_rate/400.0))>>18,12); /* bit_rate_extension */
	frag_buf->PutBits(1,1); /* marker_bit */
	frag_buf->PutBits(encparams.vbv_buffer_code>>10,8); /* vbv_buffer_size_extension */
	frag_buf->PutBits(0,1); /* low_delay  -- currently not implemented */
	frag_buf->PutBits(0,2); /* frame_rate_extension_n */
	frag_buf->PutBits(0,5); /* frame_rate_extension_d */
    frag_buf->AlignBits();
}

/*****************************
 * 
 * generate sequence display extension (6.2.2.4, 6.3.6)
 *
 ****************************/

void MPEG2CodingBuf::PutSeqDispExt()
{
	assert( frag_buf->Aligned() );
	frag_buf->PutBits(EXT_START_CODE,32); /* extension_start_code */
	frag_buf->PutBits(DISP_ID,4); /* extension_start_code_identifier */
	frag_buf->PutBits(encparams.video_format,3); /* video_format */
	frag_buf->PutBits(1,1); /* colour_description */
	frag_buf->PutBits(encparams.color_primaries,8); /* colour_primaries */
	frag_buf->PutBits(encparams.transfer_characteristics,8); /* transfer_characteristics */
	frag_buf->PutBits(encparams.matrix_coefficients,8); /* matrix_coefficients */
	frag_buf->PutBits(encparams.display_horizontal_size,14); /* display_horizontal_size */
	frag_buf->PutBits(1,1); /* marker_bit */
	frag_buf->PutBits(encparams.display_vertical_size,14); /* display_vertical_size */
    frag_buf->AlignBits();
}

/********************************
 *
 * Output user data (6.2.2.2.2, 6.3.4.1)
 *
 * TODO: string must not embed start codes 0x00 0x00 0x00 0xXX
 *
 *******************************/

void MPEG2CodingBuf::PutUserData(const uint8_t *userdata, int len)
{
	int i;
	assert( frag_buf->Aligned() );
	frag_buf->PutBits(USER_START_CODE,32); /* user_data_start_code */
	for( i =0; i < len; ++i )
		frag_buf->PutBits(userdata[i],8);
}

/* generate group of pictures header (6.2.2.6, 6.3.9)
 *
 * uses tc0 (timecode of first frame) and frame0 (number of first frame)
 */
void MPEG2CodingBuf::PutGopHdr(int frame,int closed_gop )
{
	int tc;

	frag_buf->AlignBits();
	frag_buf->PutBits(GOP_START_CODE,32); /* group_start_code */
	tc = FrameToTimeCode(frame);
	frag_buf->PutBits(tc,25); /* time_code */
	frag_buf->PutBits(closed_gop,1); /* closed_gop */
	frag_buf->PutBits(0,1); /* broken_link */
    frag_buf->AlignBits();
}


/* generate sequence_end_code (6.2.2) */
void MPEG2CodingBuf::PutSeqEnd(void)
{
	frag_buf->AlignBits();
	frag_buf->PutBits(SEQ_END_CODE,32);
}


/* generate variable length codes for an intra-coded block (6.2.6, 6.3.17) */

void MPEG2CodingBuf::PutIntraBlk(Picture *picture, int16_t *blk, int cc)
{
	int n, dct_diff, run, signed_level;

	/* DC coefficient (7.2.1) */
	dct_diff = blk[0] - picture->dc_dct_pred[cc]; /* difference to previous block */
	picture->dc_dct_pred[cc] = blk[0];

	if (cc==0)
		PutDClum(dct_diff);
	else
		PutDCchrom(dct_diff);

	/* AC coefficients (7.2.2) */
	run = 0;
	const uint8_t *scan_tbl = (picture->altscan ? alternate_scan : zig_zag_scan);
	for (n=1; n<64; n++)
	{
		/* use appropriate entropy scanning pattern */
		signed_level = blk[scan_tbl[n]];
		if (signed_level!=0)
		{
			PutAC(run,signed_level,picture->intravlc);
			run = 0;
		}
		else
			run++; /* count zero coefficients */
	}

	/* End of Block -- normative block punctuation */
	if (picture->intravlc)
		frag_buf->PutBits(6,4); /* 0110 (Table B-15) */
	else
		frag_buf->PutBits(2,2); /* 10 (Table B-14) */
}

/* generate variable length codes for a non-intra-coded block (6.2.6, 6.3.17) */
void  MPEG2CodingBuf::PutNonIntraBlk(Picture *picture, int16_t *blk)
{
	int n, run, signed_level, first;

	run = 0;
	first = 1;

	for (n=0; n<64; n++)
	{
		/* use appropriate entropy scanning pattern */
		signed_level = blk[(picture->altscan ? alternate_scan : zig_zag_scan)[n]];

		if (signed_level!=0)
		{
			if (first)
			{
				/* first coefficient in non-intra block */
				PutACfirst(run,signed_level);
				first = 0;
			}
			else
				PutAC(run,signed_level,0);

			run = 0;
		}
		else
			run++; /* count zero coefficients */
	}

	/* End of Block -- normative block punctuation  */
	frag_buf->PutBits(2,2);
}

/* generate variable length code for a motion vector component (7.6.3.1) */
void  MPEG2CodingBuf::PutMV(int dmv, int f_code)
{
  int r_size, f, vmin, vmax, dv, temp, motion_code, motion_residual;

  r_size = f_code - 1; /* number of fixed length code ('residual') bits */
  f = 1<<r_size;
  vmin = -16*f; /* lower range limit */
  vmax = 16*f - 1; /* upper range limit */
  dv = 32*f;

  /* fold vector difference into [vmin...vmax] */
  if (dmv>vmax)
    dmv-= dv;
  else if (dmv<vmin)
    dmv+= dv;

  /* check value */
  if (dmv<vmin || dmv>vmax)
  {
      fprintf(stderr,"Too large MV %03d not in [%04d..:%03d]\n", dmv, vmin, vmax);
      exit(1);
  }

  /* split dmv into motion_code and motion_residual */
  temp = ((dmv<0) ? -dmv : dmv) + f - 1;
  motion_code = temp>>r_size;
  if (dmv<0)
    motion_code = -motion_code;
  motion_residual = temp & (f-1);

  PutMotionCode(motion_code); /* variable length code */

  if (r_size!=0 && motion_code!=0)
    frag_buf->PutBits(motion_residual,r_size); /* fixed length code */
}


/* generate variable length code for DC coefficient (7.2.1) */
void MPEG2CodingBuf::PutDC(const sVLCtable *tab, int val)
{
	int absval, size;

	absval = abs(val);
    assert(absval<=encparams.dctsatlim);

	/* compute dct_dc_size */
	size = 0;

	while (absval)
	{
		absval >>= 1;
		size++;
	}

	/* generate VLC for dct_dc_size (Table B-12 or B-13) */
	frag_buf->PutBits(tab[size].code,tab[size].len);

	/* append fixed length code (dc_dct_differential) */
	if (size!=0)
	{
		if (val>=0)
			absval = val;
		else
			absval = val + (1<<size) - 1; /* val + (2 ^ size) - 1 */
		frag_buf->PutBits(absval,size);
	}
}

/* generate variable length code for DC coefficient (7.2.1) */
int MPEG2CodingBuf::DC_bits(const sVLCtable *tab, int val)
{
	int absval, size;

	absval = abs(val);

	/* compute dct_dc_size */
	size = 0;

	while (absval)
	{
		absval >>= 1;
		size++;
	}

	/* generate VLC for dct_dc_size (Table B-12 or B-13) */
	return tab[size].len+size;
}


/* generate variable length code for first coefficient
 * of a non-intra block (7.2.2.2) */
void MPEG2CodingBuf::PutACfirst(int run, int val)
{
	if (run==0 && (val==1 || val==-1)) /* these are treated differently */
		frag_buf->PutBits(2|(val<0),2); /* generate '1s' (s=sign), (Table B-14, line 2) */
	else
		PutAC(run,val,0); /* no difference for all others */
}

/* generate variable length code for other DCT coefficients (7.2.2) */
void MPEG2CodingBuf::PutAC(int run, int signed_level, int vlcformat)
{
	int level, len;
	const VLCtable *ptab = NULL;

	level = abs(signed_level);

	/* make sure run and level are valid */
	if (run<0 || run>63 || level==0 || level>encparams.dctsatlim)
	{
		assert( signed_level == -(encparams.dctsatlim+1)); 	/* Negative range is actually 1 more */
	}

	len = 0;

	if (run<2 && level<41)
	{
		/* vlcformat selects either of Table B-14 / B-15 */
		if (vlcformat)
			ptab = &dct_code_tab1a[run][level-1];
		else
			ptab = &dct_code_tab1[run][level-1];

		len = ptab->len;
	}
	else if (run<32 && level<6)
	{
		/* vlcformat selects either of Table B-14 / B-15 */
		if (vlcformat)
			ptab = &dct_code_tab2a[run-2][level-1];
		else
			ptab = &dct_code_tab2[run-2][level-1];

		len = ptab->len;
	}

	if (len!=0) /* a VLC code exists */
	{
		frag_buf->PutBits(ptab->code,len);
		frag_buf->PutBits(signed_level<0,1); /* sign */
	}
	else
	{
		/* no VLC for this (run, level) combination: use escape coding (7.2.2.3) */
		frag_buf->PutBits(1l,6); /* Escape */
		frag_buf->PutBits(run,6); /* 6 bit code for run */
		if (encparams.mpeg1)
		{
			/* ISO/IEC 11172-2 uses a 8 or 16 bit code */
			if (signed_level>127)
				frag_buf->PutBits(0,8);
			if (signed_level<-127)
				frag_buf->PutBits(128,8);
			frag_buf->PutBits(signed_level,8);
		}
		else
		{
			/* ISO/IEC 13818-2 uses a 12 bit code, Table B-16 */
			frag_buf->PutBits(signed_level,12);
		}
	}
}

/* generate variable length code for other DCT coefficients (7.2.2) */
int MPEG2CodingBuf::AC_bits(int run, int signed_level, int vlcformat)
{
	int level;
	const VLCtable *ptab;

	level = abs(signed_level);

	if (run<2 && level<41)
	{
		/* vlcformat selects either of Table B-14 / B-15 */
		if (vlcformat)
			ptab = &dct_code_tab1a[run][level-1];
		else
			ptab = &dct_code_tab1[run][level-1];

		return ptab->len+1;
	}
	else if (run<32 && level<6)
	{
		/* vlcformat selects either of Table B-14 / B-15 */
		if (vlcformat)
			ptab = &dct_code_tab2a[run-2][level-1];
		else
			ptab = &dct_code_tab2[run-2][level-1];

		return ptab->len+1;
	}
	else
	{
		return 12+12;
	}
}

/* generate variable length code for macroblock_address_increment (6.3.16) */
void MPEG2CodingBuf::PutAddrInc(int addrinc)
{
	while (addrinc>33)
	{
		frag_buf->PutBits(0x08,11); /* macroblock_escape */
		addrinc-= 33;
	}
	assert( addrinc >= 1 && addrinc <= 33 );
	frag_buf->PutBits(addrinctab[addrinc-1].code,addrinctab[addrinc-1].len);
}

int MPEG2CodingBuf::AddrInc_bits(int addrinc)
{
	int bits = 0;
	while (addrinc>33)
	{
		bits += 11;
		addrinc-= 33;
	}
	return bits + addrinctab[addrinc-1].len;
}

/* generate variable length code for macroblock_type (6.3.16.1) */
void MPEG2CodingBuf::PutMBType(int pict_type, int mb_type)
{
	frag_buf->PutBits(mbtypetab[pict_type-1][mb_type].code,
				   mbtypetab[pict_type-1][mb_type].len);
}

int MPEG2CodingBuf::MBType_bits( int pict_type, int mb_type)
{
	return mbtypetab[pict_type-1][mb_type].len;
}

/* generate variable length code for motion_code (6.3.16.3) */
void MPEG2CodingBuf::PutMotionCode(int motion_code)
{
	int abscode;

	abscode = abs( motion_code );
	frag_buf->PutBits(motionvectab[abscode].code,motionvectab[abscode].len);
	if (motion_code!=0)
		frag_buf->PutBits(motion_code<0,1); /* sign, 0=positive, 1=negative */
}

int MPEG2CodingBuf::MotionCode_bits( int motion_code )
{
	int abscode = (motion_code>=0) ? motion_code : -motion_code; 
	return 1+motionvectab[abscode].len;
}

/* generate variable length code for dmvector[t] (6.3.16.3), Table B-11 */
void MPEG2CodingBuf::PutDMV(int dmv)
{
	if (dmv==0)
		frag_buf->PutBits(0,1);
	else if (dmv>0)
		frag_buf->PutBits(2,2);
	else
		frag_buf->PutBits(3,2);
}

int MPEG2CodingBuf::DMV_bits(int dmv)
{
	return dmv == 0 ? 1 : 2;
}

/* generate variable length code for coded_block_pattern (6.3.16.4)
 *
 * 4:2:2, 4:4:4 not implemented
 */
void MPEG2CodingBuf::PutCPB(int cbp)
{
	frag_buf->PutBits(cbptable[cbp].code,cbptable[cbp].len);
}

int MPEG2CodingBuf::CBP_bits(int cbp)
{
	return cbptable[cbp].len;
}


const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::addrinctab[33]=
{
  {0x01,1},  {0x03,3},  {0x02,3},  {0x03,4},
  {0x02,4},  {0x03,5},  {0x02,5},  {0x07,7},
  {0x06,7},  {0x0b,8},  {0x0a,8},  {0x09,8},
  {0x08,8},  {0x07,8},  {0x06,8},  {0x17,10},
  {0x16,10}, {0x15,10}, {0x14,10}, {0x13,10},
  {0x12,10}, {0x23,11}, {0x22,11}, {0x21,11},
  {0x20,11}, {0x1f,11}, {0x1e,11}, {0x1d,11},
  {0x1c,11}, {0x1b,11}, {0x1a,11}, {0x19,11},
  {0x18,11}
};


/* Table B-2, B-3, B-4 variable length codes for macroblock_type
 *
 * indexed by [macroblock_type]
 */

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::mbtypetab[3][32]=
{
 /* I */
 {
  {0,0}, {1,1}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},
  {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},
  {0,0}, {1,2}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},
  {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}
 },
 /* P */
 {
  {0,0}, {3,5}, {1,2}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},
  {1,3}, {0,0}, {1,1}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},
  {0,0}, {1,6}, {1,5}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0},
  {0,0}, {0,0}, {2,5}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}
 },
 /* B */
 {
  {0,0}, {3,5}, {0,0}, {0,0}, {2,3}, {0,0}, {3,3}, {0,0},
  {2,4}, {0,0}, {3,4}, {0,0}, {2,2}, {0,0}, {3,2}, {0,0},
  {0,0}, {1,6}, {0,0}, {0,0}, {0,0}, {0,0}, {2,6}, {0,0},
  {0,0}, {0,0}, {3,6}, {0,0}, {0,0}, {0,0}, {2,5}, {0,0}
 }
};


/* Table B-5 ... B-8 variable length codes for macroblock_type in
 *  scalable sequences
 *
 * not implemented
 */

/* Table B-9, variable length codes for coded_block_pattern
 *
 * indexed by [coded_block_pattern]
 */

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::cbptable[64]=
{
  {0x01,9}, {0x0b,5}, {0x09,5}, {0x0d,6}, 
  {0x0d,4}, {0x17,7}, {0x13,7}, {0x1f,8}, 
  {0x0c,4}, {0x16,7}, {0x12,7}, {0x1e,8}, 
  {0x13,5}, {0x1b,8}, {0x17,8}, {0x13,8}, 
  {0x0b,4}, {0x15,7}, {0x11,7}, {0x1d,8}, 
  {0x11,5}, {0x19,8}, {0x15,8}, {0x11,8}, 
  {0x0f,6}, {0x0f,8}, {0x0d,8}, {0x03,9}, 
  {0x0f,5}, {0x0b,8}, {0x07,8}, {0x07,9}, 
  {0x0a,4}, {0x14,7}, {0x10,7}, {0x1c,8}, 
  {0x0e,6}, {0x0e,8}, {0x0c,8}, {0x02,9}, 
  {0x10,5}, {0x18,8}, {0x14,8}, {0x10,8}, 
  {0x0e,5}, {0x0a,8}, {0x06,8}, {0x06,9}, 
  {0x12,5}, {0x1a,8}, {0x16,8}, {0x12,8}, 
  {0x0d,5}, {0x09,8}, {0x05,8}, {0x05,9}, 
  {0x0c,5}, {0x08,8}, {0x04,8}, {0x04,9},
  {0x07,3}, {0x0a,5}, {0x08,5}, {0x0c,6}
};


/* Table B-10, variable length codes for motion_code
 *
 * indexed by [abs(motion_code)]
 * sign of motion_code is treated elsewhere
 */

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::motionvectab[17]=
{
  {0x01,1},  {0x01,2},  {0x01,3},  {0x01,4},
  {0x03,6},  {0x05,7},  {0x04,7},  {0x03,7},
  {0x0b,9},  {0x0a,9},  {0x09,9},  {0x11,10},
  {0x10,10}, {0x0f,10}, {0x0e,10}, {0x0d,10},
  {0x0c,10}
};


/* Table B-11, variable length codes for dmvector
 *
 * treated elsewhere
 */

/* Table B-12, variable length codes for dct_dc_size_luminance
 *
 * indexed by [dct_dc_size_luminance]
 */

const MPEG2CodingBuf::sVLCtable 
MPEG2CodingBuf::DClumtab[12]=
{
  {0x0004,3}, {0x0000,2}, {0x0001,2}, {0x0005,3}, {0x0006,3}, {0x000e,4},
  {0x001e,5}, {0x003e,6}, {0x007e,7}, {0x00fe,8}, {0x01fe,9}, {0x01ff,9}
};


/* Table B-13, variable length codes for dct_dc_size_chrominance
 *
 * indexed by [dct_dc_size_chrominance]
 */

const MPEG2CodingBuf::sVLCtable 
MPEG2CodingBuf::DCchromtab[12]=
{
  {0x0000,2}, {0x0001,2}, {0x0002,2}, {0x0006,3}, {0x000e,4}, {0x001e,5},
  {0x003e,6}, {0x007e,7}, {0x00fe,8}, {0x01fe,9}, {0x03fe,10},{0x03ff,10}
};


/* Table B-14, DCT coefficients table zero
 *
 * indexed by [run][level-1]
 * split into two tables (dct_code_tab1, dct_code_tab2) to reduce size
 * 'first DCT coefficient' condition and 'End of Block' are treated elsewhere
 * codes do not include s (sign bit)
 */

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::dct_code_tab1[2][40]=
{
 /* run = 0, level = 1...40 */
 {
  {0x03, 2}, {0x04, 4}, {0x05, 5}, {0x06, 7},
  {0x26, 8}, {0x21, 8}, {0x0a,10}, {0x1d,12},
  {0x18,12}, {0x13,12}, {0x10,12}, {0x1a,13},
  {0x19,13}, {0x18,13}, {0x17,13}, {0x1f,14},
  {0x1e,14}, {0x1d,14}, {0x1c,14}, {0x1b,14},
  {0x1a,14}, {0x19,14}, {0x18,14}, {0x17,14},
  {0x16,14}, {0x15,14}, {0x14,14}, {0x13,14},
  {0x12,14}, {0x11,14}, {0x10,14}, {0x18,15},
  {0x17,15}, {0x16,15}, {0x15,15}, {0x14,15},
  {0x13,15}, {0x12,15}, {0x11,15}, {0x10,15}
 },
 /* run = 1, level = 1...18 */
 {
  {0x03, 3}, {0x06, 6}, {0x25, 8}, {0x0c,10},
  {0x1b,12}, {0x16,13}, {0x15,13}, {0x1f,15},
  {0x1e,15}, {0x1d,15}, {0x1c,15}, {0x1b,15},
  {0x1a,15}, {0x19,15}, {0x13,16}, {0x12,16},
  {0x11,16}, {0x10,16}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}
 }
};

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::dct_code_tab2[30][5]=
{
  /* run = 2...31, level = 1...5 */
  {{0x05, 4}, {0x04, 7}, {0x0b,10}, {0x14,12}, {0x14,13}},
  {{0x07, 5}, {0x24, 8}, {0x1c,12}, {0x13,13}, {0x00, 0}},
  {{0x06, 5}, {0x0f,10}, {0x12,12}, {0x00, 0}, {0x00, 0}},
  {{0x07, 6}, {0x09,10}, {0x12,13}, {0x00, 0}, {0x00, 0}},
  {{0x05, 6}, {0x1e,12}, {0x14,16}, {0x00, 0}, {0x00, 0}},
  {{0x04, 6}, {0x15,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x07, 7}, {0x11,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x05, 7}, {0x11,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x27, 8}, {0x10,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x23, 8}, {0x1a,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x22, 8}, {0x19,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x20, 8}, {0x18,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x0e,10}, {0x17,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x0d,10}, {0x16,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x08,10}, {0x15,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1f,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1a,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x19,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x17,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x16,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1f,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1e,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1d,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1c,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1b,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1f,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1e,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1d,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1c,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1b,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}}
};


/* Table B-15, DCT coefficients table one
 *
 * indexed by [run][level-1]
 * split into two tables (dct_code_tab1a, dct_code_tab2a) to reduce size
 * 'End of Block' is treated elsewhere
 * codes do not include s (sign bit)
 */

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::dct_code_tab1a[2][40]=
{
 /* run = 0, level = 1...40 */
 {
  {0x02, 2}, {0x06, 3}, {0x07, 4}, {0x1c, 5},
  {0x1d, 5}, {0x05, 6}, {0x04, 6}, {0x7b, 7},
  {0x7c, 7}, {0x23, 8}, {0x22, 8}, {0xfa, 8},
  {0xfb, 8}, {0xfe, 8}, {0xff, 8}, {0x1f,14},
  {0x1e,14}, {0x1d,14}, {0x1c,14}, {0x1b,14},
  {0x1a,14}, {0x19,14}, {0x18,14}, {0x17,14},
  {0x16,14}, {0x15,14}, {0x14,14}, {0x13,14},
  {0x12,14}, {0x11,14}, {0x10,14}, {0x18,15},
  {0x17,15}, {0x16,15}, {0x15,15}, {0x14,15},
  {0x13,15}, {0x12,15}, {0x11,15}, {0x10,15}
 },
 /* run = 1, level = 1...18 */
 {
  {0x02, 3}, {0x06, 5}, {0x79, 7}, {0x27, 8},
  {0x20, 8}, {0x16,13}, {0x15,13}, {0x1f,15},
  {0x1e,15}, {0x1d,15}, {0x1c,15}, {0x1b,15},
  {0x1a,15}, {0x19,15}, {0x13,16}, {0x12,16},
  {0x11,16}, {0x10,16}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0},
  {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}
 }
};

const MPEG2CodingBuf::VLCtable 
MPEG2CodingBuf::dct_code_tab2a[30][5]=
{
  /* run = 2...31, level = 1...5 */
  {{0x05, 5}, {0x07, 7}, {0xfc, 8}, {0x0c,10}, {0x14,13}},
  {{0x07, 5}, {0x26, 8}, {0x1c,12}, {0x13,13}, {0x00, 0}},
  {{0x06, 6}, {0xfd, 8}, {0x12,12}, {0x00, 0}, {0x00, 0}},
  {{0x07, 6}, {0x04, 9}, {0x12,13}, {0x00, 0}, {0x00, 0}},
  {{0x06, 7}, {0x1e,12}, {0x14,16}, {0x00, 0}, {0x00, 0}},
  {{0x04, 7}, {0x15,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x05, 7}, {0x11,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x78, 7}, {0x11,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x7a, 7}, {0x10,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x21, 8}, {0x1a,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x25, 8}, {0x19,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x24, 8}, {0x18,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x05, 9}, {0x17,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x07, 9}, {0x16,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x0d,10}, {0x15,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1f,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1a,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x19,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x17,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x16,12}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1f,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1e,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1d,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1c,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1b,13}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1f,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1e,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1d,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1c,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}},
  {{0x1b,16}, {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}}
};



/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */



