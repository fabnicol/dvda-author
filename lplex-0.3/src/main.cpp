/*
	main.cpp - interface and input management.
	Copyright (C) 2006-2009 Bahman Negahban

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

lplexJob job;
vector<lpcmFile> Lfiles;
vector<dvdJpeg> jpegs;
vector<wxString> dirs;
vector<wxString> menufiles;
vector<infoFile> infofiles;
lpcmPGextractor dvd( &Lfiles, &infofiles, &job );


unsigned char bigBlock[BIGBLOCKLEN];

wxString dataDir, binDir, configDir, tempDir, cwd;
wxString readOnlyPath;
wxFileName lplexConfig;
wxString projectDotLplex, shebang;
wxString gzFile;
wxString menuPath;
bool startNewTitleset, projectFile, screenJpg, lgz;
wxString cmdline;

_wxStopWatch stopWatch;
wxLplexLog _wxLog;

enum
{
	editVerbose = 0x01,
	piped = 0x02,
	dot = 0x04,
	strict = 0x08,
	mismatch = 0x80
};

int debug = 0, editing = 0, edit = 0, endPause, menuForce = 0;
lpcm_video_ts userMenus;
int menuMap[99];
wxFileName optSrc;
enum { inif, commandline, prjf };
int optindl, optContext;


#ifdef lplex_console


// ----------------------------------------------------------------------------
//    done :
// ----------------------------------------------------------------------------
//    atexit() cleanup.
// ----------------------------------------------------------------------------

int exitct=0;

void done()
{
	if( exitct++ )
		return; // breaks vista recursion
	cerr.flush();
	cerr.flush();
	if( dvd.isOpen() )
		dvd.close();
	if( endPause ) _pause();

#if defined(WIN32_COLOR) || defined(ANSI_COLOR)
	consoleColorRestore();
#endif
	exit( _xcode ); // in vista this causes endless recursion via atexit()
}



// ----------------------------------------------------------------------------
//    main :
// ----------------------------------------------------------------------------


int main( int argc, char *argv[] )
{
	atexit( done );
	_verbose = false;

	if ( ! wxInitialize() )
		return -1;

	wxLogNull xx;

	if( init( argc, argv ) )
	{
		wxLog::SetActiveTarget( &_wxLog );
#ifdef _ERR2LOG
		xlog << cmdline << "\n-------------------------------------------------------------------------------\n" << endl;
#endif
		if( job.params & gzip )
		{
#ifdef lgzip_support
			if( dvd.isOpen() )
				return udfZip( dvd, true, job.outPath.GetFullPath() ) ? 0 : 1;
			else if( gzFile != wxEmptyString )
				return udfUnzip( gzFile, job.outPath.GetFullPath() );
			FATAL( "Gzip uninitialized." );
#else
			udfError( "Unsupported feature." );
#endif
		}
//      else
		if( job.params & unauth )
		{
			return unauthor( dvd );
		}
		else
		{
			dvdLayout layout( &Lfiles, &menufiles, &infofiles, &job );
			return author( layout );
		}
	}
	else
		usage( "No files to process" );
}

#endif


int deprecated;

const char *short_opts = "ud:f:t:m:r:x:i:j:M:N:R:l:s:C:c:z:p:w:a:E:v:VQh?D:n:e:P:Z:L:qG:K:";

struct option long_opts[] =
{
	{ "unauthor",    0, 0, 'u' },
	{ "dir",         1, 0, 'd' },
	{ "formatout",   1, 0, 'f' },
	{ "video",       1, 0, 't' },
	{ "md5aware",    1, 0, 'm' },
	{ "restore",     1, 0, 'r' },
	{ "infofiles",   1, 0, 'x' },
	{ "infodir",     1, 0, 'i' },
	{ "jpeg",        1, 0, 'j' },
	{ "menu",        1, 0, 'M' },
	{ "menuforce",   1, 0, 'N' },
	{ "rescale",     1, 0, 'R' },
//   { "alignment",   1, 0, 'l' },
	{ "splice",      1, 0, 'l' },
	{ "shift",       1, 0, 's' },
	{ "cleanup",     1, 0, 'C' },
	{ "create",      1, 0, 'c' },
	{ "media",       1, 0, 'z' },
	{ "dvdpath",     1, 0, 'p' },
	{ "workpath",    1, 0, 'w' },
	{ "isopath",     1, 0, 'a' },
	{ "extractpath", 1, 0, 'E' },
	{ "verbose",     1, 0, 'v' },
	{ "version",     0, 0, 'V' },
	{ "license",     0, 0, 'Q' },
	{ "help",        0, 0, 'h' },
	{ "readonlypath",1, 0, 'D' },
	{ "name",        1, 0, 'n' },
	{ "editing",     1, 0, 'e' },
	{ "pause",       1, 0, 'P' },
	{ "lgz",         1, 0, 'Z' },
	{ "color",       1, 0, 'L' },

	{ "nocerr",      0, 0, 'q' }, //(private)
	{ "debug",       1, 0, 'G' }, //(private)
	{ "skip",        1, 0, 'K' }, //(private)

	{ "formatOut",   1, &deprecated, 'f' },
	{ "md5Aware",    1, &deprecated, 'm' },
	{ "infoFiles",   1, &deprecated, 'x' },
	{ "infoDir",     1, &deprecated, 'i' },
	{ "dvdPath",     1, &deprecated, 'p' },
	{ "workPath",    1, &deprecated, 'w' },
	{ "isoPath",     1, &deprecated, 'a' },
	{ "extractPath", 1, &deprecated, 'E' },
	{ "readonlypath",  1, &deprecated, 'D' },
	{ "readonlypath",  1, &deprecated, 'D' },
	{ "alignment",   1, &deprecated, 'l' },

	{ 0,0,0,0 }
};



// ----------------------------------------------------------------------------
//    init :
// ----------------------------------------------------------------------------
//    Reads/sets the config file, parses the command line, and sets up filenames
//    and directories.
//
//    Returns 1 on success.
// ----------------------------------------------------------------------------


uint16_t init( int argc, char *argv[] )
{
	wxImage::AddHandler( new wxJPEGHandler );
											//set defaults
	initPlatform();
	_wxMakeDirs( configDir );
	logInit( configDir + "lplex.log" );
	projectDotLplex = configDir + "project.lplex";
	cwd = wxFileName::GetCwd();
	job.tempPath = tempDir;

	job.params = dvdv | md5 | restore | info | cleanup | rescale;
#ifdef lgzip_support
	job.prepare = lgzf;
#else
	job.prepare = isof;
#endif
	job.format = wavef;
	job.flacLevel = 8;
	job.tv = NTSC;
	job.jpegNow = 0;
	job.group = -1;
	job.media = plusR;
	job.trim = seamless;
	job.trim0 = job.trimCt = 0;
	job.name = wxEmptyString;
	job.extractTo = wxEmptyString;
	job.now = 0;
	job.update = 0;

	job.mplexArg = "";
	job.seqend = true;
	job.skip = 0;

	editing = false;
#if defined(WIN32_COLOR) || defined(ANSI_COLOR)
	consoleColorInit();
#if defined(WIN32_COLOR)
	setcolors();
#else
	setcolors( dark );
#endif
#endif

	wxString arg;
											// check config file

	wxFileConfig configFile( "lplex", wxEmptyString, lplexConfig.GetFullPath(), wxEmptyString,
		wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH );

	optSrc = lplexConfig.GetFullName().mb_str();
	optContext = inif;
	optindl = -1;

											// ...write a default config file if none exists
	if( ! wxFileExists( lplexConfig.GetFullPath() ) )
	{
		configFile.Write( "formatout"    , "wav" );
		configFile.Write( "video"        , "ntsc" );
		configFile.Write( "md5aware"     , "yes" );
		configFile.Write( "restore"      , "yes" );
		configFile.Write( "infofiles"    , "yes" );
		configFile.Write( "jpeg"         , "black" );
		configFile.Write( "splice"       , "seamless" );
		configFile.Write( "shift"        , "backward" );
		configFile.Write( "cleanup"      , "yes" );
#ifdef lgzip_support
		configFile.Write( "create"       , "lgz" );
#else
		configFile.Write( "create"       , "iso" );
#endif
		configFile.Write( "media"        , "dvd+r" );
		configFile.Write( "dvdpath"      , "adjacent" );
		configFile.Write( "isopath"      , "adjacent" );
		configFile.Write( "extractpath"  , "adjacent" );
		configFile.Write( "workpath"     , job.tempPath.GetFullPath() );
		configFile.Write( "readonlypath" , readOnlyPath );
		configFile.Write( "verbose"      , "no" );
		configFile.Flush();
	}

											// ...or read in config file settings
	else for( optindl=0; long_opts[optindl].name; optindl++ )
	{
		if( configFile.Read( long_opts[optindl].name, &( arg = wxEmptyString ) ) )
			setopt( long_opts[optindl].val, arg );
	}
											// check for unresolved options

	if( ! ( job.trim & ( backward | nearest | forward ) ) )
		job.trim |= backward;
											// parse the command line
	startNewTitleset = true;
	projectFile = true;
	lgz = false;
	optContext = commandline;
	optSrc = cwd + wxSEP + "command line";
	optindl = -1;

#ifdef _ERR2LOG
	_affirm = "   ";
	cmdline << "\n-------------------------------------------------------------------------------\n"
		<< INFO_TAG << "Commandline: ";
	for( int i=1; i<argc; i++ )
		cmdline << QUOTE( argv[i] ) << " ";
#endif

	getOpts( argc, argv );

	dirs.push_back( job.inPath.GetPath() );
	if( ! ( job.params & ( unauth | gzip ) ) )
		splitPaths();
	setJobTargets();

	if( jpegs.size() == 0 )
		addJpeg( "black", job );

	_affirm = "";

	return( job.params & ( auth | unauth ) );
}


// ----------------------------------------------------------------------------
//    makeAbsolute :
// ----------------------------------------------------------------------------
//    Converts given relative <filespec> into an absolute path.
//
//    Returns offset of original relative path within new absolute path.
// ----------------------------------------------------------------------------


int makeAbsolute( wxFileName &filespec )
{
#if 1
		filespec = optSrc.GetPath() + wxSEP + filespec.GetFullPath();
#else // doesn't work right in wxWidgets 2.7.2
		filespec.MakeAbsolute( optSrc.GetPath() );
#endif
	return optSrc.GetPath().Length() + 1;
}

// ----------------------------------------------------------------------------
//    addFiles :
// ----------------------------------------------------------------------------
//    Processes given <filespec> from project file or command line.
//
//    Returns 0 on success.
// ----------------------------------------------------------------------------


uint16_t addFiles( wxFileName filespec )
{
	int reauthoring = 0, rel = 0;
	wxString specPath;
	wxDir dir;
	lFileTraverser selector( editing ? edit & strict : true );
	bool isDot = false;

	flacHeader::zeroStreamInfo( &selector.lFile.fmeta );
//   wxLogNull xx;

											//resolve if relative filespec...
	if( ! wxIsAbsolutePath( filespec.GetFullPath() ) )
//   if( filespec.IsRelative() ) // this fails if filespec is just 'd:'
	{
		rel = makeAbsolute( filespec );

		if( filespec.GetFullPath().Right(1) == "."  )
		{
			filespec = filespec.GetPath();
			isDot = true;
		}
	}

											//if filespec is a directory, open it...
	if( wxDir::Exists( filespec.GetFullPath() ) )
	{
		STAT( _f("Scanning '%s'.\n", filespec.GetFullPath().mb_str()));
											//...or its parent if VIDEO_TS folder
		if( ! filespec.GetFullName().CmpNoCase( "VIDEO_TS" ) )
			filespec = filespec.GetPath();

		dir.Open( specPath = filespec.GetFullPath() );

		if ( !dir.IsOpened() )
			usage( "unable to open input directory" );

											//...unauthor if it contains a dvd structure
		if( dir.HasSubDirs( "VIDEO_TS" ) )
		{
			job.inPath = specPath + wxSEP + "VIDEO_TS";
			if( job.extractTo != wxEmptyString )
				job.outPath = job.extractTo;

			dvd.open( filespec.GetFullPath() );
			job.tv = dvd.tv;
			clearbits( job.params, jobMode );
#ifdef dvdread_udflist
			if( job.format == lgzf )
				job.params |= ( auth | gzip );
			else
#endif
				job.params |= unauth;
		}
		else
		{
			job.params |= auth;
			job.inPath = _wxEndSep( specPath );
		}

		selector.dirSpecified = true;
	}
											//else if it exists, open parent dir.
	else if( wxDir::FindFirst( filespec.GetPath(), filespec.GetFullName(), wxDIR_FILES )
			!= wxEmptyString )
	{
		if( Lfiles.size() == 0 )
		{
			dirs.push_back( filespec.GetPath() );
			if( job.inPath.GetFullPath() == wxEmptyString )
				job.inPath = filespec.GetPath() + wxSEP;
		}

		dir.Open( specPath = filespec.GetPath( wxPATH_NATIVE | wxPATH_GET_VOLUME ));
		if ( !dir.IsOpened() )
			FATAL( "Can't open input directory '" << filespec.GetFullPath() << "'." );

											//check if it's an image file
		if( dvd.open( filespec.GetFullPath(), false ) )
		{
			specPath += ( wxSEP + filespec.GetName() );
			job.media = imagefile;
			job.inPath = filespec;
			job.name = filespec.GetName();
			job.tv = dvd.tv;
			clearbits( job.params, jobMode );
#ifndef dvdread_udflist
			clearbits( job.params, info );
#else
#ifdef lgzip_support
			if( job.format == lgzf )
				job.params |= ( auth | gzip );
			else
#endif
#endif
				job.params |= unauth;
		}
		else if( dvd.isImage )
		{
			FATAL( "No lpcm audio found on dvd image." );
		}
		else
			job.params |= auth;

		selector.dirSpecified = false;
	}
											//else not found.
	else
	{
		FATAL( "Can't find '" << filespec.GetFullPath()/*.Mid( rel ) */ << "'." );
	}

											//if unauthoring, change spec to find any info files
	if( job.params & unauth )
	{
		setName( specPath );
		if( job.media & imagefile )
			return 0;
		else
			dir.Open( specPath );
	}
											//if authoring, check if project
	else
	{
		if( projectFile && filespec.GetFullPath().Right(6).CmpNoCase( ".lplex" ) == 0 )
		{
			if( Lfiles.size() == 0 && job.projectPath.GetFullPath() == wxEmptyString )
			{
				job.projectPath = filespec;
				job.now |= hasProjectFile;
			}
		}
		else
			projectFile = false;

		if( filespec.GetFullPath().Right(4).CmpNoCase( ".lgz" ) == 0 )
		{
			if( Lfiles.size() == 0 )
			{
#ifdef lgzip_support
				job.params |= ( auth | gzip );
				job.name = filespec.GetName();
				gzFile = filespec.GetFullPath();
				job.outPath = job.inPath.GetPath();
				job.isoPath = job.outPath;
//            job.projectPath = filespec;
				return 0;
#else
				udfError( "Unsupported feature." );
#endif
			}
		}

		if( ! ( job.now & isNamed ) )
		{
			if( projectFile || isDot )
				setName( filespec.GetFullPath(), isDot ? true : false );
												//or if reauthoring
			else
				reauthoring = setName( specPath );
		}

		if( projectFile )
		{
			projectFile = false;
			getOpts( filespec.GetFullPath() );
			return 1;
		}
	}

	specPath = _wxEndSep( specPath );
											//go through and select matching files
	selector.setRoot( specPath, job.params & unauth ? 0 : reauthoring ? 0 : 1 );
	dir.Traverse( selector, filespec.GetFullPath().Mid( specPath.Len() ),
		wxDIR_FILES | wxDIR_DIRS );

	selector.processFiles();

	if( selector.err & lFileTraverser::mismatchA )
	{
		edit |= ::mismatch;
		if( editing && ! ( edit & ::strict ) )
			clearbits( selector.err, lFileTraverser::mismatchA );
	}

	if( ! ( job.params & unauth ) && ( selector.err || ! Lfiles.size() ) )
	{
		_verbose = true;

		if( selector.err & lFileTraverser::mismatchA )
			ERR( "Audio characteristics (bps khz channels) must match within each titleset.\n" );

		if( selector.err & lFileTraverser::mismatchV )
			ERR( "Video dimensions must match within each titleset.\n" );

		if( selector.err & lFileTraverser::mismatchV_ar )
			ERR( "Video aspect ratio must match within each titleset.\n" );

		if( selector.err & lFileTraverser::invalid || ! Lfiles.size() )
		{
			if ( selector.err & lFileTraverser::invalid )
				ERR( "Invalid audio encountered.\n" );
			else if ( ! Lfiles.size() )
				ERR( "No valid audio to process.\n" );

			LOG( "\n" );
			LOG( "Valid audio is either wave or flac lpcm,\n" );
			LOG( "  16 bit : 48 khz, 1-8 channels\n" );
			LOG( "           96 khz, 1-2 channels\n" );
			LOG( "  24 bit : 48 khz, 1-6 channels\n" );
			LOG( "           96 khz, 1-2 channels\n\n" );
		}

		exit( -1 );
	}

	return 1;
}



// ----------------------------------------------------------------------------
//    setName :
// ----------------------------------------------------------------------------
//    Resolves the project name.
//
//    Arguments:
//       <namePath>       - filespec to use as basis for name
//       <isDir>          - whether filespec is a directory.
//
//    Returns 0 on success.
// ----------------------------------------------------------------------------


uint16_t setName( const char *namePath, bool isDir )
{
	if( namePath == projectDotLplex )
		return 0;

	int reauthoring = 0;
	wxFileName fName( namePath );
											//if as yet unspecified, resolve
	if( job.name == wxEmptyString )
	{
											//target name...
		if( job.projectPath.GetFullPath() != wxEmptyString &&
			job.projectPath.GetFullPath() != projectDotLplex )
				job.name = job.projectPath.GetName();
		if( job.name == wxEmptyString )
			job.name = fName.GetFullName();
		if( job.name == wxEmptyString )
			job.name = volumeLabel( namePath, true );
		if( job.name == wxEmptyString )
			job.name = defaultName();
  }

	wxLongLong freeSpace;
	wxGetDiskSpace( _wxValidPath( fName.GetFullPath() ), NULL, &freeSpace );
										//...and location
										//if source is on a hard drive
	if( freeSpace != 0 )
	{
										//...default output is next door to input
		if( ! job.outPath.IsOk() )
		{
//         job.outPath = fName.GetPath() + wxSEP;
//         job.outPath = fName.GetPath();
			if( wxFileName::IsDirWritable( fName.GetPath() ) )
			{
				job.outPath = fName.GetPath();
				if( ! isDir && wxFileName::IsDirWritable( job.outPath.GetPath() ) )
//               job.outPath.RemoveLastDir();
					job.outPath = job.outPath.GetPath();
				job.outPath = job.outPath.GetFullPath() + wxSEP;
			}
		}
		reauthoring = checkName( job.name, true );
	}
										//if source is read-only or outpath isn't writable
										//...output is to default folder
	/* else */ if( ! job.outPath.IsOk() )
	{
		job.outPath = readOnlyPath;
	}

	if( ! ( job.params & redirect ) )
	{
		if( job.group > 0 && ! ( job.params & unauth ) )
		{
			job.outPath = job.outPath.GetPath() + wxSEP;
			job.name = defaultName();
		}

		if( ! ( job.params & gzip ) )
		{
			if( job.params & unauth )
				job.name += "_UNPACKED";
			else if( ! ( job.params & userNamed ) )
				job.name += ( "_DVD"
					+ ( reauthoring ? _f( "_%d", reauthoring ) : "" ) );
		}
	}

	job.now |= isNamed;
	return reauthoring;
}



// ----------------------------------------------------------------------------
//    setJobTargets :
// ----------------------------------------------------------------------------
//    Resolves output paths.
// ----------------------------------------------------------------------------


void setJobTargets()
{
	if( job.projectPath.GetFullPath() == wxEmptyString )
		job.projectPath = ( ( edit & dot ) || editing ) && job.outPath != readOnlyPath ?
			job.inPath.GetFullPath() + job.name + ".lplex" :
			projectDotLplex;

	if( ! job.isoPath.IsOk() )
		job.isoPath = job.outPath.GetPath();
	job.isoPath = _wxEndSep( job.isoPath.GetFullPath() ) + job.name + ".iso";

	if( ! ( job.params & gzip ) )
		job.outPath = job.outPath.GetFullPath() + job.name;
	job.tempPath = job.tempPath.GetFullPath() + job.name + wxSEP;
}


// ----------------------------------------------------------------------------
//    checkName :
// ----------------------------------------------------------------------------
//    Checks whether <jobName> is recycled, and whether to <trim> old suffixes.
//
//    Returns incremented cycle count if recycled.
// ----------------------------------------------------------------------------



int checkName( wxString &jobName, bool trim )
{
	int suffix = 0, reauthoring = 0;
												//...if reauthoring, trim Lplex name suffixes
	if( jobName.Right( 9 ) == "_UNPACKED" )
	{
		if( ! trim )
			return 1;

		suffix = 9;
		if( jobName.Right( 13 ) == "_DVD_UNPACKED" )
		{
			suffix = 13;
			reauthoring = 2;
		}
		else if( jobName.Right( 15 ).Left( 5 ) == "_DVD_" )
		{
			suffix = 15;
			reauthoring = atoi( jobName.Right( 10 ).Left( 1 ) ) + 1;
		}

		jobName.Truncate( jobName.Len() - suffix );
	}

	return reauthoring;
}





// ----------------------------------------------------------------------------
//    defaultName :
// ----------------------------------------------------------------------------
//    Returns a string of the form YYYY-MM-DD_HHMM
// ----------------------------------------------------------------------------


wxString defaultName()
{
	wxDateTime now = wxDateTime::Now();
	return _f( "%d-%02d-%02d_%02d%02d",
		now.GetYear(), now.GetMonth() + 1, now.GetDay(),
		now.GetHour(), now.GetMinute() );
}


// ----------------------------------------------------------------------------
//    validatePath :
// ----------------------------------------------------------------------------
//    Checks whether <path> is valid.
// ----------------------------------------------------------------------------


int validatePath( const char *path )
{
	if( ! _wxValidPath( path )  )
	{
		ERR( _f( "Invalid path '%s'.\n", path ) );
		return 0;
	}
	return 1;
}


// ----------------------------------------------------------------------------
//    isSubStr :
// ----------------------------------------------------------------------------
//    Returns whether <A> is a substring of <B>.
// ----------------------------------------------------------------------------



bool isSubStr( wxString &A, wxString &B ) { return B.Contains( A ); }



// ----------------------------------------------------------------------------
//    splitPaths :
// ----------------------------------------------------------------------------
//    Resolves new rootpoint in absolute project filepaths.
// ----------------------------------------------------------------------------



void splitPaths()
{
	std::sort( dirs.begin(), dirs.end() );
	dirs.erase( std::unique( dirs.begin(), dirs.end(), isSubStr ), dirs.end() );

	sortUnique<infoFile>( infofiles );


	for( int i=0; i < infofiles.size(); i++ )
	{
		for( int j=0; j < dirs.size(); j++ )
		{
			if( isSubStr( dirs[j], infofiles[i].fName ) )
			{
				infofiles[i].root = dirs[j].Len();
				break;
			}
		}
	}

	for( int i=0; i < Lfiles.size(); i++ )
	{
		wxString dir = Lfiles[i].fName.GetFullPath();
		for( int j=0; j < dirs.size(); j++ )
		{
			if( isSubStr( dirs[j], dir ) )
			{
				Lfiles[i].root = dirs[j].Len();
				break;
			}
		}
	}
}


// ----------------------------------------------------------------------------
//    lFileTraverser::setRoot :
// ----------------------------------------------------------------------------
//    Designates which directory in the filepath to set as the root; i.e. which
//    part of the path to keep.
//
//    Arguments:
//       <rootPath>     - root directory
//       <fromParent>   - set root in parent directory of <rootPath>
//
// ----------------------------------------------------------------------------



void lFileTraverser::setRoot( const char *rootPath, int fromParent )
{
	wxFileName rootDir( rootPath );
	if( fromParent )
		rootDir.RemoveLastDir();

	root = rootDir.GetFullPath().Len();
	dirs.push_back( rootDir.GetFullPath() );
}



// ----------------------------------------------------------------------------
//    lFileTraverser::OnFile :
// ----------------------------------------------------------------------------
//    Stores <filename> for later processing.
// ----------------------------------------------------------------------------


wxDirTraverseResult lFileTraverser::OnFile( const wxString& filename )
{
	filenames.push_back( filename );
	return wxDIR_CONTINUE;
}


// ----------------------------------------------------------------------------
//    lFileTraverser::processFiles :
// ----------------------------------------------------------------------------
//    Sorts traversed filenames into alphabetic order and adds an lpcmFile
//    or infofile descriptor for each as appropriate, first verifying dvd
//    compatibility and titleset audio consistency for lpcmFiles.
// ----------------------------------------------------------------------------


void lFileTraverser::processFiles()
{
	wxString errmsg;

	// ensure alphabetic order; wxDir::Traverse() doesn't necessarily
	// proceed alphabetically.
	if( filenames.size() > 1 )
		std::sort( filenames.begin(), filenames.end() );

	for( int i=0; i < filenames.size(); i++ )
	{
		wxString& filename = filenames[i];

		lFile.fName = filename;
		bool ok = true;

		LOG(filename << "\n");
		if( ( lFile.format = isLfile( lFile.fName.GetExt() ) )
			&& ( lFile.format == wavef || lFile.format == flacf ) )
		{
			lFile.group = job.group;
			lFile.trim.type = job.trim & 0x0F;
			lFile.trim.shift = job.trim & 0xF0;
			lFile.root = root;
			lFile.jpgIndex = job.jpegNow;
			lFile.edit = 0;

			if( lFile.format == wavef )
				ok = waveHeader::audit( lFile.fName.GetFullPath(), &lFile.fmeta );
			else if( lFile.format == flacf )
				ok = flacHeader::audit( lFile.fName.GetFullPath(), &lFile.fmeta );

			if( ! ok )
				err |= invalid;

			else if( lpcmEntity::soundCheck( &lFile, false ) )
			{
				if( startNewTitleset )
				{
					lFile.group = ++job.group;
					startNewTitleset = false;
				}

				else
				{
					if( ! lpcmEntity::soundMatch( &lFile, &Lfiles.back(), &errmsg ) )
					{
						err |= mismatchA;
						if( strict )
						{
							ERR( errmsg );
							ok = false;
						}
						else
						{
							WARNv( errmsg << "\n" );
							LOG ("-forcing new titleset.\n" );
							lFile.group = ++job.group;
						}
					}
					if( jpegs[ lFile.jpgIndex ].getDim() !=
						jpegs[ Lfiles.back().jpgIndex ].getDim() )
					{
						err |= mismatchV;
						ok = false;
					}
					if( jpegs[ lFile.jpgIndex ].ar !=
						jpegs[ Lfiles.back().jpgIndex ].ar )
					{
						err |= mismatchV_ar;
						ok = false;
					}
				}
				if( ok )
					Lfiles.push_back( lFile );
			}
			else
				SCRN(LOG_TAG << "-skipping \'" << lFile.fName.GetFullName() << "\'\n");

		}

		else if( job.params & info && filename.Right(9) != "lplex.log" )
		{
#ifndef lplex_console
			job.update |= infoUnsorted;
#endif
			iFile.reject = ( lFile.format && dirSpecified ) ? true : false;
			iFile.fName = filename;
			iFile.root = root;
			if( job.params & unauth )
			{
				if( ( iFile.fName.Mid( root, 8 ) == "VIDEO_TS" ||
						iFile.fName.Mid( root, 8 ) == "AUDIO_TS" )
						&& iFile.fName[ root + 8 ] == wxSEP )
					iFile.reject = true;
				else if( ( iFile.fName.Mid( root, 4 ) == "XTRA" )
						&& iFile.fName[ root + 4 ] == wxSEP )
					iFile.root += 5;
			}
			iFile.edit = 0;
			infofiles.push_back( iFile );
		}
	}
	filenames.clear();
}



// ----------------------------------------------------------------------------
//    lFileTraverser::OnDir :
// ----------------------------------------------------------------------------
//    Adds <dirname> to dir array.
// ----------------------------------------------------------------------------


wxDirTraverseResult lFileTraverser::OnDir( const wxString& dirname )
{
	if( ! dirSpecified ||
			( dirname.Right(3) == "BUP" && dirname.Right(8).Left(4) == "XTRA" ) )
		return wxDIR_IGNORE;
	return wxDIR_CONTINUE;
}




// ----------------------------------------------------------------------------
//    lFileTraverser::OnOpenError :
// ----------------------------------------------------------------------------
//    Reports error.
// ----------------------------------------------------------------------------



wxDirTraverseResult lFileTraverser::OnOpenError( const wxString& openerrorname )
{
	FATAL( "Can't open '" << openerrorname << "'." );
//   return wxDIR_STOP;
}




// ----------------------------------------------------------------------------
//    getOpts :
// ----------------------------------------------------------------------------
//    Reads command line from plain text <filename>.
// ----------------------------------------------------------------------------



void getOpts( const char *filename )
{
	int argc = 0;
	char **argv=NULL, *args=NULL;
	size_t size;

	ifstream optFile( filename, ios::binary );
	if( ! optFile.is_open() )
		FATAL( "Can't open Project file " << filename );

	wxFileName prev = optSrc;
	optSrc = filename;


	optFile.seekg( 0, ios::end );
	size = optFile.tellg();
	args = new char[ size+1 ];
	args[ size ] = '\0';

	optFile.seekg( 0, ios::beg );
	optFile.read( args, size );


#ifdef _ERR2LOG
	cmdline << "\n-------------------------------------------------------------------------------\n"
		<< INFO_TAG << filename << ":" << "\n\n" << args << "\n";
#endif

	if( ! stdArgs( argc, argv, args, size ) )
	{
		delete args;
		FATAL( "Open-ended quotation in " << filename );
	}
	else if( argc == 1 )
	{
		editing = true;
                char *tab[]={ "", "." };
		getOpts( 2, tab );
	}
	else
	{
		getOpts( argc, argv );
		delete args;
		if( argv ) delete[] argv;
	}

	optFile.close();
	optSrc = prev;
}



// ----------------------------------------------------------------------------
//    getOpts :
// ----------------------------------------------------------------------------
//    Processes Lplex's <argc> <argv> argument structure.
//
//    Returns true on success.
// ----------------------------------------------------------------------------



bool getOpts( int argc, char *argv[] )
{
	int opt, nonopts;
											//Store current getopt state...

	int _opterr = opterr;            // (if error message should be printed)
	int _optind = optind;            // (index into parent argv vector)
	int _optopt = optopt;            // (character checked for validity)
//   int _optreset = optreset;        // (reset getopt) **undeclared in unix**
	char *_optarg = optarg;          // (argument associated with option)

											//... and reset.
	opterr = 1;
	optind = 1;
	optopt = '?';
//   optreset = 0;
											//Read the option arguments
	deprecated = 0;
	optindl = -1;

	char *argv0 = argv[0];
	wxString argvZero = _f( "\n*ERR: Bad syntax in %s:\n    ", optSrc.GetFullName().mb_str() );
	argv[0] = (char*)(const char*)argvZero.mb_str();
//DBUG("argv[0]="<<argv[0]);
	while(1)
	{
		consoleColorSet( errColor );
		opt = getopt_long( argc, argv, short_opts, long_opts, &optindl );
		consoleColorRestore();

		if( opt == -1 )
			break;

		if( deprecated )
		{
			opt = deprecated;
			deprecated = 0;
		}
//      else if( opt == ':'  )
//         usage( _f("Bad syntax in %s: Option missing parameter.\n", optSrc.GetFullName().mb_str() ) );
		else if( optarg && optarg[0] == '-'  )
			usage( _f("%s: option '%s' requires an argument.",
//            optSrc.GetFullName().mb_str(), argv[optind-2] ) );
				argv[0] + 7, argv[optind-2] ) );
		else if( opt == '?' )
		{
//DBUG("argv[0]="<<argv[0]);
			usage( "" );
		}

		setopt( opt, optarg );

		optindl = -1;
	}

	argv[0] = argv0;
	banner();

											//Set up file selection:
	nonopts = argc - optind;
	if( ! job.trim0 )
	{
		job.trim0 = job.trim;
	}

											//remaining arguments are either...
	for( int i = 0; i < nonopts; i++ )
	{
		wxString argStr = argv[optind];
		argStr.Trim();
		argStr.Trim( false );
		const char *arg = argStr.mb_str();

											//...markers
		if( ! stricmp( arg, "ts" ) )
		{
			startNewTitleset = true;
			optind++;
			continue;
		}
		else if( ! stricmp( arg, "prj" ) )
		{
			projectFile = true;
			optind++;
			continue;
		}
		else if( ! stricmp( arg, "jpg" )
			|| ! stricmp( arg, "jpgw" ) )
		{
			bool ws = stricmp( arg+3, "w" ) ? false : true;
			screenJpg = true;
			addJpeg( argv[ ++optind ], job, Lfiles.size() ? false : true, ws );
			optind++;
			i++;
			continue;
		}
		else if( ! stricmp( arg, "seamless" )
			|| ! stricmp( arg, "discrete" )
			|| ! stricmp( arg, "padded" ) )
		{
			job.trimCt++;
			setopt( 'l', argv[optind] );
			optind++;
			continue;
		}
											//...or filespecs:

											//strip end quote, if any
											//(getopt interprets quoted trailing backslash
											//in dos directory paths as a literal quote
		if( argv[optind][strlen(argv[optind])-1] == '\"' )
			argv[optind][strlen(argv[optind])-1] = '\'';

		addFiles( wxFileName( argv[optind] ) );

		if( job.params & unauth )
			return 1;
		optind++;
	}

											//restore getopt...
	opterr = _opterr;
	optind = _optind;
	optopt = _optopt;
//   optreset = _optreset;
	optarg = _optarg;

	return true;
}



// ----------------------------------------------------------------------------
//    stdArgs :
// ----------------------------------------------------------------------------
//    Reformats given char buffer <args> of total length <size> into a standard
//    <argc> <argv> argument structure, erasing any unix-style comments first.
//
//    Returns true on success.
// ----------------------------------------------------------------------------



bool stdArgs( int &argc, char** &argv, char *args, size_t size )
{
	argc = 1;
	bool enquoted = false, inComment=false, firstChar=true;

	for( int i=0; i < size; i++ )
	{
		bool whitespace = true;
		switch( args[i] )
		{
			case '\"':
				if( ! inComment )
					enquoted = ! enquoted;
				break;

			case '#':
				if( ! enquoted )
					inComment = true;
				else
					whitespace = false;
				break;

			case '\n':
				inComment = false;
				break;

			default:
				if( enquoted ||
					( ! inComment && ! ( isspace( args[i] ) || args[i] == '=' ) ) )
					whitespace = false;
				break;
		}

		if( whitespace )
		{
			args[i] = '\0';
			firstChar = true;
		}

		else if( firstChar )
		{
			argc++;
			firstChar = false;
		}
	}

	if( enquoted )
		return false;

	argv = new char*[ argc ];
	argv[0] = NULL;
	firstChar = true;

	for( int i=0, j=0; i < size; i++ )
	{
		if( args[i] == '\0' )
			firstChar = true;

		else if( firstChar )
		{
			argv[ ++j ] = args + i;
			firstChar = false;
			_wxFixSeparators( argv[j] );
		}
	}

	return true;
}



// ----------------------------------------------------------------------------
//    setopt :
// ----------------------------------------------------------------------------
//    Sets the given config file or command line option.
//
//    Arguments:
//       <opt>          - option id
//       <optarg>       - option argument
//
//    Returns 1 on success, 0 on fail.
// ----------------------------------------------------------------------------


uint16_t setopt( uint16_t opt, const char *optarg )
{
	uint16_t t;
	char *comma = NULL;
	bool ok = true, isTrue = 0, isFalse = 0;

	if( optarg && ( comma = (char*)strrchr( optarg, ',' ) ) )
		comma[0] = '\0';

	if( optarg == NULL || ! stricmp( optarg, "yes" ) || ! stricmp( optarg, "true" ) || ! stricmp( optarg, "1" ) )
		isTrue = true;
	else if( ! stricmp( optarg, "no" ) || ! stricmp( optarg, "false" ) || ! stricmp( optarg, "0" ) )
		isFalse = true;

	switch( opt )
	{
		case 0:
			break;

		case 'u':
			clearbits( job.params, jobMode );
			job.params |= unauth;
			break;

		case 'd':
			ok = validatePath( optarg );
			job.outPath = _wxEndSep( optarg );
			job.outPath = job.outPath.GetPath();
			job.name = job.outPath.GetFullName();
			job.outPath = job.outPath.GetPath() + wxSEP;
			job.params |= ( redirect | userNamed );
			break;

		case 'n':
			job.name = optarg;
			job.params |= userNamed;
			break;

		case 'f':
			if( ! strncmp( optarg, "flac", 4 ) )
			{
				job.format = flacf;
				if( strlen( optarg ) == 5 )
				{
					if( optarg[4] >= '0' && optarg[4] <= '8' )
						job.flacLevel = optarg[4] - '0';
					else ok = false;
				}
				else if( strlen( optarg ) != 4 ) ok = false;
			}
			else if( ! stricmp( optarg, "wave" ) )
				job.format = wavef;
			else if( ! stricmp( optarg, "wav" ) )
				job.format = wavef;
			else if( ! stricmp( optarg, "raw" ) )
				job.format = lpcmf;
//#ifdef lgzip_support
			else if( ! stricmp( optarg, "lgz" ) )
				job.format = lgzf;
//#endif
			else ok = false;
			break;

		case 't':
			if( job.params & customized )
				t = job.tv;
			if( ! stricmp( optarg, "pal" ) || ! stricmp( optarg, "secam" ) )
				job.tv = PAL;
			else if( ! stricmp( optarg, "ntsc" ) )
				job.tv = NTSC;
			else ok = false;
			if( job.params & customized  && t != job.tv )
				FATAL( "set --video (-t) prior to specifying any custom jpegs or menus." );
			break;

			case 'm':
			clearbits( job.params, md5 );
			if( isTrue ) job.params |= md5;
			else ok = isFalse;
			break;

			case 'r':
			clearbits( job.params, restore );
			if( isTrue ) job.params |= restore;
			else ok = isFalse;
			break;

			case 'x':
			clearbits( job.params, info );
			if( isTrue ) job.params |= info;
			else ok = isFalse;
			break;

			case 'i':
			ok = validatePath( optarg );
			job.infoPath = _wxEndSep( optarg );
			job.params |= infodir;
			break;

			case 'l':
			if( ! stricmp( optarg, "none" ) )
				job.trim = notrim;
			else if( ! stricmp( optarg, "pad" ) )
				job.trim = discrete;
			else if( ! stricmp( optarg, "discrete" ) )
				job.trim = discrete;
			else if( ! stricmp( optarg, "padded" ) || ! stricmp( optarg, "indiscrete" ) )
			{
				job.trim = discrete | padded;
			}
			else if( ! stricmp( optarg, "seamless" ) )
				job.trim = seamless;
			else ok = false;

			if( job.trim & continuous )
				job.now |= appending;
			else
				clearbits( job.now, appending );

			break;

			case 's':
			clearbits( job.trim, backward | nearest | forward );
			if( ! stricmp( optarg, "backward" ) )
				job.trim |= backward;
			else if( ! stricmp( optarg, "nearest" ) )
				job.trim |= nearest;
			else if( ! stricmp( optarg, "forward" ) )
				job.trim |= forward;
			else ok = false;
			break;

			case 'j':
			addJpeg( optarg, job, true );
			break;

			case 'N':
			menuForce = true;
			case 'M':
			addMenus( optarg, job.tv, menuForce );
//         menuForce = false;
			break;

			case 'R':
			clearbits( job.params, rescale );
			if( isTrue ) job.params |= rescale;
			else ok = isFalse;
			break;

			case 'C':
			clearbits( job.params, cleanup );
			if( isTrue ) job.params |= cleanup;
			else ok = isFalse;
			break;

			case 'c':
			clearbits( job.params, dvdStyler );
			if( ! stricmp( optarg, "lpcm" ) )
				job.prepare = lpcmf;
			else if( ! stricmp( optarg, "m2v" ) )
				job.prepare = m2vf;
			else if( ! stricmp( optarg, "mpeg" ) )
				job.prepare = mpegf;
			else if( ! stricmp( optarg, "dvdStyler" ) )
			{
				job.params |= dvdStyler;
				job.prepare = mpegf;
			}
			else if( ! stricmp( optarg, "dvd" ) )
				job.prepare = vobf;
			else if( ! stricmp( optarg, "iso" ) )
				job.prepare = isof;
//#ifdef lgzip_support
			else if( ! stricmp( optarg, "lgz" ) )
				job.prepare = lgzf;
//#endif
			else ok = false;
			break;

		case 'z':
			if( ! stricmp( optarg, "none" ) )
				job.media = unspecified;
			else if( ! stricmp( optarg, "dvd+r" ) )
				job.media = plusR;
			else if( ! stricmp( optarg, "dvd-r" ) )
				job.media = minusR;
			else if( ! stricmp( optarg, "dl" ) )
				job.media = plusR_DL;
			else ok = false;
			break;

		case 'p':
			if( ! stricmp( optarg, "adjacent" ) ) break;
			ok = validatePath( optarg );
			job.dvdPath = _wxEndSep( optarg );
			break;

		case 'w':
			ok = validatePath( optarg );
			job.tempPath = _wxEndSep( optarg );
			break;

		case 'a':
			if( ! stricmp( optarg, "adjacent" ) ) break;
			ok = validatePath( optarg );
			job.isoPath = _wxEndSep( optarg );
			break;

		case 'E':
			if( ! stricmp( optarg, "adjacent" ) ) break;
			ok = validatePath( optarg );
			job.extractTo = _wxEndSep( optarg );
			break;

		case 'D':
			ok = validatePath( optarg );
			readOnlyPath = _wxEndSep( optarg );
			break;

		case 'v':
			if( isTrue ) _verbose = 1;
			else ok = isFalse;
			break;

		case 'V':
			endPause = false;
			_verbose = 1;
			version();
			exit(0);

		case 'Q':
			endPause = false;
			version( " (GNU GPL License)" );
			GPL_notice();
			exit(0);

		case '?':
		case 'h':
			usage();

		case 'e':
			if( comma )
			{
				while( *++comma )
				{
					if( comma[0] == 'v' )
						edit |= editVerbose;
					else if( comma[0] == 'p' )
						edit |= piped;
				}
			}
			if( isTrue || ! stricmp( optarg, "relative" ) )
				editing = relative;
			else if( isFalse )
				editing = 0;
			else if( ! stricmp( optarg, "absolute" ) )
				editing = absolute;
			else if( ! stricmp( optarg, "." ) )
			{
				if( ! editing ) editing = true;
				edit |=  dot;
				addFiles( wxFileName( "." ) );
			}
			else if( ! stricmp( optarg, "strict" ) )
			{
				if( ! editing ) editing = true;
				edit |= strict;
			}
			else if( ! stricmp( optarg, "lax" ) )
			{
				if( ! editing ) editing = true;
				clearbits( edit, strict );
			}
			else ok = false;
			break;

		case 'P':
			if( isFalse ) endPause = false;
			else endPause = true;
			break;

#if defined(WIN32_COLOR) || defined(ANSI_COLOR)
		case 'L':
			if( ! stricmp( optarg, "none" ) || isFalse )
				setcolors(false);
			else if( ! stricmp( optarg, "bright" ) || isTrue )
				setcolors( bright );
			else if( ! stricmp( optarg, "dark" ) )
				setcolors( dark );
			else if( ! stricmp( optarg, "ansi" ) )
				colorMode = ansi;
			else ok = false;
			break;
#endif

		case 'Z':
#ifdef lgzip_support
			if( isTrue ) job.params |= gzip;
			else ok = isFalse;
#endif
			break;

		case 'q':
			cerr.rdbuf(cerr.rdbuf());
			break;

		case 'G':
			if( isTrue ) debug = true;
			else ok = isFalse;
			break;

		case 'K':
			t = atoi( optarg );
			job.skip = ( t ? t - 1 : 0 );
			break;

		default :
			ok = false;
			break;
	}

	if( ! ok )
		usage( _f( "Bad syntax in %s:\n    : option '%s' has invalid argument '%s'",
			optSrc.GetFullName().mb_str(),
			optindl < 0 ? _f( "-%c", opt ).mb_str() : _f( "--%s", long_opts[optindl].name ).mb_str(),
			optarg ) );

	return 1;

}



// ----------------------------------------------------------------------------
//    saveOpts :
// ----------------------------------------------------------------------------
//    Writes given <layout> to a plain-text project file with comments.
//
//    Returns true on success.
// ----------------------------------------------------------------------------


bool saveOpts( dvdLayout *layout )
{
	lplexJob &job = *layout->job;
	vector<lpcmFile> &Lfiles = *layout->Lfiles;
	vector<infoFile> &infofiles = *layout->infofiles;
	wxString projectFile;

	bool generating;

	if( generating = ! editing )
	{
		editing = absolute;
		projectFile = projectDotLplex;
	}
	else
	{
		if( job.projectPath == projectDotLplex ) editing = absolute;
		projectFile = job.projectPath.GetFullPath();
	}

	ofstream optFile;
	if( ! ( edit & piped ) )
	{
		optFile.open( projectFile );
		if( ! optFile.is_open() )
			FATAL( "Can't open project file '" << projectFile << "'" );
	}
	else
	{
//      cerr.sync_with_stdio();
		POST( "\n" );
	}

	ofstream & dotLplex = ( edit & piped ? (ofstream &)cerr : (ofstream &)optFile );

	if( ! generating )
	{
		POST( "\n" );
		if( edit & ::mismatch )
			WARNv( "Multiple titles were auto-generated to accomodate different audio types.\n"
				<< ( edit & piped ? "\n" :
					_f( "%sPlease review project file and rearrange layout as appropriate.\n\n", LOG_TAG ).mb_str() ) );
		if( ! ( edit & piped ) )
		{
			SCRN( "Writing project file '"
				<< TINT( projectFile ) << "'\n\n" );
			INFO( "Writing project file '"
				<< projectFile << "'\n\n" );
		}
	}


	char *T = "yes", *F = "no";
	int bitPos[] = { 0,1,2,0,3,0,0,0,4 };

	dotLplex << shebang << endl <<
		"# lplex version " << LPLEX_VERSION_STRING  << endl <<
		"# Project : " << job.name << endl <<
		"# " << layout->spaceTxt << endl << endl <<

		"# Settings:\n\n";

	if( job.params & redirect && editing == absolute ) dotLplex <<
		"--dir="         << QUOTE( job.outPath.GetPath() + wxSEP + job.name ) << endl; // -d
	else dotLplex <<
		"--name="        << QUOTE( job.name ) << endl; // -n

	dotLplex <<
		"--md5aware="    << ( job.params & md5 ? T:F ) << endl << // -m
		"--infoFiles="   << ( job.params & info ? T:F ) << endl << // -x
		"--splice="      << (
			job.trim0 & discrete ? job.trim0 & padded ? "padded" : "discrete" :
			job.trim0 & seamless ? "seamless " : "none" ) << endl << // -l
		"--shift="       << (
			job.trim0 & backward ? "backward" :
			job.trim0 & nearest ? "nearest" :
			job.trim0 & forward ? "forward" : "" ) << endl << // -s
		"--create="      << ( job.params & dvdStyler ? "dvdStyler" :
			((char*[]){ "lpcm", "m2v", "mpeg", "dvd", "iso", "lgz" }) [ job.prepare-3 ] ) << endl << // -c
		"--media="       << ((char*[]){ "dvd+r", "dvd-r", "dl", "none"  } ) [ bitPos[ job.media ] ] << endl << // -z
		"--cleanup="     << ( job.params & cleanup ? T:F ) << endl; // -C

	if( jpegs.size() == 1 ) dotLplex <<
		"--jpeg="        << ( alias( jpegs[0].fName ) ?
			jpegs[0].fName.GetFullPath() : QUOTE( jpegs[0].fName.GetFullPath() ) ) << endl; // -j

	if( editing == absolute )
	{
		dotLplex <<
		"--video="       << ( job.tv == NTSC ? "ntsc" : "pal" ) << endl << // -t
//      "--jpeg="        << QUOTE( job.jpeg.GetFullPath() ) << endl << // -j
		"--dvdpath="     << QUOTE( job.dvdPath.GetPath() ) << endl << // -p
		"--workpath="    << QUOTE( wxPathOnly( job.tempPath.GetPath() ) ) << endl << // -w
		"--isopath="     << QUOTE( job.isoPath.GetPath() ) << endl; // -a
//      "--extractPath=" << QUOTE( job.extractPath.GetPath() ) << endl << // -e
	}

	dotLplex <<
		"--verbose="     << ( _verbose ? T:F ) << endl << // -v
		( generating ? "#" : "" ) <<
		"--editing="     << ((char*[]){ "false", "relative", "absolute" }) [ editing ]
			<< ( edit & editVerbose ? ",v" : "" ) << endl; // -Y

	if( Lfiles.size() )
	{
		if( editing == relative  )
			for( int i=0; i < jpegs.size(); i++ )
				jpegs[i].fName.MakeRelativeTo( job.projectPath.GetPath() );

		if( menufiles.size() )
		{
			if( editing == relative  )
			{
				wxFileName fName( menuPath );
				fName.MakeRelativeTo( job.projectPath.GetPath() );
				menuPath = fName.GetFullPath();
			}
			dotLplex << "\n--menu" << ( menuForce ? "force=" : "=" ) << QUOTE( menuPath.mb_str() ) << "\n";

			if( ! generating )
			{
				for( int i=0; i < menufiles.size(); i++ )
				{
					if( editing == relative )
					{
						wxFileName fName( menufiles[i] );
						fName.MakeRelativeTo( job.projectPath.GetPath() );
						menufiles[i] = fName.GetFullPath();
					}
					dotLplex << "# " << QUOTE( menufiles[i].mb_str() ) << "\n";
				}
			}
		}

		dotLplex << "\n# VIDEO_TS" << endl;

		int titleset = 100, jpgIndex = jpegs.size() > 1 ? -1 : 0;
		uint16_t trimType = job.trimCt ? 0 : job.trim0 & 0x0F;
		lpcmFile *lFile;

		for( int i=0; i < Lfiles.size(); i++ )
		{
			lFile = &Lfiles.at(i);

			if( lFile->group != titleset )
			{
				titleset = lFile->group;
				dotLplex << ( titleset ? "\nts" : "" ) <<
					_f( "\n# Title %d - (%s / %s %s)\n",
						titleset + 1, lpcmEntity::audioInfo( lFile ).mb_str(),
						jpegs[lFile->jpgIndex].sizeStr(), jpegs[lFile->jpgIndex].aspStr() ) << endl;
			}

			if( lFile->trim.type != trimType && ! ( lFile->trim.type & autoSet ) )
			{
				trimType = lFile->trim.type;
				dotLplex << (
					trimType & discrete ? trimType & padded ? "padded" : "discrete" :
					trimType & seamless ? "seamless " : "none" ) << endl;
			}

			if( lFile->jpgIndex != jpgIndex )
			{
				jpgIndex = lFile->jpgIndex;
				dotLplex << ( jpegs[ jpgIndex ].ar == dvdJpeg::_16x9 ? "jpgw=" : "jpg=" )
					<< ( alias( jpegs[ jpgIndex ].fName ) ?
						jpegs[ jpgIndex ].fName.GetFullPath() :
						QUOTE( jpegs[ jpgIndex ].fName.GetFullPath() ) );
				dotLplex << endl;
			}

			if( editing == relative )
				lFile->fName.MakeRelativeTo( job.projectPath.GetPath() );

			dotLplex << QUOTE( lFile->fName.GetFullPath() );
			if( edit & editVerbose )
			{
				for( int s=lFile->fName.GetFullPath().Length(); s < 50; s++ )
					dotLplex << ' ';
				dotLplex << _f("   # %4d MB %7s",
					(uint32_t) (dvdUtil::sizeOnDvd( lFile, job.tv == NTSC ) / MEGABYTE),
					dvdUtil::time( lFile->videoFrames, job.tv == NTSC ).mb_str() );
			}
			dotLplex << endl;
		}
	}

	if( infofiles.size() )
	{
		dotLplex << "\n# XTRA - Info files\n" << endl;

		for( int i=0; i < infofiles.size(); i++ )
		{
			if( editing == relative )
			{
				wxFileName fName( infofiles[i].fName );
				fName.MakeRelativeTo( job.projectPath.GetPath() );
				infofiles[i].fName = fName.GetFullPath();
			}
			if( infofiles[i].reject )
				continue;
			dotLplex << QUOTE( infofiles[i].fName ) << "\n";
		}
		for( int i=0; i < infofiles.size(); i++ )
		{
			if( infofiles[i].reject )
				dotLplex << "#" << QUOTE( infofiles[i].fName ) << "\n";
		}
	}

	dotLplex << "\n";

	if( ! ( edit & piped ) )
		dotLplex.close();
}



#ifndef lplex_console

// ----------------------------------------------------------------------------
//    update :
// ----------------------------------------------------------------------------
//    3 functions to re-sort and apply editing changes to <lFiles> and <iFiles>
//    based on settings in <job>.
// ----------------------------------------------------------------------------



void update( vector<lpcmFile> *lFiles,
	vector<infoFile> *iFiles, lplexJob *job )
{
	if( job->update & edited )
	{
		update( lFiles );
	}

	if( job->update & infoUnsorted )
	{
		update( iFiles );
	}

	job->update = 0;
}


void update( vector<lpcmFile> *lFiles )
{
	vector<lpcmFile>::iterator next = lFiles->begin();

	while( next != lFiles->end() )
	{
		if( (*next).edit & lpcmEntity::remove )
			lFiles->erase( next );
		next++;
	}

	sort( lFiles->begin(), lFiles->end() );

	for( int i=0; i < lFiles->size(); i++ )
	{
		lpcmFile lFile = lFiles->at( i );
		lFile.index = i;
		lFile.edit = 0;
	}
}

void update( vector<infoFile> *infofiles )
{
	sort( infofiles->begin(), infofiles->end() );

	vector<infoFile>::iterator next = infofiles->begin() + 1;
	infoFile *prev = &*( next - 1 );

	while( next != infofiles->end() )
	{
		if( *next == *prev || (*next).edit & lpcmEntity::remove )
			infofiles->erase( next );
		else
		{
			prev = &*next;
			next++;
		}
	}
}

#endif



// ----------------------------------------------------------------------------
//    version :
// ----------------------------------------------------------------------------
//    Displays version, <str>, and build info.
// ----------------------------------------------------------------------------

void version( const char * str )
{
	POST( "lplex version " << LPLEX_VERSION_STRING << str << "\n" );
	ECHO( "build:"
#ifdef build_defs
		<< " " << build_host << " " << build_defs << "\n     :"
#endif
		<< " flac "     << FLAC__VERSION_STRING
		<< "  dvdread " << DVDREAD_VERSION
		<< "  "         << _f(wxVERSION_STRING)
		<< "\n" );
}


// ----------------------------------------------------------------------------
//    banner :
// ----------------------------------------------------------------------------
//    Displays program banner.
// ----------------------------------------------------------------------------


bool bannerShown = false;

void banner()
{
	if( bannerShown )
		return;
	bannerShown = true;
	version( " (GNU GPL License)" );
	POST( "feedback: <audioplex-lpcm@lists.sourceforge.net>\n\n" );
	wxDateTime now = wxDateTime::Now();
	INFO( now.FormatISODate().mb_str() << " " << now.FormatISOTime().mb_str() << "\n\n" );
}


// ----------------------------------------------------------------------------
//    usage :
// ----------------------------------------------------------------------------
//    Displays <str>, then generic help info.
// ----------------------------------------------------------------------------


void usage( const char *str )
{
	fflush(stderr);
	consoleColorRestore();

	if( str )
	{
		if( str[0] )
			ERR( str << "\n" );

		if( endPause )
		{
			char c = 'y';
			cerr << "\nshow lplex help? ...y\b";
			cin.get(c);
			if( c != 'y' && c != 0x0A )
			{
				endPause = false;
				exit(1);
			}
		}
		else
		{
			cerr << endl << "type 'lplex -h' for program help.\n";
			exit(1);
		}
	}

	cerr << endl <<

	" Usage: lplex [options] <files> ... [flags] <files> ...\n\n"

	" Options          Values (default=*)    (1/0 true/false can be used for yes/no)\n\n"

	" -t --video       pal|secam|ntsc *      -use this tv standard.\n"
	" -c --create      lpcm|m2v|dvdstyler    -author to this stage.\n"
#ifdef lgzip_support
	"                  mpeg|dvd|iso|lgz *\n"
#else
	"                  mpeg|dvd|iso *\n"
#endif
	" -n --name        <projectname>         -name the project this.\n"
	" -m --md5aware    no|yes *              -insert Lplex tags into the dvd.\n"
	" -l --splice                            -splice the tracks together this way:\n"
	"                  seamless *             continuous, gapless, e.g. a concert\n"
	"                  discrete|padded        separate, a compilation w gaps|padding\n"
	"                  none                   truncated, allow audio loss\n"
	" -s --shift       forward|nearest       -move seamless startpoints in this\n"
	"                  backward *             direction.\n"
	" -x --infofiles   no|yes *              -make an 'XTRA' info folder on the dvd.\n"
	" -i --infodir     <dir>                 -copy files in this folder to 'XTRA'.\n"
	" -j --jpeg        <filename>            -use this jpeg as the background,\n"
	"                  black *                or use a default black screen:\n"
	"                  black_#(L,M,S,XS)         L       M       S       XS\n"
	"                                         720x480 704x480 352x480 352x240* NTSC\n"
	"                                         720x576 704x576 352x576 352x288*  PAL\n"
	" -R --rescale     no|yes *              -rescale jpegs ntsc<->pal if necessary.\n"
	" -z --media       none|dl|dvd-r|dvd+r * -don't exceed this disc size.\n"
	" -e --editing     yes|no *              -do a demo run and write a project file\n"
	"                  absolute|relative      using this type of filename\n"
//   "                  strict|lax *           whether to auto-generate titlesets\n"
	"                  [,v]                   with verbose comments\n"
	"                  [,p]                   print project file to stderr\n"
	"                                         (e.g. '--editing=relative,v')\n"
	" -M --menu        <dir>                 -use these custom dvd menus.\n"
	" -d --dir         <dir>                 -output everything to this directory.\n"
	" -p --dvdpath     <dir>|adjacent *      -output dvd files to this directory.\n"
	" -w --workpath    <dir>|adjacent *      -use this folder for temporary space.\n"
	" -a --isopath     <dir>|adjacent *      -output disc image to this directory.\n"
	" -E --extractpath <dir>|adjacent *      -extract to this directory.\n"
	" -u --unauthor                          -extract audio from dvd.\n"
#ifdef lgzip_support
	" -f --formatout   lgz|raw|wav *         -extract audio to this format.\n"
#else
	" -f --formatout   raw|wav *             -extract audio to this format.\n"
#endif
	"                  flac|flac#(0-8)        (flac equals flac8)\n"
	" -r --restore     no|yes *              -restore files to original length.\n"
#ifdef lgzip_support
	" -Z --lgz                               -convert dvd to a .lgz container file.\n"
#endif
	" -C --cleanup     no|yes *              -delete interim files when done.\n"
	" -v --verbose     yes|no *              -show all messages.\n"
	" -L --color       "
#ifdef lplex_win32
							"dark|bright"
#else
							"bright|dark"
#endif
											" *         -colorize console output.\n"
	"                  no|yes\n"
	" -P --pause       "
#ifdef lplex_win32
							"no|yes"
#else
							"yes|no"
#endif
									" *              -pause console before exiting.\n"
	"    --version                           -print out the version and build info.\n"
	"    --license                           -print out the GNU GPL License notice.\n"
	" -h --help                              -print this lot out!\n\n"

	" Flags:\n"
	"  ts                                    -start a new titleset here.\n"
	"  jpg            <jpegfile>             -use this  4:3 background from now on.\n"
	"  jpgw           <jpegfile>             -use this 16:9 background from now on.\n"
	"  prj            <projectfile>          -merge this .lplex project file here.\n"
	"  seamless|discrete|padded              -use this splice from now on.\n"

	"\n                                Examples\n\n"
	" Create a single-title NTSC dvd fileset:\n"
#ifdef lplex_win32
	"      lplex --video=ntsc --create=dvd c:\\myAudio\n\n"
#else
	"      lplex --video=ntsc --create=dvd ~/myAudio\n\n"
#endif

	" Create a 2 title PAL dvd with different backgrounds and splicing:\n"
#ifdef lplex_win32
	"      lplex -t pal discrete jpg=c:\\a.jpg \"c:\\My Songs\" ts seamless\n"
	"         jpg=c:\\b.jpg c:\\myConcert\n\n"
#else
	"      lplex -t pal discrete jpg=a.jpg mySongs ts seamless jpg=b.jpg myConcert\n\n"
#endif

	" Extract audio from a dvd disc to a specific folder at flac level 6:\n"
#ifdef lplex_win32
	"      lplex --dir=\"c:\\My Flacs\" --formatout=flac6 d:\n\n\n";
#else
	"      lplex --dir=~/myFlacs --formatout=flac6 /dev/dvd\n\n\n";
#endif

	exit(1);
}

// ----------------------------------------------------------------------------
//    GPL_notice :
// ----------------------------------------------------------------------------


void GPL_notice()
{
	cerr << endl <<
	"   This program is free software; you can redistribute it and/or\n"
	"   modify it under the terms of the GNU General Public License as\n"
	"   published by the Free Software Foundation; either version 2 of\n"
	"   the License, or (at your option) any later version.\n\n"

	"   This program is distributed in the hope that it will be useful,\n"
	"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
	"   GNU General Public License for more details.\n\n"

	"   You should have received a copy of the GNU General Public\n"
	"   License along with this program; if not, write to the\n"
	"   Free Software Foundation, Inc., 59 Temple Place, Suite 330,\n"
	"   Boston, MA 02111-1307 USA\n\n";
}




