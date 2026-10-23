/*
 * pnmtoy4m.c:  Generate a YUV4MPEG2 stream from one or more PNM/PAM images.
 *
 *              Converts R'G'B' to ITU-Rec.601 Y'CbCr colorspace, and/or
 *               converts [0,255] grayscale to Rec.601 Y' luma.
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
#include <assert.h>

#include <yuv4mpeg.h>
#include <mpegconsts.h>
#include "colorspace.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif


/* command-line parameters */
typedef struct _cl_info {
  y4m_ratio_t output_aspect;    
  y4m_ratio_t output_framerate; 
  int output_interlace;         
  int input_interlace;         
  int deinterleave;
  int offset;
  int framecount;
  int repeatlast;
  int verbosity;
  int fdin;
  int bgr;
} cl_info_t;





static
void usage(const char *progname)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "usage:  %s [options] [pnm-file]\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "Reads RAW PNM/PAM image(s), and produces YUV4MPEG2 stream on stdout.\n");
  fprintf(stderr, "Converts computer graphics R'G'B' colorspace to digital video Y'CbCr.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "If 'pnm-file' is not specified, reads from stdin.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, " options:  (defaults specified in [])\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -o n     frame offset (skip n input frames) [0]\n");
  fprintf(stderr, "  -n n     frame count (output n frames; 0 == all of them) [0]\n");
  fprintf(stderr, "  -r       repeat last input frame\n");
  fprintf(stderr, "  -D x     treat PNM images as de-interleaved fields, with:\n");
  fprintf(stderr, "             t = first-image-is-top-field\n");
  fprintf(stderr, "             b = first-image-is-bottom-field\n");
  fprintf(stderr, "  -F n:d   output framerate [30000:1001 = NTSC]\n");
  fprintf(stderr, "  -A w:h   output pixel aspect ratio [1:1]\n");
  fprintf(stderr, "  -I x     output interlacing [from -D, or p]\n");
  fprintf(stderr, "             p = none/progressive\n");
  fprintf(stderr, "             t = top-field-first\n");
  fprintf(stderr, "             b = bottom-field-first\n");
  fprintf(stderr, "  -v n     verbosity (0,1,2) [1]\n");
  fprintf(stderr, "  -B       pixels are packed in BGR(A) format [RGB(A)]\n");
}



static
void parse_args(cl_info_t *cl, int argc, char **argv)
{
  int c;

  cl->offset = 0;
  cl->framecount = 0;
  cl->output_aspect = y4m_sar_SQUARE;
  cl->output_interlace = Y4M_UNKNOWN;
  cl->output_framerate = y4m_fps_NTSC;
  cl->deinterleave = 0;
  cl->input_interlace = Y4M_ILACE_NONE;
  cl->repeatlast = 0;
  cl->verbosity = 1;
  cl->fdin = fileno(stdin); /* default to stdin */
  cl->bgr = 0;

  while ((c = getopt(argc, argv, "A:F:I:D:o:n:rv:Bh")) != -1) {
    switch (c) {
    case 'A':
      if (y4m_parse_ratio(&(cl->output_aspect), optarg) != Y4M_OK) {
	mjpeg_error("Could not parse ratio:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'F':
      if (y4m_parse_ratio(&(cl->output_framerate), optarg) != Y4M_OK) {
	mjpeg_error("Could not parse ratio:  '%s'", optarg);
	goto ERROR_EXIT;
      }
      break;
    case 'I':
      switch (optarg[0]) {
      case 'p':  cl->output_interlace = Y4M_ILACE_NONE;  break;
      case 't':  cl->output_interlace = Y4M_ILACE_TOP_FIRST;  break;
      case 'b':  cl->output_interlace = Y4M_ILACE_BOTTOM_FIRST;  break;
      default:
	mjpeg_error("Unknown value for output interlace: '%c'", optarg[0]);
	goto ERROR_EXIT;
	break;
      }
      break;
    case 'D':
      cl->deinterleave = 1;
      switch (optarg[0]) {
      case 't':  cl->input_interlace = Y4M_ILACE_TOP_FIRST;  break;
      case 'b':  cl->input_interlace = Y4M_ILACE_BOTTOM_FIRST;  break;
      default:
	mjpeg_error("Unknown value for input interlace: '%c'", optarg[0]);
	goto ERROR_EXIT;
	break;
      }
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
    case 'v':
      cl->verbosity = atoi(optarg);
      if ((cl->verbosity < 0) || (cl->verbosity > 2))
	mjpeg_error("Verbosity must be 0, 1, or 2:  '%s'", optarg);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
      break;
    case 'B':
      cl->bgr = 1;
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

  /* output ilace defaults to match input (or none if input is interleaved) */
  if (cl->output_interlace == Y4M_UNKNOWN) {
    if (cl->deinterleave)
      cl->output_interlace = cl->input_interlace;
    else
      cl->output_interlace = Y4M_ILACE_NONE;
  }

  mjpeg_info("Command-line Parameters:");
  if (!cl->deinterleave) {
    mjpeg_info("  input format:  interleaved");
  } else {
    mjpeg_info("  input format:  field-sequential, %s",
               (cl->input_interlace == Y4M_ILACE_TOP_FIRST) ?
               "first-image-is-top" :
               "first-image-is-bottom");
  }
  mjpeg_info("    pixel packing:  %s", (cl->bgr?"BGR(A)":"RGB(A)")); 
  mjpeg_info(" output framerate:  %d:%d",
	     cl->output_framerate.n, cl->output_framerate.d);
  mjpeg_info("       output SAR:  %d:%d",
	     cl->output_aspect.n, cl->output_aspect.d);
  mjpeg_info(" output interlace:  %s",
	     mpeg_interlace_code_definition(cl->output_interlace));
  mjpeg_info("   starting frame:  %d", cl->offset);
  if (cl->framecount == 0)
    mjpeg_info("    # of frames:  all%s",
	       (cl->repeatlast) ? ", repeat last frame forever" :
	       ", until input exhausted");
  else
    mjpeg_info("    # of frames:  %d%s",
	       cl->framecount,
	       (cl->repeatlast) ? ", repeat last frame until done" :
	       ", or until input exhausted");
  /* DONE! */
  return;

 ERROR_EXIT:
  mjpeg_error("For usage hints, use option '-h'.  Please take a hint.");
  exit(1);

}


/* the various PNM formats we handle */
typedef enum {
  FMT_PAM = 0,
  FMT_PPM_RAW,
  FMT_PGM_RAW,
  FMT_PBM_RAW,
  FMT_PPM_PLAIN,
  FMT_PGM_PLAIN,
  FMT_PBM_PLAIN,
  FMT_UNKNOWN
} pnm_format_t;

#define FMT_FIRST FMT_PAM
#define FMT_COUNT FMT_UNKNOWN

/* 'magic numbers' of the PNM formats (corresponding to enum above) */
const char *magics[FMT_COUNT+1] = {
  "P7",
  "P6",
  "P5",
  "P4",
  "P3",
  "P2",
  "P1",
  "*UNKNOWN*"
};

/* the various PAM "tupls" we handle */
typedef enum {
  TUPL_RGB = 0,
  TUPL_GRAY,
  TUPL_RGB_ALPHA,
  TUPL_GRAY_ALPHA,
  TUPL_BW,
  TUPL_BW_ALPHA,
  TUPL_UNKNOWN
} pam_tupl_t;

#define TUPL_FIRST TUPL_RGB
#define TUPL_COUNT TUPL_UNKNOWN

struct tupl_info {
  const char *tag;
  int depth;
};

/* descriptions of the PAM tupls (corresponding to enum above) */
const struct tupl_info tupls[TUPL_COUNT+1] = {
  { "RGB",                  3 },
  { "GRAYSCALE",            1 },
  { "RGB_ALPHA",            4 },
  { "GRAYSCALE_ALPHA",      2 },
  { "BLACKANDWHITE",        1 },
  { "BLACKANDWHITE_ALPHA",  2 },
  { "*UNKNOWN*",            0 }
};

/* PNM image information */
typedef struct _pnm_info {
  pnm_format_t format;
  int width;
  int height;
  int depth;
  pam_tupl_t tupl;
} pnm_info_t;


static
int pnm_info_equal(pnm_info_t *a, pnm_info_t *b)
{
  return ((a->format == b->format) &&
          (a->width  == b->width)  &&
          (a->height == b->height) &&
          (a->depth  == b->depth) &&
          (a->tupl   == b->tupl));
}

static
void pnm_info_copy(pnm_info_t *dst, pnm_info_t *src)
{
  dst->format = src->format;
  dst->width  = src->width;
  dst->height = src->height;
  dst->depth  = src->depth;
  dst->tupl   = src->tupl;
}



static
char *read_tag_and_value(int fd, char *line, int maxlen)
{
  int found_end_of_tag = 0;
  char *v = NULL;
  int n;

  while ( (maxlen > 0) &&
          ((n = read(fd, line, 1)) == 1) &&
          (*line != '\n') ) {

    if (!found_end_of_tag) {
      if (isspace(*line)) {
        *line = '\0';
        found_end_of_tag = 1;
      }
    } else if (v == NULL) {
      if (!isspace(*line))
        v = line;
    }
    line++;
  }
  if (maxlen <= 0) return NULL;
  if (n != 1) return NULL;
  *line = '\0';
  if (v == NULL) return line;
  return v;
}




static
int read_pam_header(int fd, pnm_info_t *pnm)
{
  char line[128];
  int done = 0;
  int maxval = 0;
  char *val;

  pnm->width = 0;
  pnm->height = 0;
  pnm->depth = 0;
  pnm->tupl = TUPL_UNKNOWN;

  while (!done && ((val = read_tag_and_value(fd, line, 128)) != NULL)) {
    if (!strcmp(line, "ENDHDR")) {
      done = 1;
    } else if (!strcmp(line, "HEIGHT")) {
      pnm->height = atoi(val);
    } else if (!strcmp(line, "WIDTH")) {
      pnm->width = atoi(val);
    } else if (!strcmp(line, "DEPTH")) {
      pnm->depth = atoi(val);
    } else if (!strcmp(line, "MAXVAL")) {
      maxval = atoi(val);
    } else if (!strcmp(line, "TUPLTYPE")) {
      if (pnm->tupl != TUPL_UNKNOWN) 
        mjpeg_error_exit1("Too many PAM TUPLTYPE's: %s", val);
      for (pnm->tupl = TUPL_FIRST;
           pnm->tupl < TUPL_UNKNOWN; 
           (pnm->tupl)++) {
        if (!(strcmp(val, tupls[pnm->tupl].tag))) break;
      }
      if (pnm->tupl == TUPL_UNKNOWN)
        mjpeg_error_exit1("Unknown PAM TUPLTYPE: %s", val);
    }
  }
  if ( (pnm->width == 0) || (pnm->height == 0) || 
       (pnm->depth == 0) || (maxval == 0) )
    mjpeg_error_exit1("Bad PAM header!\n");
  
  if (pnm->tupl == TUPL_UNKNOWN) {
    for (pnm->tupl = TUPL_FIRST;
         pnm->tupl < TUPL_UNKNOWN;
         (pnm->tupl)++) {
      if (pnm->depth == tupls[pnm->tupl].depth) break;
    }
    if (pnm->tupl == TUPL_UNKNOWN) 
      mjpeg_error_exit1("No PAM TUPL for depth %d!", pnm->depth);
  } else {
    if (pnm->depth != tupls[pnm->tupl].depth)
      mjpeg_error_exit1("PAM depth mismatch:  %d != %d for %s.",
                        pnm->depth,
                        tupls[pnm->tupl].depth, tupls[pnm->tupl].tag);
  }
  if (maxval != 255)
    mjpeg_error_exit1("Expecting maxval == 255, not %d!", maxval);
  return 0;
}



#define DO_READ_NUMBER(var)                                     \
  do {                                                          \
    if (!isdigit(s[0]))                                         \
      mjpeg_error_exit1("PNM read error:  bad char  %d", s[0]); \
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


/*
 * returns:  0 - success, got header
 *           1 - EOF, no new frame
 *          -1 - failure (actually, just errors out...)
 */

static
int read_pnm_header(int fd, pnm_info_t *pnm)
{
  char s[6];
  int incomment;
  int n;
  int maxval = 0;
  
  pnm->width = 0;
  pnm->height = 0;
  pnm->depth = 0;

  /* look for MAGIC */
  n = y4m_read(fd, s, 3);
  if (n > 0) 
    return 1;  /* EOF */
  if (n < 0)
    mjpeg_error_exit1("Bad PNM header magic!");

  for (pnm->format = FMT_FIRST;
       pnm->format < FMT_UNKNOWN; 
       (pnm->format)++) {
    if (!(strncmp(s, magics[pnm->format], 2))) break;
  }
  if ( (pnm->format == FMT_UNKNOWN) ||
       (!isspace(s[2])) )
    mjpeg_error_exit1("Bad PNM magic!");

  if (pnm->format == FMT_PAM) {
    if (s[2] != '\n')
      mjpeg_error_exit1("Bad PAM magic!");
    return read_pam_header(fd, pnm);
  } else {
    pnm->tupl = TUPL_UNKNOWN;
    incomment = 0;
    DO_SKIP_WHITESPACE();
    DO_READ_NUMBER(pnm->width);
    DO_SKIP_WHITESPACE();
    DO_READ_NUMBER(pnm->height);
    if ((pnm->format != FMT_PBM_RAW) && (pnm->format != FMT_PBM_PLAIN)) {
      DO_SKIP_WHITESPACE();
      DO_READ_NUMBER(maxval);
      if (maxval != 255)
        mjpeg_error_exit1("Expecting maxval == 255, not %d!", maxval);
    }
    switch (pnm->format) {
    case FMT_PPM_RAW: 
    case FMT_PPM_PLAIN:
      pnm->depth = 3;
      break;
    case FMT_PGM_RAW:
    case FMT_PBM_RAW:
    case FMT_PGM_PLAIN:
    case FMT_PBM_PLAIN:
      pnm->depth = 1;
      break;
    default:
      assert(0);
      break;
    }
  }
  return 0;
}



static
void alloc_buffers(uint8_t *buffers[], int width, int height, int depth)
{
  mjpeg_debug("Alloc'ing buffers");
  buffers[0] = malloc(width * height * sizeof(buffers[0][0]));
  if (depth > 1) {
    buffers[1] = malloc(width * height * sizeof(buffers[1][0]));
    buffers[2] = malloc(width * height * sizeof(buffers[2][0]));
  }
  if (depth > 3) 
    buffers[3] = malloc(width * height * sizeof(buffers[3][0]));
}



static
void read_rgba_raw(int fd, uint8_t *buffers[],
                   uint8_t *rowbuffer, int width, int height, int bgra) 
{
  int x, y;
  uint8_t *pixels;
  uint8_t *R = buffers[0];
  uint8_t *G = buffers[1];
  uint8_t *B = buffers[2];
  uint8_t *A = buffers[3];

  for (y = 0; y < height; y++) {
    pixels = rowbuffer;
    y4m_read(fd, pixels, width * 4);
    if (!bgra) {
      for (x = 0; x < width; x++) {
	*(R++) = *(pixels++);
	*(G++) = *(pixels++);
	*(B++) = *(pixels++);
	*(A++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(B++) = *(pixels++);
	*(G++) = *(pixels++);
	*(R++) = *(pixels++);
	*(A++) = *(pixels++);
      }
    }
  }
}


static
void read_ppm_raw(int fd, uint8_t *buffers[],
                  uint8_t *rowbuffer, int width, int height, int bgr) 
{
  int x, y;
  uint8_t *pixels;
  uint8_t *R = buffers[0];
  uint8_t *G = buffers[1];
  uint8_t *B = buffers[2];

  for (y = 0; y < height; y++) {
    pixels = rowbuffer;
    y4m_read(fd, pixels, width * 3);
    if (!bgr) {
      for (x = 0; x < width; x++) {
	*(R++) = *(pixels++);
	*(G++) = *(pixels++);
	*(B++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(B++) = *(pixels++);
	*(G++) = *(pixels++);
	*(R++) = *(pixels++);
      }
    }
  }
}


static
void read_pgm_raw(int fd, uint8_t *buffer, int width, int height) 
{
  mjpeg_debug("read PGM into one buffer, %dx%d", width, height);
  y4m_read(fd, buffer, width * height);
}

#define Y_BLACK 16
#define Y_WHITE 219

/*
     000000000011111111112222222222
     012345678901234567890123456789
 
     76543210765432107654321076543210
*/


static
void read_pbm_raw(int fd, uint8_t *buffer, int width, int height) 
{
  int row_bytes = (width + 7) >> 3;
  int total_bytes = row_bytes * height;
  uint8_t *pbm  = buffer + total_bytes - 1;
  uint8_t *luma = buffer + (width * height) - 1;
  int x, y;

  mjpeg_debug("read PBM into one buffer, %dx%d", width, height);
  y4m_read(fd, buffer, total_bytes);

  for (y = height - 1; y >= 0; y--) {
    for (x = width - 1; x >= 0; ) {

      int shift = 7 - (x % 8);
      uint8_t bits = *(pbm--) >> shift;
      while (shift < 8) {
        *(luma--) = (bits & 0x1) ? Y_BLACK : Y_WHITE;
        bits >>= 1;
        shift++;
        x--;
      }

    }
  }
}



static
void read_pnm_data(int fd, uint8_t *planes[],
                   uint8_t *rowbuffer, pnm_info_t *pnm, int bgr)
{
  switch (pnm->format) {
  case FMT_PPM_RAW:
    read_ppm_raw(fd, planes, rowbuffer, pnm->width, pnm->height, bgr);
    break;
  case FMT_PGM_RAW:
    read_pgm_raw(fd, planes[0], pnm->width, pnm->height);
    break;
  case FMT_PBM_RAW:
    read_pbm_raw(fd, planes[0], pnm->width, pnm->height);
    break;
  case FMT_PAM:
    switch (pnm->tupl) {
    case TUPL_RGB:
      read_ppm_raw(fd, planes, rowbuffer, pnm->width, pnm->height, bgr);
      break;
    case TUPL_GRAY:
      read_pgm_raw(fd, planes[0], pnm->width, pnm->height);
      break;
    case TUPL_RGB_ALPHA:
      read_rgba_raw(fd, planes, rowbuffer, pnm->width, pnm->height, bgr);
      break;
    case TUPL_GRAY_ALPHA:
    case TUPL_BW:
    case TUPL_BW_ALPHA:
    case TUPL_UNKNOWN:
      mjpeg_error_exit1("Unknown/unhandled PAM tuple/format.");
      break;
    }
    break;
  case FMT_PBM_PLAIN:
  case FMT_PGM_PLAIN:
  case FMT_PPM_PLAIN:
  case FMT_UNKNOWN:
    mjpeg_error_exit1("Unknown/unhandled PNM format.");
    break;
  }
}




/* 
 * Read one whole frame.
 *
 * If non-interleaved fields, this requires reading two PNM images.
 *
 */

static
int read_pnm_frame(int fd, pnm_info_t *pnm,
		   uint8_t *buffers[], uint8_t *buffers2[],
		   int de_leaved, int bgr)
{
  static uint8_t *rowbuffer = NULL;
  pnm_info_t new_pnm;
  int err;

  err = read_pnm_header(fd, &new_pnm); //&format, &width, &height);
  if (err > 0) return 1;  /* EOF */
  if (err < 0) return -1; /* error */
  mjpeg_debug("Got PNM header:  [%s%s%s] %dx%d",
              magics[new_pnm.format],
              (new_pnm.format == FMT_PAM) ? " " : "",
              (new_pnm.format == FMT_PAM) ? tupls[new_pnm.tupl].tag : "",
              new_pnm.width, new_pnm.height);

  if (pnm->width == 0) { /* first time */
    mjpeg_debug("Initializing PNM read_frame");
    pnm_info_copy(pnm, &new_pnm);
    rowbuffer = malloc(new_pnm.width * new_pnm.depth * sizeof(rowbuffer[0]));
  } else {
    /* make sure everything matches */
    if ( !pnm_info_equal(pnm, &new_pnm) )
      mjpeg_error_exit1("One of these frames is not like the others!");
  }
  if (buffers[0] == NULL) 
    alloc_buffers(buffers, new_pnm.width, new_pnm.height, new_pnm.depth);
  if ((buffers2[0] == NULL) && (de_leaved))
    alloc_buffers(buffers2, new_pnm.width, new_pnm.height, new_pnm.depth);

  /* Interleaved or not --> read image into first buffer... */
  read_pnm_data(fd, buffers, rowbuffer, pnm, bgr);

  if (de_leaved) {
    /* Really Non-Interleaved:  --> read second field into second buffer */
    err = read_pnm_header(fd, &new_pnm); //&format, &width, &height);
    if (err > 0) return 1;  /* EOF */
    if (err < 0) return -1; /* error */
    mjpeg_debug("Got second PNM header:  [%s] %dx%d",
                magics[new_pnm.format], new_pnm.width, new_pnm.height);
    /* make sure everything matches */
    if ( !pnm_info_equal(pnm, &new_pnm) )
      mjpeg_error_exit1("One of these fields is not like the others!");
    read_pnm_data(fd, buffers2, rowbuffer, pnm, bgr);
  }

  return 0;
}



static
void setup_output_stream(int fdout, cl_info_t *cl,
			 y4m_stream_info_t *sinfo, pnm_info_t *pnm,
			 int *field_height) 
{
  int err;
  int ss_mode = Y4M_UNKNOWN;

  if ( (cl->output_interlace != Y4M_ILACE_NONE) &&
       (!cl->deinterleave) &&
       ((pnm->height % 2) != 0) )
    mjpeg_error_exit1("Frame height (%d) is not a multiple of 2!",
                      pnm->height);

  y4m_si_set_width(sinfo, pnm->width);
  if (cl->deinterleave)
    y4m_si_set_height(sinfo, pnm->height * 2);
  else 
    y4m_si_set_height(sinfo, pnm->height);
  
  y4m_si_set_sampleaspect(sinfo, cl->output_aspect);
  y4m_si_set_interlace(sinfo, cl->output_interlace);
  y4m_si_set_framerate(sinfo, cl->output_framerate);
  switch (pnm->format) {
  case FMT_PPM_RAW:
  case FMT_PPM_PLAIN:
    ss_mode = Y4M_CHROMA_444;  break;
  case FMT_PGM_RAW:
  case FMT_PGM_PLAIN:
  case FMT_PBM_RAW:
  case FMT_PBM_PLAIN:
    ss_mode = Y4M_CHROMA_MONO; break;
  case FMT_PAM:
    switch (pnm->tupl) {
    case TUPL_RGB:
      ss_mode = Y4M_CHROMA_444;       break;
    case TUPL_GRAY:
      ss_mode = Y4M_CHROMA_MONO;      break;
    case TUPL_RGB_ALPHA:
      ss_mode = Y4M_CHROMA_444ALPHA;  break;
    case TUPL_GRAY_ALPHA:
    case TUPL_BW:
    case TUPL_BW_ALPHA:
    case TUPL_UNKNOWN:
      assert(0);
      break;
    }
    break;
  case FMT_UNKNOWN:
    assert(0);
    break;
  }
  y4m_si_set_chroma(sinfo, ss_mode);

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
  uint8_t *planes[Y4M_MAX_NUM_PLANES];  /* R'G'B' or Y'CbCr */
  uint8_t *planes2[Y4M_MAX_NUM_PLANES]; /* R'G'B' or Y'CbCr */
  pnm_info_t pnm;
  int field_height;

  int fdout = fileno(stdout);
  int err, i, count, repeating_last;

  y4m_accept_extensions(1);
  y4m_init_stream_info(&sinfo);
  y4m_init_frame_info(&finfo);

  parse_args(&cl, argc, argv);

  pnm.width = 0;
  pnm.height = 0;
  for (i = 0; i < Y4M_MAX_NUM_PLANES; i++) {
    planes[i] = NULL;
    planes2[i] = NULL;
  }

  /* Read first PPM frame/field-pair, to get dimensions/format. */
  if (read_pnm_frame(cl.fdin, &pnm, planes, planes2, cl.deinterleave, cl.bgr))
    mjpeg_error_exit1("Failed to read first frame.");

  /* Setup streaminfo and write output header */
  setup_output_stream(fdout, &cl, &sinfo, &pnm, &field_height);

  /* Loop 'framecount' times, or possibly forever... */
  for (count = 0, repeating_last = 0;
       (count < (cl.offset + cl.framecount)) || (cl.framecount == 0);
       count++) {

    if (repeating_last) goto WRITE_FRAME;

    /* Read PPM frame/field */
    /* ...but skip reading very first frame, already read prior to loop */
    if (count > 0) {
      err = read_pnm_frame(cl.fdin, &pnm, planes, planes2, cl.deinterleave, cl.bgr);
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
	mjpeg_error_exit1("Error reading pnm frame");
    }
    
    /* ...skip transforms if we are just going to skip this frame anyway.
       BUT, if 'cl.repeatlast' is on, we must process/buffer every frame,
       because we don't know when we will see the last one. */
    if ((count >= cl.offset) || (cl.repeatlast)) {
      /* Transform colorspace. */
      switch (pnm.format) {
      case FMT_PPM_PLAIN:
      case FMT_PPM_RAW:
        convert_RGB_to_YCbCr(planes, pnm.width * pnm.height);
        if (cl.deinterleave)
          convert_RGB_to_YCbCr(planes2, pnm.width * pnm.height);
        break;
      case FMT_PGM_PLAIN:
      case FMT_PGM_RAW:
        convert_Y255_to_Y219(planes[0], pnm.width * pnm.height);
        if (cl.deinterleave)
          convert_Y255_to_Y219(planes2[0], pnm.width * pnm.height);
        break;
      case FMT_PBM_PLAIN:
      case FMT_PBM_RAW:
        /* all set.  (converted at read time) */
        break;
      case FMT_PAM:
        switch (pnm.tupl) {
        case TUPL_RGB:
          convert_RGB_to_YCbCr(planes, pnm.width * pnm.height);
          if (cl.deinterleave)
            convert_RGB_to_YCbCr(planes2, pnm.width * pnm.height);
          break;
        case TUPL_GRAY:
          convert_Y255_to_Y219(planes[0], pnm.width * pnm.height);
          if (cl.deinterleave)
            convert_Y255_to_Y219(planes2[0], pnm.width * pnm.height);
          break;
        case TUPL_RGB_ALPHA:
          convert_RGB_to_YCbCr(planes, pnm.width * pnm.height);
          convert_Y255_to_Y219(planes[3], pnm.width * pnm.height);
          if (cl.deinterleave) {
            convert_RGB_to_YCbCr(planes2, pnm.width * pnm.height);
            convert_Y255_to_Y219(planes2[3], pnm.width * pnm.height);
          }
          break;
        case TUPL_GRAY_ALPHA:
        case TUPL_BW:
        case TUPL_BW_ALPHA:
        case TUPL_UNKNOWN:
          assert(0);
          break;
        }
        break;
      case FMT_UNKNOWN:
        assert(0);
        break;
      }
    }

  WRITE_FRAME:
    /* Write converted frame to output */
    if (count >= cl.offset) {
      if (cl.deinterleave) {
        switch (cl.input_interlace) {
        case Y4M_ILACE_TOP_FIRST:
          err = y4m_write_fields(fdout, &sinfo, &finfo, planes, planes2);
          break;
        case Y4M_ILACE_BOTTOM_FIRST:
          err = y4m_write_fields(fdout, &sinfo, &finfo, planes2, planes);
          break;
        case Y4M_ILACE_NONE:
        default:
          assert(0);
          break;
        }
      } else {
	err = y4m_write_frame(fdout, &sinfo, &finfo, planes);
      }
      if (err != Y4M_OK)
        mjpeg_error_exit1("Write frame/fields failed: %s", y4m_strerr(err));
    }
  } 


  for (i = 0; i < Y4M_MAX_NUM_PLANES; i++) {
    if (planes[i] != NULL)  free(planes[i]);
    if (planes2[i] != NULL) free(planes2[i]);
  }
  y4m_fini_stream_info(&sinfo);
  y4m_fini_frame_info(&finfo);

  mjpeg_debug("Done.");
  return 0;
}






#if 0
static
void read_two_ppm_raw(int fd,
                      uint8_t *buffers[], uint8_t *buffers2[], 
                      uint8_t *rowbuffer, int width, int height, int bgr)
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
    if (!bgr) {
      for (x = 0; x < width; x++) {
	*(R++) = *(pixels++);
	*(G++) = *(pixels++);
	*(B++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(B++) = *(pixels++);
	*(G++) = *(pixels++);
	*(R++) = *(pixels++);
      }
    }
    pixels = rowbuffer;
    if (y4m_read(fd, pixels, width * 3))
      mjpeg_error_exit1("read error B  y=%d", y);
    if (!bgr) {
      for (x = 0; x < width; x++) {
	*(R2++) = *(pixels++);
	*(G2++) = *(pixels++);
	*(B2++) = *(pixels++);
      }
    } else {
      for (x = 0; x < width; x++) {
	*(B2++) = *(pixels++);
	*(G2++) = *(pixels++);
	*(R2++) = *(pixels++);
      }
    }
  }
}

static
void read_two_pgm_raw(int fd,
                      uint8_t *buffer, uint8_t *buffer2, 
                      int width, int height)
{
  int y;
  mjpeg_debug("read PGM into two buffers, %dx%d", width, height);
  for (y = 0; y < height; y += 2) {
    if (y4m_read(fd, buffer, width))
      mjpeg_error_exit1("read error A  y=%d", y);
    buffer += width;
    if (y4m_read(fd, buffer2, width))
      mjpeg_error_exit1("read error B  y=%d", y);
    buffer2 += width;
  }
}


static
void read_pnm_into_two_buffers(int fd,
                               uint8_t *planes[],
                               uint8_t *planes2[],
                               uint8_t *rowbuffer,
                               pnm_info_t *pnm, int bgr)
{
  switch (pnm->format) {
  case FMT_PPM_RAW:
    read_two_ppm_raw(fd, planes, planes2, rowbuffer, pnm->width, pnm->height, bgr);
    break;
  case FMT_PGM_RAW:
    read_two_pgm_raw(fd, planes[0], planes2[0], pnm->width, pnm->height);
    break;
  case FMT_PBM_RAW:
    //    read_two_pbm_raw(fd, planes[0], planes2[0], pnm->width, pnm->height);
    break;
  case FMT_PBM_PLAIN:
  case FMT_PGM_PLAIN:
  case FMT_PPM_PLAIN:
  case FMT_UNKNOWN:
    assert(0);
    break;
  }
}
#endif

