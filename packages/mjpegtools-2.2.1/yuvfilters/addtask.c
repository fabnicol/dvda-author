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
#include "yuvfilters.h"

YfTaskCore_t *
YfAddNewTask(const YfTaskClass_t *filter,
	  int argc, char **argv, const YfTaskCore_t *h0)
{
  YfTaskCore_t *h;
  if (h0) {
    while (h0->handle_outgoing)
      h0 = h0->handle_outgoing;
    h = (*filter->init)(argc, argv, h0);
    if (!h)
      return NULL;
    while (h0->handle_outgoing)
      h0 = h0->handle_outgoing;
    *(YfTaskCore_t **)&h0->handle_outgoing = h;
  } else
    h = (*filter->init)(argc, argv, h0);
  return h;
}
