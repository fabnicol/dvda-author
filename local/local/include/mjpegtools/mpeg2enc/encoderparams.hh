#ifndef _ENCODERPARAMS_HH
#define _ENCODERPARAMS_HH

/* encoderparams - class holding all the various control parameters for 
   and individual encoder instance.  For speed a lot of address offsets/sizes
   are computed once-and-for-all and held in this object.
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

/* Modifications and enhancements (C) 2000,2001,2002,2003 Andrew Stevens */

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


#include "mjpeg_types.h"
#include "syntaxconsts.h"

#include <deque>

using std::deque;


/*
  Ensure we don't bury a system in wild spawning of
  threads...
 */

#define MAX_WORKER_THREADS 16



/* motion data */
struct motion_data {
	unsigned int forw_hor_f_code,forw_vert_f_code; /* vector range */
	unsigned int sxf,syf; /* search range */
	unsigned int back_hor_f_code,back_vert_f_code;
	unsigned int sxb,syb;
};


struct RateCtl;
class MPEG2EncOptions;

class EncoderParams
{
public:
	EncoderParams( const MPEG2EncOptions &options);
	void Init(const MPEG2EncOptions &options);

private:
	void InitQuantMatrices(const class MPEG2EncOptions &options);
	void InitEncodingControls(const class MPEG2EncOptions &options);
	void RangeChecks();
	void ProfileAndLevelChecks();
public:

	/**************
	 *
	 * Global MPEG parameters (set by supplied stream, never
	 * change in a run) 
	 *
     *************/

	unsigned int horizontal_size, vertical_size;
	
	unsigned int aspectratio;	/* aspect ratio information (pel or display) */
	unsigned int frame_rate_code;	/* coded value of playback display
									 * frame rate */
	int dctsatlim;			/* Value to saturated DCT coeffs to */
	double frame_rate;		/* Playback display frames per second
							   N.b. when 3:2 pullback is active
							   this is higher than the frame
							   decode rate.  */
	double bit_rate;			/* bits per second, rate decode buffers filled (max rate) */
	double target_bitrate;		/* Target bit rate to achieve overall for VBR 0  if no target set */
	unsigned int stream_frames;		/* # Frames to representatively sample stream */
	double stream_Xhi;					/* Total stream complexity */
	bool seq_hdr_every_gop;
	bool seq_end_every_gop;	/* Useful for Stills sequences... */
	bool svcd_scan_data;
	unsigned int vbv_buffer_code;      /* Code for size of VBV buffer (*
										* 16 kbit) */
	double vbv_buffer_size;

	unsigned int still_size; /* If non-0 encode a stills sequence: 1
								I-frame per sequence pseudo VBR. Each
								frame sized to still_size KB */
	unsigned int vbv_buffer_still_size;  /* vbv_buffer_size holds
											still size.  Ensure still
											size matches. */

	bool constrparms;         /* constrained parameters flag, MPEG-1 only */
	bool load_iquant; 
	bool load_niquant;        /* use non-default quant. matrices */



    int profile, level;         /* syntax / parameter constraints */
	bool ignore_constraints;	/* Disabled conformance checking of
								 * hor_size, vert_size and
								 * samp_rate */
    bool dualprime;             /* Allow dual prime motion compensation */
			

	bool prog_seq; /* progressive sequence */
	int low_delay; /* no B pictures, skipped pictures */

    /*******
	 *
	 * sequence specific data (sequence display extension)
	 *
	 ******/
	
	unsigned int video_format; /* component, PAL, NTSC, SECAM or MAC */
	unsigned int color_primaries; /* source primary chromaticity coordinates */
	unsigned int transfer_characteristics; /* opto-electronic transfer char. (gamma) */
	unsigned int matrix_coefficients; /* Eg,Eb,Er / Y,Cb,Cr matrix coefficients */
	unsigned int display_horizontal_size;  /* display size */
	unsigned int display_vertical_size;


	bool mpeg1;				/* ISO/IEC IS 11172-2 sequence */
	bool fieldpic;			/* use field pictures */
	bool pulldown_32;		/* 3:2 Pulldown of movie material */
	bool topfirst;

	/************
	 *
	 * Picture kind specific informatino (picture header flags)
     *
     ***********/
	
	int frame_pred_dct_tab[3]; /* use only frame prediction and frame
								  DCT (I,P,B) */
	int qscale_tab[3];			/* linear/non-linear quantizaton table */
	int intravlc_tab[3];		/* intra vlc format (I,P,B) */
	int altscan_tab[3];			/* alternate scan (I,P,B */
	unsigned int dc_prec;

    /****************************
	 * Encoder internal derived values and parameters
	 *************************** */

	int enc_width, 
		enc_height;   /* encoded frame size (pels) multiples of 16 or 32 */
	
	int phy_width, 
		phy_height;   /* Physical Frame buffer size (pels) may differ
						 from encoded size due to alignment
						 constraints */
	int enc_chrom_width,enc_chrom_height;
	int phy_chrom_width,phy_chrom_height;

	int lum_buffer_size, chrom_buffer_size;

	int mb_width, mb_height;	/* frame size (macroblocks) */

	/* Picture dimensioning (allowing for interlaced/non-interlaced coding) */
	int phy_width2, phy_height2, enc_height2,
		mb_height2, phy_chrom_width2;
	int qsubsample_offset, 
		fsubsample_offset;
	int mb_per_pict;			/* Number of macro-blocks in a picture */  

	
	struct motion_data *motion_data;


    /* Selected intra/non_intra quantization matrices both ordinary*/
	/* and inverted */
	uint16_t *intra_q, *inter_q;
	
   /* **************************
    * Global flags controlling encoding behaviour 
    ************************** */

    double decode_frame_rate;	/* Actual stream frame
                                 * decode-rate. This is lower than
                                 * playback rate if 3:2 pulldown is
                                 * active.
								*/
    int video_buffer_size;    /* Video buffer requirement target */

    unsigned int N_max;				/* number of frames in Group of Pictures (max) */
    unsigned int N_min;				/* number of frames in Group of Pictures (min) */
    int M;					/* distance between I/P frames */

    int M_min;			    /* Minimum distance between I/P frames */

    bool closed_GOPs;	    /* Force all GOPs to be closed - useful
                             * for satisfying requirements for
                             * multi-angle DVD authoring */

    int me44_red;			/* Sub-mean population reduction passes
                            * for 4x4 and 2x2 */
    int me22_red;			/* Motion compensation stages  */
    int seq_length_limit;
    double nonvid_bit_rate;	/* Bit-rate for non-video to assume for
								   sequence splitting calculations */

    double quant_floor;    /* quantisation floor [1..10] (0 for
                                * CBR) */


    double act_boost;		/* Quantisation reduction factor for blocks
                                   with little texture (low variance) */

    double boost_var_ceil;		/* Variance below which
									 * quantisation boost cuts in */


    int encoding_parallelism; /* Maximum number of concurrent worker threads
                                 to be used for encoding  */

    int unit_coeff_elim;	/* Threshold of unit coefficient
                                   density below which unit
                                   coefficient blocks should be
                                   zeroed.  < 0 implies DCT
                                   coefficient should be included. */

    double coding_tolerance;  /* Fraction of bit allocation that
                                      actual coding size may deviate from
                                      target set by rate controller before
                                      a re-encoding is forced */

    deque<int> chapter_points; /* Frame #'s for where chapters occur (I frames, closed GOP's) */

};


#endif

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
