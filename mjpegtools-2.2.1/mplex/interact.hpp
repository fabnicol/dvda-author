
/*
 *  interact.hpp:  Simple command-line front-end
 *
 *  Copyright (C) 2001 Andrew Stevens <andrew.stevens@philips.com>
 *
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

#ifndef __INTERACT_HH__
#define __INTERACT_HH__

#ifndef _WIN32
#include <unistd.h>
#endif
#include <vector>
#include "mjpeg_types.h"
#include "stream_params.hpp"
#include "systems.hpp"

class IBitStream;

using std::vector;

/*************************************************************************
 *
 * The Multiplexor job Parameters:
 * The various parametes of a multiplexing job: muxing options
 *
 *************************************************************************/

struct Workarounds
{
  Workarounds();
};

class MultiplexParams
{
public:
  unsigned int data_rate;
  unsigned int packets_per_pack;
  int video_offset;             // A/V sync offset. Always one 0 and the
                                // other positive. Specified in 
  int audio_offset;             // MPEG-2 CLOCKS: 1/(90000*300)-th sec

  unsigned int sector_size;
  bool VBR;                     // Force VBR even if profile suggests CBR
  bool CBR;                     // Force CBR even if profile suggests VBR
  int mpeg;
  int mux_format;
  bool multifile_segment;
  bool always_system_headers;
  unsigned int max_PTS;
  bool stills;
  int verbose;
  int max_timeouts;
  const char *outfile_pattern;
  const char *vdr_index_pathname;
  int max_segment_size;
  int min_pes_header_len;
  int run_in_frames;            // Run-in expressed in Frame intervals
  Workarounds workarounds;      // Special work-around flags that
                                // constrain the syntax to suit
                                // the foibles of particular MPEG
                                // parsers that are (guessed) to be
                                // actually slightly broken.  Always
                                // off by default...

};

/***********************************************************************
 *
 * Multiplexor job - paramters plus the streams to mux.
 *
 *
 **********************************************************************/

enum StreamKind
  {
    MPEG_AUDIO,
    AC3_AUDIO,
    LPCM_AUDIO,
    DTS_AUDIO,
    MPEG_VIDEO,
    SUBP_STREAM,
  };

class JobStream
{
public:

  JobStream( IBitStream *_bs,  StreamKind _kind ) :
    bs(_bs),
    kind(_kind)
  {
  }

  const char *NameOfKind();
  IBitStream *bs;
  StreamKind kind;
};

class MultiplexJob : public MultiplexParams
{
public:
  MultiplexJob();
  virtual ~MultiplexJob();
  unsigned int NumberOfTracks( StreamKind kind );
  void GetInputStreams( vector<JobStream *> &streams, StreamKind kind );

  void SetupInputStreams( vector<IBitStream *> &inputs );
protected:

public:  
  vector<JobStream *> streams;
  vector<LpcmParams *> lpcm_param;
  vector<VideoParams *> video_param;
  vector<SubtitleStreamParams*> subtitle_params;
  unsigned int audio_tracks;
  unsigned int video_tracks;
  unsigned int subtitle_tracks;
  unsigned int lpcm_tracks;
};


/*************************************************************************
    Program ID
*************************************************************************/
 
#define MPLEX_VER    "2.2.7"
#define MPLEX_DATE   "$Date: 2012-11-17 01:55:16 $"

#endif // __INTERACT_H__


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
