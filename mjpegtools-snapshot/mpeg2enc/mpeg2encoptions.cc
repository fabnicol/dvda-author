/* mpeg2encoptions.cc - Encoder control parameter class   */

/* (C) 2000/2001/2003 Andrew Stevens, Rainer Johanni */

/* This software is free software; you can redistribute it
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
#include "format_codes.h"
#include "mpegconsts.h"
#include "mpeg2encoder.hh"

MPEG2EncOptions::MPEG2EncOptions()
{
    // Parameters initialised to -1 indicate a format-dependent
    // or stream inferred default.
    format = MPEG_FORMAT_MPEG1;
    level = 0;          // Use default
    display_hsize  = 0; // Use default
    display_vsize  = 0; // Use default
    bitrate    = 0;
    target_bitrate = 0;
    nonvid_bitrate = 0;
    stream_frames = 0;   			// none specified by default
    stream_Xhi = 0.0;				// off by default
    quant      = 0;
    searchrad  = 0;     // Use default
    mpeg       = 1;
    aspect_ratio = 0;
    frame_rate  = 0;
    fieldenc   = -1; /* 0: progressive, 1 = frame pictures,
                        interlace frames with field MC and DCT
                        in picture 2 = field pictures
                     */
    norm       = 0;  /* 'n': NTSC, 'p': PAL, 's': SECAM, else unspecified */
    rate_control = 0;
    me44_red	= 2;
    me22_red	= 3;	
    hf_quant = 0;
    hf_q_boost = 0.0;
    act_boost = 0.0;
    boost_var_ceil = 10*10;
    video_buffer_size = 0;
    seq_length_limit = 0;
    min_GOP_size = -1;
    max_GOP_size = -1;
    closed_GOPs = 0;
    preserve_B = 0;
    Bgrp_size = 1;
/*
 * Set the default to 0 until this error:
     INFO: [mpeg2enc] Signaling last frame = 499
     mpeg2enc: seqencoder.cc:433: void SeqEncoder::EncodeStream(): Assertion `pass1coded.size() == 0' failed.
     Abort
 * Is fixed.
*/
    num_cpus = 0;
    vid32_pulldown = 0;
    svcd_scan_data = -1;
    seq_hdr_every_gop = 0;
    seq_end_every_gop = 0;
    still_size = 0;
    pad_stills_to_vbv_buffer_size = 0;
    vbv_buffer_still_size = 0;
    force_interlacing = Y4M_UNKNOWN;
    input_interlacing = Y4M_UNKNOWN;
    mpeg2_dc_prec = 1;
    ignore_constraints = 0;
    unit_coeff_elim = 0;
    force_cbr = 0;
    verbose = 1;
    hack_svcd_hds_bug = 1;
    hack_altscan_bug = 0;
    /* dual prime Disabled by default. --dualprime-mpeg2 to enable (set to 1) */
    hack_dualprime = 0;
    force_cbr = 0;
};


static int infer_mpeg1_aspect_code( char norm, mpeg_aspect_code_t mpeg2_code )
{
	switch( mpeg2_code )
	{
	case 1 :					/* 1:1 */
		return 1;
	case 2 :					/* 4:3 */
		if( norm == 'p' || norm == 's' )
			return 8;
	    else if( norm == 'n' )
			return 12;
		else 
			return 0;
	case 3 :					/* 16:9 */
		if( norm == 'p' || norm == 's' )
			return 3;
	    else if( norm == 'n' )
			return 6;
		else
			return 0;
	default :
		return 0;				/* Unknown */
	}
}

 
int MPEG2EncOptions::InferStreamDataParams( const MPEG2EncInVidParams &strm)
{
	int nerr = 0;


	/* Infer norm, aspect ratios and frame_rate if not specified */
	if( frame_rate == 0 )
	{
		if(strm.frame_rate_code<1 || strm.frame_rate_code>8)
		{
			mjpeg_error("Input stream with unknown frame-rate and no frame-rate specified with -a!");
			++nerr;
		}
		else
			frame_rate = strm.frame_rate_code;
	}

	if( norm == 0 && (strm.frame_rate_code==3 || strm.frame_rate_code == 2) )
	{
		mjpeg_info("Assuming norm PAL");
		norm = 'p';
	}
	if( norm == 0 && (strm.frame_rate_code==4 || strm.frame_rate_code == 1) )
	{
		mjpeg_info("Assuming norm NTSC");
		norm = 'n';
	}





	if( frame_rate != 0 )
	{
		if( strm.frame_rate_code != frame_rate && 
            mpeg_valid_framerate_code(strm.frame_rate_code) )
		{
			mjpeg_warn( "Specified display frame-rate %3.2f will over-ride", 
						Y4M_RATIO_DBL(mpeg_framerate(frame_rate)));
			mjpeg_warn( "(different!) frame-rate %3.2f of the input stream",
						Y4M_RATIO_DBL(mpeg_framerate(strm.frame_rate_code)));
		}
	}

	if( aspect_ratio == 0 )
	{
		aspect_ratio = strm.aspect_ratio_code;
	}

	if( aspect_ratio == 0 )
	{
		mjpeg_warn( "No aspect ratio specifed and no guess possible: assuming 4:3 display aspect!");
		aspect_ratio = 2;
	}

	/* Convert to MPEG1 coding if we're generating MPEG1 */
	if( mpeg == 1 )
	{
		aspect_ratio = infer_mpeg1_aspect_code( norm, aspect_ratio );
	}

    input_interlacing = strm.interlacing_code;
    if (input_interlacing == Y4M_UNKNOWN) {
        mjpeg_warn("Unknown input interlacing; assuming progressive.");
        input_interlacing = Y4M_ILACE_NONE;
    }

    /* 'fieldenc' is dependent on input stream interlacing:
         a) Interlaced streams are subsampled _per_field_;
             progressive streams are subsampled over whole frame.
         b) 'fieldenc' sets/clears the MPEG2 'progressive-frame' flag,
            which tells decoder how subsampling was performed.
    */
    if (fieldenc == -1) {
        /* not set yet... so set fieldenc from input interlacing */
        switch (input_interlacing) {
        case Y4M_ILACE_TOP_FIRST:
        case Y4M_ILACE_BOTTOM_FIRST:
            fieldenc = 1; /* interlaced frames */
            mjpeg_info("Interlaced input - selecting interlaced encoding.");
            break;
        case Y4M_ILACE_NONE:
            fieldenc = 0; /* progressive frames */
            mjpeg_info("Progressive input - selecting progressive encoding.");
            break;
        default:
            mjpeg_warn("Unknown input interlacing; assuming progressive.");
            fieldenc = 0;
            break;
        }
    } else {
        /* fieldenc has been set already... so double-check for user */
        switch (input_interlacing) {
        case Y4M_ILACE_TOP_FIRST:
        case Y4M_ILACE_BOTTOM_FIRST:
            if (fieldenc == 0) {
                mjpeg_warn("Progressive encoding selected with interlaced input!");
                mjpeg_warn("  (This will damage the chroma channels.)");
            }
            break;
        case Y4M_ILACE_NONE:
            if (fieldenc != 0) {
                mjpeg_warn("Interlaced encoding selected with progressive input!");
                mjpeg_warn("  (This will damage the chroma channels.)");
            }
            break;
        }
    }

	return nerr;
}

int MPEG2EncOptions::CheckBasicConstraints()
{
	int nerr = 0;
	if( vid32_pulldown )
	{
		if( mpeg == 1 )
			mjpeg_error_exit1( "MPEG-1 cannot encode 3:2 pulldown (for transcoding to VCD set 24fps)!" );

		if( frame_rate != 4 && frame_rate != 5  )
		{
			if( frame_rate == 1 || frame_rate == 2 )
			{
				frame_rate += 3;
				mjpeg_warn("3:2 movie pulldown with frame rate set to decode rate not display rate");
				mjpeg_warn("3:2 Setting frame rate code to display rate = %d (%2.3f fps)", 
						   frame_rate,
						   Y4M_RATIO_DBL(mpeg_framerate(frame_rate)));

			}
			else
			{
				mjpeg_error( "3:2 movie pulldown not sensible for %2.3f fps dispay rate",
							Y4M_RATIO_DBL(mpeg_framerate(frame_rate)));
				++nerr;
			}
		}
		if( fieldenc == 2 )
		{
			mjpeg_error( "3:2 pulldown only possible for frame pictures (-I 1 or -I 0)");
			++nerr;
		}
	}
	
    if ((mpeg == 1) && (fieldenc != 0)) {
        mjpeg_error("Interlaced encoding (-I != 0) is not supported by MPEG-1.");
        ++nerr;
    }


	if(  !mpeg_valid_aspect_code(mpeg, aspect_ratio) )
	{
		mjpeg_error("For MPEG-%d, aspect ratio code  %d is illegal", 
					mpeg, aspect_ratio);
		++nerr;
	}
		


	if( min_GOP_size > max_GOP_size )
	{
		mjpeg_error( "Min GOP size must be <= Max GOP size" );
		++nerr;
	}

	if( stream_frames > 0 && stream_frames < 4*max_GOP_size )
	{
		mjpeg_error( "-L must be at at least 4 GOP lengths (4 * -G)" );
		++nerr;
	}
    if( preserve_B && Bgrp_size == 0 )
    {
		mjpeg_error_exit1("Preserving I/P frame spacing is impossible for still encoding" );
    }

	if( preserve_B && 
		( min_GOP_size % Bgrp_size != 0 ||
		  max_GOP_size % Bgrp_size != 0 )
		)
	{
		mjpeg_error("Preserving I/P frame spacing is impossible if min and max GOP sizes are" );
		mjpeg_error_exit1("Not both divisible by %d", Bgrp_size );
	}

	switch( format )
	{
	case MPEG_FORMAT_SVCD_STILL :
	case MPEG_FORMAT_SVCD_NSR :
	case MPEG_FORMAT_SVCD : 
		if( aspect_ratio != 2 && aspect_ratio != 3 )
			mjpeg_error_exit1("SVCD only supports 4:3 and 16:9 aspect ratios");
		if( svcd_scan_data )
		{
			mjpeg_warn( "Generating dummy SVCD scan-data offsets to be filled in by \"vcdimager\"");
			mjpeg_warn( "If you're not using vcdimager you may wish to turn this off using -d");
		}
		break;
     case MPEG_FORMAT_ATSC480i :
         if( frame_rate != 4 && frame_rate != 5 )
         {
             mjpeg_warn( "ATSC 480p only supports 29.97 and 30 frames/sec" );
         }
     case MPEG_FORMAT_ATSC480p :
         if(    (in_img_width != 704 && in_img_width != 640) 
             || in_img_height != 480 )
         {
             mjpeg_warn( "ATSC 480i/480p requires 640x480 or 704x480 input images!" );
         }
         if( in_img_width == 704 && aspect_ratio != 2 && aspect_ratio != 3 )
         {
             mjpeg_warn( "ATSC 480i/480p 704x480 only supports aspect ratio codes 2 and 3 (4:3 and 16:9)" );
         }
         if( in_img_width == 640 && aspect_ratio != 1 && aspect_ratio != 2 )
         {
             mjpeg_warn( "ATSC 480i/480p 704x480 only supports aspect ratio codes 1 and 2 (square pixel and 4:3)" );
         }
         break;
     case MPEG_FORMAT_ATSC720p :
         if(  in_img_width != 1280 || in_img_height != 720 )
         {
             mjpeg_warn( "ATSC 720p requires 1280x720 input images!" );
         }
         if( aspect_ratio != 1 && aspect_ratio != 3 )
         {
             mjpeg_warn( "ATSC 720p only supports aspect ratio codes 1 and 3 (square pixel and 16:9)" );
         }
         break;
     case MPEG_FORMAT_ATSC1080i :
         if(  in_img_width != 1920 || in_img_height != 1088 )
         {
             mjpeg_warn( "ATSC 1080i requires  1920x1088 input images!" );
         }
         if( aspect_ratio != 1 && aspect_ratio != 3 )
         {
             mjpeg_warn( "ATSC 1080i only supports aspect ratio codes 1 and 3 (square pixel and 16:9)" );
         }
         if( frame_rate > 7 )
         {
             mjpeg_warn( "ATSC 1080i only supports frame rates up to 30 frame/sec/" );
         }
         break;
	}
 
    
 if( MPEG_ATSC_FORMAT(format) )
    {
         if( bitrate > 38800000 )
        {
            mjpeg_warn( "ATSC specifies a maximum high data rate mode bitrate of 38.8Mbps" );
        }
        
        if( frame_rate == 3 || frame_rate == 6 )
        {
            mjpeg_warn( "ATSC does not support 25 or 50 frame/sec video" );
        }
    }

	return nerr;
}



bool MPEG2EncOptions::SetFormatPresets( const MPEG2EncInVidParams &strm )
{
    int nerr = 0;
    in_img_width = strm.horizontal_size;
    in_img_height = strm.vertical_size;
    mjpeg_info( "Selecting %s output profile", 
                mpeg_format_code_defintion(format));
	switch( format  )
	{
	case MPEG_FORMAT_MPEG1 :  /* Generic MPEG1 */
		if( video_buffer_size == 0 )
			video_buffer_size = 46;
		if (bitrate == 0)
			bitrate = 1151929;
        if (searchrad == 0)
            searchrad = 16;
		break;

	case MPEG_FORMAT_VCD :
		mpeg = 1;
		bitrate = 1151929;
		video_buffer_size = 46;
        	preserve_B = true;
        	Bgrp_size = 3;
        	min_GOP_size = 9;
		max_GOP_size = norm == 'n' ? 18 : 15;
		
	case MPEG_FORMAT_VCD_NSR : /* VCD format, non-standard rate */
		mpeg = 1;
		svcd_scan_data = 0;
		seq_hdr_every_gop = 1;
		if (bitrate == 0)
			bitrate = 1151929;
		if (video_buffer_size == 0)
			video_buffer_size = 46 * bitrate / 1151929;
        	if (seq_length_limit == 0 )
            		seq_length_limit = 700;
        	if (nonvid_bitrate == 0)
            		nonvid_bitrate = 230;
		break;
		
	case  MPEG_FORMAT_MPEG2 : 
		mpeg = 2;
		if (!force_cbr && quant == 0)
			quant = 8;
		if (video_buffer_size == 0)
			video_buffer_size = 230;
		break;

	case MPEG_FORMAT_SVCD :
		if (nonvid_bitrate == 0)
		   /* 224 kbps for audio + around 2% of 2788800 bits */
		   nonvid_bitrate = 288; 
		if (bitrate == 0 || bitrate > 2788800 - nonvid_bitrate * 1000)
		   bitrate = 2788800 - nonvid_bitrate * 1000;
		max_GOP_size = norm == 'n' ? 18 : 15;
		video_buffer_size = 230;

	case  MPEG_FORMAT_SVCD_NSR :		/* Non-standard data-rate */
		mpeg = 2;
		if (!force_cbr && quant == 0)
			quant = 8;
		if( svcd_scan_data == -1 )
			svcd_scan_data = 1;
		if (video_buffer_size == 0)
			video_buffer_size = 230;
		if( min_GOP_size == -1 )
            		min_GOP_size = 9;
        	seq_hdr_every_gop = 1;
        	if (seq_length_limit == 0)
            		seq_length_limit = 700;
        	if (nonvid_bitrate == 0)
            		nonvid_bitrate = 230;
        	break;

	case MPEG_FORMAT_VCD_STILL :
		mpeg = 1;
		quant = 0;	/* We want to try and hit our size target */

		/* We choose a generous nominal bit-rate as there's only 
		   one frame per sequence ;-).  It *is* too small to fill 
		   the frame-buffer in less than one PAL/NTSC frame
		   period though...*/
		bitrate = 8000000;

		/* Now we select normal/hi-resolution based on the input stream
		   resolution. 
		*/
		
		if( in_img_width == 352 && 
			(in_img_height == 240 || in_img_height == 288 ) )
		{
			/* VCD normal resolution still */
			if( still_size == 0 )
				still_size = 30*1024;
			if( still_size < 20*1024 || still_size > 42*1024 )
			{
				mjpeg_error_exit1( "VCD normal-resolution stills must be >= 20KB and <= 42KB each");
			}
			/* VBV delay encoded normally */
			vbv_buffer_still_size = 46*1024;
			video_buffer_size = 46;
			pad_stills_to_vbv_buffer_size = 0;
		}
		else if( in_img_width == 704 &&
				 (in_img_height == 480 || in_img_height == 576) )
		{
			/* VCD high-resolution stills: only these use vbv_delay
			 to encode picture size...
			*/
			if( still_size == 0 )
				still_size = 125*1024;
			if( still_size < 46*1024 || still_size > 220*1024 )
			{
				mjpeg_error_exit1( "VCD normal-resolution stills should be >= 46KB and <= 220KB each");
			}
			vbv_buffer_still_size = still_size;
			video_buffer_size = 224;
			pad_stills_to_vbv_buffer_size = 1;			
		}
		else
		{
			mjpeg_error("VCD normal resolution stills must be 352x288 (PAL) or 352x240 (NTSC)");
			mjpeg_error_exit1( "VCD high resolution stills must be 704x576 (PAL) or 704x480 (NTSC)");
		}
		seq_hdr_every_gop = 1;
		seq_end_every_gop = 1;
		min_GOP_size = 1;
		max_GOP_size = 1;
		break;

	case MPEG_FORMAT_SVCD_STILL :
		mpeg = 2;
		quant = 0;	/* We want to try and hit our size target */

		/* We choose a generous nominal bitrate as there's only one 
		   frame per sequence ;-). It *is* too small to fill the 
		   frame-buffer in less than one PAL/NTSC frame 
		   period though...
		*/
		bitrate = 2500000;
		video_buffer_size = 230;
		vbv_buffer_still_size = 220*1024;
		pad_stills_to_vbv_buffer_size = 0;

		/* Now we select normal/hi-resolution based on the input stream
		   resolution. 
		*/
		
		if( in_img_width == 480 && 
			(in_img_height == 480 || in_img_height == 576 ) )
		{
			mjpeg_info( "SVCD normal-resolution stills selected." );
			if( still_size == 0 )
				still_size = 90*1024;
		}
		else if( in_img_width == 704 &&
				 (in_img_height == 480 || in_img_height == 576) )
		{
			mjpeg_info( "SVCD high-resolution stills selected." );
			if( still_size == 0 )
				still_size = 125*1024;
		}
		else
		{
			mjpeg_error("SVCD normal resolution stills must be 480x576 (PAL) or 480x480 (NTSC)");
			mjpeg_error_exit1( "SVCD high resolution stills must be 704x576 (PAL) or 704x480 (NTSC)");
		}

		if( still_size < 30*1024 || still_size > 200*1024 )
		{
			mjpeg_error_exit1( "SVCD resolution stills must be >= 30KB and <= 200KB each");
		}


		seq_hdr_every_gop = 1;
		seq_end_every_gop = 1;
		min_GOP_size = 1;
		max_GOP_size = 1;
		break;

	case MPEG_FORMAT_DVD :
	case MPEG_FORMAT_DVD_NAV :
		mpeg = 2;
		if (bitrate == 0)
			bitrate = 7500000;
    	if (video_buffer_size == 0)
    		video_buffer_size = 230;
		if (!force_cbr && quant == 0)
			quant = 8;
		seq_hdr_every_gop = 1;
		break;
  
     case MPEG_FORMAT_ATSC720p :
     case MPEG_FORMAT_ATSC1080i :
         // Need a bigger pixel search radius for HD
         // images :-(
         if (searchrad == 0)
             searchrad = 32;
     case MPEG_FORMAT_ATSC480i :
     case MPEG_FORMAT_ATSC480p :
         mpeg = 2;
         video_buffer_size = 488;
         if (bitrate == 0)
             bitrate = 19400000;
 		if (!force_cbr && quant == 0)
 			quant = 8;
         
	}

    /*
     * At this point the command line arguments have been processed, the format (-f)
     * selection has had a chance to set the bitrate.  IF --cbr was used and we
     * STILL do not have a bitrate set then declare an error because a Constant
     * Bit Rate of 0 makes no sense (most of the time CBR doesn't either ... ;))
     */
     if (force_cbr && bitrate == 0)
        {
        nerr++;
        mjpeg_error("--cbr used but no bitrate set with -b or -f!");
        }


    switch( mpeg )
    {
    case 1 :
        if( min_GOP_size == -1 )
            min_GOP_size = 12;
        if( max_GOP_size == -1 )
            max_GOP_size = 12;
        break;
    case 2:
        if( max_GOP_size == -1 )
            max_GOP_size = (norm == 'n' ? 18 : 15);
        if( min_GOP_size == -1 )
            min_GOP_size = max_GOP_size/2;
        break;
    }
	if( svcd_scan_data == -1 )
		svcd_scan_data = 0;
    if (searchrad == 0)
        searchrad = 16;
	nerr += InferStreamDataParams(strm);
	nerr += CheckBasicConstraints();

	return nerr != 0;
}



/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
