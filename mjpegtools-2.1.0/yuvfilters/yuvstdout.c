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

DEFINE_STD_YFTASKCLASS(yuvstdout);

static const char *
do_usage(void)
{
  return "";
}

static YfTaskCore_t *
do_init(int argc, char **argv, const YfTaskCore_t *h0)
{
  YfTaskCore_t *h;

  --argc; ++argv;
  if (!(h = YfAllocateTask(&yuvstdout, sizeof *h, h0)))
    return NULL;
  y4m_si_set_width(&h->si, h0->width);
  y4m_si_set_height(&h->si, h0->height);
  y4m_si_set_framerate(&h->si, mpeg_framerate(h0->fpscode));
  if (y4m_write_stream_header(1, &h->si) != Y4M_OK) {
    YfFreeTask(h);
    h = NULL;
  }
  return h;
}

static void
do_fini(YfTaskCore_t *handle)
{
  YfFreeTask(handle);
}

static int
do_frame(YfTaskCore_t *handle, const YfTaskCore_t *h0, const YfFrame_t * frame0)
{
  uint8_t * yuv[3];
  yuv[0] = (uint8_t*)frame0->data;
  yuv[1] = yuv[0] + (handle->width * handle->height);
  yuv[2] = yuv[1] + ((handle->width  / CWDIV(y4m_si_get_chroma(&handle->si))) *
		     (handle->height / CHDIV(y4m_si_get_chroma(&handle->si))));
  return y4m_write_frame(1, &handle->si, &frame0->fi, yuv);
}
