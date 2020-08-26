/* mpeg2enc.cc, YUV4MPEG / mjpegtools command line wrapper for
 * mpeg2enc++ library */

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
/* Modifications and enhancements (C) 2000/2001 Andrew Stevens */

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
#include <stdio.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <algorithm>

#include "mpeg2encoder.hh"
#include "mpeg2encoptions.hh"
#include "encoderparams.hh"
#include "picturereader.hh"
#include "imageplanes.hh"
#include "elemstrmwriter.hh"
#include "quantize.hh"
#include "ontheflyratectlpass1.hh"
#include "ontheflyratectlpass2.hh"
#include "seqencoder.hh"
#include "mpeg2coder.hh"
#include "format_codes.h"
#include "mpegconsts.h"

#ifdef HAVE_ALTIVEC
/* needed for ALTIVEC_BENCHMARK and print_benchmark_statistics() */
#include "../utils/altivec/altivec_conf.h"
#endif



/**************************
 *
 * Derived class for File (actually: pipe and FIFO) based bitstream output
 *
 *************************/



class FILE_StrmWriter : public ElemStrmWriter
{
public:


    FILE_StrmWriter( EncoderParams &encparams, const char *outfilename ) 
        {
            /* open output file */
            if (!(outfile=fopen(outfilename,"wb")))
            {
                mjpeg_error_exit1("Couldn't create output file %s",outfilename);
            }
        }

    virtual void WriteOutBufferUpto( const uint8_t *buffer, const uint32_t flush_upto )
        {
            size_t written = fwrite( buffer, 
                                     sizeof(uint8_t), 
                                     static_cast<size_t>(flush_upto),
                                     outfile );
            if( written != static_cast<size_t>(flush_upto) )
            {
                mjpeg_error_exit1( "%s", strerror(ferror(outfile)) );
            }
	       flushed += flush_upto;
        }

    virtual ~FILE_StrmWriter()
        {
            fclose( outfile );
        }
        
    virtual uint64_t BitCount() { return flushed * 8LL; }
private:
    FILE *outfile;
};




/**************************
 *
 * Derived class for File (actually: pipe and FIFO) based input of frames
 * in the YUV4MPEG2 format
 *
 *************************/

class Y4MPipeReader : public PictureReader
{
public:

    Y4MPipeReader( EncoderParams &encparams, int istrm_fd );
    ~Y4MPipeReader();

    void StreamPictureParams( MPEG2EncInVidParams &strm );
protected:
    bool LoadFrame( ImagePlanes &image );
private:
    int PipeRead(  uint8_t *buf, int len);

    int pipe_fd;
    y4m_stream_info_t _si;
    y4m_frame_info_t _fi;
};
 

Y4MPipeReader::Y4MPipeReader( EncoderParams &encparams, int istrm_fd ) :
    PictureReader( encparams ),
    pipe_fd( istrm_fd )
{
    y4m_init_stream_info(&_si);
    y4m_init_frame_info(&_fi);
}


Y4MPipeReader::~Y4MPipeReader()
{
    y4m_fini_stream_info(&_si);
    y4m_fini_frame_info(&_fi);
}


/****************************************
 *
 * Initialise the reader and return the parameter of the video stream
 * to be encoded.
 *
 * WARNING: This routine must run before encoder parameters are defined.
 * TODO: Reader should be constructed before EncoderParams and this
 * routine invoked from EncoderParams constructor...
 *
 *****************************************/

void Y4MPipeReader::StreamPictureParams( MPEG2EncInVidParams &strm )
{
   int n;
   y4m_ratio_t sar;

   if ((n = y4m_read_stream_header (pipe_fd, &_si)) != Y4M_OK) {
       mjpeg_error("Could not read YUV4MPEG2 header: %s!", y4m_strerr(n));
      exit (1);
   }

   strm.horizontal_size = y4m_si_get_width(&_si);
   strm.vertical_size = y4m_si_get_height(&_si);
   strm.frame_rate_code = mpeg_framerate_code(y4m_si_get_framerate(&_si));
   strm.interlacing_code = y4m_si_get_interlace(&_si);

   /* Deduce MPEG aspect ratio from stream's frame size and SAR...
      (always as an MPEG-2 code; that's what caller expects). */
   sar = y4m_si_get_sampleaspect(&_si);
   strm.aspect_ratio_code = 
       mpeg_guess_mpeg_aspect_code(2, sar, 
                                   strm.horizontal_size, 
                                   strm.vertical_size);
   if(strm.horizontal_size <= 0)
   {
       mjpeg_error_exit1("Horizontal size from input stream illegal");
   }
   if(strm.vertical_size <= 0)
   {
       mjpeg_error("Vertical size from input stream illegal");
   }
}

/*****************************
 *
 * LoadFrame - pull in the image data planes from pipe (planar YUV420)
 *
 * RETURN: true iff EOF or ERROR
 *
 ****************************/

bool Y4MPipeReader::LoadFrame( ImagePlanes &image )
{
   int h,v,y;


   if ((y = y4m_read_frame_header (pipe_fd, &_si, &_fi)) != Y4M_OK) 
   {
       if( y != Y4M_ERR_EOF )
           mjpeg_warn("Error reading frame header (%d): code%s!", 
                      frames_read, y4m_strerr (y));
       return true;
      }
      
   v = encparams.vertical_size;
   h = encparams.horizontal_size;
   int i;
   for(i=0;i<v;i++)
   {
       if( PipeRead(image.Plane(0)+i*encparams.phy_width,h)!=h)
           return true;
   }

   v = encparams.vertical_size/2;
   h = encparams.horizontal_size/2;
   for(i=0;i<v;i++)
   {
       if(PipeRead(image.Plane(1)+i*encparams.phy_chrom_width,h)!=h)
           return true;
   }
   for(i=0;i<v;i++)
   {
       if(PipeRead(image.Plane(2)+i*encparams.phy_chrom_width,h)!=h)
           return true;
   }
   return false;
}



int Y4MPipeReader::PipeRead(uint8_t *buf, int len)
{
   int n, r;

   r = 0;

   while(r<len)
   {
      n = read(pipe_fd,buf+r,len-r);
      if(n==0) return r;
      r += n;
   }
   return r;
}



/**************************
 *
 * Derived class for options set from command line
 *
 *************************/

class MPEG2EncCmdLineOptions : public MPEG2EncOptions
{
public:
    MPEG2EncCmdLineOptions();
    int SetFromCmdLine(int argc, char *argv[] );
    void Usage();
    void StartupBanner();
    void SetFormatPresets( const MPEG2EncInVidParams &strm );
private:
    void DisplayFrameRates();
    void DisplayAspectRatios();
    int ParseCustomMatrixFile(const char *fname, int dbug);
    void ParseCustomOption(const char *arg);
public:
    int istrm_fd;
    char *outfilename;

};

void MPEG2EncCmdLineOptions::SetFormatPresets( const MPEG2EncInVidParams &strm )
{
    if( MPEG2EncOptions::SetFormatPresets( strm ) )
        Usage();
}

MPEG2EncCmdLineOptions::MPEG2EncCmdLineOptions() :
    MPEG2EncOptions()
{
    outfilename = 0;
    istrm_fd = 0;
        
}

void MPEG2EncCmdLineOptions::DisplayFrameRates(void)
{
 	unsigned int i;
	printf("Frame-rate codes:\n");
    for( i = 1; mpeg_valid_framerate_code(i); ++i )
	{
		printf( "%2d - %s\n", i, mpeg_framerate_code_definition(i));
	}
	exit(0);
}

void MPEG2EncCmdLineOptions::DisplayAspectRatios(void)
{
 	unsigned int i;
	printf("\nDisplay aspect ratio codes:\n");
	for( i = 1; mpeg_valid_aspect_code(2, i); ++i )
	{
		printf( "%2d - %s\n", i, mpeg_aspect_code_definition(2,i));
	}
	exit(0);
}

int MPEG2EncCmdLineOptions::ParseCustomMatrixFile(const char *fname, int dbug)
{
    FILE  *fp;
    uint16_t  q[128];
    int  i, j, row;
    char line[80];

    fp = fopen(fname, "r");
    if  (!fp)
    {
        mjpeg_error("can not open custom matrix file '%s'", fname);
        return(-1);
    }

    row = i = 0;
    while   (fgets(line, sizeof(line), fp))
    {
        row++;
        /* Empty lines (\n only) and comments are ignored */
        if  ((strlen(line) == 1) || line[0] == '#')
            continue;
        j = sscanf(line, "%hu,%hu,%hu,%hu,%hu,%hu,%hu,%hu\n",
                   &q[i+0], &q[i+1], &q[i+2], &q[i+3],
                   &q[i+4], &q[i+5], &q[i+6], &q[i+7]);
        if  (j != 8)
        {
            mjpeg_error("line %d ('%s') of '%s' malformed", row, line, fname);
            break;
        }
        for (j = 0; j < 8; j++)
        {
            if  (q[i + j] < 1 || q[i + j] > 255)
            {
                mjpeg_error("entry %d (%u) in line %d from '%s' invalid",
                            j, q[i + j], row, fname);
                i = -1;
                break;
            }
        }
        i += 8;
    }

    fclose(fp);

    if  (i != 128)
    {
        mjpeg_error("file '%s' did NOT have 128 values - ignoring custom matrix file", fname);
        return(-1);
    }

    for (j = 0; j < 64; j++)
    {
        custom_intra_quantizer_matrix[j] = q[j];
        custom_nonintra_quantizer_matrix[j] = q[j + 64];
    }

    if  (dbug)
    {
        mjpeg_info("INTRA and NONINTRA tables from '%s'",fname);
        for (j = 0; j < 128; j += 8)
        {
            mjpeg_info("%u %u %u %u %u %u %u %u", 
                       q[j + 0], q[j + 1], q[j + 2], q[j + 3], 
                       q[j + 4], q[j + 5], q[j + 6], q[j + 7]);
        }
    }
    return(0);
}

void MPEG2EncCmdLineOptions::ParseCustomOption(const char *arg)
{
    if	(strcmp(arg, "kvcd") == 0)
    	hf_quant = 3;
    else if (strcmp(arg, "hi-res") == 0)
    	hf_quant = 2;
    else if (strcmp(arg, "default") == 0)
    {
    	hf_quant = 0;
    	hf_q_boost = 0;
    }
    else if (strcmp(arg, "tmpgenc") == 0)
    	hf_quant = 4;
    else if (strcmp(arg, "flat") == 0)
    	hf_quant = 5;
    else if (strncasecmp(arg, "file=", 5) == 0)
    {
    	if  (ParseCustomMatrixFile(arg + 5, arg[0] == 'F' ? 1 : 0) == 0)
    	    hf_quant = 6;
    }
    else if (strcmp(arg, "help") == 0)
    {
    	fprintf(stderr, "Quantization matrix names:\n\n");
    	fprintf(stderr, "\thelp - this message\n");
    	fprintf(stderr, "\tkvcd - matrices from http://www.kvcd.net\n");
    	fprintf(stderr, "\thi-res - high resolution tables (same as -H)\n");
    	fprintf(stderr, "\tdefault - turn off -N or -H (use standard tables)\n");
    	fprintf(stderr, "\ttmpgenc - TMPGEnc tables (http://www.tmpgenc.com)\n");
    	fprintf(stderr, "\tflat - flat tables for high bitrate encoding\n");
    	fprintf(stderr, "\tfile=filename - filename contains custom matrices\n");
    	fprintf(stderr, "\t\t8 comma separated values per line.  8 lines per matrix, INTRA matrix first, then NONINTRA\n");
    	exit(0);
    }
    else
    	mjpeg_error_exit1("Unknown type '%s' used with -K/--custom-quant", arg);
}

void MPEG2EncCmdLineOptions::Usage()
{
	fprintf(stderr,
"--verbose|-v num\n" 
"    Level of verbosity. 0 = quiet, 1 = normal 2 = verbose/debug\n"
"--format|-f fmt\n"
"    Encoding profile\n"
"    [0 = Generic MPEG1, 1 = standard VCD, 2 = user VCD,\n"
"     3 = Generic MPEG2, 4 = standard SVCD, 5 = user SVCD,\n"
"     6 = VCD Stills sequences, 7 = SVCD Stills sequences, 8|9 = DVD,\n"
"     10 = ATSC 480i, 11 = ATSC 480p, 12 = ATSC 720p, 13 = ATSC 1080i]\n"
"--display-hsize|-x [32..16383]\n"
"   Set the the display-horizontal-size hint in MPEG-2 output to\n"
"   something other than the encoded image width\n"
"--display-vsize|-y [32..16383]\n"
"   Set the the display-vertical-size hint in MPEG-2 output to\n"
"   something other than the encoded image height\n" 
"--aspect|-a num\n"
"    Set displayed image aspect ratio image (default: 2 = 4:3)\n"
"    [1 = 1:1, 2 = 4:3, 3 = 16:9, 4 = 2.21:1]\n"
"--frame-rate|-F num\n"
"    Set playback frame rate of encoded video\n"
"    (default: frame rate of input stream)\n"
"    0 = Display frame rate code table\n"
"--video-bitrate|-b num\n"
"    Set Bitrate / peak bitrate of compressed video in KBit/sec\n"
"    (Peak bitrate if a target bitrate and/or quantisation floor is set\n"
"    (default: 1152.0 for VCD, 2500.0 for SVCD, 7500.0 for DVD)\n"
"--target-video-bitrate|-t\n"
"   Set target bitrate for entire video stream in KBit/sec\n"
"--nonvideo-bitrate|-B num\n"
"    Non-video data bitrate to assume for sequence splitting\n"
"    calculations (see also --sequence-length).\n"
"--quantisation|-q num\n"
"    Image data quantisation factor [1..31] (1 is best quality, no default)\n"
"    When quantisation is set variable bit-rate encoding is activated and\n"
"    the --bitrate value sets an *upper-bound* video data-rate\n"
"--ratecontroller|-A [0..1] (default:0)\n"
"    Specify ratecontrol alorithm\n"
"--output|-o pathname\n"
"    Pathname of output file or fifo (REQUIRED!!!)\n"
"--target-still-size|-T size\n"
"    Size in KB of VCD stills\n"
"--interlace-mode|-I num\n"
"    Sets MPEG 2 motion estimation and encoding modes:\n"
"    0 = Progressive (non-interlaced)(Movies)\n"
"    1 = Interlaced source material (video)\n"
"    2 = Interlaced source material, per-field-encoding (video)\n"
"--motion-search-radius|-r num\n"
"    Motion compensation search radius [0..32] (default 16)\n"
"--reduction-4x4|-4 num\n"
"    Reduction factor for 4x4 subsampled candidate motion estimates\n"
"    [1..4] [1 = max quality, 4 = max. speed] (default: 2)\n"
"--reduction-2x2|-2 num\n"
"    Reduction factor for 2x2 subsampled candidate motion estimates\n"
"    [1..4] [1 = max quality, 4 = max. speed] (default: 3)\n"
"--min-gop-size|-g num\n"
"    Minimum size Group-of-Pictures (default depends on selected format)\n"
"--max-gop-size|-G num\n"
"    Maximum size Group-of-Pictures (default depends on selected format)\n"
"    If min-gop is less than max-gop, mpeg2enc attempts to place GOP\n"
"    boundaries to coincide with scene changes\n"
"--closed-gop|-c\n"
"    All Group-of-Pictures are closed.  Useful for authoring multi-angle DVD\n"
"--force-b-b-p|-P\n"
"    Preserve two B frames between I/P frames when placing GOP boundaries\n"
"--quantisation-reduction|-Q num\n"
"    Max. quantisation reduction for highly active blocks\n"
"    [0.0 .. 4.0] (default: 0.0)\n"
"--quant-reduction-max-var|-X num\n"
"    Luma variance below which quantisation boost (-Q) is used\n"
"    [0.0 .. 2500.0](default: 0.0)\n"
"--video-buffer|-V num\n"
"    Target decoders video buffer size in KB (default 46)\n"
"--video-norm|-n n|p|s\n"
"    Tag output to suit playback in specified video norm\n"
"    (n = NTSC, p = PAL, s = SECAM) (default: PAL)\n"
"--sequence-length|-S num\n"
"    Place a sequence boundary in the video stream so they occur every\n"
"    num Mbytes once the video is multiplexed with audio etc.\n"
"    N.b. --non-video-bitrate is used to the bitrate of the other\n"
"    data that will be multiplexed with this video stream\n"
"--3-2-pulldown|-p\n"
"    Generate header flags for 3-2 pull down of 24fps movie material\n"
"--intra_dc_prec|-D [8..11]\n"
"    Set number of bits precision for DC (base colour) of blocks in MPEG-2\n"
"--reduce-hf|-N num\n"
"    [0.0..2.0] Reduce hf resolution (increase quantization) by num (default: 0.0)\n"
"--keep-hf|-H\n"
"    Maximise high-frequency resolution - useful for high quality sources\n"
"    and/or high bit-rates)\n"
"--sequence-header-every-gop|-s\n"
"    Include a sequence header every GOP if the selected format doesn't\n"
"    do so by default.\n"
"--no-dummy-svcd-SOF|-d\n"
"    Do not generate dummy SVCD scan-data for the ISO CD image\n"
"    generator \"vcdimager\" to fill in.\n"
"--playback-field-order|-z b|t\n"
"    Force setting of playback field order to bottom or top first\n"
"--multi-thread|-M num\n"
"    Activate multi-threading to optimise throughput on a system with num CPU's\n"
"    [0..32], 0=no multithreading, (default: 0)\n"
"--correct-svcd-hds|-C\n"
"    Force SVCD horizontal_display_size to be 480 - standards say 540 or 720\n"
"    But many DVD/SVCD players screw up with these values.\n"
"--no-constraints\n"
"    Deactivate constraints for maximum video resolution and sample rate.\n"
"    Could expose bugs in the software at very high resolutions!\n"
"--no-altscan-mpeg2\n"
"    Deactivate the use of the alternate block pattern for MPEG-2.  This is\n"
"    A work-around for a Bug in an obscure hardware decoder.\n"
"--dualprime-mpeg2\n"
"    Turn ON use of dual-prime motion compensation. Default is OFF unless this option is used\n"
"--custom-quant-matrices|-K kvcd|tmpgenc|default|hi-res|file=inputfile|help\n"
"    Request custom or userspecified (from a file) quantization matrices\n"
"--unit-coeff-elim|-E num\n"
"    Skip picture blocks which appear to carry little information\n"
"    because they code to only unit coefficients. The number specifies\n"
"    how aggresively this should be done. A negative value means DC\n"
"    coefficients are included.  Reasonable values -40 to 40\n"
"--b-per-refframe| -R 0|1|2\n"
"    The number of B frames to generate between each I/P frame\n"
"--cbr|-u\n"
"    For MPEG-2 force the use of (suboptimal) ConstantBitRate (CBR) encoding\n"
"--chapters X[,Y[,...]]\n"
"    Specifies which frames should be chapter points (first frame is 0)\n"
"    Chapter points are I frames on closed GOP's.\n"
"--help|-?\n"
"    Print this lot out!\n"
	);
	exit(0);
}


void MPEG2EncCmdLineOptions::StartupBanner()
{
	mjpeg_info("Encoding MPEG-%d video to %s",mpeg, outfilename);
	mjpeg_info("Horizontal size: %d pel",in_img_width);
	mjpeg_info("Vertical size: %d pel",in_img_height);
	mjpeg_info("Aspect ratio code: %d = %s", 
               aspect_ratio,
               mpeg_aspect_code_definition(mpeg,aspect_ratio));
	mjpeg_info("Frame rate code:   %d = %s",
               frame_rate,
               mpeg_framerate_code_definition(frame_rate));

	if(bitrate) 
		mjpeg_info("Bitrate: %d KBit/s",bitrate/1000);
	else
		mjpeg_info( "Bitrate: VCD");
	if(quant) 
		mjpeg_info("Quality factor: %d (Quantisation = %.0f) (1=best, 31=worst)",
                   quant, 
                   RateCtl::InvScaleQuant( mpeg == 1 ? 0 : 1, quant)
            );

	mjpeg_info("Field order for input: %s", 
			   mpeg_interlace_code_definition(input_interlacing) );

	if( seq_length_limit )
	{
		mjpeg_info( "New Sequence every %d Mbytes", seq_length_limit );
		mjpeg_info( "Assuming non-video stream of %d Kbps", nonvid_bitrate );
	}
	else
		mjpeg_info( "Sequence unlimited length" );

	mjpeg_info("Search radius: %d",searchrad);
	if (mpeg == 2)
           {
           mjpeg_info("DualPrime: %s", hack_dualprime == 1 ? "yes" : "no");
           }
}


int MPEG2EncCmdLineOptions::SetFromCmdLine( int argc,	char *argv[] )
{
	int n;
	int nerr = 0;

	enum LongOnlyOptions
	{
		CHAPTERS = 256
	};
static const char   short_options[]=
        "l:a:f:x:y:n:b:z:T:B:q:o:S:I:r:M:4:2:A:Q:X:D:g:G:v:V:F:N:updsHcCPK:E:R:t:L:Z:";

#ifdef HAVE_GETOPT_LONG

static struct option long_options[]=
    {
        { "verbose",           1, 0, 'v' },
        { "format",            1, 0, 'f' },
        { "level",             1, 0, 'l' },
        { "aspect",            1, 0, 'a' },
        { "display-hsize",     1, 0, 'x' },
        { "display-vsize",     1, 0, 'y' },
        { "frame-rate",        1, 0, 'F' },
        { "video-bitrate",     1, 0, 'b' },
        { "target-video-bitrate", 1, 0, 't' },
        { "sequence_length",   1, 0, 'L' },
        { "mean-complexity",	1, 0, 'Z' },
        { "nonvideo-bitrate",  1, 0, 'B' },
        { "intra_dc_prec",     1, 0, 'D' },
        { "quantisation",      1, 0, 'q' },
        { "output",            1, 0, 'o' },
        { "target-still-size", 1, 0, 'T' },
        { "interlace-mode",    1, 0, 'I' },
        { "motion-search-radius", 1, 0, 'r'},
        { "reduction-4x4",  1, 0, '4'},
        { "reduction-2x2",  1, 0, '2'},
        { "min-gop-size",      1, 0, 'g'},
        { "max-gop-size",      1, 0, 'G'},
        { "closed-gop",        0, 0, 'c'},
        { "force-b-b-p",       0, 0, 'P'},
        { "ratecontroller", 1, 0, 'A' },
        { "quantisation-reduction", 1, 0, 'Q' },
        { "quant-reduction-max-var", 1, 0, 'X' },
        { "video-buffer",      1, 0, 'V' },
        { "video-norm",        1, 0, 'n' },
        { "sequence-length",   1, 0, 'S' },
        { "3-2-pulldown",      0, 0, 'p'},
        { "keep-hf",           0, 0, 'H' },
        { "reduce-hf",         1, 0, 'N' },
        { "sequence-header-every-gop", 0, 0, 's'},
        { "no-dummy-svcd-SOF", 0, 0, 'd' },
        { "correct-svcd-hds", 0, 0, 'C'},
        { "no-constraints", 0, &ignore_constraints, 1},
        { "no-altscan-mpeg2", 0, &hack_altscan_bug, 1},
        { "dualprime-mpeg2", 0, &hack_dualprime, 1},
        { "playback-field-order", 1, 0, 'z'},
        { "multi-thread",      1, 0, 'M' },
        { "custom-quant-matrices", 1, 0, 'K'},
        { "unit-coeff-elim",   1, 0, 'E'},
        { "b-per-refframe",    1, 0, 'R' },
        { "cbr",               0, 0, 'u'},
        { "help",              0, 0, '?' },
        { "chapters",          1, 0, CHAPTERS },
        { 0,                   0, 0, 0 }
    };

while( (n=getopt_long(argc,argv,short_options,long_options, NULL)) != -1 )
#else
while( (n=getopt(argc,argv,short_options)) != -1)
#endif
{
    switch(n)
    {
    case 0 :                /* Flag setting handled by getopt-long */
        break;
    case 'l' :          /* MPEG-2 level */
        if( strcmp( optarg, "high" ) == 0 || strcmp( optarg, "h" ) == 0 )
            level = HIGH_LEVEL;
        else if( strcmp( optarg, "main") == 0 || strcmp( optarg, "m" ) == 0 )
            level = MAIN_LEVEL;
        else
        {
            mjpeg_error( "Level must be 'main', 'm', 'high' or 'h'");
            ++nerr;
        }
        break;
    case 'b':
        bitrate = static_cast<int>(atof(optarg)*1000);
        if( bitrate % 400 != 0 )
        {
            mjpeg_warn( "MPEG bitrate must be a multiple of 400 - rounding up" );
            bitrate = (bitrate / 400 + 1) * 400;
        }
        break;

    case 't':
        target_bitrate = static_cast<int>(atof(optarg)*1000);
        break;
    case 'L' :
    	stream_frames = atoi(optarg);
    	break;
    case 'Z' :
    	stream_Xhi = atof(optarg);
    	if( stream_Xhi < 1000000.0 )
    	{
    		mjpeg_error( "-Z|mean_complexity fails sanity check (< 1000000.0)");
    		++nerr;
    	}
    	break;
    case 'T' :
        still_size = atoi(optarg)*1024;
        if( still_size < 20*1024 || still_size > 500*1024 )
        {
            mjpeg_error( "-T requires arg 20..500" );
            ++nerr;
        }
        break;
    case 'B':
        nonvid_bitrate = atoi(optarg);
        if( nonvid_bitrate < 0 )
        {
            mjpeg_error("-B requires arg > 0");
            ++nerr;
        }
        break;
    case 'D':
        mpeg2_dc_prec = atoi(optarg)-8;
        if( mpeg2_dc_prec < 0 || mpeg2_dc_prec > 3 )
        {
            mjpeg_error( "-D requires arg [8..11]" );
            ++nerr;
        }
        break;
    case 'C':
        hack_svcd_hds_bug = 0;
        break;

    case 'q':
        quant = atoi(optarg);
        if(quant<1 || quant>32)
        {
            mjpeg_error("-q option requires arg 1 .. 32");
            ++nerr;
        }
        break;

    case 'x' :
        display_hsize = atoi(optarg);
        if( display_hsize < 32 || display_hsize >= 16384 )
        {
            mjpeg_error( "-x option must be in range [32..16383]" );
            ++nerr;
        }
        break;
    case 'y' :
        display_vsize = atoi(optarg);
        if( display_vsize < 32 || display_vsize >= 16384 )
        {
            mjpeg_error( "-y option must be in range [32..16383]" );
            ++nerr;
        }
        break;
    case 'a' :
        aspect_ratio = atoi(optarg);
        if( aspect_ratio == 0 )
            DisplayAspectRatios();
        /* Checking has to come later once MPEG 1/2 has been selected...*/
        if( aspect_ratio < 0 )
        {
            mjpeg_error( "-a option must be positive");
            ++nerr;
        }
        break;

    case 'F' :
        frame_rate = atoi(optarg);
        if( frame_rate == 0 )
            DisplayFrameRates();
        if( !mpeg_valid_framerate_code(frame_rate) )
        {
            mjpeg_error( "illegal -F value (use -F 0 to list options)" );
            ++nerr;
        }
        break;

    case 'o':
        outfilename = optarg;
        break;

    case 'I':
        fieldenc = atoi(optarg);
        if( fieldenc < 0 || fieldenc > 2 )
        {
            mjpeg_error("-I option requires 0,1 or 2");
            ++nerr;
        }
        break;

    case 'r':
        searchrad = atoi(optarg);
        if(searchrad<0 || searchrad>32)
        {
            mjpeg_error("-r option requires arg 0 .. 32");
            ++nerr;
        }
        break;

    case 'M':
        num_cpus = atoi(optarg);
        if(num_cpus<0 || num_cpus>32)
        {
            mjpeg_error("-M option requires arg 0..32");
            ++nerr;
        }
        break;

    case '4':
        me44_red = atoi(optarg);
        if(me44_red<0 || me44_red>4)
        {
            mjpeg_error("-4 option requires arg 0..4");
            ++nerr;
        }
        break;

    case '2':
        me22_red = atoi(optarg);
        if(me22_red<0 || me22_red>4)
        {
            mjpeg_error("-2 option requires arg 0..4");
            ++nerr;
        }
        break;

    case 'v':
        verbose = atoi(optarg);
        if( verbose < 0 || verbose > 2 )
            ++nerr;
        break;
    case 'V' :
        video_buffer_size = atoi(optarg);
        if(video_buffer_size<20 || video_buffer_size>4000)
        {
            mjpeg_error("-v option requires arg 20..4000");
            ++nerr;
        }
        break;

    case 'S' :
        seq_length_limit = atoi(optarg);
        if(seq_length_limit<1 )
        {
            mjpeg_error("-S option requires arg > 1");
            ++nerr;
        }
        break;
    case 'p' :
        vid32_pulldown = 1;
        break;

    case 'z' :
        if( strlen(optarg) != 1 || (optarg[0] != 't' && optarg[0] != 'b' ) )
        {
            mjpeg_error("-z option requires arg b or t" );
            ++nerr;
        }
        else if( optarg[0] == 't' )
            force_interlacing = Y4M_ILACE_TOP_FIRST;
        else if( optarg[0] == 'b' )
            force_interlacing = Y4M_ILACE_BOTTOM_FIRST;
        break;

    case 'f' :
        format = atoi(optarg);
        if( format < MPEG_FORMAT_FIRST ||
                format > MPEG_FORMAT_LAST )
        {
            mjpeg_error("-f option requires arg [%d..%d]",
                        MPEG_FORMAT_FIRST, MPEG_FORMAT_LAST);
            ++nerr;
        }

        break;

    case 'n' :
        switch( optarg[0] )
        {
        case 'p' :
        case 'n' :
        case 's' :
            norm = optarg[0];
            break;
        default :
            mjpeg_error("-n option requires arg n or p, or s.");
            ++nerr;
        }
        break;
    case 'g' :
        min_GOP_size = atoi(optarg);
        break;
    case 'G' :
        max_GOP_size = atoi(optarg);
        break;
    case 'c' :
        closed_GOPs = true;
        break;
    case 'P' :
        preserve_B = true;
        break;
    case 'N':
        hf_q_boost = atof(optarg);
        if (hf_q_boost <0.0 || hf_q_boost > 2.0)
        {
            mjpeg_error( "-N option requires arg 0.0 .. 2.0" );
            ++nerr;
            hf_q_boost = 0.0;
        }
        if (hf_quant == 0 && hf_q_boost != 0.0)
            hf_quant = 1;
        break;
    case 'H':
        hf_quant = 2;
        break;
    case 'K':
        ParseCustomOption(optarg);
        break;

    case 'u':
        force_cbr = 1;
        break;

    case 'E':
        unit_coeff_elim = atoi(optarg);
        if (unit_coeff_elim < -40 || unit_coeff_elim > 40)
        {
            mjpeg_error( "-E option range arg -40 to 40" );
            ++nerr;
        }
        break;
    case 'R' :
        Bgrp_size = atoi(optarg)+1;
        if( Bgrp_size<1 || Bgrp_size>3)
        {
            mjpeg_error( "-R option arg 0|1|2" );
            ++nerr;
        }
        break;
    case 's' :
        seq_hdr_every_gop = 1;
        break;
    case 'd' :
        svcd_scan_data = 0;
        break;
    case 'A' :
        rate_control = atoi(optarg);
        if( rate_control < 0 || rate_control > 1 )
        {
            mjpeg_error( "-A option requires arg [0,1]");
            ++nerr;
        }
        break;
    case 'Q' :
        act_boost = atof(optarg);
        if( act_boost < 0.0 || act_boost > 4.0)
        {
            mjpeg_error( "-Q option requires arg 0.0 .. 4.0");
            ++nerr;
        }
        break;
    case 'X' :
        boost_var_ceil = atof(optarg);
        if( boost_var_ceil <0 || boost_var_ceil > 50*50 )
        {
            mjpeg_error( "-X option requires arg 0 .. 2500" );
            ++nerr;
        }
        break;
    case 256: // --chapters=X
        for( char *x=strtok(optarg,","); x; x=strtok(0,",") )
            chapter_points.push_back(atoi(x));
        std::sort(chapter_points.begin(),chapter_points.end());
        break;
    case ':' :
        mjpeg_error( "Missing parameter to option!" );
    case '?':
    default:
        ++nerr;
    }
}

/* Select input stream */
if(optind!=argc)
{
    if( optind == argc-1 )
    {
        istrm_fd = open( argv[optind], O_RDONLY );
        if( istrm_fd < 0 )
        {
            mjpeg_error( "Unable to open: %s: ",argv[optind] );
            perror("");
            ++nerr;
        }
    }
    else
        ++nerr;
}
else
    istrm_fd = 0; /* stdin */

if(!outfilename)
{
    mjpeg_error("Output file name (-o option) is required!");
    ++nerr;
}

/*
 * Probably not necessary but err on the safe side.  If someone wants to
 * waste space by using a Constant Bit Rate stream then disable the '-q'
 * parameter.  Further checks for CBR are made in mpeg2encoptions.cc 
*/
if (force_cbr != 0)
    quant = 0;

return nerr;
}




class YUV4MPEGEncoder : public MPEG2Encoder
{
public:
    YUV4MPEGEncoder( MPEG2EncCmdLineOptions &options );
    void Encode();
};


YUV4MPEGEncoder::YUV4MPEGEncoder( MPEG2EncCmdLineOptions &cmd_options ) :
    MPEG2Encoder( cmd_options )
{
    reader = new Y4MPipeReader( parms, cmd_options.istrm_fd );
    MPEG2EncInVidParams strm;


	reader->StreamPictureParams(strm);
	cmd_options.SetFormatPresets( strm );
    cmd_options.StartupBanner();

    writer = new FILE_StrmWriter( parms, cmd_options.outfilename );
    quantizer = new Quantizer( parms );
    
    if( cmd_options.rate_control == 0 )
    {
        mjpeg_info( "Using one-pass rate controller" );
        pass1ratectl = new OnTheFlyPass1( parms );
        pass2ratectl = new OnTheFlyPass2( parms );
    }
    else
    {
        mjpeg_info( "Using statistical look-ahead/two-pass rate controller" );
#if 0
        pass1ratectl = new VBufPass1RC( parms );
        pass2ratectl = new XhiPass2RC( parms );
#else
        mjpeg_info( "Still needs updating to new interface!" );
        abort();
#endif
    }

    seqencoder = new SeqEncoder( parms, *reader, *quantizer,
                                 *writer,
                                 *pass1ratectl,
                                 *pass2ratectl
                                );

    // This order is important! Don't change...
    parms.Init( options );
    reader->Init();
    quantizer->Init();
    seqencoder->Init();

}

void YUV4MPEGEncoder::Encode( )
{
    seqencoder->EncodeStream();
}

int main( int argc, char *argv[] )
{
    MPEG2EncCmdLineOptions options;

    mjpeg_default_handler_verbosity(options.verbose);

    if( options.SetFromCmdLine( argc, argv ) != 0 )
		options.Usage();
    mjpeg_default_handler_verbosity(options.verbose);

    YUV4MPEGEncoder encoder( options );
    encoder.Encode();

#ifdef OUTPUT_STAT
	if( statfile != NULL )
		fclose(statfile);
#endif
#ifdef ALTIVEC_BENCHMARK
	print_benchmark_statistics();
#endif
	return 0;
}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
