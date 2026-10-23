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

#ifndef _YS_TARGET_H_
#define _YS_TARGET_H_

#include <stdio.h>
#include "graphics.H"
#include "ysStreamInfo.H"
#include "ysSource.H"


class ysTarget {
public:
  enum ReconcileMode_t {
    RCMD_PAD,
    RCMD_CLIP,
    RCMD_PRESERVE_X,
    RCMD_PRESERVE_Y
  };
  enum ReconcileMode2_t {
    RCMD2_SIMPLIFY,
    RCMD2_EXACT
  };
  enum PrefabType_t {
    PREFAB_VCD,
    PREFAB_CVD,
    PREFAB_SVCD,
    PREFAB_DVD,
    PREFAB_DVD_WIDE,
    PREFAB_DV,
    PREFAB_DV_WIDE,
    PREFAB_SVCD_STILL_HI,
    PREFAB_SVCD_STILL_LO,
    PREFAB_VCD_STILL_HI,
    PREFAB_VCD_STILL_LO,
    PREFAB_ATSC_720P,
    PREFAB_ATSC_1080I,
    PREFAB_ATSC_1080P
  };
		    
private:
  ysStreamInfo _stream;
  ysRegion _active_region;
  ysRatio _x_ratio;
  ysRatio _y_ratio;
  ReconcileMode_t _reconcile_mode;
  ReconcileMode2_t _reconcile_mode2;
  ysYCbCr _bgcolor;
  AnchorMode_t _anchor_mode;

  void set_prefab_target(PrefabType_t target, const ysSource &source);

  void require_interlace(const ysSource &source);
  void require_chromass(const ysSource &source);
  void require_sar(const ysSource &source);
  void require_framesize(const ysSource &source);
  void require_active_region(void);

  void reconcile_sar_with_ratios(const ysSource &source);
  void reconcile_active_regions_with_ratios(ysSource &source);
  void calculate_y_ratio_from_x_ratio_and_sars(const ysSource &source);
  void calculate_x_ratio_from_y_ratio_and_sars(const ysSource &source);
  void calculate_ratios_from_active_regions_and_sar(const ysSource &source);
  void calculate_ratios_from_active_regions(const ysSource &source);

  ysRatioRegion check_active_alignment(ysRatioRegion &r);

#ifdef _YS_DEV_BRANCH_
  static ysYCbCr default_bgcolor(void) { return ysYCbCr(128,240,108); }
#else
  static ysYCbCr default_bgcolor(void) { return ysYCbCr(16,128,128); }
#endif
public:
  ysTarget() :
    _reconcile_mode(RCMD_PAD),
    _reconcile_mode2(RCMD2_SIMPLIFY),
    _bgcolor(default_bgcolor()),
    _anchor_mode(ANC_CC)
  {}
  
  ysYCbCr bgcolor() const { return _bgcolor; }

  ysRatio x_ratio() const { return _x_ratio; }
  ysRatio y_ratio() const { return _y_ratio; }

  void bgcolor(const ysYCbCr &x) { _bgcolor = x; }
  void x_ratio(const ysRatio &r) { _x_ratio = r; }
  void y_ratio(const ysRatio &r) { _y_ratio = r; }

  ysStreamInfo &stream() { return _stream; }
  ysRegion &active_region() { return _active_region; }
  const ysStreamInfo &stream() const { return _stream; }
  ysRegion active_region() const { return _active_region; }

  
  int write_frame(int fdin, y4m_frame_info_t *frameinfo, uint8_t **frame);
  int write_fields(int fdin, y4m_frame_info_t *frameinfo,
		   uint8_t **topfield, uint8_t **bottomfield);

  void describe_keywords(FILE *fp, const char *prefix) const;
  void init_stream(const ysSource &source);
  void parse_keyword(ysSource &source, char *optarg);

  void check_parameters(ysSource &source);
  void describe_parameters() const;

};



#endif /* _YS_TARGET_H_ */
