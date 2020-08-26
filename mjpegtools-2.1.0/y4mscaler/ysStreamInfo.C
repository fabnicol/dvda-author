/*
    Copyright 2003 Matthew Marjanovic <maddog@mir.com>

    This file is part of y4mscaler.

    y4mscaler is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    y4mscaler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with y4mscaler; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "y4m-config.h"
#include "debug.h"

#include <stdio.h>

#include "ysStreamInfo.H"




int ysStreamInfo::parse_dimensions(const char *s)
{
  int sx, sy;
  if (sscanf(s, "%ux%u", &sx, &sy) == 2) {
    x_size(sx);
    y_size(sy);
    return 0;
  }
  return 1;
}


int ysStreamInfo::parse_subsampling(const char *s)
{
  if (_subsampling.parse_mode(s)) return 1;
  y4m_si_set_chroma(&_streaminfo, _subsampling.mode());
  return 0;
}


int ysStreamInfo::x_alignment() const
{
  return _subsampling.ratio().x().denominator();
}


int ysStreamInfo::y_alignment() const
{
  if (interlace() != Y4M_ILACE_NONE)
    /* worst-case:  some kind of interlacing may be involved. */
    return _subsampling.ratio().y().denominator() * 2;
  else
    return _subsampling.ratio().y().denominator();
}



const char *ilace_to_string(int i)
{
  switch (i) {
  case Y4M_UNKNOWN:             return "unknown";            break;
  case Y4M_ILACE_NONE:          return "progressive";        break;
  case Y4M_ILACE_TOP_FIRST:     return "top-field-first";    break;
  case Y4M_ILACE_BOTTOM_FIRST:  return "bottom-field-first"; break;
  case Y4M_ILACE_MIXED:         return "mixed-interlacing";  break;
  default:
    return "?????";
    break;
  }
}



int ysStreamInfo::read_stream_header(int fdin)
{
  int retval = y4m_read_stream_header(fdin, &_streaminfo);

  if (retval == Y4M_OK) {
    int chroma = y4m_si_get_chroma(&_streaminfo);
    switch (chroma) {
    case Y4M_CHROMA_420JPEG:
      _subsampling.mode(ysSubsampling::SS_420_JPEG); break;
    case Y4M_CHROMA_420MPEG2:
      _subsampling.mode(ysSubsampling::SS_420_MPEG2); break;
    case Y4M_CHROMA_420PALDV:
      _subsampling.mode(ysSubsampling::SS_420_PALDV); break;
    case Y4M_CHROMA_411: 
      _subsampling.mode(ysSubsampling::SS_411); break;
    case Y4M_CHROMA_422:  
      _subsampling.mode(ysSubsampling::SS_422); break;
    case Y4M_CHROMA_444:
      _subsampling.mode(ysSubsampling::SS_444); break;
    case Y4M_CHROMA_444ALPHA: 
      _subsampling.mode(ysSubsampling::SS_444ALPHA); break;
      //      mjpeg_error("4:4:4/alpha streams not yet supported.");
      //      retval = Y4M_ERR_FEATURE;
      //      break;
    case Y4M_CHROMA_MONO:
      _subsampling.mode(ysSubsampling::SS_MONO); break;
      //      mjpeg_error("Mono/Luma-only streams not yet supported.");
      //      retval = Y4M_ERR_FEATURE;
      //      break;
    default:
      mjpeg_error("Unknown (and unsupported) chroma format.");
      retval = Y4M_ERR_HEADER;
      break;
    }
  }
  return retval;
}


int ysStreamInfo::write_stream_header(int fdout)
{
#if 0
  /* NB:  this is now taken care of by yuv4mpeg lib. */
  y4m_xtag_list_t *xtags = y4m_si_xtags(&_streaminfo);
  /* remove any old chromass tags */
  for (int n = 0; n < y4m_xtag_count(xtags); n++) {
    const char *tag = y4m_xtag_get(xtags, n);
    if (!strncmp("XYSCSS=", tag, 7)) 
      y4m_xtag_remove(xtags, n);
  }
#endif
#if 0
  /* set correct chroma mode --- this should be done by a subsampling
     setter method everytime subsampling is modified... */
  switch (_subsampling.mode()) {
  case ysSubsampling::SS_444:
    y4m_si_set_chroma(&_streaminfo, Y4M_CHROMA_444);  break;
  case ysSubsampling::SS_422:
    y4m_si_set_chroma(&_streaminfo, Y4M_CHROMA_422);  break;
  case ysSubsampling::SS_420_MPEG2: 
    y4m_si_set_chroma(&_streaminfo, Y4M_CHROMA_420MPEG2);  break;
  case ysSubsampling::SS_420_PALDV:
    y4m_si_set_chroma(&_streaminfo, Y4M_CHROMA_420PALDV);  break;
  case ysSubsampling::SS_411:
    y4m_si_set_chroma(&_streaminfo, Y4M_CHROMA_411);  break;
  case ysSubsampling::SS_420_JPEG:
    y4m_si_set_chroma(&_streaminfo, Y4M_CHROMA_420JPEG);  break;
  default:
    mjpeg_error("OOOPS.  Encountered unknown chroma ss mode.  %d",
		_subsampling.mode());
    exit(1);
  }
#endif
  /* write that header! */
  return y4m_write_stream_header(fdout, &_streaminfo);
}

