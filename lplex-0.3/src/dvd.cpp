/*
	dvd.cpp - dvd navigation using libdvdread.
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


#include "dvd.hpp"


// ----------------------------------------------------------------------------
//    lpcm_video_ts::TVformat :
// ----------------------------------------------------------------------------
//    Returns 0 (=NTSC) 1 (=PAL) 2 (=MIXED), otherwise undefined.
// ----------------------------------------------------------------------------


int lpcm_video_ts::TVformat( ifo_handle_t *ifo )
{
	int fmt = 0, f;

	if( ifo->vmgi_mat )
	{
		if( ( f = ifo->vmgi_mat->vmgm_video_attr.video_format ) < 2 )
			fmt |= ++f;

		for( int i = 0; i < ifo->vts_atrt->nr_of_vtss; i++ )
		{
			if( ( f = ifo->vts_atrt->vts[i].vtstt_vobs_video_attr.video_format ) < 2 )
				fmt |= ++f;
		}
	}

	if( ifo->vtsi_mat )
	{
		if( ( f = ifo->vtsi_mat->vtsm_video_attr.video_format ) < 2 )
			fmt |= ++f;
	}

	return --fmt;
}


// ----------------------------------------------------------------------------
//    lpcm_video_ts::getPGC :
// ----------------------------------------------------------------------------
//    Advances PGC by PGC, stream by stream, title by title, to the next
//    Program Chain with lpcm on the dvd.
//
//    Returns 1 on success, 0 if no more PGCs
// ----------------------------------------------------------------------------


int lpcm_video_ts::getPGC()
{
	if( ++pgc == numPGCs )
	{
		if( ! getAudioStream() )
			return 0;
		else
			pgc = 0;
	}

	if( verbose ) INFO( _f( "Program chain %02d\n", pgc + 1 ) );

	pgct = ifo->vts_pgcit->pgci_srp[pgc].pgc;
	numPGs = pgct->nr_of_programs;
	numPGCcells = pgct->nr_of_cells;
	pgcCell = pg = -1;

	configurePGC();

	return 1;
}



// ----------------------------------------------------------------------------
//    ifoPrint_audio_attributes :
// ----------------------------------------------------------------------------
//    Lifted directly from 'dvdread/ifo_print.c' and modifed to return a
//    character string instead of printing to the console.
// ----------------------------------------------------------------------------


string ifoPrint_audio_attributes( audio_attr_t *attr )
{
    string info;

	if(attr->audio_format == 0
			&& attr->multichannel_extension == 0
			&& attr->lang_type == 0
			&& attr->application_mode == 0
			&& attr->quantization == 0
			&& attr->sample_frequency == 0
			&& attr->channels == 0
			&& attr->lang_code == 0
			&& attr->lang_extension == 0
			&& attr->code_extension == 0
			&& attr->unknown3 == 0
			&& attr->unknown1 == 0) {
		info += "(unspecified)";
        return "";
	}

	switch(attr->audio_format) {
	case 0:
		info += "ac3 ";
		break;
	case 1:
//      info += "(please send a bug report) ";
		break;
	case 2:
		info += "mpeg1 ";
		break;
	case 3:
		info += "mpeg2ext ";
		break;
	case 4:
		info += "lpcm ";
		break;
	case 5:
//      info += "(please send a bug report) ";
		break;
	case 6:
		info += "dts ";
		break;
	default:
//      info += "(please send a bug report) ";
		break;
	}

	if(attr->multichannel_extension)
		info += "multichannel_extension ";

	switch(attr->lang_type) {
	case 0:
		// not specified
//      CHECK_VALUE(attr->lang_code == 0 || attr->lang_code == 0xffff);
		break;
	case 1:
		info += _f("%c%c ", attr->lang_code>>8, attr->lang_code & 0xff);
//      info += _f("%c%c (%c) ", attr->lang_code>>8, attr->lang_code & 0xff,
//                attr->lang_extension ? attr->lang_extension : ' ');
//      if(attr->lang_extension) {
//         info += "(please send a bug report) lang_extension != 0";
//      }
		break;
	default:
//      info += "(please send a bug report) ";
		break;
	}

	switch(attr->application_mode) {
	case 0:
		// not specified
		break;
	case 1:
		info += "karaoke ";
		break;
	case 2:
		info += "surround_sound ";
		break;
	default:
//      info += "(please send a bug report) ";
		break;
	}

	switch(attr->audio_format) {
	case 0: //ac3
//      if(attr->quantization != 3) {
//         info += _f("(please send a bug report) ac3 quant/drc not 3 (%d)",
//          attr->quantization);
//      }
		break;
	case 2: //mpeg 1 or mpeg 2 without extension stream
	case 3: //mpeg 2 with extension stream
		switch(attr->quantization) {
		case 0: //no drc
//         info += "no drc ";
			break;
		case 1:
			info += "drc ";
			break;
		default:
//         info += _f("(please send a bug report) mpeg reserved quant/drc   (%d)",
//          attr->quantization);
			break;
		}
		break;
	case 4:
		switch(attr->quantization) {
		case 0:
			info += "16 bit ";
			break;
		case 1:
			info += "20 bit ";
			break;
		case 2:
			info += "24 bit ";
			break;
		case 3:
//         info += _f("(please send a bug report) lpcm reserved quant/drc   (%d)",
//          attr->quantization);
			break;
		}
		break;
	case 6: //dts
//      if(attr->quantization != 3) {
//         info += _f("(please send a bug report) dts quant/drc not 3 (%d)",
//          attr->quantization);
//      }
		break;
	default:
		break;
	}

	switch(attr->sample_frequency) {
	case 0:
		info += "48 kHz ";
		break;
	case 1:
		info += "96 kHz ";
		break;
	default:
//      info += _f("sample_frequency %i (please send a bug report) ",
//       attr->sample_frequency);
		break;
	}

	info += _f("%d Ch ", attr->channels + 1);

	switch(attr->code_extension) {
	case 0:
//      info += "Not specified ";
		break;
	case 1: // Normal audio
		info += "Normal Caption ";
		break;
	case 2: // visually imparied
		info += "Audio for visually impaired ";
		break;
	case 3: // Directors 1
		info += "Director's comments 1 ";
		break;
	case 4: // Directors 2
		info += "Director's comments 2 ";
		break;
		//case 4: // Music score ?
	default:
//      info += "(please send a bug report) ";
		break;
	}

//   info += _f("%d ", attr->unknown3);
	if(attr->application_mode == 1) {
		info += _f("ca=%d ", attr->app_info.karaoke.channel_assignment);
		info += _f("%d ", attr->app_info.karaoke.version);
		if(attr->app_info.karaoke.mc_intro)
			info += "mc intro ";
		info += _f("%s ", attr->app_info.karaoke.mode ? "duet" : "solo" );
		info += _f("%d ", attr->app_info.karaoke.unknown4);
	}
	if(attr->application_mode == 2) {
		if(attr->app_info.surround.dolby_encoded) {
			info += "dolby surround ";
		}
		info += _f("%d ", attr->app_info.surround.unknown5);
		info += _f("%d ", attr->app_info.surround.unknown6);
	}

	return info;
}



// ----------------------------------------------------------------------------
//    dvd_vframes :
// ----------------------------------------------------------------------------
//    Converts dvd_time_t to equivalent length in video frames
// ----------------------------------------------------------------------------


#define base16(x) x/16*10+x%16

uint32_t dvd_vframes( dvd_time_t * dvd_time )
{
	int r = ((int[]){ 0, 25, 0, 30 })[dvd_time->frame_u>>6];
	int h = base16( dvd_time->hour );
	int m = base16( dvd_time->minute );
	int s = base16( dvd_time->second );
	int f = base16( ( (dvd_time->frame_u|0xc0)^0xc0) );

	return r * ( h * 3600 + m * 60 + s ) + f;
}



// ----------------------------------------------------------------------------
//    dvd_aframes :
// ----------------------------------------------------------------------------
//    Converts dvd_time_t to equivalent rounded-up length in audio frames
// ----------------------------------------------------------------------------


uint32_t dvd_aframes( dvd_time_t * dvd_time )
{
	double r = (double)((int[]){ 0, 25, 0, 30 })[dvd_time->frame_u>>6];
	return ceil( (double)dvd_vframes( dvd_time ) / r * 600 );
}




// ----------------------------------------------------------------------------
//    lpcm_video_ts::getAudioStream :
// ----------------------------------------------------------------------------
//    Advances stream by stream, title by title, to the next lpcm audio stream
//    on the dvd.
//
//    Returns 1 on success, 0 if no more audio streams
// ----------------------------------------------------------------------------


int lpcm_video_ts::getAudioStream()
{
	while( 1 )
	{
		while( ++audioStream < numAudioStreams )
		{
			if( verbose )
				INFOv( _f( "Title %02d audio stream %d: %s\n", titleset, audioStream,
                    ifoPrint_audio_attributes( &ifo->vtsi_mat->vts_audio_attr[audioStream] ).c_str() ) );

											//...if another lpcm stream, repeat
			if( ifo->vtsi_mat->vts_audio_attr[audioStream].audio_format == 4 )
			{
				lpcm_id++;
				fmeta.data.stream_info.sample_rate =
					ifo->vtsi_mat->vts_audio_attr[audioStream].sample_frequency * 48000 + 48000;
				fmeta.data.stream_info.channels =
					ifo->vtsi_mat->vts_audio_attr[audioStream].channels + 1;
				fmeta.data.stream_info.bits_per_sample =
					ifo->vtsi_mat->vts_audio_attr[audioStream].quantization * 4 + 16;
				audioframe = dvdUtil::audioFrame( &fmeta );
				numPGs = numPGCcells = 0;
				pgc = pgcCell = pg = -1;

				configureAudioStream();
				return 1;
			}
		}
		ifoClose( ifo );

		if( ++titleset <= numTitlesets )
		{
			ifo = ifoOpen( libdvdReader, titleset );
			if( ! ifo )
				FATAL( "Unable to open ifo file." );
		}
		else
			break;
											//...start next titleset
		if( verbose ) ECHO( "\n" );

		if( ! ifo->vts_c_adt )
			ERR( "no address table found \n" );


		cells = ifo->vts_c_adt->cell_adr_table;
		numCells = ( ifo->vts_c_adt->last_byte + 1 - C_ADT_SIZE ) /
			sizeof( cell_adr_t );
		numAudioStreams = ifo->vtsi_mat->nr_of_vts_audio_streams;
		numPGCs = ifo->vts_pgcit->nr_of_pgci_srp;

		audioStream = -1;
		lpcm_id = 0xa0 - 1;

		if( verbose ) INFO( _f( "Opening title %02d (%d audio stream%s)\n",
			titleset, numAudioStreams, numAudioStreams == 1 ? "" : "s" ) );
	}

//   DVDClose( libdvdReader );
	return 0;
}


#ifdef dvdread_udflist

// ----------------------------------------------------------------------------
//    lpcm_video_ts::udfItem :
// ----------------------------------------------------------------------------
//    udfLister "callback" function for reporting filenames, returns 0.
// ----------------------------------------------------------------------------


int lpcm_video_ts::udfItem( const char *fname, uint16_t ftype, uint32_t lb, uint32_t len )
{
	INFO( _f( "[%d] %8lx: %10ld - %s\n",
		ftype, lb * DVD_VIDEO_LB_LEN, lb, fname ) );

	return 0;
}

#endif

// ----------------------------------------------------------------------------
//    lpcm_video_ts::open :
// ----------------------------------------------------------------------------
//    Opens a dvd filestructure and advances to the first lpcm audio stream,
//    if any.
//
//    Returns 1 on success, 0 if no lpcm audio streams
// ----------------------------------------------------------------------------


int lpcm_video_ts::open( const char * VIDEO_TS, bool fatal )
{
	if( isOpen() )
		close();

	// suppress console output during libdvdread calls
	putenv( const_cast<char*>("DVDREAD_VERBOSE=0" ));
	putenv( const_cast<char*>("DVDCSS_VERBOSE=0" ));
	FILE* res = freopen( NUL, "wt", stderr );
    if (res == nullptr)
    {
        cerr << "[ERR] Cannot open /dev/null" << endl;
        throw;
    }

	libdvdReader = DVDOpen( VIDEO_TS );
	res = freopen( CON, "wt", stderr );
    if (res == nullptr)
    {
        cerr << "[ERR] Cannot open console" << endl;
        throw;
    }
	if( ! libdvdReader )
	{
		if( fatal )
			FATAL( _f( "Can't open dvd files in \'%s\'.", VIDEO_TS ) );
		return 0;
	}

	res = freopen( NUL, "wt", stderr );
    if (res == nullptr)
    {
        cerr << "[ERR] Cannot open /dev/null" << endl;
        throw;
    }
	ifo = ifoOpen( libdvdReader, 0 );
	res = freopen( CON, "wt", stderr );
    if (res == nullptr)
    {
        cerr << "[ERR] Cannot open console" << endl;
        throw;
    }
	if( ! ifo )
	{
		if( fatal )
			FATAL( "Unable to open ifo file." );
		return 0;
	}

	tv = TVformat( ifo );
	fName = VIDEO_TS;

    label = fName.stem().string();
	if( label == "" || label == "VIDEO_TS" )
		label = volumeLabel( VIDEO_TS, true );

	if( verbose )
	{
        SCRN( string(INFO_TAG) + string("Opening dvd '"))
        SCRN(TINT( label.c_str() ))
        SCRN( "'.\n" )
        INFO( _f( "Opening dvd '%s'.\n", label.c_str() ) );
	}

	state |= opened;

	// **hack**: dvd_reader_t is an opaque pointer to undefined type dvd_reader_s
	// here, however 'int isImageFile' happens to be its first member.

	if( *(int*)libdvdReader )
	{
		isImage = true;
#ifdef dvdread_udflist
		infofiles.clear();
		udfList( libdvdReader, this );
		udfPrint();
#endif
	}
	else
		isImage = false;

	numTitlesets = ifo->vmgi_mat->vmg_nr_of_title_sets;
	menuFmt = ifo->vmgi_mat->vmgm_video_attr.video_format;

	titleset = 0;
	audioStream = pgc = pg = pgcCell = c = -1;
	numAudioStreams = numCells = numPGCs = numPGs = numPGCcells = -1;

	if( ! getAudioStream() )
	{
		if( fatal )
			FATAL( "No lpcm audio found on dvd." );

		if( verbose )
			INFO( "No lpcm audio found on dvd.\n" );
		return 0;
	}
	return 1;
}


// ----------------------------------------------------------------------------
//    lpcm_video_ts::cellAt :
// ----------------------------------------------------------------------------
//    Returns a pointer to the cell address table entry matching the vob
//    and cell ids specified by the given cell position entry, or nullptr on fail.
//
//    Arguments:
//       <pos>    - pointer to cell position entry to match
// ----------------------------------------------------------------------------


cell_adr_t* lpcm_video_ts::cellAt( cell_position_t *pos )
{
	return cellAt( pos->vob_id_nr, pos->cell_nr );
}



// ----------------------------------------------------------------------------
//    lpcm_video_ts::cellAt :
// ----------------------------------------------------------------------------
//    Returns a pointer to the cell address table entry matching the given
//    <vob_id> and <cell_id>, or nullptr on fail.
// ----------------------------------------------------------------------------


cell_adr_t* lpcm_video_ts::cellAt( int vob_id, int cell_id )
{
	numCells = ( ifo->vts_c_adt->last_byte + 1 - C_ADT_SIZE ) /
		sizeof( cell_adr_t );

	for( int i=0; i < numCells; i++ )
	{
		if( cells[i].vob_id == vob_id )
			if( cells[i].cell_id == cell_id )
				return &cells[i];
	}
	return nullptr;
}



// ----------------------------------------------------------------------------
//    lpcm_video_ts::cellIndex :
// ----------------------------------------------------------------------------
//    Returns the index of the cell address table entry matching the given
//    <vob_id> and <cell_id>, or -1 on fail.
// ----------------------------------------------------------------------------


int lpcm_video_ts::cellIndex( int vob_id, int cell_id )
{
	for( int i=0; i < numCells; i++ )
	{
		if( cells[i].vob_id == vob_id )
			if( cells[i].cell_id == cell_id )
				return i;
	}
	return -1;
}



// ----------------------------------------------------------------------------
//    lpcmPGtraverser::getCell :
// ----------------------------------------------------------------------------
//    Advances cell by cell, program by program, to the end of the current PGC,
//    and optionally PGC by PGC, stream by stream, title by title, to the next
//    Program Cell with lpcm on the dvd.
//
//    Arguments:
//       <searchBeyond>    - whether to continue beyond the current PGC
//
//    Returns context of new cell, i.e. whether first in program
// ----------------------------------------------------------------------------


int lpcmPGtraverser::getCell( bool searchBeyond )
{
	int context;
											// check if last cell in program chain
	if( ++pgcCell == numPGCcells )
	{
		if( searchBeyond )
		{
			if( ! getPGC() )
				return 0;
			else
				pg = pgcCell = 0;
		}
		else
			return 0;
	}
											// indicate if first cell in program
	if( pgct->program_map[pg] == pgcCell + 1 )
	{
		idNow = titleset * 100000 + audioStream * 10000
			+ ( pgc + 1 )* 100 + ( pg + 1 );

		nameNow =_f( "%02d_%d_%02d_%02d", titleset, audioStream, pgc + 1, pg + 1 );

		pg++;
		context = _isNew;
	}
											// ...or if continuing a multi-cell program
	else
		context = _reopen;
											// point to actual cell entry in address table
	cell = cellAt( &pgct->cell_position[ pgcCell ] );

	return context;
}



// ----------------------------------------------------------------------------
//    lpcmPGextractor::getCell :
// ----------------------------------------------------------------------------
//    Advances cell by cell, program by program, to the end of the current PGC,
//    and optionally PGC by PGC, stream by stream, title by title, to the next
//    lpcm Program Cell on the dvd, skipping any cells previously marked as
//    containing no audio.
//
//    Arguments:
//       <searchBeyond>    - whether to continue beyond the current PGC
//
//    Returns context of new cell, i.e. whether first in program
// ----------------------------------------------------------------------------


int lpcmPGextractor::getCell( bool searchBeyond )
{
	int context;

	if( state & traversed )
		return getCellTraversed();

	while( (context = lpcmPGtraverser::getCell( searchBeyond )) )
	{
		if( audioCells[ pgcCell ].xIndex != -1 )
			return context;
	}

	return 0;
}


// ----------------------------------------------------------------------------
//    lpcmPGextractor::traverse :
// ----------------------------------------------------------------------------
//    Opens dvd if necessary, traverses the entire dvd structure, and resets
//    the current position to the beginning.
//
//    Returns true
// ----------------------------------------------------------------------------


bool lpcmPGextractor::traverse()
{
	if( ! ( state & opened ) )
		open();

	job->tv = tv;
	while( getCell( true ) ) { blip(); }

	state |= traversed;
	pgcIndex = pgcCell = -1;
	numPGCcells = 0;

	return true;
}


// ----------------------------------------------------------------------------
//    lpcmPGextractor::getCellTraversed :
// ----------------------------------------------------------------------------
//    Advances through the pre-read audio table entries cell by cell, program
//    by program, PGC by PGC, stream by stream, title by title, to the next
//    lpcm Program Cell on the dvd.
//
//    Returns context of new cell, i.e. whether first in program
// ----------------------------------------------------------------------------


int lpcmPGextractor::getCellTraversed()
{
	int context;

	if( ++pgcCell == numPGCcells )
	{
		if( ++pgcIndex < (int) audioTables.size() )
		{
			audioCells = audioTables.at( pgcIndex ).audioCells;
			titleset = audioTables.at( pgcIndex ).titleset;
			lpcm_id = audioTables.at( pgcIndex ).lpcm_id;
			memcpy( &fmeta, &audioTables.at( pgcIndex ).fmeta, sizeof( FLAC__StreamMetadata ) );
			audioframe = audioTables.at( pgcIndex ).audioframe;
			audioStream = audioTables.at( pgcIndex ).audioStream;
			pgc = audioTables.at( pgcIndex ).pgc;
			numPGCcells = audioTables.at( pgcIndex ).numPGCcells;
			pg = pgcCell = 0;
#if DEBUG
DBUG( "pgcIndex=" << pgcIndex );
for( int c=0; c < numPGCcells + 1; c++ )
{
DBUG( "cell=" << c << " : start=" << audioCells[c].start_sector
<< "(pts " << audioCells[c].start_pts << ")"
<< " last=" << audioCells[c].last_sector
<< " xIndex=" << audioCells[c].xIndex );
}
#endif
		}
		else
			return 0;
	}

	if( ! audioCells[ pgcCell ].secondary )
	{
		idNow = titleset * 100000 + audioStream * 10000	+ ( pgc + 1 )* 100 + ( pg + 1 );
		nameNow =_f( "%02d_%d_%02d_%02d", titleset, audioStream, pgc + 1, pg + 1 );
		pg++;
		context = _isNew;
	}
											// ...or if continuing a multi-cell program
	else
		context = _reopen;

	return context;
}



// ----------------------------------------------------------------------------
//    lpcmPGextractor::nextAudioCellPts :
// ----------------------------------------------------------------------------
//    Retrieves next valid pts value in audio table after index <c>
// ----------------------------------------------------------------------------


uint32_t lpcmPGextractor::nextAudioCellPts( uint16_t c )
{
	for( ++c; c < numPGCcells; c++ )
	{
		if( audioCells[c].xIndex != -1 )
			return audioCells[c].start_pts;
	}

	return 0;
}



// ----------------------------------------------------------------------------
//    lpcmPGextractor::getWriter :
// ----------------------------------------------------------------------------
//    Closes previous writer and opens writer assigned to program cell at the
//    given Index.
//
//    Arguments:
//       <writeIndex>      - program cell index
//       <context>         - cell position context
//
//    Returns pointer to new writer on success, nullptr if index is out of range
// ----------------------------------------------------------------------------


lpcmWriter* lpcmPGextractor::getWriter( int writeIndex, int context )
{
	static int lastIndex = -1;
    string txt;

	if( writeIndex < numPGCcells )
	{
		lpcmFile *lFile = &Lfiles->at( audioCells[writeIndex].xIndex );

		if( context == _isNew )
		{
			if( lastIndex > -1 )
				Lfiles->at( lastIndex ).writer->close();

			if( lFile->format == wavef )
				lFile->writer = new waveWriter();
			else if( lFile->format == flacf )
				lFile->writer = new flacWriter( job->flacLevel );
			else if( lFile->format == lpcmf )
				lFile->writer = new rawWriter();
			lFile->writer->ct.start = lFile->writer->ct.now
				= lFile->writer->ct.max = 0;
			lFile->writer->trim.len = lFile->writer->trim.offset =
				lFile->writer->trim.unit = 0;

											// ...and initialize as appropriate
			if( lFile->state & lpcmEntity::specified )
			{
                lFile->writer->fName = job->extractPath / lFile->fName;
                if ( ! editing && ! fs::exists( lFile->writer->fName.parent_path() ) )
                    fs_MakeDirs( lFile->writer->fName.parent_path() );

				lFile->writer->state = lFile->state;
				memcpy( &lFile->writer->fmeta, &lFile->fmeta, sizeof( FLAC__StreamMetadata ) );

				if( state & lplexAuthored && job->params & md5 )
				{
					lFile->writer->state |= lpcmEntity::metered;
					if( job->params & restore )
					{
						lFile->writer->ct.max = lFile->trim.len;
						memcpy( &lFile->writer->fmeta.data.stream_info.md5sum, &lFile->md5str, 16 );
					}

					else
					{
						lFile->writer->ct.max =
							flacHeader::bytesUncompressed( &lFile->fmeta );
					}
				}
				else
					lFile->writer->ct.max = lFile->trim.len;


				if( editing )
                    lFile->writer->fName = lFile->writer->fName.string() +
						((const char*[]){ ".wav", ".flac ", ".lpcm" }) [ lFile->format-1 ];
				else
					lFile->writer->open();
			}

			txt = ( state & lplexAuthored ?  job->params & restore ?
				"Restoring " : "Extracting " : "Extracting " );
		}
		else
			txt = "Continuing ";

		ECHO( "\n" );
        STAT( txt << lFile->fName.string().substr( lFile->root ) << endl );

		if( state & lplexAuthored )
		{
			INFO( lpcmEntity::audioInfo( lFile->writer ) );
			ECHO( " (" << lFile->writer->ct.max << " audio bytes)\n" );
		}

		SCRN( "\n" );
        SCRN( string(STAT_TAG) + txt)
        SCRN(TINT( lFile->writer->fName.filename().string().c_str() ))
        SCRN( "... " )

		lastIndex = audioCells[writeIndex].xIndex;
		return lFile->writer;
	}

	return nullptr;
}



// ----------------------------------------------------------------------------
//    lpcmPGextractor::configurePGC :
// ----------------------------------------------------------------------------
//    Initializes layout of current PGC for extraction, creates a table of
//    audio start/end sectors and start pts, and configeres each cell in PGC
//    if mode is not singleStep.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


int lpcmPGextractor::configurePGC()
{
	int found = 0, context;
	prevOffset = 0;
											// set up audio table
	audioTableData data;
	data.titleset = titleset;
	data.lpcm_id = lpcm_id;
	data.audioStream = audioStream;
	data.pgc = pgc;
//   data.pg = pg;
//   data.pgcCell = pgcCell;
	data.numPGCcells = numPGCcells;
	memcpy( &data.fmeta, &fmeta, sizeof( FLAC__StreamMetadata ) );
	data.audioframe = audioframe;

	audioTables.push_back( data );
	audioTables.back().audioCells = new audio_adr_t[ numPGCcells+1 ];
	audioCells = audioTables.back().audioCells;

	pgcIndex++;

	audioCells[ numPGCcells-1 ].last_sector =
		cellAt( &pgct->cell_position[ numPGCcells-1 ] )->last_sector;

	audioCells[numPGCcells].start_sector =
		audioCells[numPGCcells].last_sector =
		audioCells[numPGCcells].start_pts = 0;
	audioCells[numPGCcells].xIndex = -1;

	vob = DVDOpenFile( libdvdReader, titleset, DVD_READ_TITLE_VOBS );
	if( ! vob )
		FATAL( "Unable to open DVD." );

	pg = 0; pgcCell = -1;

	if( state & singleStep )
		return 0;

											// for each cell in program...
	while( (context = getCell( false )) )
		found |= configureCell( context );

											// warn if ignoring restrictions
	if( found )
	{
		if( job->params & md5 )
		{
			if( ! ( job->params & restore ) )
				WARNv( "Restore disabled. Extracting as-is, discarding any padding.\n" );
		}
		else
		{
			WARNv( "Tags found, md5Aware disabled. Extracting as-is, including any padding.\n" );
		}
	}

#if DEBUG
for( int c=0; c < numCells + 1; c++ )
{
DBUG( "cell=" << c << " : start=" << audioCells[c].start_sector
<< "(pts " << audioCells[c].start_pts << ")"
<< " last=" << audioCells[c].last_sector
<< " xIndex=" << audioCells[c].xIndex );
}
#endif

	pg = pgcCell = 0;
	return 0;
}


// ----------------------------------------------------------------------------
//    lpcmPGextractor::configureCell :
// ----------------------------------------------------------------------------
//    Configures current cell in PGC for extraction, reads its audio
//    start/end sectors and start pts, and sets its unpadding/reverse-shift
//    trimpoints and md5 test values if lplex-authored.
//
//    Returns 0 on success
// ----------------------------------------------------------------------------


int lpcmPGextractor::configureCell( int context )
{
	unsigned char buf[DVD_VIDEO_LB_LEN * 2];
	int found = 0;
	lpcmFile *lFile, blank;
	pci_t pci;
	dsi_t dsi;

	blank.format = job->format;
	blank.group = titleset - 1;
	blank.trim.offset = blank.trim.len = 0;
	blank.details = _f( "%02x", lpcm_id );

	blank.root = 0;
	blank.edit = 0;

	memcpy( &blank.fmeta, &fmeta, sizeof( FLAC__StreamMetadata ) );

	if( DVDReadBlocks( vob, cell->start_sector, 2, buf ) < 2 )
		FATAL( "[configureCell] Error reading DVD data." );

										// fill in audio table
	navRead_PCI( &pci, buf + 0x2D );
	audioCells[ pgcCell ].start_pts = pci.pci_gi.vobu_s_ptm;

	navRead_DSI( &dsi, buf + 0x407 );

	int16_t a_synca = dsi.synci.a_synca[ audioStream ];
	if( a_synca && a_synca != 0x3fff )
		audioCells[ pgcCell ].start_sector = cell->start_sector + a_synca;
	else
	{
		INFO( "No audio in cell " << pgcCell << "\n" );
		audioCells[ pgcCell ].xIndex = -1 ;
		return found;
	}

	if( pgcCell )
		audioCells[ pgcCell-1 ].last_sector = audioCells[ pgcCell ].start_sector;

	uint64_t len =   (uint64_t) (
		dvd_aframes( &pgct->cell_playback[pgcCell].playback_time ) *
		audioframe );

										// add new output file if first cell in program
	if( context == _isNew )
	{
		audioCells[ pgcCell ].secondary = 0;

		Lfiles->push_back( blank );
		lFile = &Lfiles->back();

		lFile->fName = "dvd_" + nameNow;
		lFile->id = idNow;

		lFile->trim.len = len;

		const char *tag = ( state & singleStep ? INFO_TAG : LOG_TAG );
										// see if Lplex-authored...
		if( (found = readUserData( lFile, buf + 0x343 )) )
		{
			ECHO( _f( "%s(%s) Lplex tags found in nav packet PCI.RECI at LB %d::0x343\n",
                tag, nameNow.c_str(), cell->start_sector ) );
		}

		for( uint8_t *look = buf + DVD_VIDEO_LB_LEN + 17;
			look - buf + 16 < ( 2 * DVD_VIDEO_LB_LEN ); look++ )
		{
			if( look[0] == '\0' )
				if( bEndian32( look ) == 0x000001B8 )
					if( found += readUserData( lFile, look + 12 ) )
					{
						ECHO( _f( "%s(%s) Lplex tags found in GOP User Data field at LB %d::0x%03lx\n",
                            tag, nameNow.c_str(), cell->start_sector + 1,
							look + 12 - DVD_VIDEO_LB_LEN - buf ) );
						break;
					}
		}
		if( found )
			state |= lplexAuthored;

										// ...and initialize as appropriate
		if( lpcmProcessor::soundCheck( lFile, 1 ) && job->params & md5 )
		{
			if( state & lplexAuthored && job->params & restore )
			{
				int32_t offset = lFile->trim.offset;
				lFile->trim.offset -= prevOffset;
				lFile->trim.len += lFile->trim.offset;
				prevOffset = offset;
			}
			lFile->trim.padding =
				( audioframe - lFile->trim.len % audioframe ) % audioframe;
#if DEBUG
POST( "\n    dvd    : restored  : offset :  pad   :  file\n" );
POST( setw(10) << flacHeader::bytesUncompressed( &lFile->fmeta ) << " :" <<
setw(10) << lFile->trim.len << " :" <<
setw(7) << lFile->trim.offset <<" :" <<
lFile->trim.padding <<" :" <<
" " << lFile->fName.stem() << endl << setw(0) );
#endif
		}
	}
	else
	{
		audioCells[ pgcCell ].secondary = 1;
		Lfiles->back().trim.len += len;
		Lfiles->back().trim.padding =
			( audioframe - Lfiles->back().trim.len % audioframe ) % audioframe;
	}
										// cross-reference cell to output file
	audioCells[ pgcCell ].xIndex = Lfiles->size() - 1;

	return found;
}


#ifdef dvdread_udflist


// ----------------------------------------------------------------------------
//    lpcmPGextractor::udfItem :
// ----------------------------------------------------------------------------
//    udfLister "callback" function for reporting filenames, returns 0.
// ----------------------------------------------------------------------------


int lpcmPGextractor::udfItem( const char *fname, uint16_t ftype, uint32_t lb, uint32_t len )
{
	struct infoFile iFile;

	if( ftype == 4 )
		return 0;

	iFile.root = 0;
	iFile.edit = 0;
	iFile.reject = false;
	iFile.lb = lb;
	iFile.size = len;

	iFile.fName = fname + 1;
	if(   iFile.fName.Left(9) == "VIDEO_TS/" ||
			iFile.fName.Left(9) == "AUDIO_TS/" ||
			iFile.fName == "XTRA/Lplex.log" )
		iFile.reject = true;
	else if( iFile.fName.Left(5) == "XTRA/" )
		iFile.fName = fname + 6;

#if DEBUG
   if( ! infofiles->size() )
      INFO( "---offset:----lb:-------size-------------------------\n" );
   LOG( _f( " %8lx:%6ld: %10ld %c %s\n",
      lb * DVD_VIDEO_LB_LEN, lb, len, iFile.reject ? ' ' : '*', fname ) );
#endif

	infofiles->push_back( iFile );

	return 0;
}


bool udfSort( const infoFile &i, const infoFile &j ) { return( i.lb < j.lb ); }


// ----------------------------------------------------------------------------
//    lpcmPGextractor::udfPrint :
// ----------------------------------------------------------------------------
//    Sorts the udf file list by sector and prints it out.
// ----------------------------------------------------------------------------


void lpcmPGextractor::udfPrint()
{
	struct infoFile *iFile;

	std::sort( infofiles->begin(), infofiles->end(), udfSort );

	ECHO( "\n" );
    INFO( _f( "Udf dvd image '%s':\n", fName.filename().c_str() ) );
	LOG ( "----------------------------------------------------\n" );
	LOG ( "   offset      lb       size   path                 \n" );
	LOG ( "----------------------------------------------------\n" );

	for( int i=0; i < infofiles->size(); i++ )
	{
		iFile = &infofiles->at(i);
		LOG( _f( "%9lx %7ld %10ld %c %s\n",
			iFile->lb * DVD_VIDEO_LB_LEN, iFile->lb, iFile->size,
            iFile->reject ? ' ' : '*', iFile->fName.c_str() ) );
	}
	ECHO( "\n" );
}

// ----------------------------------------------------------------------------
//    lpcmPGextractor::vts_start_sector :
// ----------------------------------------------------------------------------
//    Returns start sector of <titleset> in udf image on success, 0 on fail.
// ----------------------------------------------------------------------------


uint32_t lpcmPGextractor::vts_start_sector( int titleset )
{
    string vob = _f( "VIDEO_TS/VTS_%02i_1.VOB", titleset );
	for( int j=0; j < infofiles->size(); j++ )
		if( infofiles->at(j).fName == vob )
			return infofiles->at(j).lb;
	return 0;
}


// ----------------------------------------------------------------------------
//    lpcmPGextractor::udfCopyInfoFiles :
// ----------------------------------------------------------------------------
//    Copies info files to folder <path>.
// ----------------------------------------------------------------------------


void lpcmPGextractor::udfCopyInfoFiles( const char * path )
{
    string iName, outPath;
	struct infoFile *iFile;
	size_t udfStart;
	fstream outFile, img;

	if( ! infofiles->size() )
		return;

    outPath = ( path ? path : (const char*)job->extractPath.string() );

    img.open( job->inPath.string(), ios::binary | ios::in );
	if( ! img )
	{
		ERR( "Could not open image file.\n" );
		return;
	}

	for( int i=0; i < infofiles->size(); i++ )
	{
		iFile = &infofiles->at(i);
		if( iFile->reject ||
                fs::path( iFile->fName ).filename() == "Lplex.log" )
			continue;

		iName = iFile->fName;
        fs_fixSeparators( (char*)(const char*)iName.c_str() );
		INFO( "-copying '" << iName << "'\n" );

        iName = outPath / iName;
        fs_MakeDirs( fs::path( iName ).parent_path() );

		img.seekg( iFile->lb * DVD_VIDEO_LB_LEN, ios::beg );

		outFile.open( iName, ios::binary | ios::out );
		if( ! outFile )
			ERR( "Could not open '" << iName << "'." );

		for( size_t b=0; b < iFile->size; b++ )
			outFile.put( img.get() );

		outFile.close();
	}
	ECHO( "\n" );
	img.close();
}

#endif

#ifndef lgzip_support


// ----------------------------------------------------------------------------
//    udfError :
// ----------------------------------------------------------------------------
//    Reports lack of lgz support in this build.
// ----------------------------------------------------------------------------


void udfError( const char *msg )
{
	_verbose = true;
    ERR( string(msg) + "\n" );
	LOG( "\n" );
	LOG( "This build of Lplex doesn't support dvd container (.lgz) files.\n" );
	LOG( "See source code documentation for information on how to rebuild\n" );
	LOG( "Lplex with this feature enabled.\n\n" );
	exit(-1);
}

#endif
