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

#ifndef _YS_STREAM_H_
#define _YS_STREAM_H_


#include <string.h>

extern "C" {
#include <yuv4mpeg.h>
}
#include "graphics.H"


class ysStreamInfo {
private:
  y4m_stream_info_t _streaminfo;
  ysSubsampling _subsampling;

public:
  ysStreamInfo() { 
    y4m_init_stream_info(&_streaminfo);
  }
  ~ysStreamInfo() {
    y4m_fini_stream_info(&_streaminfo);
  }
  ysStreamInfo(const ysStreamInfo &si);
  ysStreamInfo &operator=(const ysStreamInfo &si) {
    if (this != &si) {
      y4m_copy_stream_info(&_streaminfo, &si._streaminfo);
      _subsampling = si._subsampling;
    }
    return *this;
  }

  /* basic setters */
  void x_size(int a) { y4m_si_set_width(&_streaminfo, a); }
  void y_size(int a)  { y4m_si_set_height(&_streaminfo, a); }
  void interlace(int a) { y4m_si_set_interlace(&_streaminfo, a); }
  void framerate(const ysRatio &a) {
    y4m_si_set_framerate(&_streaminfo, a.ratio()); }
  void sar(const ysRatio &a) { 
    y4m_si_set_sampleaspect(&_streaminfo, a.ratio()); }

  /* basic accessors */
  int x_size() const { return y4m_si_get_width(&_streaminfo); }
  int y_size() const { return y4m_si_get_height(&_streaminfo); }
  int interlace() const { return y4m_si_get_interlace(&_streaminfo); }
  ysRatio framerate() const { return y4m_si_get_framerate(&_streaminfo); }
  ysRatio sar() const { return y4m_si_get_sampleaspect(&_streaminfo); }
  int planes() const { return y4m_si_get_plane_count(&_streaminfo); }


  const y4m_stream_info_t *streaminfo() const { return &_streaminfo; }
  const ysSubsampling &subsampling() const { return _subsampling; }

  /* chroma mode must be kept in sync... */
  void subsampling(ysSubsampling::Mode amode) {
    y4m_si_set_chroma(&_streaminfo, amode);
    _subsampling.mode(amode);
  }


  /* derivative accessors */
  int x_alignment() const;
  int y_alignment() const;
  //  int planes() const { return 3; }
  ysPoint dim() const;
  ysPoint framedim(int plane = 0) const;
  ysPoint fielddim(int plane = 0) const;

  /* derivative setters */
  void dim(const ysPoint &p);


  /* basic methods */
  int read_stream_header(int fdin);
  int write_stream_header(int fdout);
  void log_info(log_level_t level, const char *prefix) const;

  /* derivative methods */
  int parse_dimensions(const char *s);
  int parse_subsampling(const char *s);
};


const char *ilace_to_string(int i);


inline void ysStreamInfo::log_info(log_level_t level,
				   const char *prefix) const
{
  y4m_log_stream_info(level, prefix, &_streaminfo);
}


inline ysPoint ysStreamInfo::dim() const
{
  int x = x_size();
  int y = y_size();
  if ((x == Y4M_UNKNOWN) || (y == Y4M_UNKNOWN))
    return ysPoint();
  else
    return ysPoint(x,y);
}

inline void ysStreamInfo::dim(const ysPoint &p)
{
  if (p.is_known()) {
    x_size(p.x());
    y_size(p.y());
  } else {
    x_size(Y4M_UNKNOWN);
    y_size(Y4M_UNKNOWN);
  }
}

#if 0
inline ysPoint ysStreamInfo::framedim(int plane) const
{
  switch (plane) {
  case 0:
    return dim();
    break;
  case 1:
  case 2:
    {
#if 0
      int x = x_size();
      int y = y_size();
      x = (_subsampling.ratio().x() * x).to_int();
      y = (_subsampling.ratio().y() * y).to_int();
      return ysPoint(x,y);
#endif
      return ysPoint((_subsampling.ratio().x() * x_size()).to_int(),
		     (_subsampling.ratio().y() * y_size()).to_int());
    }
    break;
  default:
    assert((plane >=0) && (plane <= 2));
    break;
  }
  return ysPoint();
}

inline ysPoint ysStreamInfo::fielddim(int plane) const 
{
  switch (plane) {
  case 0:
    return ysPoint(x_size(), y_size() / 2);
    break;
  case 1:
  case 2:
    {
#if 0
      int x = x_size();
      int y = y_size();
      x = (_subsampling.ratio().x() * x).to_int();
      y = (_subsampling.ratio().y() * (y / 2)).to_int();
      return ysPoint(x,y);
#endif
      return ysPoint((_subsampling.ratio().x() * x_size()).to_int(),
		     (_subsampling.ratio().y() * (y_size() / 2)).to_int());
    }
    break;
  default:
    assert((plane >=0) && (plane <= 2));
    break;
  }
  return ysPoint();
}
#endif


inline ysPoint ysStreamInfo::framedim(int plane) const
{
  int x = y4m_si_get_plane_width(&_streaminfo, plane);
  int y = y4m_si_get_plane_height(&_streaminfo, plane);
  if ((x == Y4M_UNKNOWN) || (y == Y4M_UNKNOWN))
    return ysPoint();
  else
    return ysPoint(x, y);
}


inline ysPoint ysStreamInfo::fielddim(int plane) const
{
  int x = y4m_si_get_plane_width(&_streaminfo, plane);
  int y = y4m_si_get_plane_height(&_streaminfo, plane);
  if ((x == Y4M_UNKNOWN) || (y == Y4M_UNKNOWN))
    return ysPoint();
  else
    return ysPoint(x, y/2);
}


#endif /* _YS_STREAM_H_ */
