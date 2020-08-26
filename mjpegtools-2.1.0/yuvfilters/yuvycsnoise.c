/*
 *  Copyright (C) 2001 Kawamata/Hitoshi <hitoshi.kawamata@nifty.ne.jp>
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
#include <unistd.h>
#include "yuvfilters.h"

typedef struct {
  YfTaskCore_t _;
  unsigned int iframe;
  unsigned char flags;
  unsigned char min;
  unsigned char maxt, maxb, maxi, maxc;
  unsigned char errt, errb, erri, errc;
  YfFrame_t frame;
} YfTask_t;

/* bits of flags */
#define TESTNOISE 1
#define  TRIFRAME 2
#define   BIFRAME 4
#define   INFIELD 8
#define    CHROMA 16
#define ALLMETHODS (TRIFRAME|BIFRAME|INFIELD|CHROMA)

DEFINE_STD_YFTASKCLASS(yuvycsnoise);

static const char *
do_usage(void)
{
  return "[-t] [-m {tbic}] [-S min] [-T errt,maxt] [-B errb,maxb] [-I erri,maxi] [-C errc,maxc]";
}

static YfTaskCore_t *
do_init(int argc, char **argv, const YfTaskCore_t *h0)
{
  YfTask_t *h;
  int c;
  unsigned int flags = ALLMETHODS;
  int min = 4;
  int maxt = 255, maxb = 255, maxi = 255, maxc = 255;
  int errt =  32, errb =  32, erri =  16, errc =  12;

  while ((c = getopt(argc, argv, "tm:S:T:B:I:C:")) != -1) {
    switch (c) {
    case 't':
      flags |= TESTNOISE;
      break;
    case 'm':
      flags &= ~ALLMETHODS;
      for (; *optarg; optarg++)
	switch (*optarg) {
	case 't': flags |= TRIFRAME; break;
	case 'b': flags |= BIFRAME;  break;
	case 'i': flags |= INFIELD;  break;
	case 'c': flags |= CHROMA;   break;
	}
      break;
    case 'S':
      sscanf(optarg, "%d", &min);
    case 'T':
      switch (sscanf(optarg, "%d,%d", &errt, &maxt)) {
      case 0:
	sscanf(optarg, ",%d", &maxt);
	break;
      }
      break;
    case 'B':
      switch (sscanf(optarg, "%d,%d", &errb, &maxb)) {
      case 0:
	sscanf(optarg, ",%d", &maxb);
	break;
      }
      break;
    case 'I':
      switch (sscanf(optarg, "%d,%d", &erri, &maxi)) {
      case 0:
	sscanf(optarg, ",%d", &maxi);
	break;
      }
      break;
    case 'C':
      switch (sscanf(optarg, "%d,%d", &errc, &maxc)) {
      case 0:
	sscanf(optarg, ",%d", &maxc);
	break;
      }
      break;
    }
  }
  if (min < 1 ||
      maxt < min || 255 < maxt ||
      maxb < min || 255 < maxb ||
      maxi < min || 255 < maxi ||
      maxc < min || 255 < maxi) {
    WERROR("illeagal threshold");
    return NULL;
  }
  if (errt < 1 || 255 < errt ||
      errb < 1 || 255 < errb ||
      erri < 1 || 255 < erri ||
      errc < 1 || 255 < errc) {
    WERROR("illeagal error");
    return NULL;
  }
  if (y4m_si_get_interlace(&h0->si) == Y4M_ILACE_BOTTOM_FIRST) {
    WERROR("unsupported field order");
    return NULL;
  }
  if (h0->height != 480 || h0->fpscode != 4)
    WWARN("input doesn't seem NTSC full height / full motion video");
  h = (YfTask_t *)
    YfAllocateTask(&yuvycsnoise,
		   (sizeof *h + DATABYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height) + /* frame */
		    (DATABYTES(y4m_si_get_chroma(&h0->si), h0->width, h0->height) * 4) + /* frprv, frnow, dfprv, dfnow */
		    ((h0->width / CWDIV(y4m_si_get_chroma(&h0->si))) * (h0->height / CHDIV(y4m_si_get_chroma(&h0->si))) * 2) + /* dfpr2 */
		    (h0->width * h0->height * 2) + (h0->width * 2)), /* dlprv, dlnow */
		   h0);
  if (!h)
    return NULL;
  h->flags = flags;
  h->min   = min;
  h->maxt  = maxt;
  h->maxb  = maxb;
  h->maxi  = maxi;
  h->maxc  = maxc;
  h->errt  = errt;
  h->errb  = errb;
  h->erri  = erri;
  h->errc  = errc;
  YfInitFrame(&h->frame, &h->_);
  return (YfTaskCore_t *)h;
}

static void
do_fini(YfTaskCore_t *handle)
{
  YfTask_t *h = (YfTask_t *)handle;

  do_frame(handle, NULL, NULL);
  YfFiniFrame(&h->frame);
  YfFreeTask(handle);
}

static void
uvnoise(YfTask_t *h, char *dfpr2_u, char *dfprv, char *dfnow,
	unsigned char *frprv, unsigned char *frnow, const unsigned char *frnxt)
{
  int uw, uh, x, y;
  char *dfpr2_v, *dfprv_u, *dfprv_v, *dfnow_u, *dfnow_v;
  unsigned char *frprv_u, *frprv_v, *frnow_u, *frnow_v;
  const unsigned char *frnxt_u, *frnxt_v;
  unsigned char *frout_u, *frout_v;

  if (!(h->flags & CHROMA))
    return;

  uw = h->_.width  / CWDIV(y4m_si_get_chroma(&h->_.si));
  uh = h->_.height / CHDIV(y4m_si_get_chroma(&h->_.si));
  dfpr2_v = dfpr2_u + (uw * uh);
  dfprv_u = dfprv + (h->_.width * h->_.height); dfprv_v = dfprv_u + (uw * uh);
  dfnow_u = dfnow + (h->_.width * h->_.height); dfnow_v = dfnow_u + (uw * uh);
  frprv_u = frprv + (h->_.width * h->_.height); frprv_v = frprv_u + (uw * uh);
  frnow_u = frnow + (h->_.width * h->_.height); frnow_v = frnow_u + (uw * uh);
  frnxt_u = frnxt + (h->_.width * h->_.height); frnxt_v = frnxt_u + (uw * uh);
  frout_u = h->frame.data + (h->_.width * h->_.height);
  frout_v = frout_u + (uw * uh);

  for (y = 0; y < uh; y++) {
    int i, i0, i1;
    i = (uw * y);
    if (y & 1) {
      i0 = h->_.width * ((y * 2) - 1);
      i1 = h->_.width * ((y * 2) + 1);
    } else {
      i0 = h->_.width * ((y * 2));
      i1 = h->_.width * ((y * 2) + 2);
    }
    for (x = 0; x < uw; x++, i++, i0 +=2, i1 += 2) {
      int d0;
      if ((((d0 = dfnow_u[i]) &&
	    dfprv_u[i] && dfprv_u[i] != d0 &&
	    dfpr2_u[i] && dfpr2_u[i] == d0) ||
	   ((d0 = dfnow_v[i]) &&
	    dfprv_v[i] && dfprv_v[i] != d0 &&
	    dfpr2_v[i] && dfpr2_v[i] == d0)) &&
	  frprv_u[i] - h->errc < frnxt_u[i] && frnxt_u[i] < frprv_u[i] + h->errc &&
	  frprv_v[i] - h->errc < frnxt_v[i] && frnxt_v[i] < frprv_v[i] + h->errc &&
	  frnow[i0]   - (h->errc + h->min) < frprv[i0]   && frprv[i0]   < frnow[i0]   + (h->errc + h->min) &&
	  frnxt[i0]   - (h->errc)          < frprv[i0]   && frprv[i0]   < frnxt[i0]   + (h->errc)          &&
	  frnow[i1]   - (h->errc + h->min) < frprv[i1]   && frprv[i1]   < frnow[i1]   + (h->errc + h->min) &&
	  frnxt[i1]   - (h->errc)          < frprv[i1]   && frprv[i1]   < frnxt[i1]   + (h->errc)          &&
	  frnow[i0+1] - (h->errc + h->min) < frprv[i0+1] && frprv[i0+1] < frnow[i0+1] + (h->errc + h->min) &&
	  frnxt[i0+1] - (h->errc)          < frprv[i0+1] && frprv[i0+1] < frnxt[i0+1] + (h->errc)          &&
	  frnow[i1+1] - (h->errc + h->min) < frprv[i1+1] && frprv[i1+1] < frnow[i1+1] + (h->errc + h->min) &&
	  frnxt[i1+1] - (h->errc)          < frprv[i1+1] && frprv[i1+1] < frnxt[i1+1] + (h->errc)) {
	d0 = (frprv_u[i] + frnxt_u[i]) / 2;
	d0 -= frnow_u[i];
	d0 /= 2;
	if (-h->maxc <= d0 && d0 <= h->maxc)
	  frout_u[i] = ((h->flags & TESTNOISE)? 0: (frnow_u[i] + d0));
	d0 = (frprv_v[i] + frnxt_v[i]) / 2;
	d0 -= frnow_v[i];
	d0 /= 2;
	if (-h->maxc <= d0 && d0 <= h->maxc)
	  frout_v[i] = ((h->flags & TESTNOISE)? 0: (frnow_v[i] + d0));
      }
    }
  }
}

static void
ynoise(YfTask_t *h, int btmfld,
       char *dnow, char *daux, char *dfprv, char *dfnow,
       unsigned char *frprv, unsigned char *frnow, const unsigned char *frnxt)
{
  char *dprv, *dnxt, *dffld, *dfaux;
  const unsigned char *fraux;
  int x, y;

  if (!btmfld) {		/* top field */
    dprv = daux; dnxt = dnow; dffld = dfprv; dfaux = dfnow;
    fraux = frprv;
  } else {			/* bottom field */
    dprv = dnow; dnxt = daux; dffld = dfnow; dfaux = dfprv;
    fraux = frnxt;
  }
  for (y = btmfld; y < h->_.height; y += 2) {
    for (x = 0; x < h->_.width; x++) {
      int max;
      int i0 = (h->_.width * (y))     + x;
      int i1 = (h->_.width * (y + 1)) + x;
      int j1 = (h->_.width * (y - 1)) + x;
      int i2 = (h->_.width * (y + 2)) + x;
      int d0 = dfprv[i0];
      if ((h->flags & TRIFRAME) &&
	  d0 &&
	  dfnow[i0] && dfnow[i0] != d0 &&
	  (!dffld[j1] || dffld[j1] == d0) &&
	  (!dffld[i1] || dffld[i1] != d0) &&
	  (!dfaux[j1] || dfaux[j1] != d0) &&
	  (!dfaux[i1] || dfaux[i1] == d0) &&
	  frprv[i0] - h->errt < frnxt[i0] && frnxt[i0] < frprv[i0] + h->errt &&
	  frprv[j1] - h->errt < frnxt[j1] && frnxt[j1] < frprv[j1] + h->errt &&
	  frprv[i1] - h->errt < frnxt[i1] && frnxt[i1] < frprv[i1] + h->errt) {
	max = h->maxt;
	d0 = (frprv[i0] + frnxt[i0]) / 2;
	goto YNOISE;
      }
      d0 = dnow[i0];
      if (d0 && dnow[i2] && dnow[i2] != d0) {
	int j2 = (h->_.width * (y - 2)) + x;
	if ((h->flags & BIFRAME) &&
	    dprv[i1] && dprv[i1] == d0 &&
	    dnxt[i1] && dnxt[i1] != d0 &&
	    daux[i0] && daux[i0] != d0 &&
	    daux[i2] && daux[i2] == d0 &&
	    frnow[i0] - h->errb < fraux[j2] && fraux[j2] < frnow[i0] + h->errb &&
	    frnow[i0] - h->errb < fraux[i2] && fraux[i2] < frnow[i0] + h->errb &&
	    frnow[i1] - h->errb < fraux[j1] && fraux[j1] < frnow[i1] + h->errb &&
	    fraux[i1] - h->errb < frnow[j1] && frnow[j1] < fraux[i1] + h->errb &&
	    fraux[i0] - h->errb < frnow[j2] && frnow[j2] < fraux[i0] + h->errb &&
	    fraux[i0] - h->errb < frnow[i2] && frnow[i2] < fraux[i0] + h->errb) {
	  max = h->maxb;
	  goto YNOISE_XI;
	} else {
	  int i4 = (h->_.width * (y + 4)) + x;
	  int j4 = (h->_.width * (y - 4)) + x;
	  int i6 = (h->_.width * (y + 6)) + x;
	  int j6 = (h->_.width * (y - 6)) + x;
	  if ((h->flags & INFIELD) &&
	      dnow[j2] && dnow[j2] != d0 &&
	      dnow[i4] && dnow[i4] == d0 &&
	      dnow[j4] && dnow[j4] == d0 &&
	      dnow[i6] && dnow[i6] != d0 &&
	      !(daux[i0] && daux[i0] == d0 &&
		daux[i2] && daux[i2] != d0 &&
		daux[j2] && daux[j2] != d0 &&
		daux[i4] && daux[i4] == d0 &&
		daux[j4] && daux[j4] == d0 &&
		daux[i6] && daux[i6] != d0) &&
	      frnow[i0] - h->erri < frnow[j4] && frnow[j4] < frnow[i0] + h->erri &&
	      frnow[i0] - h->erri < frnow[i4] && frnow[i4] < frnow[i0] + h->erri &&
	      frnow[j2] - h->erri < frnow[j6] && frnow[j6] < frnow[j2] + h->erri &&
	      frnow[j2] - h->erri < frnow[i2] && frnow[i2] < frnow[j2] + h->erri &&
	      frnow[i2] - h->erri < frnow[i6] && frnow[i6] < frnow[i2] + h->erri) {
	    max = h->maxi;
	    goto YNOISE_XI;
	  }
	}
	continue;
      YNOISE_XI:
	d0 = (frnow[j2] + frnow[i2]) / 2;
	goto YNOISE;
      }
      continue;
    YNOISE:
      d0 -= frnow[i0];
      d0 /= 2;
      if (-max <= d0 && d0 <= max)
	h->frame.data[i0] = ((h->flags & TESTNOISE)? 0: (frnow[i0] + d0));
    }
  }
}

/*
 *              triframe           biframe          infield
 *           prv   now   nxt   prv   now   nxt         now 
 *     T  B  T  B  T  B  T  B  T  B  T  B  T  B  T  B  T  B
 * -4  -     +     -     +     -     +     -     +     t   
 * -3     +     -     +     -     +     -     +     -  |  b
 * -2  +     -     +     -     t     t     +     -     t  |
 * -1     -     t-----t-----t  |  t  |  t     -     +  |  b
 *  0  -     t-----T-----t     t  |  T  |  -     +     t  |
 *  1     +     t-----t-----t  |  t  |  t     +     -  |  b
 *  2  +     -     +     -     t     t     +     -     T  |
 * -2     -     +     -     +     -     b     b     +  |  B
 * -1  -     b-----b-----b     -     b  |  b  |  +     t  |
 *  0     +     b-----B-----b     +  |  B  |  b     -  |  b
 *  1  +     b-----b-----b     +     b  |  b  |  -     t  |
 *  2     -     +     -     +     -     b     b     +  |  b
 *  3  -     +     -     +     -     +     -     +     t  |
 *  4     +     -     +     -     +     -     +     -     b
 */

static int
do_frame(YfTaskCore_t *handle, const YfTaskCore_t *h0, const YfFrame_t *frame0)
{
  YfTask_t *h = (YfTask_t *)handle;
  int databytes = DATABYTES(y4m_si_get_chroma(&h->_.si), h->_.width, h->_.height);
  const unsigned char *frnxt = frame0->data;
  unsigned char *frprv = h->frame.data + databytes;
  unsigned char *frnow =         frprv + databytes;
  char          *dfprv = (char *)frnow + databytes;
  char          *dfnow =         dfprv + databytes;
  char          *dfpr2 =         dfnow + databytes;
  char          *dlprv =         dfpr2 + ((h->_.width  / CWDIV(y4m_si_get_chroma(&h->_.si))) *
					  (h->_.height / CHDIV(y4m_si_get_chroma(&h->_.si))) * 2);
  char          *dlnow =         dlprv + (h->_.width * h->_.height);
#define dlnxt dlprv

  if (h->iframe & 1) {
    char *tmp;
    tmp = (char *)frprv; frprv = frnow; frnow = (unsigned char *)tmp;
    tmp =         dfprv; dfprv = dfnow; dfnow = tmp;
    tmp =         dlprv; dlprv = dlnow; dlnow = tmp;
  }
  if (h->flags & CHROMA)
    memcpy(dfpr2, dfnow + (h->_.width * h->_.height),
	   (h->_.width  / CWDIV(y4m_si_get_chroma(&h->_.si))) *
	   (h->_.height / CHDIV(y4m_si_get_chroma(&h->_.si))) * 2); /* dfnow: 2 frames ago */
  if (!frame0)
    frnxt = frnow;
  if (h->flags & (CHROMA|TRIFRAME)) {
    if (!frame0) {
      memset(dfnow, 0, databytes);
    } else {
      int i;
      for (i = 0; i < databytes; i++) {
	int d = frnow[i] - frnxt[i];
	dfnow[i] = ((d <= -h->min)? -1:
		    (h->min  <= d)?  1: 0);
      }
    }
  }
  if (h->iframe) {
    memcpy(h->frame.data, frnow, databytes);
    uvnoise(h, dfpr2, dfprv, dfnow, frprv, frnow, frnxt);
    ynoise(h, 0, dlnow, dlprv, dfprv, dfnow, frprv, frnow, frnxt); /* top field */
  }
  if (h->flags & (BIFRAME|INFIELD)) {
    if (!frame0) {
      memset(dlnxt, 0, h->_.width * h->_.height);
    } else {
      int x, y;
      for (y = 2; y < h->_.height; y++) {
	for (x = 0; x < h->_.width; x++) {
	  int j2 = (h->_.width * (y - 2)) + x;
	  int i0 = (h->_.width * (y))     + x;
	  int d = frnxt[j2] - frnxt[i0];
	  dlnxt[i0] = ((d <= -h->min)? -1:
		       (h->min  <= d)?  1: 0);
	}
      }
    }
  }
  if (h->iframe) {
    int ret;
    ynoise(h, 1, dlnow, dlnxt, dfprv, dfnow, frprv, frnow, frnxt); /* bottom field */
    if ((ret = YfPutFrame(&h->_, &h->frame)))
      return ret;
  }
  if (!frame0)
    return 0;
  memcpy(frprv, frnxt, databytes); /* frprv: frnow at next */
  h->iframe++;
  return 0;
}
