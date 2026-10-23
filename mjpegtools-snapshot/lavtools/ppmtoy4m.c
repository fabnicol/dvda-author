/*
 * ppmtoy4m.c:  Generate a YUV4MPEG2 stream from one or more PPM images
 *
 *              Converts R'G'B' to ITU-Rec.601 Y'CbCr colorspace and
 *               performs 4:2:0 chroma subsampling.
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
#include <fcntl.h>

#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include "subsample.h"
#include "colorspace.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

#define DEFAULT_CHROMA_MODE Y4M_CHROMA_444


/* command-line parameters */
typedef struct _cl_info {
  y4m_ratio_t aspect;    
  y4m_ratio_t framerate; 
  int interlace;         
  int interleave;
  int offset;
  int framecount;
  int repeatlast;
  int ss_mode;
  int verbosity;
  int fdin;
  int bgr;
} cl_info_t;


/* PPM image information */
typedef struct _ppm_info {
  int width;
  int height;
} ppm_info_t;



static
void usage(const char *progname)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "usage:  %s [options] [ppm-file]\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "Reads RAW PPM image(s), and produces YUV4MPEG2 stream on stdout.\n");
  fprintf(stderr, "Converts computer graphics R'G'B' colorspace to digital video Y'CbCr,\n");
  fprintf(stderr, " and performs chroma subsampling.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If 'ppm-file' is not specified, reads from stdin.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, " options:  (defaults specified in [])\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -o n     frame offset (skip n input frames) [0]\n");
  fprintf(stderr, "  -n n     frame count (output n frames; 0 == all of them) [0]\n");
  fprintf(stderr, "  -F n:d   framerate [30000:1001 = NTSC]\n");
  fprintf(stderr, "  -A w:h   pixel aspect ratio [1:1]\n");
  fprintf(stderr, "  -I x     interlacing [p]\n");
  fprintf(stderr, "             p = none/progressive\n");
  fprintf(stderr, "             t = top-field-first\n");
  fprintf(stderr, "             b = bottom-field-first\n");
  fprintf(stderr, "  -L       treat PPM images as 2-field interleaved\n");
  fprintf(stderr, "  -r       repeat last input frame\n");
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
  /*  fprintf(stderr, "  -R type  subsampling filter type\n");*/
  fprintf(stderr, "  -v n     verbosity (0,1,2) [1]\n");
  fprintf(stderr, "  -B       PPM image is packed with BGR pixels [RGB]\n");
}



static
void parse_args(cl_info_t *cl, int argc, char **argv)
{
  int c;

  cl->offset = 0;
  cl->framecount = 0;
  cl->aspect = y4m_sar_SQUARE;
  cl->interlace = Y4M_ILACE_NONE;
  cl->framerate = y4m_fps_NTSC;
  cl->interleave = 0;
  cl->repeatlast = 0;
  cl->ss_mode = DEFAULT_CHROMA_MODE;
  cl->verbosity = 1;
  cl->bgr = 0;
  cl->fdin = fileno(stdin); /* default to stdin */

  while ((c = getopt(argc, argv, "BA:F:I:Lo:n:rS:v:h")) != -1) {
    switch (c) {
    case 'A':
      if (y4m_parse_ratio(&(cl->aspect), optarg) != Y4M_OK) {
	mjpeg_error("Could not parse ratio:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'F':
      if (y4m_parse_ratio(&(cl->framerate), optarg) != Y4M_OK) {
	mjpeg_error("Could not parse ratio:  '%s'", optarg);
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
    case 'L':
      cl->interleave = 1;
      break;
    case 'B':
      cl->bgr = 1;
      break;
    case 'o':
      if ((cl->offset = atoi(optarg)) < 0)
	mjpeg_error_exit1("Offset must be >= 0:  '%s'", optarg);
      break;
    case 'n':
      if ((cl->framecount = atoi(optarg)) < 0)
	mjpeg_error_exit1("Frame count must be >= 0:  '%s'", optarg);
      break;
    case 'r':
      cl->repeatlast = 1;
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
    case 'v':
      cl->verbosity = atoi(optarg);
      if ((cl->verbosity < 0) || (cl->verbosity > 2))
	mjpeg_error("Verbosity must be 0, 1, or 2:  '%s'", optarg);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
      break;
    case '?':
    default:
      goto ERROR_EXIT;
      break;
    }
  }
  /* optional remaining argument is a filename */
  if (optind == (argc - 1)) {
    if ((cl->fdin = open(argv[optind], O_RDONLY | O_BINARY)) == -1)
      mjpeg_error_exit1("Failed to open '%s':  %s",
			argv[optind], strerror(errno));
  } else if (optind != argc) 
    goto ERROR_EXIT;


  mjpeg_default_handler_verbosity(cl->verbosity);

  mjpeg_info("Command-line Parameters:");
  mjpeg_info("             framerate:  %d:%d",
	     cl->framerate.n, cl->framerate.d);
  mjpeg_info("    pixel aspect ratio:  %d:%d",
	     cl->aspect.n, cl->aspect.d);
  mjpeg_info("         pixel packing:  %s",
	     cl->bgr?"BGR":"RGB");
  mjpeg_info("             interlace:  %s%s",
	     mpeg_interlace_code_definition(cl->interlace),
	     (cl->interlace == Y4M_ILACE_NONE) ? "" :
	     (cl->interleave) ? " (interleaved PPM input)" :
	     " (field-sequential PPM input)");
  mjpeg_info("        starting frame:  %d", cl->offset);
  if (cl->framecount == 0)
    mjpeg_info("           # of frames:  all%s",
	       (cl->repeatlast) ? ", repeat last frame forever" :
	       ", until input exhausted");
  else
    mjpeg_info("           # of frames:  %d%s",
	       cl->framecount,
	       (cl->repeatlast) ? ", repeat last frame until done" :
	       ", or until input exhausted");
  mjpeg_info("    chroma subsampling:  %s",
	     y4m_chroma_description(cl->ss_mode));

  /* DONE! */
  return;

 ERROR_EXIT:
  mjpeg_error("For usage hints, use option '-h'.  Please take a hint.");
  exit(1);

}



/*
 * returns:  0 - success, got header
 *           1 - EOF, no new frame
 *          -1 - failure
 */


#define DO_READ_NUMBER(var)                                     \
  do {                                                          \
    if (!isdigit(s[0]))                                         \
      mjpeg_error_exit1("PPM read error:  bad char");         \
    (var) = ((var) * 10) + (s[0] - '0');                        \
  } while (((n = read(fd, s, 1)) == 1) && (!isspace(s[0])));    \
  if (n <= 0) return -1;                                        



#define DO_SKIP_WHITESPACE()                                     \
  incomment = 0;                                                 \
  while ( ((n = read(fd, s, 1)) == 1) &&                         \
	  ((isspace(s[0])) || (s[0] == '#') || (incomment)) ) {  \
    if (s[0] == '#') incomment = 1;                              \
    if (s[0] == '\n') incomment = 0;                             \
  }                                                              \
  if (n <= 0) return -1;                                         


static
int read_ppm_header(int fd, int *width, int *height)
{
  char s[6];
  int incomment;
  int n;
  int maxval = 0;
  
  *width = 0;
  *height = 0;

  /* look for "P6" */
  n = y4m_read(fd, s, 3);
  if (n > 0) 
    return 1;  /* EOF */

  if ((n < 0) || (strncmp(s, "P6", 2)))
    mjpeg_error_exit1("Bad Raw PPM magic!");

  incomment = 0;
  DO_SKIP_WHITESPACE();
  DO_READ_NUMBER(*width);
  DO_SKIP_WHITESPACE();
  DO_READ_NUMBER(*height);
  DO_SKIP_WHITESPACE();
  DO_READ_NUMBER(maxval);

  if (maxval != 255)
    mjpeg_error_exit1("Expecting maxval == 255, not %d!", maxval);

  return 0;
}



static
void alloc_buffers(uint8_t *buffers[], int width, int height)
{
  mjpeg_debug("Alloc'ing buffers");
  buffers[0] = malloc(width * height * 2 * sizeof(buffers[0][0]));
  buffers[1] = malloc(width * height * 2 * sizeof(buffers[1][0]));
  buffers[2] = malloc(width * height * 2 * sizeof(buffers[2][0]));
}






static
void read_ppm_into_two_buffers(int fd,
			       uint8_t *buffers[], 
			       uint8_t *buffers2[], 
			       uint8_t *rowbuffer,
			       int width, int height, int bgr)
{
  int x, y;
  uint8_t *pixels;
  uint8_t *R = buffers[0];
  uint8_t *G = buffers[1];
  uint8_t *B = buffers[2];
  uint8_t *R2 = buffers2[0];
  uint8_t *G2 = buffers2[1];
  uint8_t *B2 = buffers2[2];

  mjpeg_debug("read into two buffers, %dx%d", width, height);
  height /= 2;
  for (y = 0; y < height; y++) {
    pixels = rowbuffer;
    if (y4m_read(fd, pixels, width * 3))
      mjpeg_error_exit1("read error A  y=%d", y);
    if (bgr) {
      for (x = 0; x < width; x++) {
	*(B++) = *(pixels++);
	*(G++) = *(pixels++);
	*(R++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(R++) = *(pixels++);
	*(G++) = *(pixels++);
	*(B++) = *(pixels++);
      }
    }
    pixels = rowbuffer;
    if (y4m_read(fd, pixels, width * 3))
      mjpeg_error_exit1("read error B  y=%d", y);
    if (bgr) {
      for (x = 0; x < width; x++) {
	*(B2++) = *(pixels++);
	*(G2++) = *(pixels++);
	*(R2++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(R2++) = *(pixels++);
	*(G2++) = *(pixels++);
	*(B2++) = *(pixels++);
      }
    }
  }
}



static
void read_ppm_into_one_buffer(int fd,
			      uint8_t *buffers[],
			      uint8_t *rowbuffer,
			      int width, int height, int bgr) 
{
  int x, y;
  uint8_t *pixels;
  uint8_t *R = buffers[0];
  uint8_t *G = buffers[1];
  uint8_t *B = buffers[2];

  for (y = 0; y < height; y++) {
    pixels = rowbuffer;
    y4m_read(fd, pixels, width * 3);
    if (bgr) {
      for (x = 0; x < width; x++) {
	*(B++) = *(pixels++);
	*(G++) = *(pixels++);
	*(R++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(R++) = *(pixels++);
	*(G++) = *(pixels++);
	*(B++) = *(pixels++);
      }
    }
  }
}


/* 
 * read one whole frame
 * if field-sequential interlaced, this requires reading two PPM images
 *
 * if interlaced, fields are written into separate buffers
 *
 * ppm.width/height refer to image dimensions
 */

static
int read_ppm_frame(int fd, ppm_info_t *ppm,
		   uint8_t *buffers[], uint8_t *buffers2[],
		   int ilace, int ileave, int bgr)
{
  int width, height;
  static uint8_t *rowbuffer = NULL;
  int err;

  err = read_ppm_header(fd, &width, &height);
  if (err > 0) return 1;  /* EOF */
  if (err < 0) return -1; /* error */
  mjpeg_debug("Got PPM header:  %dx%d", width, height);

  if (ppm->width == 0) {
    /* first time */
    mjpeg_debug("Initializing PPM read_frame");
    ppm->width = width;
    ppm->height = height;
    rowbuffer = malloc(width * 3 * sizeof(rowbuffer[0]));
  } else {
    /* make sure everything matches */
    if ( (ppm->width != width) ||
	 (ppm->height != height) )
      mjpeg_error_exit1("One of these frames is not like the others!");
  }
  if (buffers[0] == NULL) 
    alloc_buffers(buffers, width, height);
  if ((buffers2[0] == NULL) && (ilace != Y4M_ILACE_NONE))
    alloc_buffers(buffers2, width, height);

  mjpeg_debug("Reading rows");

  if ((ilace != Y4M_ILACE_NONE) && (ileave)) {
    /* Interlaced and Interleaved:
       --> read image and deinterleave fields at same time */
    if (ilace == Y4M_ILACE_TOP_FIRST) {
      /* 1st buff arg == top field == temporally first == "buffers" */
      read_ppm_into_two_buffers(fd, buffers, buffers2,
				rowbuffer, width, height, bgr);
    } else {
      /* bottom-field-first */
      /* 1st buff art == top field == temporally second == "buffers2" */
      read_ppm_into_two_buffers(fd, buffers2, buffers,
				rowbuffer, width, height, bgr);
    }      
  } else if ((ilace == Y4M_ILACE_NONE) || (!ileave)) {
    /* Not Interlaced, or Not Interleaved:
       --> read image into first buffer... */
    read_ppm_into_one_buffer(fd, buffers, rowbuffer, width, height, bgr);
    if ((ilace != Y4M_ILACE_NONE) && (!ileave)) {
      /* ...Actually Interlaced:
	 --> read the second image/field into second buffer */
      err = read_ppm_header(fd, &width, &height);
      if (err > 0) return 1;  /* EOF */
      if (err < 0) return -1; /* error */
      mjpeg_debug("Got PPM header:  %dx%d", width, height);
      
      /* make sure everything matches */
      if ( (ppm->width != width) ||
	   (ppm->height != height) )
	mjpeg_error_exit1("One of these frames is not like the others!");
      read_ppm_into_one_buffer(fd, buffers2, rowbuffer, width, height, bgr);
    }
  }
  return 0;
}



static
void setup_output_stream(int fdout, cl_info_t *cl,
			 y4m_stream_info_t *sinfo, ppm_info_t *ppm,
			 int *field_height) 
{
  int err;
  int x_alignment = y4m_chroma_ss_x_ratio(cl->ss_mode).d;
  int y_alignment = y4m_chroma_ss_y_ratio(cl->ss_mode).d;
    
  if (cl->interlace != Y4M_ILACE_NONE) 
    y_alignment *= 2;

  if ((ppm->width % x_alignment) != 0) {
    mjpeg_error_exit1("PPM width (%d) is not a multiple of %d!",
		      ppm->width, x_alignment);
  }
  if ((ppm->height % y_alignment) != 0) {
    mjpeg_error_exit1("PPM height (%d) is not a multiple of %d!",
		      ppm->height, y_alignment);
  }

  y4m_si_set_width(sinfo, ppm->width);
  if (cl->interlace == Y4M_ILACE_NONE) {
    y4m_si_set_height(sinfo, ppm->height);
    *field_height = ppm->height;
  } else if (cl->interleave) {
    y4m_si_set_height(sinfo, ppm->height);
    *field_height = ppm->height / 2;
  } else {
    y4m_si_set_height(sinfo, ppm->height * 2);
    *field_height = ppm->height;
  }
  y4m_si_set_sampleaspect(sinfo, cl->aspect);
  y4m_si_set_interlace(sinfo, cl->interlace);
  y4m_si_set_framerate(sinfo, cl->framerate);
  y4m_si_set_chroma(sinfo, cl->ss_mode);

  if ((err = y4m_write_stream_header(fdout, sinfo)) != Y4M_OK)
    mjpeg_error_exit1("Write header failed:  %s", y4m_strerr(err));

  mjpeg_info("Output Stream parameters:");
  y4m_log_stream_info(mjpeg_loglev_t("info"), "  ", sinfo);
}


int main(int argc, char **argv)
{
  cl_info_t cl;
  y4m_stream_info_t sinfo;
  y4m_frame_info_t finfo;
  uint8_t *buffers[Y4M_MAX_NUM_PLANES];  /* R'G'B' or Y'CbCr */
  uint8_t *buffers2[Y4M_MAX_NUM_PLANES]; /* R'G'B' or Y'CbCr */
  ppm_info_t ppm;
  int field_height;

  int fdout = 1;
  int err, i, count, repeating_last;

  y4m_accept_extensions(1);
  y4m_init_stream_info(&sinfo);
  y4m_init_frame_info(&finfo);

  parse_args(&cl, argc, argv);

  ppm.width = 0;
  ppm.height = 0;
  for (i = 0; i < 3; i++) {
    buffers[i] = NULL;
    buffers2[i] = NULL;
  }

  /* Read first PPM frame/field-pair, to get dimensions */
  if (read_ppm_frame(cl.fdin, &ppm, buffers, buffers2, 
		     cl.interlace, cl.interleave, cl.bgr))
    mjpeg_error_exit1("Failed to read first frame.");

  /* Setup streaminfo and write output header */
  setup_output_stream(fdout, &cl, &sinfo, &ppm, &field_height);

  /* Loop 'framecount' times, or possibly forever... */
  for (count = 0, repeating_last = 0;
       (count < (cl.offset + cl.framecount)) || (cl.framecount == 0);
       count++) {

    if (repeating_last) goto WRITE_FRAME;

    /* Read PPM frame/field */
    /* ...but skip reading very first frame, already read prior to loop */
    if (count > 0) {
      err = read_ppm_frame(cl.fdin, &ppm, buffers, buffers2, 
			   cl.interlace, cl.interleave, cl.bgr);
      if (err == 1) {
	/* clean input EOF */
	if (cl.repeatlast) {
	  repeating_last = 1;
	  goto WRITE_FRAME;
	} else if (cl.framecount != 0) {
	  mjpeg_error_exit1("Input frame shortfall (only %d converted).",
			    count - cl.offset);
	} else {
	  break;  /* input is exhausted; we are done!  time to go home! */
	}
      } else if (err)
	mjpeg_error_exit1("Error reading ppm frame");
    }
    
    /* ...skip transforms if we are just going to skip this frame anyway.
       BUT, if 'cl.repeatlast' is on, we must process/buffer every frame,
       because we don't know when we will see the last one. */
    if ((count >= cl.offset) || (cl.repeatlast)) {
      /* Transform colorspace, then subsample (in place) */
      convert_RGB_to_YCbCr(buffers, ppm.width * field_height);
      chroma_subsample(cl.ss_mode, buffers, ppm.width, field_height);
      if (cl.interlace != Y4M_ILACE_NONE) {
	convert_RGB_to_YCbCr(buffers2, ppm.width * field_height);
	chroma_subsample(cl.ss_mode, buffers2, ppm.width, field_height);
      }
    }

  WRITE_FRAME:
    /* Write converted frame to output */
    if (count >= cl.offset) {
      switch (cl.interlace) {
      case Y4M_ILACE_NONE:
	if ((err = y4m_write_frame(fdout, &sinfo, &finfo, buffers)) != Y4M_OK)
	  mjpeg_error_exit1("Write frame failed: %s", y4m_strerr(err));
	break;
      case Y4M_ILACE_TOP_FIRST:
	if ((err = y4m_write_fields(fdout, &sinfo, &finfo, buffers, buffers2))
	    != Y4M_OK)
	  mjpeg_error_exit1("Write fields failed: %s", y4m_strerr(err));
	break;
      case Y4M_ILACE_BOTTOM_FIRST:
	if ((err = y4m_write_fields(fdout, &sinfo, &finfo, buffers2, buffers))
	    != Y4M_OK)
	  mjpeg_error_exit1("Write fields failed: %s", y4m_strerr(err));
	break;
      default:
	mjpeg_error_exit1("Unknown ilace type!   %d", cl.interlace);
	break;
      }
    }
  } 


  for (i = 0; i < 3; i++) {
    free(buffers[i]);
    free(buffers2[i]);
  }
  y4m_fini_stream_info(&sinfo);
  y4m_fini_frame_info(&finfo);

  mjpeg_debug("Done.");
  return 0;
}
