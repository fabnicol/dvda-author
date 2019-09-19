/*
	lplex.hpp - top-level authoring and extraction.
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



#ifndef LPLEX_HPP_INCLUDED
#define LPLEX_HPP_INCLUDED

#ifndef LPLEX_PCH_INCLUDED
#include "lplex_precompile.h"
#endif


#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <md5/md5.h>
#include <filesystem>

#include "util.h"
#include "wx.hpp"
#include "platform.h"
#include "lpcm.hpp"
#include "flac.hpp"
#include "processor.hpp"

#include "lplex.def"

#define VERSION "0.3.1-rc2"

#ifndef build_defs
#define build_ver ""
#endif

#ifndef RELEASE
#define LPLEX_VERSION_STRING VERSION build_ver
#else
#define LPLEX_VERSION_STRING VERSION build_ver " " RELEASE
#endif


using namespace std;
namespace fs = std::filesystem;

class dvdLayout;
class lpcmPGextractor;

											//possible job settings
enum
{
	dvdv = 0x0001,
//   reserved = 0x0002,
	dvdFormat = 0x0003,
	auth = 0x0004,
	unauth = 0x0008,
	jobMode = 0x000C,

	redirect = 0x0010,
	md5 = 0x0020,
	restore = 0x0040,
	info = 0x0080,

	infodir = 0x0100,
	customized = 0x0200,
	userNamed = 0x0400,
	dvdStyler = 0x0800,

	rescale = 0x1000,
	gzip = 0x2000,
	cleanup = 0x4000,
	verbose = 0x8000
};

enum
{
	edited = 0x01,
	infoUnsorted = 0x02
};

enum
{
	relative = 1,
	absolute
};

extern option long_opts[];

#define BIGBLOCKCT 512
#define DVD_VIDEO_LB_LEN 2048
#define BIGBLOCKLEN ( DVD_VIDEO_LB_LEN * BIGBLOCKCT )

struct dvdJpeg
{
	fs::path fName, tName;
	int dim, rescale, ar;
	uint32_t roughGOP, roughGOP2;

	enum { _4x3, _16x9 };
	dvdJpeg(int asprat=_4x3) : rescale(0), ar(asprat) {}
	string getName() { return rescale ? tName.string() : fName.string(); }
	int getDim() { return dim + ( rescale ? ( dim < 4 ? 4 : -4 ) : 0 ); }

	const char *sizeStr( bool outputSize = true )
	{
		return (const char*[]){
			"720x480", "704x480", "352x480", "352x240", // NTSC
			"720x576", "704x576", "352x576", "352x288"  // PAL
		} [ outputSize ? getDim() : dim ];
	}

	const char *aspStr()
	{
		return (const char*[]){ " 4:3", "16:9" } [ ar ];
	}
};


struct lpcmFile : public lpcmEntity
{
	enum
	{
		untyped = 0x0,
		titleStart = 0x01,
		trimStart = 0x02,
		seqStart = titleStart | trimStart,
		titleEnd = 0x04,
		trimEnd = 0x08,
		seqEnd = titleEnd | trimEnd,
		appended = 0x10,
		readComplete = 0x80
	};

	uint16_t group, type, format, jpgIndex;
	uint32_t id, dvdFrames, audioFrames, videoFrames;
	counter<uint32_t> ct;
	lpcmWriter *writer;
	string details;
};

inline bool operator < (const lpcmFile& a, const lpcmFile& b)
	{ return (lpcmEntity&)a < (lpcmEntity&)b; }

struct infoFile
{
	string fName;
	uint16_t root, edit;
	bool reject;
#ifdef dvdread_udflist
	size_t lb, size;
#endif
};

inline bool operator < (const infoFile& a, const infoFile& b)
	{ return a.fName < b.fName; }
inline bool operator == (const infoFile& a, const infoFile& b)
	{ return a.fName == b.fName; }

enum { NTSC, PAL };

#define DVD_PLUS_R     4700372992LL // 4488 MB
#define DVD_MINUS_R    4706074624LL // 4482 MB
#define DVD_PLUS_R_DL  8547993600LL // 8152 MB

enum
{
	plusR = 0,
	minusR = 1,
	plusR_DL = 2,
	unspecified = 4
};

enum
{
    DIR_IGNORE = 0,
    DIR_CONTINUE = 1
};

enum
{
	imagefile = 1,
	dvdr = 2
};

#define dvdPath outPath
#define extractPath outPath

enum
{
	isNamed = 0x01,
	hasProjectFile = 0x02,
	appending = 0x04,
	updating = 0x08
};

typedef struct
{
	uint32_t params, roughGOP;
	Ltype prepare;
	uint16_t format, tv, media, flacLevel, jpegNow;
	int16_t group;
	string name, extractTo;
	fs::path inPath, outPath, infoPath, tempPath, isoPath, projectPath;
	uint16_t trim, trim0, trimCt, update, now;
	string mplexArg;
	bool seqend;
	uint16_t skip;
}lplexJob;

#include "dvd.hpp"

extern lplexJob job;
extern vector<lpcmFile> Lfiles;
extern vector<dvdJpeg> jpegs;
extern vector<string> dirs;
extern vector<string> menufiles;
extern vector<infoFile> infofiles;

extern unsigned char bigBlock[BIGBLOCKLEN];

extern string defaultOutPath;
extern char* defaultStill;
extern _wxStopWatch stopWatch;
extern int editing;
extern int debug;
extern int endPause;
extern int menuForce;
extern lpcm_video_ts userMenus;
extern string menuPath;
extern int menusMap[99];

uint16_t init( int argc, char *argv[] );

uint16_t addFiles( fs::path filespec );
string defaultName();
int checkName( string &jobName, bool trim = false );

uint16_t setName( const char *namePath, bool isDirPath=true );
void setJobTargets();
void splitPaths();

uint16_t setopt( uint16_t opt, const char *optarg );
bool getOpts( int argc, char *argv[] );
void getOpts( const char *filename );
bool saveOpts( dvdLayout *layout );
bool stdArgs( int &argc, char** &argv, char *args, size_t size );

void version( const char * str="" );
void banner();
void usage( const char *str=NULL );
void GPL_notice();

int author( dvdLayout &layout );
int unauthor( lpcmPGextractor &dvd );
void copyInfoFiles(const fs::path& nameA );
uint16_t readUserData( lpcmEntity *lFile, uint8_t *userData );
uint16_t writeUserData( lpcmEntity *lFile, uint8_t *userData);
int tagEmbed();
int mkisofs(const fs::path &isoPath, const fs::path &dvdPath, const string& name );

#ifndef lplex_console
void update( vector<lpcmFile> *lFiles, vector<infoFile> *iFiles, lplexJob *job );
void update( vector<lpcmFile> *lFiles );
void update( vector<infoFile> *infofiles );
#endif

extern counter<uint32_t> _progress;
extern int _tag0;
int mpeg2encProgress( const char *msg, bool echo=true );
int mplexProgress( const char *msg, bool echo=true );
int dvdauthorProgress( const char *msg, bool echo=true );
int mkisofsProgress( const char *msg, bool echo=true );
int addMenus( const char * filename, int tv, bool force );
void copyMenufiles( int *map );
int* mapMenus();

int addJpeg( const char * fname, lplexJob &job, bool zero=false, bool ws=false );
int jpegCheck( dvdJpeg &jpeg, bool ntsc, bool rescale );

uint32_t roughGOP( const char *jpeg, const char *m2vName, bool ntsc );
uint32_t roughGOP( dvdJpeg &dvdJpeg, const char *m2vName, bool ntsc );
bool alias( fs::path &jpeg );

ofstream* m2v( uint32_t vFrames, const char *jpeg, const char *m2vName,
	uint16_t tv=NTSC, bool ws=false, void *userData=NULL, uint32_t sizeofUData=0,
	uint16_t GOPsize=18, bool append=false, bool endSeq=false, bool close=true );


class wxLplexLog : public ofstream
{
public:

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

    virtual void DoLog( int level, const char *msg, time_t timestamp )
	{
		switch( level )
		{
			#if 0
			case FatalError: FATAL( msg ); break;
			case Error:        ERR( msg ); break;
			case Warning:     WARN( msg ); break;
			case Message:     INFO( msg ); break;
			case Status:      STAT( msg ); break;
			case Info:        INFO( msg ); break;
			case Debug:
			case Trace:
			case Progress:
			#endif
			default:
				break;
		}
	}


    virtual void DoLogString( const char* *msg, time_t timestamp ) {}
#pragma GCC diagnostic pop
};


class lFileTraverser //: public wxDirTraverser
{
public:

	enum{ invalid=0x1, mismatchA=0x2, mismatchV=0x4, mismatchV_ar=0x8, notFound=0x10 };
	uint16_t root, titleset, err, strict;
	bool dirSpecified;
	struct lpcmFile lFile;
	struct infoFile iFile;
	vector<string> filenames;

	lFileTraverser( bool isStrict=true ) : titleset(100), err(0), strict(isStrict) {}

	string setRoot( const char *rootPath, int fromParent );
	void processFiles();

    void Traverse(const string& path);

    virtual void OnFile( const string& filename );
    virtual int OnDir( const string& dirname );
    virtual void OnOpenError( const string& openerrorname );

};


class dvdauthorXml
{
public:

	int video, titlesets, dvdStyler, disabled;
	ofstream xml;
	string name;

	enum xmlContext
	{
		open, close, setDest, addTitle, openVob, closeVob, addChapter, fileclose
	};

    dvdauthorXml( const string& xmlName, int tv=NTSC, int ct=1, bool styler=0, bool disable=0 ) :
		video(tv), titlesets( ct ), dvdStyler( styler ), disabled( disable )
	{
        name = _f( "%s_dvd%s.xml", xmlName.c_str(), dvdStyler ? "styler" : "author" );
		write( open, name );
	}

    int write( xmlContext context, const string& str="", int flag=0 );

	static string timestampFractional( uint32_t f, bool ntsc, float boost=0 );
};


class dvdUtil
{
public:

	static string timestamp( uint32_t f, bool ntsc );
	static string time( uint32_t f, bool ntsc );
	static uint64_t m2vEstimate( lpcmFile * lFile, bool ntsc );
	static uint64_t sizeOnDvd( lpcmFile * lFile, bool ntsc );
	static int sampleSeam( int channels, int bits_per_sample );
	static int sampleSeam( FLAC__StreamMetadata *fmeta );
	static int audioFrame( int sample_rate, int channels, int bits_per_sample );
	static int audioFrame( FLAC__StreamMetadata *fmeta );
	static int AVseam( int audioFrameLen, bool ntsc );
};


class dvdLayout : public dvdUtil
{
public:

	uint16_t dvdAudioFrame, dvdSampleSeam;
	uint32_t dvdVideoFrame;
    uint32_t dvdAVseam;
	struct{ uint64_t estimate, audio, video, info; } total;

	alignment trim;

	lpcmFile *lFile;
	vector<lpcmFile> *Lfiles;
	vector<string> *menufiles;
	vector<infoFile> *infofiles;
    lplexJob *job;
  	int16_t readIndex;
    int16_t writeIndex;
	lpcmReader *reader;
	md5_state_t md5sum;
	counter<uint64_t> ct;
	string nameNow, spaceTxt;

	dvdLayout( vector<lpcmFile> *lFiles, vector<string> *vFiles, vector<infoFile> *iFiles, lplexJob *plexJob ) :
		Lfiles( lFiles ), menufiles( vFiles ), infofiles( iFiles ), job( plexJob ),
		readIndex(-1), writeIndex(-1), reader(NULL)
	{}
	~dvdLayout() { /* if( editing ) */ saveOpts( this ); /* if( reader ) delete reader; */ }

	int configure();
	uint64_t vobEstimate();
	int checkSpace();
	void setAudioUnits( FLAC__StreamMetadata *fmeta );
	int readerNext();
	int getNext();
};


class xmlAttr
{
public:

	char *buf;
	char *name;
    char *val;
    int i;

	xmlAttr( char *buffer ) : buf(buffer), name(NULL), val(NULL), i(0) {}

	uint16_t get()
	{
		int nameTok = 0, valTok = 0;

		while( ! ( buf[i] == '/' && buf[i+1] == '>' ) )
		{
			switch( buf[i] )
			{
				case ' ':
					nameTok = i;
					break;

				case '=':
					name = buf + nameTok + 1;
					buf[i]='\0';
					if( buf[i+1] == '\"' )
					{
						i++;
						valTok = i;
					}
					break;

				case '"':
					val = buf + valTok + 1;
					buf[i]='\0';
					i++;
					return 1;

				default:
					break;
			}
			i++;
		}

		return 0;
	}
};




#endif
