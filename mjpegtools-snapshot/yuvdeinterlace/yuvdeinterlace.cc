/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <cstring>
#include <cstdlib>
#include "mjpeg_types.h"
#include "yuv4mpeg.h"
#include "mjpeg_logging.h"
#include "cpu_accel.h"
#include "motionsearch.h"

#ifdef __GNUC__
#define RESTRICT __restrict__
#else
#define RESTRICT
#endif

namespace
{

class y4mstream
{
public:
  int fd_in;
  int fd_out;
  y4m_frame_info_t iframeinfo;
  y4m_stream_info_t istreaminfo;
  y4m_frame_info_t oframeinfo;
  y4m_stream_info_t ostreaminfo;

    y4mstream ()
  {
    fd_in = 0;
    fd_out = 1;
  };

};

class deinterlacer
{
public:
  int width;
  int height;
  int cwidth;
  int cheight;
  int field_order;
  int both_fields;
  int verbose;
  int input_chroma_subsampling;
  int output_chroma_subsampling;
  int vertical_overshot_luma;
  int vertical_overshot_chroma;
  int just_anti_alias;

  y4mstream Y4MStream;

  uint8_t *inframe[3];
  uint8_t *inframe0[3];
  uint8_t *inframe1[3];

  uint8_t *outframe[3];
  uint8_t * RESTRICT scratch;
  uint8_t * RESTRICT mmap;

  int_least16_t (* RESTRICT motion[2])[2];

  void initialize_memory (int w, int h, int cw, int ch)
  {
    int luma_size;
    int chroma_size;
    int lmotion_size;
    int cmotion_size;

    // Some functions need some vertical overshoot area
    // above and below the image. So we make the buffer
    // a little bigger...
      vertical_overshot_luma = 32 * w;
      vertical_overshot_chroma = 32 * cw;
      luma_size = (w * h) + 2 * vertical_overshot_luma;
      chroma_size = (cw * ch) + 2 * vertical_overshot_chroma;
      lmotion_size = ((w + 7) / 8) * ((h + 7) / 8);
      cmotion_size = ((cw + 7) / 8) * ((ch + 7) / 8);

      inframe[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      inframe[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      inframe[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      inframe0[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      inframe0[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      inframe0[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      inframe1[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      inframe1[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      inframe1[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      outframe[0] = (uint8_t *) calloc (luma_size, 1) + vertical_overshot_luma;
      outframe[1] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;
      outframe[2] = (uint8_t *) calloc (chroma_size, 1) + vertical_overshot_chroma;

      scratch = (uint8_t *) malloc (luma_size) + vertical_overshot_luma;
      mmap = (uint8_t *) malloc (w * h);

      motion[0] = (int_least16_t (*)[2]) calloc (lmotion_size, sizeof(motion[0][0]));
      motion[1] = (int_least16_t (*)[2]) calloc (cmotion_size, sizeof(motion[0][0]));

      width = w;
      height = h;
      cwidth = cw;
      cheight = ch;
  }

  deinterlacer ()
  {
    both_fields = 0;
    just_anti_alias = 0;
  }

  ~deinterlacer ()
  {
    free (inframe[0] - vertical_overshot_luma);
    free (inframe[1] - vertical_overshot_chroma);
    free (inframe[2] - vertical_overshot_chroma);

    free (inframe0[0] - vertical_overshot_luma);
    free (inframe0[1] - vertical_overshot_chroma);
    free (inframe0[2] - vertical_overshot_chroma);

    free (inframe1[0] - vertical_overshot_luma);
    free (inframe1[1] - vertical_overshot_chroma);
    free (inframe1[2] - vertical_overshot_chroma);

    free (outframe[0] - vertical_overshot_luma);
    free (outframe[1] - vertical_overshot_chroma);
    free (outframe[2] - vertical_overshot_chroma);

    free (scratch - vertical_overshot_luma);
    free (mmap);

    free (motion[0]);
    free (motion[1]);
    
  }

  void temporal_reconstruct_frame (uint8_t * RESTRICT out, const uint8_t * const in, uint8_t * RESTRICT in0, const uint8_t * const in1, int w, int h, int field, int_least16_t (* RESTRICT lvxy)[2])
  {
    int_fast16_t x, y;
    int_fast16_t vx, vy, dx, dy, px, py;
    uint_fast16_t min, sad;
    int_fast16_t a, b, c, d, e, f, g, m, i;
    const uint_fast16_t iw = (w + 7) / 8;

// the ELA-algorithm overshots by one line above and below the
// frame-size, so fill the ELA-overshot-area in the inframe to
// ensure that no green or purple lines are generated...
    std::memcpy (in0 - w, in + w, w);
    std::memcpy (in0 + (w * h), in + (w * h) - 2 * w, w);

// create deinterlaced frame of the reference-field in scratch
    for (y = (1 - field); y < h; y += 2)
      for (x = 0; x < w; x++)
	{

	a  = abs( *(in +x+(y-1)*w)-*(in0+x+(y-1)*w) );
	a += abs( *(in1+x+(y-1)*w)-*(in0+x+(y-1)*w) );

	b  = abs( *(in +x+(y  )*w)-*(in0+x+(y  )*w) );
	b += abs( *(in1+x+(y  )*w)-*(in0+x+(y  )*w) );

	c  = abs( *(in +x+(y+1)*w)-*(in0+x+(y+1)*w) );
	c += abs( *(in1+x+(y+1)*w)-*(in0+x+(y+1)*w) );

	*(scratch+x+(y-1)*w) = *(in0+x+(y-1)*w);
	
	if( (a<16 || c<16) && b<16 ) // Pixel is static?
	{
	// Yes...
	*(scratch+x+(y  )*w) = *(in0+x+(y  )*w);
	*(mmap+x+y*w) = 255; // mark pixel as static in motion-map...
	}
	else
	{
	// No...
	// Do an edge-directed-interpolation

	m  = *(in0+(x-3)+(y-2)*w);
	m += *(in0+(x-2)+(y-2)*w);
	m += *(in0+(x-1)+(y-2)*w);
	m += *(in0+(x-0)+(y-2)*w);
	m += *(in0+(x+1)+(y-2)*w);
	m += *(in0+(x+2)+(y-2)*w);
	m += *(in0+(x+3)+(y-2)*w);
	m += *(in0+(x-3)+(y-1)*w);
	m += *(in0+(x-2)+(y-1)*w);
	m += *(in0+(x-1)+(y-1)*w);
	m += *(in0+(x-0)+(y-1)*w);
	m += *(in0+(x+1)+(y-1)*w);
	m += *(in0+(x+2)+(y-1)*w);
	m += *(in0+(x+3)+(y-1)*w);
	m += *(in0+(x-3)+(y+1)*w);
	m += *(in0+(x-2)+(y+1)*w);
	m += *(in0+(x-1)+(y+1)*w);
	m += *(in0+(x-0)+(y+1)*w);
	m += *(in0+(x+1)+(y+1)*w);
	m += *(in0+(x+2)+(y+1)*w);
	m += *(in0+(x+3)+(y+1)*w);
	m += *(in0+(x-3)+(y+2)*w);
	m += *(in0+(x-2)+(y+2)*w);
	m += *(in0+(x-1)+(y+2)*w);
	m += *(in0+(x-0)+(y+2)*w);
	m += *(in0+(x+1)+(y+2)*w);
	m += *(in0+(x+2)+(y+2)*w);
	m += *(in0+(x+3)+(y+2)*w);
	m /= 28;

	a  = abs(  *(in0+(x-3)+(y-1)*w) - *(in0+(x+3)+(y+1)*w) );
	i = ( *(in0+(x-3)+(y-1)*w) + *(in0+(x+3)+(y+1)*w) )/2;
	a -= abs(m-i);

	b  = abs(  *(in0+(x-2)+(y-1)*w) - *(in0+(x+2)+(y+1)*w) );
	i = ( *(in0+(x-2)+(y-1)*w) + *(in0+(x+2)+(y+1)*w) )/2;
	b -= abs(m-i);

	c  = abs(  *(in0+(x-1)+(y-1)*w) - *(in0+(x+1)+(y+1)*w) );
	i = ( *(in0+(x-1)+(y-1)*w) + *(in0+(x+1)+(y+1)*w) )/2;
	c -= abs(m-i);

	e  = abs(  *(in0+(x+1)+(y-1)*w) - *(in0+(x-1)+(y+1)*w) );
	i = ( *(in0+(x+1)+(y-1)*w) + *(in0+(x-1)+(y+1)*w) )/2;
	e -= abs(m-i);

	f  = abs(  *(in0+(x+2)+(y-1)*w) - *(in0+(x-2)+(y+1)*w) );
	i = ( *(in0+(x+2)+(y-1)*w) + *(in0+(x-2)+(y+1)*w) )/2;
	f -= abs(m-i);

	g  = abs(  *(in0+(x+3)+(y-1)*w) - *(in0+(x-3)+(y+1)*w) );
	i = ( *(in0+(x+3)+(y-1)*w) + *(in0+(x-3)+(y+1)*w) )/2;
	g -= abs(m-i);

	d  = abs(  *(in0+(x  )+(y-1)*w) - *(in0+(x  )+(y+1)*w) );
	i = ( *(in0+(x  )+(y-1)*w) + *(in0+(x  )+(y+1)*w) )/2;
	d -= abs(m-i);
	
	if (a<b && a<c && a<d && a<e && a<f && a<g )
		i = ( *(in0+(x-3)+(y-1)*w) + *(in0+(x+3)+(y+1)*w) )/2;
	else
	if (b<a && b<c && b<d && b<e && b<f && b<g )
		i = ( *(in0+(x-2)+(y-1)*w) + *(in0+(x+2)+(y+1)*w) )/2;
	else
	if (c<a && c<b && c<d && c<e && c<f && c<g )
		i = ( *(in0+(x-1)+(y-1)*w) + *(in0+(x+1)+(y+1)*w) )/2;
	else
	if (e<a && e<b && e<c && e<d && e<f && e<g )
		i = ( *(in0+(x+1)+(y-1)*w) + *(in0+(x-1)+(y+1)*w) )/2;
	else
	if (f<a && f<b && f<c && f<d && f<e && f<g )
		i = ( *(in0+(x+2)+(y-1)*w) + *(in0+(x-2)+(y+1)*w) )/2;
	else
	if (g<a && g<b && g<c && g<d && g<e && g<f )
		i = ( *(in0+(x+3)+(y-1)*w) + *(in0+(x-3)+(y+1)*w) )/2;

	*(scratch+x+(y  )*w) = i;
	*(mmap+x+y*w) = 0; // mark pixel as moving in motion-map...
	}

	}

    if(y == h)
      std::memcpy (scratch + w * (h - 1), in0 + w * (h - 1), w);

// As we now have a rather good interpolation of how the reference frame
// might have been looking for when it had been scanned progressively,
// we try to motion-compensate the former reconstructed frame (the reconstruction
// of the previous field) to the new frame
//
// The block-size may not be too large but to get halfway decent motion-vectors
// we need to have a matching-size of 16x16 at least as most of the material will
// be noisy...

// we need some overshot areas, again
    std::memcpy (out - w, out, w);
    std::memcpy (out - w * 2, out, w);
    std::memcpy (out - w * 3, out, w);

    std::memcpy (out + (w * h), out + (w * h) - w, w);
    std::memcpy (out + (w * h) + w, out + (w * h) - w, w);
    std::memcpy (out + (w * h) + w * 2, out + (w * h) - w, w);
    std::memcpy (out + (w * h) + w * 3, out + (w * h) - w, w);

    std::memcpy (scratch - w, scratch, w);
    std::memcpy (scratch - w * 2, scratch, w);
    std::memcpy (scratch - w * 3, scratch, w);
    std::memcpy (scratch - w * 4, scratch, w);
    std::memset (scratch - w * 4 - 4, scratch[0], 4);

    std::memcpy (scratch + (w * h), scratch + (w * h) - w, w);
    std::memcpy (scratch + (w * h) + w, scratch + (w * h) - w, w);
    std::memcpy (scratch + (w * h) + w * 2, scratch + (w * h) - w, w);
    std::memcpy (scratch + (w * h) + w * 3, scratch + (w * h) - w, w);
    std::memset (scratch + (w * h) + w * 4, scratch[w * h - 1], 11);

    for (y = 0; y < h; y += 8)
      for (x = 0; x < w; x += 8)
	{
	  uint_fast16_t ix = (unsigned)x / 8;
	  uint_fast16_t iy = (unsigned)y / 8;
	  
	  // offset x+y so we get an overlapped search
	  x -= 4;
	  y -= 4;

	  // reset motion-search with the zero-motion-vector (0;0)
	  min = psad_00 (scratch + x + y * w, out + x + y * w, w, 16, 16*16*255);
	  vx = 0;
	  vy = 0;

	  // check some "hot" candidates first...

	  // if possible check all-neighbors-interpolation-vector
	  if (min > 512)
	    if (iy && ix && ix < (iw - 1))
	      {
		dx  = lvxy[ix - 1 + (iy - 1) * iw][0];
		dx += lvxy[ix     + (iy - 1) * iw][0];
		dx += lvxy[ix + 1 + (iy - 1) * iw][0];
		dx += lvxy[ix - 1 + (iy    ) * iw][0];
		dx += lvxy[ix     + (iy    ) * iw][0];
		dx /= 5;
		
		dy  = lvxy[ix - 1 + (iy - 1) * iw][1];
		dy += lvxy[ix     + (iy - 1) * iw][1];
		dy += lvxy[ix + 1 + (iy - 1) * iw][1];
		dy += lvxy[ix - 1 + (iy    ) * iw][1];
		dy += lvxy[ix     + (iy    ) * iw][1];
		dy /= 10;
		dy *= 2;

		sad =
		  psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	      }

	  // if possible check top-left-neighbor-vector
	  if (min > 512)
	    if (iy && ix)
	      {
		dx = lvxy[ix - 1 + (iy - 1) * iw][0];
		dy = lvxy[ix - 1 + (iy - 1) * iw][1];
		sad =
		  psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	      }

	  // if possible check top-neighbor-vector
	  if (min > 512)
	    if (iy)
	      {
		dx = lvxy[ix + (iy - 1) * iw][0];
		dy = lvxy[ix + (iy - 1) * iw][1];
		sad =
		  psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	      }

	  // if possible check top-right-neighbor-vector
	  if (min > 512)
	    if (iy && ix < (iw - 1))
	      {
		dx = lvxy[ix + 1 + (iy - 1) * iw][0];
		dy = lvxy[ix + 1 + (iy - 1) * iw][1];
		sad =
		  psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	      }

	  // if possible check left-neighbor-vector
	  if (min > 512)
	    if (ix)
	      {
		dx = lvxy[ix - 1 + iy * iw][0];
		dy = lvxy[ix - 1 + iy * iw][1];
		sad =
		  psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	      }

	  // check temporal-neighbor-vector
	  if (min > 512)
	    {
	      dx = lvxy[ix + iy * iw][0];
	      dy = lvxy[ix + iy * iw][1];
	      sad = psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
	      if (sad < min)
		{
		  min = sad;
		  vx = dx;
		  vy = dy;
		}
	    }

	  // search for a better one...
	  px = vx;
	  py = vy;

	  if( min>1024 ) // the found predicted location [px;py] is a bad match...
	    {
		// Do a large but coarse diamond search arround the prediction-vector
		//
		//         X
		//        ---
		//       X-X-X
		//      -------
		//     -X-X-X-X-
		//    -----------
		//   X-X-X-O-X-X-X
		//    -----------
		//     -X-X-X-X-
		//      -------
		//       X-X-X
		//        ---
		//         X
		// 
		// The locations marked with an X are checked, only. The location marked with
		// an O was already checked before...
		//
		dx = px-2;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px+2;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px-4;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px+4;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px;
		dy = py-4;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px;
		dy = py+4;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	    }
	// update prediction-vector
	  px = vx;
	  py = vy;

	    {
		// only do a small diamond search here... Either we have passed the large
		// diamond search or the predicted vector was good enough
		//
		//     X
		//    ---
		//   XXOXX
		//    ---
		//     X
		// 
		// The locations marked with an X are checked, only. The location marked with
		// an O was already checked before...
		//

		dx = px-1;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px+1;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px-2;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px+2;
		dy = py;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px;
		dy = py-2;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }

		dx = px;
		dy = py+2;
		sad =
		    psad_00 (scratch + x + y * w, out + (x + dx) + (y + dy) * w, w, 16, min);
		if (sad < min)
		  {
		    min = sad;
		    vx = dx;
		    vy = dy;
		  }
	    }

	  // store the found vector, so we can do a candidates-check...
	  lvxy[ix + iy * iw][0] = vx;
	  lvxy[ix + iy * iw][1] = vy;

	  // remove x+y offset...
	  x += 4;
	  y += 4;

	  // reconstruct that block in scratch
	  // do so by using the lowpass (and alias-term) from the current field
	  // and the highpass (and phase-inverted alias-term) from the previous frame(!)
#if 1
	  for (dy = (1 - field); dy < 8; dy += 2)
	    {
	      uint8_t * RESTRICT dest = scratch + x + (y + dy) * w;
	      uint8_t * RESTRICT src1 = scratch + x + (y + dy) * w;
	      uint8_t * RESTRICT src2 = out + (x + vx) + (y + dy + vy) * w;

	      for (dx = 0; dx < 8; dx++)
		{
		  a =
		    src1[dx - 3 * w] * -1 +
		    src1[dx - 1 * w] * +17 +
		    src1[dx + 1 * w] * +17 +
		    src1[dx + 3 * w] * -1;

		  b =
		    src2[dx - 3 * w] * +1 +
		    src2[dx - 1 * w] * -17 +
		    src2[dx + 0 * w] * +32 +
		    src2[dx + 1 * w] * -17 +
		    src2[dx + 3 * w] * +1;

		  a = a + b;
		  a = a < 0 ? 0 : (a > 8160 ? 255 : (unsigned)a / 32);

		  dest[dx] = a;
	        }
	    }
#else
	  for (dy = (1 - field); dy < 8; dy += 2)
	    for (dx = 0; dx < 8; dx++)
	      {
		*(scratch + (x + dx) + (y + dy) * w) =
			(
			//*(scratch + (x + dx) + (y + dy) * w) +
			*(out + (x + dx + vx) + (y + dy +vy) * w) 
			)/1;
	      }
#endif
	}

    for (y = (1 - field); y < h; y += 2)
      for (x = 0; x < w; x++)
	{
		if ( *(mmap+x+y*w)==255 ) // if pixel is static
		{
		*(scratch + x + (y) * w) = *(in0+x+(y  )*w);
		}
	}

#if 1
    std::memcpy (out, scratch, w * h);
#else
// copy a gauss-filtered variant of the reconstructed image to the out-buffer
// the reason why this must be filtered is not so easy to understand, so I leave
// some room for anyone who might try without... :-)
//
// If you want a starting point: remember an interlaced camera *must* *not* have
// more vertical resolution than approximatly 0.6 times the nominal vertical
// resolution... (So a progressive-scan camera will allways be better in this
// regard but will introduce severe defects when the movie is displayed on an
// interlaced screen...) Eitherways, who cares: this is better than a frame-mode
// were the missing information is generated via a pixel-shift... :-)

    for (y = 0; y < h; y++)
      for (x = 0; x < w; x++)
	{
	  *(out + x + y * w) = (4 * *(scratch + x + (y) * w)+*(scratch + x + (y-1) * w)+*(scratch + x + (y+1) * w))/6;
	}
#endif

	antialias_plane ( out, w, h );
  }

  void scale_motion_vectors (int nom, int rshift)
  {
    int i;

    for (i = (width / 8) * (height / 8); i--; )
      {
	motion[0][i][0] = (motion[0][i][0] * nom) >> rshift;
	motion[0][i][1] = (motion[0][i][1] * nom) >> rshift;
      }
	
    for (i = (cwidth / 8) * (cheight / 8); i--; )
      {
	motion[1][i][0] = (motion[1][i][0] * nom) >> rshift;
	motion[1][i][1] = (motion[1][i][1] * nom) >> rshift;
      }
  }

  void deinterlace_motion_compensated (int frame)
  {
    uint8_t *tmpptr;

    if(frame)
      {
	uint8_t *saveptr[3] = {0, 0, 0};
    
        if(frame == 1)
          {
	    temporal_reconstruct_frame (outframe[0], inframe0[0], inframe[0],  inframe0[0], width, height, field_order, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe0[1], inframe[1],  inframe0[1], cwidth, cheight, field_order, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe0[2], inframe[2],  inframe0[2], cwidth, cheight, field_order, motion[1]);

	    temporal_reconstruct_frame (outframe[0], inframe0[0], inframe[0],  inframe0[0], width, height, 1 - field_order, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe0[1], inframe[1],  inframe0[1], cwidth, cheight, 1 - field_order, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe0[2], inframe[2],  inframe0[2], cwidth, cheight, 1 - field_order, motion[1]);

	    scale_motion_vectors (2, 0);

	    saveptr[0] = inframe1[0];
	    saveptr[1] = inframe1[1];
	    saveptr[2] = inframe1[2];
	    inframe1[0] = inframe[0];
	    inframe1[1] = inframe[1];
	    inframe1[2] = inframe[2];
          }
	else if(frame < 0)
	  {
	    saveptr[0] = inframe[0];
	    saveptr[1] = inframe[1];
	    saveptr[2] = inframe[2];
	    inframe[0] = inframe1[0];
	    inframe[1] = inframe1[1];
	    inframe[2] = inframe1[2];
	  }

	if (field_order == 0)
	  {
	    temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 1, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 1, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 1, motion[1]);

	    y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
	                     &Y4MStream.oframeinfo, outframe);

	    if (frame == 1)
	      scale_motion_vectors (-1, both_fields);

	    if (both_fields == 1)
	      {
		temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 0, motion[0]);
		temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 0, motion[1]);
		temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 0, motion[1]);

		y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
		                 &Y4MStream.oframeinfo, outframe);
	      }
	  }
	else
	  {
	    temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 0, motion[0]);
	    temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 0, motion[1]);
	    temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 0, motion[1]);

	    y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
	                     &Y4MStream.oframeinfo, outframe);

	    if (frame == 1)
	      scale_motion_vectors (-1, both_fields);

	    if (both_fields == 1)
	      {
		temporal_reconstruct_frame (outframe[0], inframe[0], inframe0[0],  inframe1[0], width, height, 1, motion[0]);
		temporal_reconstruct_frame (outframe[1], inframe[1], inframe0[1],  inframe1[1], cwidth, cheight, 1, motion[1]);
		temporal_reconstruct_frame (outframe[2], inframe[2], inframe0[2],  inframe1[2], cwidth, cheight, 1, motion[1]);

		y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo,
		                 &Y4MStream.oframeinfo, outframe);
	      }
	  }

	if (frame < 2)
	  {
	    inframe1[0] = saveptr[0];
	    inframe1[1] = saveptr[1];
	    inframe1[2] = saveptr[2];
	  }
      }

    tmpptr = inframe1[0];
    inframe1[0] = inframe0[0];
    inframe0[0] = inframe[0];
    inframe[0] = tmpptr;

    tmpptr = inframe1[1];
    inframe1[1] = inframe0[1];
    inframe0[1] = inframe[1];
    inframe[1] = tmpptr;

    tmpptr = inframe1[2];
    inframe1[2] = inframe0[2];
    inframe0[2] = inframe[2];
    inframe[2] = tmpptr;
  }

  void antialias_plane (uint8_t * RESTRICT out, int w, int h)
  {
    int x, y;
    int vx;
    uint_fast16_t sad;
    uint_fast16_t min;
    int dx;

    for (y = 2; y < (h - 2); y++)
      for (x = 2; x < (w - 2); x++)
	{
	  min = ~0;
	  vx = 0;
	  for (dx = -3; dx <= 3; dx++)
	    {
	      sad = 0;
//      sad  = abs( *(out+(x+dx-3)+(y-1)*w) - *(out+(x-3)+(y+0)*w) );
//      sad += abs( *(out+(x+dx-2)+(y-1)*w) - *(out+(x-2)+(y+0)*w) );
	      sad += abs (*(out + (x + dx - 1) + (y - 1) * w) - *(out + (x - 1) + (y + 0) * w));
	      sad += abs (*(out + (x + dx + 0) + (y - 1) * w) - *(out + (x + 0) + (y + 0) * w));
	      sad += abs (*(out + (x + dx + 1) + (y - 1) * w) - *(out + (x + 1) + (y + 0) * w));
//      sad += abs( *(out+(x+dx+2)+(y-1)*w) - *(out+(x+2)+(y+0)*w) );
//      sad += abs( *(out+(x+dx+3)+(y-1)*w) - *(out+(x+3)+(y+0)*w) );

//      sad += abs( *(out+(x-dx-3)+(y+1)*w) - *(out+(x-3)+(y+0)*w) );
//      sad += abs( *(out+(x-dx-2)+(y+1)*w) - *(out+(x-2)+(y+0)*w) );
	      sad += abs (*(out + (x - dx - 1) + (y + 1) * w) - *(out + (x - 1) + (y + 0) * w));
	      sad += abs (*(out + (x - dx + 0) + (y + 1) * w) - *(out + (x + 0) + (y + 0) * w));
	      sad += abs (*(out + (x - dx + 1) + (y + 1) * w) - *(out + (x + 1) + (y + 0) * w));
//      sad += abs( *(out+(x-dx+2)+(y+1)*w) - *(out+(x+2)+(y+0)*w) );
//      sad += abs( *(out+(x-dx+3)+(y+1)*w) - *(out+(x+3)+(y+0)*w) );

	      if (sad < min)
		{
		  min = sad;
		  vx = dx;
		}
	    }

	  if (abs (vx) <= 1)
	    *(scratch + x + y * w) =
	      (2**(out + (x) + (y) * w) + *(out + (x + vx) + (y - 1) * w) +
	       *(out + (x - vx) + (y + 1) * w)) / 4;
	  else if (abs (vx) <= 3)
	    *(scratch + x + y * w) =
	      (2 * *(out + (x - 1) + (y) * w) +
	       4 * *(out + (x + 0) + (y) * w) +
	       2 * *(out + (x + 1) + (y) * w) +
	       1 * *(out + (x + vx - 1) + (y - 1) * w) +
	       2 * *(out + (x + vx + 0) + (y - 1) * w) +
	       1 * *(out + (x + vx + 1) + (y - 1) * w) +
	       1 * *(out + (x - vx - 1) + (y + 1) * w) +
	       2 * *(out + (x - vx + 0) + (y + 1) * w) +
	       1 * *(out + (x - vx + 1) + (y + 1) * w)) / 16;
	  else
	    *(scratch + x + y * w) =
	      (2 * *(out + (x - 1) + (y) * w) +
	       4 * *(out + (x - 1) + (y) * w) +
	       8 * *(out + (x + 0) + (y) * w) +
	       4 * *(out + (x + 1) + (y) * w) +
	       2 * *(out + (x + 1) + (y) * w) +
	       1 * *(out + (x + vx - 1) + (y - 1) * w) +
	       2 * *(out + (x + vx - 1) + (y - 1) * w) +
	       4 * *(out + (x + vx + 0) + (y - 1) * w) +
	       2 * *(out + (x + vx + 1) + (y - 1) * w) +
	       1 * *(out + (x + vx + 1) + (y - 1) * w) +
	       1 * *(out + (x - vx - 1) + (y + 1) * w) +
	       2 * *(out + (x - vx - 1) + (y + 1) * w) +
	       4 * *(out + (x - vx + 0) + (y + 1) * w) +
	       2 * *(out + (x - vx + 1) + (y + 1) * w) +
	       1 * *(out + (x - vx + 1) + (y + 1) * w)) / 40;

	}

    for (y = 2; y < (h - 2); y++)
      for (x = 2; x < (w - 2); x++)
	{
	  *(out + (x) + (y) * w) = (*(out + (x) + (y) * w) + *(scratch + (x) + (y + 0) * w)) / 2;
	}

  }

  void antialias_frame ()
  {
    antialias_plane (inframe[0], width, height);
    antialias_plane (inframe[1], cwidth, cheight);
    antialias_plane (inframe[2], cwidth, cheight);

    y4m_write_frame (Y4MStream.fd_out, &Y4MStream.ostreaminfo, &Y4MStream.oframeinfo, inframe);
  }
};

}

int
main (int argc, char *argv[])
{
  int frame = 0;
  int errno = 0;
  int ss_h, ss_v;

  deinterlacer YUVdeint;

  char c;

  YUVdeint.field_order = -1;

  mjpeg_info("-------------------------------------------------");
  mjpeg_info( "       Motion-Compensating-Deinterlacer");
  mjpeg_info("-------------------------------------------------");

  while ((c = getopt (argc, argv, "hvds:t:a")) != -1)
    {
      switch (c)
	{
	case 'h':
	  {
	    mjpeg_info(" Usage of the deinterlacer");
	    mjpeg_info(" -------------------------");
	    mjpeg_info(" -v be verbose");
	    mjpeg_info(" -d output both fields");
	    mjpeg_info(" -a just antialias the frames! This will");
	    mjpeg_info("    assume progressive but aliased input.");
	    mjpeg_info("    you can use this to improve badly deinterlaced");
	    mjpeg_info("    footage. EG: deinterlaced with cubic-interpolation");
	    mjpeg_info("    or worse...");

	    mjpeg_info(" -s [n=0/1] forces field-order in case of misflagged streams");
	    mjpeg_info("    -s0 is bottom-field-first");
	    mjpeg_info("    -s1 is top-field-first");
	    exit (0);
	    break;
	  }
	case 'v':
	  {
	    YUVdeint.verbose = 1;
	    break;
	  }
	case 'd':
	  {
	    YUVdeint.both_fields = 1;
	    mjpeg_info("Regenerating both fields. Please fix the Framerate.");
	    break;
	  }
	case 'a':
	  {
	    YUVdeint.just_anti_alias = 1;
	    YUVdeint.field_order = 0;	// just to prevent the program to barf in this case
	    mjpeg_info("I will just anti-alias the frames. make sure they are progressive!");
	    break;
	  }
	case 't':
	  {
	    mjpeg_info("motion-threshold not used");
	    break;
	  }
	case 's':
	  {
	    YUVdeint.field_order = atoi (optarg);
	    if (YUVdeint.field_order != 0)
	      {
		mjpeg_info("forced top-field-first!");
		YUVdeint.field_order = 1;
	      }
	    else
	      {
		mjpeg_info("forced bottom-field-first!");
		YUVdeint.field_order = 0;
	      }
	    break;
	  }
	}
    }

  // initialize motionsearch-library      
  init_motion_search ();

#ifdef HAVE_ALTIVEC
  reset_motion_simd ("sad_00");
#endif

  // initialize stream-information 
  y4m_accept_extensions (1);
  y4m_init_stream_info (&YUVdeint.Y4MStream.istreaminfo);
  y4m_init_frame_info (&YUVdeint.Y4MStream.iframeinfo);
  y4m_init_stream_info (&YUVdeint.Y4MStream.ostreaminfo);
  y4m_init_frame_info (&YUVdeint.Y4MStream.oframeinfo);

/* open input stream */
  if ((errno = y4m_read_stream_header (YUVdeint.Y4MStream.fd_in,
				       &YUVdeint.Y4MStream.istreaminfo)) != Y4M_OK)
    {
      mjpeg_error_exit1 ("Couldn't read YUV4MPEG header: %s!", y4m_strerr (errno));
    }

  /* get format information */
  YUVdeint.width = y4m_si_get_width (&YUVdeint.Y4MStream.istreaminfo);
  YUVdeint.height = y4m_si_get_height (&YUVdeint.Y4MStream.istreaminfo);
  YUVdeint.input_chroma_subsampling = y4m_si_get_chroma (&YUVdeint.Y4MStream.istreaminfo);
  mjpeg_info("Y4M-Stream is %ix%i(%s)", YUVdeint.width,
	     YUVdeint.height, y4m_chroma_keyword (YUVdeint.input_chroma_subsampling));

  /* if chroma-subsampling isn't supported bail out ... */
  switch (YUVdeint.input_chroma_subsampling)
    {
    case Y4M_CHROMA_420JPEG:
    case Y4M_CHROMA_420MPEG2:
    case Y4M_CHROMA_420PALDV:
    case Y4M_CHROMA_444:
    case Y4M_CHROMA_422:
    case Y4M_CHROMA_411:
      ss_h = y4m_chroma_ss_x_ratio (YUVdeint.input_chroma_subsampling).d;
      ss_v = y4m_chroma_ss_y_ratio (YUVdeint.input_chroma_subsampling).d;
      YUVdeint.cwidth = YUVdeint.width / ss_h;
      YUVdeint.cheight = YUVdeint.height / ss_v;
      break;
    default:
      mjpeg_error_exit1 ("%s is not in supported chroma-format. Sorry.",
			 y4m_chroma_keyword (YUVdeint.input_chroma_subsampling));
    }

  /* the output is progressive 4:2:0 MPEG 1 */
  y4m_si_set_interlace (&YUVdeint.Y4MStream.ostreaminfo, Y4M_ILACE_NONE);
  y4m_si_set_chroma (&YUVdeint.Y4MStream.ostreaminfo, YUVdeint.input_chroma_subsampling);
  y4m_si_set_width (&YUVdeint.Y4MStream.ostreaminfo, YUVdeint.width);
  y4m_si_set_height (&YUVdeint.Y4MStream.ostreaminfo, YUVdeint.height);
  y4m_si_set_framerate (&YUVdeint.Y4MStream.ostreaminfo,
			y4m_si_get_framerate (&YUVdeint.Y4MStream.istreaminfo));
  y4m_si_set_sampleaspect (&YUVdeint.Y4MStream.ostreaminfo,
			   y4m_si_get_sampleaspect (&YUVdeint.Y4MStream.istreaminfo));

/* check for field dominance */

  if (YUVdeint.field_order == -1)
    {
      /* field-order was not specified on commandline. So we try to
       * get it from the stream itself...
       */

      if (y4m_si_get_interlace (&YUVdeint.Y4MStream.istreaminfo) == Y4M_ILACE_TOP_FIRST)
	{
	  /* got it: Top-field-first... */
	  mjpeg_info(" Stream is interlaced, top-field-first.");
	  YUVdeint.field_order = 1;
	}
      else if (y4m_si_get_interlace (&YUVdeint.Y4MStream.istreaminfo) == Y4M_ILACE_BOTTOM_FIRST)
	{
	  /* got it: Bottom-field-first... */
	  mjpeg_info(" Stream is interlaced, bottom-field-first.");
	  YUVdeint.field_order = 0;
	}
      else
	{
	  mjpeg_error("Unable to determine field-order from input-stream.");
	  mjpeg_error("This is most likely the case when using mplayer to produce the input-stream.");
	  mjpeg_error("Either the stream is misflagged or progressive...");
	  mjpeg_error("I will stop here, sorry. Please choose a field-order");
	  mjpeg_error("with -s0 or -s1. Otherwise I can't do anything for you. TERMINATED. Thanks...");
	  exit (-1);
	}
    }

  // initialize deinterlacer internals
  YUVdeint.initialize_memory (YUVdeint.width, YUVdeint.height, YUVdeint.cwidth, YUVdeint.cheight);

  /* write the outstream header */
  y4m_write_stream_header (YUVdeint.Y4MStream.fd_out, &YUVdeint.Y4MStream.ostreaminfo);

  /* read every frame until the end of the input stream and process it */
  while (Y4M_OK == (errno = y4m_read_frame (YUVdeint.Y4MStream.fd_in,
					    &YUVdeint.Y4MStream.istreaminfo,
					    &YUVdeint.Y4MStream.iframeinfo, YUVdeint.inframe)))
    {
      if (!YUVdeint.just_anti_alias)
	YUVdeint.deinterlace_motion_compensated (frame);
      else
	YUVdeint.antialias_frame ();
      frame++;
    }

  if (!YUVdeint.just_anti_alias)
    YUVdeint.deinterlace_motion_compensated (-frame);

  return 0;
}
