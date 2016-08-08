#ifndef _MPEG2ENCPARAMS_H
#define _MPEG2ENCPARAMS_H

/* mpeg2encoptions.h - Encoding options for mpeg2enc++ MPEG-1/2
 * encoder library */
/*  (C) 2000/2001 Andrew Stevens */

/*  This Software is free software; you can redistribute it
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


struct MPEG2EncParams
{
    int in_img_width;       // MUST match the actual dimensions of the passed frames!
    int in_img_height;
    int display_hsize;      // Passed on to display extension header
    int display_vsize;      // not used in encoding.
    int level;              // MPEG-2 main profile level to enforce.
    int format;
    int bitrate;
    int target_bitrate;
    int nonvid_bitrate;
    unsigned int stream_frames;   	// # Frames of entire stream
    double stream_Xhi;	 	 		// debug feature total stream complexity...
    int quant;
    int searchrad;
    int mpeg;
    unsigned int aspect_ratio;
    unsigned int frame_rate;
    int fieldenc; /* 0: progressive, 
                     1 = frame pictures, 
                     interlace frames with field
                     MC and DCT in picture 
                     2 = field pictures
                  */
    int norm;  /* 'n': NTSC, 'p': PAL, 's': SECAM, else unspecified */
    int me44_red	;
    int me22_red	;	
    int hf_quant;
    double hf_q_boost;
    double act_boost;
    double boost_var_ceil;
    int rate_control;           /* The rate controller to use */
    int video_buffer_size;
    int seq_length_limit;
    int min_GOP_size;
    int max_GOP_size;
    int closed_GOPs;
    int preserve_B;
    int Bgrp_size;
    int num_cpus;
    int vid32_pulldown;
    int svcd_scan_data;
    int seq_hdr_every_gop;
    int seq_end_every_gop;
    int still_size;
    int pad_stills_to_vbv_buffer_size;
    int vbv_buffer_still_size;
    int force_interlacing;
    int input_interlacing;
    int hack_svcd_hds_bug;
    int hack_altscan_bug;
    int hack_dualprime;
    int mpeg2_dc_prec;
    int ignore_constraints;
    int unit_coeff_elim;
    int force_cbr;
    int verbose;
};

#endif

/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
