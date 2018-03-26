/*
	dvd.hpp - dvd navigation using libdvdread.
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



#ifndef DVD_HPP_INCLUDED
#define DVD_HPP_INCLUDED

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
#include <climits>
#include <string>
#include "util.h"

class lpcm_video_ts;
class lpcmPGtraverser;
class lpcmPGextractor;

#include "lplex.hpp"

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
											// prevents #error from dvdread/ifo_types.h
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef UINT8_MAX
#define UINT8_MAX 0xff /* 255U */
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xffff /* 65535U */
#endif

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_read.h>

#ifdef dvdread_udflist
#include "../patch/udflist.h"
#ifdef lgzip_support
const char * udfZip( lpcmPGextractor &dvd, bool verbose=true, const char * outPath = NULL );
int udfUnzip( const char *fName, const char * outPath = NULL );
#endif
#endif
#ifndef lgzip_support
void udfError( const char *msg );
#endif

typedef struct
{
	int16_t xIndex, secondary;
	uint32_t start_sector, last_sector;
	uint64_t start_pts;
}audio_adr_t;

string ifoPrint_audio_attributes( audio_attr_t *attr );

class lpcm_video_ts
#ifdef dvdread_udflist
	: public udfLister
#endif
{
public:
	enum { opened = 0x01 };

    fs::path fName;
	int state, tv, titleset, lpcm_id, audioStream, pgc, pg, pgcCell, c;
	int menuFmt, numTitlesets, numAudioStreams, numCells, numPGCs, numPGs, numPGCcells;
	FLAC__StreamMetadata fmeta;
	int audioframe;
	bool verbose, isImage;

	dvd_reader_t *libdvdReader;
	ifo_handle_t *ifo;
	pgc_t *pgct;
	cell_adr_t *cells, *cell;
	dvd_file_t *vob;

	uint32_t idNow;
	string nameNow, label;

	lpcm_video_ts( const char * VIDEO_TS=NULL, bool v=true )
			: state(0), libdvdReader(NULL), verbose(v), isImage(false)
		{ if( VIDEO_TS ) open( VIDEO_TS ); }
	~lpcm_video_ts() { close(); }

	bool isOpen() { return state & opened; }
	virtual int open( const char * VIDEO_TS, bool fatal=true );
	virtual void close()
	{
		if( libdvdReader )
			DVDClose( libdvdReader );
		libdvdReader = NULL;
		state = 0;
	}

	int getAudioStream();
	int getPGC();

	virtual int configurePGC() { return 1; }
	virtual int configureCell( int context ) { return 1; }
	virtual int configureAudioStream() { return 1; }
	virtual int getCell( bool searchBeyond = true ) { return 0; }

	cell_adr_t* cellAt( cell_position_t *pos );
	cell_adr_t* cellAt( int vob_id, int cell_id );
	int cellIndex( cell_position_t *pos );
	int cellIndex( int vob_id, int cell_id );
	int TVformat( ifo_handle_t *ifo );
#ifdef dvdread_udflist
	virtual int udfItem( const char *fname, uint16_t ftype, uint32_t lb, uint32_t len );
	virtual void udfPrint() {}
#endif
};



class lpcmPGtraverser : public lpcm_video_ts
{
public:
	lpcmPGtraverser( const char * VIDEO_TS=NULL, bool v=true ) : lpcm_video_ts( VIDEO_TS, v ) {}
	virtual int getCell( bool searchBeyond = true );
};


class lpcmPGextractor : public lpcmPGtraverser
{
public:
	enum
	{
//      opened = 0x01,
		traversed = 0x02,
		singleStep = 0x04,
		lplexAuthored = 0x80
	};

	int16_t readIndex, writeIndex, pgcIndex;
	lplexJob *job;
	lpcmFile *lFile;
	vector<lpcmFile> *Lfiles;
	vector<infoFile> *infofiles;
	lpcmWriter *writer;
	md5_state_t md5sum;
	counter<uint64_t> ct;
	audio_adr_t *audioCells;

	typedef struct
	{
		int titleset, lpcm_id, audioStream, pgc, /*pg, pgcCell, c, */numPGCcells;
		FLAC__StreamMetadata fmeta;
		int audioframe;
		audio_adr_t *audioCells;
	} audioTableData;

	vector<audioTableData> audioTables;

	lpcmPGextractor( vector<lpcmFile> *lFiles, vector<infoFile> *iFiles, lplexJob *plexJob, bool travel=false, bool v=true ) :
		Lfiles( lFiles ), infofiles( iFiles ), job( plexJob ), audioCells(NULL),
		readIndex(-1), writeIndex(-1), pgcIndex(-1)

	{
		verbose = v;
        if( Right(job->name, 9) == "_UNPACKED" )
			label = job->name.Left( job->name.length() - 9 );
		else
			label = job->name;

		flacHeader::zeroStreamInfo( &fmeta );
		if( travel )
			traverse();
	}

	~lpcmPGextractor()
	{
		for( int i=0; i < audioTables.size(); i++ )
			delete[] audioTables[i].audioCells;
	}

	virtual int configurePGC();
	virtual int configureCell( int context );
	virtual int getCell( bool searchBeyond = true );

	virtual int open( const char * VIDEO_TS = NULL, bool fatal=true )
	{
//      if( ! ( state & opened ) )
//      {
			if( lpcm_video_ts::open(
                    VIDEO_TS ? VIDEO_TS : job->inPath.generic_string().c_str(), fatal ) )
			{
				if( ! ( state & singleStep ) )
					traverse();
				return 1;
			}
//      }
		return 0;
	}

	bool traverse();
	int getCellTraversed();
	lpcmWriter* getWriter( int writeIndex, int context=_isNew );
	uint32_t nextAudioCellPts( uint16_t c );
#ifdef dvdread_udflist
	virtual int udfItem( const char *fname, uint16_t ftype, uint32_t lb, uint32_t len );
	virtual void udfPrint();
	void udfCopyInfoFiles( const char * path=NULL );
	uint32_t vts_start_sector( int titleset );
#endif

private:
	int32_t prevOffset;
};


#endif
