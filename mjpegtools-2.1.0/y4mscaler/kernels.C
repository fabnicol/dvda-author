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
#include "kernels.H"

#include <stdlib.h>
#include <math.h>
#include <string.h>


/********************************************************************
 * Plain-old Polynomial Kernels
 ********************************************************************/

class BoxKernel : public ysKernel {
public:
  virtual const char *name() const { return "box"; }
  virtual const char *desc() const {
    return "Box (zero order)"; }
  virtual ysKernel *clone() const { return new BoxKernel(*this); }
  virtual double k(double t) const {
#if 0
    if (t < -0.5) return 0.0;
    if (t < 0.5) return 1.0;
#elif 0
    if (t <= -0.5) return 0.0;
    if (t < 0.5) return 1.0;
#else
    if (t < -0.5) return 0.0;
    if (t <= 0.5) return 1.0;
#endif
    return 0.0;
  }
  virtual double support() const { return 0.5; }
};


class LinearKernel : public ysKernel {
public:
  virtual const char *name() const { return "linear"; }
  virtual const char *desc() const {
    return "Linear (1st order)"; }
  virtual ysKernel *clone() const { return new LinearKernel(*this); }
  virtual double k(double t) const {
    if (t <= -1.0) return 0.0;
    if (t < 0.0) return (t + 1.0);
    if (t < 1.0) return (1.0 - t);
    return 0.0;
  }
  virtual double support() const { return 1.0; }
};


class QuadraticKernel : public ysKernel {
public:
  virtual const char *name() const { return "quadratic"; }
  virtual const char *desc() const {
    return "Quadratic (2rd order)"; }
  virtual ysKernel *clone() const { return new QuadraticKernel(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t < 0.5) 
      return 0.75 - (t * t);
    if (t < 1.5) {
      t -= 1.5;
      return 0.5 * t * t;
    }
    return 0.0;
  }
  virtual double support() const { return 1.5; }
};


/********************************************************************
 * Cubic Kernels (BC-splines)
 *
 *           B-spline:  B=1,   C=0
 * Mitchell-Netravali:  B=1/3, C=1/3
 *        Catmull-Rom:  B=0,   C=1/2
 ********************************************************************/

class CubicBKernel : public ysKernel {
public:
  virtual const char *name() const { return "cubicB"; }
  virtual const char *desc() const { return "Cubic, B-spline (3rd order)"; }
  virtual ysKernel *clone() const { return new CubicBKernel(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t < 1.0) {
      return (4.0 + (t * t * (-6.0 + (t * 3.0)))) / 6.0;
    }
    if (t < 2.0) {
      t = 2.0 - t;
      return t * t * t / 6.0;
    }
    return 0.0;
  }

  virtual double support() const { return 2.0; }
};


class CubicMitchellKernel : public ysKernel {
public:
  virtual const char *name() const { return "cubic"; }
  virtual const char *desc() const {
    return "Cubic, Mitchell-Netravali spline (3rd order)"; }
  virtual ysKernel *clone() const { return new CubicMitchellKernel(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t < 1.0) {
      return (16.0/3.0 + (t * (0.0 + (t * (-12.0 + (t * (7.0))))))) / 6.0;
      /*   (3)t^3 - (6)t^2 + 4    */

      /*   (1)t^3 - (2)t^2 + 1    */
      /*    3        6       3    */
      /* 2(a+2) = a+3    2a+4=a+3  a=-1 */
    }
    if (t < 2.0) {
      return (32.0/3.0 + (t * (-20.0 + (t * (12.0 + (t * -7.0/3.0)))))) / 6.0;
    }
    return 0.0;
  }

  virtual double support() const { return 2.0; }
};


class CubicCatmullKernel : public ysKernel {
public:
  virtual const char *name() const { return "cubicCR"; }
  virtual const char *desc() const {
    return "Cubic, Catmull-Rom spline (3rd order)"; }
  virtual ysKernel *clone() const { return new CubicCatmullKernel(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t < 1.0)  return (2.0 + (t * t * (-5.0 + (t * 3.0)))) / 2.0;
    if (t < 2.0)  return (4.0 + (t * (-8.0 + (t * (5.0 - t))))) / 2.0;
    return 0.0;
  }
  virtual double support() const { return 2.0; }
};


class CubicKeys4Kernel : public ysKernel {
public:
  virtual const char *name() const { return "cubicK4"; }
  virtual const char *desc() const {
    return "Keys 4th-order Cubic"; }
  virtual ysKernel *clone() const { return new CubicKeys4Kernel(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t < 1.0) 
      return (3.0 + (t * t * (-7.0 + (t * 4.0)))) / 3.0;
    if (t < 2.0) 
      return (30.0 + (t * (-59.0 + (t * (36.0 + (t * -7.0)))))) / 12.0;
    if (t < 3.0)
      return (-18.0 + (t * (21.0 + (t * (-8.0 + t))))) / 12.0;
    return 0.0;
  }
  virtual double support() const { return 3.0; }
};



/********************************************************************
 * Windowed Sinc Kernels
 ********************************************************************/

#define PI 3.1415926535897932384626433832795029L

#if 0

class Sinc4Hann : public ysKernel {
private:
  double _order;
public:
  Sinc4Hann(void) : _order(4.0) {}
  virtual const char *name() const { return "sinc4hann"; }
  virtual const char *desc() const {
    return "Experimental Nth order Sinc"; }
  virtual ysKernel *clone() const { return new Sinc4Hann(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t == 0.0) {
      return 1.0;
    } else if (t < _order) {
      double w = 0.5 + (0.5)*cos(PI*t / _order);
      return w * sin(t*PI) / (t*PI);
    } else {
      return 0.0;
    }
  }
  virtual double support() const { return 1.0 * _order; }
};

#endif

#if 0
#include <stdio.h>

class Sinc4Lanczos : public ysKernel {
private:
  double _order;
public:
  Sinc4Lanczos(void) : _order(4.0) {}
  virtual const char *name() const { return "sinc4lan"; }
  virtual const char *desc() const {
    return "Sinc with Lanczos window, 4 cycles"; }
  virtual ysKernel *clone() const { return new Sinc4Lanczos(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t == 0.0) {
      //      DBG("ZERO\n");
      return 1.0;
    } else if (t < _order) {
      double w = sin(PI*t / _order) / (PI*t / _order);
      return w * sin(t*PI) / (t*PI);
    } else {
      return 0.0;
    }
  }
  virtual double support() const { return 1.0 * _order; }
};



class Sinc8Lanczos : public ysKernel {
private:
  double _order;
public:
  Sinc8Lanczos(void) : _order(8.0) {}
  virtual const char *name() const { return "sinc8lan"; }
  virtual const char *desc() const {
    return "Sinc with Lanczos window, 8 cycles"; }
  virtual ysKernel *clone() const { return new Sinc8Lanczos(*this); }
  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t == 0.0) {
      return 1.0;
    } else if (t < _order) {
      double w = sin(PI*t / _order) / (PI*t / _order);
      return w * sin(t*PI) / (t*PI);
    } else {
      return 0.0;
    }
  }
  virtual double support() const { return 1.0 * _order; }
};

#endif


class SincLanczos : public ysKernel {
private:
  double _order;
public:
  SincLanczos(void) : _order(6.0) {}
  virtual const char *name() const { return "sinc"; }
  virtual const char *desc() const {
    return "Sinc with Lanczos window, N cycles [default: 6]"; }
  virtual ysKernel *clone() const { return new SincLanczos(*this); }

  virtual const char *pdesc() const { return ":N"; }
  virtual int pcount() const { return 1; }
  virtual int set_params(int count, double *params) {
    if (count == 1) {
      _order = params[0];
      if (_order <= 0) return 1;
    } else if (count != 0) {
      return 1;
    }
    return 0;
  }
  virtual double param(int index) {  /* just ignore index?? */
    index = index;
    return _order;
  }

  virtual double k(double t) const {
    if (t < 0.0) t = -t;
    if (t == 0.0) {
      return 1.0;
    } else if (t < _order) {
      double w = sin(PI*t / _order) / (PI*t / _order);
      return w * sin(t*PI) / (t*PI);
    } else {
      return 0.0;
    }
  }
  virtual double support() const { return 1.0 * _order; }
};



/************************************************************************
 ************************************************************************
 **** Kernel Factory:  singleton which creates instances of kernels  ****
 ****                  with given parameters                         ****
 ************************************************************************
 ************************************************************************/


//ysKernelFactory *ysKernelFactory::_instance = NULL;


const ysKernelFactory *ysKernelFactory::instance()
{
  static ysKernelFactory *me = new ysKernelFactory;
  //  if (_instance == NULL)
  //    _instance = new ysKernelFactory;
  //  return _instance;
  return me;
}


ysKernelFactory::ysKernelFactory()
{
  _kernel_list = new ysKernel *[20];
  _count = 0;
  _kernel_list[_count++] = new BoxKernel;
  _kernel_list[_count++] = new LinearKernel;
  _kernel_list[_count++] = new QuadraticKernel;
  _kernel_list[_count++] = new CubicMitchellKernel;
  _kernel_list[_count++] = new CubicCatmullKernel;
  _kernel_list[_count++] = new CubicBKernel;
  _kernel_list[_count++] = new CubicKeys4Kernel;
  //  _kernel_list[_count++] = new Sinc10Kernel;
  //  _kernel_list[_count++] = new Sinc4Hann;
  //  _kernel_list[_count++] = new Sinc4Lanczos;
  //  _kernel_list[_count++] = new Sinc8Lanczos;
  _kernel_list[_count++] = new SincLanczos;
}



static int count_delimiters(const char *s, char d)
{
  if (s == NULL) return 0;
  int count = 0;
  for ( ; *s != '\0'; s++) {
    if (*s == d) count++;
  }
  return count;
}


ysKernel *ysKernelFactory::create_kernel(const char *name) const
{
  /* parse "name:p1:p2...:pN" into name and array of parameters */
  int num = count_delimiters(name, ':');
  char *root = strdup(name);
  double *params = new double[num];
  for (int n = num - 1; n >= 0; n--) {
    char *colon = strrchr(root, ':');
    *colon = '\0';
    params[n] = atof(colon+1);
  }

  ysKernel *new_kernel = NULL;
  /* lookup */
  const ysKernel *prototype = lookup(root);
  if (prototype == NULL) goto bug_out;
  /* copy */
  new_kernel = prototype->clone();
  /* set params */
  if (new_kernel->set_params(num, params)) {
    delete new_kernel;
    new_kernel = NULL;
  }
 bug_out:
  free(root);
  delete[] params;
  return new_kernel;
}
  


const ysKernel *ysKernelFactory::lookup(const char *name) const
{
  int i;
  for (i = 0; i < _count; i++) {
    if (!strcmp(name, _kernel_list[i]->name()))
      return _kernel_list[i];
  }
  return NULL;
}


const ysKernel *ysKernelFactory::lookup(int i) const
{
  if ((i < 0) || (i >= _count))
    return NULL;
  else
    return _kernel_list[i];
}


