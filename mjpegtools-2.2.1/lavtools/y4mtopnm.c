/*
 * y4mtopnm.c:  Convert a YUV4MPEG2 stream into one or more PPM/PGM/PAM images.
 *
 *              Converts ITU-Rec.601 Y'CbCr to R'G'B' colorspace
 *               (or Rec.601 Y' to [0,255] grayscale, for PGM).
 *
 *
 *  Copyright (C) 2005 Matthew J. Marjanovic <maddog@mir.com>
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

#include "config.h"

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


/* command-line parameters */
typedef struct _cl_info {
  int make_pam;
  int make_flat;
  int deinterleave;
  int verbosity;
  FILE *outfp;
} cl_info_t;



static
void usage(const char *progname)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "usage:  %s [options]\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "Reads YUV4MPEG2 stream from stdin and produces RAW PPM, PGM\n");
  fprintf(stderr, " or PAM image(s) on stdout.  Converts digital video Y'CbCr colorspace\n");
  fprintf(stderr, " to computer graphics R'G'B'.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, " options:  (defaults specified in [])\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -P    produce PAM output instead of PPM/PGM\n");
  fprintf(stderr, "  -D    de-interleave fields into two PNM images\n");
  fprintf(stderr, "  -f    'flatten' multiplane frames/fields in raw composite grayscale images\n");
  fprintf(stderr, "          (for stream debugging, see manpage for details)\n");
  fprintf(stderr, "  -v n  verbosity (0,1,2) [1]\n");
  fprintf(stderr, "  -h    print this help message\n");
}



static
void parse_args(cl_info_t *cl, int argc, char **argv)
{
  int c;

  cl->make_pam = 0;
  cl->make_flat = 0;
  cl->deinterleave = 0;
  cl->verbosity = 1;
  cl->outfp = stdout; /* default to stdout */

  while ((c = getopt(argc, argv, "PDfv:h")) != -1) {
    switch (c) {
    case 'P':
      cl->make_pam = 1;
      break;
    case 'D':
      cl->deinterleave = 1;
      break;
    case 'f':
      cl->make_flat = 1;
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
  if (optind != argc) 
    goto ERROR_EXIT;

  mjpeg_default_handler_verbosity(cl->verbosity);
  /* DONE! */
  return;

 ERROR_EXIT:
  mjpeg_error("For usage hints, use option '-h'.  Please take a hint.");
  exit(1);

}


/* Write a single PAM image. */
static
void write_pam(FILE *fp,
               int chroma,
               uint8_t *buffers[],
               uint8_t *rowbuffer,
               int width, int height) 
{
  mjpeg_debug("write raw PAM image from one buffer, %dx%d", width, height);
  fprintf(fp, "P7\n");
  fprintf(fp, "WIDTH %d\n", width);
  fprintf(fp, "HEIGHT %d\n", height);
  fprintf(fp, "MAXVAL 255\n");
  switch (chroma) {
  case Y4M_CHROMA_444:
    fprintf(fp, "DEPTH 3\n");
    fprintf(fp, "TUPLTYPE RGB\n");
    fprintf(fp, "ENDHDR\n");
    {
      int x, y;
      uint8_t *pixels;
      uint8_t *R = buffers[0];
      uint8_t *G = buffers[1];
      uint8_t *B = buffers[2];
      assert(rowbuffer != NULL);
      for (y = 0; y < height; y++) {
        pixels = rowbuffer;
        for (x = 0; x < width; x++) {
          *(pixels++) = *(R++);
          *(pixels++) = *(G++);
          *(pixels++) = *(B++);
        }
        fwrite(rowbuffer, sizeof(rowbuffer[0]), width * 3, fp);
      }
    }
    break;
  case Y4M_CHROMA_444ALPHA:
    fprintf(fp, "DEPTH 4\n");
    fprintf(fp, "TUPLTYPE RGB_ALPHA\n");
    fprintf(fp, "ENDHDR\n");
    {
      int x, y;
      uint8_t *pixels;
      uint8_t *R = buffers[0];
      uint8_t *G = buffers[1];
      uint8_t *B = buffers[2];
      uint8_t *A = buffers[3];
      assert(rowbuffer != NULL);
      for (y = 0; y < height; y++) {
        pixels = rowbuffer;
        for (x = 0; x < width; x++) {
          *(pixels++) = *(R++);
          *(pixels++) = *(G++);
          *(pixels++) = *(B++);
          *(pixels++) = *(A++);
        }
        fwrite(rowbuffer, sizeof(rowbuffer[0]), width * 4, fp);
      }
    }
    break;
  case Y4M_CHROMA_MONO:
    fprintf(fp, "DEPTH 1\n");
    fprintf(fp, "TUPLTYPE GRAYSCALE\n");
    fprintf(fp, "ENDHDR\n");
    fwrite(buffers[0], sizeof(buffers[0][0]), width * height, fp);
    break;
  }
  
}


/* Write a single PPM image. */
static
void write_ppm(FILE *fp,
               uint8_t *buffers[],
               uint8_t *rowbuffer,
               int width, int height) 
{
  int x, y;
  uint8_t *pixels;
  uint8_t *R = buffers[0];
  uint8_t *G = buffers[1];
  uint8_t *B = buffers[2];

  mjpeg_debug("write raw PPM image from one buffer, %dx%d", width, height);
  fprintf(fp, "P6\n%d %d 255\n", width, height);
  assert(rowbuffer != NULL);
  for (y = 0; y < height; y++) {
    pixels = rowbuffer;
    for (x = 0; x < width; x++) {
      *(pixels++) = *(R++);
      *(pixels++) = *(G++);
      *(pixels++) = *(B++);
    }
    fwrite(rowbuffer, sizeof(rowbuffer[0]), width * 3, fp);
  }
}


/* Write a single PGM image. */
static
void write_pgm(FILE *fp, uint8_t *buffer, int width, int height) 
{
  mjpeg_debug("write raw PGM image from one buffer, %dx%d", width, height);
  fprintf(fp, "P5\n%d %d 255\n", width, height);
  fwrite(buffer, sizeof(buffer[0]), width * height, fp);
}



/* Write a frame or field out as "an image".
   A frame/field with an alpha channel may become two images... */
static
void write_image(FILE *fp,
                 int make_pam,
                 int chroma,
                 uint8_t *buffers[],
                 uint8_t *rowbuffer,
                 int width, int height) 
{
  if (make_pam) {
    write_pam(fp, chroma, buffers, rowbuffer, width, height);
  } else {
    switch (chroma) {
    case Y4M_CHROMA_444:
      write_ppm(fp, buffers, rowbuffer, width, height);
      break;
    case Y4M_CHROMA_444ALPHA:
      write_ppm(fp, buffers, rowbuffer, width, height);
      write_pgm(fp, buffers[3], width, height);
      break;
    case Y4M_CHROMA_MONO:
      write_pgm(fp, buffers[0], width, height);
      break;
    default:
      assert(0);  break;
    }
  }
}

/************************************************************************/
/************** Structure/code for 'flattening' images ******************/
/************************************************************************/

typedef struct {
  int chroma;
  int planes;
  int image_w;
  int image_h;
  int flat_w;
  int flat_h;
  uint8_t *flat_buffer;
} flattener_t;

static
void init_flattener(flattener_t *fl,
                    int chroma, int planes, int width, int height)
{
  fl->chroma = chroma;
  fl->planes = planes;
  fl->image_w = width;
  fl->image_h = height;
  switch (chroma) {
  case Y4M_CHROMA_444:        
  case Y4M_CHROMA_444ALPHA:   
  case Y4M_CHROMA_MONO:       
  case Y4M_CHROMA_420JPEG:
  case Y4M_CHROMA_420MPEG2:
  case Y4M_CHROMA_420PALDV:   
  case Y4M_CHROMA_422:        fl->flat_w = width;           break;
  case Y4M_CHROMA_411:        fl->flat_w = 3 * width / 2;   break;
  default:
    assert(0); break;
  }
  switch (chroma) {
  case Y4M_CHROMA_444:        fl->flat_h = 3 * height;      break;
  case Y4M_CHROMA_444ALPHA:   fl->flat_h = 4 * height;      break; 
  case Y4M_CHROMA_MONO:       fl->flat_h = height;          break;
  case Y4M_CHROMA_420JPEG:
  case Y4M_CHROMA_420MPEG2:
  case Y4M_CHROMA_420PALDV:   fl->flat_h = 3 * height / 2;  break;
  case Y4M_CHROMA_422:        fl->flat_h = 2 * height;      break;
  case Y4M_CHROMA_411:        fl->flat_h = height;          break;
  default:
    assert(0); break;
  }
  fl->flat_buffer = (uint8_t *) malloc(fl->flat_w * fl->flat_h);
  assert(fl->flat_buffer != NULL);
}

static
void fini_flattener(flattener_t *fl)
{
  if (fl->flat_buffer != NULL) {
    free(fl->flat_buffer);
    fl->flat_buffer = NULL;
  }
}


static
void flatten(flattener_t *fl, uint8_t *img_buffers[])
{
  switch (fl->chroma) {
    /* Non-subsampled:  Y, Cb, Cr, (A) vertically appended */
  case Y4M_CHROMA_444ALPHA:
  case Y4M_CHROMA_444:
  case Y4M_CHROMA_MONO:
    {
      int planesize = fl->image_w * fl->image_h;
      uint8_t *fbuf = fl->flat_buffer;
      int p;
      for (p = 0; p < fl->planes; p++) {
        memcpy(fbuf, img_buffers[p], planesize);
        fbuf += planesize;
      }
    }
    break;
    /* 1/2 horizontal:  Y followed by (Cb,Cr) */
  case Y4M_CHROMA_422:
    {
      uint8_t *fbuf = fl->flat_buffer;
      uint8_t *ib1 = img_buffers[1];
      uint8_t *ib2 = img_buffers[2];
      int planesize = fl->image_w * fl->image_h;
      int halfwidth = fl->image_w / 2;
      int y;
      memcpy(fbuf, img_buffers[0], planesize);
      fbuf += planesize;
      for (y = 0; y < fl->image_h; y++) {
        memcpy(fbuf, ib1, halfwidth);
        fbuf += halfwidth;
        ib1 += halfwidth;
        memcpy(fbuf, ib2, halfwidth);
        fbuf += halfwidth;
        ib2 += halfwidth;
      }
    }
    break;
    /* 1/2 horizontal, 1/2 vertical:  Y followed by (Cb,Cr) */
  case Y4M_CHROMA_420JPEG:
  case Y4M_CHROMA_420MPEG2:
  case Y4M_CHROMA_420PALDV:
    {
      uint8_t *fbuf = fl->flat_buffer;
      uint8_t *ib1 = img_buffers[1];
      uint8_t *ib2 = img_buffers[2];
      int planesize = fl->image_w * fl->image_h;
      int halfwidth = fl->image_w / 2;
      int y;
      memcpy(fbuf, img_buffers[0], planesize);
      fbuf += planesize;
      for (y = 0; y < fl->image_h / 2; y++) {
        memcpy(fbuf, ib1, halfwidth);
        fbuf += halfwidth;
        ib1 += halfwidth;
        memcpy(fbuf, ib2, halfwidth);
        fbuf += halfwidth;
        ib2 += halfwidth;
      }
    }
    break;
    /* 1/4 horizontal:  Y, Cb, Cr horizontally appended */
  case Y4M_CHROMA_411:
    {
      uint8_t *fbuf = fl->flat_buffer;
      uint8_t *ib0 = img_buffers[0];
      uint8_t *ib1 = img_buffers[1];
      uint8_t *ib2 = img_buffers[2];
      int qtrwidth = fl->image_w / 4;
      int y;
      for (y = 0; y < fl->image_h; y++) {
        memcpy(fbuf, ib0, fl->image_w);
        fbuf += fl->image_w;
        ib0 += fl->image_w;
        memcpy(fbuf, ib1, qtrwidth);
        fbuf += qtrwidth;
        ib1 += qtrwidth;
        memcpy(fbuf, ib2, qtrwidth);
        fbuf += qtrwidth;
        ib2 += qtrwidth;
      }
    }
    break;
  default:
    assert(0); break;
  }
}


static
void write_flattened(flattener_t *fl, FILE *fp, int make_pam)
{
  write_image(fp, make_pam, Y4M_CHROMA_MONO,
              &(fl->flat_buffer), NULL /* rowbuffer */,
              fl->flat_w, fl->flat_h);
}



static
void convert_chroma(int chroma, uint8_t *buffers[], int width, int height)
{
  switch (chroma) {
  case Y4M_CHROMA_444:
    convert_YCbCr_to_RGB(buffers, width * height);
    break;
  case Y4M_CHROMA_444ALPHA:
    convert_YCbCr_to_RGB(buffers, width * height);
    convert_Y219_to_Y255(buffers[3], width * height);
    break;
  case Y4M_CHROMA_MONO:
    convert_Y219_to_Y255(buffers[0], width * height);
    break;
  default:
    assert(0);  break;
  }
}



int main(int argc, char **argv)
{
  int in_fd = 0;
  cl_info_t cl;
  y4m_stream_info_t streaminfo;
  y4m_frame_info_t frameinfo;
  uint8_t *buffers[Y4M_MAX_NUM_PLANES];  /* R'G'B' or Y'CbCr */
  uint8_t *buffers2[Y4M_MAX_NUM_PLANES]; /* R'G'B' or Y'CbCr */
  int err, i;
  int width, height;
  int interlace, chroma, planes;
  uint8_t *rowbuffer = NULL;
  int frame_count;
  flattener_t fl;

  y4m_accept_extensions(1); /* allow non-4:2:0 chroma */
  y4m_init_stream_info(&streaminfo);
  y4m_init_frame_info(&frameinfo);

  parse_args(&cl, argc, argv);

  if ((err = y4m_read_stream_header(in_fd, &streaminfo)) != Y4M_OK) {
    mjpeg_error("Couldn't read YUV4MPEG2 header: %s!", y4m_strerr(err));
    exit(1);
  }
  mjpeg_info("Input stream parameters:");
  y4m_log_stream_info(mjpeg_loglev_t("info"), "<<<", &streaminfo);

  width = y4m_si_get_width(&streaminfo);
  height = y4m_si_get_height(&streaminfo);
  interlace = y4m_si_get_interlace(&streaminfo);
  chroma = y4m_si_get_chroma(&streaminfo);
  planes = y4m_si_get_plane_count(&streaminfo);

  switch (chroma) {
  case Y4M_CHROMA_444:
  case Y4M_CHROMA_444ALPHA:
  case Y4M_CHROMA_MONO:
    break;
  default:
    if (!cl.make_flat) {
      mjpeg_error("Cannot handle input stream's chroma mode!");
      mjpeg_error_exit1("Input must be non-subsampled (e.g. 4:4:4).");
    }
  }
  
  /*** Tell the user what is going to happen ***/
  mjpeg_info("Output image parameters:");
  mjpeg_info("   format:  %s",
             (cl.make_pam) ? "PAM" :
             (cl.make_flat) ? "PGM" :
             (chroma == Y4M_CHROMA_444) ? "PPM" :
             (chroma == Y4M_CHROMA_444ALPHA) ? "PPM+PGM" :
             (chroma == Y4M_CHROMA_MONO) ? "PGM" : "???");
  mjpeg_info("  framing:  %s",
             (cl.deinterleave) ? "two images per frame (deinterleaved)" :
             "one image per frame (interleaved)");
  if (cl.deinterleave) {
    mjpeg_info("    order:  %s",
               (interlace == Y4M_ILACE_BOTTOM_FIRST) ? "bottom field first" :
               "top field first");
  }
  if (cl.make_flat) 
    mjpeg_info(" 'flattened' --> all planes in one image");

  /*** Allocate buffers ***/
  mjpeg_debug("allocating buffers...");
  if (cl.make_flat) {
    if (!cl.deinterleave) 
      init_flattener(&fl, chroma, planes, width, height);
    else 
      init_flattener(&fl, chroma, planes, width, height / 2);
    rowbuffer = NULL;
  } else {
    rowbuffer = malloc(width * planes * sizeof(rowbuffer[0]));
  }
  mjpeg_debug("  rowbuffer %p", rowbuffer);
  for (i = 0; i < planes; i++) {
    switch (interlace) {
    case Y4M_ILACE_NONE:
      buffers[i] = malloc(width * height * sizeof(buffers[i][0]));
      buffers2[i] = NULL;
      break;
    case Y4M_ILACE_TOP_FIRST:
    case Y4M_ILACE_BOTTOM_FIRST:
    case Y4M_ILACE_MIXED:
      /* 'buffers' may hold whole frame or one field... */
      buffers[i] = malloc(width * height * sizeof(buffers[i][0]));
      /* ... but 'buffers2' will never hold more than one field. */
      buffers2[i] = malloc(width * height / 2 * sizeof(buffers[i][0]));
      break;
    default:
      assert(0);
      break;
    }
    mjpeg_debug("  buffers[%d] %p   buffers2[%d] %p", 
                i, buffers[i], i, buffers2[i]);
  }

  /*** Process frames ***/
  frame_count = 0;
  while (1) {
    err = y4m_read_frame_header(in_fd, &streaminfo, &frameinfo);
    if (err != Y4M_OK) break;

    frame_count++;

    if (!cl.deinterleave) {
      mjpeg_debug("reading whole frame...");
      err = y4m_read_frame_data(in_fd, &streaminfo, &frameinfo, buffers);
      if (err != Y4M_OK) break;
      if (cl.make_flat) {
        flatten(&fl, buffers);
        write_flattened(&fl, cl.outfp, cl.make_pam);
      } else {
        convert_chroma(chroma, buffers, width, height);
        write_image(cl.outfp, cl.make_pam,
                    chroma, buffers, rowbuffer, width, height);
      }
    } else {
      mjpeg_debug("reading separate fields...");
      err = y4m_read_fields_data(in_fd, &streaminfo, &frameinfo, 
                                 buffers, buffers2);
      if (err != Y4M_OK) break;
      if (cl.make_flat) {
        switch (interlace) {
        case Y4M_ILACE_NONE:       /* ambiguous temporal order */
        case Y4M_ILACE_MIXED:      /* ambiguous temporal order */
        case Y4M_ILACE_TOP_FIRST:
          /* write top field first */
          flatten(&fl, buffers);
          write_flattened(&fl, cl.outfp, cl.make_pam);
          flatten(&fl, buffers2);
          write_flattened(&fl, cl.outfp, cl.make_pam);
          break;
        case Y4M_ILACE_BOTTOM_FIRST:
          /* write bottom field first */
          flatten(&fl, buffers2);
          write_flattened(&fl, cl.outfp, cl.make_pam);
          flatten(&fl, buffers);
          write_flattened(&fl, cl.outfp, cl.make_pam);
          break;
        default:
          mjpeg_error_exit1("Unknown input interlacing mode (%d).\n",
                            interlace);
          break;
        }
      } else {
        convert_chroma(chroma, buffers, width, height / 2);
        convert_chroma(chroma, buffers2, width, height / 2);
        switch (interlace) {
        case Y4M_ILACE_NONE:       /* ambiguous temporal order */
        case Y4M_ILACE_MIXED:      /* ambiguous temporal order */
        case Y4M_ILACE_TOP_FIRST:
          /* write top field first */
          write_image(cl.outfp, cl.make_pam,
                      chroma, buffers, rowbuffer, width, height / 2);
          write_image(cl.outfp, cl.make_pam,
                      chroma, buffers2, rowbuffer, width, height / 2);
          break;
        case Y4M_ILACE_BOTTOM_FIRST:
          /* write bottom field first */
          write_image(cl.outfp, cl.make_pam,
                      chroma, buffers2, rowbuffer, width, height / 2);
          write_image(cl.outfp, cl.make_pam,
                      chroma, buffers, rowbuffer, width, height / 2);
          break;
        default:
          mjpeg_error_exit1("Unknown input interlacing mode (%d).\n",
                            interlace);
          break;
        }
      }
    }
  }     
  if ((err != Y4M_OK) && (err != Y4M_ERR_EOF))
    mjpeg_error("Couldn't read frame:  %s", y4m_strerr(err));

  mjpeg_info("Processed %d frames.", frame_count);

  /*** Clean-up after ourselves ***/
  mjpeg_debug("freeing buffers; cleaning up");
  if (rowbuffer != NULL) 
    free(rowbuffer);
  for (i = 0; i < planes; i++) {
    free(buffers[i]);
    if (buffers2[i] != NULL)
      free(buffers2[i]);
  }
  y4m_fini_stream_info(&streaminfo);
  y4m_fini_frame_info(&frameinfo);
  if (cl.make_flat)
    fini_flattener(&fl);
  
  mjpeg_debug("Done.");
  return 0;
}


