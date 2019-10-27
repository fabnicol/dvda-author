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
//

int waveHeader::tag(const fs::path &path, const FLAC__StreamMetadata& meta )
{
//  {'R','I','F','F',    //  0 - ChunkID
//    0,0,0,0,            //  4 - chunkSize (filesize - 8 - padbyte)
//    'W','A','V','E',    //  8 - format
//    'f','m','t',' ',    // 12 - subChunkID
//    40,0,0,0,           // 16 - subChunkSize  // 40 for extensible PCM as 16 is only for WAVE_FORMAT_PCM
//    0xFE, 0xFF,          // 20 - audioFormat (1=16-bit)
//    2,0,                // 22 - numChannels
//    0,0,0,0,            // 24 - sampleRate in Hz
//    0,0,0,0,            // 28 - byteRate (SampleRate*NumChannels*(BitsPerSample/8)
//    4,0,                // 32 - blockAlign (== NumChannels * BitsPerSample/8)
//    16,0,               // 34 - bitsPerSample
//    22,0,               // 36 - wavext  (0 or 22 bytes)
//    if not 0:
//    0,0,                // 38 - validBits
//    0,0,0,0,            // 40 - dwChannelMmask
//    [16 B]              // 44 - GUID
//    'f','a','c','t',    // 60 - ckID
//    4,0,0,0,            // 64 - ckSize
//    0,0,0,0,            // 68 - dwSampleLength
//   // some software pack up various tags in here... + x bytes
//    'd','a','t','a',    // 72 + x - sunchunk2ID
//    0,0,0,0             // 76 + x - 80 subchunk2Size
//  };

    extensible w;

	uint32_t fileSize;
	fileSize = fs::file_size(path);
    cga2wav_channels[6] = {0x4, 0x3, 0x7, 0x33, 0x10F, 0x3F};

  	if( ! LPCM ) return -1;

    w.chunkID[0] = 'R';
    w.chunkID[1] = 'I';
    w.chunkID[2] = 'F';
    w.chunkID[3] = 'F';
    w.chunkSize = filesize - 8;
    w.format[0] = 'W';
    w.format[1] = 'A';
    w.format[2] = 'V';
    w.format[3] = 'E';
    w.subChunkID[0] = 'f';
    w.subChunkID[1] = 'm';
    w.subChunkID[2] = 't';
    w.subChunkID[3] = ' ';
    w.subChunkSize = 40;
    w.audioFormat = 0xFFFE;

    w.numChannels = meta.data.stream_info.channels ;
    w.sampleRate = meta.data.stream_info.sample_rate ;
    w.bitsPerSample = meta.data.stream_info.bits_per_sample ;
    w.blockAlign = meta.data.stream_info.channels * meta.data.stream_info.bits_per_sample / 8 ;
    w.byteRate = meta.data.stream_info.sample_rate * w->blockAlign;

    w.wavext = 22;
    w.validBits = w.bitsPerSample;
    w.dwChannelMask = cga2wav_channels[w.numChannels - 1];
    w.ckID[0] = 'f';
    w.ckID[1] = 'a';
    w.ckID[2] = 'c';
    w.ckID[3] = 't';
    w.ckSize = 4;
    w.subchunk2ID[0] = 'd';
    w.subchunk2ID[1] = 'a';
    w.subchunk2ID[2] = 't';
    w.subchunk2ID[3] = 'a';
    w.subchunk2Size = filesize - 80 - (fileSize % 2);

    uint8_t P[80];
    uint8_t *p=&P[0];

    uint32_copy_reverse(p, w.chunkID), p+=4;
    uint32_copy_reverse(p, w.chunkSize), p+=4;
    uint32_copy_reverse(p, w.format), p+=4;
    uint32_copy_reverse(p, w.subChunkID), p+=4;
    uint32_copy_reverse(p, w.SubChunkSize), p+=4;
    uint16_copy_reverse(p, w.audioFormat), p+=2;
    uint16_copy_reverse(p, w.numChannels), p+=2;
    uint32_copy_reverse(p, w.sampleRate), p+=4;
    uint32_copy_reverse(p, w.byteRate), p+=4;
    uint16_copy_reverse(p, w.blockAlign), p+=2;
    uint16_copy_reverse(p, w.bitsPerSample), p+=2;
    uint16_copy_reverse(p, w.wavext), p+=2;
    uint16_copy_reverse(p, w.bitsPerSample), p+=2;  // in principle, wValidBitsPerSample
    uint32_copy_reverse(p, w.dwChannelMask), p+=4;
    const uint8_t GUID[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71};
    memcpy(p, GUID, 16), p+=16;
    uint32_copy_reverse(p, w.ckID), p+=4;
    uint32_copy_reverse(p, w.ckSize), p+=4;
    if (w.numChannels&& w.bitsPerSample)
      {
        uint32_copy_reverse(p, (fileSize /(header->channels * header->wBitsPerSample / 8)), p+=4;  //dwSampleLength
      }

    uint32_copy_reverse(p, w.subchunk2ID), p+=4;
    uint32_copy_reverse(p, w.subchunk2Size);

	ofstream out;
	out.open(path, ios::binary);

	out.seekp( 0, ios::beg );
	out.write( P, 80);
	out.seekp( 80, ios::beg );

	return 1;

	return 1;
}



// ----------------------------------------------------------------------------
//    waveHeader::tag :
// ----------------------------------------------------------------------------
//    Variant of above using PES packet lpcm header for audio characteristics.
// ----------------------------------------------------------------------------

// apparently not used

inline static void uint32_copy_reverse(uint8_t* buf, uint32_t x)
{
    buf[0]=x&0xff;
    buf[1]=(x&0xff00)>>8;
    buf[2]=(x&0xff0000)>>16;
    buf[3]=x>>24;
}

inline static void uint16_copy_reverse(uint8_t* buf, uint16_t x)
{
    buf[0]=x&0xff;
    buf[1]=(x&0xff00)>>8;
}

int waveHeader::tag(const fs::path &path, const PES_packet::LPCM_header* LPCM)
{
//  {'R','I','F','F',    //  0 - ChunkID
//    0,0,0,0,            //  4 - chunkSize (filesize - 8 - padbyte)
//    'W','A','V','E',    //  8 - format
//    'f','m','t',' ',    // 12 - subChunkID
//    40,0,0,0,           // 16 - subChunkSize  // 40 for extensible PCM as 16 is only for WAVE_FORMAT_PCM
//    0xFE, 0xFF,          // 20 - audioFormat (1=16-bit)
//    2,0,                // 22 - numChannels
//    0,0,0,0,            // 24 - sampleRate in Hz
//    0,0,0,0,            // 28 - byteRate (SampleRate*NumChannels*(BitsPerSample/8)
//    4,0,                // 32 - blockAlign (== NumChannels * BitsPerSample/8)
//    16,0,               // 34 - bitsPerSample
//    22,0,               // 36 - wavext  (0 or 22 bytes)
//    if not 0:
//    0,0,                // 38 - validBits
//    0,0,0,0,            // 40 - dwChannelMmask
//    [16 B]              // 44 - GUID
//    'f','a','c','t',    // 60 - ckID
//    4,0,0,0,            // 64 - ckSize
//    0,0,0,0,            // 68 - dwSampleLength
//   // some software pack up various tags in here... + x bytes
//    'd','a','t','a',    // 72 + x - sunchunk2ID
//    0,0,0,0             // 76 + x - 80 subchunk2Size
//  };

    extensible w;

	uint32_t fileSize;
	fileSize = fs::file_size(path);
    cga2wav_channels[6] = {0x4, 0x3, 0x7, 0x33, 0x10F, 0x3F};

  	if( ! LPCM ) return -1;

    w.chunkID[0] = 'R';
    w.chunkID[1] = 'I';
    w.chunkID[2] = 'F';
    w.chunkID[3] = 'F';
    w.chunkSize = filesize - 8;
    w.format[0] = 'W';
    w.format[1] = 'A';
    w.format[2] = 'V';
    w.format[3] = 'E';
    w.subChunkID[0] = 'f';
    w.subChunkID[1] = 'm';
    w.subChunkID[2] = 't';
    w.subChunkID[3] = ' ';
    w.subChunkSize = 40;
    w.audioFormat = 0xFFFE;
    w.numChannels = PES_packet::channels(LPCM);
    w.sampleRate = PES_packet::frequency(LPCM);
    w.bitsPerSample = PES_packet::bitsPerSample(LPCM);
    w.blockAlign = w->numChannels * w->bitsPerSample / 8;
    w.byteRate = w->sampleRate * w->blockAlign;
    w.wavext = 22;
    w.validBits = w.bitsPerSample;
    w.dwChannelMask = cga2wav_channels[w.numChannels - 1];
    w.ckID[0] = 'f';
    w.ckID[1] = 'a';
    w.ckID[2] = 'c';
    w.ckID[3] = 't';
    w.ckSize = 4;
    w.subchunk2ID[0] = 'd';
    w.subchunk2ID[1] = 'a';
    w.subchunk2ID[2] = 't';
    w.subchunk2ID[3] = 'a';
    w.subchunk2Size = filesize - 80 - (fileSize % 2);

    uint8_t P[80];
    uint8_t *p=&P[0];

    uint32_copy_reverse(p, w.chunkID), p+=4;
    uint32_copy_reverse(p, w.chunkSize), p+=4;
    uint32_copy_reverse(p, w.format), p+=4;
    uint32_copy_reverse(p, w.subChunkID), p+=4;
    uint32_copy_reverse(p, w.SubChunkSize), p+=4;
    uint16_copy_reverse(p, w.audioFormat), p+=2;
    uint16_copy_reverse(p, w.numChannels), p+=2;
    uint32_copy_reverse(p, w.sampleRate), p+=4;
    uint32_copy_reverse(p, w.byteRate), p+=4;
    uint16_copy_reverse(p, w.blockAlign), p+=2;
    uint16_copy_reverse(p, w.bitsPerSample), p+=2;
    uint16_copy_reverse(p, w.wavext), p+=2;
    uint16_copy_reverse(p, w.bitsPerSample), p+=2;  // in principle, wValidBitsPerSample
    uint32_copy_reverse(p, w.dwChannelMask), p+=4;
    const uint8_t GUID[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71};
    memcpy(p, GUID, 16), p+=16;
    uint32_copy_reverse(p, w.ckID), p+=4;
    uint32_copy_reverse(p, w.ckSize), p+=4;
    if (w.numChannels&& w.bitsPerSample)
      {
        uint32_copy_reverse(p, (fileSize /(header->channels * header->wBitsPerSample / 8)), p+=4;  //dwSampleLength
      }

    uint32_copy_reverse(p, w.subchunk2ID), p+=4;
    uint32_copy_reverse(p, w.subchunk2Size);

	ofstream out;
	out.open(path, ios::binary);

	out.seekp( 0, ios::beg );
	out.write( P, 80);
	out.seekp( 80, ios::beg );

	return 1;
}



// ----------------------------------------------------------------------------
//    waveHeader::audit :
// ----------------------------------------------------------------------------
//    Verifies wav header of <filename> as canonical, then generates <fmeta>.
//
//    Returns 1 on success, 0 on fail
// ----------------------------------------------------------------------------


int waveHeader::audit( const char *filename, const FLAC__StreamMetadata *fmeta )
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


int waveHeader::open( ifstream &wavefile, const FLAC__StreamMetadata *fmeta, bool mute )
{
	extensible hdr;
	uint32_t fmtChunk = 0, dataChunk = 0, factChunk = 0;
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

    	if (! fmtChunk)
        {
            chunk.size = lEndian( chunk.size );  // 16 (canonical) or 40 (extensible)
            switch (chunk.size)
            {
                case 16:
                    msg += _f("%s ",  "Canonical header");
                    break;
                case 40:
                    msg += _f("%s ",  "Extensible header");
                    break;
                default:
                    ERR("Wav header type not recognized (neither canonical nor extensible");
                    return 0;
            }
        }

		if( ! fmtChunk
			&& chunk.ID[0] == 'f'
			&& chunk.ID[1] == 'm'
			&& chunk.ID[2] == 't' )
		{
			fmtChunk = wavefile.tellg();

			wavefile.read((char *)&hdr + 20, chunk.size);

			if( hdr.audioFormat != WAVE_FORMAT_PCM && hdr.audioFormat != WAVE_FORMAT_EXTENSIBLE)
			{
				ERR( "Audio is not lpcm.\n" );
				return 0;
			}
			continue;
		}
		else if( ! factChunk
            && hdr.audioFormat == WAVE_FORMAT_EXTENSIBLE
			&& chunk.ID[0] == 'f'
			&& chunk.ID[1] == 'a'
			&& chunk.ID[2] == 'c'
			&& chunk.ID[3] == 't' )
		{
		    factChunk = wavefile.tellg();
            chunk.size = lEndian( chunk.size );  // 4
            if (chunk.size != 4)
            {
                 ERR("fact chunk issue (should be 4)");
                 return 0;
            }

		    wavefile.read((char *)&hdr + 68, chunk.size);
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
		}
		else
        {
            char c;
            while( wavefile.read(&c, 1).good() )
            {
                if (c != 'd') continue;
                char data[4] = {0};
                wavefile.read(&data[0], 3);
                if (strcmp(data, "ata") == 0)
                {
                    wavefile.read( &data[0], 4);
                    memcpy(&chunk.size, data, 4);
                    hdr.subchunk2Size = lEndian( chunk.size );
			        dataChunk = wavefile.tellg();
                }
                else
                {

                 ERR("Could not find data tag");
                 return 0;
                }
            }
            return 0;
        }
	}
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
