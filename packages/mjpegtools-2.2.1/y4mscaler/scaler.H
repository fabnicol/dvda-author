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

#ifndef _YS_SCALER_H_
#define _YS_SCALER_H_


#include "graphics.H"

/*
 * ysScaler is a generic interface to a scaling engine.
 * ysScalerFactory is a generic interface to a factory class which creates
 *  instances of the scaling engine.
 *
 * ysScaler has two public methods, setup() and scale() (as well as a virtual
 *  destructor).
 *
 * setup() initializes/sets-up the engine to perform a specific scaling
 *  operation.
 *
 *     source_size:  XxY size of the whole source frame
 *   source_offset:  (x,y) position of the upper-left corner of
 *                    the source region
 *       dest_size:  XxY size of the target frame
 *     dest_region:  XxY size and (x,y) position of the target region
 *                    (within the target frame)
 *         x_scale:  X scale factor (target/source)
 *         y_scale:  Y scale factor (target/source)
 *     matte_pixel:  background value used for pixels outside of the source
 *                    region
 *
 *   setup() returns 0 on success, non-zero on failure.
 *   Failure may occur is the engine cannot provide the requested scaling.
 *
 *
 * scale() actually performs the scaling, given source and target buffers.
 *  Pixels in the target frame which lie outside of the target region
 *  are left alone.
 *
 *     source:  raster buffer of source pixels (row-major ordering)
 *     target:  raster buffer of target pixels (row-major ordering)
 *
 */

class ysScaler {
public:
  virtual ~ysScaler() {}
  virtual int setup(const ysPoint &source_size,
		    const ysRatioPoint &source_offset,
		    const ysRegion &source_matte,
		    const ysPoint &dest_size,
		    const ysRegion &dest_region,
		    const ysRatio &x_scale, const ysRatio &y_scale,
		    uint8_t matte_pixel) = 0;
  virtual int scale(uint8_t *source, uint8_t *dest) = 0;
};



class ysScalerFactory {
public:
  virtual ~ysScalerFactory() {}
  virtual const char *description() const = 0;
  virtual void describe_options(FILE *fp, const char *prefix) const = 0;

  virtual int parse_option(const char *s) = 0;
  typedef void (logging_function)(const char format[], ...);
  virtual void describe_parameters(logging_function *logger,
				   const char *prefix) const = 0;
  virtual ysScaler *new_scaler() const = 0;
};




#endif /* _YS_SCALER_H_ */
