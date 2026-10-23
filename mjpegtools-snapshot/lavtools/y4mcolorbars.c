/*
 * y4mcolorbars.c:  Generate real, standard colorbars in POG form 
 *                   (where "POG" == "YUV4MPEG2 stream").
 *
 *
 *  Copyright (C) 2004 Matthew J. Marjanovic <maddog@mir.com>
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <yuv4mpeg.h>
#include <mpegconsts.h>

#include "subsample.h"
#include "colorspace.h"

#define DEFAULT_CHROMA_MODE Y4M_CHROMA_444

#define IQ_MODE_IQ20    0  /* 20 percent -I,+Q                         */
#define IQ_MODE_IQ50    1  /* 50 percent -I,+Q                         */
#define IQ_MODE_CBCR100 2  /* 100 percent -Cb,+Cr  (original behavior) */


typedef struct _cl_info {
  y4m_ratio_t aspect;
  int interlace;
  y4m_ratio_t framerate;
  int framecount;
  int ss_mode;
  int width;
  int height;
  int verbosity;
  int iq_mode;
} cl_info_t;



static
void usage(const char *progname)
{
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "Creates a YUV4MPEG2 stream consisting of frames containing a standard\n");
  fprintf(stderr, " colorbar test pattern (SMPTE, 75%%).\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:  (defaults specified in [])\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -n n     frame count (output n frames) [1]\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -W w     frame width [720]\n");
  fprintf(stderr, "  -H h     frame height [480]\n");
  fprintf(stderr, "  -F n:d   framerate (as ratio) [30000:1001]\n");
  fprintf(stderr, "  -A w:h   pixel aspect ratio [10:11]\n");
  fprintf(stderr, "  -I x     interlacing [p]\n");
  fprintf(stderr, "             p = none/progressive\n");
  fprintf(stderr, "             t = top-field-first\n");
  fprintf(stderr, "             b = bottom-field-first\n");
  fprintf(stderr, "  -Q n     content of -I/Q areas:  [0]\n");
  fprintf(stderr, "             0 = 20%% -I,+Q\n");
  fprintf(stderr, "             1 = 50%% -I,+Q\n");
  fprintf(stderr, "             2 = 100%% +Cb,+Cr\n");
  fprintf(stderr, "\n");
  {
    int m;
    const char *keyword;

    fprintf(stderr, "  -S mode  chroma subsampling mode [%s]\n",
	    y4m_chroma_keyword(DEFAULT_CHROMA_MODE));
    for (m = 0;
	 (keyword = y4m_chroma_keyword(m)) != NULL;
	 m++)
      if (chroma_sub_implemented(m))
	fprintf(stderr, "            '%s' -> %s\n",
		keyword, y4m_chroma_description(m));
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "  -v n     verbosity (0,1,2) [1]\n");
}



static
void parse_args(cl_info_t *cl, int argc, char **argv)
{
  int c;

  cl->interlace = Y4M_ILACE_NONE;
  cl->framerate = y4m_fps_NTSC;
  cl->framecount = 1;
  cl->ss_mode = DEFAULT_CHROMA_MODE;
  cl->width = 720;
  cl->height = 480;
  cl->aspect = y4m_sar_NTSC_CCIR601;
  cl->verbosity = 1;
  cl->iq_mode = IQ_MODE_IQ20;

  while ((c = getopt(argc, argv, "A:F:I:W:H:n:S:Q:v:h")) != -1) {
    switch (c) {
    case 'W':
      if ((cl->width = atoi(optarg)) <= 0) {
	mjpeg_error("Invalid width:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'H':
      if ((cl->height = atoi(optarg)) <= 0) {
	mjpeg_error("Invalid height:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'A':
      if (y4m_parse_ratio(&(cl->aspect), optarg) != Y4M_OK) {
	mjpeg_error("Could not parse ratio:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'F':
      if (y4m_parse_ratio(&(cl->framerate), optarg) != Y4M_OK) {
	mjpeg_error("Could not parse framerate:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'I':
      switch (optarg[0]) {
      case 'p':  cl->interlace = Y4M_ILACE_NONE;  break;
      case 't':  cl->interlace = Y4M_ILACE_TOP_FIRST;  break;
      case 'b':  cl->interlace = Y4M_ILACE_BOTTOM_FIRST;  break;
      default:
	mjpeg_error("Unknown value for interlace: '%c'", optarg[0]);
	goto ERROR_EXIT;
	break;
      }
      break;
    case 'n':
      if ((cl->framecount = atoi(optarg)) <= 0) {
	mjpeg_error("Invalid frame count:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'S':
      cl->ss_mode = y4m_chroma_parse_keyword(optarg);
      if (cl->ss_mode == Y4M_UNKNOWN) {
	mjpeg_error("Unknown subsampling mode option:  %s", optarg);
	goto ERROR_EXIT;
      } else if (!chroma_sub_implemented(cl->ss_mode)) {
	mjpeg_error("Unsupported subsampling mode option:  %s", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'Q':
      switch (atoi(optarg)) {
      case 0:  cl->iq_mode = IQ_MODE_IQ20;     break;
      case 1:  cl->iq_mode = IQ_MODE_IQ50;     break;
      case 2:  cl->iq_mode = IQ_MODE_CBCR100;  break;
      default:
	mjpeg_error("Unknown -I/+Q mode:  %d", atoi(optarg));
	goto ERROR_EXIT;
      }
      break;
    case 'v':
      cl->verbosity = atoi(optarg);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
      break;
    case '?':
    default:
      mjpeg_error("Unknown option:  '-%c'", optopt);
      goto ERROR_EXIT;
      break;
    }
  }

  mjpeg_default_handler_verbosity(cl->verbosity);

  mjpeg_info("Colorbar Command-line Parameters:");
  mjpeg_info("            frame size:  %dx%d", cl->width, cl->height);
  mjpeg_info("             framerate:  %d:%d",
	     cl->framerate.n, cl->framerate.d);
  mjpeg_info("    pixel aspect ratio:  %d:%d",
	     cl->aspect.n, cl->aspect.d);
  mjpeg_info("             interlace:  %s",
	     mpeg_interlace_code_definition(cl->interlace));
  mjpeg_info("           # of frames:  %d", cl->framecount);
  mjpeg_info("    chroma subsampling:  %s",
	     y4m_chroma_description(cl->ss_mode));
  return;

 ERROR_EXIT:
  mjpeg_error("For usage hints, use option '-h':  '%s -h'", argv[0]);
  exit(1);

}




/*
 * Color Bars:
 *
 *     top 2/3:  75% white, followed by 75% binary combinations
 *                of R', G', and B' with decreasing luma
 *
 * middle 1/12:  reverse order of above, but with 75% white and
 *                alternating black
 *
 *  bottom 1/4:  -I, 100% white, +Q, black, PLUGE, black,
 *                where PLUGE is (black - 4 IRE), black, (black + 4 IRE)
 *
 */

/*  75% white   */
/*  75% yellow  */
/*  75% cyan    */
/*  75% green   */
/*  75% magenta */
/*  75% red     */
/*  75% blue    */
static uint8_t rainbowRGB[][7] = {
  { 191, 191,   0,   0, 191, 191,   0 },
  { 191, 191, 191, 191,   0,   0,   0 },
  { 191,   0, 191,   0, 191,   0, 191 }
};
static uint8_t *rainbow[3] = {
  rainbowRGB[0], rainbowRGB[1], rainbowRGB[2]
};


/*  75% blue    */
/*      black   */
/*  75% magenta */
/*      black   */
/*  75% cyan    */
/*      black   */
/*  75% white   */
static uint8_t wobnairRGB[][7] = {
  {   0,   0, 191,   0,   0,   0, 191 },
  {   0,   0,   0,   0, 191,   0, 191 },
  { 191,   0, 191,   0, 191,   0, 191 }
};
static uint8_t *wobnair[3] = {
  wobnairRGB[0], wobnairRGB[1], wobnairRGB[2]
};


/* The ancient Y'IQ
 * 
 * Classic colorbars have -I and +Q in the PLUGE section.
 *
 * Following Poynton, "Digital Video and HDTV", p367:
 *
 *    -I = (-0.955987, +0.272013, +1.106740) R'G'B'
 *    +Q = (+0.620825, -0.647204, +1.704231) R'G'B'
 *
 * Converting to Y'PbPr and Y'CbCr:
 *
 *    -I = (0, 0.624571 -0.681873) -> (16, 268, -25)
 *    +Q = (0, 0.961755  0.442815) -> (16, 343, 227)
 *
 * Uh, oh:  these are outside of the Y'CbCr gamut.
 * We can get around this, kind of, by reducing the magnitude until they
 *  fit, keeping the phase the same.  Q is the worst offender, and a factor
 *  of 0.5 does the trick:
 *
 *   (0.5)*(-I) = (0 0.312286 -0.340937) -> (16, 198, 52)
 *   (0.5)*(+Q) = (0 0.480878  0.221407) -> (16, 236, 178)
 * 
 *   (0.2)*(-I) = (0 0.124914 -0.136375) -> (16 156  97)
 *   (0.2)*(+Q) = (0 0.192351  0.088563) -> (16 171 148)

10, 9E, 5F   16, 158, 95    
10, AE, 95   16, 174, 149

i  244 612 395  61 153 99
q  141 697 606  35 174 151

-4  29 512 512   7.25 128 128
+4  99 512 512  24.75 128 128


AD-723:  burst ampl: 185-250-315 mV
Bt860:       285.7 mV --> 40 IRE         (per RS-170A -- 40 IRE top-bottom?)

Maxim app notes:   NTSC: 286mV peak-to-peak
                    PAL: 300mV peak-to-peak

 *
 */
static uint8_t negI_50percent[] = { 16, 198,  52 };
static uint8_t posQ_50percent[] = { 16, 236, 178 };

static uint8_t negI_20percent[] = { 16, 156,  97 };
static uint8_t posQ_20percent[] = { 16, 171, 148 };

/* static uint8_t negCb_100percent[] = { 16,  16, 128 }; */
static uint8_t posCb_100percent[] = { 16, 240, 128 };

/* static uint8_t negCr_100percent[] = { 16, 128,  16 }; */
static uint8_t posCr_100percent[] = { 16, 128, 240 };


/* PLUGE  (SMPTE EG-1-1990)
 *
 * This part of the signal can never be truly synthesized in Y'CbCr space,
 * because it is fundamentally tied to the analog signal specification.
 *
 * The two main components are a (Black - 4 IRE) and (Black + 4 IRE).
 * Simple enough, except that the relation between "IRE" and Y' units depends
 *  on the specific analog output format, which sets the black-white range:
 *
 *                    Y':  16-235       -> excursion of 219
 *    NTSC with 0% setup:  0-100 IRE    -> excursion of 100 IRE
 *  NTSC with 7.5% setup:  7.5-100 IRE  -> excursion of 92.5 IRE
 *
 * It's the analog output stage (e.g. graphics card, decoder chip) that
 *  converts the Y'CbCr digital values to an analog voltage appropriate
 *  for whatever analog equipment comes next.
 * To perfectly synthesize a 4-IRE signal in a 0% setup system, we'd
 *  want 8.76 "Y' units".  For a 7.5% setup system, we'd want 9.47.
 *  Lucky for us, we're not allowed to use fractions anyway, so we
 *  just split the difference and settle for "9".
 * 
 */
static uint8_t neg4IRE[] = { (16 - 9), 128, 128 };
static uint8_t pos4IRE[] = { (16 + 9), 128, 128 };

static uint8_t black[] = {  16, 128, 128 };
static uint8_t white[] = { 235, 128, 128 };




static
void create_bars(uint8_t *ycbcr[], int width, int height, int iq_mode)
{
  int i, x, y, w;
  int bnb_start;
  int pluge_start;
  int stripe_width;
  int pl_width;
  uint8_t *lineY;
  uint8_t *lineCb;
  uint8_t *lineCr;

  uint8_t *Y = ycbcr[0];
  uint8_t *Cb = ycbcr[1];
  uint8_t *Cr = ycbcr[2];

  uint8_t *i_pixel;
  uint8_t *q_pixel;

  convert_RGB_to_YCbCr(rainbow, 7);
  convert_RGB_to_YCbCr(wobnair, 7);

  switch (iq_mode) {
  case IQ_MODE_CBCR100:
    i_pixel = posCb_100percent;
    q_pixel = posCr_100percent;
    break;
  case IQ_MODE_IQ50:
    i_pixel = negI_50percent;
    q_pixel = posQ_50percent;
    break;
  case IQ_MODE_IQ20:
  default:
    i_pixel = negI_20percent;
    q_pixel = posQ_20percent;
    break;
  }

  bnb_start = height * 2 / 3;
  pluge_start = height * 3 / 4;
  stripe_width = (width + 6) / 7;
  lineY = malloc(width * sizeof(lineY[0]));
  lineCb = malloc(width * sizeof(lineCb[0]));
  lineCr = malloc(width * sizeof(lineCr[0]));

  /* Top:  Rainbow */
  for (i = 0, x = 0; i < 7; i++) {
    for (w = 0; (w < stripe_width) && (x < width); w++, x++) {
      lineY[x] = rainbow[0][i];
      lineCb[x] = rainbow[1][i];
      lineCr[x] = rainbow[2][i];
    }
  }
  for (y = 0; y < bnb_start; y++) {
    memcpy(Y, lineY, width);
    memcpy(Cb, lineCb, width);
    memcpy(Cr, lineCr, width);
    Y += width;
    Cb += width;
    Cr += width;
  }

  /* Middle:  Wobnair */
  for (i = 0, x = 0; i < 7; i++) {
    for (w = 0; (w < stripe_width) && (x < width); w++, x++) {
      lineY[x] = wobnair[0][i];
      lineCb[x] = wobnair[1][i];
      lineCr[x] = wobnair[2][i];
    }
  }
  for (; y < pluge_start; y++) {
    memcpy(Y, lineY, width);
    memcpy(Cb, lineCb, width);
    memcpy(Cr, lineCr, width);
    Y += width;
    Cb += width;
    Cr += width;
  }

  /* Bottom:  PLUGE */
  pl_width = 5 * stripe_width / 4;
  /* -I patch */
  for (x = 0; x < pl_width; x++) {
    lineY[x] = i_pixel[0];
    lineCb[x] = i_pixel[1];
    lineCr[x] = i_pixel[2];
  }
  /* white */
  for (; x < (2 * pl_width); x++) {
    lineY[x] =  white[0];
    lineCb[x] = white[1];
    lineCr[x] = white[2];
  }
  /* +Q patch */
  for (; x < (3 * pl_width); x++) {
    lineY[x] = q_pixel[0];
    lineCb[x] = q_pixel[1];
    lineCr[x] = q_pixel[2];
  }
  /* black */
  for (; x < (5 * stripe_width); x++) {
    lineY[x] =  black[0];
    lineCb[x] = black[1];
    lineCr[x] = black[2];
  }
  /* (black - 4IRE) | black | (black + 4IRE)  */
  for (; x < (5 * stripe_width) + (stripe_width / 3); x++) {
    lineY[x] =  neg4IRE[0];
    lineCb[x] = neg4IRE[1];
    lineCr[x] = neg4IRE[2];
  }
  for (; x < (5 * stripe_width) + (2 * (stripe_width / 3)); x++) {
    lineY[x] =  black[0];
    lineCb[x] = black[1];
    lineCr[x] = black[2];
  }
  for (; x < (6 * stripe_width); x++) {
    lineY[x] =  pos4IRE[0];
    lineCb[x] = pos4IRE[1];
    lineCr[x] = pos4IRE[2];
  }
  /* black */
  for (; x < width; x++) {
    lineY[x] =  black[0];
    lineCb[x] = black[1];
    lineCr[x] = black[2];
  }
  for (; y < height; y++) {
    memcpy(Y, lineY, width);
    memcpy(Cb, lineCb, width);
    memcpy(Cr, lineCr, width);
    Y += width;
    Cb += width;
    Cr += width;
  }
  free(lineY);
  free(lineCb);
  free(lineCr);
}




int main(int argc, char **argv)
{
  cl_info_t cl;
  y4m_stream_info_t sinfo;
  y4m_frame_info_t finfo;
  uint8_t *planes[Y4M_MAX_NUM_PLANES];  /* Y'CbCr frame buffer */
  int fdout = fileno(stdout);
  int i;
  int err;

  y4m_accept_extensions(1);
  y4m_init_stream_info(&sinfo);
  y4m_init_frame_info(&finfo);

  parse_args(&cl, argc, argv);

  /* Setup streaminfo and write output header */
  y4m_si_set_width(&sinfo, cl.width);
  y4m_si_set_height(&sinfo, cl.height);
  y4m_si_set_sampleaspect(&sinfo, cl.aspect);
  y4m_si_set_interlace(&sinfo, cl.interlace);
  y4m_si_set_framerate(&sinfo, cl.framerate);
  y4m_si_set_chroma(&sinfo, cl.ss_mode);
  if ((err = y4m_write_stream_header(fdout, &sinfo)) != Y4M_OK)
    mjpeg_error_exit1("Write header failed: %s", y4m_strerr(err));
  mjpeg_info("Colorbar Stream parameters:");
  y4m_log_stream_info(mjpeg_loglev_t("info"), "  ", &sinfo);

  /* Create the colorbars frame */
  for (i = 0; i < 3; i++)
    planes[i] = malloc(cl.width * cl.height * sizeof(planes[i][0]));
  create_bars(planes, cl.width, cl.height, cl.iq_mode);
  chroma_subsample(cl.ss_mode, planes, cl.width, cl.height);

  /* We're on the air! */
  for (i = 0; i < cl.framecount; i++) {
    if ((err = y4m_write_frame(fdout, &sinfo, &finfo, planes)) != Y4M_OK)
      mjpeg_error_exit1("Write frame failed: %s", y4m_strerr(err));
  }

  /* We're off the air. :( */
  for (i = 0; i < 3; i++)
    free(planes[i]);
  y4m_fini_stream_info(&sinfo);
  y4m_fini_frame_info(&finfo);
  return 0;
}
