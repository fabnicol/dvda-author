//
// C++ Implementation: subpstream
//
// Description: 
//
//
// Author: grotti <grotti@verstehts.net>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
// Most of the code is stolen from ac3stream.cpp
//
//
#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "audiostrm.hpp"
#include "interact.hpp"
#include "multiplexor.hpp"
// minimum version code for subtitle stream that
// can be handled by the program
#define MIN_VERSION_CODE 0x00030000
// common part of the subtitle header struct for major version 3
// taken from subtitle2vobsub

typedef struct {

  unsigned int header_length;
  unsigned int header_version;
  unsigned int payload_length;

  unsigned int lpts;
  double rpts;
  
  // version 0x00030001
  unsigned int discont_ctr;

} subtitle_header_v3_t;

typedef struct {
	char marker[8];
	subtitle_header_v3_t header;
} vobsub_header;

static unsigned int minor_version(unsigned int version)
{
    // bit 0-15 contain the minor version number
    return version & 0xffff;
}

// get the major version number from the version code
static unsigned int major_version(unsigned int version)
{
    // bit 16-31 contain the major version number
    return version >> 16;
}


static const char* SUBPHEADER="SUBTITLE";

SUBPStream::SUBPStream(IBitStream &ibs, SubtitleStreamParams* subpparm, Multiplexor &into) : 
	AudioStream( ibs, into ),parms(subpparm)
    
{
 
 num_frames =0;
 initial_offset = -1;
}




bool SUBPStream::Probe(IBitStream &bs )
{
    //char *last_dot = strrchr( bs, '.' );
 char buffer[20];
 bs.GetBytes((uint8_t*)buffer,strlen(SUBPHEADER));
if (strncmp(buffer,SUBPHEADER,strlen(SUBPHEADER)) == 0)
	return true;
return false;
    

}

void SUBPStream::Init(const int stream_num)
{
MuxStream::Init( PRIVATE_STR_1, 
					 1,  // Buffer scale
					 8192, // default buffer size
					 false,
					 muxinto.buffers_in_audio,
					 muxinto.always_buffers_in_audio
		);
	mjpeg_info ("Scanning for header info: Subpicture stream %02x (%s)",
                stream_num,
                bs.StreamName()
                );
    sub_stream_id =parms->StreamId();
    
    ParseAUBitwise();
    
}

bool SUBPStream::ParseAUBitwise()
{
	vobsub_header vobsub;
	uint8_t* ptr = reinterpret_cast<uint8_t*> (&vobsub);
	subtitle_header_v3_t &header = vobsub.header;
	char buffer[20];
	int i;
	for (i=0; i<sizeof(vobsub); i++)
		*(ptr+i) = bs.GetBits(8);

	if (strncmp(vobsub.marker,SUBPHEADER,strlen(SUBPHEADER)) != 0)
	{
		mjpeg_error( "Subtitle: expected header %s!",SUBPHEADER);
		return false; 
	
	}

	if (major_version(header.header_version) != major_version(MIN_VERSION_CODE))
	{
		mjpeg_error( "Subtitle: expected version 0x%08X, got version 0x%08X while reading subtitle header!",MIN_VERSION_CODE,header.header_version);
		return false; 
	}
	int16_t skip_len = header.header_length-sizeof(header);

	if (skip_len)
	{
		assert (skip_len>0);
		bs.SeekFwdBits(skip_len);
	}
	uint8_t subpid;
	prev_offset = AU_start;
	AU_start = bs.bitcount();
	// packet starts here...
	subpid = bs.GetBits(8);

	
	access_unit.start = AU_start;
	access_unit.length = header.payload_length;
	if(header.rpts > 0){

		access_unit.PTS=header.rpts*300.0*90000.0;
		if (initial_offset == -1)
		{
			if (sub_stream_id == -1)
				sub_stream_id = subpid;
			mjpeg_info( "SUBTITLE id 0x%02X => 0x%02X", subpid,sub_stream_id);
			initial_offset = access_unit.PTS;
			mjpeg_info("Stream  offset is :       %lld (PTS)",access_unit.PTS);
			mjpeg_info("Initial offset is :       %lld (PTS)",initial_offset);
			mjpeg_info("Cmd line offset is:       %lld (PTS)",parms->Offset());
			initial_offset -= parms->Offset();
			mjpeg_info("Adjustment offset :       %lld (PTS)",initial_offset);
		}
		access_unit.PTS-=initial_offset;
		mjpeg_debug("Subtitle: appending PTS/DTS (int64_t) %lld",access_unit.PTS);
		
		} else {
		// calculate the time from lpts
		mjpeg_info( "Subtitle: fallback to lpts", subpid);
		access_unit.PTS= (double)(header.lpts/300)/90000.0;
		}

	access_unit.DTS = access_unit.PTS;
	access_unit.dorder = decoding_order;
	mjpeg_debug("appending PTS/DTS %lld",access_unit.DTS);
	decoding_order++;
	aunits.Append( access_unit );
	

	bs.SeekFwdBits(header.payload_length-1);
	num_frames++;
	return true;
}

void SUBPStream::Close()
{
    stream_length = AU_start >> 3;
	 mjpeg_info ("SUBTITLE STATISTICS:  0x%02x", sub_stream_id); 
    mjpeg_info ("Subtitle stream length  %lld bytes.", stream_length);
    mjpeg_info ("Nr. of subtitle packets:%d",  num_frames);

}

void SUBPStream::FillAUbuffer(unsigned int frames_to_buffer)
{
	uint32_t discont_ctr;
	last_buffered_AU += frames_to_buffer;
	mjpeg_debug( "Scanning %d Subpicture frames to frame %d", 
				 frames_to_buffer, last_buffered_AU );
	prev_offset = AU_start;
	AU_start = bs.bitcount();
	while (decoding_order < last_buffered_AU && !bs.eos()
           && !muxinto.AfterMaxPTS(access_unit.PTS))
	{
		if (!ParseAUBitwise())
			break;
	
	}

	last_buffered_AU = decoding_order;
	eoscan = bs.eos() || muxinto.AfterMaxPTS(access_unit.PTS);
}

unsigned int SUBPStream::ReadPacketPayload(uint8_t *dst, unsigned int to_read)
{
	bitcount_t read_start = bs.GetBytePos();
   mjpeg_debug( "SUBPStream called: ReadPacketPayload at 0x%08lld", read_start);
	vobsub_header vobsub;
	subtitle_header_v3_t &header = vobsub.header;
	bs.GetBytes((uint8_t*) &vobsub,sizeof(vobsub));
	int16_t skip_len = header.header_length-sizeof(header);

	if (strncmp(vobsub.marker,SUBPHEADER,strlen(SUBPHEADER)) != 0)
	{
		mjpeg_error( "Subtitle: expected header %s!",SUBPHEADER);
		return 0; 
	
	}

	assert (skip_len>=0);
	if (skip_len)
	{
		uint8_t* b = (uint8_t* ) malloc(skip_len);
		
		bs.GetBytes(b,skip_len);
		free (b);
	}
	unsigned int bytes_read = bs.GetBytes( dst, header.payload_length );
	bs.Flush( read_start );
	dst[0] = sub_stream_id;
	clockticks   decode_time;

   unsigned int first_header = 
        (new_au_next_sec || au_unsent > bytes_read )
        ? 0 
        : au_unsent;

    // BUG BUG BUG: how do we set the 1st header pointer if we have
    // the *middle* part of a large frame?
    assert( first_header+2 <= to_read );

    unsigned int syncwords = 0;
    unsigned int bytes_muxed = bytes_read;
  
	 if (bytes_muxed == 0 || MuxCompleted() )
    {
		goto completion;
    }


	/* Work through what's left of the current AU and the following AU's
	   updating the info until we reach a point where an AU had to be
	   split between packets.
	   NOTE: It *is* possible for this loop to iterate. 

	   The DTS/PTS field for the packet in this case would have been
	   given the that for the first AU to start in the packet.

	*/

	
	decode_time = RequiredDTS();
	mjpeg_debug("SUBPStream: Required DTS is %lld",decode_time);
	while (au_unsent < bytes_muxed)
	{	  
        // BUG BUG BUG: if we ever had odd payload / packet size we might
        // split an AC3 frame in the middle of the syncword!
        assert( bytes_muxed > 1 );
		bufmodel.Queued(au_unsent, decode_time);
		bytes_muxed -= au_unsent;
        if( new_au_next_sec )
            ++syncwords;
		if( !NextAU() )
        {
            goto completion;
        }
		new_au_next_sec = true;
		decode_time = RequiredDTS();
		mjpeg_debug("Required DTS is %lld (while-loop)",decode_time);
	};

	// We've now reached a point where the current AU overran or
	// fitted exactly.  We need to distinguish the latter case
	// so we can record whether the next packet starts with an
	// existing AU or not - info we need to decide what PTS/DTS
	// info to write at the start of the next packet.
	
	if (au_unsent > bytes_muxed)
	{
        if( new_au_next_sec )
            ++syncwords;
		bufmodel.Queued( bytes_muxed, decode_time);
		au_unsent -= bytes_muxed;
		new_au_next_sec = false;
	} 
	else //  if (au_unsent == bytes_muxed)
	{
		bufmodel.Queued(bytes_muxed, decode_time);
      if( new_au_next_sec )
           ++syncwords;
      new_au_next_sec = NextAU();
	}	   
completion:
    
	return bytes_read;
}
