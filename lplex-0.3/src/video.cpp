/*
	video.cpp - still video creation.
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


typedef struct
{
	uint16_t dropFrame, hour, min, sec, frame, closed, broken;
}gopInfo;


class gopHeader
{
public:
	static void read( uint8_t* buf, gopInfo *GOP );
	static void write( uint8_t* buf, gopInfo *GOP );

	static void display( const gopInfo *GOP, const char* prefix="", ostream &stream=cerr );
	friend ostream& operator << ( ostream& stream, const gopInfo &g )
		{ display( &g, "", stream ); return stream; }
};


// uncomment line below if required.  mplex/dvdauthor redo tc's regardless.
#define NEED_PROPER_TIMECODES



// ----------------------------------------------------------------------------
//    m2v :
// ----------------------------------------------------------------------------
//    Creates a mini m2v video file by running jpeg2yuv | mpeg2enc and expands
//    it to the required size, inserting a userdata packet if given.
//
//    Arguments:
//       <vFrames>      - number of video frames to create
//       <jpeg>         - image to use
//       <m2vName>      - output filename
//       <tv>           - video system to use
//       <ws>           - whether aspect ratio is widescreen (16:9)
//       <userData>     - contents to insert in GOP User-Data packet
//       <sizeofUData>  - number of bytes of userData
//       <framesPerGOP> - ...
//       <append>       - whether appending video or starting a new m2v
//       <endSeq>       - whether to include an end-of-sequence marker
//       <close>        - whether to close the output file
//
//    Returns m2vFile stream with write position at userData contents on
//    success, fatal on fail
// ----------------------------------------------------------------------------


ofstream* m2v( uint32_t vFrames, const char *jpeg, const char *m2vName,
	uint16_t tv, bool ws, void *userData, uint32_t sizeofUData, uint16_t framesPerGOP,
	bool append, bool endSeq, bool close )
{
	uint32_t look, validBytes, f;
	uint32_t miniFileSize, seq[3], GOPct, endFrames;
	uint16_t seqCt;
	uint32_t GOP[3];
	ios::pos_type uDataPos = 0;

#ifdef NEED_PROPER_TIMECODES
	uint16_t fps, fpm, fph;
	gopInfo GOPheader;
#endif

    INFO( "Using \'" << fs::path( jpeg ).filename() << "\' to create "
			<< ( tv == NTSC ? "NTSC" : "PAL" ) << " video stream " );
	ECHO( "\n" );

	GOPct = ( vFrames + framesPerGOP - 1 ) / framesPerGOP;
	endFrames = vFrames % framesPerGOP;

    fs::path m2vPath = fs::path( m2vName ).parent_path();

	_progress.max = 2 * framesPerGOP + endFrames;


	if( execute((binDir / "jpeg2yuv").string(),
                        {"-v",
                         "1",
                         "-I",
                         "p",
                         "-f",
                         (tv == NTSC ? "29.97" : "25" ),
                         "-n",
                         _f("%d", 2 * framesPerGOP + endFrames).c_str(),
                         "-j",
                         jpeg,
                         },
                        (binDir / "mpeg2enc").string(),
                        {"-v",
                         "1",
                         "-b",
                         "3800",
                         "-f",
                         "8",
                         "-r",
                         "0",
                         "-n",
                         (tv == NTSC ? "n" : "p"),
                         "-a",
                         (ws ? "3" : "2" ),        // 1=1:1, 2=4:3, 3=16:9, 4=2.21:1
                         "-G",
                         to_string(framesPerGOP).c_str(),
                         "-g",
                         to_string(framesPerGOP).c_str(),
                         "-o",
                         (m2vPath / "mini.m2v").string().c_str()
                        },
                 false))
    {
	  //FATAL( "jpeg2yuv|mpeg2enc failed. See Lplex.log for details.\n" );
    }

    ifstream miniFile( (m2vPath / "mini.m2v").string(), ios::binary | ios::ate );

	if( ! miniFile.is_open() )
        FATAL( "Can't find input file " + (m2vPath / "mini.m2v").string() );

	static ofstream m2vFile;

	if( ! m2vFile.good() )
		m2vFile.clear();
	if( m2vFile.is_open() )
		m2vFile.seekp( 0, ios::end );

    char* _m2vName = normalize_windows_paths(m2vName);

	if( ! append )
	{
		if( m2vFile.is_open() )
		{
			size_t filesize = m2vFile.tellp();
			INFO( "Closing m2v file: " << filesize << " bytes\n");
			m2vFile.close();
		}

		m2vFile.open( _m2vName, ios::binary );
		if( ! m2vFile.is_open() )
            FATAL( "Can't open output file " + string(_m2vName) );
	}

	miniFileSize = miniFile.tellg();
	miniFile.seekg( 0 );
	miniFile.read( (char*) bigBlock, miniFileSize );
	validBytes = miniFile.gcount();

	look = seqCt = 0;

	while( look < validBytes )
	{
		if( bigBlock[look] == 0 )
		{
			if( bEndian32( bigBlock + look ) == 0x000001B3 )
				seq[seqCt] = look;

			else if( bEndian32( bigBlock + look ) == 0x000001B8 )
			{
				GOP[seqCt] = look + 4;
				if( ++seqCt == 3 )
					break;
			}
		}
		look++;
	}

	if( seqCt == 2 )
		seq[2] = miniFileSize - 4; //everything but end-of-sequence marker 0x000001b7

#ifdef NEED_PROPER_TIMECODES
	gopHeader::read( bigBlock + GOP[1], &GOPheader);
//   gopHeader::display( &GOPheader, LOG_TAG, cerr );
	fps = tv == NTSC ? 30 : 25;
	fpm = fps * 60;
	fph = fps * 3600;
	uint8_t *midGOP = bigBlock + GOP[1];
#endif

	if( userData && sizeofUData )
	{
		//INFO( "-inserting Lplex tags into first GOP as User Data field\n" );
		m2vFile.write( (char*)bigBlock, GOP[0]+4 );
        m2vFile << 0x00 << 0x00 << 0x01 << 0xb2;
		uDataPos = m2vFile.tellp();
		m2vFile.write( (char*)userData, sizeofUData );
		m2vFile.write( (char*)bigBlock+GOP[0]+4, seq[1]-(GOP[0]+4) );
	}
	else
		m2vFile.write( (char*)bigBlock, seq[1] );

	INFO( _f( "-expanding mini m2v (%d GOP=%d+%d+%d=%df) to %d GOP=%d+%dx%d+%d=%d frames\n",
		endFrames ? 3 : 2, framesPerGOP, framesPerGOP, endFrames, 2 * framesPerGOP + endFrames,
		GOPct, framesPerGOP, GOPct - (( vFrames % framesPerGOP ) ? 2 : 1 ),
		framesPerGOP, endFrames, vFrames ) );


	char *midSeq = (char*)bigBlock + seq[1];
	uint32_t sizeofMidSeq = seq[2] - seq[1] ;

	for( f = 2 * framesPerGOP; f < vFrames + 1; f += framesPerGOP )
	{
		m2vFile.write( midSeq, sizeofMidSeq );

#ifdef NEED_PROPER_TIMECODES
		GOPheader.hour = f / fph;
		GOPheader.min = (f % fph) / fpm;
		GOPheader.sec = (f % fpm) / fps;
		GOPheader.frame = (f % fps);

		gopHeader::write( midGOP, &GOPheader);
	}

	if( seqCt == 3 )
		gopHeader::write( bigBlock + GOP[2], &GOPheader);
#else
	}
#endif

	m2vFile.write( (char*)(bigBlock + seq[2]),
		( endSeq ? miniFileSize - seq[2] : ( miniFileSize - 4 ) - seq[2] ) );

	miniFile.close();

	if( ! m2vFile.good())
		m2vFile.clear();
	m2vFile.flush();

	if( close )
	{
		m2vFile.seekp( 0, ios::end );
		size_t filesize = m2vFile.tellp();
		INFO( "Closing \'" <<   _m2vName << "\': " << filesize << " bytes\n");
		m2vFile.close();
	}
	else
		m2vFile.seekp( uDataPos, ios::beg );

#ifndef __linux__
    free(_m2vName);
#endif

	return &m2vFile;
}



// ----------------------------------------------------------------------------
//    gopHeader::read :
// ----------------------------------------------------------------------------
//    Bitfield reader for given GOP header
//
//    Arguments:
//       <buf>    - pointer to GOP header
//       <GOP>    - storage structure for header info
// ----------------------------------------------------------------------------


void gopHeader::read( uint8_t* buf, gopInfo *GOP )
{
	bs_t bits;
	bs_init( &bits, (void*)buf, 4 );

	GOP->dropFrame = bs_read( &bits, 1 );
	GOP->hour = bs_read( &bits, 5 );
	GOP->min = bs_read( &bits, 6 );
	bs_read( &bits, 1 );
	GOP->sec = bs_read( &bits, 6 );
	GOP->frame = bs_read( &bits, 6 );
	GOP->closed = bs_read( &bits, 1 );
	GOP->broken = bs_read( &bits, 1 );
	bs_read( &bits, 5 );
}


// ----------------------------------------------------------------------------
//    gopHeader::write :
// ----------------------------------------------------------------------------
//    Bitfield writer for given GOP info.
//
//    Arguments:
//       <buf>    - pointer to GOP header
//       <GOP>    - structure with header values
// ----------------------------------------------------------------------------


void gopHeader::write( uint8_t* buf, gopInfo *GOP )
{
	bs_t bits;
	bs_init( &bits, (void*)buf, 4 );

	bs_write( &bits, 1, GOP->dropFrame );
	bs_write( &bits, 5, GOP->hour );
	bs_write( &bits, 6, GOP->min );
	bs_write( &bits, 1, 1 );
	bs_write( &bits, 6, GOP->sec );
	bs_write( &bits, 6, GOP->frame );
	bs_write( &bits, 1, GOP->closed );
	bs_write( &bits, 1, GOP->broken );
	bs_write( &bits, 5, 0 );
}



// ----------------------------------------------------------------------------
//    gopHeader::display :
// ----------------------------------------------------------------------------
//    Displays info for GOP header <GOP> on <stream>, using <prefix> as a title.
// ----------------------------------------------------------------------------


void gopHeader::display(const gopInfo *GOP, const char* prefix, ostream &stream)
{
	stream << prefix <<
		"dropFrame  :" << GOP->dropFrame << endl << prefix <<
		"timestamp  :" << GOP->hour<<":"<<GOP->min<<":"<<GOP->sec<<"."<<GOP->frame << endl << prefix <<
		"closed     :" << GOP->closed << endl << prefix <<
		"broken     :" << GOP->broken << endl;
}




// ----------------------------------------------------------------------------
//    addJpeg :
// ----------------------------------------------------------------------------
//    Checks given jpeg for dvd compatibility, and if so makes a rough estimate
//    of average gop bytes and adds it to the jpegFile array.
//
//    Arguments:
//       <fname>     - full path to jpeg
//       <job>       - relevant job descriptor
//       <zero>      - whether to place jpeg in front array slot
//
//    Returns 0 on success.
// ----------------------------------------------------------------------------



int addJpeg( const char * fname, lplexJob &job, bool zero, bool ws )
{
	dvdJpeg jpeg( ws ? dvdJpeg::_16x9 : dvdJpeg::_4x3 );
	jpeg.fName = fname;
	bool isBlack = false;

	if( zero && jpegs.size() )
		jpegs.clear();

    if( jpeg.fName.stem().string().Left( 5 ) == "black" )
	{
		alias( jpeg.fName );
		isBlack = true;
        if( jpeg.fName.string() == "black" || jpeg.fName.string() == "black_XS" )
            jpeg.fName = dataDir / ( job.tv == NTSC ? "black_NTSC_352x240.jpg" : "black_PAL_352x288.jpg" );
        else if( jpeg.fName.string() == "black_S" )
            jpeg.fName = dataDir / ( job.tv == NTSC ? "black_NTSC_352x480.jpg" : "black_PAL_352x576.jpg" );
        else if( jpeg.fName.string() == "black_M" )
            jpeg.fName = dataDir / ( job.tv == NTSC ? "black_NTSC_704x480.jpg" : "black_PAL_704x576.jpg" );
        else if( jpeg.fName.string() == "black_L" )
            jpeg.fName = dataDir / ( job.tv == NTSC ? "black_NTSC_720x480.jpg" : "black_PAL_720x576.jpg" );
		else
			isBlack = false;
	}
    else if( jpeg.fName.is_relative() )
	{
        jpeg.fName = fs::absolute( jpeg.fName );
	}

	// check if previously added
	bool prev = false;
	for( uint i=0; i < jpegs.size(); ++i )
	{
		if( jpeg.fName == jpegs[i].fName )
		{
			if( jpeg.ar == jpegs[i].ar )
			{
				job.jpegNow = i;
				return 0;
			}
			else
			{
				jpeg.dim = jpegs[i].dim;
				jpeg.tName = jpegs[i].tName;
				jpeg.roughGOP = jpegs[i].roughGOP;
				jpeg.roughGOP2 = jpegs[i].roughGOP2;
				jpeg.rescale = jpegs[i].rescale;
			}
			prev = true;
			//break;
		}
	}

	if( ! prev )
	{
		jpeg.dim = jpegCheck( jpeg, job.tv == NTSC, job.params & rescale );

		if( isBlack )
		{
			jpeg.roughGOP = ( ( uint16_t[] )
			{
			// 720x480, 704x480, 352x480, 352x240 NTSC
				35465, 34565, 18995, 9680,
			// 720x576, 704x576, 352x576, 352x288 PAL
				36851, 35915, 19643, 9977
			} ) [ jpeg.getDim() ];
		}

		else
		{
			if( jpegs.size() == 0 )
			{
				if( ! ( job.params & customized ) )
                    SCRN( STAT_TAG "Checking custom " )
                SCRN( "jpegs... " )
				STAT( "Checking custom screen jpegs...\n" );
			}
			LOG( fname << "\n" );
			LOG( _affirm << jpeg.sizeStr() << "\n" );
			jpeg.roughGOP = roughGOP( jpeg,
                (job.tempPath/ "mini.m2v").string().c_str(), job.tv == NTSC );
			job.params |= customized;
		}
	}

	jpegs.push_back( jpeg );
	job.jpegNow = jpegs.size() - 1;

	return 0;
}



// ----------------------------------------------------------------------------
//    alias :
// ----------------------------------------------------------------------------
//    Replaces given jpeg <filename> with shorthand mnemonic if it's one of
//    the default Lplex black images, returning true if so.
// ----------------------------------------------------------------------------


bool alias( fs::path &jpeg )
{
    string name = jpeg.stem().string();

	if( name == "black" || name == "black_XS" || name == "black_S"
		|| name == "black_M" || name == "black_L" )
		;
	else if( name == "black_NTSC_352x240" || name == "black_PAL_352x288" )
		jpeg = "black";
	else if( name == "black_NTSC_352x480" || name == "black_PAL_352x576" )
		jpeg = "black_S";
	else if( name == "black_NTSC_704x480" || name == "black_PAL_704x576" )
		jpeg = "black_M";
	else if( name == "black_NTSC_720x480" || name == "black_PAL_720x576" )
		jpeg = "black_L";
	else
		return false;

	return true;
}



#if 0
#include <jpeglib.h>
#endif

// ----------------------------------------------------------------------------
//    jpegCheck :
// ----------------------------------------------------------------------------
//    Checks <jpegName> for dvd compatibility.
//
//    Returns index of dvd screen type on success, fatal on fail
// ----------------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int jpegCheck( dvdJpeg &jpeg, bool ntsc, bool rescale )
{
//	uint16_t width, height, ok = false;

#if 0 // libjpeg version : no rescaling

	struct jpeg_decompress_struct j;
	struct jpeg_error_mgr jErr;

    FILE *dvdJpeg = fopen( jpeg.fName.string(), "rb" );

	if ( dvdJpeg == nullptr )
        FATAL( "Can't open jpeg file " + jpeg.fName.string() );

	j.err = jpeg_std_error( &jErr );  /* ?????????? */
	jpeg_create_decompress( &j );
	jpeg_stdio_src( &j, dvdJpeg );
	jpeg_read_header( &j, true );
	width = j.image_width;
	height = j.image_height;

	JDIMENSION output_width;   /* scaled image width */
	JDIMENSION output_height;   /* scaled image height */
	UINT8 density_unit;      /* JFIF code for pixel size units */
	UINT16 X_density;      /* Horizontal pixel density */
	UINT16 Y_density;      /* Vertical pixel density */
	jpeg_destroy_decompress( &j );
	fclose( dvdJpeg );

#endif // libjpeg version

#if 0
	wxImage jpg;
    if( ! jpg.LoadFile( jpeg.fName.string(), wxBITMAP_TYPE_JPEG ) )
        FATAL( "Can't open jpeg file '" + jpeg.fName.string() + "'." );

	width = jpg.GetWidth();
	height = jpg.GetHeight();

	struct{ uint16_t w, h; } allowed[] =
	{
		{ 720, 480 }, { 704, 480 }, { 352, 480 }, { 352, 240 }, // NTSC
		{ 720, 576 }, { 704, 576 }, { 352, 576 }, { 352, 288 }, // PAL
	};

	for( int i=0; i < 8; i++)
		if( width == allowed[ i ].w )
			if( height == allowed[ i ].h )
			{
				ok = i + 1;
				break;
			}

	if( ok )
	{
		if( ! ( ok > ( ntsc ? 0 : 4 ) && ok < ( ntsc ? 5 : 9 ) ) )
		{
			if( rescale )
			{
				jpeg.rescale = true;
				int i = ok + ( ntsc ? -5 : 3 );

//            LOG( _f( "-rescaling from %s %dx%d to %s %dx%d.\n",
//               ntsc ? "PAL" : "NTSC", allowed[ok-1].w, allowed[ok-1].h,
//               ntsc ? "NTSC" : "PAL", allowed[i].w, allowed[i].h )  );

                jpeg.tName = (job.tempPath / jpeg.fName.stem()).string()
					+ _f( "_rescaled_%s_%dx%d.jpg",
						ntsc ? "NTSC" : "PAL", allowed[i].w, allowed[i].h );

				jpg.Rescale( allowed[i].w, allowed[i].h, wxIMAGE_QUALITY_HIGH );
				jpg.SetOption( "quality", "100" );
                jpg.SaveFile( jpeg.tName.string(), wxBITMAP_TYPE_JPEG );
			}
			else
				ok = 0;
		}
	}

	else
	{
        SCRN( "\n" )
		_verbose = true;

		ERR( _f( "\'%s\' (%dx%d) is not compatible with %s dvd.\n",
            jpeg.fName.filename().c_str(), width, height,
			ntsc ? "NTSC" : "PAL" ) );
		LOG( "\n" );
		LOG( "Allowable image dimensions are\n" );
		LOG( "-NTSC: 720x480  704x480  352x480  352x240\n" );
		LOG( "-PAL : 720x576  704x576  352x576  352x288\n\n" );

		exit( -1 );
	}
#endif

    return 0;
    //return --ok;
}
#pragma GCC diagnostic pop




// ----------------------------------------------------------------------------
//    roughGOP :
// ----------------------------------------------------------------------------
//    Generates and measures a 7-frame (IPPPPPP) <ntsc>/pal m2v test file
//    <m2vName> using <jpeg>, and adds a standard unit length for each
//    remaining P-frame in the GOP.
//
//    Returns rough estimate of average gop byte length on success, fatal
//    on fail.
// ----------------------------------------------------------------------------


uint32_t roughGOP( dvdJpeg &jpegfile, const char *m2vName, bool ntsc )
{
	string jpeg = jpegfile.getName();
	uint16_t testFrames = 7, framesPerGOP = ntsc ? 18 : 15;
	uint32_t IPPPPPP;

	if( execute(
        (binDir / "jpeg2yuv").string(),  // app1 name
			{ "-v",
              "1",
              "-I",
              "p",
              "-f",
			  ( ntsc ? "29.97" : "25" ),
			  "-n",
              to_string(testFrames).c_str(),
			  "-j",
              jpeg.c_str()
            },
                // Piped to:
            (binDir / "mpeg2enc").string(), // app2 name
			{
			  "-v",
              "1",
              "-b",
              "3800",
              "-f",
              "8",
              "-r",
              "0",
              "-a",
			  ( jpegfile.ar == dvdJpeg::_16x9 ? "3" : "2"),
              "-n",
			  (ntsc ? "n" : "p" ),
              "-G",
			  ntsc ? "18" : "15",
              "-g",
              ntsc ? "18" : "15",
			  "-o",
              m2vName
            },
		false)

	){
	} //FATAL( "jpeg2yuv|mpeg2enc failed. See Lplex.log for details.\n" );


	ifstream mini2File( m2vName, ios::binary | ios::ate );
	if( ! mini2File.is_open() )
        FATAL( "Can't find input file " + string(m2vName) );

	IPPPPPP = mini2File.tellg();
	mini2File.close();

	return IPPPPPP + ( framesPerGOP - testFrames ) * (int[]) {
		288, 288, 258, 138, // NTSC
		342, 342, 306, 162  // PAL
	} [ jpegfile.getDim() ];
}


