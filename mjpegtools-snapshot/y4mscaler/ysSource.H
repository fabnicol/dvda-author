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

#ifndef _YS_SOURCE_H_
#define _YS_SOURCE_H_

#include <stdio.h>
#include "graphics.H"
#include "ysStreamInfo.H"


class ysSource {
public:
  enum Norm_t {
    NORM_UNKNOWN = -1,
    NORM_NTSC,
    NORM_PAL
  };
  enum FakeProg_t {
    FAKE_NO_FAKE = 0,
    FAKE_TOP_ONLY,
    FAKE_BOT_ONLY
  };
private:
  ysStreamInfo _stream;
  ysStreamInfo _real_stream;
  ysRatioRegion _active_region;
  ysRegion _matte_region;
  ysYCbCr _bgcolor;
  Norm_t _norm;
  FakeProg_t _fake_progressive;
  uint8_t *_fake_field[3];

  void norm(Norm_t n) { _norm = n; }
  void interlace(int ilace) {
    if ((_stream.interlace() != Y4M_UNKNOWN) &&
	(_stream.interlace() != ilace)) {
#if 0
      mjpeg_warn("Casting source interlace from %s to %s.",
		 ilace_to_string(_stream.interlace()),
		 ilace_to_string(ilace));
#else
      mjpeg_warn("Casting source interlacing to %s.", ilace_to_string(ilace));
#endif
      if (((_stream.interlace() == Y4M_ILACE_NONE) ||
	   (ilace == Y4M_ILACE_NONE)) &&
	  (_stream.subsampling().ratio().y() != ysRatio(1,1))) {
	mjpeg_warn("...chroma planes will get commangled!");
      }
    }
    _stream.interlace(ilace);
  }
  void sar(const ysRatio &r) {
    if (_stream.sar().is_known())
      mjpeg_warn("Casting source SAR from %d:%d to %d:%d",
		 _stream.sar().ratio().n, _stream.sar().ratio().d,
		 r.ratio().n, r.ratio().d);
    _stream.sar(r);
  }
  void fake_progressive(FakeProg_t f);

  static ysYCbCr default_bgcolor(void) { return ysYCbCr(16,128,128); }

  ysSource &operator=(const ysSource &other);
  ysSource(const ysSource &other);
public:
  ysSource() :
    _bgcolor(default_bgcolor()),
    _norm(NORM_UNKNOWN),
    _fake_progressive(FAKE_NO_FAKE)
  {
  _fake_field[0] = NULL;
  }
  ~ysSource() {
    if (_fake_field[0] != NULL) delete[] _fake_field[0];
  }

  /* const accessors */
  ysYCbCr bgcolor() const { return _bgcolor; }
  const ysRatioRegion &active_region() const { return _active_region; }
  const ysRegion &matte_region() const { return _matte_region; }
  const ysStreamInfo &stream() const { return _stream; }

  /* non-const accessors */
  ysRatioRegion &active_region() { return _active_region; }
  ysRegion &matte_region() { return _matte_region; }

  /* setters */
  void bgcolor(const ysYCbCr &x) { _bgcolor = x; }

  Norm_t norm() const {
    if (_norm == NORM_UNKNOWN) {
      if ((_stream.framerate() == y4m_fps_NTSC) ||
	  (_stream.framerate() == y4m_fps_NTSC_FIELD) ||
	  (_stream.framerate() == y4m_fps_NTSC_FILM))
        return NORM_NTSC;
      else if ((_stream.framerate() == y4m_fps_PAL) ||
	       (_stream.framerate() == y4m_fps_PAL_FIELD))
        return NORM_PAL;
      else
        return NORM_UNKNOWN;
    } else 
      return _norm;
  }

  int read_stream_header(int fdin);
  int read_frame_header(int fdin, y4m_frame_info_t *frameinfo);

  int read_frame(int fdin, y4m_frame_info_t *frameinfo, uint8_t **frame);
  int read_fields(int fdin, y4m_frame_info_t *frameinfo,
		  uint8_t **topfield, uint8_t **bottomfield);

  //int read_frame_or_fields(int fdin, y4m_frame_info_t *frameinfo,
  //                        uint8_t **frame, uint8_t **upper, uint8_t **lower);

  int read_frame_data(int fdin, y4m_frame_info_t *frameinfo, uint8_t **frame);
  int read_fields_data(int fdin, y4m_frame_info_t *frameinfo,
                       uint8_t **upper, uint8_t **lower);

  void describe_keywords(FILE *fp, const char *prefix) const;
  void parse_keyword(char *optarg);

  void check_parameters();
  void describe_parameters() const;
};




#endif /* _YS_SOURCE_H_ */
