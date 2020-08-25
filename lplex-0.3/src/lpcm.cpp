/*
	lpcm.cpp - general lpcm descriptors, dvd-v and wave header utilities.
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



#include "lpcm.hpp"
#include "processor.hpp"



// ----------------------------------------------------------------------------
//    PES_packet::payload :
// ----------------------------------------------------------------------------
//    Determines extent of lpcm payload for given Private Stream 1 PES packet,
//    and depending on context of mode flag, swaps in-place to wave-order and
//    limits range from/to given pts boundary.
//
//    Arguments:
//       <PS1>          - pointer to PES header
//       <audio>        - structure to report payload extents
//       <mode>         - context of current call
//       <ptsBoundary>  - pts start or end boundary
//
//    Returns current mode
// ----------------------------------------------------------------------------


uint16_t PES_packet::payload( PES_packet::header* PS1,
	byteRange *audio, uint8_t mode, uint64_t ptsBoundary )
{
	static uint8_t orphanage[11];
	static uint16_t orphans, sampleSize = 1, bps, ch;
	uint16_t i, o;
	uint8_t * seam = nullptr;

	audio->start = dataAddr( PS1 );
	audio->len = dataLen( PS1 ) + 1;

	if ( mode & raw ) //no swap
	{
//      clearbits( mode, swap );
		return mode;
	}

	static bool inBounds = false;
	uint32_t framesToGo = 0;

	if( ptsBoundary )
	{
		if( ( framesToGo = ( ptsBoundary - readpts( PS1 ) ) / 150 ) <
			lpcmAddr( PS1 )->frames )
		{
			seam = firstFrame( lpcmAddr( PS1 ) )
				+ framesToGo * bytesPerFrame( PS1 );

			if ( mode & start )
			{
				clearbits( mode, unfinished );
				inBounds = true;
			}

			else
			{
				mode |= end;
			}
		}

		else
		{
			clearbits( mode, end );
		}
	}

	else if( mode & start )
	{
		clearbits( mode, unfinished );
		inBounds = true;
		seam = dataAddr( PS1 );
	}

	else
	{
		if( ! ( mode & end ) )
			inBounds = true;
		seam = dataAddr( PS1 ) + dataLen( PS1 ) + 1;
	}

	if( mode & start )
	{
		bps = bitsPerSample( PS1 );
		ch = channels( PS1 );

		audio->start = seam;
		audio->len -= ( audio->start - dataAddr( PS1 ) );
		sampleSize = 2 * bps * ch / 8;
		orphans = 0;
		mode ^= start;
	}

	else if( mode & flush ) //raw, no swap
	{
		if( mode & adopt )
		{
			audio->start = orphanage;
			audio->len = orphans;
		}
		sampleSize = 1;
		orphans = 0;
		mode = 0;
		return 0;
	}

	if( mode & end )
	{
		audio->len = seam - audio->start;
//      mode ^= end;
	}

	if( mode & adopt )
	{
		audio->start -= orphans;
		audio->len += orphans;
								// caution: header is trashed after this...
		for( i=0; i < orphans; i++ )
			audio->start[i] = orphanage[i];
		mode ^= adopt;
	}

	if( (orphans = ( audio->len % sampleSize )) )
	{
		audio->len -= orphans;
		for( o=0; o < orphans; o++ )
			orphanage[o] = audio->start[audio->len + o];
		mode |= adopt;
	}

	if( mode & swap && inBounds )
	{
		lpcmWriter::swap2wav( audio->start, audio->len, ch, bps );
	}

	if( mode & end )
	{
		inBounds = false;
		if( seam < ( dataAddr( PS1 ) + dataLen( PS1 ) ) )
		{
			mode |= unfinished;
		}
	}


	return mode;
}



// ----------------------------------------------------------------------------
//    PES_packet::readpts :
// ----------------------------------------------------------------------------
//    Bitfield reader for PTS DTS location in PES header Data area.
//
//    Arguments:
//       <buf>    - pointer to read location
//
//    Returns pts
// ----------------------------------------------------------------------------
// (more or less verbatim from dvdauthor::mpeg2desc.c)


uint64_t PES_packet::readpts( uint8_t* buf )
{
	uint64_t b1, b2, b3;

	b1 = ( buf[0] & 0xf ) >> 1;
	b2 = ( ( buf[1] << 8 ) | buf[2] ) >> 1;
	b3 = ( ( buf[3] << 8 ) | buf[4] ) >> 1;

	return ( (uint64_t) b1 << 30 ) | ( b2 << 15 ) | b3;
}



// ----------------------------------------------------------------------------
//    PES_packet::display :
// ----------------------------------------------------------------------------
//    stderr display function for PES header info, 3 variants...
// ----------------------------------------------------------------------------


void PES_packet::display(PES_packet::header* h)
{

	uint32_t pts = readpts( h );

	cerr <<
		"startCode   :" << hex << bEndian(h->startCode) << dec << endl <<
		"packetLen   :" << bEndian(h->packetLen) << endl <<
		"flags1      :" << hex << (short) h->flags1 << endl <<
		"flags2      :" << (short) h->flags2 << dec << endl <<
		"headerLen   :" << (short) h->headerLen << endl <<
		"pts         :" << pts <<
			" (" << pts/PTSTIME << "." << (pts%PTSTIME)/(PTSTIME/1000) << ")"<< endl << endl;
}


void PES_packet::display(PES_packet::LPCM_header* h)
{

	cerr <<
		"streamID    :" << hex << (short) h->streamID << dec << endl <<
		"frames      :" << (short) h->frames << endl <<
		"dataPointer :" << bEndian(h->dataPointer) << endl <<
		"flags1      :" << hex << (short) h->flags1 << endl <<
		"flags2      :" << (short) h->flags2 << dec << endl <<
		"dynamicRange:" << (short) h->dynamicRange << endl << endl;
}


void PES_packet::displayFlags(PES_packet::LPCM_header* h)
{
	cerr <<
		"quantization:" << quantization(h) << endl <<
		"frequency   :" << frequency(h) << endl <<
		"channels    :" << channels(h) << endl;
}



// ----------------------------------------------------------------------------
//    waveHeader::tag :
// ----------------------------------------------------------------------------
//    Creates and writes either a blank or valid canononical wave header at the
//    start of given wave ofstream, matching audio fields to flac header if
//    given, and calculating size fields from ofstream extents.
//
//    Arguments:
//       <out>    - output wav file
//       <meta>   - flac header
//
//    Returns 1
// ----------------------------------------------------------------------------


int waveHeader::tag( ofstream& out, FLAC__StreamMetadata *meta )
{
	static uint8_t h[44]=
	{
		'R','I','F','F',  //  0 - ChunkID
		0,0,0,0,          //  4 - ChunkSize (filesize-8 or rawdata+36)
		'W','A','V','E',  //  8 - Format
		'f','m','t',' ',  // 12 - SubChunkID
		16,0,0,0,         // 16 - SubChunkSize (16 or 22? for PCM)
		1,0,              // 20 - AudioFormat (1 for PCM)
		2,0,              // 22 - NumChannels
		0,0,0,0,          // 24 - SampleRate (Hz)
		0,0,0,0,          // 28 - Byte Rate (SampleRate*NumChannels*BitsPerSample/8)
		4,0,              // 32 - BlockAlign (NumChannels*BitsPerSample/8)
		16,0,             // 34 - BitsPerSample
		'd','a','t','a',  // 36 - Subchunk2ID
		0,0,0,0           // 40 - Subchunk2Size (NumSamples*NumChannels*BitsPerSample/8)
	};

	canonical *w = (canonical*)h;
	uint32_t fileSize, prev;

	if( meta )
	{
		w->numChannels = lEndian( meta->data.stream_info.channels );
		w->sampleRate = lEndian( meta->data.stream_info.sample_rate );
		w->bitsPerSample = lEndian( meta->data.stream_info.bits_per_sample );
		w->blockAlign = lEndian( meta->data.stream_info.channels * meta->data.stream_info.bits_per_sample / 8 );
		w->byteRate = lEndian( meta->data.stream_info.sample_rate * lEndian( w->blockAlign ) );
	}

	if( (prev = out.tellp())== 0 ) prev = 44;
	out.seekp( 0, ios::end );
	fileSize = out.tellp();

	w->chunkSize = fileSize ? lEndian( fileSize - 8 ) : 0;
	w->subchunk2Size = fileSize ? lEndian( fileSize - 44 ) : 0;

	out.seekp( 0, ios::beg );
	out.write( (char*)h, 44 );
	out.seekp( prev, ios::beg );

	return 1;
}



// ----------------------------------------------------------------------------
//    waveHeader::tag :
// ----------------------------------------------------------------------------
//    Variant of above using PES packet lpcm header for audio characteristics.
// ----------------------------------------------------------------------------


int waveHeader::tag( ofstream& out, PES_packet::LPCM_header* LPCM)
{
	static uint8_t h[44]=
	{
		'R','I','F','F',  //  0 - ChunkID
		0,0,0,0,          //  4 - ChunkSize (filesize-8 or rawdata+36)
		'W','A','V','E',  //  8 - Format
		'f','m','t',' ',  // 12 - SubChunkID
		16,0,0,0,         // 16 - SubChunkSize (16 or 22? for PCM)
		1,0,              // 20 - AudioFormat (1 for PCM)
		2,0,              // 22 - NumChannels
		0,0,0,0,          // 24 - SampleRate (Hz)
		0,0,0,0,          // 28 - Byte Rate (SampleRate*NumChannels*BitsPerSample/8)
		4,0,              // 32 - BlockAlign (NumChannels*BitsPerSample/8)
		16,0,             // 34 - BitsPerSample
		'd','a','t','a',  // 36 - Subchunk2ID
		0,0,0,0           // 40 - Subchunk2Size (NumSamples*NumChannels*BitsPerSample/8)
	};

	canonical* w = (canonical*)h;
	uint32_t fileSize, prev;

	if( LPCM )
	{
//      w->SubChunkSize = ;
//      w->audioFormat;
		w->numChannels = PES_packet::channels(LPCM);
		w->sampleRate = PES_packet::frequency(LPCM);
		w->bitsPerSample = PES_packet::bitsPerSample(LPCM);
		w->blockAlign = w->numChannels * w->bitsPerSample / 8;
		w->byteRate = w->sampleRate * w->blockAlign;

		w->numChannels = lEndian( w->numChannels );
		w->sampleRate = lEndian( w->sampleRate );
		w->bitsPerSample = lEndian( w->bitsPerSample );
		w->blockAlign = lEndian( w->blockAlign );
		w->byteRate = lEndian( w->byteRate );
	}

	if( (prev = out.tellp())== 0 ) prev = 44;
	out.seekp( 0, ios::end );
	fileSize = out.tellp();

	w->chunkSize = fileSize ? lEndian( fileSize - 8 ) : 0;
	w->subchunk2Size = fileSize ? lEndian( fileSize - 44 ) : 0;

	out.seekp( 0, ios::beg );
	out.write( (char*)h, 44 );
	out.seekp( prev, ios::beg );

	return 1;
}



// ----------------------------------------------------------------------------
//    waveHeader::audit :
// ----------------------------------------------------------------------------
//    Verifies wav header of <filename> as canonical, then generates <fmeta>.
//
//    Returns 1 on success, 0 on fail
// ----------------------------------------------------------------------------


int waveHeader::audit( const char *filename, FLAC__StreamMetadata *fmeta )
{
	ifstream infile( filename, ios::binary );
	int r = open( infile, fmeta, true );
	infile.close();
	return r;
}



// ----------------------------------------------------------------------------
//    waveHeader::open :
// ----------------------------------------------------------------------------
//    Verifies format is lpcm and seeks to start of 'data' chunk; then
//    generates <fmeta>.
//
//    Returns 1 on success, 0 on fail
// ----------------------------------------------------------------------------


int waveHeader::open( ifstream &wavefile, FLAC__StreamMetadata *fmeta, bool mute )
{
	canonical hdr;
	uint32_t fmtChunk = 0, dataChunk = 0;
	string msg;
	struct{ uint8_t ID[4]; uint32_t size; } chunk;

    wavefile.seekg ( 0, ios::end );
#if 0
	off_t filelength;
	filelength = wavefile.tellg();
#endif

	wavefile.seekg( 0 );
	wavefile.read( (char *)&hdr, 12 );

	if( ! (
			hdr.chunkID[0] == 'R'
		&& hdr.chunkID[1] == 'I'
		&& hdr.chunkID[2] == 'F'
		&& hdr.chunkID[3] == 'F'

		&& hdr.format[0] == 'W'
		&& hdr.format[1] == 'A'
		&& hdr.format[2] == 'V'
		&& hdr.format[3] == 'E' ) )

		ERR( "No Wave header found.\n" );

	while( wavefile.read( (char *)&chunk, 8 ).good() )
	{
		if( wavefile.eof() || wavefile.peek() == EOF )
			break;

		chunk.size = lEndian( chunk.size );

        msg += _f( "%c%c%c%c=%d  ", chunk.ID[0], chunk.ID[1], chunk.ID[2], chunk.ID[3], chunk.size );

		if( ! fmtChunk
			&& chunk.ID[0] == 'f'
			&& chunk.ID[1] == 'm'
			&& chunk.ID[2] == 't' )
		{
			fmtChunk = wavefile.tellg();
			if( chunk.size > 16 )
                msg += _f( "[16 read, %d ignored]  ", chunk.size - 16  );

			wavefile.read( (char *)&hdr+20, chunk.size > 16 ? 16 : chunk.size );

			if( hdr.audioFormat != 1 )
			{
				ERR( "Audio is not lpcm.\n" );
				return 0;
			}
			continue;
		}

		else if( ! dataChunk
			&& chunk.ID[0] == 'd'
			&& chunk.ID[1] == 'a'
			&& chunk.ID[2] == 't'
			&& chunk.ID[3] == 'a' )
		{
			hdr.subchunk2Size = lEndian( chunk.size );
			dataChunk = wavefile.tellg();
#if 0
			if( hdr.subchunk2Size != filelength - dataChunk )
			{
				hdr.subchunk2Size = filelength - dataChunk;
			}
#endif
		}

		wavefile.seekg( chunk.size, ios::cur );
	}

	if( fmtChunk && dataChunk )
	{
		if( ! mute && ( fmtChunk != 20 || dataChunk != 44 ) )
			LOG( "Non-canonical header. Subchunks: " << msg << "\n" );
		flacHeader::readStreamInfo( &hdr, fmeta );
		wavefile.clear();
		wavefile.seekg( dataChunk );
		return 1;
	}

	ERR( _f( "Non-canonical header. Can't find %s%s%s chunk%s.\n",
		fmtChunk ? "" : "'fmt'", ! fmtChunk && ! dataChunk ? " or " : "",
		dataChunk ? "" : "'data'", ! fmtChunk && ! dataChunk ? "s" : "" ) );
	ECHO( "Try converting this file to flac and using that instead." );
	return 0;
}



// ----------------------------------------------------------------------------
//    waveHeader::display :
// ----------------------------------------------------------------------------
//    Displays info for wave header <w> on <stream>, using <prefix> as a title.
// ----------------------------------------------------------------------------


void waveHeader::display(waveHeader::canonical* w, const char* prefix, ostream &stream)
{
	unsigned char* b = (uint8_t*) w;
	stream << prefix <<
		"chunkID       :" << b[0] << b[1] << b[2] << b[3] << endl << prefix <<
		"chunkSize     :" << lEndian( w->chunkSize ) << endl << prefix <<
		"format        :" << b[8] << b[9] << b[10] << b[11] << endl << prefix <<
		"subChunkID    :" << b[12] << b[13] << b[14] << b[15] << endl << prefix <<
		"SubChunkSize  :" << lEndian( w->SubChunkSize ) << endl << prefix <<
		"audioFormat   :" << lEndian( w->audioFormat ) << endl << prefix <<
		"numChannels   :" << lEndian( w->numChannels ) << endl << prefix <<
		"sampleRate    :" << lEndian( w->sampleRate ) << endl << prefix <<
		"byteRate      :" << lEndian( w->byteRate ) << endl << prefix <<
		"blockAlign    :" << lEndian( w->blockAlign ) << endl << prefix <<
		"bitsPerSample :" << lEndian( w->bitsPerSample ) << endl << prefix <<
		"subchunk2ID   :" << b[36] << b[37] << b[38] << b[39] << endl << prefix <<
		"subchunk2Size :" << lEndian( w->subchunk2Size ) << endl << prefix << endl << prefix;
}
