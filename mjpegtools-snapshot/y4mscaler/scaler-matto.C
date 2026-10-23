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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "scaler-matto.H"

#define FSHIFT 10
//#define FSHIFT 20
#define FUNIT  (1<<FSHIFT)
#define FHALF  (1<<(FSHIFT-1))


#define CLIP_PIXEL(dst, src, min_pixel, max_pixel)        \
  {                                                       \
    if ( (src) < (min_pixel) ) (dst) = (min_pixel);       \
    else if ( (src) > (max_pixel) ) (dst) = (max_pixel);  \
    else (dst) = (src);                                   \
  }

#define MIN_PIXEL 1
#define MAX_PIXEL 254



//===========================================================================
//===========================================================================
//==== scaler factory:  parses options and generates an instance of      ====
//====                  the engine                                       ====
//===========================================================================
//===========================================================================

const char *mattoScalerFactory::description() const
{
  return "Matto's Generic Scaler";
}

void mattoScalerFactory::describe_options(FILE *fp,
					  const char *prefix) const
{
  fprintf(fp, "%sOptions specify scaling kernels/modes:\n", prefix);
  fprintf(fp, "%s   KKK     - use 'KKK' kernel for x and/or y scaling\n",
	  prefix);
  fprintf(fp, "%s   XXX,YYY - use 'XXX' for x scaling, 'YYY' for y scaling\n",
	  prefix);
  fprintf(fp, "%sSome kernels take a numeric parameter:  'KKK:N'\n",
	  prefix);
  fprintf(fp, "%sKernel list:\n", prefix);

  int maxlen = 0;
  for (int i = 0; i < ysKernelFactory::instance()->count(); i++) {
    int x = 
      strlen(ysKernelFactory::instance()->lookup(i)->name()) +
      strlen(ysKernelFactory::instance()->lookup(i)->pdesc());
    if (x > maxlen) maxlen = x;
  }
  for (int i = 0; i < ysKernelFactory::instance()->count(); i++) {
    const char *name = ysKernelFactory::instance()->lookup(i)->name();
    const char *pdesc = ysKernelFactory::instance()->lookup(i)->pdesc();
    fprintf(fp, "%s   %s%s%*s - %s\n", prefix,
	    name, pdesc,
	    maxlen - (int)strlen(name) - (int)strlen(pdesc), "",
	    ysKernelFactory::instance()->lookup(i)->desc());

    int x = strlen(ysKernelFactory::instance()->lookup(i)->name());
    if (x > maxlen) maxlen = x;
  }
  fprintf(fp, "%sThe default kernel is '%s'.\n", prefix, DEFAULT_KERNEL);
}


void mattoScalerFactory::describe_parameters(logging_function *logger,
					     const char *prefix) const
{
  char sout[256];
  char *s = sout;
  int rem = sizeof(sout);
  int len;
#define CHECK_LENGTH  rem -= len; s += len; if (rem <= 0) goto done;

  len = snprintf(s, rem, "%skernels:  ", prefix);
  CHECK_LENGTH;
  if (_the_x_kernel == NULL) {
    len = snprintf(s, rem, "???");
    CHECK_LENGTH;
  } else {
    len = snprintf(s, rem, "%s", _the_x_kernel->name());
    CHECK_LENGTH;
    for (int i = 0; i < _the_x_kernel->pcount(); i++) {
      len = snprintf(s, rem, ":%1g", _the_x_kernel->param(i));
      CHECK_LENGTH;
    }
  }
  if (_the_y_kernel == NULL) {
    len = snprintf(s, rem, ", ???");
    CHECK_LENGTH;
  } else {
    len = snprintf(s, rem, ", %s", _the_y_kernel->name());
    CHECK_LENGTH;
    for (int i = 0; i < _the_y_kernel->pcount(); i++) {
      len = snprintf(s, rem, ":%1g", _the_y_kernel->param(i));
      CHECK_LENGTH;
    }
  }

 done:
  (*logger)("%s", sout);
}


int mattoScalerFactory::parse_option(const char *option)
{
  /* look for comma; split string */
  char *xname = strdup(option);
  char *yname = strchr(xname, ',');
  if (yname != NULL) {
    *yname = '\0';
    yname++;
  } else {
    yname = xname;
  }
  /* create our own prototypes, with provided parameters, etc. */
  ysKernel *new_x_kernel = ysKernelFactory::instance()->create_kernel(xname);
  ysKernel *new_y_kernel = ysKernelFactory::instance()->create_kernel(yname);
  free(xname);
  if ((new_x_kernel == NULL) || (new_y_kernel == NULL)) {
    delete new_x_kernel;
    delete new_y_kernel;
    return 1;
  }
  /* clean out previous kernels (if any) */
  delete _the_x_kernel;
  delete _the_y_kernel;
  _the_x_kernel = new_x_kernel;
  _the_y_kernel = new_y_kernel;
  return 0;
}



ysScaler *mattoScalerFactory::new_scaler() const
{
  /* assume _the_*_kernel's have valid, non-NULL values... */
  /*  (i.e. defaults assigned to/by factory earlier)       */
  return new mattoScaler(_the_x_kernel->clone(), _the_y_kernel->clone());
}


//=========================================================================
//=========================================================================
//==== scaler - the actual engine that does scaling on an image plane  ====
//=========================================================================
//=========================================================================


int mattoScaler::setup(const ysPoint &source_size,
		    const ysRatioPoint &source_offset,
		    const ysRegion &source_matte,
		    const ysPoint &dest_size,
		    const ysRegion &dest_region,
		    const ysRatio &x_scale, const ysRatio &y_scale,
		    uint8_t matte_pixel)
{
  SframeX = source_size.x();
  SframeY = source_size.y();
  DframeX = dest_size.x();
  DframeY = dest_size.y();

  DBG("S (%d, %d)  D (%d, %d)\n",
	  SframeX, SframeY, DframeX, DframeY);
  DBG("scale:  (%f, %f)\n", 
	  x_scale.to_double(), y_scale.to_double());

  Dx = dest_region.dim().x();
  Dy = dest_region.dim().y();
  xq0 = dest_region.offset().x();
  yq0 = dest_region.offset().y();

  Sxmin = source_matte.offset().x();
  Symin = source_matte.offset().y();
  Sxmax = Sxmin + source_matte.dim().x() - 1;
  Symax = Symin + source_matte.dim().y() - 1;
  if (Sxmin < 0) Sxmin = 0;
  if (Sxmax >= SframeX) Sxmax = SframeX - 1;
  if (Symin < 0) Symin = 0;
  if (Symax >= SframeY) Symax = SframeY - 1;

  // DBG("Sx (%d %d)  Sy (%d %d)\n", Sxmin, Sxmax, Symin, Symax);

  xp0 = source_offset.x();
  yp0 = source_offset.y();

  sigmaX = x_scale;
  sigmaY = y_scale;

  zero_pixel = matte_pixel;

  return internal_setup();
}



int mattoScaler::scale(uint8_t *source, uint8_t *dest)
{
  (this->*(scaling_function))(source, dest);
  return 0;
}




//=========================================================================
//=========================================================================
//=========================================================================


int mattoScaler::internal_setup() 
{
  /* Pick order of x and y scaling to minimize computation:
    
    Dx / sigmaX = sourceX
    Dy / sigmaY = sourceY

    x_then_y:  SX*SY -> DX*SY -> DX*DY
    y_then_x:  SX*SY -> SX*DY -> DX*DY
    
      DX*SY = DX * DY / sigY  = DX*DY / sigY
      SX*DY = DX / sigX * DY  = DX*DY / sigX

    SX*DY < DX*SY --> y_then_x else x_then_y
    ==>
    sigX > sigY --> y_then_x else x_then_y
  */

  if ((sigmaX == 1) && (xp0.is_integral())) {
    if ((sigmaY == 1) && (yp0.is_integral())) {
      DBG("SCALER MODE:  copy\n");
      setup_copy();
    } else {
      DBG("SCALER MODE:  y only\n");
      //      setup_x_then_y();
      setup_y_only();
    }
  } else if ((sigmaY == 1) && (yp0.is_integral())) {
    DBG("SCALER MODE:  x only\n");
    //setup_x_then_y();
    setup_x_only();
  } else if (sigmaX > sigmaY) {
    DBG("SCALER MODE:  y then x\n");
    setup_y_then_x();
  } else {
    DBG("SCALER MODE:  x then y\n");
    setup_x_then_y();
    //setup_y_then_x();
  }
#if 0
  DBG("setup scaler:\n");
  DBG("src frame:  %dx%d    dst frame:  %dx%d\n",
	  SframeX, SframeY, DframeX, DframeY);
  DBG("map  xdxxd (+%f+%f)  to  %dx%d (+%d+%d)\n",
  	  /*Sx,Sy,*/xp0.to_double(),yp0.to_double(), Dx,Dy,xq0,yq0);
  DBG("x-ratio:  %d/%d   y-ratio:  %d/%d\n",
	  sigmaX.ratio().n, sigmaX.ratio().d,
	  sigmaY.ratio().n, sigmaY.ratio().d);
#endif
  return 0;
}




class mattoScaler::kernelSet {
public:
  int *K;
  int width;
  int spot0;
  int offset;

  kernelSet() : K(NULL) {}
  ~kernelSet() { delete[](K); }
protected:
  kernelSet(const kernelSet &k);
  kernelSet &operator=(const kernelSet &v);
};



/*
  translate a discrete (pixel) destination coordinate back into
   the corresponding continuous source coordinate

   q - dest coord (pixel)
   scale - src->dst scale factor
   p0 - source offset (pixel)
*/

static double push_back(int q, double scale, double p0)
{
  return ((double)(q) + 0.5) / scale - 0.5 + p0;
}



void mattoScaler::setup_kernel_cache(double scale, double p0,
				     const ysKernel *kernel,
				     int Dsize, int Smin, int Smax, int Spitch,
				     int zero_pixel,
				     int offset_premult, int offset_offset,
				     kernelSet *&KS,
				     int &minspot, int &maxspot)
{

  DBG("setup kernel cache\n");
  DBG("  scale %f  p0 %f\n", scale, p0);

  KS = new mattoScaler::kernelSet[Dsize];
  minspot = Smax + 1;
  maxspot = Smin - 1;

  double kscale = scale;
  if (kscale > 1.0) kscale = 1.0;
  int supp = (int)ceil(kernel->support() / kscale);
  DBG("  support:  %d\n", supp);

  for (int q = 0; q < Dsize; q++) {
    double Pq = push_back(q, scale, p0);

    int spot0 = (int)floor(Pq) - supp;
    int spot1 = (int)floor(Pq) + supp + 1;
    KS[q].K = new int[spot1 - spot0 + 1];
    KS[q].offset = 0;
    KS[q].width = 0;
    int valsum = 0;
    int valmax = 0;
    int maxpos = -1;
    int m, s;
    for (m = 0, s = spot0; s <= spot1; s++) {
#if 0
      int value = 
	(int)( (kernel->k((Pq - s) * kscale) * kscale * (double)FUNIT) + 0.5 );
#else
      double fvalue = kernel->k((Pq - s) * kscale) * kscale * (double)FUNIT;
      int value = (fvalue >= 0.0) ? (int)(fvalue + 0.5)	: (int)(fvalue - 0.5);
#endif

#ifdef _YS_DEBUG
      if (q == 10) {
	fprintf(stderr, "  %2d:  %3d  %4d  %f\n", m, s, value, fvalue / (double)FUNIT);
      }
#endif

      if ((m == 0) && (value == 0)) {
	/* remove zero entries from head of list */
	spot0++;
      } else if ((s < Smin) || (s > Smax)) {
	/* remove out-of-region entries from head and tail of list,
	   and add to offset instead */
	KS[q].offset += value; // * zero_pixel;
	if (m == 0) spot0++;
	if (value > valmax) {
	  valmax = value;
	  maxpos = -1;
	}
      } else {
	if (value > valmax) {
	  valmax = value;
	  maxpos = m;
	}
	KS[q].K[m++] = value;
	KS[q].width++;
      }
      
      valsum += value;
    }
    /* remove zero entries from tail of list */
    while (--m >= 0) {
      if (KS[q].K[m] != 0) break;
      KS[q].width--;
    }
    
    if (valsum != FUNIT) {
      DBG("valsum over:  %d at %d\n", valsum, q);
      if (maxpos == -1) 
	KS[q].offset -= (valsum - FUNIT);
      else 
	KS[q].K[maxpos] -= (valsum - FUNIT);
      // Q:  what about symmetric, even (two peak) lists?
    }
#if 0
    if (KS[q].width == 0) {
      DBG("zero width:  [%d]  off %d  %f\n",
	      q, KS[q].offset, (double)(KS[q].offset)/(double)FUNIT);
    }
#endif
    KS[q].offset *= zero_pixel;
    KS[q].offset *= offset_premult;
    KS[q].offset += offset_offset;
    KS[q].spot0 = spot0 * Spitch;

    if (KS[q].width > 0) {
      /* i.e. if spot0 and spot1 actually mean anything */
      if (spot0 < minspot) minspot = spot0;
      int real_spot1 = spot0 + KS[q].width - 1;
      if (real_spot1 > maxspot) maxspot = real_spot1;
    }

  }
  //  DBG("minspot: %d   maxspot: %d\n", minspot, maxspot);
}




mattoScaler::~mattoScaler()
{
  delete _the_x_kernel;
  delete _the_y_kernel;
  delete[](_KX);
  delete[](_KY);
  delete[](tempo);
}






//=========================================================================
//=========================================================================
//=========================================================================


void mattoScaler::setup_x_then_y()
{
  setup_kernel_cache(sigmaX.to_double(), xp0.to_double(), _the_x_kernel,
		     Dx, Sxmin, Sxmax, 1,
		     zero_pixel,
		     1, 0,
		     _KX, _Xminspot, _Xmaxspot);

  setup_kernel_cache(sigmaY.to_double(), yp0.to_double(), _the_y_kernel,
		     Dy, Symin, Symax, 1,
		     zero_pixel,
		     FUNIT, (1<<(2*FSHIFT - 1)), //(not) 2*FHALF,
		     _KY, _Yminspot, _Ymaxspot);

  for (int q = 0; q < Dy; q++) {
    _KY[q].spot0 -= _Yminspot;
    _KY[q].spot0 *= Dx; /*TframeX;*/
  }

  //  DBG("Y spot limits:  %d -- %d\n", _Yminspot, _Ymaxspot);

  if ((_Xminspot > _Xmaxspot) || (_Yminspot > _Ymaxspot)) {
    scaling_function = &mattoScaler::scale_fill;
  } else {
    TframeX = Dx;
    TframeY = _Ymaxspot - _Yminspot + 1;
    tempo = new int[TframeX * TframeY];
    scaling_function = &mattoScaler::scale_x_then_y;
  }

  //  DBG("caches set up\n");
}
    


void mattoScaler::scale_x_then_y(uint8_t *src, uint8_t *dst)
{
  /* scale x direction, src into tempo */
  int *Tptr = tempo;
  uint8_t *srcline = src + (_Yminspot * SframeX);

  for (int y = _Yminspot; y <= _Ymaxspot; y++) {
    //uint8_t *srcline = src + y*SframeX;
    for (int xq = 0; xq < Dx; xq++) {
      int sum = _KX[xq].offset; /*FHALF;*/
      int *KXptr = _KX[xq].K;
      uint8_t *srcspot = srcline + _KX[xq].spot0;
      for (int s = 0; s < _KX[xq].width; s++) {
	sum += (*KXptr) * (*srcspot);
	srcspot++;
	KXptr++;
      }
      *Tptr = sum;
      Tptr++;
    }
    srcline += SframeX;
  }

  /* scale y direction, tempo into dst */
  for (int x = 0; x < Dx; x++) {
    uint8_t *Dptr = (dst + xq0 + (yq0 * DframeX)) + x;
    int *tempcol = tempo + x;
    for (int yq = 0; yq < Dy; yq++) {
      ///////      int sum = (_KY[yq].offset * FUNIT) + (2*FHALF);
      int sum = _KY[yq].offset;
      ///////      int *Tptr = tempcol + (_KY[yq].spot0 * TframeX);
      int *Tptr = tempcol + _KY[yq].spot0;
      int *KYptr = _KY[yq].K;
      for (int s = 0; s < _KY[yq].width; s++) {
	sum +=  (*KYptr) * (*Tptr);
	Tptr += TframeX;
	KYptr++;
      }
      /*      *Dptr = sum >> (2*FSHIFT);*/
      sum >>= (2*FSHIFT);
      CLIP_PIXEL(*Dptr, sum, MIN_PIXEL, MAX_PIXEL);
      Dptr += DframeX;
    }
  }

}









//=========================================================================
//=========================================================================
//=========================================================================


void mattoScaler::setup_y_then_x()
{
  setup_kernel_cache(sigmaY.to_double(), yp0.to_double(), _the_y_kernel,
		     Dy, Symin, Symax, SframeX,
		     zero_pixel,
		     1, 0,
		     _KY, _Yminspot, _Ymaxspot);
  setup_kernel_cache(sigmaX.to_double(), xp0.to_double(), _the_x_kernel,
		     Dx, Sxmin, Sxmax, 1,
		     zero_pixel,
		     FUNIT, (1<<(2*FSHIFT - 1)), //(not) 2*FHALF,
		     _KX, _Xminspot, _Xmaxspot);

  for (int q = 0; q < Dx; q++) {
    _KX[q].spot0 -= _Xminspot;
  }


  //  DBG("X spot limits:  %d -- %d\n", _Xminspot, _Xmaxspot);
  ////  DBG("X kernel max width:  %d\n", xmaxwidth);

  //  DBG("caches set up\n");
  if ((_Xminspot > _Xmaxspot) || (_Yminspot > _Ymaxspot)) {
    scaling_function = &mattoScaler::scale_fill;
  } else {
    TframeY = Dy;
    TframeX = _Xmaxspot - _Xminspot + 1;
    tempo = new int[TframeY * TframeX];
    scaling_function = &mattoScaler::scale_y_then_x;
  }
}
    


void mattoScaler::scale_y_then_x(uint8_t *src, uint8_t *dst)
{
  /* scale y direction, src into tempo */
  int *tempcol = tempo;
  uint8_t *srccol = src + _Xminspot;

  for (int x = _Xminspot; x <= _Xmaxspot; x++) {
    int *Tptr = tempcol;
    for (int yq = 0; yq < Dy; yq++) {
      int sum = _KY[yq].offset;
      int *KYptr = _KY[yq].K;
      uint8_t *srcspot = srccol + _KY[yq].spot0;
      for (int s = 0; s < _KY[yq].width; s++) {
	sum += (*KYptr) * (*srcspot);
	srcspot += SframeX;
	KYptr++;
      }
      *Tptr = sum;
      Tptr += TframeX;
    }
    srccol++;
    tempcol++;
  }


  /* scale x direction, tempo into dst */
  for (int y = 0; y < Dy; y++) {
    uint8_t *Dptr = (dst + xq0 + ((yq0 + y) * DframeX));
    int *templine = tempo + (y * TframeX);
    for (int xq = 0; xq < Dx; xq++) {
      int sum = _KX[xq].offset;
      int *Tptr = templine + _KX[xq].spot0;
      int *KXptr = _KX[xq].K;
      for (int s = 0; s < _KX[xq].width; s++) {
	sum +=  (*KXptr) * (*Tptr);
	Tptr++;
	KXptr++;
      }
      /*      *Dptr = sum >> (2*FSHIFT);*/
      sum >>= (2*FSHIFT);
      CLIP_PIXEL(*Dptr, sum, MIN_PIXEL, MAX_PIXEL);
      Dptr++;
    }
  }

}



//=========================================================================
//=========================================================================
//=========================================================================


void mattoScaler::setup_copy()
{
  Sx0 = xp0.to_int();
  Sy0 = yp0.to_int();

  int Sx1 = Sx0 + Dx - 1;
  Sy1 = Sy0 + Dy - 1;

  DBG("SC: sx %d-%d sy %d-%d  Dx %d  Dy %d\n",
	  Sx0, Sx1, Sy0, Sy1, Dx, Dy);

  if (Sx0 < Sxmin)
    Dx_pre = Sxmin - Sx0;
  else
    Dx_pre = 0;
  if (Sx1 > Sxmax)
    Dx_post = Sx1 - Sxmax;
  else
    Dx_post = 0;
  Dx_fill = Dx - Dx_pre - Dx_post;

  if ((Sx0 > Sxmax) || (Sx1 < Sxmin) ||
      (Sy0 > Symax) || (Sy1 < Symin)) 
    scaling_function = &mattoScaler::scale_fill;
  else if ((Sx0 >= Sxmin) && (Sx1 <= Sxmax) &&
	   (Sy0 >= Symin) && (Sy1 <= Symax))
    scaling_function = &mattoScaler::scale_copy_direct;
  else
    scaling_function = &mattoScaler::scale_copy;
}


void mattoScaler::scale_copy(uint8_t *src, uint8_t *dst)
{
  uint8_t *srcspot = src + (Sy0 * SframeX) + Sx0 + Dx_pre;
  uint8_t *dstspot = dst + (yq0 * DframeX) + xq0;

  int y;
  /* above Symin:  matte color */
  for (y = Sy0; (y < Symin) && (y <= Sy1); y++) {
    memset(dstspot, zero_pixel, Dx);
    srcspot += SframeX;
    dstspot += DframeX;
  }
  /* within [Symin,Symax]:  copy */
  for ( ; (y <= Symax) && (y <= Sy1); y++) {
    memset(dstspot, zero_pixel, Dx_pre);
    memcpy(dstspot + Dx_pre, srcspot, Dx_fill);
    memset(dstspot + Dx_pre + Dx_fill, zero_pixel, Dx_post);
    srcspot += SframeX;
    dstspot += DframeX;
  }
  /* below Symax:  matte color */
  for ( ; y <= Sy1; y++) {
    memset(dstspot, zero_pixel, Dx);
    srcspot += SframeX;
    dstspot += DframeX;
  }
}


void mattoScaler::scale_copy_direct(uint8_t *src, uint8_t *dst)
{
  uint8_t *srcspot = src + (Sy0 * SframeX) + Sx0;
  uint8_t *dstspot = dst + (yq0 * DframeX) + xq0;

  for (int y = 0; y < Dy; y++) {
    memcpy(dstspot, srcspot, Dx);
    srcspot += SframeX;
    dstspot += DframeX;
  }
}


void mattoScaler::scale_fill(uint8_t *src, uint8_t *dst)
{
  src = src; /* unused parameter */
  uint8_t *dstspot = dst + (yq0 * DframeX) + xq0;

  DBG("SCALE FILL\n");
  for (int y = 0; y < Dy; y++) {
    memset(dstspot, zero_pixel, Dx);
    dstspot += DframeX;
  }
}



//=========================================================================
//=========================================================================
//=========================================================================


void mattoScaler::setup_x_only()
{
  setup_kernel_cache(sigmaX.to_double(), xp0.to_double(), _the_x_kernel,
		     Dx, Sxmin, Sxmax, 1,
		     zero_pixel,
		     1, FHALF,
		     _KX, _Xminspot, _Xmaxspot);

#ifdef _YS_DEBUG
  {
    int x;
    for (x = 0; x < Dx; x++) {
      fprintf(stderr, "%3d:  %2d  %2d  %3d\n",
	      x, _KX[x].width, _KX[x].spot0, _KX[x].offset);
      int s = 0;
      for (int i = 0; i < _KX[x].width; i++) {
	fprintf(stderr, "%d ", _KX[x].K[i]);
	s += _KX[x].K[i];
      }
      fprintf(stderr, "  = %d\n", s);
    }
  }
#endif
  Sy0 = yp0.to_int();
  Sy1 = Sy0 + Dy - 1;

  //  DBG("caches set up\n");
  if ((_Xminspot > _Xmaxspot) || (Sy0 > Symax) || (Sy1 < Symin)) {
    scaling_function = &mattoScaler::scale_fill;
  } else {
    scaling_function = &mattoScaler::scale_x_only;
  }
}
    


void mattoScaler::scale_x_only(uint8_t *src, uint8_t *dst)
{
  /* scale x direction, src into tempo */
  uint8_t *srcline = src + (Sy0 * SframeX);
  uint8_t *dstline = dst + (yq0 * DframeX) + xq0;

  int y;
  for (y = Sy0; (y < Symin) && (y <= Sy1); y++) {
    memset(dstline, zero_pixel, Dx);
    srcline += SframeX;
    dstline += DframeX;
  }

  for ( ; (y <= Symax) && (y <= Sy1); y++) {
    //uint8_t *srcline = src + y*SframeX;
    uint8_t *dstspot = dstline;
    for (int xq = 0; xq < Dx; xq++) {
      int sum = _KX[xq].offset; /*FHALF;*/
      int *KXptr = _KX[xq].K;
      uint8_t *srcspot = srcline + _KX[xq].spot0;
      for (int s = 0; s < _KX[xq].width; s++) {
	//for (int s = _KX[xq].width; s != 0; s--) {
	sum += (*KXptr) * (*srcspot);
	srcspot++;
	KXptr++;
      }
#if 0
      {
	uint8_t *dmax = dst + (DframeX * DframeY);
	if ((dstspot < dst) || (dstspot >= dmax))
	  DBG("%p (%p - %p) : xq %d  y %d \n",
		  dstspot, dst, dst + (DframeX * DframeY),
		  xq, y);
      }
#endif

#if 0
      *dstspot = sum >> FSHIFT;
#elif 0
      /* EXPERIMENT:  avoid under/over-flow when casting */
      sum >>= FSHIFT;
      sum = (sum < 0) ? 0 : sum;
      sum = (sum > 255) ? 255 : sum;
      *dstspot = sum;
#elif 0
      /* EXPERIMENT:  avoid under/over-flow when casting */
      sum >>= FSHIFT;
      if (sum < 0) sum = 0;
      if (sum > 255) sum = 255;
      *dstspot = sum;
#elif 0
      /* EXPERIMENT:  avoid under/over-flow when casting */
      sum >>= FSHIFT;
      if (sum < 0) 
	*dstspot = 0;
      else if (sum > 255) 
	*dstspot = 255;
      else
	*dstspot = sum;
#elif 0
      /* EXPERIMENT:  avoid under/over-flow when casting */
      {
	int val = sum >> FSHIFT;
	if (val < 0) 
	  *dstspot = 0;
	else if (val > 255) 
	  *dstspot = 255;
	else
	  *dstspot = val;
      }
#else
      /* EXPERIMENT:  avoid under/over-flow when casting */
      sum >>= FSHIFT;
      CLIP_PIXEL(*dstspot, sum, MIN_PIXEL, MAX_PIXEL);
#endif
      dstspot++;
    }
    srcline += SframeX;
    dstline += DframeX;
  }

  for ( ; y <= Sy1; y++) {
    memset(dstline, zero_pixel, Dx);
    srcline += SframeX;
    dstline += DframeX;
  }
}


//=========================================================================
//=========================================================================
//=========================================================================




void mattoScaler::setup_y_only()
{
  setup_kernel_cache(sigmaY.to_double(), yp0.to_double(), _the_y_kernel,
		     Dy, Symin, Symax, SframeX,
		     zero_pixel,
		     1, FHALF,
		     _KY, _Yminspot, _Ymaxspot);

  Sx0 = xp0.to_int();
  int Sx1 = Sx0 + Dx - 1;
  
  if (Sx0 < Sxmin)
    Dx_pre = Sxmin - Sx0;
  else
    Dx_pre = 0;
  if (Sx1 > Sxmax)
    Dx_post = Sx1 - Sxmax;
  else
    Dx_post = 0;
  Dx_fill = Dx - Dx_pre - Dx_post;

  if ((_Yminspot > _Ymaxspot) || (Sx0 > Sxmax) || (Sx1 < Sxmin)) {
    scaling_function = &mattoScaler::scale_fill;
  } else {
    scaling_function = &mattoScaler::scale_y_only;
  }
}
    


void mattoScaler::scale_y_only(uint8_t *src, uint8_t *dst)
{
  /* scale y direction, src into dst */
  //  uint8_t *srccol = src + xp0.to_int();
  uint8_t *srccol = src + Sx0 + Dx_pre;
  uint8_t *dstcol;

  /* right side, outside of src matte */
  dstcol = dst + (yq0 * DframeX) + xq0;
  for (int yq = 0; yq < Dy; yq++) {
    memset(dstcol, zero_pixel, Dx_pre);
    dstcol += DframeX;
  }

  /* middle, inside src matte */
  dstcol = dst + (yq0 * DframeX) + xq0 + Dx_pre;
  for (int x = 0; x < Dx_fill; x++) {
    //uint8_t *srcline = src + y*SframeX;
    uint8_t *dstspot = dstcol;
    for (int yq = 0; yq < Dy; yq++) {
      int sum = _KY[yq].offset; /*FHALF;*/
      int *KYptr = _KY[yq].K;
      uint8_t *srcspot = srccol + _KY[yq].spot0;
      for (int s = 0; s < _KY[yq].width; s++) {
	sum += (*KYptr) * (*srcspot);
	srcspot += SframeX;
	KYptr++;
      }
      /*      *dstspot = sum >> FSHIFT;*/
      sum >>= FSHIFT;
      CLIP_PIXEL(*dstspot, sum, MIN_PIXEL, MAX_PIXEL);
      dstspot += DframeX;
    }
    srccol++;
    dstcol++;
  }

  /* left side, outside of src matte */
  dstcol = dst + (yq0 * DframeX) + xq0 + Dx_pre + Dx_fill;
  for (int yq = 0; yq < Dy; yq++) {
    memset(dstcol, zero_pixel, Dx_post);
    dstcol += DframeX;
  }
}



