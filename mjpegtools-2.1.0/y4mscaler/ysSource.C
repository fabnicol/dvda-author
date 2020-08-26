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


#include "ysSource.H"
#include <string.h>


void ysSource::describe_keywords(FILE *fp, const char *prefix) const 
{
  fprintf(fp, "%sactive=WxH+X+Y\n", prefix);
  fprintf(fp, "%smatte=WxH+X+Y\n", prefix);
  {
    ysYCbCr defbg(default_bgcolor());
    fprintf(fp, "%sbg=[ RGB:r,g,b | YCBCR:y,cb,cr |\n", prefix);
    fprintf(fp, "%s     RGBA:r,g,b,a | YCBCR:y,cb,cr,a ] (YCBCRA:%d,%d,%d,%d)\n",
	    prefix, defbg(0), defbg(1), defbg(2), defbg(3));
  }
  fprintf(fp, "%snorm=[ NTSC | PAL | SECAM ]\n", prefix);
  fprintf(fp, "%silace=[ NONE | TOP_FIRST | BOTTOM_FIRST | TOP_ONLY | BOTTOM_ONLY ]\n", prefix);
  {
    fprintf(fp, "%schromass=[ ", prefix);
    int i = 0;
    const char *keyword;
    while ((keyword = y4m_chroma_keyword(i++)) != NULL)
      fprintf(fp, "%s%s ", (i < 2) ? "" : "| ", keyword);
    fprintf(fp, "]\n");
  }
  fprintf(fp, "%ssar=[ N:D | NTSC | PAL | NTSC_WIDE | PAL_WIDE ]\n", prefix);
}


void ysSource::fake_progressive(FakeProg_t f)
{
  if (f) {
    if ( (_stream.interlace() == Y4M_UNKNOWN) ||
	 (_stream.interlace() == Y4M_ILACE_NONE) ) {
      mjpeg_error_exit1("Must be interlaced to fake non-interlaced!");
    }
    if (_stream.interlace() == Y4M_ILACE_MIXED) 
      mjpeg_error_exit1("Cannot fake non-interlaced from mixed-mode source!");
    if (f == FAKE_TOP_ONLY)
      mjpeg_info("Faking non-interlaced source; using top field only.");
    else
      mjpeg_info("Faking non-interlaced source; using bottom field only.");
    _fake_progressive = f;
    _real_stream = _stream;
    _stream.interlace(Y4M_ILACE_NONE);
    _stream.y_size(_stream.y_size() / 2);
    _stream.sar(_stream.sar() / 2);  // ????????
    if (_fake_field[0] != NULL) delete[] _fake_field[0];
    _fake_field[0] = new uint8_t[_real_stream.fielddim().area()];
    _fake_field[1] = _fake_field[2] = _fake_field[0];
  }
}
    


void ysSource::parse_keyword(char *optarg)
{
  
  if (!strncasecmp(optarg, "ACTIVE=", 7)) {
    if (_active_region.parse_geometry(optarg+7)) {
      mjpeg_error_exit1("Bad ACTIVE keyword: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "MATTE=", 6)) {
    if (_matte_region.parse_geometry(optarg+6)) {
      mjpeg_error_exit1("Bad MATTE keyword: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "BG=", 3)) {
    bgcolor(ysYCbCr::parse_string(optarg+3));

  } else if (!strcasecmp(optarg, "NORM=NTSC")) {
    norm(NORM_NTSC);

  } else if (!strcasecmp(optarg, "NORM=PAL")) {
    norm(NORM_PAL);

  } else if (!strcasecmp(optarg, "NORM=SECAM")) {
    norm(NORM_PAL);

  } else if (!strncasecmp(optarg, "CHROMASS=", 9)) {
    //    if (_stream.subsampling().parse_mode(optarg+9)) {
    if (_stream.subsampling().is_known()) {
      mjpeg_warn("Overriding source's chroma subsampling mode!");
      //  Was %s",
      //                 _stream.subsampling().mode_to_string());
    }
    if (_stream.parse_subsampling(optarg+9)) {
      mjpeg_error_exit1("Bad chroma subsampling spec: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "ILACE=", 6)) {
    if (!strcasecmp(optarg+6, "TOP_FIRST")) {
      interlace(Y4M_ILACE_TOP_FIRST);
    } else if (!strcasecmp(optarg+6, "BOTTOM_FIRST")) {
      interlace(Y4M_ILACE_BOTTOM_FIRST);
    } else if (!strcasecmp(optarg+6, "NONE")) {
      interlace(Y4M_ILACE_NONE);
    } else if (!strcasecmp(optarg+6, "TOP_ONLY")) {
      fake_progressive(FAKE_TOP_ONLY);
    } else if (!strcasecmp(optarg+6, "BOTTOM_ONLY")) {
      fake_progressive(FAKE_BOT_ONLY);
    } else {
      mjpeg_error_exit1("Bad interlace spec: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "SAR=", 4)) {
    ysRatio sar;
    if (!strcasecmp(optarg+4, "NTSC")) {
      sar = y4m_sar_NTSC_CCIR601;
    } else if (!strcasecmp(optarg+4, "PAL")) {
      sar = y4m_sar_PAL_CCIR601;
    } else if (!strcasecmp(optarg+4, "NTSC_WIDE")) {
      sar = y4m_sar_NTSC_16_9;
    } else if (!strcasecmp(optarg+4, "PAL_WIDE")) {
      sar = y4m_sar_PAL_16_9;
    } else if (sar.parse_ratio(optarg+4)) {
      mjpeg_error_exit1("Bad ratio spec: '%s'", optarg);
    }
    _stream.sar(sar);

  } else
    mjpeg_error_exit1 ("Unrecognized input parameter:  '%s'", optarg);
}



void ysSource::check_parameters()
{
  int cause_to_exit = 0;

  /* init interlacing */
  if (_stream.interlace() == Y4M_UNKNOWN) {
    mjpeg_error("Source interlacing is unknown!");
    cause_to_exit = 1;
  }
  /* init/constrain SAR */
  if (!_stream.sar().is_known()) {
    mjpeg_error("Source sample aspect ratio unknown!");
    cause_to_exit = 1;
  }

  /* init/constrain chroma subsampling */
  if (!_stream.subsampling().is_known()) {
    mjpeg_error("Source chroma subsampling is unknown!");
    cause_to_exit = 1;
  }

  /* init/clip matte region */
  /* default is entire source frame --- so convolution can extend beyond
     the active region */
  if (!_matte_region.is_known()) {
    if (_matte_region.offset().is_known()) {
      mjpeg_info("Source matte region defaulting to source frame size.");
      _matte_region.dim(_stream.dim());
    } else {
      mjpeg_info("Source matte region defaulting to full source frame.");
      _matte_region = ysRegion(_stream.dim());
      _matte_region.origin_mode(ANC_TL);
    }
  }
  _matte_region.fixate(_stream.dim());

  /* check alignment */
  /* frame size and matte region must conform to alignment */
  {
    int xal = _stream.x_alignment();
    int yal = _stream.y_alignment();

    if (_stream.x_size() % xal) {
      mjpeg_error("Source x size (%d) is not multiple of %d!",
		  _stream.x_size(), xal);
      cause_to_exit = 1;
    }
    if (_stream.y_size() % yal) {
      mjpeg_error("Source y size (%d) is not multiple of %d!",
		  _stream.y_size(), yal);
      cause_to_exit = 1;
    }

    if (_matte_region.dim().x() % xal) {
      mjpeg_error("Source matte region x size (%d) is not multiple of %d!",
		  _matte_region.dim().x(), xal);
      cause_to_exit = 1;
    }
    if (_matte_region.offset().x() % xal) {
      mjpeg_error("Source matte region x offset (%d) is not multiple of %d!",
		  _matte_region.offset().x(), xal);
      cause_to_exit = 1;
    }
    if (_matte_region.dim().y() % yal) {
      mjpeg_error("Source matte region y size (%d) is not multiple of %d!",
		  _matte_region.dim().y(), yal);
      cause_to_exit = 1;
    }
    if (_matte_region.offset().y() % yal) {
      mjpeg_error("Source matte region y offset (%d) is not multiple of %d!",
		  _matte_region.offset().y(), yal);
      cause_to_exit = 1;
    }
  }

  if (cause_to_exit) 
    exit(1);

  /* init/clip active region */
  if (!_active_region.is_known()) {
    if (_active_region.offset().is_known()) {
      mjpeg_info("Source active region defaulting to source frame size.");
      _active_region.dim(_stream.dim());
    } else {
      mjpeg_info("Source active region defaulting to full source frame.");
      _active_region = ysRegion(_stream.dim());
      _active_region.origin_mode(ANC_TL);
    }
  }
  _active_region.fixate(_stream.dim());


#if 0  /* do clipping later, after ratios are established */
  if (_active_region.clip(ysRatioPoint(_stream.dim()))) {
    mjpeg_warn("Source active region clipped by frame size.");
  }
#endif

}




void ysSource::describe_parameters() const
{
  mjpeg_info("=== SOURCE parameters: =================");
  if (_fake_progressive) {
    mjpeg_info("< effective stream:");
  } else {
    mjpeg_info("< stream:");
  }
  mjpeg_info("<   %dx%d, SAR %d:%d, %s",
	     _stream.x_size(), 
	     _stream.y_size(), 
	     _stream.sar().ratio().n,
	     _stream.sar().ratio().d,
	     ilace_to_string(_stream.interlace()));
  mjpeg_info("<   chroma subsampling:  %s", 
	     _stream.subsampling().mode_to_string());
  mjpeg_info("<   chroma ss ratios:  x %d:%d  y %d:%d",
	     _stream.subsampling().ratio().x().numerator(),
	     _stream.subsampling().ratio().x().denominator(),
	     _stream.subsampling().ratio().y().numerator(),
	     _stream.subsampling().ratio().y().denominator());
  mjpeg_info("< active region:");
  mjpeg_info("<   %fx%f at %f,%f",
	     _active_region.dim().x().to_double(),
	     _active_region.dim().y().to_double(),
	     _active_region.offset().x().to_double(),
	     _active_region.offset().y().to_double());
  mjpeg_info("< matte region:");
  mjpeg_info("<   %dx%d at %d,%d  (bg Y'CbCr: %d,%d,%d)",
	     _matte_region.dim().x(),
	     _matte_region.dim().y(),
	     _matte_region.offset().x(),
	     _matte_region.offset().y(),
	     _bgcolor(0), _bgcolor(1), _bgcolor(2));
}




int ysSource::read_stream_header(int fdin)
{
  return _stream.read_stream_header(fdin);
}


#if 0
static int internal_read_444_frame(int fd, 
				   const y4m_stream_info_t *si, 
				   y4m_frame_info_t *fi,
				   uint8_t * const yuv[3])
{
  int err;
  int w = y4m_si_get_width(si);
  int h = y4m_si_get_height(si);
  
  /* Read frame header */
  if ((err = y4m_read_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Read luminance scanlines */
  if (y4m_read(fd, yuv[0], w*h)) return Y4M_ERR_SYSTEM;
  /* Read chrominance scanlines */
  if (y4m_read(fd, yuv[1], w*h)) return Y4M_ERR_SYSTEM;
  if (y4m_read(fd, yuv[2], w*h)) return Y4M_ERR_SYSTEM;

  return Y4M_OK;
}


static int internal_read_422_frame(int fd, 
				   const y4m_stream_info_t *si, 
				   y4m_frame_info_t *fi,
				   uint8_t * const yuv[3])
{
  int err;
  int w = y4m_si_get_width(si);
  int h = y4m_si_get_height(si);
  
  /* Read frame header */
  if ((err = y4m_read_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Read luminance scanlines */
  if (y4m_read(fd, yuv[0], w*h)) return Y4M_ERR_SYSTEM;
  /* Read chrominance scanlines */
  if (y4m_read(fd, yuv[1], w*h/2)) return Y4M_ERR_SYSTEM;
  if (y4m_read(fd, yuv[2], w*h/2)) return Y4M_ERR_SYSTEM;

  return Y4M_OK;
}


static int internal_read_411_fields(int fd,
				    const y4m_stream_info_t *si,
				    y4m_frame_info_t *fi,
				    uint8_t * const upper_field[3], 
				    uint8_t * const lower_field[3])
{
  int i, y, err;
  int width = y4m_si_get_width(si);
  int height = y4m_si_get_height(si);
  
  /* Read frame header */
  if ((err = y4m_read_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Read Y', Cb, and Cr planes */
  for (i = 0; i < 3; i++) {
    uint8_t *srctop = upper_field[i];
    uint8_t *srcbot = lower_field[i];
    /* alternately write one line from each */
    for (y = 0; y < height; y += 2) {
      if (y4m_read(fd, srctop, width)) return Y4M_ERR_SYSTEM;
      srctop += width;
      if (y4m_read(fd, srcbot, width)) return Y4M_ERR_SYSTEM;
      srcbot += width;
    }
    /* for chroma, width/height are half as big */
    if (i == 0) {
      width /= 4;
      //      height /= 2;
    }
  }
  return Y4M_OK;
}


static int internal_read_422_fields(int fd,
				    const y4m_stream_info_t *si,
				    y4m_frame_info_t *fi,
				    uint8_t * const upper_field[3], 
				    uint8_t * const lower_field[3])
{
  int i, y, err;
  int width = y4m_si_get_width(si);
  int height = y4m_si_get_height(si);
  
  /* Read frame header */
  if ((err = y4m_read_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Read Y', Cb, and Cr planes */
  for (i = 0; i < 3; i++) {
    uint8_t *srctop = upper_field[i];
    uint8_t *srcbot = lower_field[i];
    /* alternately write one line from each */
    for (y = 0; y < height; y += 2) {
      if (y4m_read(fd, srctop, width)) return Y4M_ERR_SYSTEM;
      srctop += width;
      if (y4m_read(fd, srcbot, width)) return Y4M_ERR_SYSTEM;
      srcbot += width;
    }
    /* for chroma, width/height are half as big */
    if (i == 0) {
      width /= 2;
      //      height /= 2;
    }
  }
  return Y4M_OK;
}


static int internal_read_444_fields(int fd,
				    const y4m_stream_info_t *si,
				    y4m_frame_info_t *fi,
				    uint8_t * const upper_field[3], 
				    uint8_t * const lower_field[3])
{
  int i, y, err;
  int width = y4m_si_get_width(si);
  int height = y4m_si_get_height(si);
  
  /* Read frame header */
  if ((err = y4m_read_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Read Y', Cb, and Cr planes */
  for (i = 0; i < 3; i++) {
    uint8_t *srctop = upper_field[i];
    uint8_t *srcbot = lower_field[i];
    /* alternately write one line from each */
    for (y = 0; y < height; y += 2) {
      if (y4m_read(fd, srctop, width)) return Y4M_ERR_SYSTEM;
      srctop += width;
      if (y4m_read(fd, srcbot, width)) return Y4M_ERR_SYSTEM;
      srcbot += width;
    }
  }
  return Y4M_OK;
}
#endif


#if 0
int ysSource::read_frame(int fdin, y4m_frame_info_t *frameinfo,
			 uint8_t **frame)
{
#if 1
  switch (_fake_progressive) {
  case FAKE_NO_FAKE:
    switch (_stream.subsampling().mode()) {
    case ysSubsampling::SS_444:
      return internal_read_444_frame(fdin, _stream.streaminfo(), frameinfo,
				     frame);
    case ysSubsampling::SS_422:
      return internal_read_422_frame(fdin, _stream.streaminfo(), frameinfo,
				     frame);
    case ysSubsampling::SS_411:
    case ysSubsampling::SS_420_JPEG:
    case ysSubsampling::SS_420_MPEG2:
    case ysSubsampling::SS_420_PALDV:
    default:
      return y4m_read_frame(fdin, _stream.streaminfo(), frameinfo, frame);
    }
  case FAKE_TOP_ONLY:
    switch (_stream.subsampling().mode()) {
    case ysSubsampling::SS_411:
      return internal_read_411_fields(fdin, 
				      _real_stream.streaminfo(), frameinfo,
				      frame, _fake_field);
    case ysSubsampling::SS_444:
      return internal_read_444_fields(fdin,
				      _real_stream.streaminfo(), frameinfo,
				      frame, _fake_field);
    case ysSubsampling::SS_422:
      return internal_read_422_fields(fdin,
				      _real_stream.streaminfo(), frameinfo,
				      frame, _fake_field);
    case ysSubsampling::SS_420_JPEG:
    case ysSubsampling::SS_420_MPEG2:
    case ysSubsampling::SS_420_PALDV:
    default:
      return y4m_read_fields(fdin,
			     _real_stream.streaminfo(), frameinfo,
			     frame, _fake_field);
    }
  case FAKE_BOT_ONLY:
    switch (_stream.subsampling().mode()) {
    case ysSubsampling::SS_411:
      return internal_read_411_fields(fdin, 
				      _real_stream.streaminfo(), frameinfo,
				      _fake_field, frame);
    case ysSubsampling::SS_444:
      return internal_read_444_fields(fdin,
				      _real_stream.streaminfo(), frameinfo,
				      _fake_field, frame);
    case ysSubsampling::SS_422:
      return internal_read_422_fields(fdin,
				      _real_stream.streaminfo(), frameinfo,
				      _fake_field, frame);
    case ysSubsampling::SS_420_JPEG:
    case ysSubsampling::SS_420_MPEG2:
    case ysSubsampling::SS_420_PALDV:
    default:
      return y4m_read_fields(fdin,
			     _real_stream.streaminfo(), frameinfo,
			     _fake_field, frame);
    }
  }
  return -1;  /*????? */
#else
  switch (_fake_progressive) {
  case FAKE_NO_FAKE:
    return y4m_read_frame(fdin, _stream.streaminfo(), frameinfo, frame);
  case FAKE_TOP_ONLY:
    return y4m_read_fields(fdin, _real_stream.streaminfo(), frameinfo,
			   frame, _fake_field);
  case FAKE_BOT_ONLY:
    return y4m_read_fields(fdin, _real_stream.streaminfo(), frameinfo,
			   _fake_field, frame);
  }
  return -1;  /*????? */
#endif
}
#endif

#if 0
int ysSource::read_fields(int fdin, y4m_frame_info_t *frameinfo,
			  uint8_t **topfield, uint8_t **bottomfield)
{
  switch (_stream.subsampling().mode()) {
  case ysSubsampling::SS_411:
    return internal_read_411_fields(fdin, _stream.streaminfo(), frameinfo,
				    topfield, bottomfield);
  case ysSubsampling::SS_444:
    return internal_read_444_fields(fdin, _stream.streaminfo(), frameinfo,
				    topfield, bottomfield);
  case ysSubsampling::SS_422:
    return internal_read_422_fields(fdin, _stream.streaminfo(), frameinfo,
				    topfield, bottomfield);
  case ysSubsampling::SS_420_JPEG:
  case ysSubsampling::SS_420_MPEG2:
  case ysSubsampling::SS_420_PALDV:
  default:
    return y4m_read_fields(fdin, _stream.streaminfo(), frameinfo,
			   topfield, bottomfield);
  }
}
#endif




int ysSource::read_frame(int fdin, y4m_frame_info_t *frameinfo,
			 uint8_t **frame)
{
  switch (_fake_progressive) {
  case FAKE_NO_FAKE:
    return y4m_read_frame(fdin, _stream.streaminfo(), frameinfo, frame);
  case FAKE_TOP_ONLY:
    return y4m_read_fields(fdin, _real_stream.streaminfo(), frameinfo,
                           frame, _fake_field);
  case FAKE_BOT_ONLY:
    return y4m_read_fields(fdin, _real_stream.streaminfo(), frameinfo,
                           _fake_field, frame);
  }
  return -1;  /*????? */
}


int ysSource::read_frame_data(int fdin, y4m_frame_info_t *frameinfo,
                              uint8_t **frame)
{
  switch (_fake_progressive) {
  case FAKE_NO_FAKE:
    return y4m_read_frame_data(fdin, _stream.streaminfo(), frameinfo, frame);
  case FAKE_TOP_ONLY:
    return y4m_read_fields_data(fdin, _real_stream.streaminfo(), frameinfo,
                                frame, _fake_field);
  case FAKE_BOT_ONLY:
    return y4m_read_fields_data(fdin, _real_stream.streaminfo(), frameinfo,
                                _fake_field, frame);
  }
  return -1;  /*????? */
}


int ysSource::read_fields(int fdin, y4m_frame_info_t *frameinfo,
			  uint8_t **topfield, uint8_t **bottomfield)
{
  return y4m_read_fields(fdin, _stream.streaminfo(), frameinfo,
                         topfield, bottomfield);
}

int ysSource::read_fields_data(int fdin, y4m_frame_info_t *frameinfo,
                               uint8_t **topfield, uint8_t **bottomfield)
{
  return y4m_read_fields_data(fdin, _stream.streaminfo(), frameinfo,
                              topfield, bottomfield);
}


#if 0
// Should probably just remove this...
int ysSource::read_frame_or_fields(int fdin, y4m_frame_info_t *frameinfo,
                                   uint8_t **frame,
                                   uint8_t **upper, uint8_t **lower)
{
  int err;
  err = y4m_read_frame_header(fdin, _stream.streaminfo(), frameinfo);
  if (err != Y4M_OK) return err;

  mjpeg_info("F-or-F:  %d  %d",
             y4m_fi_get_temporal(frameinfo),
             y4m_fi_get_spatial(frameinfo));

  /* XXXXXX what about ip/pi/420 issues? XXXXXXX */
  int sampling = y4m_fi_get_temporal(frameinfo);
  if (sampling == Y4M_SAMPLING_PROGRESSIVE) 
    return y4m_read_frame_data(fdin, _stream.streaminfo(), frameinfo, frame);
  else /* == Y4M_SAMPLING_INTERLACED */
    return y4m_read_fields_data(fdin, _stream.streaminfo(), frameinfo,
                                upper, lower);
}
#endif

int ysSource::read_frame_header(int fdin, y4m_frame_info_t *frameinfo)
{
  return y4m_read_frame_header(fdin, _stream.streaminfo(), frameinfo);
}
