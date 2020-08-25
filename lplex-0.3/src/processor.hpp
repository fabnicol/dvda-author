/*
	processor.hpp - lpcm audio processing.
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



#ifndef PROCESSOR_HPP_INCLUDED
#define PROCESSOR_HPP_INCLUDED

#ifndef LPLEX_PCH_INCLUDED
#include "lplex_precompile.h"
#endif


using namespace std;

#include <FLAC++/all.h>

#include "util.h"
#include "lpcm.hpp"
#include "flac.hpp"



class lpcmProcessor : public lpcmEntity
{
public:
	counter<uint64_t> ct;
	lpcmProcessor() { state = 0; }

	bool md5compare( const void *md5, const char * prefix="" );
	bool eof() { return state & _eof; }
};



class lpcmReader : public lpcmProcessor
{
public:
	unsigned char *bigBuf;
	uint32_t sizeofbigBuf;
	uint16_t alignment, surplus;
	uint64_t unread;
	uint64_t gcount;
	uint64_t bufPos;
	counter<uint64_t> pos;


	lpcmReader( unsigned char *buf, uint32_t size )
		: bigBuf( buf ), sizeofbigBuf( size ) { bufPos = 0; }

	void setbuf( unsigned char *buf, uint32_t size )
		{ bigBuf = buf; sizeofbigBuf = size; }
	static int swap2dvd( unsigned char *data, uint32_t count,
		int channels, int bitspersample );

	int adjust( int prepend, bool pad );

	virtual uint64_t read( unsigned char *buf, uint64_t len );
	virtual uint64_t fillBuf( uint64_t limit=0, counter<uint64_t> *midCount=NULL ) = 0;
    virtual uint16_t reset( const string& filename, int alignUnit=0 ) = 0;
};



class waveReader : public lpcmReader
{
public:
	ifstream waveFile;

    waveReader( const string& filename, unsigned char *buf, uint32_t size,
		int alignUnit=0 )
        : lpcmReader( buf, size ) { reset( filename, alignUnit ); }
	~waveReader() { if( waveFile.is_open() ) waveFile.close(); }

	// from lpcmReader
	virtual uint64_t fillBuf( uint64_t limit=0, counter<uint64_t> *midCount=NULL );
    virtual uint16_t reset( const string& filename, int alignUnit=0 );
};



class flacReader : public lpcmReader, public FLAC::Decoder::File
{
public:
	char *reserve;
	int32_t maxFrame, unsent;

    flacReader( const string& filename, unsigned char *buf, uint32_t size,
		int alignUnit=0 )
		: lpcmReader( buf, size ), FLAC::Decoder::File(), reserve( NULL )
        { reset( filename, alignUnit ); }
	~flacReader() { if( reserve ) delete reserve; }

	// from lpcmReader
	virtual uint64_t fillBuf( uint64_t limit=0, counter<uint64_t> *midCount=NULL );
    virtual uint16_t reset( const string& filename, int alignUnit=0 );

	// from FLAC::Decoder::File
	virtual ::FLAC__StreamDecoderWriteStatus write_callback(
		const ::FLAC__Frame *frame, const FLAC__int32 * const buf[]);
	virtual void metadata_callback(const ::FLAC__StreamMetadata *meta);
	virtual void error_callback(::FLAC__StreamDecoderErrorStatus status);
};



class lpcmWriter : public lpcmProcessor
{
public:

	uint32_t unsent;

	uint16_t preset( const char *filename )
		{
          fName = filename;
          state |= named;
          return state;
         }

	uint16_t preset( ::FLAC__StreamMetadata *f )
		{
           fmeta = *f;
           return soundCheck( this );
        }

	uint16_t preset( PES_packet::LPCM_header *LPCM )
		{
          flacHeader::readStreamInfo( LPCM, &fmeta );
          return soundCheck( this, false );
        }

	static int swap2wav( unsigned char *data, uint32_t count,
		int channels, int bitspersample );

	uint32_t process( byteRange *audio )
		{
           return process( audio->start, audio->len );
        }

	virtual uint32_t process( unsigned char *buf, uint32_t size ) = 0;
	virtual uint16_t isOpen() = 0;
	virtual uint16_t open() = 0;
	virtual uint16_t close() = 0;
	virtual uint16_t md5Report() = 0;
};



class rawWriter : public lpcmWriter
{
public:
	uint16_t interSamp;
	ofstream rawFile;
	md5_state_t md5raw;
	md5_byte_t md5strRaw[16];

	rawWriter() {}
	~rawWriter() {}

	// from lpcmWriter
	virtual uint32_t process( unsigned char *buf, uint32_t size );
	virtual uint16_t open();
	virtual uint16_t isOpen() { return rawFile.is_open(); }
	virtual uint16_t close();
	virtual uint16_t md5Report();
};



class waveWriter : public lpcmWriter
{
public:
	uint16_t interSamp;
	ofstream waveFile;

	waveWriter() {}
	~waveWriter() {}

	// from lpcmWriter
	virtual uint32_t process( unsigned char *buf, uint32_t size );
	virtual uint16_t open();
	virtual uint16_t isOpen() { return waveFile.is_open(); }
	virtual uint16_t close();
	virtual uint16_t md5Report();
};



class flacWriter : public lpcmWriter, public FLAC::Encoder::File
{
public:
	uint16_t interSamp, level, pad/*, vorbiscomment, seektable, cuesheet*/;
	FLAC__int32 samples[1024];
	FLAC__int32* channels[6];

	flacWriter( uint16_t l=6 ) : level(l), pad(4096) {}
	~flacWriter() {}

	void presetCompress(uint16_t l) { level = l; }
	void presetPadding(uint16_t p) { pad = p; }

	int swap2flac( FLAC__int32 *flac, unsigned char *ucDVD,
		uint32_t dvdCount, int channels, int bitspersample );
	void showState( int initcode = 0, const char *message = "" );

		// from lpcmWriter
	virtual uint32_t process( unsigned char *buf, uint32_t size );
	virtual uint16_t open();
	virtual uint16_t isOpen() { return is_valid() ? 1 : 0; }
	virtual uint16_t close();
	virtual uint16_t md5Report();

	// from FLAC::Encoder::File
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
	virtual void progress_callback (FLAC__uint64 bytes_written,
		FLAC__uint64 samples_written,
		unsigned frames_written,
		unsigned total_frames_estimate) {}
#pragma GCC diagnostic pop

};



#endif
