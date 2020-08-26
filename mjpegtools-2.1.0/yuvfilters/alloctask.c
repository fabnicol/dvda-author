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
#include "yuvfilters.h"

YfTaskCore_t *
YfAllocateTask(const YfTaskClass_t *filter, size_t size, const YfTaskCore_t *h0)
{
  YfTaskCore_t *h = malloc(size);
  if (!h) {
    perror("malloc");
    return NULL;
  }
  memset(h, 0, size);
  h->method = filter;
  h->handle_outgoing = NULL;
  y4m_init_stream_info(&h->si);
  if (h0) {
    y4m_copy_stream_info(&h->si, &h0->si);
    h->width   = h0->width;
    h->height  = h0->height;
    h->fpscode = h0->fpscode;
  }
  return h;
}

void
YfFreeTask(YfTaskCore_t *handle)
{
  y4m_fini_stream_info(&handle->si);
  free(handle);
}
