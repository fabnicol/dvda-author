/*
 *  Copyright (C) 2001,2002 Kawamata/Hitoshi <hitoshi.kawamata@nifty.ne.jp>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <mpegtimecode.h>
#include "yuvfilters.h"

#define NOISERATIO(N) (((N)+63)/64*63)
#define MAXHALFHEIGHT 288
#define NOISEMAX 31
#define DEINTLRESO 1000
#define DEFAULTDTHR16 4
#define DEFAULTNOISE 10
#define DEFAULTDEINTL 10

static void
buf_debug(char *buf, FILE *fp, const char *format, ...)
{
  size_t n;
  va_list ap;
  va_start(ap, format);
  vsprintf(buf + strlen(buf), format, ap);
  va_end(ap);
  if (buf[(n = strlen(buf) - 1)] == '\n') {
    if (fp) {
      fputs(buf, fp);
    } else {
#ifdef MJPEGTOOLS
      buf[n] = '\0';
      mjpeg_debug(buf);
#else
      fputs(buf, stderr);
#endif
    }
    buf[0] = '\0';
  }
}

typedef struct {
  YfTaskCore_t _;
  int fpscode0;
  FILE *cyfp;
  int cytype;
  int nfields;
  int nframes;
  int iget;
  int iuse;
  int iput;
  YfFrame_t *frame;
  int dthr16;
  int deintl;
  int nointlmax, deintlmin;
  unsigned int deintlframes;
  unsigned int deintldist[100];
  union {
    struct {			/* use in 1st try */
      unsigned long dist[NOISEMAX+1];
      unsigned long total;
      unsigned int level0, level;
    } noise;
    struct {			/* use in retry */
      char buff[32];
      char *p;
    } cy;
  } u;
  struct {
    unsigned long odiff;	/* odd field (or non-intarlaced frame) diff */
    unsigned long ediff;	/* even field diff (only when interlaced) */
    long eoediff;		/* previous even - odd - even diff */
    unsigned long diffdist[NOISEMAX+1];
  } framestat[0];
} YfTask_t;

DEFINE_STD_YFTASKCLASS(yuvkineco);
DECLARE_YFTASKCLASS(yuvycsnoise);

static const char *
do_usage(void)
{
  return "[-{u|s}] [-F OutputFPSCODE] [-S YCSNoiseThreashold] [-n NoiseLevel] "
    "[-c CycleSearchThreashold] [-i DeinterlacePixelsPermil] "
    "{[-C OutputCycleListName] | -[ON] InputCycleListName}";
}

static YfTaskCore_t *
do_init(int argc, char **argv, const YfTaskCore_t *h0)
{
  int fpscode;
  int nframes, nfields;
  YfTask_t *h;
  int c;
  int cytype = 0;
  char *cyname = NULL;
  char *ycsthres = NULL;
  YfTaskCore_t *hycs = NULL;
  int dthr16 = DEFAULTDTHR16;
  int noiselevel = DEFAULTNOISE;
  int deintlval = DEFAULTDEINTL;
  int output_interlace = 0;

  fpscode = h0->fpscode;
  while ((c = getopt(argc, argv, "usF:S:n:c:i:C:O:N:")) != -1) {
    switch (c) {
    case 'u':
      output_interlace = 1;
      break;
    case 's':
      output_interlace = 2;
      break;
    case 'F':
      fpscode = atoi(optarg);
      break;
    case 'S':
      ycsthres = optarg;
      break;
    case 'n':
      noiselevel = atoi(optarg);
      break;
    case 'c':
      dthr16 = atoi(optarg);
      break;
    case 'i':
      deintlval = atoi(optarg);
      break;
    case 'C':
    case 'O':
    case 'N':
      cytype = c;
      cyname = optarg;
      break;
    default:
      return NULL;
    }
  }
  if (cytype != 'N' && (h0->fpscode < 3 || 5 < h0->fpscode)) {
    WERROR("unsupported input fps");
    return NULL;
  }
  if (fpscode < 1 || h0->fpscode < fpscode) {
    WERROR("illegal output fpscode");
    return NULL;
  }
  if (y4m_si_get_interlace(&h0->si) == Y4M_ILACE_BOTTOM_FIRST) {
    WERROR("unsupported field order");
    return NULL;
  }
  if (ycsthres) {
    char ycssopt[] = "-S";
    char *ycsargv[] = { argv[0], ycssopt, ycsthres, NULL, };
    optind = 1;
    if (!(hycs = YfAddNewTask(&yuvycsnoise,
			   (sizeof ycsargv / sizeof ycsargv[0]) - 1,
			   ycsargv, h0)))
      return NULL;
  }
  nframes = ((cytype && cytype != 'C')? 2:
	     (h0->fpscode == 3)? 49: 9);
  nfields = ((y4m_si_get_interlace(&h0->si) == Y4M_UNKNOWN)?
	     (MAXHALFHEIGHT < h0->height): y4m_si_get_interlace(&h0->si)) + 1;
  if (deintlval < 0 || nfields == 1 || (cytype && cytype != 'C'))
    deintlval = -1;
  else if (2 * DEINTLRESO < deintlval)
    deintlval = 2 * DEINTLRESO;
  if (dthr16 < 0)
    dthr16 = 0;
  else if (32 < dthr16)
    dthr16 = 32;
  if (noiselevel < 0)
    noiselevel = 0;
  else if (255 < noiselevel)
    noiselevel = 255;
  h = (YfTask_t *)
    YfAllocateTask(&yuvkineco,
		   (sizeof *h +
		    ((((!cytype || cytype == 'C')? (sizeof h->framestat[0]):
		       0) + FRAMEBYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height)) * nframes) +
		    ((0 <= deintlval)? FRAMEBYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height): 0)),
		   (hycs? hycs: h0));
  if (!h)
    return NULL;
  if (output_interlace == 2 &&
      (y4m_si_get_interlace(&h0->si) != Y4M_ILACE_TOP_FIRST ||
       CHDIV(y4m_si_get_chroma(&h0->si)) != 2))
    output_interlace = 3;
  y4m_si_set_interlace(&h->_.si, ((output_interlace == 1)? Y4M_UNKNOWN:
				  (output_interlace == 2)? Y4M_ILACE_TOP_FIRST:
				  Y4M_ILACE_NONE));
  h->_.fpscode = fpscode;
  h->fpscode0  = h0->fpscode;
  h->cytype    = cytype;
  h->nfields   = nfields;
  h->nframes   = nframes;
  h->frame     = (YfFrame_t *)(h->framestat +
			       (nframes * (!cytype || cytype == 'C')));
  h->dthr16    = dthr16;
  h->deintl    = deintlval * DATABYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height) / (2*DEINTLRESO);
  h->nointlmax = -1;
  h->deintlmin = 0x7fffffff;
  h->u.noise.level0 = h->u.noise.level = noiselevel;
  if (0 <= h->deintl)
    nframes++;
  while (0 <= --nframes)
    YfInitFrame((YfFrame_t *)((char *)h->frame +
			      (nframes * FRAMEBYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height))),
		&h->_);
  if (!cytype)
    goto RETURN;
  if ((h->cyfp = fopen(cyname, ((cytype == 'C')? "w": "r"))) == NULL) {
    perror(cyname);
    goto ERROR_RETURN;
  }
  if (cytype == 'C') {
    fprintf(h->cyfp, "# 2-3 pull down cycle list\n"
"# Generated by 'yuvkineco%s -F%d%s%s -n%d -c%d -i%d -C %s'\n"
"#\n"
"# Each character in list mean how each frame processed:\n"
"#   O,o:    output\n"
"#   D,d:    deinterlaced and output\n"
"#   X,x:    bottom field replaced by previous one and output\n"
"#   M,m:    bottom field replaced by previous one and deinterlaced and output\n"
"#   _:      dropped\n"
"# Lower case characters mean output but might drop if framerate was lower.\n"
"#\n"
"# You can edit this list by hand and retry yuvkineco.\n"
"# When edit, you can use characters in addition to above:\n"
"#   E,e:    always deinterlace\n"
"#   Y,y:    always replace bottom field by previous one\n"
"#   N,n:    always replace bottom field by previous one and deinterlace\n"
"#   T,t:    duplicate top field\n"
"#   B,b:    duplicate bottom field\n"
"# Upper case characters mean always output.\n"
"# Lower case characters mean output or drop according to framerate.\n"
"#\n"
"# Use this list with old input to generator yuvkineco,\n"
"# do 'yuvkineco -F fpscode -O %s',\n"
"# with output new,\n"
"# do 'yuvkineco -F fpscode -N %s'.\n"
"#\n"
"# When used as '-O %s':\n"
"#   O,o:        output\n"
"#   D,d,E,e:    deinterlace and output\n"
"#   X,x,Y,y:    replace bottom field by previous one and output\n"
"#   M,m,N,n:    replace bottom field by previous one and deinterlace and output\n"
"#   T,t:        duplicate top field and output\n"
"#   B,b:        duplicate bottom field and output\n"
"#   _:          drop\n"
"#\n"
"# When used as '-N %s':\n"
"#   O,o,X,x,D,d,M,m:    output\n"
"#   E,e:        deinterlace and output\n"
"#   Y,y:        replace bottom field by previous one and output\n"
"#   N,n:        replace bottom field by previous one and deinterlace and output\n"
"#   T,t:        duplicate top field and output\n"
"#   B,b:        duplicate bottom field and output\n"
"#   _:          ignore\n"
"#\n"
"SIZE:%dx%d OLD_FPS:%d NEW_FPS:%d # DON'T CHANGE THIS LINE!!!",
	    ((output_interlace == 1)? " -u": (2 <= output_interlace)? " -s": ""), fpscode,
	    (ycsthres? " -S": ""), (ycsthres? ycsthres: ""),
	    noiselevel, dthr16, deintlval,
	    cyname, cyname, cyname, cyname, cyname,
	    h0->width, h0->height, h0->fpscode, fpscode);
  } else {
    int width, height, oldfps, newfps;
    char buff[128];

    h->u.cy.p = h->u.cy.buff;
    buff[sizeof buff - 1] = buff[sizeof buff - 2] = '\0';
    while (fgets(buff, sizeof buff, h->cyfp)) {
      if (sscanf(buff, "SIZE:%dx%d OLD_FPS:%d NEW_FPS:%d ",
		 &width, &height, &oldfps, &newfps) == 4) {
	if (width != h0->width || height != h0->height ||
	    ((h->cytype == 'O')? oldfps: newfps) != h0->fpscode) {
	  WERROR("input and cycle list not match.");
	  goto ERROR;
	}
	if (h->cytype == 'O' && newfps < fpscode) {
	  WERROR("output fpscode greater than specified at 1st try.");
	  goto ERROR;
	}
	goto RETURN;
      }
    }
    WERROR("broken cycle list.");
  ERROR:
    fclose(h->cyfp);
    goto ERROR_RETURN;
  }
 RETURN:
  return (YfTaskCore_t *)h;
 ERROR_RETURN:
  if (0 <= h->deintl)
    h->nframes++;
  while (0 <= --h->nframes)
    YfFiniFrame((YfFrame_t *)((char *)h->frame +
			      (h->nframes * FRAMEBYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height))));
  YfFreeTask((YfTaskCore_t *)h);
  return NULL;
}

static void
dumpnoise(YfTask_t *h, FILE *fp)
{
  int i;
  char buf[256];
  if (!h->u.noise.total)
    return;
  buf[0] = '\0';
  buf_debug(buf, fp, "#\n");
  buf_debug(buf, fp, "# noise level: %u (# of sample: %lu);  noise distribution:\n",
	    h->u.noise.level, h->u.noise.total);
  buf_debug(buf, fp, "#");
  for (i = 0; i <= NOISEMAX; i++)
    buf_debug(buf, fp, " %lu",
	      (unsigned long)((((uint64_t)h->u.noise.dist[i] * 1000) +
			       h->u.noise.total - 1) / h->u.noise.total));
  buf_debug(buf, fp, "\n");
  if (0 <= h->deintl) {
    int bytes = DATABYTES(y4m_si_get_chroma(&h->_.si), h->_.width, h->_.height);
    buf_debug(buf, fp, "# deinterlaced frames: %u;  pixels/frame distribution:",
	      h->deintlframes);
    for (i = 0; i < sizeof h->deintldist / sizeof h->deintldist[0]; i++) {
      if (!(i % 10)) {
	buf_debug(buf, fp, "\n");
	buf_debug(buf, fp, "# %3d:", i);
      }
      buf_debug(buf, fp, "%7u", h->deintldist[i]);
    }
    buf_debug(buf, fp, "\n");
    buf_debug(buf, fp, "# maximum permil of non-interlaced frames: %3d\n",
	      h->nointlmax * (2 * DEINTLRESO) / bytes);
    buf_debug(buf, fp, "# minimum permil of   deinterlaced frames: %3d\n",
	      ((h->deintlmin == 0x7fffffff)? -1:
	       (h->deintlmin * (2 * DEINTLRESO) / bytes)));
  }
}

static void
do_fini(YfTaskCore_t *handle)
{
  YfTask_t *h = (YfTask_t *)handle;
  while (h->iuse < h->iget)
    do_frame((YfTaskCore_t *)h, NULL, NULL);
  if (1 < verbose)
    dumpnoise(h, NULL);
  if (h->cytype == 'C') {
    putc('\n', h->cyfp);
    dumpnoise(h, h->cyfp);
    fclose(h->cyfp);
  }
  if (0 <= h->deintl)
    h->nframes++;
  while (0 <= --h->nframes)
    YfFiniFrame((YfFrame_t *)((char *)h->frame +
			      (h->nframes * FRAMEBYTES(y4m_si_get_chroma(&h->_.si), h->_.width, h->_.height))));
  YfFreeTask((YfTaskCore_t *)h);
}

static void
putcy(YfTask_t *h, int c)
{
  MPEG_timecode_t tc;
  int f;
  f = mpeg_timecode(&tc, h->iuse, h->fpscode0, 0.);
  if (f <= 0) {
    putc('\n', h->cyfp);
    if (tc.s == 0) {
      if (tc.m % 10 == 0)
	dumpnoise(h, h->cyfp);
      fputs("#\n"
"#    OLD                NEW            0.... 5.... 10....15.... 20....25....\n",
	    h->cyfp);
    }
    fprintf(h->cyfp, "%06d/%02d:%02d:%02d:%02d ", h->iuse, tc.h, tc.m, tc.s, tc.f);
    mpeg_timecode(&tc, h->iput, h->_.fpscode, 0.);
    fprintf(h->cyfp, "%06d/%02d:%02d:%02d:%02d", h->iput, tc.h, tc.m, tc.s, tc.f);
  }
  if (f < 0) {
    int i;
    f = -f;
    for (i = -1; i <= f; i++)
      putc(' ', h->cyfp);
  }
  if (f % 10 == 0)
    putc(' ', h->cyfp);
  if (f %  5 == 0)
    putc(' ', h->cyfp);
  if (c != '_' &&
      h->framestat[h->iuse % h->nframes].eoediff == -9999999)
    c += 'X' - 'O';
  putc(c, h->cyfp);
}

static int
deinterlace(YfTask_t *h, unsigned char *data, int width, int height, int noise)
{
  int pln, i, j, n = 0;
  for (pln = 0; pln < 3; pln++) {
    for (i = 1; i < height - 1; i += 2) {
      for (j = 0; j < width; j++) {
	int ymin = data[((i - 1) * width) + j];
	int ynow = data[((i    ) * width) + j];
	int ymax = data[((i + 1) * width) + j];
	int ynxt = data[((i + 2) * width) + j];
	if (ymax < ymin) {
	  int ytmp = ymax; ymax = ymin; ymin = ytmp;
	}
	ymin -= noise;
	ymax += noise;
	if ((ynow < ymin && ynxt < ymin ) || (ymax < ynow && ymax < ynxt ))
	  n++;
	else if (!(ynow < ymin || ymax < ynow))
	  continue;
#if 0
	if (i < height - 3) {
	  int ynx2 = data[((i + 3) * width) + j];
	  if ((ynx2 < ynxt && ynxt < ymin) || (ymax < ynxt && ynxt < ynx2))
	    continue;
	}
#endif
	ynow = (ymin + ymax) / 2;
	data[((i    ) * width) + j] = ynow;
	if (i < height - 3) {
#if 0
	  int ynx2 = data[((i + 4) * width) + j];
	  if ((ynx2 < ymin && ynxt < ymin) || (ymax < ynx2 && ymax < ynxt))
#endif
	    continue;
	}
	data[((i + 2) * width) + j] = ynow;
      }
    }
    data += width * height;
    if (pln == 0) {
      width  /= CWDIV(y4m_si_get_chroma(&h->_.si));
      height /= CHDIV(y4m_si_get_chroma(&h->_.si));
    }
  }
  return n;
}

static int
putframe(YfTask_t *h, int c, YfFrame_t *frame)
{
  if (c <= 1) {
    if (c)
      deinterlace(h, frame->data, h->_.width, h->_.height, h->u.noise.level0);
  } else {
    if (0 <= h->deintl) {
      int n;
      int bytes = FRAMEBYTES(y4m_si_get_chroma(&h->_.si), h->_.width, h->_.height);
      YfFrame_t *fdst = (YfFrame_t *)((char *)h->frame + (h->nframes * bytes));
      y4m_copy_frame_info(&fdst->fi, &frame->fi);
      memcpy(fdst->data, frame->data, bytes - sizeof frame->fi);
      n = deinterlace(h, fdst->data, h->_.width, h->_.height, h->u.noise.level0);
      if (h->deintl < n) {
	frame = fdst;
	c -= 'O' - 'D';
	h->deintlframes++;
	if (n < h->deintlmin)
	  h->deintlmin = n;
      } else {
	if (h->nointlmax < n)
	  h->nointlmax = n;
      }
      n *= (2 * DEINTLRESO);
      n /= (bytes - sizeof frame->fi);
      if (sizeof h->deintldist / sizeof h->deintldist[0] <= n)
	n = (sizeof h->deintldist / sizeof h->deintldist[0]) - 1;
      h->deintldist[n]++;
    }
    if (h->cytype == 'C')
      putcy(h, c);
  }
  return YfPutFrame(&h->_, frame);
}

static int
do_frame(YfTaskCore_t *handle, const YfTaskCore_t *h0, const YfFrame_t *frame0)
{
  static const unsigned long fp1001s[] = {
    0, 24000, 24024, 25025, 30000, 30030, 50050, 60000, 60060, };
  YfTask_t *h = (YfTask_t *)handle;
  int framebytes = FRAMEBYTES(y4m_si_get_chroma(&h->_.si), h->_.width, h->_.height);
  int iadjust = ((h->_.fpscode == h->fpscode0)? 1:
		 (((int64_t)h->iuse * fp1001s[h->_.fpscode] /
		   fp1001s[h->fpscode0]) - h->iput));
  int wdiv = CWDIV(y4m_si_get_chroma(&h->_.si));
  int hdiv = CHDIV(y4m_si_get_chroma(&h->_.si));

  /* copy frame to buffer */
  if (frame0) {
    int b = h->iget % h->nframes;
    YfFrame_t *fget = (YfFrame_t *)((char *)h->frame + (b * framebytes));
    y4m_copy_frame_info(&fget->fi, &frame0->fi);
    memcpy(fget->data, frame0->data, framebytes - sizeof frame0->fi);
    /* get frame summary */
    if (h->cytype == 'O' || h->cytype =='N') {
      /* do nothing */
    } else if (!h->iget) {
      h->framestat[b].odiff = 0 /* 9999999 */;
      h->framestat[b].ediff = 0 /* 9999999 */;
    } else {
      int i, j, d;
      unsigned int ypre, yget;
      unsigned int noise;
      int ipre = (h->iget - 1) % h->nframes;
      YfFrame_t *fpre = (YfFrame_t *)((char *)h->frame + (ipre * framebytes));
      memset(&h->framestat[b], 0, sizeof h->framestat[b]);
      for (i = h->nfields; i < h->_.height; i += h->nfields) {
	for (j = 0; j < h->_.width; j++) {
	  ypre = fpre->data[(i * h->_.width) + j];
	  yget = fget->data[(i * h->_.width) + j];
	  noise = h->u.noise.level0;
	  if (yget < noise)
	    noise = yget;
	  else if (256 - yget < noise)
	    noise = 256 - yget;
	  d = abs(ypre - yget);
	  h->framestat[b].diffdist[(d < NOISEMAX)? d: NOISEMAX]++;
	  if (noise < d) {
	    d -= noise;
	    h->framestat[b].odiff += (((d * d) + 8) >> 4);
	  }
	  if (1 < h->nfields) {
	    unsigned int ygte;
	    ypre = fpre->data[((i - 1) * h->_.width) + j];
	    ygte = fget->data[((i - 1) * h->_.width) + j];
	    d = abs(yget - ygte);
	    if (noise < d) {
	      d -= noise;
	      h->framestat[b].eoediff -= (((d * d) + 16) >> 5);
	    }
	    d = abs(yget - ypre);
	    if (noise < d) {
	      d -= noise;
	      h->framestat[b].eoediff += (((d * d) + 16) >> 5);
	    }
	    ypre = fpre->data[((i + 1) * h->_.width) + j];
	    ygte = fget->data[((i + 1) * h->_.width) + j];
	    d = abs(yget - ygte);
	    if (noise < d) {
	      d -= noise;
	      h->framestat[b].eoediff -= (((d * d) + 16) >> 5);
	    }
	    d = abs(yget - ypre);
	    if (noise < d) {
	      d -= noise;
	      h->framestat[b].eoediff += (((d * d) + 16) >> 5);
	    }
	    d = abs(ypre - ygte);
	    if (noise < d) {
	      d -= noise;
	      h->framestat[b].ediff += (((d * d) + 8) >> 4);
	    }
	  }
	}
      }
#define PER1024PIXEL(p,h,w) (((p)/=(w)), ((p)<<=10), ((p)/=(h)))
      PER1024PIXEL(h->framestat[b].odiff, ((h->_.height / h->nfields) - 1), h->_.width);
      if (1 < h->nfields) {
	PER1024PIXEL(h->framestat[b].ediff, ((h->_.height / 2) - 1), h->_.width);
	PER1024PIXEL(h->framestat[b].eoediff, ((h->_.height / 2) - 1), h->_.width);
      }
    }
    h->iget++;
  }

  if (!h->cytype || h->cytype =='C') { /* 1st try */
    /* process frames in buffer */
    if (h->iget - h->iuse == h->nframes || !frame0) {
      int b = h->iuse % h->nframes;
      int i, idrp;
      unsigned long dthr = 0;
      char notdrop[48];
      char debugbuf[1024];

      debugbuf[0] = '\0';
      if (1 < verbose) {
	MPEG_timecode_t tc;
	mpeg_timecode(&tc, h->iput, h->_.fpscode, 0.);
	buf_debug(debugbuf, NULL, "%02d:%02d:%02d ", tc.m, tc.s, tc.f);
      }
      {				/* get threshold */
	unsigned long dmin = 0xffffffffUL, dmax = 0;
	int imin = -1, imax = -1;
	for (i = 0; i < h->iget - h->iuse - 1; i++) {
	  unsigned long odiff = h->framestat[(b + i) % h->nframes].odiff;
	  if (1 < verbose)
	    buf_debug(debugbuf, NULL, "%8ld:%-7lu",
		      h->framestat[(b + i) % h->nframes].eoediff, odiff);
	  if (odiff < dmin) {
	    dmin = odiff;
	    imin = i;		/* maybe repeated field */
	  }
	  if (dmax < odiff) {
	    dmax = odiff;
	    imax = i;		/* maybe cut changed */
	  }
	}
	for (i = 0; i < h->iget - h->iuse - 1; i++)
	  if (i != imin && i != imax)
	    dthr += h->framestat[(b + i) % h->nframes].odiff; /* sum */
	if (0 < i - 2) {
	  dthr /= (i - 2);	/* average */
	  dthr -= dmin;
	  dthr *= h->dthr16;
	  dthr += 15;		/* round up */
	  dthr >>= 4;		/* /= 16 */
	  dthr += dmin;
	  dthr += h->u.noise.level0;
	}
      }
      /* search frame to drop */
      memset(notdrop, ((1 < h->nfields)? '2': '0'), sizeof notdrop);
      notdrop[h->iget - h->iuse - 1] = '\0';
      /* check motion top field (or frame of non-interlaced) */
      for (i = 0; i < h->iget - h->iuse - 1; i++)
	if (dthr < h->framestat[(b + i) % h->nframes].odiff)
	  notdrop[i] |= 1;
      if (1 < h->nfields) {	/* check field merged */
	int merged = 0;
#if 0
	/* codes for video editing fade-in/out */
	long dmax = -0x7fffffffL, d2nd = -0x7fffffffL, d3rd = -0x7fffffffL;
	for (i = 1; i < (h->nframes / 2) + 2 && i < h->iget - h->iuse; i++) {
	  long eoediff = h->framestat[(b + i) % h->nframes].eoediff;
	  if (eoediff == -9999999)
	    continue;
	  if        (dmax < eoediff) {
	    d3rd = d2nd; d2nd = dmax; dmax = eoediff;
	  } else if (d2nd < eoediff) {
	    d3rd = d2nd; d2nd = eoediff;
	  } else if (d3rd < eoediff) {
	    d3rd = eoediff;
	  }
	}
	if (d3rd == -0x7fffffffL)
	  d3rd = 0;
	if (1 < verbose)
	  buf_debug(debugbuf, NULL, "%6ld", d3rd);
	for (i = 1; i < (h->nframes / 2) + 2 && i < h->iget - h->iuse; i++)
	  if (h->framestat[(b + i) % h->nframes].eoediff < d3rd - (long)dthr)
#else
	    for (i = 1; i < h->iget - h->iuse - 1; i++)
	      if (h->framestat[(b + i) % h->nframes].eoediff < -(long)dthr)
#endif
		{
		  merged = 1;
		  notdrop[i - 1] &= ~2;
		  notdrop[i    ] &= ~2;
		}
	if (!merged)
	  for (i = 0; i < h->iget - h->iuse - 1; i++)
	    notdrop[i] &= ~2;
      }

      idrp = h->nframes / 2;
      if (h->iget - h->iuse - 1 < idrp)
	idrp = h->iget - h->iuse - 1;
      while (0 <= idrp && notdrop[idrp] != '0')
	idrp--;
      if (idrp < 0) {
	idrp = h->nframes / 2;
	if (h->iget - h->iuse - 1 < idrp)
	  idrp = h->iget - h->iuse - 1;
	while (0 <= idrp && (notdrop[idrp] & 1))
	  idrp--;
      }
      if (idrp < 0 && (h->nframes / 2) + 1 < h->iget - h->iuse - 1) {
	idrp = (h->nframes / 2) + 1;
	while (idrp < h->iget - h->iuse - 1 && (notdrop[idrp] & 1))
	  idrp++;
	if (idrp == h->iget - h->iuse - 1) {
	  idrp = h->nframes / 2;
	  goto DONE;
	}
      }
      if (idrp == h->nframes / 2) {
	unsigned long noisetotal;
	int bdrp;
#if 0
	int diff1024;
#endif
	for (i = 0; i < idrp; i++)
	  if (notdrop[i] == '0')
	    goto DONE;
	/* calculate noise level */
	while ((noisetotal = (h->u.noise.total +
			      (((h->_.height / h->nfields) - 1) * h->_.width))) <
	       h->u.noise.total)	/* overflow */
	  for (h->u.noise.total = 0, i = 0; i <= NOISEMAX; i++)
	    h->u.noise.total += (h->u.noise.dist[i] >>= 1);
	h->u.noise.total = noisetotal;
	bdrp = (b + idrp) % h->nframes;
	for (i = 0; i <= NOISEMAX; i++)
	  h->u.noise.dist[i] += h->framestat[bdrp].diffdist[i];
	noisetotal = NOISERATIO(noisetotal);
	for (i = 0; i <= NOISEMAX; i++) {
	  if (noisetotal < h->u.noise.dist[i])
	    break;
	  noisetotal -= h->u.noise.dist[i];
	}
#if 0
	diff1024 = (i - h->u.noise.level) * 1024;
#endif
	h->u.noise.level = i;
#if 0
	if (diff1024) {
	  for (i = 0; i < h->nframes; i++) {
	    if (diff1024 < -(int)h->framestat[i].odiff)
	      h->framestat[i].odiff = 0;
	    else
	      h->framestat[i].odiff += diff1024;
	  }
	  if (1 < h->nfields)
	    for (i = 0; i < h->nframes; i++) {
	      if (diff1024 < -(int)h->framestat[i].ediff)
		h->framestat[i].ediff = 0;
	      else
		h->framestat[i].ediff += diff1024;
	    }
	}
#endif
      }
    DONE:
      if (1 < verbose)
	buf_debug(debugbuf, NULL, "%3d%7lu %s%3d%3u",
		  iadjust, dthr, notdrop, idrp, h->u.noise.level);

      /* reconstruct frame field merged */
      if (1 < h->nfields) {
	int isrc, isrc0;
	isrc0 = ((0 < iadjust && 0 < idrp)? (idrp - 1): idrp);
	isrc = idrp + (h->nframes / 4) - 1;
	if (h->iget - h->iuse - 2 < isrc)
	  isrc = h->iget - h->iuse - 2;
	for (; isrc0 <= isrc; --isrc) {
	  int bsrc = (b + isrc) % h->nframes;
	  int bdst = (bsrc + 1) % h->nframes;
	  if ((isrc < idrp + (h->nframes / 4) - 1 ||
	       dthr < h->framestat[bdst].ediff) &&
	      h->framestat[bdst].eoediff <= 0 &&
	      h->framestat[bdst].eoediff != -9999999) {
	    YfFrame_t *fsrc = (YfFrame_t *)((char *)h->frame + (bsrc * framebytes));
	    YfFrame_t *fdst = (YfFrame_t *)((char *)h->frame + (bdst * framebytes));
	    for (i = 1; i < h->_.height; i += 2) /* copy bottom field Y */
	      memcpy(&fdst->data[i * h->_.width],
		     &fsrc->data[i * h->_.width], h->_.width);
	    for (i = (h->_.height * wdiv) + 1; i < h->_.height * (wdiv + (2 / hdiv)); i += 2) /* UV */
	      memcpy(&fdst->data[i * h->_.width / wdiv],
		     &fsrc->data[i * h->_.width / wdiv],
		     h->_.width / wdiv);
	    if (1 < verbose)
	      buf_debug(debugbuf, NULL, "%2d", isrc + 1);
	    h->framestat[bdst].eoediff = -9999999;
	  } else {
	    if (1 < verbose)
	      buf_debug(debugbuf, NULL, " -");
	  }
	}
      }
      if (1 < verbose)
	buf_debug(debugbuf, NULL, "\n");

      /* output frames */
      for (i = 0; i < idrp; i++) {
	int ret;
	YfFrame_t *fout = (YfFrame_t *)((char *)h->frame + (b * framebytes));
	if ((ret = putframe(h, 'O', fout)))
	  return ret;
	h->iput++;
	h->iuse++;
	b = h->iuse % h->nframes;
      }
      if (h->iuse < h->iget) {
	YfFrame_t *fout = (YfFrame_t *)((char *)h->frame + (b * framebytes));
	if (iadjust <= 0) {
	  if (h->cytype == 'C')
	    putcy(h, '_');
	} else {
	  int ret;
	  if ((ret = putframe(h, 'o', fout)))
	    return ret;
	  h->iput++;
	}
	h->iuse++;
      }
    }
  } else {			/* retry */
    int i, c;
    YfFrame_t *fsrc, *fdst;
    int deintl = 0;
    while (!*h->u.cy.p) {
      char *s, *d;
      char buff[128];
      d = h->u.cy.p = h->u.cy.buff;
      if (!fgets(buff, sizeof buff, h->cyfp)) {
	perror("cycle list");
	return 1;
      }
      for (s = buff; (c = *s); s++) { /* FIXME: frame# should be checked */
	if (c == '#')
	  break;
	if (isalpha(c) || (c == '_' && h->cytype == 'O')) {
	  if (h->u.cy.buff + 30 <= d) {
	    WWARN("too long line in cycle list.");
	    break;
	  }
	  *d++ = c;
	}
      }
      *d = '\0';
    }
    fsrc = (YfFrame_t *)((char *)h->frame + (((h->iuse - 1) % h->nframes) * framebytes));
    fdst = (YfFrame_t *)((char *)h->frame + (((h->iuse)     % h->nframes) * framebytes));
    h->iuse++;
    c = *h->u.cy.p++;
    if (c == '_')
      return 0;
    if (islower(c)) {
      if (iadjust <= 0)
	return 0;
      c += 'A' - 'a';
    }
    switch (c) {
    case 'D':
    case 'M':
      if (h->cytype == 'N')
	goto DEINTLDECIDED;
      /* else do as E, N */
    case 'E':
    case 'N':
      deintl = 1;
    DEINTLDECIDED:
      c += 'O' - 'D';		/* D->O, M->X, E->P(do as O), N->Y */
      break;
    }
    i = 0;
    switch (c) {
    case 'X':
      if (h->cytype == 'N')
	break;
      /* else do as Y */
    case 'Y':
      for (i = 1; i < h->_.height; i += 2) /* copy bottom field Y */
	memcpy(&fdst->data[i * h->_.width],
	       &fsrc->data[i * h->_.width], h->_.width);
      for (i = (h->_.height * wdiv) + 1; i < h->_.height * (wdiv + (2 / hdiv)); i += 2) /* UV */
	memcpy(&fdst->data[i * h->_.width / wdiv],
	       &fsrc->data[i * h->_.width / wdiv],
	       h->_.width / wdiv);
      break;
    case 'T':
      for (i = 0; i < h->_.height; i += 2) /* duplicate top field Y */
	memcpy(&fdst->data[(i + 1) * h->_.width],
	       &fdst->data[(i)     * h->_.width], h->_.width);
      for (i = (h->_.height * wdiv); i < h->_.height * (wdiv + (2 / hdiv)); i += 2) /* UV */
	memcpy(&fdst->data[(i + 1) * h->_.width / wdiv],
	       &fsrc->data[(i)     * h->_.width / wdiv],
	       h->_.width / wdiv);
      break;
    case 'B':
      for (i = 0; i < h->_.height; i += 2) /* duplicate bottom field Y */
	memcpy(&fdst->data[(i)     * h->_.width],
	       &fdst->data[(i + 1) * h->_.width], h->_.width);
      for (i = (h->_.height * wdiv); i < h->_.height * (wdiv + (2 / hdiv)); i += 2) /* UV */
	memcpy(&fdst->data[(i)     * h->_.width / wdiv],
	       &fsrc->data[(i + 1) * h->_.width / wdiv],
	       h->_.width / wdiv);
      break;
    }
    if ((c = putframe(h, deintl, fdst)))
      return c;
    h->iput++;
    if (i || deintl)
      memcpy(fdst->data, frame0->data, framebytes - sizeof frame0->fi);
  }
  return 0;
}
