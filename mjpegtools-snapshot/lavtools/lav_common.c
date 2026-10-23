/* lav_common - some general utility functionality used by multiple
	lavtool utilities. */

/* Copyright (C) 2000, Rainer Johanni, Andrew Stevens */
/* - added scene change detection code 2001, pHilipp Zabel */
/* - broke some code out to lav_common.h and lav_common.c 
 *   July 2001, Shawn Sulma <lavtools@athos.cx>.  In doing this,
 *   I replaced the large number of globals with a handful of structs
 *   that are passed into the appropriate methods.  Check lav_common.h
 *   for the structs.  I'm sure some of what I've done is inefficient,
 *   subtly incorrect or just plain Wrong.  Feedback is welcome.
 */
/* - removed a lot of subsumed functionality and unnecessary cruft
 *   March 2002, Matthew Marjanovic <maddog@mir.com>.
 */


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

#include <config.h>
#include <stdio.h>
#include <string.h>

#include "lav_common.h"
#include "jpegutils.h"
#include "mpegconsts.h"
#include "cpu_accel.h"

static uint8_t jpeg_data[MAX_JPEG_LEN];


#ifdef HAVE_LIBDV

dv_decoder_t *decoder;
uint16_t pitches[3];
uint8_t *dv_frame[3] = {NULL,NULL,NULL};

/*
 * As far as I (maddog) can tell, this is what is going on with libdv-0.9
 *  and the unpacking routine... 
 *    o Ft/Fb refer to top/bottom scanlines (interleaved) --- each sample
 *       is implicitly tagged by 't' or 'b' (samples are not mixed between
 *       fields)
 *    o Indices on Cb and Cr samples indicate the Y sample with which
 *       they are co-sited.
 *    o '^' indicates which samples are preserved by the unpacking
 *
 * libdv packs both NTSC 4:1:1 and PAL 4:2:0 into a common frame format of
 *  packed 4:2:2 pixels as follows:
 *
 *
 ***** NTSC 4:1:1 *****
 *
 *   libdv's 4:2:2-packed representation  (chroma repeated horizontally)
 *
 *Ft Y00.Cb00.Y01.Cr00.Y02.Cb00.Y03.Cr00 Y04.Cb04.Y05.Cr04.Y06.Cb04.Y07.Cr04
 *    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 *Fb Y00.Cb00.Y01.Cr00.Y02.Cb00.Y03.Cr00 Y04.Cb04.Y05.Cr04.Y06.Cb04.Y07.Cr04
 *    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 *Ft Y10.Cb10.Y11.Cr10.Y12.Cb10.Y13.Cr10 Y14.Cb14.Y15.Cr14.Y16.Cb14.Y17.Cr14
 *    ^        ^        ^        ^        ^        ^        ^        ^    
 *Fb Y10.Cb10.Y11.Cr10.Y12.Cb10.Y13.Cr10 Y14.Cb14.Y15.Cr14.Y16.Cb14.Y17.Cr14
 *    ^        ^        ^        ^        ^        ^        ^        ^    
 *
 *    lavtools unpacking into 4:2:0-planar  (note lossiness)
 *
 *Ft  Y00.Y01.Y02.Y03.Y04.Y05.Y06.Y07...
 *Fb  Y00.Y01.Y02.Y03.Y04.Y05.Y06.Y07...
 *Ft  Y10.Y11.Y12.Y13.Y14.Y15.Y16.Y17...
 *Fb  Y10.Y11.Y12.Y13.Y14.Y15.Y16.Y17...
 *
 *Ft  Cb00.Cb00.Cb04.Cb04...    Cb00,Cb04... are doubled
 *Fb  Cb00.Cb00.Cb04.Cb04...    Cb10,Cb14... are ignored
 *
 *Ft  Cr00.Cr00.Cr04.Cr04...
 *Fb  Cr00.Cr00.Cr04.Cr04...
 *
 * 
 ***** PAL 4:2:0 *****
 *
 *   libdv's 4:2:2-packed representation  (chroma repeated vertically)
 *
 *Ft Y00.Cb00.Y01.Cr10.Y02.Cb02.Y03.Cr12 Y04.Cb04.Y05.Cr14.Y06.Cb06.Y07.Cr16
 *    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 *Fb Y00.Cb00.Y01.Cr10.Y02.Cb02.Y03.Cr12 Y04.Cb04.Y05.Cr14.Y06.Cb06.Y07.Cr16
 *    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 *Ft Y10.Cb00.Y11.Cr10.Y12.Cb02.Y13.Cr12 Y14.Cb04.Y15.Cr14.Y16.Cb06.Y17.Cr16
 *    ^        ^        ^        ^        ^        ^        ^        ^    
 *Fb Y10.Cb00.Y11.Cr10.Y12.Cb02.Y13.Cr12 Y14.Cb04.Y15.Cr14.Y16.Cb06.Y17.Cr16
 *    ^        ^        ^        ^        ^        ^        ^        ^    
 *
 *    lavtools unpacking into 4:2:0-planar
 *
 *Ft  Y00.Y01.Y02.Y03.Y04.Y05.Y06.Y07...
 *Fb  Y00.Y01.Y02.Y03.Y04.Y05.Y06.Y07...
 *Ft  Y10.Y11.Y12.Y13.Y14.Y15.Y16.Y17...
 *Fb  Y10.Y11.Y12.Y13.Y14.Y15.Y16.Y17...
 *
 *Ft  Cb00.Cb02.Cb04.Cb06...
 *Fb  Cb00.Cb02.Cb04.Cb06...
 *
 *Ft  Cr10.Cr12.Cr14.Cr16...
 *Fb  Cr10.Cr12.Cr14.Cr16...
 *
 */

/*
 * Unpack libdv's 4:2:2-packed into our 4:2:0-planar or 4:2:2-planar,
 *  treating each interlaced field independently
 *
 */
static void frame_YUV422_to_planar_42x(uint8_t **output, uint8_t *input,
				       int width, int height, int chroma)
{
    int i, j, w2;
    uint8_t *y, *cb, *cr;

    w2 = width/2;
    y = output[0];
    cb = output[1];
    cr = output[2];

    for (i=0; i<height;) {
	/* process two scanlines (one from each field, interleaved) */
        /* ...top-field scanline */
        for (j=0; j<w2; j++) {
            /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
            *(y++) =  *(input++);
            *(cb++) = *(input++);
            *(y++) =  *(input++);
            *(cr++) = *(input++);
        }
	i++;
        /* ...bottom-field scanline */
        for (j=0; j<w2; j++) {
            /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
            *(y++) =  *(input++);
            *(cb++) = *(input++);
            *(y++) =  *(input++);
            *(cr++) = *(input++);
        }
	i++;
	if (chroma == Y4M_CHROMA_422)
	  continue;
	/* process next two scanlines (one from each field, interleaved) */
        /* ...top-field scanline */
	for (j=0; j<w2; j++) {
	  /* skip every second line for U and V */
	  *(y++) = *(input++);
	  input++;
	  *(y++) = *(input++);
	  input++;
	}
	i++;
        /* ...bottom-field scanline */
	for (j=0; j<w2; j++) {
	  /* skip every second line for U and V */
	  *(y++) = *(input++);
	  input++;
	  *(y++) = *(input++);
	  input++;
	}
	i++;
    }
}

/***** NTSC 4:1:1 *****
 *
 *   libdv's 4:2:2-packed representation  (chroma repeated horizontally)
 *	
 *  23		Ft Y00.Cb00.Y01.Cr00.Y02.Cb00.Y03.Cr00 Y04.Cb04.Y05.Cr04.Y06.Cb04.Y07.Cr04
 *		    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 * 285		Fb Y00.Cb00.Y01.Cr00.Y02.Cb00.Y03.Cr00 Y04.Cb04.Y05.Cr04.Y06.Cb04.Y07.Cr04
 *		    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 *  24		Ft Y10.Cb10.Y11.Cr10.Y12.Cb10.Y13.Cr10 Y14.Cb14.Y15.Cr14.Y16.Cb14.Y17.Cr14
 *		    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 * 286		Fb Y10.Cb10.Y11.Cr10.Y12.Cb10.Y13.Cr10 Y14.Cb14.Y15.Cr14.Y16.Cb14.Y17.Cr14
 *		    ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^
 *	
 *   lavtools unpacking into 4:1:1-planar 
 *	
 *  23		Ft  Y00.Y01.Y02.Y03.Y04.Y05.Y06.Y07...
 * 285		Fb  Y00.Y01.Y02.Y03.Y04.Y05.Y06.Y07...
 *  24		Ft  Y10.Y11.Y12.Y13.Y14.Y15.Y16.Y17...
 * 286		Fb  Y10.Y11.Y12.Y13.Y14.Y15.Y16.Y17...
 *		
 * 23       	Ft  Cb00.Cb04...
 * 285	        Fb  Cb00.Cb04...
 * 24       	Ft  Cb10.Cb14...
 * 286	        Fb  Cb10.Cb14...
 *		
 * 23       	Ft  Cr00.Cr04...
 * 285	        Fb  Cr00.Cr04...
 * 24       	Ft  Cr10.Cr14...
 * 286	        Fb  Cr10.Cr14...
 *		
 *		
 * cf. http://www.sony.ca/dvcam/pdfs/dvcam%20format%20overview.pdf
 */
static void frame_YUV422_to_planar_411(uint8_t **output, uint8_t *input,
				       int width, int height)
{
    int i, j, w4;
    uint8_t *y, *cb, *cr;

    w4 = width/4;
    y = output[0];
    cb = output[1];
    cr = output[2];

    for (i=0; i<height;) {
	/* process two scanlines (one from each field, interleaved) */
        /* ...top-field scanline */
        for (j=0; j<w4; j++) {
            /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
            *(y++) =  *(input++);
            *(cb++) = *(input++);       // NTSC-specific: assert( j%2==0 || cb[-1]==cb[-2]);
            *(y++) =  *(input++);
            *(cr++) = *(input++);       // NTSC-specific: assert( j%2==0 || cr[-1]==cr[-2]);

            *(y++) =  *(input++);
            (input++);                  // NTSC-specific: assert( j%2==0 || cb[-1]==cb[-2]);
            *(y++) =  *(input++);
            (input++);                  // NTSC-specific: assert( j%2==0 || cr[-1]==cr[-2]);
        }
	i++;
        /* ...bottom-field scanline */
        for (j=0; j<w4; j++) {
            /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
            *(y++) =  *(input++);
            *(cb++) = *(input++);       // NTSC-specific: assert( j%2==0 || cb[-1]==cb[-2]);
            *(y++) =  *(input++);
            *(cr++) = *(input++);       // NTSC-specific: assert( j%2==0 || cr[-1]==cr[-2]);

            *(y++) =  *(input++);
            (input++);                  // NTSC-specific: assert( j%2==0 || cb[-1]==cb[-2]);
            *(y++) =  *(input++);
            (input++);                  // NTSC-specific: assert( j%2==0 || cr[-1]==cr[-2]);
        }
	i++;
    }
}

void frame_YUV422_to_planar(uint8_t **output, uint8_t *input,
			    int width, int height, int chroma)
{
    if (chroma == Y4M_CHROMA_411)
        frame_YUV422_to_planar_411(output, input, width, height);
    else
        frame_YUV422_to_planar_42x(output, input, width, height, chroma);
                
}
#endif /* HAVE_LIBDV */

/***********************
 *
 * Take a random(ish) sampled mean of a frame luma and chroma
 * Its intended as a rough and ready hash of frame content.
 * The idea is that changes above a certain threshold are treated as
 * scene changes.
 *
 **********************/

int luminance_mean(uint8_t *frame[], int w, int h )
{
	uint8_t *p;
	uint8_t *lim;
	int sum = 0;
	int count = 0;
	p = frame[0];
	lim = frame[0] + w*(h-1);
	while( p < lim )
	{
		sum += (p[0] + p[1]) + (p[w-3] + p[w-2]);
		p += 31;
		count += 4;
	}

    w = w / 2;
	h = h / 2;

	p = frame[1];
	lim = frame[1] + w*(h-1);
	while( p < lim )
	{
		sum += (p[0] + p[1]) + (p[w-3] + p[w-2]);
		p += 31;
		count += 4;
	}
	p = frame[2];
	lim = frame[2] + w*(h-1);
	while( p < lim )
	{
		sum += (p[0] + p[1]) + (p[w-3] + p[w-2]);
		p += 31;
		count += 4;
	}
	return sum / count;
}

void init(LavParam *param, uint8_t *buffer[])
{
   param->luma_size = param->output_width * param->output_height;
   switch (param->chroma) {
   default:
     mjpeg_warn("unsupported chroma (%d), assume '420jpeg'", param->chroma);
     param->chroma = Y4M_UNKNOWN; /* will update in writeoutYUV4MPEGheader() */
     /* and do same as case Y4M_CHROMA_420JPEG... */
   case Y4M_UNKNOWN:
   case Y4M_CHROMA_420JPEG:
   case Y4M_CHROMA_420MPEG2:
   case Y4M_CHROMA_420PALDV:
     param->chroma_width  = param->output_width  / 2;
     param->chroma_height = param->output_height / 2;
     break;
   case Y4M_CHROMA_422:
     param->chroma_width  = param->output_width  / 2;
     param->chroma_height = param->output_height;
     break;
   case Y4M_CHROMA_411:
     param->chroma_width  = param->output_width  / 4;
     param->chroma_height = param->output_height;
     break;
   }
   param->chroma_size = param->chroma_height * param->chroma_width;

   buffer[0] = (uint8_t *)bufalloc(param->luma_size);
   buffer[1] = (uint8_t *)bufalloc(param->chroma_size);
   buffer[2] = (uint8_t *)bufalloc(param->chroma_size);
   
#ifdef HAVE_LIBDV
   dv_frame[0] = (uint8_t *)bufalloc(3 * param->output_width * param->output_height);
   dv_frame[1] = buffer[1];
   dv_frame[2] = buffer[2];
#endif
}

/*
 * readframe - read jpeg or dv frame into yuv buffer
 *
 * returns:
 *	0   success
 *	1   fatal error
 *	2   corrupt data encountered; 
 *		decoding can continue, but this frame may be damaged 
 */
int readframe(int numframe, 
	      uint8_t *frame[],
	      LavParam *param,
	      EditList el)
{
  int len, i, res, data_format;
  uint8_t *frame_tmp;
  int warn;
  warn = 0;

  if (MAX_JPEG_LEN < el.max_frame_size) {
    mjpeg_error_exit1( "Max size of JPEG frame = %ld: too big",
		       el.max_frame_size);
  }
  
  len = el_get_video_frame(jpeg_data, numframe, &el);
  data_format = el_video_frame_data_format(numframe, &el);
  
  switch(data_format) {

  case DATAFORMAT_DV2 :
#ifndef HAVE_LIBDV
    mjpeg_error("DV input was not configured at compile time");
    res = 1;
#else
    mjpeg_debug("DV frame %d   len %d",numframe,len);
    res = 0;
    dv_parse_header(decoder, jpeg_data);
    switch(decoder->sampling) {
    case e_dv_sample_420:
      /* libdv decodes PAL DV directly as planar YUV 420
       * (YV12 or 4CC 0x32315659) if configured with the flag
       * --with-pal-yuv=YV12 which is not (!) the default
       */
      if (libdv_pal_yv12 == 1) {
	pitches[0] = decoder->width;
	pitches[1] = decoder->width / 2;
	pitches[2] = decoder->width / 2;
	if (pitches[0] != param->output_width ||
	    pitches[1] != param->chroma_width) {
	  mjpeg_error("for DV 4:2:0 only full width output is supported");
	  res = 1;
	} else {
	  dv_decode_full_frame(decoder, jpeg_data, e_dv_color_yuv,
			       frame, (int *)pitches);
	  /* swap the U and V components */
	  frame_tmp = frame[2];
	  frame[2] = frame[1];
	  frame[1] = frame_tmp;
	}
	break;
      }
    case e_dv_sample_411:
    case e_dv_sample_422:
      /* libdv decodes NTSC DV (native 411) and by default also PAL
       * DV (native 420) as packed YUV 422 (YUY2 or 4CC 0x32595559)
       * where the U and V information is repeated.  This can be
       * transformed to planar 420 (YV12 or 4CC 0x32315659).
       * For NTSC DV this transformation is lossy.
       */
      pitches[0] = decoder->width * 2;
      pitches[1] = 0;
      pitches[2] = 0;
      if (decoder->width != param->output_width) {
	mjpeg_error("for DV only full width output is supported");
	res = 1;
      } else {
	dv_decode_full_frame(decoder, jpeg_data, e_dv_color_yuv,
			     dv_frame, (int *)pitches);
	frame_YUV422_to_planar(frame, dv_frame[0],
			       decoder->width,	decoder->height,
			       param->chroma);
      }
      break;
    default:
      res = 1;
      break;
    }
#endif /* HAVE_LIBDV */
    break;

  case DATAFORMAT_YUV420 :
  case DATAFORMAT_YUV422 :
    mjpeg_debug("raw YUV frame %d   len %d",numframe,len);
    frame_tmp = jpeg_data;
    memcpy(frame[0], frame_tmp, param->luma_size);
    frame_tmp += param->luma_size;
    memcpy(frame[1], frame_tmp, param->chroma_size);
    frame_tmp += param->chroma_size;
    memcpy(frame[2], frame_tmp, param->chroma_size);
    res = 0;
    break;

  default:
    mjpeg_debug("MJPEG frame %d   len %d",numframe,len);
    res = decode_jpeg_raw(jpeg_data, len, el.video_inter,
			  param->chroma,
			  param->output_width, param->output_height,
			  frame[0], frame[1], frame[2]);
  }
  
  if (res < 0) {
    mjpeg_warn( "Fatal Error Decoding Frame %d", numframe);
    return 1;
  } else if (res == 1) {
    mjpeg_warn( "Decoding of Frame %d failed", numframe);
    warn = 1;
    res = 0;
  }
  
  
  if (param->mono) {
    for (i = 0;
	 i < param->chroma_size;
	 ++i) {
      frame[1][i] = 0x80;
      frame[2][i] = 0x80;
    }
  }

  if(warn)
	  return 2;
  else
	  return 0;
}


void writeoutYUV4MPEGheader(int out_fd,
			    LavParam *param,
			    EditList el,
			    y4m_stream_info_t *streaminfo)
{
   int n;

   y4m_si_set_width(streaminfo, param->output_width);
   y4m_si_set_height(streaminfo, param->output_height);
   y4m_si_set_interlace(streaminfo, param->interlace);
   y4m_si_set_framerate(streaminfo, mpeg_conform_framerate(el.video_fps));
   if (!Y4M_RATIO_EQL(param->sar, y4m_sar_UNKNOWN)) {
     y4m_si_set_sampleaspect(streaminfo, param->sar);
   } else if ((el.video_sar_width != 0) || (el.video_sar_height != 0)) {
     y4m_ratio_t sar;
     sar.n = el.video_sar_width;
     sar.d = el.video_sar_height;
     y4m_si_set_sampleaspect(streaminfo, sar);
   } else {
     /* no idea! ...eh, just guess. */
     mjpeg_warn("unspecified sample-aspect-ratio --- taking a guess...");
     y4m_si_set_sampleaspect(streaminfo,
			     y4m_guess_sar(param->output_width, 
					   param->output_height,
					   param->dar));
   }

   switch (el_video_frame_data_format(0, &el)) { /* FIXME: checking only 0-th frame. */
   case DATAFORMAT_YUV420:
     switch (param->chroma) {
     case Y4M_UNKNOWN:
     case Y4M_CHROMA_420JPEG:
       break;
     case Y4M_CHROMA_420MPEG2:
     case Y4M_CHROMA_420PALDV:
       mjpeg_warn("4:2:0 chroma should be '420jpeg' with this input");
       break;
     default:
       mjpeg_error_exit1("must specify 4:2:0 chroma (should be '420jpeg') with this input");
       break;
     }
     break;

   case DATAFORMAT_YUV422:
     switch (param->chroma) {
     case Y4M_CHROMA_422:
       break;
     default:
       mjpeg_error_exit1("must specify chroma '422' with this input");
       break;
     }
     break;

   case DATAFORMAT_DV2:
#ifndef HAVE_LIBDV
     mjpeg_error_exit1("DV input was not configured at compile time");
#else
     el_get_video_frame(jpeg_data, 0, &el); /* FIXME: checking only 0-th frame. */
     dv_parse_header(decoder, jpeg_data);
     switch(decoder->sampling) {
     case e_dv_sample_420:
       switch (param->chroma) {
       case Y4M_UNKNOWN:
	 mjpeg_info("set chroma '420paldv' from input");
	 param->chroma = Y4M_CHROMA_420PALDV;
	 break;
       case Y4M_CHROMA_420PALDV:
	 break;
       case Y4M_CHROMA_420JPEG:
       case Y4M_CHROMA_420MPEG2:
	 mjpeg_warn("4:2:0 chroma should be '420paldv' with this input");
	 break;
       case Y4M_CHROMA_422:
         if(libdv_pal_yv12 == 1 )
	   mjpeg_error_exit1("must specify 4:2:0 chroma (should be '420paldv') with this input");
	 break;
       default:
	 mjpeg_error_exit1("must specify 4:2:0 chroma (should be '420paldv') with this input");
	 break;
       }
       break;
     case e_dv_sample_411:
       if (param->chroma != Y4M_CHROMA_411)
	 mjpeg_info("chroma '411' recommended with this input");
       switch (param->chroma) {
       case Y4M_CHROMA_420MPEG2:
       case Y4M_CHROMA_420PALDV:
	 mjpeg_warn("4:2:0 chroma should be '420jpeg' with this input");
	 break;
       }
       break;
     case e_dv_sample_422:
       if (param->chroma != Y4M_CHROMA_422)
	 mjpeg_info("chroma '422' recommended with this input");
       switch (param->chroma) {
       case Y4M_CHROMA_420MPEG2:
       case Y4M_CHROMA_420PALDV:
	 mjpeg_warn("4:2:0 chroma should be '420jpeg' with this input");
	 break;
       }
       break;
     default:
       break;
     }
#endif
     break;

   case DATAFORMAT_MJPG:
     if (param->chroma != Y4M_CHROMA_422 && el.chroma == Y4M_CHROMA_422)
       mjpeg_info("chroma '422' recommended with this input");
     switch (param->chroma) {
     case Y4M_CHROMA_420MPEG2:
     case Y4M_CHROMA_420PALDV:
       mjpeg_warn("4:2:0 chroma should be '420jpeg' with this input");
       break;
     }
     break;
   }
   if (param->chroma == Y4M_UNKNOWN) {
     mjpeg_info("set default chroma '420jpeg'");
     param->chroma = Y4M_CHROMA_420JPEG;
   }
   y4m_si_set_chroma(streaminfo, param->chroma);

   n = y4m_write_stream_header(out_fd, streaminfo);
   if (n != Y4M_OK)
      mjpeg_error("Failed to write stream header: %s", y4m_strerr(n));
}

#ifdef HAVE_LIBDV
void lav_init_dv_decoder()
{
   decoder = dv_decoder_new(0,0,0);
   decoder->quality = DV_QUALITY_BEST;
}
#endif
