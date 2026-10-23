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


#include "ysTarget.H"


#include <string.h>


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

void ysTarget::set_prefab_target(PrefabType_t target, const ysSource &source)
{
  switch (target) {
  case PREFAB_VCD:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("VCD output format requested in NTSC norm.");
	  _stream.x_size(352);
	  _stream.y_size(240);
	  _stream.sar(y4m_sar_NTSC_CCIR601);
	  _stream.interlace(Y4M_ILACE_NONE);
	  //_stream.subsampling().mode(ysSubsampling::SS_420_JPEG);
          _stream.subsampling(ysSubsampling::SS_420_JPEG);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("VCD output format requested in PAL/SECAM norm.");
	  _stream.x_size(352);
	  _stream.y_size(288);
	  _stream.sar(y4m_sar_PAL_CCIR601);
	  _stream.interlace(Y4M_ILACE_NONE);
	  //_stream.subsampling().mode(ysSubsampling::SS_420_JPEG);
          _stream.subsampling(ysSubsampling::SS_420_JPEG);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine VCD format.");
	break;
      }
    }
    break;

  case PREFAB_CVD:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("CVD output format requested in NTSC norm.");
	  _stream.x_size(352);
	  _stream.y_size(480);
	  _stream.sar(ysRatio(y4m_sar_NTSC_CCIR601) * 2);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
	  //_stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
          _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("CVD output format requested in PAL/SECAM norm.");
	  _stream.x_size(352);
	  _stream.y_size(576);
	  _stream.sar(ysRatio(y4m_sar_PAL_CCIR601) * 2);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
	  //_stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
          _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine CVD format.");
	break;
      }
    }
    break;

  case PREFAB_SVCD:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("SVCD output format requested in NTSC norm.");
	  _stream.x_size(480);
	  _stream.y_size(480);
	  _stream.sar(y4m_sar_NTSC_SVCD_4_3);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
	  //_stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
          _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("SVCD output format requested in PAL/SECAM norm.");
	  _stream.x_size(480);
	  _stream.y_size(576);
	  _stream.sar(y4m_sar_PAL_SVCD_4_3);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
          //	  _stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
	  _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine SVCD format.");
	break;
      }
    }
    break;

  case PREFAB_DVD:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("DVD output format requested in NTSC norm.");
	  _stream.x_size(720);
	  _stream.y_size(480);
	  _stream.sar(y4m_sar_NTSC_CCIR601);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
	  //_stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
          _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("DVD output format requested in PAL/SECAM norm.");
	  _stream.x_size(720);
	  _stream.y_size(576);
	  _stream.sar(y4m_sar_PAL_CCIR601);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
          //	  _stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
	  _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine DVD format.");
	break;
      }
    }
    break;

  case PREFAB_DVD_WIDE:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("DVD(widescreen) output format requested in NTSC norm.");
	  _stream.x_size(720);
	  _stream.y_size(480);
	  _stream.sar(y4m_sar_NTSC_16_9);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
	  //_stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
          _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("DVD(widescreen) output format requested in PAL/SECAM norm.");
	  _stream.x_size(720);
	  _stream.y_size(576);
	  _stream.sar(y4m_sar_PAL_16_9);
	  //_stream.interlace(Y4M_ILACE_NONE);
	  // interlace ok --- but is it required????
          //	  _stream.subsampling().mode(ysSubsampling::SS_420_MPEG2);
	  _stream.subsampling(ysSubsampling::SS_420_MPEG2);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine DVD(widescreen) format.");
	break;
      }
    }
    break;

  case PREFAB_DV:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("DV output format requested in NTSC norm.");
	  _stream.x_size(720);
	  _stream.y_size(480);
	  _stream.sar(y4m_sar_NTSC_CCIR601);
	  _stream.interlace(Y4M_ILACE_BOTTOM_FIRST);
	  //_stream.subsampling().mode(ysSubsampling::SS_411);
          _stream.subsampling(ysSubsampling::SS_411);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("DV output format requested in PAL/SECAM norm.");
	  _stream.x_size(720);
	  _stream.y_size(576);
	  _stream.sar(y4m_sar_PAL_CCIR601);
	  _stream.interlace(Y4M_ILACE_BOTTOM_FIRST);
          //	  _stream.subsampling().mode(ysSubsampling::SS_420_PALDV);
	  _stream.subsampling(ysSubsampling::SS_420_PALDV);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine DV format.");
	break;
      }
    }
    break;

  case PREFAB_DV_WIDE:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	{
	  mjpeg_info ("DV(widescreen) output format requested in NTSC norm.");
	  _stream.x_size(720);
	  _stream.y_size(480);
	  _stream.sar(y4m_sar_NTSC_16_9);
	  _stream.interlace(Y4M_ILACE_BOTTOM_FIRST);
	  _stream.subsampling(ysSubsampling::SS_411);
	}
	break;
      case ysSource::NORM_PAL:
	{
	  mjpeg_info("DV(widescreen) output format requested in PAL/SECAM norm.");
	  _stream.x_size(720);
	  _stream.y_size(576);
	  _stream.sar(y4m_sar_PAL_16_9);
	  _stream.interlace(Y4M_ILACE_BOTTOM_FIRST);
	  _stream.subsampling(ysSubsampling::SS_420_PALDV);
	}
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine DV(widescreen) format.");
	break;
      }
    }
    break;

  case PREFAB_SVCD_STILL_HI:
  case PREFAB_VCD_STILL_HI:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	mjpeg_info ("SVCD/VCD hi-res still image format requested, NTSC.");
	_stream.y_size(480);
	_stream.sar(y4m_sar_NTSC_CCIR601);
	break;
      case ysSource::NORM_PAL:
	mjpeg_info ("SVCD/VCD hi-res still image format requested, PAL.");
	_stream.y_size(576);
	_stream.sar(y4m_sar_PAL_CCIR601);
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine image format.");
	break;
      }
      _stream.x_size(704);
      _stream.interlace(Y4M_ILACE_NONE);
      if (target == PREFAB_SVCD_STILL_HI) 
	_stream.subsampling(ysSubsampling::SS_420_MPEG2);
      else
	_stream.subsampling(ysSubsampling::SS_420_JPEG);
    }
    break;

  case PREFAB_SVCD_STILL_LO:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	mjpeg_info ("SVCD lo-res still image format requested, NTSC.");
	_stream.y_size(480);
	_stream.sar(y4m_sar_NTSC_SVCD_4_3);
	break;
      case ysSource::NORM_PAL:
	mjpeg_info ("SVCD lo-res still image format requested, PAL.");
	_stream.y_size(576);
	_stream.sar(y4m_sar_PAL_SVCD_4_3);
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine image format.");
	break;
      }
      _stream.x_size(480);
      _stream.interlace(Y4M_ILACE_NONE);
      _stream.subsampling(ysSubsampling::SS_420_MPEG2);
    }
    break;
    
  case PREFAB_VCD_STILL_LO:
    {
      switch (source.norm()) {
      case ysSource::NORM_NTSC:
	mjpeg_info ("VCD lo-res still image format requested, NTSC.");
	_stream.y_size(240);
	_stream.sar(y4m_sar_NTSC_CCIR601);
	break;
      case ysSource::NORM_PAL:
	mjpeg_info ("VCD lo-res still image format requested, PAL.");
	_stream.y_size(288);
	_stream.sar(y4m_sar_PAL_CCIR601);
	break;
      default:
	mjpeg_error_exit1("Unknown norm; cannot determine image format.");
	break;
      }
      _stream.x_size(352);
      _stream.interlace(Y4M_ILACE_NONE);
      _stream.subsampling(ysSubsampling::SS_420_JPEG);
    }
    break;

  case PREFAB_ATSC_720P:
    {
      if (source.norm() == ysSource::NORM_PAL) 
        mjpeg_warn("ATSC preset requested for PAL norm --- ignoring norm.");
      mjpeg_info ("ATSC 720p output format requested.");
      _stream.x_size(1280);
      _stream.y_size(720);
      _stream.sar(y4m_sar_SQUARE);
      _stream.interlace(Y4M_ILACE_NONE);
      _stream.subsampling(ysSubsampling::SS_420_MPEG2);
    }
    break;

  case PREFAB_ATSC_1080I:
    {
      if (source.norm() == ysSource::NORM_PAL) 
        mjpeg_warn("ATSC preset requested for PAL norm --- ignoring norm.");
      mjpeg_info ("ATSC 1080i output format requested.");
      _stream.x_size(1920);
      _stream.y_size(1080);
      _stream.sar(y4m_sar_SQUARE);
      _stream.subsampling(ysSubsampling::SS_420_MPEG2);
      { 
        int ilace = source.stream().interlace();
        if (ilace == Y4M_ILACE_NONE) {
          mjpeg_warn("1080i requires interlacing; defaulting to bottom-first.");
          ilace = Y4M_ILACE_BOTTOM_FIRST;
        }
        _stream.interlace(ilace);
      }
    }
    break;

  case PREFAB_ATSC_1080P:
    {
      if (source.norm() == ysSource::NORM_PAL) 
        mjpeg_warn("ATSC preset requested for PAL norm --- ignoring norm.");
      mjpeg_info ("ATSC 1080p output format requested.");
      _stream.x_size(1920);
      _stream.y_size(1080);
      _stream.sar(y4m_sar_SQUARE);
      _stream.interlace(Y4M_ILACE_NONE);
      _stream.subsampling(ysSubsampling::SS_420_MPEG2);
    }
    break;

  default:
    mjpeg_error_exit1("Spurious prefab format requested!");
    break;
  }
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

void ysTarget::init_stream(const ysSource &source)
{
  _stream = source.stream();
  _stream.x_size(Y4M_UNKNOWN);
  _stream.y_size(Y4M_UNKNOWN);
  _stream.interlace(Y4M_UNKNOWN);
  _stream.sar(ysRatio());
  _stream.subsampling(ysSubsampling::SS_UNKNOWN);
}  


void ysTarget::describe_keywords(FILE *fp, const char *prefix) const 
{
  fprintf(fp, "%ssize=[ WxH | src ]\n", prefix);
  fprintf(fp, "%sactive=WxH+X+Y\n", prefix);
  {
    ysYCbCr defbg(default_bgcolor());
    fprintf(fp, "%sbg=[ RGB:r,g,b | YCBCR:y,cb,cr |\n", prefix);
    fprintf(fp, "%s     RGBA:r,g,b,a | YCBCR:y,cb,cr,a ] (YCBCRA:%d,%d,%d,%d)\n",
	    prefix, defbg(0), defbg(1), defbg(2), defbg(3));
  }
  fprintf(fp, "%silace=[ NONE | TOP_FIRST | BOTTOM_FIRST ]\n", prefix);
  //fprintf(fp, "%schromass=[ 420_JPEG | 420_MPEG2 | 420_PALDV ]\n", prefix);
  {
    fprintf(fp, "%schromass=[ ", prefix);
    int i = 0;
    const char *keyword;
    while ((keyword = y4m_chroma_keyword(i++)) != NULL)
      fprintf(fp, "%s%s ", (i < 2) ? "" : "| ", keyword);
    fprintf(fp, "]\n");
  }
  fprintf(fp, "%ssar=[ N:D | src | NTSC | PAL | NTSC_WIDE | PAL_WIDE ]\n", prefix);
  fprintf(fp, "%sscale=N/D\n", prefix);
  fprintf(fp, "%sXscale=N/D\n", prefix);
  fprintf(fp, "%sYscale=N/D\n", prefix);
  fprintf(fp, "%sinfer=[ PAD | CLIP | PRESERVE_X | PRESERVE_Y ] (CLIP)\n",
	  prefix);
  fprintf(fp, "%sinfer=[ SIMPLIFY | EXACT ] (SIMPLIFY)\n", prefix);
  fprintf(fp, "%salign=[ TL | TC | TR | CL | CC | CR | BL | BC | BR ] (CC)\n", prefix);
  fprintf(fp, "%spreset=[ VCD | CVD | SVCD | DVD | DVD_WIDE | DV | DV_WIDE |\n", prefix);
  fprintf(fp, "%s         VCD_STILL_LO | VCD_STILL_HI | SVCD_STILL_LO | SVCD_STILL_HI |\n", prefix);
  fprintf(fp, "%s         ATSC_720P | ATSC_1080I | ATSC_1080P ]\n", prefix);
}
  


void ysTarget::parse_keyword(ysSource &source, char *optarg)
{
  if (!(strncasecmp(optarg, "SIZE=", 5))) {
    if (!strcasecmp(optarg+5, "src")) {
      _stream.dim(source.stream().dim());
    } else if (_stream.parse_dimensions(optarg+5)) {
      mjpeg_error_exit1("Bad dimensions spec: '%s'", optarg+5);
    }
    
  } else if (!strncasecmp(optarg, "ACTIVE=", 7)) {
    if (_active_region.parse_geometry(optarg+7)) {
      mjpeg_error_exit1("Bad geometery spec: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "BG=", 3)) {
    bgcolor(ysYCbCr::parse_string(optarg+3));

  } else if (!strncasecmp(optarg, "PRESET=", 7)) {
    if (!(strcasecmp(optarg+7, "VCD"))) {
      set_prefab_target(PREFAB_VCD, source);
    } else if (!(strcasecmp(optarg+7, "CVD"))) {
      set_prefab_target(PREFAB_CVD, source);
    } else if (!(strcasecmp(optarg+7, "SVCD"))) {
      set_prefab_target(PREFAB_SVCD, source);
    } else if (!(strcasecmp(optarg+7, "DVD"))) {
      set_prefab_target(PREFAB_DVD, source);
    } else if (!(strcasecmp(optarg+7, "DVD_WIDE"))) {
      set_prefab_target(PREFAB_DVD_WIDE, source);
    } else if (!(strcasecmp(optarg+7, "DV"))) {
      set_prefab_target(PREFAB_DV, source);
    } else if (!(strcasecmp(optarg+7, "DV_WIDE"))) {
      set_prefab_target(PREFAB_DV_WIDE, source);
    } else if (!(strcasecmp(optarg+7, "VCD_STILL_LO"))) {
      set_prefab_target(PREFAB_VCD_STILL_LO, source);
    } else if (!(strcasecmp(optarg+7, "VCD_STILL_HI"))) {
      set_prefab_target(PREFAB_VCD_STILL_HI, source);
    } else if (!(strcasecmp(optarg+7, "SVCD_STILL_LO"))) {
      set_prefab_target(PREFAB_SVCD_STILL_LO, source);
    } else if (!(strcasecmp(optarg+7, "SVCD_STILL_HI"))) {
      set_prefab_target(PREFAB_SVCD_STILL_HI, source);
    } else if (!(strcasecmp(optarg+7, "ATSC_720P"))) {
      set_prefab_target(PREFAB_ATSC_720P, source);
    } else if (!(strcasecmp(optarg+7, "ATSC_1080I"))) {
      set_prefab_target(PREFAB_ATSC_1080I, source);
    } else if (!(strcasecmp(optarg+7, "ATSC_1080P"))) {
      set_prefab_target(PREFAB_ATSC_1080P, source);
    } else {
      mjpeg_error_exit1("Unknown preset keyword: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "SCALE=", 6)) {
    ysRatio s;
    if (s.parse_ratio(optarg+6)) {
      mjpeg_error_exit1("Bad ratio spec: '%s'", optarg);
    }
    _x_ratio = s;
    _y_ratio = s;

  } else if (!strncasecmp(optarg, "XSCALE=", 7)) {
    ysRatio s;
    if (s.parse_ratio(optarg+7)) {
      mjpeg_error_exit1("Bad ratio spec: '%s'", optarg);
    }
    _x_ratio = s;

  } else if (!strncasecmp(optarg, "YSCALE=", 7)) {
    ysRatio s;
    if (s.parse_ratio(optarg+7)) {
      mjpeg_error_exit1("Bad ratio spec: '%s'", optarg);
    }
    _y_ratio = s;

  } else if (!strncasecmp(optarg, "SAR=", 4)) {
    ysRatio sar;
    if (!strcasecmp(optarg+4, "src")) {
      sar = source.stream().sar();
    } else if (!strcasecmp(optarg+4, "NTSC")) {
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

  } else if (!strncasecmp(optarg, "CHROMASS=", 9)) {
    if (_stream.parse_subsampling(optarg+9)) {
      mjpeg_error_exit1("Bad chroma subsampling spec: '%s'", optarg);
    }

  } else if (!strncasecmp(optarg, "ILACE=", 6)) {
    if (!strcasecmp(optarg+6, "TOP_FIRST")) {
      _stream.interlace(Y4M_ILACE_TOP_FIRST);
    } else if (!strcasecmp(optarg+6, "BOTTOM_FIRST")) {
      _stream.interlace(Y4M_ILACE_BOTTOM_FIRST);
    } else if (!strcasecmp(optarg+6, "NONE")) {
      _stream.interlace(Y4M_ILACE_NONE);
    } else {
      mjpeg_error_exit1("Bad interlace spec: '%s'", optarg);
    }

  } else if (!strcasecmp(optarg, "INFER=CLIP")) {
    _reconcile_mode = RCMD_CLIP;

  } else if (!strcasecmp(optarg, "INFER=PAD")) {
    _reconcile_mode = RCMD_PAD;

  } else if (!strcasecmp(optarg, "INFER=PRESERVE_X")) {
    _reconcile_mode = RCMD_PRESERVE_X;

  } else if (!strcasecmp(optarg, "INFER=PRESERVE_Y")) {
    _reconcile_mode = RCMD_PRESERVE_Y;

  } else if (!strcasecmp(optarg, "INFER=SIMPLIFY")) {
    _reconcile_mode2 = RCMD2_SIMPLIFY;

  } else if (!strcasecmp(optarg, "INFER=EXACT")) {
    _reconcile_mode2 = RCMD2_EXACT;

  } else if (!strncasecmp(optarg, "ALIGN=", 6)) {
    _anchor_mode = parse_anchor_mode(optarg+6);
    if (_anchor_mode == ANC_UNKNOWN) 
      mjpeg_error_exit1("Bad anchor mode specifier: '%s'", optarg);

  } else
    mjpeg_error_exit1("Unrecognized output parameter: %s", optarg);

}



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


void ysTarget::require_interlace(const ysSource &source)
{
  if (_stream.interlace() == Y4M_UNKNOWN) {
    mjpeg_info("Target interlacing defaulting to match source.");
    _stream.interlace(source.stream().interlace());
  }
}


void ysTarget::require_chromass(const ysSource &source)
{
  if (!_stream.subsampling().is_known()) {
    mjpeg_info("Target chroma subsampling defaulting to match source.");
    _stream.subsampling(source.stream().subsampling().mode());
  }
}


void ysTarget::require_sar(const ysSource &source)
{
  if (!_stream.sar().is_known()) {
    mjpeg_info("Target SAR defaulting to match source.");
    _stream.sar(source.stream().sar());
  }
}



void ysTarget::require_framesize(const ysSource &source)
{
  int cause_to_exit = 0;

  if (_stream.dim().is_known()) {
    /* size set by user; verify correct alignment */
    int xal = _stream.x_alignment();
    int yal = _stream.y_alignment();

    if (_stream.x_size() % xal) {
      mjpeg_error("Target x size (%d) is not multiple of %d!",
		  _stream.x_size(), xal);
      cause_to_exit = 1;
    }
    if (_stream.y_size() % yal) {
      mjpeg_error("Target y size (%d) is not multiple of %d!",
		  _stream.y_size(), yal);
      cause_to_exit = 1;
    }

  } else {
    /* if both ratios set:        use ratios to calc from source size
       else if one ratio and sar: use ratio and sar to calc from source size
       else:                      use source size */
    ysRatio xsize = source.stream().x_size();
    ysRatio ysize = source.stream().y_size();

    if (_x_ratio.is_known() && _y_ratio.is_known()) {
      mjpeg_info("Target frame size defaulting to scaled source size.");
      xsize *= _x_ratio;
      ysize *= _y_ratio;

    } else if (_x_ratio.is_known() || _y_ratio.is_known()) {
      require_sar(source);
      if (_x_ratio.is_known()) {
	mjpeg_info("Target frame size defaulting to scaled source size.");
	xsize *= _x_ratio;
	ysize *= _x_ratio * _stream.sar() / source.stream().sar();
      } else if (_y_ratio.is_known()) {
	mjpeg_info("Target frame size defaulting to scaled source size.");
	xsize *= _y_ratio / _stream.sar() * source.stream().sar();
	ysize *= _y_ratio;
      }
      
    } else {
      mjpeg_info("Target frame size defaulting to match source.");
    }

    /* perhaps we should adjust instead of flagging error... */
    int xal = _stream.x_alignment();
    int yal = _stream.y_alignment();
    if (!xsize.is_integral()) {
      mjpeg_error("Scaled source x size (%f) is not integral!",
		  xsize.to_double());
      cause_to_exit = 1;
    } else if (xsize.to_int() % xal) {
      mjpeg_error("Scaled source x size (%d) is not multiple of %d!",
		  xsize.to_int(), xal);
      cause_to_exit = 1;
    }

    if (!ysize.is_integral()) {
      mjpeg_error("Scaled source y size (%f) is not integral!",
		  ysize.to_double());
      cause_to_exit = 1;
    } else if (ysize.to_int() % yal) {
      mjpeg_error("Scaled source y size (%d) is not multiple of %d!",
		  ysize.to_int(), yal);
      cause_to_exit = 1;
    }

    _stream.x_size(xsize.to_int());
    _stream.y_size(ysize.to_int());
  }
  if (cause_to_exit) exit(1);
}


void ysTarget::require_active_region(void)
{
#if 0
  if (!_active_region.is_known()) {
    mjpeg_info("Target active region defaulting to target frame size.");
    _active_region = ysRegion(_stream.dim(), ysPoint(0,0));
  } else {
    _active_region.fixate(_stream.dim());
  }
#else
  if (!_active_region.is_known()) {
    if (_active_region.offset().is_known()) {
      mjpeg_info("Target active region defaulting to target frame size.");
      _active_region.dim(_stream.dim());
    } else {
      mjpeg_info("Target active region defaulting to full target frame.");
      _active_region = ysRegion(_stream.dim());
      _active_region.origin_mode(ANC_TL);
    }
  } 
  _active_region.fixate(_stream.dim());
#endif
}




void ysTarget::calculate_y_ratio_from_x_ratio_and_sars(const ysSource &source)
{
  mjpeg_info("Deriving Y ratio from X ratio and SARs...");
  _y_ratio = _x_ratio * _stream.sar() / source.stream().sar();
}

void ysTarget::calculate_x_ratio_from_y_ratio_and_sars(const ysSource &source)
{
  mjpeg_info("Deriving X ratio from Y ratio and SARs...");
  _x_ratio = _y_ratio / _stream.sar() * source.stream().sar();
}


void ysTarget::calculate_ratios_from_active_regions(const ysSource &source)
{
  mjpeg_info("Deriving ratios from active regions...");
  /* calculate scaling ratios */
  _x_ratio = 
    ysRatio(_active_region.dim().x()) / source.active_region().dim().x();
  _y_ratio = 
    ysRatio(_active_region.dim().y()) / source.active_region().dim().y();
  /* calculate target SAR */
  //  _stream.sar(source.stream().sar() / _x_ratio * _y_ratio);
  // ^... subsumed by reconcile_sar_with_ratios()
}


static inline float score_ratios(ysRatio xr, ysRatio yr)
{
  //  return ((double)xr.numerator() * (double)xr.denominator() *
  //	  (double)yr.numerator() * (double)yr.denominator());
  // One only needs to score the denominators --- since the X and Y ratios
  //  are within P% of some optimum, then the numerators are always going
  //  to scale with the denominators.  I.e. since xr is roughly 'F', then
  //    xr.num = F * xr.den --- if the denominator is huge, so is the 
  //  numerator.
  return ((float)xr.denominator() * (float)yr.denominator());
}


/*
  ry = rx * scale_ratio
*/

static inline bool within_percent(float A, float A0, float pcent)
{
  float Amin = A0 * (1.0 - pcent);
  float Amax = A0 * (1.0 + pcent);
  return (Amin < A) && (A < Amax);
}


#define SLIP_LIMIT 0.10
#include <limits.h>

static void find_simpler_scales_x_y(ysRatio &sx, ysRatio &sy,
                                    ysRatio sx_base, ysRatio scale_ratio)
{

  int xn = sx_base.numerator();
  int xd = sx_base.denominator();
  float xf = sx_base.to_float();

  int n;
  int d;
  int n0 = xn * 900 / 1000;
  int n1 = xn * 1100 / 1000;
  int d0 = xd * 900 / 1000;
  int d1 = xd * 1100 / 1000;

  if (n0 <= 0) n0 = 1;
  if (d0 <= 0) d0 = 1;
  //  DBG("n %d-%d  d %d-%d\n", n0, n1, d0, d1);
  
  sx = sx_base;
  sy = sx * scale_ratio;
  float best_score = score_ratios(sx, sy);

  float yf = sy.to_float();

  for (n = n0; n <= n1; n++) {
    for (d = d0; d <= d1; d++) {

      ysRatio newx(n, d);
      ysRatio newy = newx * scale_ratio;
      float newscore = score_ratios(newx, newy);

      //      DBG("n/d  %d/%d   %d/%d   %d\n",
      //	      n, d, newy.numerator(), newy.denominator(), newscore);

      if (newscore < best_score) {
	if (within_percent(newx.to_float(), xf, SLIP_LIMIT) &&
	    within_percent(newy.to_float(), yf, SLIP_LIMIT)) {
	  best_score = newscore;
	  sx = newx;
	  sy = newy;
	  //DBG("better:  %" PRID64_STRING_FORMAT "  %d/%d  %d/%d\n",
	  //best_score, newx.numerator(), newx.denominator(),
	  //newy.numerator(), newy.denominator());
	}
      }
    }
  }

}



void ysTarget::calculate_ratios_from_active_regions_and_sar(const ysSource &source)
{
  mjpeg_info("Deriving ratios from active regions and SARs...");

  ysRatio xratioA = 
    ysRatio(_active_region.dim().x()) / source.active_region().dim().x();
  ysRatio yratioA =
    xratioA * _stream.sar() / source.stream().sar();
    
  ysRatio yratioB = 
    ysRatio(_active_region.dim().y()) / source.active_region().dim().y();
  ysRatio xratioB = 
    yratioB / _stream.sar() * source.stream().sar();

  mjpeg_debug("xrA  %d : %d", xratioA.numerator(), xratioA.denominator());
  mjpeg_debug("yrA  %d : %d", yratioA.numerator(), yratioA.denominator());
  mjpeg_debug("xrB  %d : %d", xratioB.numerator(), xratioB.denominator());
  mjpeg_debug("yrB  %d : %d", yratioB.numerator(), yratioB.denominator());


  int syA = (source.active_region().dim().y() * yratioA).to_int();
  //  int sxB = (source.active_region().dim().x() * xratioB).to_int();

  switch (_reconcile_mode) {
  case RCMD_PRESERVE_X:
    mjpeg_info("...using scaling ratios which preserve X size.");
    _x_ratio = xratioA;
    _y_ratio = yratioA;
    break;
  case RCMD_PRESERVE_Y:
    mjpeg_info("...using scaling ratios which preserve Y size.");
    _x_ratio = xratioB;
    _y_ratio = yratioB;
    break;
  case RCMD_CLIP:
    mjpeg_info("...using scaling ratios which clip target.");
    if (syA > _active_region.dim().y()) { /* 'A' will clip in Y direction */
      _x_ratio = xratioA;
      _y_ratio = yratioA;
    } else { /* 'B' will clip in X direction */
      _x_ratio = xratioB;
      _y_ratio = yratioB;
    }
    break;
    //  default:
  case RCMD_PAD:
    mjpeg_info("...using scaling ratios which pad target.");
    if (syA < _active_region.dim().y()) { /* 'A' will pad in Y direction */
      _x_ratio = xratioA;
      _y_ratio = yratioA;
    } else { /* 'B' will pad in X direction */
      _x_ratio = xratioB;
      _y_ratio = yratioB;
    }
    break;
  }

  mjpeg_debug("pre x  %d : %d", _x_ratio.numerator(), _x_ratio.denominator());
  mjpeg_debug("pre y  %d : %d", _y_ratio.numerator(), _y_ratio.denominator());

  switch (_reconcile_mode2) {
  case RCMD2_SIMPLIFY:
    mjpeg_info("...using scaling ratios which are simple.");
    find_simpler_scales_x_y(_x_ratio, _y_ratio,
			    _x_ratio, _stream.sar() / source.stream().sar());
    break;
  case RCMD2_EXACT:
    //  default:
    mjpeg_info("...using scaling ratios which are exact.");
    break;
  }

  mjpeg_debug("post x  %d : %d", _x_ratio.numerator(), _x_ratio.denominator());
  mjpeg_debug("post y  %d : %d", _y_ratio.numerator(), _y_ratio.denominator());

}



void ysTarget::reconcile_active_regions_with_ratios(ysSource &source)
{
  /*
   * new target region = intersection of (old region) and (proj. of src region)
   * new source region = intersection of (old region) and (proj. of tgt region)
   *
   */

  ysRatioRegion real_tgt(_active_region);

  /* project src region -> tgt space */
  ysRatioRegion proj_src(source.active_region().dim() *
			 ysRatioPoint(_x_ratio, _y_ratio),
			 ysPoint(0,0));
  /* align */
  //  proj_src.align_to(real_tgt, _align_mode);
  /////  proj_src.center_to(real_tgt);
  proj_src.align_to(real_tgt, _anchor_mode);

  /* clip target to projected source */
  if (real_tgt.clip(proj_src) != ysRatioRegion(0,0,0,0)) {
    mjpeg_warn("Target active region clipped by projection of source.");
  }

  /* project new tgt region -> src space */
  ysRatioRegion proj_tgt(real_tgt.dim() / ysRatioPoint(_x_ratio, _y_ratio),
			 ysPoint(0,0));
  /* align */
  //  proj_tgt.align_to(source.active_region(), _align_mode);
  ////  proj_tgt.center_to(source.active_region());
  proj_tgt.align_to(source.active_region(), _anchor_mode);
  /* clip source to projected target */
  if (source.active_region().clip(proj_tgt) != ysRatioRegion(0,0,0,0)) {
    mjpeg_warn("Source active region clipped by projection of target.");
  }

  /* at this point,
   *  source.active_region() and real_tgt are properly aligned with respect
   *  to each other and to the frames, and mutually clipped.
   */

#if 0  /* source doesn't really need to be clipped to frame, since it is
	  matted to at most frame-size anyway. */
  {
    /* clip source to frame */
    ysRatioRegion delta =
      source.active_region().clip(ysRatioPoint(source.stream().dim()));
    if (delta != ysRatioRegion(0,0,0,0)) {
      mjpeg_warn("Source active region clipped by source frame size.");
    }

    ysRatioPoint src_ul_delta = delta.ul();
    ysRatioPoint src_lr_delta = delta.lr();

    /* project delta back to target */
    src_ul_delta *= _x_ratio;
    src_lr_delta *= _y_ratio;

    /* if delta is 'inward', clip target */
    if (src_ul_delta.x() < 0)
      src_ul_delta = ysRatioPoint(0, src_ul_delta.y());
    if (src_ul_delta.y() < 0)
      src_ul_delta = ysRatioPoint(src_ul_delta.x(), 0);
    if (src_lr_delta.x() > 0)
      src_ul_delta = ysRatioPoint(0, src_ul_delta.y());
    if (src_lr_delta.y() > 0)
      src_ul_delta = ysRatioPoint(src_ul_delta.x(), 0);
    
    ysRatioPoint tgt_ul_new = real_tgt.ul() + src_ul_delta;
    ysRatioPoint tgt_lr_new = real_tgt.lr() + src_lr_delta;
    real_tgt = ysRatioRegion(tgt_lr_new - tgt_ul_new, tgt_ul_new);
  }
#endif

  {
    ysRatioRegion delta;

    /* clip target to frame */
    delta = real_tgt.clip(ysRatioPoint(_stream.dim()));
    if (delta != ysRatioRegion(0,0,0,0)) {
      mjpeg_warn("Target active region clipped by target frame size.");
    }
    ysRatioPoint tgt_ul_delta = delta.ul();
    ysRatioPoint tgt_lr_delta = delta.lr();

    /* make target integral and check alignment */
    delta = check_active_alignment(real_tgt);
    tgt_ul_delta += delta.ul();
    tgt_lr_delta += delta.lr();

    DBG("tgt d (%f,%f)  (%f,%f)\n",
	tgt_ul_delta.x().to_double(), tgt_ul_delta.y().to_double(),
	tgt_lr_delta.x().to_double(), tgt_lr_delta.y().to_double());

    /* project delta back to source */
    tgt_ul_delta /= _x_ratio;
    tgt_lr_delta /= _y_ratio;

    /* if delta is 'inward', clip target */
    if (tgt_ul_delta.x() < 0)
      tgt_ul_delta = ysRatioPoint(0, tgt_ul_delta.y());
    if (tgt_ul_delta.y() < 0)
      tgt_ul_delta = ysRatioPoint(tgt_ul_delta.x(), 0);
    if (tgt_lr_delta.x() > 0)
      tgt_ul_delta = ysRatioPoint(0, tgt_ul_delta.y());
    if (tgt_lr_delta.y() > 0)
      tgt_ul_delta = ysRatioPoint(tgt_ul_delta.x(), 0);
    
    ysRatioPoint src_ul_new = source.active_region().ul() + tgt_ul_delta;
    ysRatioPoint src_lr_new = source.active_region().lr() + tgt_lr_delta;
    source.active_region() =
      ysRatioRegion(src_lr_new - src_ul_new, src_ul_new);
  }
  /* set target active region for real */
  _active_region = ysRegion(real_tgt.dim().x().to_int(),
			    real_tgt.dim().y().to_int(),
			    real_tgt.offset().x().to_int(),
			    real_tgt.offset().y().to_int());

}



ysRatioRegion ysTarget::check_active_alignment(ysRatioRegion &r)
{
  int xal = _stream.x_alignment();
  int yal = _stream.y_alignment();
  int rem;
  /* are sizes/offsets compatible with subsampling/interlacing? */

  ysRatio dox, doy, dpx, dpy;
  int ox, oy, px, py;

  /* round 'inwards' */
  if (!r.ul().x().is_integral()) {
    ox = r.ul().x().to_int() + 1;
    dox = ysRatio(ox) - r.ul().x();
  } else {
    ox = r.ul().x().to_int();
    dox = 0;
  }
  if (!r.ul().y().is_integral()) {
    oy = r.ul().y().to_int() + 1;
    doy = ysRatio(oy) - r.ul().y();
  } else {
    oy = r.ul().y().to_int();
    doy = 0;
  }
  if (!r.lr().x().is_integral()) {
    px = r.lr().x().to_int();
    dpx = ysRatio(px) - r.lr().x();
  } else {
    px = r.lr().x().to_int();
    dpx = 0;
  }
  if (!r.lr().y().is_integral()) {
    py = r.lr().y().to_int();
    dpy = ysRatio(py) - r.lr().y();
  } else {
    py = r.lr().y().to_int();
    dpy = 0;
  }

  if ((rem = (ox % xal)) != 0) {
    mjpeg_warn("Target active region x1 (%d) is not multiple of %d...",
		ox, xal);
    mjpeg_warn("   ...shifted right by %d.", rem);
    ox += xal - rem;
    dox += xal - rem;
  }
  if ((rem = (px % xal)) != 0) {
    mjpeg_warn("Target active region x2 (%d) is not multiple of %d...",
		px, xal);
    mjpeg_warn("   ...shifted left by %d.", rem);
    px -= rem;
    dpx -= rem;
  }
  if ((rem = (oy % yal)) != 0) {
    mjpeg_warn("Target active region y1 (%d) is not multiple of %d...",
		oy, yal);
    mjpeg_warn("   ...shifted down by %d.", rem);
    oy += yal - rem;
    doy += yal - rem;
  }
  if ((rem = (py % yal)) != 0) {
    mjpeg_warn("Target active region y2 (%d) is not multiple of %d...",
		py, yal);
    mjpeg_warn("   ...shifted up by %d.", rem);
    py -= rem;
    dpy -= rem;
  }
  r = ysRatioRegion((px - ox), (py - oy), ox, oy);

  return ysRatioRegion((dpx - dox), (dpy - doy), dox, doy);
}





void ysTarget::reconcile_sar_with_ratios(const ysSource &source)
{
  ysRatio out_sar = source.stream().sar() * _y_ratio / _x_ratio;

  if (!_stream.sar().is_known()) {
    _stream.sar(out_sar);
  } else if (_stream.sar() != out_sar) {
    mjpeg_warn("Target SAR mismatch... image will look squished!");
  }
}



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

  /* known:  ilace, framesize
     maybe:  sar, x_ratio, y_ratio, active region
  */
  /* elements:  x ratio, y ratio, sar, active region,   src active region */
  /*  ---> active regions are malleable,
   *       ratios/sar will not be changed once established
   */

  /* priority list for setting scale:
      o x & y ratios 
         - must be consistent with active region
      o x or y ratio, and sar
         - calculate other ratio
	 - must be consistent with active region
      o sar and active region
         - 
  */

void ysTarget::check_parameters(ysSource &source)
{
  require_interlace(source);
  require_chromass(source);

  require_framesize(source);
  require_active_region();

  /* decide Xscale, Yscale, and SAR, if it hasn't happened already */
  if (_x_ratio.is_known() && _y_ratio.is_known()) {
    reconcile_sar_with_ratios(source);
  } else if (_stream.sar().is_known()) {
    if (_x_ratio.is_known()) {
      calculate_y_ratio_from_x_ratio_and_sars(source);
    } else if (_y_ratio.is_known()) {
      calculate_x_ratio_from_y_ratio_and_sars(source);
    } else {
      calculate_ratios_from_active_regions_and_sar(source);
    }
  } else {
    calculate_ratios_from_active_regions(source);
    reconcile_sar_with_ratios(source);
  }


  reconcile_active_regions_with_ratios(source);

}





void ysTarget::describe_parameters() const
{
  mjpeg_info("=== TARGET parameters: =================");
  mjpeg_info("> stream:");
  mjpeg_info(">   %dx%d, SAR %d:%d, %s",
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
  mjpeg_info("> active region:");
  mjpeg_info(">   %dx%d at %d,%d  (bg Y'CbCr: %d,%d,%d)",
	     _active_region.dim().x(),
	     _active_region.dim().y(),
	     _active_region.offset().x(),
	     _active_region.offset().y(),
	     _bgcolor(0), _bgcolor(1), _bgcolor(2));
  mjpeg_info("> X ratio:  %d/%d",
	     _x_ratio.ratio().n,
	     _x_ratio.ratio().d);
  mjpeg_info("> Y ratio:  %d/%d",
	     _y_ratio.ratio().n,
	     _y_ratio.ratio().d);
}



#if 0
static int internal_write_444_frame(int fd, 
				    const y4m_stream_info_t *si, 
				    y4m_frame_info_t *fi,
				    uint8_t * const yuv[3])
{
  int err;
  int w = y4m_si_get_width(si);
  int h = y4m_si_get_height(si);
  
  /* Write frame header */
  if ((err = y4m_write_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Write luminance scanlines */
  if (y4m_write(fd, yuv[0], w*h)) return Y4M_ERR_SYSTEM;
  /* Write chrominance scanlines */
  if (y4m_write(fd, yuv[1], w*h)) return Y4M_ERR_SYSTEM;
  if (y4m_write(fd, yuv[2], w*h)) return Y4M_ERR_SYSTEM;

  return Y4M_OK;
}


static int internal_write_422_frame(int fd, 
				    const y4m_stream_info_t *si, 
				    y4m_frame_info_t *fi,
				    uint8_t * const yuv[3])
{
  int err;
  int w = y4m_si_get_width(si);
  int h = y4m_si_get_height(si);
  
  /* Write frame header */
  if ((err = y4m_write_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Write luminance scanlines */
  if (y4m_write(fd, yuv[0], w*h)) return Y4M_ERR_SYSTEM;
  /* Write chrominance scanlines */
  if (y4m_write(fd, yuv[1], w*h/2)) return Y4M_ERR_SYSTEM;
  if (y4m_write(fd, yuv[2], w*h/2)) return Y4M_ERR_SYSTEM;

  return Y4M_OK;
}


static int internal_write_411_fields(int fd,
				     const y4m_stream_info_t *si,
				     y4m_frame_info_t *fi,
				     uint8_t * const upper_field[3], 
				     uint8_t * const lower_field[3])
{
  int i, y, err;
  int width = y4m_si_get_width(si);
  int height = y4m_si_get_height(si);
  
  /* Write frame header */
  if ((err = y4m_write_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Write Y', Cb, and Cr planes */
  for (i = 0; i < 3; i++) {
    uint8_t *srctop = upper_field[i];
    uint8_t *srcbot = lower_field[i];
    /* alternately write one line from each */
    for (y = 0; y < height; y += 2) {
      if (y4m_write(fd, srctop, width)) return Y4M_ERR_SYSTEM;
      srctop += width;
      if (y4m_write(fd, srcbot, width)) return Y4M_ERR_SYSTEM;
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


static int internal_write_422_fields(int fd,
				     const y4m_stream_info_t *si,
				     y4m_frame_info_t *fi,
				     uint8_t * const upper_field[3], 
				     uint8_t * const lower_field[3])
{
  int i, y, err;
  int width = y4m_si_get_width(si);
  int height = y4m_si_get_height(si);
  
  /* Write frame header */
  if ((err = y4m_write_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Write Y', Cb, and Cr planes */
  for (i = 0; i < 3; i++) {
    uint8_t *srctop = upper_field[i];
    uint8_t *srcbot = lower_field[i];
    /* alternately write one line from each */
    for (y = 0; y < height; y += 2) {
      if (y4m_write(fd, srctop, width)) return Y4M_ERR_SYSTEM;
      srctop += width;
      if (y4m_write(fd, srcbot, width)) return Y4M_ERR_SYSTEM;
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


static int internal_write_444_fields(int fd,
				     const y4m_stream_info_t *si,
				     y4m_frame_info_t *fi,
				     uint8_t * const upper_field[3], 
				     uint8_t * const lower_field[3])
{
  int i, y, err;
  int width = y4m_si_get_width(si);
  int height = y4m_si_get_height(si);
  
  /* Write frame header */
  if ((err = y4m_write_frame_header(fd, si, fi)) != Y4M_OK) return err;
  /* Write Y', Cb, and Cr planes */
  for (i = 0; i < 3; i++) {
    uint8_t *srctop = upper_field[i];
    uint8_t *srcbot = lower_field[i];
    /* alternately write one line from each */
    for (y = 0; y < height; y += 2) {
      if (y4m_write(fd, srctop, width)) return Y4M_ERR_SYSTEM;
      srctop += width;
      if (y4m_write(fd, srcbot, width)) return Y4M_ERR_SYSTEM;
      srcbot += width;
    }
  }
  return Y4M_OK;
}
#endif




#if 0
int ysTarget::write_frame(int fdin, y4m_frame_info_t *frameinfo,
			  uint8_t **frame)
{
  switch (_stream.subsampling().mode()) {
  case ysSubsampling::SS_444:
    return internal_write_444_frame(fdin, _stream.streaminfo(), frameinfo,
				    frame);
  case ysSubsampling::SS_422:
    return internal_write_422_frame(fdin, _stream.streaminfo(), frameinfo,
				    frame);
  case ysSubsampling::SS_411:
  case ysSubsampling::SS_420_JPEG:
  case ysSubsampling::SS_420_MPEG2:
  case ysSubsampling::SS_420_PALDV:
  default:
    return y4m_write_frame(fdin, _stream.streaminfo(), frameinfo, frame);
  }
}

int ysTarget::write_fields(int fdin, y4m_frame_info_t *frameinfo,
			   uint8_t **topfield, uint8_t **bottomfield)
{
  switch (_stream.subsampling().mode()) {
  case ysSubsampling::SS_411:
    return internal_write_411_fields(fdin, _stream.streaminfo(), frameinfo,
				     topfield, bottomfield);
  case ysSubsampling::SS_444:
    return internal_write_444_fields(fdin, _stream.streaminfo(), frameinfo,
				     topfield, bottomfield);
  case ysSubsampling::SS_422:
    return internal_write_422_fields(fdin, _stream.streaminfo(), frameinfo,
				     topfield, bottomfield);
  case ysSubsampling::SS_420_JPEG:
  case ysSubsampling::SS_420_MPEG2:
  case ysSubsampling::SS_420_PALDV:
  default:
    return y4m_write_fields(fdin, _stream.streaminfo(), frameinfo,
			    topfield, bottomfield);
  }
}
#endif


int ysTarget::write_frame(int fdout, y4m_frame_info_t *frameinfo,
			  uint8_t **frame)
{
  return y4m_write_frame(fdout, _stream.streaminfo(), frameinfo, frame);
}


int ysTarget::write_fields(int fdout, y4m_frame_info_t *frameinfo,
			   uint8_t **topfield, uint8_t **bottomfield)
{
  return y4m_write_fields(fdout, _stream.streaminfo(), frameinfo,
                          topfield, bottomfield);
}
