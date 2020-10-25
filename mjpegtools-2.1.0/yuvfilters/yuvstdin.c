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
#include <string.h>
#include <unistd.h>
#include <mpegconsts.h>
#include "yuvfilters.h"

DEFINE_STD_YFTASKCLASS(yuvstdin);

static const char *
do_usage(void)
{
  return "";
}

static YfTaskCore_t *
do_init(int argc, char **argv, const YfTaskCore_t *h0)
{
  int framebytes;
  YfTaskCore_t *h;
  y4m_stream_info_t si;

  --argc; ++argv;
  h = NULL;
  y4m_init_stream_info(&si);
  if (y4m_read_stream_header(0, &si) != Y4M_OK)
    goto FINI_SI;
  framebytes = FRAMEBYTES(y4m_si_get_chroma(&si), y4m_si_get_width(&si), y4m_si_get_height(&si));
  h = YfAllocateTask(&yuvstdin, sizeof *h + framebytes, h0);
  if (!h)
    goto FINI_SI;
  y4m_copy_stream_info(&h->si, &si);
  h->width   = y4m_si_get_width(&si);
  h->height  = y4m_si_get_height(&si);
  h->fpscode = mpeg_framerate_code(y4m_si_get_framerate(&si));
 FINI_SI:
  y4m_fini_stream_info(&si);
  return h;
}

static void
do_fini(YfTaskCore_t *handle)
{
  YfFreeTask(handle);
}

static int
do_frame(YfTaskCore_t *handle, const YfTaskCore_t *h0, const YfFrame_t * frame)
{
  YfTaskCore_t *h = handle;
  YfFrame_t *f = (YfFrame_t *)(h + 1);
  int ret;
  unsigned char * yuv[3];

  YfInitFrame(f, h);
  yuv[0] = f->data;
  yuv[1] = yuv[0] + (h->width * h->height);
  yuv[2] = yuv[1] + ((h->width  / CWDIV(y4m_si_get_chroma(&h->si))) *
		     (h->height / CHDIV(y4m_si_get_chroma(&h->si))));
  while ((ret = y4m_read_frame(0, &h->si, &f->fi, yuv)) == Y4M_OK) {
    if ((ret = YfPutFrame(h, f)) != Y4M_OK)
      break;
  }
  YfFiniFrame(f);
  return ret;
}
