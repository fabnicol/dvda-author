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

typedef struct vobsub_header_s {

    char marker[8];
    subtitle_header_v3_t header;
} vobsub_header ;

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
    kind =  ElementaryStream::subtitle;
    num_frames =0;
    initial_offset = -1;
    last_sub_stream_id =-1;

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

bool SUBPStream::CheckAndSkipHeader( vobsub_header& vobsub, bool bitwise)
{


    uint8_t* ptr = reinterpret_cast<uint8_t*> (&vobsub);
    subtitle_header_v3_t &header = vobsub.header;
    int i;

    if (bitwise)
    {

        for (i=0; i<sizeof(vobsub); i++)
            *(ptr+i) = bs.GetBits(8);
    }
    else
    {
        bs.GetBytes((uint8_t*) &vobsub,sizeof(vobsub));
    }

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
        if (bitwise)
            bs.SeekFwdBits(skip_len);
        else
        {
            uint8_t* b = (uint8_t* ) malloc(skip_len);

            bs.GetBytes(b,skip_len);
            free (b);
        }
    }
    return true;

}


bool SUBPStream::ParseAUBitwise()
{
    vobsub_header vobsub;


    if (!CheckAndSkipHeader(vobsub, true))
        return false;
    subtitle_header_v3_t &header = vobsub.header;
    uint8_t subpid;
    prev_offset = AU_start;
    AU_start = bs.bitcount();
    // packet starts here...
    subpid = bs.GetBits(8);


    access_unit.start = AU_start;
    access_unit.length = header.payload_length;
    access_unit.PTS=header.rpts*CLOCKS;
    if(header.rpts > 0) {


        if (initial_offset == -1)
        {
            if (sub_stream_id == -1)
            {
                sub_stream_id = subpid;
                last_sub_stream_id =subpid;
            }
            mjpeg_info( "SUBTITLE id 0x%02X => 0x%02X", subpid,sub_stream_id);
            initial_offset = header.rpts*CLOCKS;
            mjpeg_info("Stream  offset is :       %lld (PTS)",access_unit.PTS);
            mjpeg_info("Initial offset is :       %lld (PTS)",initial_offset);
            mjpeg_info("Cmd line offset is:       %lld (PTS)",parms->Offset());
            initial_offset -= parms->Offset();
            mjpeg_info("Adjustment offset :       %lld (PTS)",initial_offset);
//			mjpeg_info("fist sector scr   :       %f   (LPTS)",((double)header.lpts)/CLOCKS);
        }
        access_unit.PTS-=initial_offset;
        //access_unit.PTS-=initial_offset;


    } else {
        // calculate the time from lpts
        mjpeg_info( "Subtitle %d: fallback to lpts", subpid);
        access_unit.PTS= ((double)header.lpts)/CLOCKS -initial_offset;
    }
    if (subpid != last_sub_stream_id)
        mjpeg_info("Subtitle id changes from 0x%02X to 0x%02X in during muxing, is that realy what you want?",last_sub_stream_id, subpid);

    access_unit.DTS = access_unit.PTS;
    access_unit.dorder = decoding_order;

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
    mjpeg_info ("Nr. of subtitle packets read: %d, wrote: %d",  num_frames, nsec);

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
    bool finished_pack = true;
    bitcount_t read_start = bs.GetBytePos();
    mjpeg_debug( "SUBPStream called: ReadPacketPayload at 0x%08lld", read_start);
    unsigned int bytes_max, bytes_read;
    unsigned int bytes_muxed = bytes_read;
    if (new_au_next_sec == true)
    {   // check and skip header

        vobsub_header vobsub;
        if (!CheckAndSkipHeader(vobsub, false))
            return 0;


        bytes_max = (au_unsent<to_read)?au_unsent:to_read;
        bytes_read = bs.GetBytes( dst, bytes_max);
        if (au_unsent>bytes_read)
        {

            finished_pack=false;
        }
        else
        {

            finished_pack=true;
        }
        bs.Flush( read_start );
        dst[0] = sub_stream_id;
        bytes_muxed = bytes_read;
    }
    else
    {
        // continious block
        bytes_max = (au_unsent<to_read-1)?au_unsent:to_read-1;
        bytes_read = bs.GetBytes( dst+1, bytes_max);
        dst[0] = sub_stream_id;
        if (bytes_read == au_unsent)
            finished_pack=true;
        bytes_muxed = bytes_read+1;

        bs.Flush( read_start );

    }



    clockticks   decode_time;


    if (bytes_muxed == 0 || MuxCompleted() )
    {
        goto completion;
    }


    /* Check if we're finished with that specific pack

    */


    decode_time = RequiredDTS();
    mjpeg_debug("SUBPStream: Required DTS is %f",decode_time/300.0);

    if (!finished_pack)
    {
        bufmodel.Queued( bytes_muxed, decode_time);
        au_unsent -= bytes_muxed;
        if (!new_au_next_sec)
            au_unsent++;
        new_au_next_sec = false;
    }
    else //  if (au_unsent == bytes_muxed)
    {
        bufmodel.Queued(bytes_muxed, decode_time);
        new_au_next_sec = NextAU();
    }
completion:

    return bytes_muxed;
    //return bytes_read;
}
void SUBPStream::OutputSector ( )

{
    clockticks   PTS;
    unsigned int max_packet_data;
    unsigned int actual_payload;
    unsigned int old_au_then_new_payload;

    PTS = RequiredDTS();
    old_au_then_new_payload =
        muxinto.PacketPayload( *this, buffers_in_header, false, false );
    bool last_packet = Lookahead() == 0;
    // Ensure we have access units data buffered to allow a sector to be
    // written.
    max_packet_data = 0;
    if( (muxinto.running_out && NextRequiredPTS() > muxinto.runout_PTS)
            || last_packet)
    {
        /* We're now in the last AU of a segment.  So we don't want to
           go beyond it's end when writing sectors. Hence we limit
           packet payload size to (remaining) AU length.
        */
        max_packet_data = au_unsent+StreamHeaderSize();
    }

    /* CASE: packet starts with new access unit			*/


    if (new_au_next_sec)
    {

        actual_payload =
            muxinto.WritePacket ( max_packet_data,
                                  *this,
                                  buffers_in_header, PTS, 0,
                                  TIMESTAMPBITS_PTS);

    }


    /* CASE: packet starts with old access unit, no new one	*/
    /*       starts in this very same packet			*/
    /* for subtitles: never start a new subtitle in the same packet */
    else
    {



        actual_payload =
            muxinto.WritePacket ( max_packet_data,
                                  *this,
                                  buffers_in_header, 0, 0,
                                  TIMESTAMPBITS_NO );
    }



    ++nsec;

    buffers_in_header = always_buffers_in_header;

}
