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

#include <string.h>
#include <stdio.h>

#include "graphics.H"


// const uint8_t ysYCbCr::Y_black = 16;
// const uint8_t ysYCbCr::Y_white = 219;

ysYCbCr ysYCbCr::parse_string(char *s)
{
  if (!strncasecmp(s, "RGB:", 4)) {
    int r, g, b;
    if (sscanf(s+4, "%u,%u,%u", &r, &g, &b) == 3) {
      return ysYCbCr((int)(16.5 + 219.0 * (0.299 * (double)r / 255.0 +
					   0.587 * (double)g / 255.0 +
					   0.114 * (double)b / 255.0)),
                     (int)(128.5 + 224.0 * (-0.168736 * (double)r / 255.0 +
					    -0.331264 * (double)g / 255.0 +
					    0.500 * (double)b / 255.0)),
                     (int)(128.5 + 224.0 * (0.500 * (double)r / 255.0 +
					    -0.418688 * (double)g / 255.0 +
					    -0.081312 * (double)b / 255.0))
                     );
    }
  } else if (!strncasecmp(s, "YCBCR:", 6)) {
    int y, cb, cr;
    if (sscanf(s+6, "%u,%u,%u", &y, &cb, &cr) == 3) {
      return ysYCbCr(y, cb, cr);
    }
  } if (!strncasecmp(s, "RGBA:", 5)) {
    int r, g, b, a;
    if (sscanf(s+5, "%u,%u,%u,%u", &r, &g, &b, &a) == 4) {
      return ysYCbCr((int)(16.5 + 219.0 * (0.299 * (double)r / 255.0 +
					   0.587 * (double)g / 255.0 +
					   0.114 * (double)b / 255.0)),
                     (int)(128.5 + 224.0 * (-0.168736 * (double)r / 255.0 +
					    -0.331264 * (double)g / 255.0 +
					    0.500 * (double)b / 255.0)),
                     (int)(128.5 + 224.0 * (0.500 * (double)r / 255.0 +
					    -0.418688 * (double)g / 255.0 +
					    -0.081312 * (double)b / 255.0)),
                     (int)(16.5 + 219.0 * (double)a / 255.0)
                     );
    }
  } else if (!strncasecmp(s, "YCBCRA:", 7)) {
    int y, cb, cr, a;
    if (sscanf(s+7, "%u,%u,%u,%u", &y, &cb, &cr, &a) == 4) {
      return ysYCbCr(y, cb, cr, a);
    }
  }
  return ysYCbCr();
}



/*
 *  Euler's algorithm for greatest common divisor
 */

int64_t ysRatio::_gcd64(int64_t a, int64_t b)
{
  a = (a >= 0) ? a : -a;
  b = (b >= 0) ? b : -b;

  while (b > 0) {
    int64_t x = b;
    b = a % b;
    a = x;
  }
  return a;
}


void ysRatio::_reduce64(int64_t &nn, int64_t &dd)
{
  int64_t c;
  if ((nn == 0) && (dd == 0)) return;  /* "unknown" */
  c = _gcd64(nn, dd);
  nn = (int64_t)nn / c;
  dd = (int64_t)dd / c;
}



/*
  a ratio can be expressed as either "N:D" or "N/D" (same meaning either way)
  sometimes one way is easier to think of than the other...

  D must be strictly positive.
*/

int ysRatio::parse_ratio(const char *s)
{
  int n, d;
  if ( !( (sscanf(s, "%d/%d", &n, &d) == 2) ||
	  (sscanf(s, "%d:%d", &n, &d) == 2) )  ||
       (d <= 0) )
       //       ((n == 0) && (d == 0)) )
    return 1;
  r.n = n;
  r.d = d;
  y4m_ratio_reduce(&r);
  return 0;
}


int ysRegion::parse_geometry(const char *s)
{
  /*   XXXXxYYYY+NNN+NNNcc  */
  int sx, sy, ox, oy;
  char origin[5], signX, signY;
  int args;

  if (strchr(s, 'x')) {
    args = sscanf(s, "%ux%u%c%d%c%d%2s",
		  &sx, &sy, &signX, &ox, &signY, &oy, origin);
    if ((args != 6) && (args != 7)) return 1;
    _dim = ysPoint(sx, sy);
  } else {
    args = sscanf(s, "%c%d%c%d%2s",
		  &signX, &ox, &signY, &oy, origin);
    if ((args != 4) && (args != 5)) return 1;
    _dim = ysPoint();
  }
  DBG("signX %c  signY %c\n", signX, signY);
  switch (signX) {
  case '+': break;
  case '-': ox = -ox; break;
  default: return 1;
  }
  switch (signY) {
  case '+': break;
  case '-': oy = -oy; break;
  default: return 1;
  }
  if (args == 7) {
    _origin_mode = parse_anchor_mode(origin);
    if (_origin_mode == ANC_UNKNOWN) return 1;
  } else {
    _origin_mode = ANC_TL;
  }
  _offset = ysPoint(ox, oy);
  return 0;
}


#if 0
int ysRatioRegion::parse_geometry(const char *s)
{
  /*   XXXXxYYYY+NNN+NNNcc  */
  int sx, sy, ox, oy;
  char origin[5], signX, signY;
  int args;
  args = sscanf(s, "%ux%u%c%d%c%d%2s",
		&sx, &sy, &signX, &ox, &signY, &oy, origin);
  if ((args != 6) && (args != 7)) return 1;
  DBG("signX %c  signY %c\n", signX, signY);
  switch (signX) {
  case '+': break;
  case '-': ox = -ox; break;
  default: return 1;
  }
  switch (signY) {
  case '+': break;
  case '-': oy = -oy; break;
  default: return 1;
  }
  if (args == 7) {
    _origin_mode = parse_anchor_mode(origin);
    if (_origin_mode == ANC_UNKNOWN) return 1;
  } else {
    _origin_mode = ANC_TL;
  }
  _dim = ysRatioPoint(sx, sy);
  _offset = ysRatioPoint(ox, oy);
  return 0;
}
#else
int ysRatioRegion::parse_geometry(const char *s)
{
  /*   XXXXxYYYY+NNN+NNNcc  */
  int sx, sy, ox, oy;
  char origin[5], signX, signY;
  int args;

  if (strchr(s, 'x')) {
    args = sscanf(s, "%ux%u%c%d%c%d%2s",
		  &sx, &sy, &signX, &ox, &signY, &oy, origin);
    
    if ((args != 6) && (args != 7)) return 1;
    _dim = ysRatioPoint(sx, sy);
  } else {
    args = sscanf(s, "%c%d%c%d%2s",
		  &signX, &ox, &signY, &oy, origin);
    
    if ((args != 4) && (args != 5)) return 1;
    _dim = ysRatioPoint(); /* unknown */
  }

  DBG("signX %c  signY %c\n", signX, signY);
  switch (signX) {
  case '+': break;
  case '-': ox = -ox; break;
  default: return 1;
  }
  switch (signY) {
  case '+': break;
  case '-': oy = -oy; break;
  default: return 1;
  }
  if (args == 7) {
    _origin_mode = parse_anchor_mode(origin);
    if (_origin_mode == ANC_UNKNOWN) return 1;
  } else {
    _origin_mode = ANC_TL;
  }
  _offset = ysRatioPoint(ox, oy);
  return 0;
}
#endif


enum AnchorMode_t parse_anchor_mode(const char *s)
{
  if (!strcasecmp("TL", s)) 
    return ANC_TL;
  else if (!strcasecmp("TC", s)) 
    return ANC_TC;
  else if (!strcasecmp("TR", s)) 
    return ANC_TR;
  else if (!strcasecmp("CL", s)) 
    return ANC_CL;
  else if (!strcasecmp("CC", s)) 
    return ANC_CC;
  else if (!strcasecmp("CR", s)) 
    return ANC_CR;
  else if (!strcasecmp("BL", s)) 
    return ANC_BL;
  else if (!strcasecmp("BC", s)) 
    return ANC_BC;
  else if (!strcasecmp("BR", s)) 
    return ANC_BR;
  else 
    return ANC_UNKNOWN;
}



int ysRegion::clip(const ysRegion &bounds)
{
  int clipped = 0;
  int ox = _offset.x();
  int oy = _offset.y();
  int px = ox + _dim.x();
  int py = oy + _dim.y();

  if (ox < bounds.offset().x()) {
    ox = bounds.offset().x();
    clipped = 1;
  }
  if (oy < bounds.offset().y()) {
    oy = bounds.offset().y();
    clipped = 1;
  }
  if (px > (bounds.offset().x() + bounds.dim().x())) {
    px = bounds.offset().x() + bounds.dim().x();
    clipped = 1;
  }
  if (py > (bounds.offset().y() + bounds.dim().y())) {
    py = bounds.offset().y() + bounds.dim().y();
    clipped = 1;
  }
  if (clipped) {
    _offset = ysPoint(ox,oy);
    _dim = ysPoint(px - ox, py - oy);
  }
  return clipped;
}


int ysRegion::clip(const ysRatioRegion &bounds)
{
  int clipped = 0;
  ysRatio ox = _offset.x();
  ysRatio oy = _offset.y();
  ysRatio px = ox + _dim.x();
  ysRatio py = oy + _dim.y();

  DBG("ox,oy:  %f, %f\n", ox.to_double(), oy.to_double());
  DBG("px,py:  %f, %f\n", px.to_double(), py.to_double());
  DBG("b ox,y:  %f, %f\n", bounds.offset().x().to_double(), bounds.offset().y().to_double());
  DBG("b dx,y:  %f, %f\n", bounds.dim().x().to_double(), bounds.dim().y().to_double());
  if (ox < bounds.offset().x()) {
    ox = bounds.offset().x();
    clipped = 1;
  }
  if (oy < bounds.offset().y()) {
    oy = bounds.offset().y();
    clipped = 1;
  }
  if (px > (bounds.offset().x() + bounds.dim().x())) {
    px = bounds.offset().x() + bounds.dim().x();
    clipped = 1;
  }
  if (py > (bounds.offset().y() + bounds.dim().y())) {
    py = bounds.offset().y() + bounds.dim().y();
    clipped = 1;
  }
  if (clipped) {
    /* round TL corner upwards (inwards)... */
    _offset = ysPoint((int)(ox.to_double() + 0.5),
		      (int)(oy.to_double() + 0.5));
    /* round BR corner downwards (inwards)... */
    _dim = ysPoint((px - ox).to_int(),
		   (py - oy).to_int());
    DBG("o x,y:  %d, %d\n", _offset.x(), _offset.y());
    DBG("d x,y:  %d, %d\n", _dim.x(), _dim.y());
  }
  return clipped;
}

#if 0
void ysRegion::center_to(const ysRegion &bounds)
{
  int ox = _offset.x();
  int oy = _offset.y();
  int cx = ox + (_dim.x() / 2);
  int cy = oy + (_dim.y() / 2);
  
  ox += (bounds.offset().x() + (bounds.dim().x() / 2)) - cx;
  oy += (bounds.offset().y() + (bounds.dim().y() / 2)) - cy;
  _offset = ysPoint(ox, oy);
}
#endif

#if 0
int ysRatioRegion::clip(const ysRatioRegion &bounds)
{
  int clipped = 0;
  ysRatio ox = _offset.x();
  ysRatio oy = _offset.y();
  ysRatio px = ox + _dim.x();
  ysRatio py = oy + _dim.y();

  if (ox < bounds.offset().x()) {
    ox = bounds.offset().x();
    clipped = 1;
  }
  if (oy < bounds.offset().y()) {
    oy = bounds.offset().y();
    clipped = 1;
  }
  if (px > (bounds.offset().x() + bounds.dim().x())) {
    px = bounds.offset().x() + bounds.dim().x();
    clipped = 1;
  }
  if (py > (bounds.offset().y() + bounds.dim().y())) {
    py = bounds.offset().y() + bounds.dim().y();
    clipped = 1;
  }
  if (clipped) {
    _offset = ysRatioPoint(ox,oy);
    _dim = ysRatioPoint(px - ox, py - oy);
  }
  return clipped;
}
#else
ysRatioRegion ysRatioRegion::clip(const ysRatioRegion &bounds)
{
  int clipped = 0;
  ysRatio ox = _offset.x();
  ysRatio oy = _offset.y();
  ysRatio px = ox + _dim.x();
  ysRatio py = oy + _dim.y();
  ysRatio dox = 0;
  ysRatio doy = 0;
  ysRatio dpx = 0;
  ysRatio dpy = 0;

  if (ox < bounds.ul().x()) {
    dox = bounds.ul().x() - ox;
    ox = bounds.ul().x();
    clipped = 1;
  }
  if (oy < bounds.ul().y()) {
    doy = bounds.ul().y() - oy;
    oy = bounds.ul().y();
    clipped = 1;
  }
  if (px > bounds.lr().x()) {
    dpx = bounds.lr().x()  - px;
    px = bounds.lr().x();
    clipped = 1;
  }
  if (py > bounds.lr().y()) {
    dpy = bounds.lr().y() - py;
    py = bounds.lr().y();
    clipped = 1;
  }
  if (clipped) {
    _offset = ysRatioPoint(ox,oy);
    _dim = ysRatioPoint(px - ox, py - oy);
  }
  return ysRatioRegion((dpx - dox), (dpy - doy), dox, doy);
}
#endif

void ysRatioRegion::center_to(const ysRatioRegion &bounds)
{
  ysRatio ox = _offset.x();
  ysRatio oy = _offset.y();
  ysRatio cx = ox + (_dim.x() / 2);
  ysRatio cy = oy + (_dim.y() / 2);
  
  ox += (bounds.offset().x() + (bounds.dim().x() / 2)) - cx;
  oy += (bounds.offset().y() + (bounds.dim().y() / 2)) - cy;
  _offset = ysRatioPoint(ox, oy);
}


#if 1
void ysRegion::fixate(const ysPoint &framesize)
{
  //align_to(rel_anchor(_origin_mode),
  //	   ysRatioRegion(framesize, ysPoint(0,0)).rel_anchor(_origin_mode));
  DBG("ba %d %d    ta %d %d\n",
	  ysRegion(framesize).rel_anchor(_origin_mode).x(),
	  ysRegion(framesize).rel_anchor(_origin_mode).y(),
	  rel_anchor(_origin_mode).x(),
	  rel_anchor(_origin_mode).y());

  _offset +=
    ysRegion(framesize).rel_anchor(_origin_mode)
    - rel_anchor(_origin_mode);

}



/* define anchor point as named corner point */
ysPoint ysRegion::rel_anchor(AnchorMode_t mode) const
{
  switch (mode) {
  case ANC_TL:
    return ysPoint(0,0);
  case ANC_TC:
    return ysPoint(_dim.x() / 2, 0);
  case ANC_TR:
    return ysPoint(_dim.x(), 0);
  case ANC_CL:
    return ysPoint(0, _dim.y() / 2);
  case ANC_CC:
    return ysPoint(_dim.x() / 2, _dim.y() / 2);
  case ANC_CR:
    return ysPoint(_dim.x(), _dim.y() / 2);
  case ANC_BL:
    return ysPoint(0, _dim.y());
  case ANC_BC:
    return ysPoint(_dim.x() / 2, _dim.y());
  case ANC_BR:
    return ysPoint(_dim.x(), _dim.y());
  case ANC_UNKNOWN:
    return ysPoint();
  }
  return ysPoint();
}



void ysRatioRegion::fixate(const ysRatioPoint &framesize)
{
  //align_to(rel_anchor(_origin_mode),
  //	   ysRatioRegion(framesize, ysPoint(0,0)).rel_anchor(_origin_mode));
  DBG("ba %f %f    ta %f %f\n",
	  ysRatioRegion(framesize).rel_anchor(_origin_mode).x().to_double(),
	  ysRatioRegion(framesize).rel_anchor(_origin_mode).y().to_double(),
	  rel_anchor(_origin_mode).x().to_double(),
	  rel_anchor(_origin_mode).y().to_double());

  _offset +=
    ysRatioRegion(framesize).rel_anchor(_origin_mode)
    - rel_anchor(_origin_mode);

}


/* move *this so that its relative 'anchor' point coincides with the
   absolute 'fixed point' */
void ysRatioRegion::align_to(const ysRatioPoint &relative_anchor,
			     const ysRatioPoint &fixed_point)
{
  _offset = fixed_point - relative_anchor;
}


/* move *this so that its relative 'anchor' point coincides with the
   absolute 'fixed point' */
void ysRatioRegion::align_to(const ysRatioRegion &bounds,
			     AnchorMode_t mode)
			     
{
  _offset = (bounds.offset() + bounds.rel_anchor(mode)) - rel_anchor(mode);
}



// something.align_to(something.rel_anchor(ANC_CC), 
//                    bounds.offset() + bounds.rel_anchor(ANC_CC));


/* define anchor point by fractional distance from TL */
ysRatioPoint ysRatioRegion::rel_anchor(ysRatioPoint &relative) const
{
  return _dim * relative;
}


/* define anchor point as named corner point */
ysRatioPoint ysRatioRegion::rel_anchor(AnchorMode_t mode) const
{
  switch (mode) {
  case ANC_TL:
    return ysRatioPoint(0,0);
  case ANC_TC:
    return ysRatioPoint(_dim.x() / 2, 0);
  case ANC_TR:
    return ysRatioPoint(_dim.x(), 0);
  case ANC_CL:
    return ysRatioPoint(0, _dim.y() / 2);
  case ANC_CC:
    return ysRatioPoint(_dim.x() / 2, _dim.y() / 2);
  case ANC_CR:
    return ysRatioPoint(_dim.x(), _dim.y() / 2);
  case ANC_BL:
    return ysRatioPoint(0, _dim.y());
  case ANC_BC:
    return ysRatioPoint(_dim.x() / 2, _dim.y());
  case ANC_BR:
    return ysRatioPoint(_dim.x(), _dim.y());
  case ANC_UNKNOWN:
    return ysRatioPoint();
  }
  return ysRatioPoint();
}

#if 0
/* move *this so that its anchor point coincides with the
 * anchor point of bounds
 */
void ysRatioRegion::align_to(const ysRatioRegion &bounds)
{
  ysRatio ox = _offset.x();
  ysRatio oy = _offset.y();
  ysRatio cx = ox + (_dim.x() / 2);
  ysRatio cy = oy + (_dim.y() / 2);
  
  ox += (bounds.offset().x() + (bounds.dim().x() / 2)) - cx;
  oy += (bounds.offset().y() + (bounds.dim().y() / 2)) - cy;
  _offset = ysRatioPoint(ox, oy);
}
#endif
#endif


#if 0
void ysSubsampling::mode(Mode amode)
{
  _mode = amode;
  _offset[FRAME][PLANE_Y] = ysRatioPoint(0,0);
  _offset[UPPER_FIELD][PLANE_Y] = ysRatioPoint(0,0);
  _offset[LOWER_FIELD][PLANE_Y] = ysRatioPoint(0,0);
  switch (amode) {
  case SS_UNKNOWN:
    _ratio = ysRatioPoint();
    _offset[FRAME][PLANE_Cb] = ysRatioPoint();
    _offset[FRAME][PLANE_Cr] = ysRatioPoint();
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint();
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint();
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint();
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint();
    break; // leave everything unknown;
  case SS_444:
    _ratio = ysRatioPoint(1,1);
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    break;
  case SS_422:
    _ratio = ysRatioPoint(ysRatio(1,2), ysRatio(1));
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    break;
  case SS_420_JPEG:
    _ratio = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    break;
  case SS_420_MPEG2:
    _ratio = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(ysRatio(0), ysRatio(1,2));
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(ysRatio(0), ysRatio(1,2));
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(0), ysRatio(1,4));
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(0), ysRatio(1,4));
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(0), ysRatio(3,4));
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(0), ysRatio(3,4));
    break;
  case SS_420_PALDV:
    _ratio = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(); // unknown
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(); // unknonw
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0, 1);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0, 0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0, 1);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0, 0);
    break;
  case SS_411:
    _ratio = ysRatioPoint(ysRatio(1,4), ysRatio(1));
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    break;
  }
}
#endif

void ysSubsampling::mode(Mode amode)
{
  _mode = amode;

  _ratio = ysRatioPoint(y4m_chroma_ss_x_ratio(_mode),
                        y4m_chroma_ss_y_ratio(_mode));

  _offset[FRAME][PLANE_Y] = ysRatioPoint(0,0);
  _offset[UPPER_FIELD][PLANE_Y] = ysRatioPoint(0,0);
  _offset[LOWER_FIELD][PLANE_Y] = ysRatioPoint(0,0);
  _offset[FRAME][PLANE_ALPHA] = ysRatioPoint(0,0);
  _offset[UPPER_FIELD][PLANE_ALPHA] = ysRatioPoint(0,0);
  _offset[LOWER_FIELD][PLANE_ALPHA] = ysRatioPoint(0,0);

  switch (amode) {
  case SS_UNKNOWN:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint();
    _offset[FRAME][PLANE_Cr] = ysRatioPoint();
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint();
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint();
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint();
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint();
    break; // leave everything unknown;
  case SS_444:
  case SS_444ALPHA:
  case SS_MONO:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    break;
  case SS_422:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    break;
  case SS_420_JPEG:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(1,2), ysRatio(1,2));
    break;
  case SS_420_MPEG2:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(ysRatio(0), ysRatio(1,2));
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(ysRatio(0), ysRatio(1,2));
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(0), ysRatio(1,4));
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(0), ysRatio(1,4));
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(ysRatio(0), ysRatio(3,4));
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(ysRatio(0), ysRatio(3,4));
    break;
  case SS_420_PALDV:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(); // unknown
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(); // unknonw
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0, 1);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0, 0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0, 1);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0, 0);
    break;
  case SS_411:
    _offset[FRAME][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[FRAME][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[UPPER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cb] = ysRatioPoint(0,0);
    _offset[LOWER_FIELD][PLANE_Cr] = ysRatioPoint(0,0);
    break;
  }
}


/*
 * continuous position of (chroma) sample:  x = (m - q) + 1/2
 *
 * for 1:K subsampling, with pel offset D,
 *  q = 1/2 - (2D + 1)/(2K)
 */
ysRatioPoint ysSubsampling::effective_sample_offset(Field field, Plane plane) const
{
  ysRatioPoint D = offset(field, plane);
  ysRatioPoint K = ratio();
  //  return ysRatioPoint( ysRatio(1,2) - (D.x() * 2 + 1) / (K.x() / 2)),
  //		           ysRatio(1,2) - (D.y() * 2 + 1) / (K.y() / 2)) );
  return ysRatioPoint( (ysRatio(1) - (D.x() * 2 + 1) * K.x()) / 2,
		       (ysRatio(1) - (D.y() * 2 + 1) * K.y()) / 2 );
}


int ysSubsampling::parse_mode(const char *s)
{
  int chroma = y4m_chroma_parse_keyword(s);
  if (chroma != Y4M_UNKNOWN) {
    switch (chroma) {
    case Y4M_CHROMA_420JPEG:
      mode(ysSubsampling::SS_420_JPEG); break;
    case Y4M_CHROMA_420MPEG2:
      mode(ysSubsampling::SS_420_MPEG2); break;
    case Y4M_CHROMA_420PALDV:
      mode(ysSubsampling::SS_420_PALDV); break;
    case Y4M_CHROMA_411: 
      mode(ysSubsampling::SS_411); break;
    case Y4M_CHROMA_422:  
      mode(ysSubsampling::SS_422); break;
    case Y4M_CHROMA_444:
      mode(ysSubsampling::SS_444); break;
    case Y4M_CHROMA_444ALPHA: 
      mode(ysSubsampling::SS_444ALPHA); break;
      //      mjpeg_error("4:4:4/alpha streams not yet supported.");
      //      return 1;
    case Y4M_CHROMA_MONO:
      mode(ysSubsampling::SS_MONO); break;
      //      mjpeg_error("Mono/Luma-only streams not yet supported.");
      //      return 1;
    default:
      mjpeg_error("Unknown (and/or unsupported) chroma format.");
      return 1;
    }
    return 0;
  }
  /* fall-back to old-style keywords */
  if (!strcasecmp("444", s)) {
    mode(SS_444);
    return 0;
  } else if (!strcasecmp("422", s)) {
    mode(SS_422);
    return 0;
  } else if (!strcasecmp("420_jpeg", s)) {
    mode(SS_420_JPEG);
    return 0;
  } else if (!strcasecmp("420_mpeg2", s)) {
    mode(SS_420_MPEG2);
    return 0;
  } else if (!strcasecmp("420_paldv", s)) {
    mode(SS_420_PALDV);
    return 0;
  } else if (!strcasecmp("411", s)) {
    mode(SS_411);
    return 0;
  } else {
    return 1;
  }
}


#if 0
const char *ysSubsampling::mode_to_string() const
{
  switch (_mode) {
  case SS_444:  return "444";
  case SS_422:  return "422";
  case SS_420_JPEG:  return "420_JPEG";
  case SS_420_MPEG2:  return "420_MPEG2";
  case SS_420_PALDV:  return "420_PALDV";
  case SS_411:  return "411";
  case SS_UNKNOWN:  return "*unknown*";
  }
  return "*unknown*";
}
#endif

const char *ysSubsampling::mode_to_string() const
{
  const char *s = y4m_chroma_description(_mode);
  if (s == NULL)
    return "*unknown*";
  else
    return s;
}
