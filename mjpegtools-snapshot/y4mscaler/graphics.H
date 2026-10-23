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

#ifndef _YS_GRAPHICS_H_
#define _YS_GRAPHICS_H_


#include <assert.h>
extern "C" {
#include <yuv4mpeg.h>
}


#define Y_WHITE 235
#define Y_BLACK 16

class ysYCbCr {
public:
  //  const uint8_t Y_black;
  //  const uint8_t Y_white;

  uint8_t Y;
  uint8_t Cb;
  uint8_t Cr;
  uint8_t A;
public:
  ysYCbCr(void) : Y(0), Cb(0), Cr(0), A(0) {}
  ysYCbCr(int y, int cb, int cr, int a = Y_WHITE) :
    Y(y), Cb(cb), Cr(cr), A(a) {}

  uint8_t operator()(int plane) const {
    switch (plane) {
#if 1
    case 0:  return Y;  break;
    case 1:  return Cb; break;
    case 2:  return Cr; break;
    case 3:  return A; break;
#else
    case PLANE_Y:   return Y;  break;
    case PLANE_CB:  return Cb; break;
    case PLANE_CR:  return Cr; break;
    case PLANE_A:   return A; break;
#endif
    default:  assert(0); break;
    }
    return 0;
  }
  static ysYCbCr parse_string(char *);
};


void ys_ratio_reduce(y4m_ratio_t *r);



class ysRatio {
private:
  y4m_ratio_t r;

  static int64_t _gcd64(int64_t a, int64_t b);
  static void _reduce64(int64_t &nn, int64_t &dd);

public:
  ysRatio() {
    r.n = 0;
    r.d = 0;
  }
  ysRatio(int n, int d) {
    r.n = n;
    r.d = d;
    y4m_ratio_reduce(&r);
  }
  ysRatio(const y4m_ratio_t &ar) : r(ar) {
    y4m_ratio_reduce(&r);
  }
  ysRatio(const int &i) {
    r.n = i;
    r.d = 1;
  }

  bool is_known() const { return ((r.n != 0) || (r.d != 0)); }
  bool is_integral() const { return (r.d == 1); }
  
  const y4m_ratio_t &ratio() const { return r; }

  float to_float() const { return (float)r.n / (float)r.d; }
  double to_double() const { return (double)r.n / (double)r.d; }
  int to_int() const { return r.n / r.d; }
  int numerator() const { return r.n; }
  int denominator() const { return r.d; }

  /* comparison operations */
  bool operator==(const ysRatio &rb) const;
  bool operator!=(const ysRatio &rb) const;
  bool operator<(const ysRatio &b) const;
  bool operator>(const ysRatio &b) const;

  /* arithmetic operations */
  ysRatio operator*(const ysRatio &b) const;
  ysRatio operator/(const ysRatio &b) const;
  ysRatio operator+(const ysRatio &b) const;
  ysRatio operator-(const ysRatio &b) const;

  ysRatio &operator*=(const ysRatio &b);
  ysRatio &operator/=(const ysRatio &b);
  ysRatio &operator+=(const ysRatio &b);
  ysRatio &operator-=(const ysRatio &b);

  int parse_ratio(const char *s);
};


class ysRatioPoint;

class ysPoint {
private:
  int _x, _y;
  int _unknown;
public:
  ysPoint() : _x(0), _y(0), _unknown(1) {}
  ysPoint(int x, int y) : _x(x), _y(y), _unknown(0) {}
  
  int x() const { return _x; }
  int y() const { return _y; }
  int area() const { return _x * _y; }

  bool is_known() const { return !_unknown; }

  ysPoint &operator*=(int z);
  ysPoint &operator/=(int z);

  ysPoint operator*(int z) const;
  ysPoint operator/(int z) const;

  ysPoint &operator+=(const ysPoint &b);
  ysPoint &operator-=(const ysPoint &b);
  ysPoint &operator*=(const ysPoint &b);
  ysPoint &operator/=(const ysPoint &b);

  ysPoint operator+(const ysPoint &b) const;
  ysPoint operator-(const ysPoint &b) const;
  ysPoint operator*(const ysPoint &b) const;
  ysPoint operator/(const ysPoint &b) const;

  ysPoint &operator*=(const ysRatioPoint &b);
  ysPoint operator*(const ysRatioPoint &b) const;

};


class ysRatioPoint {
private:
  ysRatio _x, _y;
  int _unknown;
public:
  ysRatioPoint() : _x(0,1), _y(0,1), _unknown(1) {}
  ysRatioPoint(ysRatio x, ysRatio y) : _x(x), _y(y), _unknown(0) {}
  ysRatioPoint(int x, int y) : _x(x), _y(y), _unknown(0) {}
  ysRatioPoint(const ysPoint &p) : _x(p.x()), _y(p.y()), _unknown(0) {}
  
  ysRatio x() const { return _x; }
  ysRatio y() const { return _y; }
  ysRatio area() const { return _x * _y; }

  bool is_known() const { return !_unknown; }

  ysRatioPoint &operator+=(const ysRatioPoint &b);
  ysRatioPoint &operator-=(const ysRatioPoint &b);
  ysRatioPoint &operator*=(const ysRatioPoint &b);
  ysRatioPoint &operator/=(const ysRatioPoint &b);

  ysRatioPoint operator+(const ysRatioPoint &b) const;
  ysRatioPoint operator-(const ysRatioPoint &b) const;
  ysRatioPoint operator*(const ysRatioPoint &b) const;
  ysRatioPoint operator/(const ysRatioPoint &b) const;

  ysRatioPoint &operator*=(const ysRatio &r);
  ysRatioPoint &operator/=(const ysRatio &r);

  ysRatioPoint operator*(const ysRatio &r) const;
  ysRatioPoint operator/(const ysRatio &r) const;
};


enum AnchorMode_t {
  ANC_UNKNOWN,
  ANC_TL,
  ANC_TC,
  ANC_TR,
  ANC_CL,
  ANC_CC,
  ANC_CR,
  ANC_BL,
  ANC_BC,
  ANC_BR
};

enum AnchorMode_t parse_anchor_mode(const char *s);


class ysRatioRegion;

class ysRegion {
private:
  ysPoint _dim;
  ysPoint _offset;
  AnchorMode_t _origin_mode;
public:
  ysRegion() : _origin_mode(ANC_UNKNOWN) {}
  ysRegion(ysPoint dim, ysPoint offset) : _dim(dim), _offset(offset), _origin_mode(ANC_UNKNOWN) {}
  ysRegion(ysPoint dim) : _dim(dim), _offset(0,0), _origin_mode(ANC_UNKNOWN) {}
  ysRegion(int sx, int sy, int ox, int oy) :
    _dim(sx, sy), _offset(ox, oy), _origin_mode(ANC_UNKNOWN) {}

  const ysPoint &dim() const { return _dim; }
  const ysPoint &offset() const { return _offset; }
  bool is_known() const { return (_dim.is_known() && _offset.is_known()); }
  int area() const { return _dim.area(); }

  const ysPoint &ul() const { return _offset; }
  ysPoint lr() const { return _offset + _dim; }

  void dim(const ysPoint &p) { _dim = p; }
  void offset(const ysPoint &p) { _offset = p; }
  void origin_mode(AnchorMode_t mode) { _origin_mode = mode; }
  
  void fixate(const ysPoint &framesize);
  
  int clip(const ysRegion &bounds); /* return 1 if clipping occurs */
  int clip(const ysRatioRegion &bounds); /* return 1 if clipping occurs */
  //  void center_to(const ysRegion &bounds);

  ysPoint rel_anchor(ysRatioPoint &relative) const;
  ysPoint rel_anchor(AnchorMode_t mode) const;

  int parse_geometry(const char *s);
};

class ysRatioRegion {
private:
  ysRatioPoint _dim;
  ysRatioPoint _offset;
  AnchorMode_t _origin_mode;
public:
  ysRatioRegion() : _origin_mode(ANC_UNKNOWN) {}
  ysRatioRegion(ysRatioPoint dim, ysRatioPoint offset) :
    _dim(dim), _offset(offset), _origin_mode(ANC_UNKNOWN) {}
  ysRatioRegion(ysRatioPoint dim) : _dim(dim), _offset(0,0), _origin_mode(ANC_UNKNOWN) {}
  ysRatioRegion(ysRatio sx, ysRatio sy, ysRatio ox, ysRatio oy) :
    _dim(sx, sy), _offset(ox, oy), _origin_mode(ANC_UNKNOWN) {}
  ysRatioRegion(const ysRegion &region) :
    _dim(region.dim()),
    _offset(region.offset()),
    _origin_mode(ANC_UNKNOWN) {}

  const ysRatioPoint &dim() const { return _dim; }
  const ysRatioPoint &offset() const { return _offset; }

  ysRatio area() const { return _dim.area(); }
  bool is_known() const { return (_dim.is_known() && _offset.is_known()); }
  
  const ysRatioPoint &ul() const { return _offset; }
  ysRatioPoint lr() const { return _offset + _dim; }

  void dim(const ysRatioPoint &p) { _dim = p; }
  void offset(const ysRatioPoint &p) { _offset = p; }
  void origin_mode(AnchorMode_t mode) { _origin_mode = mode; }
  
  void fixate(const ysRatioPoint &framesize);
  
  //int clip(const ysRatioRegion &bounds); /* return 1 if clipping occurs */
  ysRatioRegion clip(const ysRatioRegion &bounds); /* return delta */
  void center_to(const ysRatioRegion &bounds);

  void align_to(const ysRatioPoint &relative_anchor,
		const ysRatioPoint &fixed_point);
  void align_to(const ysRatioRegion &bounds, AnchorMode_t mode);
		

  ysRatioPoint rel_anchor(ysRatioPoint &relative) const;
  ysRatioPoint rel_anchor(AnchorMode_t mode) const;


  int parse_geometry(const char *s);

  bool operator==(const ysRatioRegion &b) const {
    return ((_dim.x() == b._dim.x()) && (_dim.y() == b._dim.y()) &&
	    (_offset.x() == b._offset.x()) && (_offset.y() == b._offset.y()));
  }
  bool operator!=(const ysRatioRegion &b) const {
    return !((_dim.x() == b._dim.x()) && (_dim.y() == b._dim.y()) &&
	     (_offset.x() == b._offset.x()) && (_offset.y() == b._offset.y()));
  }
};







class ysSubsampling {
public:
  enum Mode {
    SS_UNKNOWN = Y4M_UNKNOWN,
    SS_444       = Y4M_CHROMA_444,
    SS_422       = Y4M_CHROMA_422,
    SS_420_JPEG  = Y4M_CHROMA_420JPEG,
    SS_420_MPEG2 = Y4M_CHROMA_420MPEG2,
    SS_420_PALDV = Y4M_CHROMA_420PALDV,
    SS_411       = Y4M_CHROMA_411,
    SS_MONO      = Y4M_CHROMA_MONO,
    SS_444ALPHA  = Y4M_CHROMA_444ALPHA
  };
  enum Field {
    FRAME = 0,
    UPPER_FIELD,
    LOWER_FIELD,
    FT_COUNT
  };
  enum Plane {
    PLANE_Y = 0,
    PLANE_Cb,
    PLANE_Cr,
    PLANE_ALPHA,
    PLANES
  };

private:
  Mode _mode;
  ysRatioPoint _ratio;
  ysRatioPoint _offset[FT_COUNT][PLANES]; // pels, relative to luma plane

public:
  ysSubsampling(Mode amode = SS_UNKNOWN) {
    mode(amode);
  }

  Mode mode(void) const { return _mode; }
  void mode(Mode amode);
  bool is_known(void) const { return _mode != SS_UNKNOWN; }
  
  ysRatioPoint ratio(void) const { return _ratio; }
  ysRatioPoint offset(Field field, Plane plane) const {
    return _offset[field][plane]; }

  ysRatioPoint effective_sample_offset(Field field, Plane plane) const;

  const char *mode_to_string() const;

  int parse_mode(const char *s);
};







/*************************************************************************/
/*************************************************************************/
/* operators for ysRatio */
/*************************************************************************/
/*************************************************************************/

inline bool ysRatio::operator==(const ysRatio &rb) const {
  return (is_known() && rb.is_known() &&
	  (r.n == rb.r.n) && (r.d == rb.r.d));
}
inline bool ysRatio::operator!=(const ysRatio &rb) const {
  return (!is_known() || !rb.is_known() ||
	  (r.n != rb.r.n) || (r.d != rb.r.d));
}
inline bool ysRatio::operator<(const ysRatio &b) const {
  return ((r.n * b.r.d) - (r.d * b.r.n)) < 0;
}
inline bool ysRatio::operator>(const ysRatio &b) const {
  return ((r.n * b.r.d) - (r.d * b.r.n)) > 0;
}

/* arithmetic operations */
inline ysRatio &ysRatio::operator*=(const ysRatio &b) {
#if _YS_DEBUG
  if ( ((r.n * b.r.n) != ((int64_t)r.n * (int64_t)b.r.n)) ||
       ((r.d * b.r.d) != ((int64_t)r.d * (int64_t)b.r.d)) )
    mjpeg_error("*= OVERFLOW %d %d   %d %d", r.n, r.d, b.r.n, b.r.d);
#endif
  int64_t nn = (int64_t)r.n * (int64_t)b.r.n;
  int64_t dd = (int64_t)r.d * (int64_t)b.r.d;
  _reduce64(nn, dd);
  r.n = nn;
  r.d = dd;
  return *this;
}
inline ysRatio &ysRatio::operator/=(const ysRatio &b) {
#if _YS_DEBUG
  if ( ((r.n * b.r.d) != ((int64_t)r.n * (int64_t)b.r.d)) ||
       ((r.d * b.r.n) != ((int64_t)r.d * (int64_t)b.r.n)) )
    mjpeg_error("/= OVERFLOW %d %d   %d %d", r.n, r.d, b.r.n, b.r.d);
#endif
  int64_t nn = (int64_t)r.n * (int64_t)b.r.d;
  int64_t dd = (int64_t)r.d * (int64_t)b.r.n;
  _reduce64(nn, dd);
  r.n = nn;
  r.d = dd;
  return *this;
}
inline ysRatio ysRatio::operator*(const ysRatio &b) const {
  //  if ( ((r.n * b.r.n) != ((int64_t)r.n * (int64_t)b.r.n)) ||
  //       ((r.d * b.r.d) != ((int64_t)r.d * (int64_t)b.r.d)) )
  //    mjpeg_error("* OVERFLOW %d %d   %d %d", r.n, r.d, b.r.n, b.r.d);
  //  int64_t nn = (int64_t)r.n * (int64_t)b.r.n;
  //  int64_t dd = (int64_t)r.d * (int64_t)b.r.d;
  //  _reduce64(nn, dd);
  //  return ysRatio(nn, dd);
  ysRatio a = *this;
  a *= b;
  return a;
}
inline ysRatio ysRatio::operator/(const ysRatio &b) const {
  //  if ( ((r.n * b.r.d) != ((int64_t)r.n * (int64_t)b.r.d)) ||
  //       ((r.d * b.r.n) != ((int64_t)r.d * (int64_t)b.r.n)) )
  //    mjpeg_error("/ OVERFLOW %d %d   %d %d", r.n, r.d, b.r.n, b.r.d);
  //  int64_t nn = (int64_t)r.n * (int64_t)b.r.d;
  //  int64_t dd = (int64_t)r.d * (int64_t)b.r.n;
  //  _reduce64(nn, dd);
  //  return ysRatio(nn, dd);
  ysRatio a = *this;
  a /= b;
  return a;
}

inline ysRatio &ysRatio::operator+=(const ysRatio &b) {
  int64_t nn = ( ((int64_t)r.n * (int64_t)b.r.d) +
                 ((int64_t)r.d * (int64_t)b.r.n) );
  int64_t dd = (int64_t)r.d * (int64_t)b.r.d;
  _reduce64(nn, dd);
  r.n = nn;
  r.d = dd;
  return *this;
}
inline ysRatio &ysRatio::operator-=(const ysRatio &b) {
  int64_t nn = ( ((int64_t)r.n * (int64_t)b.r.d) -
                 ((int64_t)r.d * (int64_t)b.r.n) );
  int64_t dd = (int64_t)r.d * (int64_t)b.r.d;
  _reduce64(nn, dd);
  r.n = nn;
  r.d = dd;
  return *this;
}
inline ysRatio ysRatio::operator+(const ysRatio &b) const {
  ysRatio a = *this;
  a += b;
  return a;
  //    return ysRatio( (r.n * b.r.d) + (r.d * b.r.n),
  //		    r.d * b.r.d );
}
inline ysRatio ysRatio::operator-(const ysRatio &b) const {
  ysRatio a = *this;
  a -= b;
  return a;
  //  return ysRatio( (r.n * b.r.d) - (r.d * b.r.n),
  //		  r.d * b.r.d );
}


/*************************************************************************/
/*************************************************************************/
/* operators for ysPoint */
/*************************************************************************/
/*************************************************************************/


inline ysPoint &ysPoint::operator+=(const ysPoint &b) {
  _unknown = _unknown || b._unknown;
  _x += b._x;
  _y += b._y;
  return *this;
}
inline ysPoint &ysPoint::operator-=(const ysPoint &b) {
  _unknown = _unknown || b._unknown;
  _x -= b._x;
  _y -= b._y;
  return *this;
}
inline ysPoint ysPoint::operator+(const ysPoint &b) const {
  ysPoint a = *this;
  return (a += b);
}
inline ysPoint ysPoint::operator-(const ysPoint &b) const {
  ysPoint a = *this;
  return (a -= b);
}

inline ysPoint &ysPoint::operator*=(const ysPoint &b) {
  _unknown = _unknown || b._unknown;
  _x *= b._x;
  _y *= b._y;
  return *this;
}
inline ysPoint &ysPoint::operator/=(const ysPoint &b) {
  _unknown = _unknown || b._unknown;
  if (!_unknown) {
    _x /= b._x;
    _y /= b._y;
  }
  return *this;
}
inline ysPoint ysPoint::operator*(const ysPoint &b) const {
  ysPoint a = *this;
  return (a *= b);
}
inline ysPoint ysPoint::operator/(const ysPoint &b) const {
  ysPoint a = *this;
  return (a /= b);
}

inline ysPoint &ysPoint::operator*=(int z) {
  _x *= z;
  _y *= z;
  return *this;
}
inline ysPoint &ysPoint::operator/=(int z) {
  _x /= z;
  _y /= z;
  return *this;
}
inline ysPoint ysPoint::operator*(int z) const {
  ysPoint a = *this;
  return (a *= z);
}
inline ysPoint ysPoint::operator/(int z) const {
  ysPoint a = *this;
  return (a /= z);
}

inline ysPoint &ysPoint::operator*=(const ysRatioPoint &b) {
  _unknown = _unknown || !(b.is_known());
  if (!_unknown) {
    ysRatio rx = b.x() * _x;
    ysRatio ry = b.y() * _y;
    assert(rx.is_integral());
    assert(ry.is_integral());
    _x = rx.to_int();
    _y = ry.to_int();
  }
  return *this;
}

inline ysPoint ysPoint::operator*(const ysRatioPoint &b) const {
  ysPoint a = *this;
  return (a *= b);
}


/*************************************************************************/
/*************************************************************************/
/* operators for ysRatioPoint */
/*************************************************************************/
/*************************************************************************/

inline ysRatioPoint &ysRatioPoint::operator+=(const ysRatioPoint &b) {
    _unknown = _unknown || b._unknown;
    _x += b._x;
    _y += b._y;
    return *this;
  }
inline ysRatioPoint &ysRatioPoint::operator-=(const ysRatioPoint &b) {
    _unknown = _unknown || b._unknown;
    _x -= b._x;
    _y -= b._y;
    return *this;
  }
inline ysRatioPoint ysRatioPoint::operator+(const ysRatioPoint &b) const {
    ysRatioPoint a = *this;
    return (a += b);
  }
inline ysRatioPoint ysRatioPoint::operator-(const ysRatioPoint &b) const {
    ysRatioPoint a = *this;
    return (a -= b);
  }

inline ysRatioPoint &ysRatioPoint::operator*=(const ysRatioPoint &b) {
    _unknown = _unknown || b._unknown;
    _x *= b._x;
    _y *= b._y;
    return *this;
  }
inline ysRatioPoint &ysRatioPoint::operator/=(const ysRatioPoint &b) {
    _unknown = _unknown || b._unknown;
    if (!_unknown) {
      _x /= b._x;
      _y /= b._y;
    }
    return *this;
  }
inline ysRatioPoint ysRatioPoint::operator*(const ysRatioPoint &b) const {
    ysRatioPoint a = *this;
    return (a *= b);
  }
inline ysRatioPoint ysRatioPoint::operator/(const ysRatioPoint &b) const {
    ysRatioPoint a = *this;
    return (a /= b);
  }

inline ysRatioPoint &ysRatioPoint::operator*=(const ysRatio &r) {
    _unknown = _unknown || !(r.is_known());
    _x *= r;
    _y *= r;
    return *this;
  }
inline ysRatioPoint &ysRatioPoint::operator/=(const ysRatio &r) {
    _unknown = _unknown || !(r.is_known());
    if (!_unknown) {
      _x /= r;
      _y /= r;
    }
    return *this;
  }
inline ysRatioPoint ysRatioPoint::operator*(const ysRatio &r) const {
    ysRatioPoint a = *this;
    return (a *= r);
  }
inline ysRatioPoint ysRatioPoint::operator/(const ysRatio &r) const {
    ysRatioPoint a = *this;
    return (a /= r);
  }



#endif /* _YS_GRAPHICS_H_ */
