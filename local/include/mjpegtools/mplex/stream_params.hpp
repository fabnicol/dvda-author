/*
 * streamparams.hpp:  User specifiable parameter classes for various types of stream
 *
 * The class constructors etc are defined so that it is impossible to build
 * objects with illegal combinations of constructors.
 *
 *  Copyright (C) 2002 Andrew Stevens <andrew.stevens@philips.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __STREAM_PARAMS_HH__
#define __STREAM_PARAMS_HH__

#include <mjpeg_types.h>
class LpcmParams
{
public:
  static LpcmParams *Checked(  unsigned int samples,  
                               unsigned int chans, 
                               unsigned int bits );
  static LpcmParams *Default(  unsigned int opt_mux_format );
  inline unsigned int SamplesPerSec() { return samples_per_sec; }
  inline unsigned int Channels() { return channels; }
  inline unsigned int BitsPerSample() { return bits_per_sample; }
  
private:
  LpcmParams(unsigned int samples,  
             unsigned int chans, 
             unsigned int bits);
  unsigned int samples_per_sec;
  unsigned int channels;
  unsigned int bits_per_sample;
};


class VideoParams
{
public:
  static VideoParams *Checked(unsigned int bufsiz);
  static VideoParams *Default(unsigned int mux_format);
  bool Force(unsigned int mux_format);
  inline unsigned int DecodeBufferSize() { return decode_buffer_size; }
private:
  VideoParams(unsigned int bufsiz);
  unsigned int decode_buffer_size;
};

class SubtitleStreamParams
{
public:
  static SubtitleStreamParams *Checked(const int64_t stream_offset,int8_t stream_id){ return new SubtitleStreamParams (stream_offset,stream_id);} ;
  static SubtitleStreamParams *Default(unsigned int mux_format)	{ return Checked(0,-1);};
  bool Force(unsigned int mux_format);
  inline int64_t Offset() { return _stream_offset; }
  inline int8_t  StreamId() { return _stream_id; }
private:
  SubtitleStreamParams(const int64_t stream_offset, int8_t stream_id)
   {_stream_offset = stream_offset;
   	_stream_id = stream_id;};
  
  int64_t _stream_offset;
  int8_t _stream_id;
};


//
// Class of sequence of frame intervals.
//



class FrameIntervals
{
public:
    virtual ~FrameIntervals() {}
	virtual int NextFrameInterval() = 0;
};


class ConstantFrameIntervals : public FrameIntervals
{
public:
	ConstantFrameIntervals( int _frame_interval ) :
		frame_interval( _frame_interval )
		{
		}
	int NextFrameInterval() { return frame_interval; };
private:
	int frame_interval;
};


class StillsParams : public VideoParams
{
public:
  StillsParams( VideoParams *parms, FrameIntervals *ints ) :
    VideoParams(*parms),
    intervals(ints)
  {}
  inline FrameIntervals *Intervals() { return intervals; }
private:
  FrameIntervals *intervals;
};


#endif

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
