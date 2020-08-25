/*
	reader.cpp - wave and flac input processing.
	Copyright (C) 2006-2011 Bahman Negahban

    Adapted to C++-17 in 2018 by Fabrice Nicol

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2 of the License, or (at your
	option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
	Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation,
	Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/



#include "processor.hpp"


// ----------------------------------------------------------------------------
//    lpcmReader::adjust :
// ----------------------------------------------------------------------------
//    External adjustment of stream dimension.
//
//    Arguments:
//       <prepend>      - number of bytes prepended to stream (must already
//                        be present at start of bigBuf).
//       <pad>          - whether to allow padding of final sample if
//                        necessary for dvd sample alignment.
//
//    Returns number of bytes to be padded at end of stream.
// ----------------------------------------------------------------------------


int lpcmReader::adjust( int prepend, bool pad )
{
	int padding = 0;

	if( prepend )
	{
		ct.now += prepend;
		if( alignment )
		{
			surplus += prepend;
			surplus %= alignment;
		}
		state |= prepended;
	}

	if( pad )
	{
		padding = ( surplus ? alignment - surplus : 0 );
		if( padding )
			state |= padded;
		else
			clearbits( state, padded );
	}

	return padding;
}


// ----------------------------------------------------------------------------
//    lpcmReader::read :
// ----------------------------------------------------------------------------
//    Buffered reading of audio file in dvd byte-order.
//
//    Arguments:
//       <buf>       - pointer to start of receiving buffer
//       <len>       - number of bytes to transfer
//
//    Returns number of bytes actually transferred
// ----------------------------------------------------------------------------


uint64_t lpcmReader::read( unsigned char *buf, uint64_t len )
{
	int64_t avail = ct.now - bufPos;

	if( avail < (int64_t) len && ! ( state & _eoi ) )
	{
		memmove( bigBuf, bigBuf + bufPos, avail );

		counter<uint64_t> c( 0, avail, 0 );
		fillBuf( 0, &c );
		if( state & _eof )
		{
			clearbits( state, _eof );
			state |= _eoi;
			if( surplus && state & padded )
			{
				int padding = alignment - surplus;
				memset( bigBuf + ct.now, 0, padding );
				gcount += padding;
				ct.now += padding;
				surplus = 0;
			}
			else
			{
				gcount -= surplus;
				ct.now -= surplus;
			}
		}

		if( state & prepended )
		{
			gcount += avail;
			avail = 0;
			clearbits( state, prepended );
		}

		swap2dvd( bigBuf + avail, gcount,
			fmeta.data.stream_info.channels,
			fmeta.data.stream_info.bits_per_sample );

		bufPos = 0;
		avail = ct.now;
	}

	else if( ! avail && state & _eoi )
	{
		state |= _eof;
		memmove( bigBuf, bigBuf + bufPos, surplus );
	}

	len = ( avail < (int64_t) len ) ? avail : len;
	memcpy( buf, bigBuf + bufPos, len );
	bufPos += len;
	pos.now += len;

	return len;
}

#if 0 // testing Lee Feldkamp's permutation() framework
#define READER_CPP
#include "multichannel.cpp"
#else


// ----------------------------------------------------------------------------
//    lpcmReader::swap2dvd :
// ----------------------------------------------------------------------------
//    Swaps given lpcm data buffer from wave to dvd byte-order.
//
//    Arguments:
//       <data>            - pointer to start of input buffer
//       <count>           - number of bytes to process
//       <channels>        - number of audio channels
//       <bitspersample>   - audio quantization
//
//    Returns 0 on success, number of remaining unswapped bytes if <count>
//    is out of alignment with interleaved dvd sample size.
// ----------------------------------------------------------------------------
// (more or less verbatim from Dave Chapman's dvda-author::audio.c::audio_read())


int lpcmReader::swap2dvd( unsigned char *data, uint32_t count,
	int channels, int bitspersample )
{
	uint32_t i = 0;
	int x;
	// Convert little-endian WAV samples to big-endian MPEG LPCM samples
	if ( bitspersample == 16 )
		for( i=0; i < count; i+=2 )
		{
			x = data[i+1];
			data[i+1] = data[i];
			data[i] = x;
		}

	else if( bitspersample == 24 )
	{
		if( channels == 1 )
		{
			/* 24-bit mono samples are packed as follows:

					0  1  2  3  4  5
			WAV: 01 23 45 12 34 56
			DVD: 45 23 56 34 01 12

			*/
			for( i=0; i < count; i+=6 )
			{
				x = data[i];
				data[i] = data[i+2];
				data[i+2] = data[i+5];
				data[i+5] = data[i+3];
				data[i+3] = data[i+4];
				data[i+4] = x;
			}
		}
		else /*if( channels == 2 )*/
		{
			/* 24-bit Stereo samples are packed as follows:

					0  1  2  3  4  5  6  7  8  9 10 11
			WAV: 01 23 45 bf 60 8c 67 89 ab b7 d4 e3
			DVD: 45 23 8c 60 ab 89 e3 d4 01 bf 67 b7

			*/

			for( i=0; i < count; i+=12 )
			{
				x = data[i];
				data[i] = data[i+2];
				data[i+2] = data[i+5];
				data[i+5] = data[i+7];
				data[i+7] = data[i+10];
				data[i+10] = data[i+6];
				data[i+6] = data[i+11];
				data[i+11] = data[i+9];
				data[i+9] = data[i+3];
				data[i+3] = data[i+4];
				data[i+4] = data[i+8];
				data[i+8] = x;
			}
		}
	}

	return count - i;
}

#endif


// ----------------------------------------------------------------------------
//    waveReader::reset :
// ----------------------------------------------------------------------------
//    Opens input file, reads wave header, initializes md5 signature and block
//    alignment, and checks audio characteristics (Resolves inherited virtual
//    lpcmReader::reset).
//
//    Arguments:
//       <filename>  - wave input filename
//       <alignUnit> - block alignment to use (1 interchannel sample if omitted)
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t waveReader::reset( const string& filename, int alignUnit )
{
#if 0
	waveHeader::canonical header;
#endif
	if( waveFile.is_open() )
		waveFile.close();

	state = 0;

	waveFile.open( filename, ios::binary );

	if( ! waveFile.is_open() )
        FATAL( "Can't find input file " + string(filename) );

	fName = filename;
	state |= named;

	if( ! waveHeader::open( waveFile, &fmeta ) )
        FATAL( "Can't open wave file " + filename );

	pos.max = unread = flacHeader::bytesUncompressed( &fmeta );
	surplus = ( alignment ? unread % alignment : 0 );
	gcount = bufPos = 0;

	alignment = alignUnit ? alignUnit :
		fmeta.data.stream_info.channels *
		fmeta.data.stream_info.bits_per_sample / 8;

	md5_init( &md5 );

	soundCheck( this );

	return 0;
}



// ----------------------------------------------------------------------------
//    waveReader::fillBuf :
// ----------------------------------------------------------------------------
//    Fills external buffer, resets counter, and maintains running md5
//    signature. (Resolves inherited virtual lpcmReader::fillBuf).
//
//    Arguments:
//       <limit>     - fill limit (to nearest block alignment if omitted)
//       <midCount>  - reset values for counter (0 if omitted or NULL)
//
//    Returns number of bytes processed.
// ----------------------------------------------------------------------------



uint64_t waveReader::fillBuf( uint64_t limit, counter<uint64_t> *midCount )
{
//   uint64_t bytesRead;

	if( midCount )
		ct = *midCount;
	else
		ct.start = ct.now = 0;

	ct.max = sizeofbigBuf / alignment * alignment;

	if( limit && limit < ct.max )
		ct.max = limit;

	waveFile.read( (char *) bigBuf + ct.now, ct.max - ct.now );

	if( (gcount = waveFile.gcount()) )
	{
		md5_append( &md5, bigBuf + ct.now, gcount );
		ct.now += gcount;
		unread -= gcount;
	}

	if( unread <= 0 || waveFile.eof() || waveFile.peek() == EOF )
	{
		state |= _eof;
		md5_finish( &md5, (md5_byte_t*) &fmeta.data.stream_info.md5sum );
	}

	return ct.now;
}




// ----------------------------------------------------------------------------
//    flacReader::reset :
// ----------------------------------------------------------------------------
//    Initializes flac decoder and reads flac header, initializes block
//    alignments/counters and creates reservoir (Resolves inherited virtual
//    lpcmReader::reset).
//
//    Arguments:
//       <filename>  - flac input filename
//       <alignUnit> - block alignment to use (1 interchannel sample if omitted)
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t flacReader::reset( const string&  filename, int alignUnit )
{
	if( is_valid() )
		finish();

	state = 0;

#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
	set_filename( filename );
	if ( init() != FLAC__FILE_DECODER_OK )
#else // flac 1.1.3+
	if ( init( filename ) != FLAC__STREAM_DECODER_INIT_STATUS_OK )
#endif
	{
		ERR( "Failed to initialize flac decoder\n" );
		return( 1 );
	}

	fName = filename;
	state |= named;

	if ( ! process_until_end_of_metadata() )
	{
		ERR( "Failed to read flac file metadata\n" );
		return( 1 );
	}

	soundCheck( this );

	unsent = 0;
	alignment = alignUnit ? alignUnit :
		fmeta.data.stream_info.channels *
		fmeta.data.stream_info.bits_per_sample / 8;

	maxFrame = fmeta.data.stream_info.max_blocksize *
		fmeta.data.stream_info.channels *
		fmeta.data.stream_info.bits_per_sample / 8;

	if( reserve ) delete reserve;
	reserve = new char[ maxFrame ];

	pos.max = unread = flacHeader::bytesUncompressed( &fmeta );
	surplus = ( alignment ? unread % alignment : 0 );
	gcount = bufPos = 0;

	return 0;
}


// ----------------------------------------------------------------------------
//    flacReader::fillBuf :
// ----------------------------------------------------------------------------
//    Fills external buffer to nearest aligned flac frame, first transferring
//    reservoir data in if any; then storing unsent portion of flac frame, if
//    any, in reservoir (Resolves inherited virtual lpcmReader::fillBuf).
//
//    Arguments:
//       <limit>     - fill limit (to nearest block alignment under nearest
//                     flac frame if omitted)
//       <midCount>  - reset values for counter (0 if omitted or NULL)
//
//    Returns number of bytes processed.
// ----------------------------------------------------------------------------



uint64_t flacReader::fillBuf( uint64_t limit, counter<uint64_t> *midCount )
{
	if( midCount )
		ct = *midCount;
	else
		ct.start = ct.now = 0;

	ct.max = ( sizeofbigBuf - maxFrame ) / 12 * 12;
	gcount = ct.now;

	if( unsent )
	{
		memcpy( bigBuf + ct.now, reserve, unsent );
		ct.now += unsent;
		unsent = 0;
	}

	if( limit && limit < ct.max )
		ct.max = limit;


	while ( ct.now < ct.max )
	{
		if ( process_single() == 0 )
			FATAL( "Unable to decode flac file." );

#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
		if( get_state() == FLAC__FILE_DECODER_END_OF_FILE )
#else // flac 1.1.3+
		if( get_state() == FLAC__STREAM_DECODER_END_OF_STREAM )
#endif
		{
			state |= _eof;
			break;
		}
	}

	if( unread == 0 )
		state |= _eof;

	if( /* limit && */ ct.now > ct.max )
	{
		if( (unsent = ct.now - ct.max ))
			clearbits( state, _eof );
		memcpy( reserve, bigBuf + ct.max, unsent );
		ct.now = ct.max;
	}

	gcount = ct.now - gcount;

	return ct.now;
}



// ----------------------------------------------------------------------------
//    flacReader::write_callback :
// ----------------------------------------------------------------------------
//    Transfers given decoded flac frame to external buffer, sorting samples
//    to wave (channel-interleaved) order. (Resolves inherited virtual
//    FLAC::Decoder::File::write_callback).
//
//    Arguments/Return: see flac API documentation
// ----------------------------------------------------------------------------
// (more or less verbatim from Dave Chapman's dvda-author::audio.c::flac_write_callback())


::FLAC__StreamDecoderWriteStatus flacReader::write_callback(
	const ::FLAC__Frame *frame, const FLAC__int32 * const buf[] )
{

	uint64_t i;
	uint32_t frameLen = frame->header.blocksize * frame->header.channels *
		frame->header.bits_per_sample / 8;

	if( ct.now + frameLen > sizeofbigBuf )
		FATAL( "Flac read buffer overflow." );

	i = ct.now;

	if( frame->header.bits_per_sample == 24 )
		for( uint samp = 0; samp < frame->header.blocksize; samp++ )
			for( uint chan = 0; chan < frame->header.channels; chan++ )
			{
				bigBuf[i++] = ( buf[chan][samp] & 0xff );
				bigBuf[i++] = ( buf[chan][samp] & 0xff00 ) >> 8;
				bigBuf[i++] = ( buf[chan][samp] & 0xff0000 ) >> 16;
			}
	else
		for( uint samp = 0; samp < frame->header.blocksize; samp++ )
			for( uint chan = 0; chan < frame->header.channels; chan++ )
			{
				bigBuf[i++] = ( buf[chan][samp] & 0xff );
				bigBuf[i++] = ( buf[chan][samp] & 0xff00 ) >> 8;
			}

	unread -= ( i - ct.now );
	ct.now = i;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


// ----------------------------------------------------------------------------
//    flacReader::metadata_callback :
// ----------------------------------------------------------------------------
//    Makes an internal copy of the given streamInfo block.
//    (Resolves inherited virtual FLAC::Decoder::File::metadata_callback).
//
//    Arguments: see flac API documentation
// ----------------------------------------------------------------------------


void flacReader::metadata_callback( const ::FLAC__StreamMetadata *meta )
{
	if( meta->type == FLAC__METADATA_TYPE_STREAMINFO )
		memcpy( &fmeta, meta, sizeof( FLAC__StreamMetadata ) );
}


// ----------------------------------------------------------------------------
//    flacReader::error_callback :
// ----------------------------------------------------------------------------
//    Logs flac API error event.
//    (Resolves inherited virtual FLAC::Decoder::File::error_callback).
//
//    Arguments: see flac API documentation
// ----------------------------------------------------------------------------


void flacReader::error_callback( ::FLAC__StreamDecoderErrorStatus status )
{
	ERR( _f( "flac %s API error %d: '%s'\n", FLAC__VERSION_STRING, status,
		(const char*[]){
			// from FLAC/stream_decoder.h
			"FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC",
			"FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER",
			"FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH",
			"FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM"
		} [status] ) );
}


