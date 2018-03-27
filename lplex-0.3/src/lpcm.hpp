/*
	lpcm.hpp - general lpcm descriptors, dvd-v and wave header utilities.
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



#ifndef LPCM_HPP_INCLUDED
#define LPCM_HPP_INCLUDED

#ifndef LPLEX_PCH_INCLUDED
#include "lplex_precompile.h"
#endif

#include <experimental/filesystem>

using namespace std;
namespace fs = std::experimental::filesystem;

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <md5/md5.h>
#include <FLAC/format.h>

#include "util.h"
#include "wx.hpp"
#include "platform.h"



struct alignment
{
	uint16_t type, shift;
	uint64_t len;
	int32_t unit, padding, offset;
};

struct lpcmEntity
{
	enum
	{
		specified = 0x01,
		named = 0x02,
		ready = named | specified,
		metered = 0x04,
		padded = 0x10,
		prepended = 0x20,
		_eoi = 0x40, // end of input
		_eof = 0x80
	};

	enum
	{
		skip = 0x01,
		remove = 0x02,
		marked = 0x04,
		enabled = 0x08,
		disabled = 0x10
	};

	uint16_t state;
	FLAC__StreamMetadata fmeta;
	md5_state_t md5;
	md5_byte_t md5str[16];
    fs::path fName;
	uint16_t root, index, edit;
	alignment trim;

	static bool soundCheck( lpcmEntity *l, bool mute=true );
    static bool soundMatch( lpcmEntity *a, lpcmEntity *b, char* errmsg = nullptr );
	static string audioInfo( lpcmEntity *l );
	static string audioInfo( FLAC__StreamMetadata *fmeta );
};

inline bool operator < (const lpcmEntity& a, const lpcmEntity& b)
	{ return a.index < b.index; }


enum Ltype
{
	isNot = 0,
	wavef = 1,
	flacf = 2,
	lpcmf = 3,
	m2vf  = 4,
	mpegf = 5,
	vobf  = 6,
	isof  = 7,
	lgzf  = 8
};

inline Ltype isLfile(const char *ext)
{
	return
		! stricmp( ext,".wav" ) ? wavef :
		! stricmp( ext,".flac" ) ? flacf :
		! stricmp( ext,".lpcm" ) ? lpcmf :
		! stricmp( ext,".m2v" ) ? m2vf :
		! stricmp( ext,".mpg" ) ? mpegf :
		! stricmp( ext,".vob" ) ? vobf :
		! stricmp( ext,".iso" ) ? isof :
		isNot;
}

#define SCRTIME 27000000
#define PTSTIME 90000

class PES_packet
{
public:
	enum
	{
		swap=0x01,
		raw=0x02,
		adopt=0x04,
		start=0x08,
		flush=0x10,
		unfinished=0x20,
		end=0x80
	};

	struct header
	{
		uint32_t startCode;
		uint16_t packetLen;
		uint8_t flags1;
		uint8_t flags2;
		uint8_t headerLen;
	};

	struct LPCM_header
	{
		uint8_t streamID;
		uint8_t frames;
		uint16_t dataPointer;
		uint8_t flags1;
		uint8_t flags2;
		uint8_t dynamicRange;
	};



	static inline LPCM_header* lpcmAddr(PES_packet::header* PS1)
		{ return (LPCM_header*)( (uint8_t*)PS1 + 9 + PS1->headerLen ); }

	static inline uint8_t* dataAddr(PES_packet::header* PS1)
		{ return (uint8_t*) lpcmAddr( PS1 ) + 7; }

	static inline uint16_t dataLen(PES_packet::header* PS1)
		{ return bEndian( PS1->packetLen ) - PS1->headerLen - 11; }

	static inline uint8_t* firstFrame(PES_packet::LPCM_header* LPCM)
		{ return (uint8_t*)LPCM + 3 + bEndian( LPCM->dataPointer ); }

	static inline uint8_t* firstFrame(PES_packet::header* PS1)
		{ return firstFrame( lpcmAddr( PS1 ) ); }

	static inline uint16_t bitsPerSample(PES_packet::LPCM_header* LPCM)
		{ return ( LPCM->flags2 >> 6 ) * 4 + 16; }

	static inline uint16_t bytesPerSample(PES_packet::LPCM_header* LPCM)
		{ return bitsPerSample( LPCM ) / 2; }

	static inline uint8_t audioType(PES_packet::LPCM_header* LPCM)
		{ return ( LPCM->flags2 | 0x30 ) ^ 0x30; }

	static inline uint16_t quantization(PES_packet::LPCM_header* LPCM)
		{ return ( LPCM->flags2 >> 6 )* 4 + 16; }

	static inline uint32_t frequency(PES_packet::LPCM_header* LPCM)
		{ return 48000 * (((( LPCM->flags2 >> 4 ) | 0xFC ) ^ 0xFC ) + 1 ); }

	static inline uint16_t channels(PES_packet::LPCM_header* LPCM)
		{ return ( ( LPCM->flags2 | 0xF8 ) ^ 0xF8 ) + 1; }

	static inline uint16_t bytesPerFrame(PES_packet::LPCM_header* LPCM)
	{
		return frequency(LPCM) * channels(LPCM) * quantization(LPCM) /
			( 8 /*bits per byte*/ * 600 /*dvd lpcm audio frames per sec*/ );
	}

	static inline uint16_t bitsPerSample(PES_packet::header* PS1)
		{ return bitsPerSample( lpcmAddr( PS1 ) ); }
	static inline uint16_t bytesPerSample(PES_packet::header* PS1)
		{ return bytesPerSample( lpcmAddr( PS1 ) ); }

	static inline uint8_t audioType(PES_packet::header* PS1)
		{ return audioType( lpcmAddr( PS1 ) ); }
	static inline uint16_t quantization(PES_packet::header* PS1)
		{ return quantization( lpcmAddr( PS1 ) ); }
	static inline uint16_t frequency(PES_packet::header* PS1)
		{ return frequency( lpcmAddr( PS1 ) ); }
	static inline uint16_t channels(PES_packet::header* PS1)
		{ return channels( lpcmAddr( PS1 ) ); }
	static inline uint16_t bytesPerFrame(PES_packet::header* PS1)
		{ return bytesPerFrame( lpcmAddr( PS1 ) ); }


	static uint16_t payload( PES_packet::header* PS1,
		byteRange *audio, uint8_t mode, uint64_t ptsSeam );
	static inline unsigned char * ptsAddr( PES_packet::header* PS1 )
		{ return ( (uint8_t*)PS1 + 9 ); }


	static void display( header* h );
	static void display( LPCM_header* h );
	static void displayFlags( LPCM_header* h );

	static uint64_t readpts( PES_packet::header* PS1 )
		{ return readpts( (uint8_t*)PS1 + 9 ); }
	static uint64_t readpts( uint8_t* buf );

	static LPCM_header * isLpcmPacket( unsigned char *lb )
	{
		header *PS1 = (header*) &lb[ 14 ];
		if( PS1->startCode == bEndian( (uint32_t) 0x000001BD ) )
		{
			LPCM_header *LPCM = lpcmAddr( PS1 );
			if( LPCM->streamID >= 0xA0 )
				return LPCM;
		}
		return NULL;
	}


};



class waveHeader
{
public:
	struct canonical
	{
		uint8_t chunkID[4];
		uint32_t chunkSize;
		uint8_t format[4];
		uint8_t subChunkID[4];
		uint32_t SubChunkSize;
		uint16_t audioFormat;
		uint16_t numChannels;
		uint32_t sampleRate;
		uint32_t byteRate;
		uint16_t blockAlign;
		uint16_t bitsPerSample;
		uint8_t subchunk2ID[4];
		uint32_t subchunk2Size;
	};

	static int tag( ofstream& out, FLAC__StreamMetadata *meta=0 );
	static int tag( ofstream& out, PES_packet::LPCM_header* LPCM=0 );
	static int tag( ofstream& out, PES_packet::header* PS1 )
		{ return tag( out, PES_packet::lpcmAddr( PS1 ) ); }

	static int open( ifstream &wavefile, FLAC__StreamMetadata *fmeta, bool mute=false );
	static int audit( const char *filename, FLAC__StreamMetadata *fmeta );
	static void display( canonical* h, const char* prefix="", ostream &stream=cerr  );

};



#endif
