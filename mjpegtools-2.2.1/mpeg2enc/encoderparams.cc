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

#include <config.h>
#include "encoderparams.hh"
#include "format_codes.h"
#include "mpegconsts.h"
#include "mpeg2encoptions.hh"
//#include "mpeg2syntaxcodes.h"
#include "tables.h"
#include "ratectl.hh"
#include "cpu_accel.h"
#include "motionsearch.h"
#include <string.h> // REMOVE

#define MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define MIN(a,b) ( (a)<(b) ? (a) : (b) )

EncoderParams::EncoderParams( const MPEG2EncOptions &encoptions)
{
}

void EncoderParams::InitEncodingControls( const MPEG2EncOptions &options)
{

    coding_tolerance = 0.1;
	/* Tune threading and motion compensation for specified number of CPU's 
	   and specified speed parameters.
	 */
    
	act_boost =  1.0+options.act_boost;
    boost_var_ceil = options.boost_var_ceil;
	switch( options.num_cpus )
	{

	case 0 : /* Special case for debugging... turns of all multi-threading */
		encoding_parallelism = 0;
		break;
  case 1 : /* Currently this is the default option */
		encoding_parallelism = 1;
		break;
	case 2:
		encoding_parallelism = 2;
		break;
	default :
		encoding_parallelism = options.num_cpus > MAX_WORKER_THREADS-1 ?
			                  MAX_WORKER_THREADS-1 :
			                  options.num_cpus;
		break;
	}

	me44_red		= options.me44_red;
	me22_red		= options.me22_red;

    unit_coeff_elim	= options.unit_coeff_elim;

	/* round picture dimensions to nearest multiple of 16 or 32 */
	mb_width = (horizontal_size+15)/16;
	mb_height = prog_seq ? (vertical_size+15)/16 : 2*((vertical_size+31)/32);
	mb_height2 = fieldpic ? mb_height>>1 : mb_height; /* for field pictures */
	enc_width = 16*mb_width;
	enc_height = 16*mb_height;

#ifdef DEBUG_MOTION_EST
    static const int MARGIN = 64;
#else
    static const int MARGIN = 8;
#endif
    
#ifdef HAVE_ALTIVEC
	/* Pad phy_width to 64 so that the rowstride of 4*4
	 * sub-sampled data will be a multiple of 16 (ideal for AltiVec)
	 * and the rowstride of 2*2 sub-sampled data will be a multiple
	 * of 32. Height does not affect rowstride, no padding needed.
	 */
	phy_width = (enc_width + 63) & (~63);
#else
	phy_width = enc_width+MARGIN;
#endif
	phy_height = enc_height+MARGIN;

	/* Calculate the sizes and offsets in to luminance and chrominance
	   buffers.  A.Stevens 2000 for luminance data we allow space for
	   fast motion estimation data.  This is actually 2*2 pixel
	   sub-sampled uint8_t followed by 4*4 sub-sampled.  We add an
	   extra row to act as a margin to allow us to neglect / postpone
	   edge condition checking in time-critical loops...  */

	phy_chrom_width = phy_width>>1;
	phy_chrom_height = phy_height>>1;
	enc_chrom_width = enc_width>>1;
	enc_chrom_height = enc_height>>1;

	phy_height2 = fieldpic ? phy_height>>1 : phy_height;
	enc_height2 = fieldpic ? enc_height>>1 : enc_height;
	phy_width2 = fieldpic ? phy_width<<1 : phy_width;
	phy_chrom_width2 = fieldpic 
        ? phy_chrom_width<<1 
        : phy_chrom_width;
 
	lum_buffer_size = (phy_width*phy_height) +
					 sizeof(uint8_t) *(phy_width/2)*(phy_height/2) +
					 sizeof(uint8_t) *(phy_width/4)*(phy_height/4);
	chrom_buffer_size = phy_chrom_width*phy_chrom_height;
    

	fsubsample_offset = (phy_width)*(phy_height) * sizeof(uint8_t);
	qsubsample_offset =  fsubsample_offset 
        + (phy_width/2)*(phy_height/2)*sizeof(uint8_t);

	mb_per_pict = mb_width*mb_height2;


#ifdef OUTPUT_STAT
	/* open statistics output file */
	if (!(statfile = fopen(statname,"w")))
	{
		mjpeg_error_exit1( "Couldn't create statistics output file %s",
						   statname);
	}
#endif
}


static int f_code( int max_radius )
{
	int c=5;
	if( max_radius < 64) c = 4;
	if( max_radius < 32) c = 3;
	if( max_radius < 16) c = 2;
	if( max_radius < 8) c = 1;
	return c;
}

void EncoderParams::Init( const MPEG2EncOptions &options )
{
	int i;
    const char *msg = 0;

	//istrm_nframes = 999999999; /* determined by EOF of stdin */

	N_min = options.min_GOP_size;      /* I frame distance */
	N_max = options.max_GOP_size;
    closed_GOPs = options.closed_GOPs;
	mjpeg_info( "GOP SIZE RANGE %d TO %d %s", 
                N_min, N_max,
                closed_GOPs ? "(all GOPs closed)" : "" 
                );
	M = options.Bgrp_size;             /* I or P frame distance */
	M_min = options.preserve_B ? M : 1;
	if( M > N_max )
		M = N_max;
	mpeg1       = (options.mpeg == 1);
	fieldpic        = (options.fieldenc == 2);
    dualprime   = options.hack_dualprime == 1 && M == 1;

    pulldown_32     = options.vid32_pulldown;

    aspectratio     = options.aspect_ratio;
    frame_rate_code = options.frame_rate;
    // SVCD and probably DVD? mandate progressive_sequence = 0 
    switch( options.format )
    {
    case MPEG_FORMAT_SVCD :
    case MPEG_FORMAT_SVCD_NSR :
    case MPEG_FORMAT_SVCD_STILL :
    case MPEG_FORMAT_DVD :
    case MPEG_FORMAT_DVD_NAV :
        prog_seq = 0;
        break;
    case MPEG_FORMAT_ATSC1080i :
    case MPEG_FORMAT_ATSC480i :
        prog_seq = frame_rate_code == 1 || frame_rate_code == 2 ? 0 : 1;
        break;
    case MPEG_FORMAT_ATSC720p :
    case MPEG_FORMAT_ATSC480p :
        prog_seq = 1;
        break;
    default :
        // If we want 3:2 pulldown must code prog_seq as otherwise
        // repeat_first_field and topfirst encode frame repetitions!!!
        prog_seq        = (options.mpeg == 1 || (options.fieldenc == 0 && !options.vid32_pulldown));
        break;
    }

	dctsatlim		= mpeg1 ? 255 : 2047;

	/* If we're using a non standard (VCD?) profile bit-rate adjust	the vbv
		buffer accordingly... */

	if(options.bitrate == 0 )
	{
		mjpeg_error_exit1( "Generic format - must specify bit-rate!" );
	}

	still_size = 0;
	if( MPEG_STILLS_FORMAT(options.format) )
	{
		vbv_buffer_code = options.vbv_buffer_still_size / 2048;
		vbv_buffer_still_size = options.pad_stills_to_vbv_buffer_size;
		bit_rate = options.bitrate;
		still_size = options.still_size;
	}
	else if( options.mpeg == 1 )
	{
		/* Scale VBV relative to VCD  */
		bit_rate = MAX(10000, options.bitrate);
		vbv_buffer_code = (20 * options.bitrate  / 1151929);
	}
	else
	{
		bit_rate = MAX(10000, options.bitrate);
		vbv_buffer_code = MIN(112,options.video_buffer_size / 2);
	}

	target_bitrate = options.target_bitrate;
	if( target_bitrate > bit_rate )
	{
		target_bitrate = bit_rate;
		mjpeg_warn( "Target bit rate may not exceed specified (max) bit-rate: reducing to %.0fKbps",
					target_bitrate	);
	}

	stream_Xhi = options.stream_Xhi;
	stream_frames = options.stream_frames;
	vbv_buffer_size = vbv_buffer_code*16384;

	if( options.quant )
	{

		quant_floor = RateCtl::InvScaleQuant( options.mpeg == 1 ? 0 : 1, 
                                              options.quant );
		mjpeg_info( "Quant  code = %.0f quantizer-scale = %d", quant_floor, options.quant );
	}
	else
	{
		quant_floor = 0.0;		/* Larger than max quantisation */
	}

	video_buffer_size = options.video_buffer_size * 1024 * 8;
	
	seq_hdr_every_gop = options.seq_hdr_every_gop;
	seq_end_every_gop = options.seq_end_every_gop;
	svcd_scan_data = options.svcd_scan_data;
	ignore_constraints = options.ignore_constraints;
	seq_length_limit = options.seq_length_limit;
	nonvid_bit_rate = options.nonvid_bitrate * 1000;
	low_delay       = 0;
	constrparms     = (options.mpeg == 1 &&
						   !MPEG_STILLS_FORMAT(options.format));
	profile         = MAIN_PROFILE;
    
    level = options.level;
    // Force appropriate level for standards-compliant preset-formats
    switch(options.format)
    {
        case MPEG_FORMAT_ATSC720p :
        case MPEG_FORMAT_ATSC1080i :
            level = HIGH_LEVEL;
            break;
        case MPEG_FORMAT_SVCD_STILL :
        case MPEG_FORMAT_SVCD :
        case MPEG_FORMAT_DVD :
        case MPEG_FORMAT_DVD_NAV :
        case MPEG_FORMAT_VCD :
        default :
            level = MAIN_LEVEL;
            break;
    };
    if( level == 0 )
        level = MAIN_LEVEL;

    if( MPEG_SDTV_FORMAT(options.format) )
    {
        switch(options.norm)
        {
        case 'p': video_format = 1; break;
        case 'n': video_format = 2; break;
        case 's': video_format = 3; break;
        default:  video_format = 5; break; /* unspec. */
        }
     
        switch(options.norm)
        {
        case 's':
        case 'p':  /* ITU BT.470  B,G */
            color_primaries = 5;
            transfer_characteristics = 5; /* Gamma = 2.8 (!!) */
            matrix_coefficients = 5; 
            msg = "PAL B/G";
            break;
        case 'n': /* SMPTPE 170M "modern NTSC" */
            color_primaries = 6;
            matrix_coefficients = 6; 
            transfer_characteristics = 6;
            msg = "NTSC";
            break; 
        default:   /* unspec. */
            color_primaries = 2;
            matrix_coefficients = 2; 
            transfer_characteristics = 2;
            msg = "unspecified";
            break;
        }
    }
    else
    {
        video_format = 0; // 'Component'
        switch( options.format )
        {
            case MPEG_FORMAT_ATSC480i : /* SMPTPE 170M "modern NTSC" */
            case MPEG_FORMAT_ATSC480p :
                color_primaries = 6;
                matrix_coefficients = 6; 
                transfer_characteristics = 6;
                break;
            case MPEG_FORMAT_ATSC720p :/* ITU.R BT.709 HDTV */
            case MPEG_FORMAT_ATSC1080i :
                color_primaries = 1;
                matrix_coefficients = 1; 
                transfer_characteristics = 1;
                break;
            default : 
                abort();
                
        };
    };
    mjpeg_info( "Setting colour/gamma parameters to \"%s\"", msg);

    horizontal_size = options.in_img_width;
    vertical_size = options.in_img_height;
	switch( options.format )
	{
	case MPEG_FORMAT_SVCD_STILL :
	case MPEG_FORMAT_SVCD_NSR :
	case MPEG_FORMAT_SVCD :
    case MPEG_FORMAT_DVD :
    case MPEG_FORMAT_DVD_NAV :
        /* It would seem DVD and perhaps SVCD demand a 540 pixel display size
           for 4:3 aspect video. However, many players expect 480 and go weird
           if this isn't set...
        */
        if( options.hack_svcd_hds_bug )
        {
            display_horizontal_size  = options.in_img_width;
            display_vertical_size    = options.in_img_height;
        }
        else
        {
            display_horizontal_size  = aspectratio == 2 ? 540 : 720;
            display_vertical_size    = options.in_img_height;
        }
		break;
  
      // ATSC 1080i is unusual in that it *requires* display of 1080 lines
      // when 1088 are coded 
     case MPEG_FORMAT_ATSC1080i :
         display_vertical_size = 1080;
         break;
     
     default:
        if( options.display_hsize <= 0 )
            display_horizontal_size  = options.in_img_width;
        else
            display_horizontal_size = options.display_hsize;
        if( options.display_vsize <= 0 )
		  display_vertical_size = options.in_img_height;
        else
            display_vertical_size = options.display_vsize;
		break;
	}

	dc_prec         = options.mpeg2_dc_prec;  /* 9 bits */
    topfirst = 0;
	if( ! prog_seq )
	{
		int fieldorder;
		if( options.force_interlacing != Y4M_UNKNOWN ) 
		{
			mjpeg_info( "Forcing playback video to be: %s",
						mpeg_interlace_code_definition(	options.force_interlacing ) );	
			fieldorder = options.force_interlacing;
		}
		else
			fieldorder = options.input_interlacing;

		topfirst = 
            (fieldorder == Y4M_ILACE_TOP_FIRST || fieldorder ==Y4M_ILACE_NONE );
	}
	else
		topfirst = 0;

    // Restrict to frame motion estimation and DCT modes only when MPEG1
    // or when progressive content is specified for MPEG2.
    // Note that for some profiles although we have progressive sequence 
    // header bit = 0 we still only encode with frame modes (for speed).
	frame_pred_dct_tab[0] 
		= frame_pred_dct_tab[1] 
		= frame_pred_dct_tab[2] 
        = (options.mpeg == 1 || options.fieldenc == 0) ? 1 : 0;

    mjpeg_info( "Progressive format frames = %d", 	frame_pred_dct_tab[0] );
	qscale_tab[0] 
		= qscale_tab[1] 
		= qscale_tab[2] 
		= options.mpeg == 1 ? 0 : 1;
    mjpeg_info( "q_scale_type  = %d",	qscale_tab[0] );
	intravlc_tab[0] 
		= intravlc_tab[1] 
		= intravlc_tab[2] 
		= options.mpeg == 1 ? 0 : 1;

	altscan_tab[2]  
		= altscan_tab[1]  
		= altscan_tab[0]  
		= (options.mpeg == 1 || options.hack_altscan_bug) ? 0 : 1;
	

	/*  A.Stevens 2000: The search radius *has* to be a multiple of 8
		for the new fast motion compensation search to work correctly.
		We simply round it up if needs be.  */

    int searchrad = options.searchrad;
	if(searchrad*M>127)
	{
		searchrad = 127/M;
		mjpeg_warn("Search radius reduced to %d",searchrad);
	}
	
	{ 
		int radius_x = searchrad;
		int radius_y = searchrad*vertical_size/horizontal_size;

		/* TODO: These f-codes should really be adjusted for each
		   picture type... */

		motion_data = (struct motion_data *)malloc(M*sizeof(struct motion_data));
		if (!motion_data)
			mjpeg_error_exit1("malloc failed");

		for (i=0; i<M; i++)
		{
			if(i==0)
			{
				motion_data[i].sxf = round_search_radius(radius_x*M);
				motion_data[i].forw_hor_f_code  = f_code(motion_data[i].sxf);
				motion_data[i].syf = round_search_radius(radius_y*M);
				motion_data[i].forw_vert_f_code  = f_code(motion_data[i].syf);
			}
			else
			{
				motion_data[i].sxf = round_search_radius(radius_x*i);
				motion_data[i].forw_hor_f_code  = f_code(motion_data[i].sxf);
				motion_data[i].syf = round_search_radius(radius_y*i);
				motion_data[i].forw_vert_f_code  = f_code(motion_data[i].syf);
				motion_data[i].sxb = round_search_radius(radius_x*(M-i));
				motion_data[i].back_hor_f_code  = f_code(motion_data[i].sxb);
				motion_data[i].syb = round_search_radius(radius_y*(M-i));
				motion_data[i].back_vert_f_code  = f_code(motion_data[i].syb);
			}

			/* MPEG-1 demands f-codes for vertical and horizontal axes are
			   identical!!!!
			*/
			if( mpeg1 )
			{
				motion_data[i].syf = motion_data[i].sxf;
				motion_data[i].syb  = motion_data[i].sxb;
				motion_data[i].forw_vert_f_code  = 
					motion_data[i].forw_hor_f_code;
				motion_data[i].back_vert_f_code  = 
					motion_data[i].back_hor_f_code;
				
			}
		}
		
	}
	


	/* make sure MPEG specific parameters are valid */
	RangeChecks();

	/* Set the frame decode rate and frame display rates.
	   For 3:2 movie pulldown decode rate is != display rate due to
	   the repeated field that appears every other frame.
	*/
	frame_rate = Y4M_RATIO_DBL(mpeg_framerate(frame_rate_code));
	if( options.vid32_pulldown )
	{
		decode_frame_rate = frame_rate * (2.0 + 2.0) / (3.0 + 2.0);
		mjpeg_info( "3:2 Pulldown selected frame decode rate = %3.3f fps", 
					decode_frame_rate);
	}
	else
		decode_frame_rate = frame_rate;

	if ( !mpeg1)
	{
		ProfileAndLevelChecks();
	}
	else
	{
		/* MPEG-1 */
		if (constrparms)
		{
			if (horizontal_size>768
				|| vertical_size>576
				|| ((horizontal_size+15)/16)*((vertical_size+15)/16)>396
				|| ((horizontal_size+15)/16)*((vertical_size+15)/16)*frame_rate>396*25.0
				|| frame_rate>30.0)
			{
				mjpeg_info( "size - setting constrained_parameters_flag = 0");
				constrparms = 0;
			}
		}

		if (constrparms)
		{
			for (i=0; i<M; i++)
			{
				if (motion_data[i].forw_hor_f_code>4)
				{
					mjpeg_info("Hor. motion search forces constrained_parameters_flag = 0");
					constrparms = 0;
					break;
				}

				if (motion_data[i].forw_vert_f_code>4)
				{
					mjpeg_info("Ver. motion search forces constrained_parameters_flag = 0");
					constrparms = 0;
					break;
				}

				if (i!=0)
				{
					if (motion_data[i].back_hor_f_code>4)
					{
						mjpeg_info("Hor. motion search setting constrained_parameters_flag = 0");
						constrparms = 0;
						break;
					}

					if (motion_data[i].back_vert_f_code>4)
					{
						mjpeg_info("Ver. motion search setting constrained_parameters_flag = 0");
						constrparms = 0;
						break;
					}
				}
			}
		}
	}

	/* relational checks */
	if ( mpeg1 )
	{
		if (!prog_seq)
		{
			mjpeg_warn("mpeg1 specified - setting progressive_sequence = 1");
			prog_seq = 1;
		}

		if (dc_prec!=0)
		{
			mjpeg_info("mpeg1 - setting intra_dc_precision = 0");
			dc_prec = 0;
		}

		for (i=0; i<3; i++)
			if (qscale_tab[i])
			{
				mjpeg_info("mpeg1 - setting qscale_tab[%d] = 0",i);
				qscale_tab[i] = 0;
			}

		for (i=0; i<3; i++)
			if (intravlc_tab[i])
			{
				mjpeg_info("mpeg1 - setting intravlc_tab[%d] = 0",i);
				intravlc_tab[i] = 0;
			}

		for (i=0; i<3; i++)
			if (altscan_tab[i])
			{
				mjpeg_info("mpeg1 - setting altscan_tab[%d] = 0",i);
				altscan_tab[i] = 0;
			}
	}

	if ( !mpeg1 && constrparms)
	{
		mjpeg_info("not mpeg1 - setting constrained_parameters_flag = 0");
		constrparms = 0;
	}


	if( (!prog_seq || fieldpic != 0 ) &&
		( (vertical_size+15) / 16)%2 != 0 )
	{
		mjpeg_warn( "Frame height won't split into two equal field pictures...");
		mjpeg_warn( "forcing encoding as progressive video");
		prog_seq = 1;
        pulldown_32 = false;
		fieldpic = 0;
	}


	if (prog_seq && fieldpic != 0)
	{
		mjpeg_info("prog sequence - forcing progressive frame encoding");
		fieldpic = 0;
	}


	if (prog_seq && topfirst )
	{
		mjpeg_info("prog sequence setting top_field_first = 0");
		topfirst = 0;
	}

	/* search windows */
	for (i=0; i<M; i++)
	{
		if (motion_data[i].sxf > (4U<<motion_data[i].forw_hor_f_code)-1)
		{
			mjpeg_info(
				"reducing forward horizontal search width to %d",
						(4<<motion_data[i].forw_hor_f_code)-1);
			motion_data[i].sxf = (4U<<motion_data[i].forw_hor_f_code)-1;
		}

		if (motion_data[i].syf > (4U<<motion_data[i].forw_vert_f_code)-1)
		{
			mjpeg_info(
				"reducing forward vertical search width to %d",
				(4<<motion_data[i].forw_vert_f_code)-1);
			motion_data[i].syf = (4U<<motion_data[i].forw_vert_f_code)-1;
		}

		if (i!=0)
		{
			if (motion_data[i].sxb > (4U<<motion_data[i].back_hor_f_code)-1)
			{
				mjpeg_info(
					"reducing backward horizontal search width to %d",
					(4<<motion_data[i].back_hor_f_code)-1);
				motion_data[i].sxb = (4U<<motion_data[i].back_hor_f_code)-1;
			}

			if (motion_data[i].syb > (4U<<motion_data[i].back_vert_f_code)-1)
			{
				mjpeg_info(
					"reducing backward vertical search width to %d",
					(4<<motion_data[i].back_vert_f_code)-1);
				motion_data[i].syb = (4U<<motion_data[i].back_vert_f_code)-1;
			}
		}
	}

    InitQuantMatrices( options );
    InitEncodingControls( options );

    chapter_points.insert(chapter_points.end(),options.chapter_points.begin(),options.chapter_points.end());
}


/*
  If the use has selected suppression of hf noise via quantisation
  then we boost quantisation of hf components EXPERIMENTAL: currently
  a linear ramp from 0 at 4pel to hf_q_boost increased
  quantisation...

*/

static int quant_hfnoise_filt(int orgquant, int qmat_pos, double hf_q_boost )
    {
    int orgdist = MAX(qmat_pos % 8, qmat_pos/8);
    double qboost;

    /* Maximum hf_q_boost quantisation boost for HF components.. */
    qboost = 1.0 + ((hf_q_boost * orgdist) / 8);
    return static_cast<int>(orgquant * qboost);
    }


void EncoderParams::InitQuantMatrices( const MPEG2EncOptions &options )
{
    int i, v;
    const char *msg = NULL;
    const uint16_t *qmat = 0;
    const uint16_t *niqmat = 0;
    load_iquant = 0;
    load_niquant = 0;

    /* bufalloc to ensure alignment */
    intra_q = static_cast<uint16_t*>(bufalloc(sizeof(uint16_t[64])));
    inter_q = static_cast<uint16_t*>(bufalloc(sizeof(uint16_t[64])));

    switch  (options.hf_quant)
    {
    case  0:    /* No -N, -H or -K used.  Default matrices */
        msg = "Using default unmodified quantization matrices";
        qmat = default_intra_quantizer_matrix;
        niqmat = default_nonintra_quantizer_matrix;
        break;
    case  1:    /* "-N value" used but not -K or -H */
        msg = "Using -N modified default quantization matrices";
        qmat = default_intra_quantizer_matrix;
        niqmat = default_nonintra_quantizer_matrix;
        load_iquant = 1;
        load_niquant = 1;
        break;
    case  2:    /* -H used OR -H followed by "-N value" */
        msg = "Setting hi-res intra Quantisation matrix";
        qmat = hires_intra_quantizer_matrix;
        niqmat = hires_nonintra_quantizer_matrix;
        load_iquant = 1;
        if(options.hf_q_boost)
            load_niquant = 1;   /* Custom matrix if -N used */
        break;
    case  3:
        msg = "KVCD Notch Quantization Matrix";
        qmat = kvcd_intra_quantizer_matrix;
        niqmat = kvcd_nonintra_quantizer_matrix;
        load_iquant = 1;
        load_niquant = 1;
        break;
    case  4:
        msg = "TMPGEnc Quantization matrix";
        qmat = tmpgenc_intra_quantizer_matrix;
        niqmat = tmpgenc_nonintra_quantizer_matrix;
        load_iquant = 1;
        load_niquant = 1;
        break;
    case  5:
        msg = "Flat quantization matrix for ultra high quality encoding";
        load_iquant = 1;
        load_niquant = 1;
        qmat = flat_intra_quantizer_matrix;
        niqmat = flat_nonintra_quantizer_matrix;
        break;
    case  6:            /* -K file=qmatrixfilename */
        msg = "Loading custom matrices from user specified file";
        load_iquant = 1;
        load_niquant = 1;
        qmat = options.custom_intra_quantizer_matrix;
        niqmat = options.custom_nonintra_quantizer_matrix;
        break;
    default:
        mjpeg_error_exit1("Help!  Unknown hf_quant value %d",
                          options.hf_quant);
        /* NOTREACHED */
    }

    if  (msg)
        mjpeg_info( "%s", msg);
    
    for (i = 0; i < 64; i++)
    {
        v = quant_hfnoise_filt(qmat[i], i, options.hf_q_boost);
        if  (v < 1 || v > 255)
            mjpeg_error_exit1("bad intra value after -N adjust");
        intra_q[i] = v;

        v = quant_hfnoise_filt(niqmat[i], i, options.hf_q_boost);
        if  (v < 1 || v > 255)
            mjpeg_error_exit1("bad nonintra value after -N adjust");
        inter_q[i] = v;
    }

}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
