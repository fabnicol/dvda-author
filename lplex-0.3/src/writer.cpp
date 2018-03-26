/*
	writer.cpp - wave, flac and raw lpcm output processing.
	Copyright (C) 2006-2011 Bahman Negahban

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


using namespace std;
#include "processor.hpp"




// ----------------------------------------------------------------------------
//    lpcmEntity::soundMatch :
// ----------------------------------------------------------------------------
//    Returns whether audio characteristics of <a> matches <b>.
// ----------------------------------------------------------------------------


bool lpcmEntity::soundMatch( lpcmEntity *a, lpcmEntity *b, char* errmsg )
{
	uint16_t bpsA = a->fmeta.data.stream_info.bits_per_sample,
		khzA = a->fmeta.data.stream_info.sample_rate / 1000,
		chA = a->fmeta.data.stream_info.channels,
		bpsB = b->fmeta.data.stream_info.bits_per_sample,
		khzB = b->fmeta.data.stream_info.sample_rate / 1000,
		chB = b->fmeta.data.stream_info.channels;

	bool bpsOk = ( bpsA == bpsB ), khzOk = ( khzA == khzB ), chOk = ( chA == chB );

	if( ! bpsOk || ! khzOk || ! chOk )
	{
#if 0
			ERR( "Audio mismatch ("
//         << ( bpsOk ? "" : _f( "%dbit ", bpsB ) )
//         << ( khzOk ? "" : _f( "%dkhz ", khzB ) )
//         << ( chOk  ? "" : _f( "%dch ",  chB ) )
//         << "to"
//         << ( bpsOk ? "" : _f( " %dbit", bpsA ) )
//         << ( khzOk ? "" : _f( " %dkhz", khzA ) )
//         << ( chOk  ? "" : _f( " %dch",  chA ) )
			<< _f( "%dbit %dkhz %dch", bpsA, khzA, chA )
            << ") at \'" << a->fName.filename() << "\'.\n" );
#else
        if(! errmsg)
            errmsg = (char*) _f( "Audio mismatch (%dbit %dkhz %dch) at '%s'",
                bpsA, khzA, chA, a->fName.filename().c_str() ).c_str();
#endif

		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
//    lpcmEntity::audioInfo :
// ----------------------------------------------------------------------------
//    Returns string describing audio characteristics of given descriptor <l>.
// ----------------------------------------------------------------------------


string lpcmEntity::audioInfo( lpcmEntity *l )
{
	return _f( "%d bit %d khz %d ch",
		l->fmeta.data.stream_info.bits_per_sample,
		l->fmeta.data.stream_info.sample_rate / 1000,
		l->fmeta.data.stream_info.channels );
}

string lpcmEntity::audioInfo( FLAC__StreamMetadata *fmeta )
{
	return _f( "%d bit %d khz %d ch",
		fmeta->data.stream_info.bits_per_sample,
		fmeta->data.stream_info.sample_rate / 1000,
		fmeta->data.stream_info.channels );
}



// ----------------------------------------------------------------------------
//    lpcmEntity::soundCheck :
// ----------------------------------------------------------------------------
//    Checks whether lpcm characteristics of given descriptor are legal
//    in dvd-video and supported by Lplex.
//
//    Arguments:
//       <l>      - lpcm descriptor
//       <mute>   - whether to suppress stderr success messages
//
//    Returns true on success, false on fail
// ----------------------------------------------------------------------------


bool lpcmEntity::soundCheck( lpcmEntity *l, bool mute )
{
	uint16_t bpsOk=0, khzOk=0, chOk=1, notAllowed=0;
	uint16_t bps = l->fmeta.data.stream_info.bits_per_sample,
		khz = l->fmeta.data.stream_info.sample_rate / 1000,
		ch = l->fmeta.data.stream_info.channels;

	uint8_t maxDvdChans[][2] =
	{// 48    96 khz
		{ 8,    2 },   // 16
//    { 6,    2 },   // 20
		{ 2,    2 },   // 20
		{ 6,    2 }    // 24 bit
	};

	if( ! mute )
		LOG( _f( "%s%d bit %d khz %d ch\n", _affirm.c_str(), bps, khz, ch ) );

	switch( bps ) { case 16: case 24: bpsOk = 1; case 20: break; default: notAllowed = 1; }
	switch( khz ) { case 48: case 96: khzOk = 1; break; default: notAllowed = 1; }

	if( bpsOk && khzOk )
		switch( ch )
		{
			case  1: case  2: case 6: case 8:
#if 0
			case  4:
#endif
				if( ch <= maxDvdChans[( bps-16)/4][(khz/48)-1] ) break;
				else bpsOk = khzOk = 0;
			default: chOk = 0; notAllowed = 1;
		}

	if( ! bpsOk || ! khzOk || ! chOk )
	{
		if( ! mute )
		{
            ERR(  ( bpsOk ? string("") : _f( "%d bit ", bps ) )
                + ( khzOk ? string("") : _f( "%d khz ", khz ) )
                + ( chOk  ? string("") : _f( "%d ch ",  ch ) )
                + string("lpcm is not ") + ( notAllowed ? "allowed in dvd-video\n" : "supported\n" ) );
		}

		clearbits( l->state, specified );
		return false;
	}

	l->state |= specified;
	return true;
}


// ----------------------------------------------------------------------------
//    lpcmProcessor::md5compare :
// ----------------------------------------------------------------------------
//    Compares given md5 signature to internal md5 signature.
//
//    Arguments:
//       <md5>    - signature for comparison
//       <prefix> - string to prefix to stderr messages.
//
//    Returns true if identical or if internal copy is blank, false if not
// ----------------------------------------------------------------------------



bool lpcmProcessor::md5compare( const void *md5, const char * prefix )
{
	bool r = true;

	if( memcmp( md5, fmeta.data.stream_info.md5sum, 16 ) )
	{
		if( otherThan( 0, fmeta.data.stream_info.md5sum, 16 ) )
		{
			WARN( prefix << "md5 FAILED" );
			r = false;
		}
		else
			INFO( prefix << "md5" );
	}
	else
		INFO( prefix << "md5 ok" );

    ECHO( " : " << hexStr( md5, 16 ) << " : " << fName.stem() << endl );
	if( ! r )
		LOG( "           : " << hexStr( fmeta.data.stream_info.md5sum, 16 )
			<< " : (original)" << endl );

	return r;
}


#if 0 // testing Lee Feldkamp's permutation() framework
#define WRITER_CPP
#include "multichannel.cpp"
#else

// ----------------------------------------------------------------------------
//    lpcmWriter::swap2wav :
// ----------------------------------------------------------------------------
//    Swaps given lpcm data buffer from dvd to wave byte-order.
//
//    Arguments:
//       <data>            - pointer to start of wave data buffer
//       <count>           - number of bytes to process
//       <channels>        - number of audio channels
//       <bitspersample>   - audio quantization
//
//    Returns 0 on success
// ----------------------------------------------------------------------------
// (adapted from Dave Chapman's dvda-author::audio.c::audio_read())


int lpcmWriter::swap2wav( unsigned char *data, uint32_t count,
	int channels, int bitspersample )
{
	uint32_t i;
	int x;
	// Convert big-endian MPEG LPCM samples to little-endian data samples
	if( bitspersample == 16 )
	{
		for( i=0; i < count; i+=2 )
		{
			x = data[i+1];
			data[i+1] = data[i];
			data[i] = x;
		}
	}

	else if( bitspersample == 24 )
	{
		if( channels == 1 )
		{
			/* 24-bit mono samples are packed as follows:

					0  1  2  3  4  5
			DVD: 45 23 56 34 01 12
			WAV: 01 23 45 12 34 56

			*/
			for( i=0; i < count; i+=6 )
			{
				x = data[i];
				data[i] = data[i+4];
				data[i+4] = data[i+3];
				data[i+3] = data[i+5];
				data[i+5] = data[i+2];
				data[i+2] = x;
			}
		}
		else /*if( channels == 2 )*/
		{
			/* 24-bit Stereo samples are packed as follows:

					0  1  2  3  4  5  6  7  8  9 10 11
			DVD: 45 23 8c 60 ab 89 e3 d4 01 bf 67 b7
			WAV: 01 23 45 bf 60 8c 67 89 ab b7 d4 e3

			*/

			for( i=0; i < count; i+=12 )
			{
				x = data[i];
				data[i] = data[i+8];
				data[i+8] = data[i+4];
				data[i+4] = data[i+3];
				data[i+3] = data[i+9];
				data[i+9] = data[i+11];
				data[i+11] = data[i+6];
				data[i+6] = data[i+10];
				data[i+10] = data[i+7];
				data[i+7] = data[i+5];
				data[i+5] = data[i+2];
				data[i+2] = x;
			}
		}
	}

	return 0;
}

#endif


// ----------------------------------------------------------------------------
//    rawWriter::open :
// ----------------------------------------------------------------------------
//    Opens a wav output file, writes a canonical header, and initializes the
//    md5 signature and block alignment.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t rawWriter::open()
{
	if( rawFile.is_open() )
		rawFile.close();

    fName = fName.generic_string() + ".lpcm";
    rawFile.open( fName.generic_string(), ios::binary );

	if( ! rawFile.is_open() )
        FATAL( "Can't open output file " + fName.generic_string() );

	md5_init( &md5 );
	md5_init( &md5raw );


	interSamp = fmeta.data.stream_info.channels *
		fmeta.data.stream_info.bits_per_sample / 4;

	return 0;
}

// ----------------------------------------------------------------------------
//    rawWriter::process :
// ----------------------------------------------------------------------------
//    Writes given dvd-order lpcm data to output file, then swaps to wave-order
//    before updating md5 signature.
//
//    Arguments:
//       <buf>    - pointer to start of lpcm data data buffer
//       <size>   - number of bytes to process
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint32_t rawWriter::process( unsigned char *buf, uint32_t size/*, bool swap*/ )
{
	if( ! ( state & specified ) )
		return 0;

	unsent = size;
	if( state & metered && ct.max && size > ct.max - ct.now )
	{
		size = ct.max - ct.now;
//      uint16_t diff;
//      if( ( diff = size % interSamp ) != 0  )
//         size -= diff;
	}
	unsent -= size;

//   assert( ( size % interSamp ) == 0 );

	ct.now += size;

	rawFile.write( (char *)buf, size );
	md5_append( &md5raw, (unsigned char*)buf, size );


	swap2wav( buf, size, fmeta.data.stream_info.channels,
		fmeta.data.stream_info.bits_per_sample );
	md5_append( &md5, (unsigned char*)buf, size );

	blip( &ct, 20, _verbose ? "done" : "", STAT_TAG );
	return 0;
}



// ----------------------------------------------------------------------------
//    rawWriter::close :
// ----------------------------------------------------------------------------
//    Closes output file, and checks wave-order md5 signature.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t rawWriter::close()
{
	rawFile.close();
	md5_finish( &md5raw, md5strRaw );
	md5_finish( &md5, md5str );
}



// ----------------------------------------------------------------------------
//    rawWriter::md5Report :
// ----------------------------------------------------------------------------
//    Compares wave-order, then reports both wave and dvd-order md5 signatures.
//
//    Returns true if writer is closed, false if still open
// ----------------------------------------------------------------------------


uint16_t rawWriter::md5Report()
{
	if( isOpen() )
		return false;

	md5compare( md5str, "Wave order " );
	LOG( "-dvd order md5" << ( ct.max ? "    : " : " : " )
		<< hexStr( md5strRaw, 16 ) << " : (raw)" << endl );

	return true;
}



// ----------------------------------------------------------------------------
//    waveWriter::open :
// ----------------------------------------------------------------------------
//    Opens a wav output file, writes a canonical header, and initializes the
//    md5 signature and block alignment.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t waveWriter::open()
{
	if( waveFile.is_open() )
		waveFile.close();

    fName = fName.generic_string() + ".wav";
    waveFile.open( fName.generic_string(), ios::binary );

	if( ! waveFile.is_open() )
        FATAL( "Can't open output file " + fName.generic_string() );

	waveHeader::tag( waveFile, &fmeta );
	md5_init( &md5 );

	interSamp = fmeta.data.stream_info.bits_per_sample *
		fmeta.data.stream_info.channels / 8;

	return 0;
}



// ----------------------------------------------------------------------------
//    waveWriter::process :
// ----------------------------------------------------------------------------
//    Updates md5 signature, and writes given wave-order data to output file.
//
//    Arguments:
//       <buf>             - pointer to start of lpcm data data buffer
//       <size>            - number of bytes to process
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint32_t waveWriter::process( unsigned char *buf, uint32_t size )
{
	if( ! ( state & specified ) )
		return 0;

	unsent = size;
	if( state & metered && ct.max && size > ct.max - ct.now )
		size = ct.max - ct.now;
	unsent -= size;

	assert( ( size % interSamp ) == 0 );

	ct.now += size;

	md5_append( &md5, (unsigned char*)buf, size );
	waveFile.write( (char *)buf, size );

	blip( &ct, 20, _verbose ? "done" : "", STAT_TAG );
	return 0;
}



// ----------------------------------------------------------------------------
//    waveWriter::close :
// ----------------------------------------------------------------------------
//    Updates wave header, closes output file, and checks md5 signature.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t waveWriter::close()
{
	waveHeader::tag( waveFile, &fmeta );
	waveFile.close();
	md5_finish( &md5, md5str );
	return 0;
}



// ----------------------------------------------------------------------------
//    waveWriter::md5Report :
// ----------------------------------------------------------------------------
//    Compares md5 signature and reports.
//
//    Returns true if writer is closed, false if still open
// ----------------------------------------------------------------------------


uint16_t waveWriter::md5Report()
{
	if( isOpen() )
		return false;

	md5compare( md5str );
	return true;
}



// ----------------------------------------------------------------------------
//    flacWriter::open :
// ----------------------------------------------------------------------------
//    Initializes the flac encoder, writes metadata (header) blocks.
//
//    Returns 0 on success, -1 on failure
// ----------------------------------------------------------------------------


uint16_t flacWriter::open()
{
	uint16_t m=0;
	int r=0;
	FLAC__StreamMetadata* metadata[4];
	FLAC__StreamMetadata* padding;

	if( is_valid() )
		finish();

	interSamp = fmeta.data.stream_info.channels *
		fmeta.data.stream_info.bits_per_sample / 8;

    fName = fName.generic_string() + ".flac";

#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
    set_filename( fName.generic_string() );
#endif
	set_channels( fmeta.data.stream_info.channels );
	set_bits_per_sample( fmeta.data.stream_info.bits_per_sample );
	set_sample_rate( fmeta.data.stream_info.sample_rate );
//   set_total_samples_estimate( fmeta.data.stream_info.total_samples );


	uint16_t settings[10][9] = {

//    0     1     2     3     4     5     6     7     8      // compression level
	{ 1152, 1152, 1152, 4608, 4608, 4608, 4608, 4608, 4608 }, // blocksize
	{  0,    0,    0,    0,    0,    0,    0,    1,    1   }, // do_exhaustive_model_search
	{  0,    0,    0,    0,    0,    0,    0,    0,    0   }, // do_escape_coding
	{  0,    1,    1,    0,    1,    1,    1,    1,    1   }, // do_mid_side_stereo
	{  0,    1,    0,    0,    1,    0,    0,    0,    0   }, // loose_mid_side_stereo
	{  0,    0,    0,    0,    0,    0,    0,    0,    0   }, // qlp_coeff_precision
	{  2,    2,    0,    3,    3,    3,    0,    0,    0   }, // min_residual_partition_order
	{  2,    2,    3,    3,    3,    3,    4,    6,    6   }, // max_residual_partition_order
	{  0,    0,    0,    0,    0,    0,    0,    0,    0   }, // rice_parameter_search_dist
	{  0,    0,    0,    6,    8,    8,    8,    8,    12  }  // max_lpc_order
	};

	set_blocksize( settings[0][level] );
	set_do_exhaustive_model_search( settings[1][level] );
	set_do_escape_coding( settings[2][level] );
	if( fmeta.data.stream_info.channels == 2 )
	{
		set_do_mid_side_stereo( settings[3][level] );
		set_loose_mid_side_stereo( settings[4][level] );
	}
	set_qlp_coeff_precision( settings[5][level] );
	set_min_residual_partition_order( settings[6][level] );
	set_max_residual_partition_order( settings[7][level] );
	set_rice_parameter_search_dist( settings[8][level] );
	set_max_lpc_order( settings[9][level] );


	if( pad )
	{
		padding = FLAC__metadata_object_new( FLAC__METADATA_TYPE_PADDING );
		padding->length = pad;
		metadata[m++] = padding;
	}

	set_metadata( metadata, m );

	int err;
#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7
	if( ( err = init() ) != FLAC__FILE_ENCODER_OK )
#else // flac 1.1.3+
    if( ( err = init( fName.generic_string() ) ) != FLAC__STREAM_ENCODER_INIT_STATUS_OK )
#endif
	{
		showState( err, "initialization" );
		r = -1;
	}

	for( int i=0; i < m; i++ )
		FLAC__metadata_object_delete( metadata[i] );

	return r;
}



// ----------------------------------------------------------------------------
//    flacWriter::process :
// ----------------------------------------------------------------------------
//    Transfers given wave-order lpcm data to internal 32bit-aligned channel-
//    interleaved sample array and submits it to the flac encoder.
//
//    Arguments:
//       <uBuf>   - pointer to start of dvd-ordered data buffer
//       <size>   - number of bytes to process (must be aligned to one
//                  interchannel sample)
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint32_t flacWriter::process( unsigned char *uBuf, uint32_t size )
{
	if( ! ( state & specified ) )
		return 0;

	unsent = size;
	if( state & metered && ct.max && size > ct.max - ct.now )
		size = ct.max - ct.now;
	unsent -= size;

	assert( ( size % interSamp ) == 0 );

	if( fmeta.data.stream_info.bits_per_sample == 16 )
	{
		FLAC__int16 *ssBuf = (FLAC__int16 *)uBuf;
		for( uint32_t f=0; f < size / 2; f++ )
			samples[f] = (FLAC__int32)ssBuf[f];
	}

	else if( fmeta.data.stream_info.bits_per_sample == 24 )
	{
		signed char *sBuf = (signed char *)uBuf;
		for( uint32_t f=0, i=0; i < size; f++, i+=3 )
		{
			samples[f]  = sBuf[i+2]; samples[f] <<= 8;
			samples[f] |= uBuf[i+1]; samples[f] <<= 8;
			samples[f] |= uBuf[i];
		}
	}

	if( ! process_interleaved( samples, size / interSamp ) )
		showState( 0, "processing" );

	blip( &ct, 20, _verbose ? "done" : "", STAT_TAG );

	ct.now += size;
	return 0;
}




// ----------------------------------------------------------------------------
//    flacWriter::close :
// ----------------------------------------------------------------------------
//    Finishes encoder, reads in and checks md5 signature.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


uint16_t flacWriter::close()
{
	finish();
	return 0;
}


// ----------------------------------------------------------------------------
//    flacWriter::md5Report :
// ----------------------------------------------------------------------------
//    Compares md5 signature and reports.
//
//    Returns true if writer is closed, false if still open
// ----------------------------------------------------------------------------


uint16_t flacWriter::md5Report()
{
	uint8_t h[42];
	::FLAC__StreamMetadata meta;

	if( ! isOpen() )
		return false;

    ifstream flacFile( fName.generic_string(), ios::binary );
	if( ! flacFile.is_open() )
	{
        ERR( "Can't open input file \'" + fName.generic_string() + "\'\n" );
		return 1;
	}
	flacFile.seekg( 0 );
	flacFile.read( (char *) h, 42 );
	flacFile.close();

	flacHeader::readStreamInfo( h + 8, &meta );
	md5compare( meta.data.stream_info.md5sum );
	return true;
}



// ----------------------------------------------------------------------------
//    flacWriter::swap2flac :
// ----------------------------------------------------------------------------
//    Swaps and sorts given lpcm data buffer from dvd byte-order into separate
//    consecutive single-channel 32bit-aligned sample arrays.
//
//    Note: caller is responsible for overflow management.
//
//    Arguments:
//       <flac>            - pointer to start of destination flac sample buffer
//       <dvd>             - pointer to start of source dvd data buffer
//       <dvdCount>        - number of dvd bytes to process
//       <channels>        - number of audio channels
//       <bitspersample>   - audio quantization
//
//    Returns 0 on success
// ----------------------------------------------------------------------------

int flacWriter::swap2flac( FLAC__int32 *flac, unsigned char *udvd,
	uint32_t dvdCount, int channels, int bitspersample )
{
	uint16_t sampsPerChan = dvdCount / (channels * bitspersample / 8);
	signed char *sdvd = (signed char *)udvd;

	if( bitspersample == 16 )
	{
		for( uint32_t i=0, s=0; i < dvdCount; i+=2, s++ )
		{
			flac[s]  = sdvd[ i ]; flac[s] <<= 8;
			flac[s] |= udvd[i+1]; flac[s] <<= 16;
		}
	}

	else if( bitspersample == 20 )
	{
#define HI(b)   ((b >> 4) & 0x0f)
#define LO(b)   (b & 0x0f)

		if( channels == 1 )
		{
			for( uint32_t i=0, m=0; i < dvdCount; i+=5 )
			{
				flac[m]  = sdvd[ i ]; flac[m] <<= 8;
				flac[m] |= udvd[i+1]; flac[m] <<= 4;
				flac[m] |= HI(udvd[i+4]);
				m++;
				flac[m]  = sdvd[i+2]; flac[m] <<= 8;
				flac[m] |= udvd[i+3]; flac[m] <<= 4;
				flac[m] |= LO(udvd[i+4]);
				m++;
			}
		}
		else if( channels == 2 )
		{
			for( uint32_t i=0, l=0, r = sampsPerChan; i < dvdCount; i+=10 )
			{
				flac[l]  = sdvd[ i ]; flac[l] <<= 8;
				flac[l] |= udvd[i+1]; flac[l] <<= 4;
				flac[l] |= HI(udvd[i+8]);
				l++;
				flac[l]  = sdvd[i+4]; flac[l] <<= 8;
				flac[l] |= udvd[i+5]; flac[l] <<= 4;
				flac[l] |= HI(udvd[i+9]);
				l++;
				flac[r]  = sdvd[i+2]; flac[r] <<= 8;
				flac[r] |= udvd[i+3]; flac[r] <<= 8;
				flac[l] |= LO(udvd[i+8]);
				r++;
				flac[r]  = sdvd[i+6]; flac[r] <<= 8;
				flac[r] |= udvd[i+7]; flac[r] <<= 8;
				flac[l] |= LO(udvd[i+9]);
				r++;
			}
		}
	}

	else if( bitspersample == 24 )
	{
		if( channels == 1 )
		{
//  Packing order of 24-bit mono lpcm samples:
//
//     *DVD  : 00  01 . 02  03 . 04  05  :
//           :   S0   .   S1   . S0  S1  :
//
//    **WAV  : 04  01  00  : 05  03  02  :
//   **FLAC  : 04 01 00 __ : 05 03 02 __ :
//           :     S0      :     S1      :
//
//    *big-endian / 48bit block-aligned (block = 2 samples)
//   **little-endian / 24bit-aligned
//  ***big-endian / 32bit-aligned

			for( uint32_t i=0, m=0; i < dvdCount; i+=6 )
			{
				flac[m]  = sdvd[ i ]; flac[m] <<= 8;
				flac[m] |= udvd[i+1]; flac[m] <<= 8;
				flac[m] |= udvd[i+4];
				m++;
				flac[m]  = sdvd[i+2]; flac[m] <<= 8;
				flac[m] |= udvd[i+3]; flac[m] <<= 8;
				flac[m] |= udvd[i+5];
				m++;
			}
		}
		else
		{

//  Packing order of 24-bit stereo lpcm samples:
//
//     *DVD  :  00  01 : 02  03 : 04  05 : 06  07 : 08  09 : 10  11  :
//           :    L0   .   R0   .   L1   .   R1   . L0  R0 . L1  R1  : ...block n
//
//    **WAV  : 08  01  00  : 09  03  02  : 10  05  04  : 11  07  06  :
//           :     L0      :     R0      :     L1      :     R1      : ...Ln,Rn
//
//  ***FLAC  : 08 01 00 __ : 10 05 04 __ | 09 03 02 __ : 11 07 06 __ |
//           :      L0     :   L1   ...Ln|      R0     :   R1   ...Rn|
//
//    *big-endian / 96bit block-aligned (block=2 interchannel samples)
//   **little-endian / 24bit-aligned / channel-interleaved
//  ***big-endian / 32bit-aligned / channel-separated

			for( uint32_t i=0, l=0, r = sampsPerChan; i < dvdCount; i+=12 )
			{
				flac[l]  = sdvd[ i ]; flac[l] <<= 8;
				flac[l] |= udvd[i+1]; flac[l] <<= 8;
				flac[l] |= udvd[i+8];
				l++;
				flac[l]  = sdvd[i+4]; flac[l] <<= 8;
				flac[l] |= udvd[i+5]; flac[l] <<= 8;
				flac[l] |= udvd[i+10];
				l++;
				flac[r]  = sdvd[i+2]; flac[r] <<= 8;
				flac[r] |= udvd[i+3]; flac[r] <<= 8;
				flac[r] |= udvd[i+9];
				r++;
				flac[r]  = sdvd[i+6]; flac[r] <<= 8;
				flac[r] |= udvd[i+7]; flac[r] <<= 8;
				flac[r] |= udvd[i+11];
				r++;
			}
		}
	}
	else
	{
		// FIX: Handle 20-bit audio and maybe convert other formats.
        ERR( to_string(bitspersample) + "bit audio is not supported\n" );
	}
	return 0;
}



// ----------------------------------------------------------------------------
//    flacWriter::showState :
// ----------------------------------------------------------------------------
//    Displays <message>, then flac encoder status flags
//    (codes are in FLAC/stream_encoder.h).
// ----------------------------------------------------------------------------


void flacWriter::showState( int initcode, const char *msg )
{
	ERR( _f( "flac %s API %s error:\n", FLAC__VERSION_STRING, msg ? msg : "" ) );

#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT <= 7

	switch( get_state() )
	{
		case FLAC__FILE_ENCODER_OK : break;
		case FLAC__FILE_ENCODER_NO_FILENAME : LOG( "FILE_ENCODER_NO_FILENAME\n" ); break;
		case FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR : LOG( "FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR\n" ); break;
		case FLAC__FILE_ENCODER_FATAL_ERROR_WHILE_WRITING : LOG( "FILE_ENCODER_FATAL_ERROR_WHILE_WRITING\n" ); break;
		case FLAC__FILE_ENCODER_ERROR_OPENING_FILE : LOG( "FILE_ENCODER_ERROR_OPENING_FILE\n" ); break;
		case FLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR : LOG( "FILE_ENCODER_MEMORY_ALLOCATION_ERROR\n" ); break;
		case FLAC__FILE_ENCODER_ALREADY_INITIALIZED : LOG( "FILE_ENCODER_ALREADY_INITIALIZED\n" ); break;
		case FLAC__FILE_ENCODER_UNINITIALIZED : LOG( "FILE_ENCODER_UNINITIALIZED\n" ); break;
	}

	switch( get_seekable_stream_encoder_state() )
	{
		case FLAC__SEEKABLE_STREAM_ENCODER_OK: break;
		case FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR: LOG( "SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR: LOG( "SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR: LOG( "SEEKABLE_STREAM_ENCODER_WRITE_ERROR\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR: LOG( "SEEKABLE_STREAM_ENCODER_READ_ERROR\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR: LOG( "SEEKABLE_STREAM_ENCODER_SEEK_ERROR\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR: LOG( "SEEKABLE_STREAM_ENCODER_TELL_ERROR\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED: LOG( "SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK: LOG( "SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE: LOG( "SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE\n" ); break;
		case FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED: LOG( "SEEKABLE_STREAM_ENCODER_UNINITIALIZED\n" ); break;
	}

	switch( get_stream_encoder_state() )
	{
		case FLAC__STREAM_ENCODER_OK : break;
		case FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR : LOG( "STREAM_ENCODER_VERIFY_DECODER_ERROR\n" ); break;
		case FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA : LOG( "STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_CALLBACK : LOG( "STREAM_ENCODER_INVALID_CALLBACK\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_NUMBER_OF_CHANNELS : LOG( "STREAM_ENCODER_INVALID_NUMBER_OF_CHANNELS\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_BITS_PER_SAMPLE : LOG( "STREAM_ENCODER_INVALID_BITS_PER_SAMPLE\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_SAMPLE_RATE : LOG( "STREAM_ENCODER_INVALID_SAMPLE_RATE\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_BLOCK_SIZE : LOG( "STREAM_ENCODER_INVALID_BLOCK_SIZE\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_MAX_LPC_ORDER : LOG( "STREAM_ENCODER_INVALID_MAX_LPC_ORDER\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_QLP_COEFF_PRECISION : LOG( "STREAM_ENCODER_INVALID_QLP_COEFF_PRECISION\n" ); break;
		case FLAC__STREAM_ENCODER_MID_SIDE_CHANNELS_MISMATCH : LOG( "STREAM_ENCODER_MID_SIDE_CHANNELS_MISMATCH\n" ); break;
		case FLAC__STREAM_ENCODER_MID_SIDE_SAMPLE_SIZE_MISMATCH : LOG( "STREAM_ENCODER_MID_SIDE_SAMPLE_SIZE_MISMATCH\n" ); break;
		case FLAC__STREAM_ENCODER_ILLEGAL_MID_SIDE_FORCE : LOG( "STREAM_ENCODER_ILLEGAL_MID_SIDE_FORCE\n" ); break;
		case FLAC__STREAM_ENCODER_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER : LOG( "STREAM_ENCODER_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER\n" ); break;
		case FLAC__STREAM_ENCODER_NOT_STREAMABLE : LOG( "STREAM_ENCODER_NOT_STREAMABLE\n" ); break;
		case FLAC__STREAM_ENCODER_FRAMING_ERROR : LOG( "STREAM_ENCODER_FRAMING_ERROR\n" ); break;
		case FLAC__STREAM_ENCODER_INVALID_METADATA : LOG( "STREAM_ENCODER_INVALID_METADATA\n" ); break;
		case FLAC__STREAM_ENCODER_FATAL_ERROR_WHILE_ENCODING : LOG( "STREAM_ENCODER_FATAL_ERROR_WHILE_ENCODING\n" ); break;
		case FLAC__STREAM_ENCODER_FATAL_ERROR_WHILE_WRITING : LOG( "STREAM_ENCODER_FATAL_ERROR_WHILE_WRITING\n" ); break;
		case FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR : LOG( "STREAM_ENCODER_MEMORY_ALLOCATION_ERROR\n" ); break;
		case FLAC__STREAM_ENCODER_ALREADY_INITIALIZED : LOG( "STREAM_ENCODER_ALREADY_INITIALIZED\n" ); break;
	}
/*
	switch( get_verify_decoder_state() )
	{
		case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA : LOG( "STREAM_DECODER_SEARCH_FOR_METADATA\n" ); break;
		case FLAC__STREAM_DECODER_READ_METADATA : LOG( "STREAM_DECODER_READ_METADATA\n" ); break;
		case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC : LOG( "STREAM_DECODER_SEARCH_FOR_FRAME_SYNC\n" ); break;
		case FLAC__STREAM_DECODER_READ_FRAME : LOG( "STREAM_DECODER_READ_FRAME\n" ); break;
		case FLAC__STREAM_DECODER_END_OF_STREAM : LOG( "STREAM_DECODER_END_OF_STREAM\n" ); break;
		case FLAC__STREAM_DECODER_ABORTED : LOG( "STREAM_DECODER_ABORTED\n" ); break;
		case FLAC__STREAM_DECODER_UNPARSEABLE_STREAM : LOG( "STREAM_DECODER_UNPARSEABLE_STREAM\n" ); break;
		case FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR : LOG( "STREAM_DECODER_MEMORY_ALLOCATION_ERROR\n" ); break;
		case FLAC__STREAM_DECODER_ALREADY_INITIALIZED : LOG( "STREAM_DECODER_ALREADY_INITIALIZED\n" ); break;
		case FLAC__STREAM_DECODER_INVALID_CALLBACK : LOG( "STREAM_DECODER_INVALID_CALLBACK\n" ); break;
		case FLAC__STREAM_DECODER_UNINITIALIZED : LOG( "STREAM_DECODER_UNINITIALIZED\n" ); break;
	}
*/
#else // flac 1.1.3+

	if( initcode )
		LOG( _f( " %2d: '%s'\n", initcode, (char*[]){
				"FLAC__STREAM_ENCODER_INIT_STATUS_OK",
				"FLAC__STREAM_ENCODER_INIT_STATUS_ENCODER_ERROR",
				"FLAC__STREAM_ENCODER_INIT_STATUS_UNSUPPORTED_CONTAINER",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_CALLBACKS",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_NUMBER_OF_CHANNELS",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_BITS_PER_SAMPLE",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_SAMPLE_RATE",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_BLOCK_SIZE",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_MAX_LPC_ORDER",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_QLP_COEFF_PRECISION",
				"FLAC__STREAM_ENCODER_INIT_STATUS_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER",
				"FLAC__STREAM_ENCODER_INIT_STATUS_NOT_STREAMABLE",
				"FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_METADATA",
				"FLAC__STREAM_ENCODER_INIT_STATUS_ALREADY_INITIALIZED"
			} [initcode] ) );

	FLAC__StreamEncoderState status = get_state();
	LOG( _f( " %2d: '%s'\n", status, (char*[]){
			"FLAC__STREAM_ENCODER_OK",
			"FLAC__STREAM_ENCODER_UNINITIALIZED",
			"FLAC__STREAM_ENCODER_OGG_ERROR",
			"FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR",
			"FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA",
			"FLAC__STREAM_ENCODER_CLIENT_ERROR",
			"FLAC__STREAM_ENCODER_IO_ERROR",
			"FLAC__STREAM_ENCODER_FRAMING_ERROR",
			"FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR"
		} [status] ) );

#endif

}
