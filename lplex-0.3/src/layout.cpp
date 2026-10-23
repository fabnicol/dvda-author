/*
	layout.cpp - dvd layout design and management.
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



#include "lplex.hpp"
#include "jobs.hpp"

// ----------------------------------------------------------------------------
//    dvdLayout::readerNext :
// ----------------------------------------------------------------------------
//    Records md5 and closes current reader, then opens and initializes next
//    reader.
//
//    Returns 1 on success, 0 on failure or if no more readers
// ----------------------------------------------------------------------------


int dvdLayout::readerNext()
{
	lpcmFile *lFile;

	if( ++readIndex )
	{
		lFile = &Lfiles->at( readIndex - 1 );
		memcpy( &lFile->md5str, &reader->fmeta.data.stream_info.md5sum, 16 );
		lFile->type |= lpcmFile::readComplete;
		//delete reader;
		reader = NULL;
	}

	if( readIndex < (int) Lfiles->size() )
		lFile = &Lfiles->at( readIndex );
	else
	{
		readIndex = -1;
		return 0;
	}

	if( lFile->format == wavef )
        reader = new waveReader( lFile->fName.string(), bigBlock,
			sizeof( bigBlock ), dvdSampleSeam );
	else if( lFile->format == flacf )
        reader = new flacReader( lFile->fName.string(), bigBlock,
			sizeof( bigBlock ), dvdSampleSeam );

	if( ! reader )
		FATAL( "Could not create audio file reader." );

	if( reader->state != lpcmEntity::ready )
		return 0;

	return 1;
}



// ----------------------------------------------------------------------------
//    dvdUtil::timestamp :
// ----------------------------------------------------------------------------
//    Returns hh:mm:ss.ff frame-accurate timestamp string.
//
//    Arguments:
//       <f>      - absolute frame offset
//       <ntsc>   - whether tv system is ntsc
// ----------------------------------------------------------------------------


string dvdUtil::timestamp( uint32_t f, bool ntsc )
{
	uint32_t fph;
	uint16_t fps, fpm, hour, min, sec, frame;

	fps = ( ntsc ? 30 : 25 );
	fpm = fps * 60;
	fph = fps * 3600;

	hour = f / fph;
	min = ( f % fph ) / fpm;
	sec = ( f % fpm ) / fps;
	frame = f % fps;

	return _f( "%02d:%02d:%02d.%02d", hour, min, sec, frame );
}



// ----------------------------------------------------------------------------
//    dvdUtil::time :
// ----------------------------------------------------------------------------
//    Returns mm:ss time string.
//
//    Arguments:
//       <f>      - absolute frame offset
//       <ntsc>   - whether tv system is ntsc
// ----------------------------------------------------------------------------


string dvdUtil::time( uint32_t f, bool ntsc )
{
	uint16_t min;
	uint64_t ticks = f * ( ntsc ? 3003 : 3600 );
	double sec = (double)ticks / 90000;

	min = (int)sec / 60;
	sec -= ( min * 60 );

	return _f( "%d:%02.0f", min, sec );
}



// ----------------------------------------------------------------------------
//    dvdUtil::sampleSeam :
// ----------------------------------------------------------------------------
//    Returns the size of the minimum coherent sample block in bytes
// ----------------------------------------------------------------------------


int dvdUtil::sampleSeam( int channels, int bits_per_sample )
{
	return channels * bits_per_sample / 4;
}

int dvdUtil::sampleSeam( FLAC__StreamMetadata *fmeta )
{
	return sampleSeam(
		fmeta->data.stream_info.channels,
		fmeta->data.stream_info.bits_per_sample );
}



// ----------------------------------------------------------------------------
//    dvdUtil::audioFrame :
// ----------------------------------------------------------------------------
//    Returns the size in bytes of 1 dvd audio frame (bytes per sec/600)
// ----------------------------------------------------------------------------


int dvdUtil::audioFrame( int sample_rate, int channels, int bits_per_sample )
{
	return sample_rate *  channels * bits_per_sample /
		( 8 /*bits per byte*/ * 600 /*dvd lpcm audio frames per sec*/ );
}

int dvdUtil::audioFrame( FLAC__StreamMetadata *fmeta )
{
	return audioFrame(
		fmeta->data.stream_info.sample_rate,
		fmeta->data.stream_info.channels,
		fmeta->data.stream_info.bits_per_sample );
}



// ----------------------------------------------------------------------------
//    dvdUtil::AVseam :
// ----------------------------------------------------------------------------
//    Returns the minimum packet size in bytes that is fully aligned to both
//    audio and video frames for the given tv standard.
// ----------------------------------------------------------------------------


int dvdUtil::AVseam( int audioFrameLen, bool ntsc )
{
	return audioFrameLen * ( ntsc ? 1001 : 24 );
}



// ----------------------------------------------------------------------------
//    dvdLayout::setAudioUnits :
// ----------------------------------------------------------------------------
//    Resets dvd unit sizes based on audio characteristics in <fmeta>.
//
//    Returns
// ----------------------------------------------------------------------------


void dvdLayout::setAudioUnits( FLAC__StreamMetadata *fmeta )
{
	dvdSampleSeam = sampleSeam( fmeta );
	dvdAudioFrame = audioFrame( fmeta );
	dvdAVseam = AVseam( dvdAudioFrame, job->tv == NTSC );
}



// ----------------------------------------------------------------------------
//    dvdUtil::m2vEstimate :
// ----------------------------------------------------------------------------
//    Multiplies <lFile>'s previously calculated rough average gop size to full
//    video length.
// ----------------------------------------------------------------------------


uint64_t dvdUtil::m2vEstimate( lpcmFile * lFile, bool ntsc )
{
	uint16_t gops = lFile->videoFrames / ( ntsc ? 18 : 15 ) + 1;
	return (uint64_t) jpegs[ lFile->jpgIndex ].roughGOP * gops;
}


// ----------------------------------------------------------------------------
//    dvdUtil::sizeOnDvd :
// ----------------------------------------------------------------------------
//    Estimates space required on dvd (audio + video) for given file.
// ----------------------------------------------------------------------------


uint64_t dvdUtil::sizeOnDvd( lpcmFile * lFile, bool ntsc )
{
	return ( lFile->trim.len + m2vEstimate( lFile, ntsc ) ) * 103 / 100;
}


// ----------------------------------------------------------------------------
//    dvdLayout::vobEstimate :
// ----------------------------------------------------------------------------
//    Estimates size of dvd filestructure.
// ----------------------------------------------------------------------------


uint64_t dvdLayout::vobEstimate()
{
	uint32_t overhead = ( total.audio > GIGABYTE ? 1 : total.audio / GIGABYTE )
		* 24 * MEGABYTE;
	return (uint64_t) ( (double)( total.audio + total.video ) * 1.03 + overhead );
}



// ----------------------------------------------------------------------------
//    dvdLayout::checkSpace :
// ----------------------------------------------------------------------------
//    Checks whether layout fits on media, then whether free space is
//    sufficient.
//
//    Returns 0 on success, fatal on failure
// ----------------------------------------------------------------------------


int dvdLayout::checkSpace()
{
	struct{ const char * name; uint64_t capacity; } media[] =
	{
		{ "DVD+R/RW", 4700372992LL },
		{ "DVD-R/RW", 4706074624LL },
		{ "DVD+R DL", 8547993600LL }
	};

	total.estimate = (uint64_t) ( vobEstimate() + total.info );

	spaceTxt = _f( "Layout is roughly %d MB", (int)( total.estimate / MEGABYTE ) );

	if( job->media != unspecified )
	{
		if( ! editing
			&& total.estimate > media[ job->media ].capacity
			&& job->prepare >= mpegf )

			FATAL( _f( "Exceeding %s capacity by roughly %d MB.\n",
				media[ job->media ].name,
				( total.estimate - media[ job->media ].capacity ) / MEGABYTE ) );

		spaceTxt += _f( " (%s is %d%% full)", media[ job->media ].name,
			(int)( (double) total.estimate /
				(double) media[ job->media ].capacity  * 100 ) );
	}

	INFOv( spaceTxt << ".\n" );

#if 0
    uint64_t freeSpace = fs::space(job->outPath).available;
    uint64_t tempSpace = fs::space(job->tempPath).available;
    uint64_t isoSpace  = fs::space(job->isoPath).available;

    if ( job->prepare == lpcmf )
		total.estimate = total.audio;
    else if ( job->prepare == m2vf )
		total.estimate = total.audio + total.video;

	enum{ _dvd = 0x01, _temp = 0x02, _iso = 0x04 };
	uint16_t fail = 0;

	uint16_t f = ((uint16_t[]){ 0, 0, 0, 1, 1, 1 }) [ job->prepare-3 ];
	uint16_t w = ((uint16_t[]){ 1, 1, 1, 1, 0, 0 }) [ job->prepare-3 ];
	uint16_t n = ((uint16_t[]){ 1, 1, 2, 2, 2, 2 }) [ job->prepare-3 ];
	uint16_t s = ((uint16_t[]){ 0, 0, 0, 0, 1, 1 }) [ job->prepare-3 ];
	w = ( job->params & cleanup ? w : n );
	if( job->params & dvdStyler )
		f = 1;

	if( freeSpace == tempSpace && freeSpace == isoSpace )
	{
		if( freeSpace < total.estimate * ( f + w + s ) )
			fail |= _dvd;
	}

	else if( freeSpace == tempSpace )
	{
		if( freeSpace < total.estimate * ( f + w ) )
			fail |= _dvd;
		if( isoSpace < total.estimate * s )
			fail |= _iso;
	}

	else if( freeSpace == isoSpace )
	{
		if( freeSpace < total.estimate * ( f + s ) )
			fail |= _dvd;
		if( tempSpace < total.estimate * n )
			fail |= _temp;
	}

	else if( tempSpace == isoSpace )
	{
		if( freeSpace < total.estimate * f )
			fail |= _dvd;
		if( tempSpace < total.estimate * ( w + s ) )
			fail |= _temp;
	}

	else
	{
		if( freeSpace < total.estimate * f )
			fail |= _dvd;
		if( tempSpace < total.estimate * n )
			fail |= _temp;
		if( isoSpace < total.estimate * s )
			fail |= _iso;
	}

	if( fail )
	{
		POST( "\n" );
		_verbose = true;
		_xcode = 2;
#if 0
		dev_t devices[] = {
            deviceNum( fs_validPath( job->outPath) ),
            deviceNum( fs_validPath( job->tempPath) ),
            deviceNum( fs_validPath( job->isoPath) ) };

		for( int i=0; i < 3; i++ )
			if( fail & ( 0x01 << i ) )
				ERR( "Not enough space on device " << device( devices[i] ) << " (" << sizeStr( freeSpace ) << " free)\n" );
#else
		struct{ dev_t id; uint64_t space; char *path; } devices[] = {
            { deviceNum( fs_validPath( job->outPath ) ), freeSpace, "dvdpath " },
            { deviceNum( fs_validPath( job->tempPath) ), tempSpace, "workpath" },
            { deviceNum( fs_validPath( job->isoPath) ), isoSpace, "isopath " } };

		for( int i=0; i < 3; i++ )
			if( fail & ( 0x01 << i ) )
            {
              //  ERR( string(devices[i].path) + string(": not enough space on device ") + string(device( devices[i].id )))
                ERR( " - " + string(sizeStr( devices[i].space )) + string(" free )\n") );
            }
#endif
		LOG( "\n" );

		LOG( _f( "You need one device with %s free\n",
            sizeStr( total.estimate * ( f + w + s ) ).c_str() ) );

		if( job->prepare >= vobf || job->params & dvdStyler )
		{
			LOG( "\n" );
			LOG( _f( "or separate free areas of %s (\'dvdpath\')\n",
                sizeStr( total.estimate ).c_str() ) );
			LOG( _f( "                        + %s (\'workpath\'%s\n",
                sizeStr( total.estimate * ( job->params & cleanup ? 1 : 2 ) ).c_str(),
				job->params & cleanup ?
					( job->prepare >= isof ? " and \'isopath\', shared)" : ")" ) : ")" ) );
			if( job->prepare >= isof && ! ( job->params & cleanup ) )
				LOG( _f( "                        + %s (\'isopath\')\n",
                    sizeStr( total.estimate ).c_str() ) );

			if( w == n )
			{
				LOG( "\n" );
				LOG( "-setting \'cleanup\' to \'true\' will reduce required space to "
					<< sizeStr( total.estimate * 2 ) << ".\n" );
			}

			LOG( "\n" );
			LOG( "-you can redirect \'dvdpath\', \'workpath\', or \'isopath\' by either\n" );
            LOG( _f( " -editing %s\n", lplexConfig.filename().c_str() ) );
			LOG( " -or using the -w, -a, and -p command line options.\n" );
            if( job->projectPath != projectDotLplex )
                LOG( _f( " -or editing %s\n", job->projectPath.string().c_str() ) );
		}

		ECHO( endl << endl );

		if( ! editing )
		{
			exit( -1 );
		}
	}

	ECHO( "\n" );
 #endif
	return 0;
}



// ----------------------------------------------------------------------------
//    dvdLayout::configure :
// ----------------------------------------------------------------------------
//    Details layout alignments and reports layout characteristics.
//
//    Returns
// ----------------------------------------------------------------------------

#include <sstream>

int dvdLayout::configure()
{

#ifndef lplex_console
	if( job->update )
		update( Lfiles, infofiles, job );
#endif

	enum
	{
		padding = 0x1,
		loss = 0x2,
		discon = 0x4
	};

	int titleset = 100, note = 0;
	int32_t orphans = 0, audioFrames, audioLoss, audioGap, audioPadding;
	uint64_t audioBytes;
	uint32_t titleVframes = 0;

	total.audio = total.video = total.info = 0;

	for( uint i=0; i < Lfiles->size(); ++i )
	{
		lFile = &Lfiles->at(i);
		lFile->index = i;
		if( lFile->trim.type & jobs::autoSet )
		{
			lFile->trim.type >>= 4;
		}
		lFile->type = lpcmFile::untyped;

		audioBytes = flacHeader::bytesUncompressed( &lFile->fmeta );

		if( lFile->group != titleset )
		{
			setAudioUnits( &lFile->fmeta );
			titleset = lFile->group;
			titleVframes = 0;
			orphans = 0;
			lFile->type |= lpcmFile::titleStart;
		}

		if( i == Lfiles->size()-1
			|| (  i < Lfiles->size()-1  &&  (*Lfiles)[i+1].group != titleset ) )
		{
			lFile->type |= lpcmFile::titleEnd;
		}

		if( i < Lfiles->size()-1  &&  (*Lfiles)[i+1].trim.type != lFile->trim.type )
		{
			lFile->type |= lpcmFile::trimEnd;
		}
		if( i &&  (*Lfiles)[i-1].trim.type != lFile->trim.type )
		{
			lFile->type |= lpcmFile::trimStart;
		}
        if( lFile->trim.type & jobs::discrete )
		{
			lFile->type |= lpcmFile::trimStart;
		}


        if( lFile->trim.type & jobs::notrim )
		{
			lFile->trim.unit = dvdSampleSeam;
			lFile->trim.type <<= 4;
            lFile->trim.type |= jobs::discrete;
			note |= discon;
		}

        else if( lFile->trim.type & jobs::discrete )
		{
			lFile->trim.unit = dvdAudioFrame;
			note |= discon;
		}

		else if( lFile->type & lpcmFile::seqEnd )
		{
			lFile->trim.unit = dvdAudioFrame;
			lFile->trim.type <<= 4;
            lFile->trim.type |= jobs::discrete;
		}

		else
			lFile->trim.unit = dvdAVseam;

		if( ! ( lFile->type & lpcmFile::seqStart ) &&
				( lFile->trim.type & ( jobs::continuous | ( jobs::continuous << 4 ) ) ) )
			lFile->type |= lpcmFile::appended;

		lFile->trim.offset = -orphans;
												// calculate next forward trim point...
		lFile->trim.len =
			( audioBytes - lFile->trim.offset + lFile->trim.unit - 1 ) /
			lFile->trim.unit * lFile->trim.unit;

        if( lFile->trim.type & jobs::discrete )
			orphans = 0;

		else if( lFile->trim.len != audioBytes )
		{
			orphans = audioBytes - lFile->trim.offset - lFile->trim.len;
												// ...and send it backward if appropriate.
			if( lFile->trim.shift & jobs::backward ||
				( lFile->trim.shift & jobs::nearest && abs( orphans ) > ( lFile->trim.unit / 2 ) ) )
			{
				lFile->trim.len -= lFile->trim.unit;
				orphans += lFile->trim.unit;
			}
		}

		audioFrames = lFile->trim.len / dvdAudioFrame;
		if( (audioLoss = lFile->trim.len % dvdAudioFrame ))
			note |= loss;

												//One lpcm audio frame = 150 PTS ticks
												//One NTSC video frame = 3003 PTS ticks
												//One PAL video frame = 3600 PTS ticks
												//(One second = 90000 PTS ticks)
		lFile->videoFrames = ( job->tv == NTSC ?
			( ( audioFrames * 150 ) + 3002 ) / 3003 :
			( ( audioFrames * 150 ) + 3599 ) / 3600 );

		audioGap = lFile->videoFrames * ( job->tv == NTSC ? 3003 : 3600 )
			- audioFrames * 150;
        audioPadding = ( lFile->trim.type & jobs::seamless ? 0 :
			lFile->trim.len - ( audioBytes - lFile->trim.offset ) );
		if( audioPadding )
			note |= padding;


		if( lFile->type & lpcmFile::titleStart )
		{
			ECHO( "\n" );
			ECHO( ( titleset ? LOG_TAG : INFO_TAG ) <<
				_f( "Title %d - (%s / %s %s)\n", titleset + 1,
                lpcmEntity::audioInfo( lFile ).c_str(),
				jpegs[lFile->jpgIndex].sizeStr(),
				jpegs[lFile->jpgIndex].aspStr() ) );
			LOG ( "t:  original : realigned : offset :  pad   : loss :  gap   :     frames     :  timestamp  :jpg: file\n" );
			LOG ( ".:...........:...........:........:........:......:..pts...:....A...:...V...:.............:.#.:.........\n" );
		}

		char p[100];
		stringstream x;
		x << setw(10) << audioBytes << " :" << setw(10) << lFile->trim.len << endl;
		x.getline( p, 100 );

		lFile->details = _f( "%s:%s :%7d :%7d :%5d%s:%7d%s:%7d :%6d : %s :%2d",
            lFile->type & lpcmFile::seqEnd ? "e" : lFile->trim.type & jobs::discrete ? lFile->trim.type & jobs::padded ? "p" : "d" :
                lFile->trim.type & jobs::seamless ? "s" : "n" ,
			p,
			lFile->trim.offset,
			audioPadding,
			audioLoss,
				( audioLoss ? "*" : " " ),
			audioGap + (uint16_t)( 150 * ( (float)audioPadding / (float)dvdAudioFrame ) ),
                lFile->trim.type & ( jobs::discrete | jobs::notrim ) ? "+" : " ",
			audioFrames,
			lFile->videoFrames,
            timestamp( titleVframes, job->tv == NTSC ).c_str(),
			lFile->jpgIndex );

// 		LOG( lFile->details << " : " <<
//             ( editing ? lFile->fName.string() : lFile->fName.filename() ) << endl );

		if( lFile->type & lpcmFile::titleEnd )
		{
			LOG( _f( "[col 1] -trim units: s(AVseam)=%d d,e(audioFrame)=%d n(sampleSeam)=%d bytes.\n",
				dvdAVseam, dvdAudioFrame, dvdSampleSeam ) );
		}

		titleVframes += lFile->videoFrames;
		total.video += m2vEstimate( lFile/*->videoFrames*/, job->tv == NTSC );
		total.audio += ( lFile->trim.len + audioPadding );
	}

	ECHO( "\n" );
	LOG( "Legend:\n" );
	LOG( "[col 1] -trim types: (s)eamless (d)iscrete (e)ndOfSequence (p)added (n)one.\n" );
	if( note & discon )
		LOG( "(+)     -dvdauthor's STC discontinuity will add a half-second pause between chapters.\n" );
	if( note & loss )
		LOG( "(*)     -bytes will be discarded as 'incomplete final frame' by mplex.\n" );
	if( note & padding )
		LOG( "[gap]   -times include duration of any padding.\n" );
	ECHO( "\n" );

	INFO( "Screen Jpeg" << ( jpegs.size() > 1 ? "s" : "" ) << "\n" );
	for( uint i=0; i < jpegs.size(); ++i )
	{
		LOG( _f( "%2d (%s%s%s %s) : %s\n", i,
			jpegs[i].rescale ? "rescaled to " : "",
			( job->tv == NTSC ? "NTSC " : "PAL " ), jpegs[i].sizeStr(),
			jpegs[i].aspStr(),
            jpegs[i].fName.string().c_str() ) );
	}
	ECHO( "\n\n" );

	for( uint i=0; i < infofiles->size(); ++i )
		if( ! infofiles->at(i).reject )
            total.info += filesize( infofiles->at(i).fName.c_str() );

	for( uint i=0; i < menufiles->size(); ++i )
        total.info += filesize( menufiles->at(i).c_str() );

	checkSpace();
	return 0;
}



// ----------------------------------------------------------------------------
//    dvdLayout::getNext :
// ----------------------------------------------------------------------------
//    Incrementally reads in lpcm input data to next trimpoint and swaps to
//    dvd-order.
//
//    Returns 1 on success, 0 on no further input
// ----------------------------------------------------------------------------


int dvdLayout::getNext()
{
	if( ++writeIndex >= (int) Lfiles->size() )
			return 0;
	if( readIndex == -1 )
		readerNext();

	lpcmFile *writeFile = &Lfiles->at( writeIndex );
	uint16_t finished = false, nextReader = false, mid = false, samplePadding;
	uint32_t padding;
	counter<uint64_t> total, midCount;
	ofstream out;

	if( writeFile->type & lpcmFile::appended )
		job->now |= appending;
	else
		clearbits( job->now, appending );

	if( writeFile->type & lpcmFile::seqStart )
        nameNow = _f( "%s/%s_title_%02d-%02d", job->tempPath.string().c_str(),
            job->name.c_str(), writeFile->group + 1, writeFile->index );

	if( writeFile->type & lpcmFile::titleStart )
	{
//      SCRN( "\n" );
//      SCRN( INFO_TAG << TINT( _f( "Title %d - (%s / %s %s)\n\n",
        SCRN( "\n")
        SCRN(TINT( _f( "Title %d - (%s / %s %s)\n\n",
            writeFile->group + 1, lpcmEntity::audioInfo( writeFile ).c_str(),
			jpegs[writeFile->jpgIndex].sizeStr(),
            jpegs[writeFile->jpgIndex].aspStr() ).c_str() ) )
    }

	string txt;

    txt = writeFile->fName.filename().string();
	INFO( "Processing " << txt << "\n" );
	INFO( lpcmEntity::audioInfo( writeFile ) << " : formatting audio...\n" );
    SCRN( TINT( txt.c_str() ) )
	BLIP( " ...formatting audio " );

	out.open( nameNow + ".lpcm", job->now & appending ?
		( writeFile->type & lpcmFile::titleStart ?
			ios::binary : ios::binary | ios::app ) :
		ios::binary );

	if( ! out.is_open() )
        FATAL( "Can't open output file " + nameNow );

	total.start = total.now = padding = samplePadding = 0;
	total.max = writeFile->trim.len;
	midCount.start = midCount.now = midCount.max = 0;

	md5_init( &md5sum );

	while( reader->fillBuf( total.max - total.now , mid ? &midCount : NULL ) )
	{
#ifndef lplex_console
		wxYieldIfNeeded();
#endif
		mid = false;
											// if at trimpoint
		if ( total.now + reader->ct.now == total.max )
			finished = true;

											// if at _eof
		if( reader->state & lpcmEntity::_eof )
		{
            if( writeIndex == readIndex && writeFile->trim.type & jobs::discrete )
				finished = true;

			if( finished )
			{
				padding = writeFile->trim.len - ( total.now + reader->ct.now );
				if( (samplePadding = ( total.now + reader->ct.now ) %
					sampleSeam( &reader->fmeta ) ))
				{
					memset( bigBlock + reader->ct.now, '\0', samplePadding );
					padding -= samplePadding;
				}
				nextReader = true;
			}

			else
			{
											// continue into next file
				midCount = reader->ct;
				mid = true;
				readerNext();
				continue;
			}
		}
											// process the data
		md5_append( &md5sum, bigBlock, reader->ct.now );

		reader->swap2dvd( bigBlock, reader->ct.now + samplePadding,
			reader->fmeta.data.stream_info.channels,
			reader->fmeta.data.stream_info.bits_per_sample );

		out.write( (char*)bigBlock, reader->ct.now + samplePadding );

		total.now += reader->ct.now;

		if( finished )
			break;

		blip( &total, 1, _verbose ? "done" : "", STAT_TAG );
	}

	md5_finish( &md5sum, (md5_byte_t*) &writeFile->fmeta.data.stream_info.md5sum );
	writeFile->fmeta.data.stream_info.total_samples = total.now / (
		writeFile->fmeta.data.stream_info.channels *
		writeFile->fmeta.data.stream_info.bits_per_sample / 8 );

	if( padding )
	{
		memset( bigBlock, '\0', padding );
		out.write( (char*)bigBlock, padding );
	}
	out.close();

	if( nextReader )
	{
		readerNext();
		nextReader = false;
	}

	writeFile->trim.len = flacHeader::bytesUncompressed( &writeFile->fmeta );
	writeFile->trim.offset = writeFile->type & lpcmFile::seqEnd ?
		0 : Lfiles->at( writeIndex + 1 ).trim.offset;
	return 1;
}



// ----------------------------------------------------------------------------
//    dvdauthorXml::timestampFractional :
// ----------------------------------------------------------------------------
//    Returns hh:mm:ss.frac rounded timestamp string, as used by dvdauthor.
//
//    Arguments:
//       <f>         - absolute frame offset
//       <ntsc>      - whether tv system is ntsc
//       <boost>     - amount to inflate the timestamp
// ----------------------------------------------------------------------------


string dvdauthorXml::timestampFractional( uint32_t f, bool ntsc, float boost )
{
	uint16_t hour, min;
	uint64_t ticks = f * ( ntsc ? 3003 : 3600 );
	double sec = (double)ticks / 90000;

	hour = (int)sec / 3600;
	min = ( (int)sec % 3600 ) / 60;
	sec -= ( (int)sec / 60 * 60 );

	return _f( "%02d:%02d:%05.3f", hour, min, sec + boost );
}



// ----------------------------------------------------------------------------
//    dvdauthorXml::write :
// ----------------------------------------------------------------------------
//    Incrementally writes an xml config file for input to either dvdauthor or
//    dvdstyler.
//
//    Arguments:
//       <context>   - context of current call
//       <str>       - string to process depending on <context>.
//
//    Returns 0 on success, fatal on failure
// ----------------------------------------------------------------------------


int dvdauthorXml::write( xmlContext context, const string& str, int flag )
{
	static int chapter, titleset=1;

	if( disabled )
		return 0;

	switch( context )
	{
		case open:
            xml.open(str, ofstream::out | ofstream::trunc);

			if (! xml.is_open ())
                cerr << "[ERR] Could not open xml" << endl;
			break;

		case setDest:
			xml

			<< "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

			if( ! dvdStyler )
				xml << "<dvdauthor dest=" << "\"" << str << "\"" << ">\n\n";
			else
#if 0
                xml << _f( "<dvdstyler name=\"%s\">\n\n", fs::path( str ).stem().c_str() );
#else
//            xml << _f( "<dvdstyler name=\"%s\" videoFormat=\"%d\" aspectRatio=\"1\" audioFormat=\"%d\" videoBitrate=\"-1\" capacity=\"0\" emptyMenu=\"1\" format=\"4\" jumppad=\"1\">\n\n",
				xml << _f( "<dvdstyler name=\"%s\" videoFormat=\"%d\" aspectRatio=\"%d\">\n\n",
                    fs::path( str ).stem().c_str(), video == NTSC ? 3 : 2, flag ? 2 : 1 );
#endif
			if( titlesets > 1 || dvdStyler )
			{
				xml << "\t<vmgm>\n"
						<< "\t\t<menus>\n"
						<< _f("\t\t\t<video format=\"%s\"/>\n", video == NTSC ? "ntsc" : "pal")
						<< "\t\t\t<pgc>\n"
#if 0
						<< ( dvdStyler ? "\t\t\t\t<vob><menu></menu></vob>\n" : "" )
#else

						<< ( dvdStyler ? _f( "\t\t\t\t<vob><menu videoFormat=\"%s\"></menu></vob>\n",
                            video == NTSC ? "NTSC" : "PAL" ).c_str() : "" )
#endif
						<< "\t\t\t\t<post>";

				for( int i=titlesets; i>1; i-- )
					xml << _f( "if(g1 eq %d){jump titleset %d menu;} ",
						i, i );
				xml
				<< "g1=1; jump titleset 1 menu;"
				<< "</post>\n"
				<< "\t\t\t</pgc>\n"
				<< "\t\t</menus>\n"
				<< "\t</vmgm>\n\n";
			}

			else
//            xml << "\t<vmgm />\n\n";
				// nonsense, but otherwise dvdauthor 0.7.0 fails with "no video format specified for VMGM"
				xml << _f("\t<vmgm><menus><video format=\"%s\"/></menus></vmgm>\n\n", video == NTSC ? "ntsc" : "pal");

			xml
			<< "\t<titleset>\n";

			if( titlesets > 1 || dvdStyler )
			{
				xml
				<< "\t\t<menus>\n"
				<< _f("\t\t\t<video format=\"%s\"/>\n", video == NTSC ? "ntsc" : "pal")
				<< "\t\t\t<pgc>\n"
#if 0
				<< ( dvdStyler ? "\t\t\t\t<vob><menu></menu></vob>\n" : "" )
#else
						<< ( dvdStyler ? _f( "\t\t\t\t<vob><menu videoFormat=\"%s\"></menu></vob>\n",
                            video == NTSC ? "NTSC" : "PAL" ).c_str() : "" )
#endif
				<< "\t\t\t\t<post>"
				<< _f( "if(g1 eq %d){g1=%d; jump title 1;} jump vmgm menu;",
					titleset, titleset + 1 )
				<< "</post>\n"
				<< "\t\t\t</pgc>\n"
				<< "\t\t</menus>\n";
			}

			xml
			<< "\t\t<titles>\n"
			<< _f("\t\t\t<video format=\"%s\" aspect=\"%s\"/>\n", video == NTSC ? "ntsc" : "pal", flag ? "16:9" : "4:3" )
//         << ( flag ? "\t\t\t<video aspect=\"16:9\"/>\n" : "" )
//TODO:<audio format="pcm" channels="2" quant="24bps" samplerate="96khz" />
			<< "\t\t\t<pgc>\n";

			chapter = 0;
			break;

		case addTitle:
			xml
			<< "\t\t\t\t<post>call vmgm menu;</post>\n"
			<< "\t\t\t</pgc>\n"
			<< "\t\t</titles>\n"
			<< "\t</titleset>\n\n"

			<< "\t<titleset>\n";

			titleset++;

			xml
			<< "\t\t<menus>\n"
			<< _f("\t\t\t<video format=\"%s\"/>\n", video == NTSC ? "ntsc" : "pal")
			<< "\t\t\t<pgc>\n"
#if 0
			<< ( dvdStyler ? "\t\t\t\t<vob><menu></menu></vob>\n" : "" )
#else
						<< ( dvdStyler ? _f( "\t\t\t\t<vob><menu videoFormat=\"%s\"></menu></vob>\n",
                            video == NTSC ? "NTSC" : "PAL" ).c_str() : "" )
#endif
			<< "\t\t\t\t<post>"
			<< _f( "if(g1 eq %d){g1=%d; jump title 1;} jump vmgm menu;",
				titleset, titleset + 1 )
			<< "</post>\n"
			<< "\t\t\t</pgc>\n"
			<< "\t\t</menus>\n";

			xml
			<< "\t\t<titles>\n"
			<< _f("\t\t\t<video format=\"%s\" aspect=\"%s\"/>\n", video == NTSC ? "ntsc" : "pal", flag ? "16:9" : "4:3" )
 //        << ( flag ? "\t\t\t<video aspect=\"16:9\"/>\n" : "" )
			<< "\t\t\t<pgc>\n";
			chapter = 0;
			break;

		case openVob:
			xml
			<< "\t\t\t\t<vob file=" << "\"" << str << "\""
			<< ( dvdStyler ? "\n\t\t\t\t\tdoNotTranscode=\"1\"" : "" );
			break;

		case addChapter:
			xml
			<< ( chapter ? "," : "\n\t\t\t\t\tchapters=\"" )
			<< str;
			chapter = 1;
			break;

		case closeVob:
			xml
			<< ( chapter ? "\"\n\t\t\t\t" : " " )
			<< "/>\n";
			chapter = 0;
			break;

		case close:
			xml
			<< "\t\t\t</pgc>\n"
			<< "\t\t</titles>\n"
			<< "\t</titleset>\n\n"

			<< ( dvdStyler ? "</dvdstyler>\n" : "</dvdauthor>\n" );

#ifdef _ERR2LOG
	//		xml.seekg( 0 );
		//	xlog << xml.rdbuf() << endl;
#endif

			xml.flush();
            xml.close();
			break;

		default:
			break;
	}

	return 0;
}

#if 0
void writeStyler()
{
	for( int i=0; i < Lfiles->size(); i++ )
	{
		lFile = &Lfiles->at(i);
	}
}
#endif

