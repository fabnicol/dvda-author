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

#ifndef _YS_KERNELS_H_
#define _YS_KERNELS_H_


class ysKernel {
public:
  virtual ~ysKernel() {}
  virtual double k(double t) const = 0;
  virtual double support() const = 0;
  virtual const char *name() const = 0;
  virtual const char *desc() const = 0;

  virtual ysKernel *clone() const = 0;

  /* default behavior:  accept no parameters */
  virtual const char *pdesc() const { return ""; }
  virtual int pcount() const { return 0; }
  virtual int set_params(int count, double *params) {
    params = params;
    return (count != 0); }
  virtual double param(int index) {
    index = index;
    return 0.0;
  }
};




class ysKernelFactory {
private:
  ysKernel **_kernel_list;
  int _count;
  //  static ysKernelFactory *_instance;
protected:
  ysKernelFactory();  /* "There can be only one!" */
  ~ysKernelFactory();
public:
  static const ysKernelFactory *instance();
  const ysKernel *lookup(const char *name) const;
  const ysKernel *lookup(int i) const;
  int count() const { return _count; }

  /* take name of form "name:p1:p2...:pN" and create instance of
     kernel "name" with the given parameters */
  ysKernel *create_kernel(const char *name) const;
};


#endif /* _YS_KERNELS_H_ */





