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

#ifndef _SCALER_MATTO_H_
#define _SCALER_MATTO_H_

#include "scaler.H"
#include "kernels.H"


#define DEFAULT_KERNEL "cubicK4"


class mattoScaler;

class mattoScalerFactory : public ysScalerFactory {
private:
  ysKernel *_the_x_kernel;
  ysKernel *_the_y_kernel;
  
  mattoScalerFactory(const mattoScalerFactory &orig); /* copy */
  mattoScalerFactory &operator=(const mattoScalerFactory &orig); /* assign */
public:
  mattoScalerFactory() :
    _the_x_kernel(ysKernelFactory::instance()->create_kernel(DEFAULT_KERNEL)),
    _the_y_kernel(ysKernelFactory::instance()->create_kernel(DEFAULT_KERNEL))
  {}
  virtual ~mattoScalerFactory() {
    delete _the_x_kernel;
    delete _the_y_kernel;
  }

  virtual const char *description() const;
  virtual void describe_options(FILE *fp, const char *prefix) const;

  virtual int parse_option(const char *s);
  virtual void describe_parameters(logging_function *logger,
				   const char *prefix) const;
  virtual ysScaler *new_scaler() const;
};



class mattoScaler : public ysScaler {
private:
  class kernelSet;

  int SframeX, SframeY;   // source frame size
  int DframeX, DframeY;   // dest frame size
  int Sxmin, Sxmax, Symin, Symax;  // source matte/sample boundary
  int Dx, Dy;             // dest window size
  int xq0, yq0;           // dest window offset
  ysRatio xp0, yp0;       // source window offset
  ysRatio sigmaX, sigmaY; // scaling ratios
  int zero_pixel;         // background/matte pixel value

  /* setup variables for scale_* */
  //XXXXX  const ysKernel *_the_kernel;  // kernel function
  ysKernel *_the_x_kernel;  // x kernel function
  ysKernel *_the_y_kernel;  // y kernel function

  kernelSet *_KX;
  kernelSet *_KY;
  int _Yminspot, _Ymaxspot;
  int _Xminspot, _Xmaxspot;
  int TframeX, TframeY;  // temporary frame size
  int *tempo;            // temporary frame data
  void (mattoScaler::*scaling_function)(uint8_t *src, uint8_t *dst);

  int Dx_pre, Dx_fill, Dx_post;
  int Sx0, Sy0, Sy1;

  static void setup_kernel_cache(double scale, double p0,
				 const ysKernel *kernel,
				 int Dsize, int Smin, int Smax, int Spitch,
				 int zero_pixel,
				 int offset_premult, int offset_offset,
				 kernelSet *&KS, int &minspot, int &maxspot);

  void setup_x_then_y();
  void scale_x_then_y(uint8_t *src, uint8_t *dst);
  void setup_y_then_x();
  void scale_y_then_x(uint8_t *src, uint8_t *dst);
  void setup_y_only();
  void scale_y_only(uint8_t *src, uint8_t *dst);
  void setup_x_only();
  void scale_x_only(uint8_t *src, uint8_t *dst);
  void setup_copy();
  void scale_copy(uint8_t *src, uint8_t *dst);
  void scale_copy_direct(uint8_t *src, uint8_t *dst);

  void scale_fill(uint8_t *src, uint8_t *dst);

  int internal_setup();
protected:
  friend class mattoScalerFactory;
  mattoScaler(ysKernel *x_kernel, ysKernel *y_kernel) :
    _the_x_kernel(x_kernel), _the_y_kernel(y_kernel),
    _KX(NULL), _KY(NULL), tempo(NULL) {}
  mattoScaler(const mattoScaler &k);            /* copy   */
  mattoScaler &operator=(const mattoScaler &v); /* assign */

public:
  virtual ~mattoScaler();
  virtual int setup(const ysPoint &source_size,
		    const ysRatioPoint &source_offset,
		    const ysRegion &source_matte,
		    const ysPoint &dest_size,
		    const ysRegion &dest_region,
		    const ysRatio &x_scale, const ysRatio &y_scale,
		    uint8_t matte_pixel);
  virtual int scale(uint8_t *source, uint8_t *dest);

};


#endif /* _SCALER_MATTO_H_ */

