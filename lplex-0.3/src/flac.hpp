/*
	flac.hpp - flac header utilities.
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



#ifndef FLAC_HPP_INCLUDED
#define FLAC_HPP_INCLUDED

#ifndef LPLEX_PCH_INCLUDED
#include "lplex_precompile.h"
#endif



using namespace std;

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <FLAC++/all.h>

#include "util.h"
#include "lpcm.hpp"

class flacHeader
{
public:
	static inline uint64_t bytesUncompressed(FLAC__StreamMetadata *meta)
		{return
			meta->data.stream_info.total_samples *
			meta->data.stream_info.channels *
			meta->data.stream_info.bits_per_sample / 8 ;}

	static void zeroStreamInfo( FLAC__StreamMetadata *meta );
	static void readStreamInfo( uint8_t* buf, FLAC__StreamMetadata *meta );
	static void readStreamInfo( waveHeader::canonical *wave, FLAC__StreamMetadata *meta );
	static void readStreamInfo( PES_packet::LPCM_header *LPCM, FLAC__StreamMetadata *meta );
	static void writeStreamInfo( uint8_t* buf, FLAC__StreamMetadata *meta );
	static int write( fstream &out, FLAC__StreamMetadata *meta );
	static int write( uint8_t *buf, FLAC__StreamMetadata *meta );

	static int audit( const char *filename, FLAC__StreamMetadata *fmeta );
	static void display( const FLAC__StreamMetadata *meta, const char* prefix="", ostream &stream=cerr );

	friend ostream& operator << ( ostream& stream, const FLAC__StreamMetadata& f )
		{ display( &f, "", stream ); return stream; }

};


//typedef struct {
//   unsigned min_blocksize, max_blocksize;
//   unsigned min_framesize, max_framesize;
//   unsigned sample_rate;
//   unsigned channels;
//   unsigned bits_per_sample;
//   FLAC__uint64 total_samples;
//   FLAC__byte md5sum[16];
//} FLAC__StreamMetadata_StreamInfo;


#endif
