/*
	flac.cpp - flac header utilities.
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



#include "flac.hpp"

void flacHeader::display(const FLAC__StreamMetadata *meta, const char* prefix, ostream &stream)
{
	stream << prefix <<
	"min_blocksize    :" << meta->data.stream_info.min_blocksize << endl << prefix <<
	"max_blocksize    :" << meta->data.stream_info.max_blocksize << endl << prefix <<
	"min_framesize    :" << meta->data.stream_info.min_framesize << endl << prefix <<
	"max_framesize    :" << meta->data.stream_info.max_framesize << endl << prefix <<
	"sample_rate      :" << meta->data.stream_info.sample_rate << endl << prefix <<
	"channels         :" << meta->data.stream_info.channels << endl << prefix <<
	"bits_per_sample  :" << meta->data.stream_info.bits_per_sample << endl << prefix <<
	"total_samples    :" << meta->data.stream_info.total_samples << endl << prefix <<
	"md5sum           :" << hexStr( meta->data.stream_info.md5sum, 16 ) << dec << endl;
}


void flacHeader::readStreamInfo( uint8_t* buf, FLAC__StreamMetadata *meta )
{
	bs_t bits;
	bs_init( &bits, (void*)buf, 18 );

	meta->data.stream_info.min_blocksize = bs_read( &bits, 16 );
	meta->data.stream_info.max_blocksize = bs_read( &bits, 16 );
	meta->data.stream_info.min_framesize = bs_read( &bits, 24 );
	meta->data.stream_info.max_framesize = bs_read( &bits, 24 );
	meta->data.stream_info.sample_rate = bs_read( &bits, 20 );
	meta->data.stream_info.channels = bs_read( &bits, 3 ) + 1;
	meta->data.stream_info.bits_per_sample = bs_read( &bits, 5 ) + 1;
	meta->data.stream_info.total_samples = bs_read( &bits, 36 );
	memcpy( meta->data.stream_info.md5sum, buf + 18, 16 );
}


void flacHeader::readStreamInfo( waveHeader::canonical *wave, FLAC__StreamMetadata *meta )
{
	meta->data.stream_info.min_blocksize =
	meta->data.stream_info.max_blocksize =
	meta->data.stream_info.min_framesize =
	meta->data.stream_info.max_framesize = 0;
	meta->data.stream_info.sample_rate = lEndian( wave->sampleRate );
	meta->data.stream_info.channels = lEndian( wave->numChannels );
	meta->data.stream_info.bits_per_sample = lEndian( wave->bitsPerSample );
	meta->data.stream_info.total_samples = lEndian( wave->subchunk2Size ) / lEndian( wave->blockAlign );
	memset( meta->data.stream_info.md5sum, 0, 16 );
}


void flacHeader::readStreamInfo( PES_packet::LPCM_header *LPCM, FLAC__StreamMetadata *meta )
{
	meta->data.stream_info.min_blocksize =
	meta->data.stream_info.max_blocksize =
	meta->data.stream_info.min_framesize =
	meta->data.stream_info.max_framesize = 0;
	meta->data.stream_info.sample_rate = PES_packet::frequency( LPCM );
	meta->data.stream_info.channels = PES_packet::channels( LPCM );
	meta->data.stream_info.bits_per_sample = PES_packet::bitsPerSample( LPCM );
	meta->data.stream_info.total_samples = 0;
	memset( meta->data.stream_info.md5sum, 0, 16 );
}


void flacHeader::writeStreamInfo(uint8_t* buf, FLAC__StreamMetadata *meta)
{
	bs_t bits;
	bs_init( &bits, (void*)buf, 18 );

	bs_write( &bits, 16, meta->data.stream_info.min_blocksize );
	bs_write( &bits, 16, meta->data.stream_info.max_blocksize );
	bs_write( &bits, 24, meta->data.stream_info.min_framesize );
	bs_write( &bits, 24, meta->data.stream_info.max_framesize );
	bs_write( &bits, 20, meta->data.stream_info.sample_rate );
	bs_write( &bits,  3, meta->data.stream_info.channels - 1);
	bs_write( &bits,  5, meta->data.stream_info.bits_per_sample - 1);
	bs_write( &bits, 36, meta->data.stream_info.total_samples );
	memcpy( buf+18, meta->data.stream_info.md5sum, 16 );
}

void flacHeader::zeroStreamInfo( FLAC__StreamMetadata *meta )
{
	meta->data.stream_info.min_blocksize =
	meta->data.stream_info.max_blocksize =
	meta->data.stream_info.min_framesize =
	meta->data.stream_info.max_framesize =
	meta->data.stream_info.sample_rate =
	meta->data.stream_info.channels =
	meta->data.stream_info.bits_per_sample =
	meta->data.stream_info.total_samples = 0;
	memset( meta->data.stream_info.md5sum, 0, 16 );
}

int flacHeader::write(fstream &out, FLAC__StreamMetadata *meta)
{
	static uint8_t fHeader[42] = { 'f','L','a','C', 0x80, 0x20, 0x00, 0x00 };
	writeStreamInfo( fHeader + 8, meta );
	out.write( (const char*)fHeader, 42 );
	return 1;
}

int flacHeader::write( uint8_t *buf, FLAC__StreamMetadata *meta )
{
    uint8_t tab[]={ 'f','L','a','C', 0x80, 0x20, 0x00, 0x00 };
	memcpy( buf, tab, 8 );
	writeStreamInfo( buf + 8, meta );
	return 1;
}



// ----------------------------------------------------------------------------
//    flacHeader::audit :
// ----------------------------------------------------------------------------
//    Copies flac header from <filename> to <fmeta>.
//
//    Returns
// ----------------------------------------------------------------------------


int flacHeader::audit( const char *filename, FLAC__StreamMetadata *fmeta )
{
	return FLAC__metadata_get_streaminfo( filename, fmeta );
}

