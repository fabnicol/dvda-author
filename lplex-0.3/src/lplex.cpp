/*
	lplex.cpp - top-level authoring and extraction.
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
//    author :
// ----------------------------------------------------------------------------
//    Control routine to transcode and multiplex audio files into DVD format.
//    Runs mplex, dvdauthor, and mkisofs.
//
//    Returns 0 on success, fatal on fail
// ----------------------------------------------------------------------------


int author( dvdLayout &layout )
{
	int i, valid_audio = 0, m2vUpdate = -1, multichapter = 0, done = 0;
	uint16_t uDataLen = 0;
	uint8_t userData[512];
	uint32_t vFrames = 0;
	ofstream *m2vFile = NULL;


#ifndef lgzip_support
	if( job.prepare >= lgzf )
	{
		ECHOv ( "\n" );
		WARNv( "Changing target from 'lgz' to 'iso'.\n" );
		LOGv ( "-this build of Lplex doesn't support dvd container (.lgz) files.\n\n" );
		job.prepare = isof;
	}
#endif

	if( job.params & customized ) ECHO( "\n" );
	string txt = " ";
	txt +=
        job.trim0 & jobs::discrete ? job.trim0 & jobs::padded ? "padded" : "discrete" :
        job.trim0 & jobs::seamless ? "seamless " : "none";

    if( job.trim & jobs::seamless ) txt +=
        job.trim & jobs::backward ? "backward" :
        job.trim & jobs::nearest ? "nearest" :
        job.trim & jobs::forward ? "forward" : "";

	if( job.params & customized )
        SCRN( "\n\n" )

    INFOv( string("Creating ")
        + string(((const char*[]){ "LPCM", "M2V", "MPEG", "DVD", "ISO", "LGZ" }) [ job.prepare-3 ])
        + string(" : [ ")
        + string(( job.tv == NTSC ? "NTSC" : "PAL" ))
        + string(( job.params & md5 ? " md5aware" : "" ))
        + string(( job.params & info ? " infofiles" : "" ))
        + string(( job.params & dvdStyler ? " dvdStyler" : "" ))
        + string(( job.trimCt == 0 ? txt.c_str() : "" )) + string(" ]\n") );


	layout.configure();

	if( editing )
		return 0;


	stopWatch.Start();

	dvdauthorXml xml( (job.tempPath / job.name).string(),
		job.tv, job.group + 1, job.params & dvdStyler, job.prepare < mpegf );

	xml.write( dvdauthorXml::setDest, job.dvdPath.string(),
		jpegs[ Lfiles[0].jpgIndex ].ar == dvdJpeg::_16x9 ? true : false );

	while( layout.getNext() )
	{
#ifndef lplex_console
		wxYieldIfNeeded();
#endif
		valid_audio++;
		multichapter |= ( job.now & appending );

		if( job.prepare <= lpcmf )
			continue;

		i = layout.writeIndex;

		if( ! ( job.now & appending ) )
		{
			vFrames = 0;

			if( i )
			{
				xml.write( dvdauthorXml::closeVob );

				if( Lfiles[i].type & lpcmFile::titleStart )
					xml.write( dvdauthorXml::addTitle, job.dvdPath.string(),
						jpegs[ Lfiles[i].jpgIndex ].ar == dvdJpeg::_16x9 ? true : false );
			}
			xml.write( dvdauthorXml::openVob, layout.nameNow + ".mpg" );

		}


        txt.clear();
        txt = STAT_TAG + Lfiles[i].fName.filename().string();
		if( jpegs.size() > 1 )
		{
            txt += string(" + ") + jpegs[ Lfiles[i].jpgIndex ].fName.filename().string();
            SCRN( string(" + ") +  jpegs[ Lfiles[i].jpgIndex ].fName.filename().string()  )
		}

		if( job.params & md5 )
		{
			if( m2vUpdate > -1 && Lfiles[m2vUpdate].type & lpcmFile::readComplete )
			{
				uDataLen = writeUserData( &Lfiles[m2vUpdate], userData );
				m2vFile->write( (char*)userData, uDataLen );
			}
			uDataLen = writeUserData( &Lfiles[i], userData );
		}

		m2vUpdate = job.params & md5 ?
			Lfiles[i].type & lpcmFile::readComplete ? -1 : i : -1;

		BLIP( " ...creating video " );
		cerr << "m2v" << " " << layout.nameNow << ".m2v" << endl;

		m2vFile = m2v(Lfiles[i].videoFrames,
			          jpegs[ Lfiles[i].jpgIndex ].getName().c_str(),
                      string(layout.nameNow + ".m2v").c_str(),
		       	      job.tv,
			          jpegs[ Lfiles[i].jpgIndex ].ar == dvdJpeg::_16x9 ? true : false,
			          userData,
		       	      uDataLen,
		       	      (job.tv == NTSC ? 18 : 15),
			          job.now & appending,
                      ! (Lfiles[i].trim.type & jobs::continuous),
                      ! (Lfiles[i].trim.type & jobs::continuous));

		xml.write( dvdauthorXml::addChapter, job.now & appending ?
			dvdauthorXml::timestampFractional( vFrames, job.tv == NTSC, .001 ) : "0" );

		vFrames += Lfiles[i].videoFrames;

        if( Lfiles[i].trim.type & jobs::continuous )
            SCRN( "\n" )

		ECHO( "\n" );

        if( Lfiles[i].trim.type & jobs::continuous &&
				! ( Lfiles[i].type & lpcmFile::seqEnd ) )
			continue;

		if( job.prepare < mpegf )
			continue;

		ECHO( "----------------------------------- MPLEX -------------------------------------\n\n"  );
		BLIP( " ...multiplexing " );

		_progress.max = vFrames;

        string o1 = layout.nameNow + string(".mpg");
        string o2 = layout.nameNow + string(".m2v");
        string o3 = layout.nameNow + string(".lpcm");
        string Larg = _f( "%d:%d:%d",
                          Lfiles[i].fmeta.data.stream_info.sample_rate,
                          Lfiles[i].fmeta.data.stream_info.channels,
                          Lfiles[i].fmeta.data.stream_info.bits_per_sample) ;


        const vector<const char*>&  cmdline =
                 {"-f",
                  "8",
                  //job.mplexArg.c_str(),
 				  "-L",
                  Larg.c_str(),
				  "-o",
                  o1.c_str(),
                  o2.c_str(),
                  o3.c_str()
                 };

        cerr << "[MSG] Now launching: \n\n";
        for (const char* s: cmdline) cerr << s << " ";
        cerr << endl << endl;

		if( execute(
                    (binDir / "mplex").string(),
                    {"-f",
                     "8",
                   // job.mplexArg.c_str(),
                     "-L",
                     Larg.c_str(),
                     "-o",
                     o1.c_str(),
                     o2.c_str(),
                     o3.c_str()
                    },
                    _verbose))
        {
			FATAL( "mplex failed. See lplex.log for details.\n" );
        }

		ECHO( "\n-------------------------------------------------------------------------------\n\n"  );
        SCRN( "\n" )

		if( job.params & cleanup )
		{
            remove(string(layout.nameNow + ".m2v").c_str() );
            remove(string(layout.nameNow + ".lpcm").c_str() );
		}
	}

	if( ! valid_audio )
		FATAL( "No audio to process!\n" );

    if  ( fs::exists( job.dvdPath ))   fs_DeleteDir( job.dvdPath);

	xml.write( dvdauthorXml::closeVob );

	if( job.prepare >= mpegf )
	{
		done = mpegf;
		XLOG( INFO_TAG << "dvd" << ( xml.dvdStyler ? "Styler" : "author" )
			<< " xml configuration file:\n" );

		if( multichapter )
		{
			XLOG( LOG_TAG  << "-chapters below are in dvdauthor's rounded fractional format\n" );
			XLOG( LOG_TAG  << "-see layout table(s) above for frame-accurate timestamps\n\n" );
		}
	}
	else
		done = job.prepare;

  	xml.write( dvdauthorXml::close );

	if( job.prepare >= vobf )
	{
		ECHO( "--------------------------------- DVDAUTHOR -----------------------------------\n\n"  );
        SCRN( "\n" )
        SCRN( STAT_TAG "Authoring DVD... " )

		_progress.start = _progress.now = 0;
		_progress.max = 2 * (uint32_t) ( layout.vobEstimate() / MEGABYTE );

		if( execute(
            (binDir / "dvdauthor").string(),
            {"-x", xml.name.c_str() },
            _verbose))
        {
			FATAL( "dvdauthor failed. See lplex.log for details.\n" );
        }
		ECHO( "\n-------------------------------------------------------------------------------\n"  );
		done = vobf;
	}

	ECHO( "\n" );

	if( job.params & md5 )
	{
		INFO( "Input wave-data signatures:\n" );
		for( uint i=0; i < Lfiles.size(); ++i )
			LOG( "md5 : " << hexStr( Lfiles[i].md5str, 16 )
				<< " : " << Lfiles[i].fName.stem() << endl );
		ECHO( "\n" );
		INFO( "Output wave-data signatures:\n" );
		for( uint i=0; i < Lfiles.size(); ++i )
			LOG( "md5 : " << hexStr( Lfiles[i].fmeta.data.stream_info.md5sum, 16 )
				<< " : " << Lfiles[i].fName.stem() << endl );
		ECHO( "\n" );

		if( job.prepare >= vobf )
		{
            SCRN( "\n"  STAT_TAG  "Inserting Lplex tags... \n" )
			//tagEmbed();
		}
	}

	ECHO( "\n"  );

	if( job.params & info && job.prepare >= mpegf )
	{
        SCRN( STAT_TAG  "Copying info files... \n" )

		INFO( "Creating \'XTRA\' info directory\n" );
        fs_MakeDirs( job.dvdPath /  fs::path("XTRA") );

        copyInfoFiles( job.dvdPath / fs::path("XTRA") );
	}

	if( job.prepare >= vobf )
	{
		int *map = NULL;
		if( menufiles.size() && ( map = mapMenus() ) )
		{
            SCRN( STAT_TAG  "Copying custom menu files... \n" )
			copyMenufiles( map );
			delete[] map;
		}
	}

	if( xlogExists )
	{
        // XLOG( INFO_TAG << _f( "Elapsed time=%ld:%02ld\n", stopWatch.Time()/60000, (stopWatch.Time()%60000)/1000 ) );
#if 0
		logCopy( job.prepare >= mpegf ? (
			job.params & info ?
            job.dvdPath / fs::path("XTRA") / fs::path("lplex.log") :
            job.dvdPath.parent_path() / fs::path("lplex.log") ) :
            job.tempPath / fs::path("lplex.log") ) ;
#endif
	}


	if( job.prepare >= isof )
	{
		ECHO( "\n\n\n---------------------------------- MKISOFS ------------------------------------\n\n"  );
		if( mkisofs( job.isoPath, job.dvdPath, job.name ) == 0 )
			done = isof;
	}

	string lgzName;

#ifdef lgzip_support
	if( job.prepare >= lgzf )
	{
        SCRN( "\n" )
        SCRN( STAT_TAG  "Creating dvd container... " )
		lpcmPGextractor dvd( &Lfiles, &infofiles, &job, false, false );
		if( dvd.open( job.isoPath.string(), false ) )
		{
			if( lgzName = udfZip( dvd, false, job.inPath.string() ) )
				done = lgzf;
		}
	}
#endif

    SCRN( "\n\n" )
	stopWatch.pause();

	if( xlogExists )
		logClose();

    SCRN( LOG_TAG  "\n" )
	if( done >= mpegf )
    {
        SCRN( LOG_TAG  "Dvd folder    : ")
        SCRN(TINT( QUOTE( job.dvdPath.string()).c_str()))
        SCRN("\n")
    }
	if( done <= mpegf || ! ( job.params & cleanup ) )
    {
        SCRN( LOG_TAG  "Work folder   : ")
        SCRN(TINT( QUOTE( job.tempPath.string()).c_str()))
        SCRN("\n" )
    }
	if( done >= lgzf )
    {
        SCRN( LOG_TAG  "Dvd container : ")
        SCRN( TINT( QUOTE( lgzName ).c_str() ))
        SCRN( "\n" )
    }
    SCRN( LOG_TAG  "Project file  : ")
    SCRN(TINT( QUOTE( job.projectPath.string() ).c_str() ))
    SCRN( "\n" )
	if( done >= isof )
    {
        SCRN( LOG_TAG  "\n")
        SCRN(LOG_TAG  "Burn '")
        SCRN(TINT( job.isoPath.string().c_str() ))
        SCRN("' with any burning app.\n" )
    }
    else if( job.params & dvdStyler )
    {
        SCRN( LOG_TAG  "\n")
        SCRN( LOG_TAG  "To create menus open '")
        SCRN(TINT( xml.name.c_str() ))
        SCRN("' in dvdStyler.\n" )
    }

    SCRN( "\n" )
	return 0;
}


// ----------------------------------------------------------------------------
//    mkisofs :
// ----------------------------------------------------------------------------
//    Runs mkisofs, creating image <isoPath> from dvd filestructure <dvdPath>
//    using <name> as the disc volume Id.
//
//    Returns mkisofs exit code.
// ----------------------------------------------------------------------------


int mkisofs(const fs::path &isoPath, const fs::path &dvdPath, const string& name )
{
//   ECHO( "\n\n\n---------------------------------- MKISOFS ------------------------------------\n\n"  );
	int exitCode;

    SCRN( STAT_TAG  "Creating iso image... " )

    if ( ! fs::exists( isoPath.parent_path() ) )
    {
        fs_MakeDirs( isoPath.parent_path());
    }

	string volumeID = name;
    if( Right(volumeID, 4) == "_DVD" )
    {
        volumeID = volumeID.substr(0, volumeID.length() - 4);
    }

    volumeID = volumeID.substr(0, 32);

    INFO( "Creating disc image \'" + isoPath.string() + string("\'\n") );
    INFO( "Using volume id \'" + volumeID + string("\'\n\n") );

	if( (exitCode = execute(
            (binDir / "mkisofs").string(),
			{ "-dvd-video",
              "-udf",
			  "-V",
              volumeID.c_str(),
			  "-o",
              isoPath.string().c_str(),
			  dvdPath.string().c_str()
             },
        _verbose))
	)
		ERR( "mkisofs failed. No iso created, see lplex.log for details.\n" );

	return exitCode;

}


// ----------------------------------------------------------------------------
//    unauthor :
// ----------------------------------------------------------------------------
//    Control routine for extracting individual Lpcm audio files out of a
//    dvd format vob file structure.
//
//    Returns 0 on success.
// ----------------------------------------------------------------------------


int unauthor( lpcmPGextractor &dvd )
{
	int c, context, titleset = 0, v, processErr, writeIndex = 0, finish;
	uint16_t s;
	uint32_t b, blockCt, unfinishedBlock=0;
	lpcmFile *lFile;
	lpcmWriter *writer = nullptr;
	dvd_file_t *vobs = nullptr;
	//md5_byte_t md5str[16];
	uint64_t ptsBoundary;
	counter<uint64_t> total;

#ifndef lgzip_support
	if( job.format == lgzf )
		udfError( "Try setting '--formatout' to 'flac' or 'wave'." );
#endif

	POST( "\n" );
	INFOv( "Extracting    : [ "
		<< ( job.format == wavef ? "wave" : job.format == flacf ? "flac" : "raw" )
		<< ( job.format == flacf ? _f( "%d", job.flacLevel ).c_str() : "" )
		<< ( job.params & md5 ? " md5aware" : "" )
		<< ( job.params & restore ? " restore" : "" )
		<< ( job.params & info ? " infofiles" : "" ) << " ]\n" );
    SCRN( INFO_TAG  "Output folder : \'")
    SCRN(TINT( job.extractPath.string() ))
    SCRN("\'\n" )
	INFO( "Output folder : \'" << job.extractPath.string() << "\'\n" );

	editing = false;

    if ( ! editing && ! fs::exists( job.extractPath.string() ) )
    {
        fs_MakeDirs( job.extractPath );
    }

	stopWatch.Start();
	dvd.traverse();

	/* TODO (#1#): check extraction free space */

	while( (context = dvd.getCell() ))
	{
		c = dvd.pgcCell;

		lFile = &Lfiles.at( dvd.audioCells[c].xIndex );

		if( titleset != dvd.titleset )
		{
			unfinishedBlock = 0;
			if( titleset )
                SCRN( "\n" )
			titleset = dvd.titleset;

			vobs = DVDOpenFile( dvd.libdvdReader, dvd.titleset, DVD_READ_TITLE_VOBS );
			if( ! vobs )
				FATAL( "Unable to open DVD." );
		}

		if( c == 0 )
			writer = dvd.getWriter( writeIndex = c, context );

		else if( ! ( writer->state & lpcmEntity::metered ) || editing ||
				( writer->state & lpcmEntity::metered && writer->ct.max &&
				( writer->ct.now == writer->ct.max ) ) )
			writer = dvd.getWriter( writeIndex = c, context );

if( lFile->edit & lpcmEntity::skip )
{
			continue;
}

if( job.skip && ( c < job.skip ) )
{

continue;
}

		if( editing )
			continue;

		finish = processErr = false;
		total.start = total.now = 0;
		total.max = job.params & md5 ?
			flacHeader::bytesUncompressed( &lFile->fmeta ) : 0;
		ptsBoundary = dvd.audioCells[c].start_pts;

		for( b = ( unfinishedBlock &&
						unfinishedBlock < dvd.audioCells[c].start_sector ) ?
						unfinishedBlock : dvd.audioCells[c].start_sector;
				b <= dvd.audioCells[c].last_sector;
				b += BIGBLOCKCT )
		{
			blockCt = dvd.audioCells[c].last_sector + 1 - b;
			if( blockCt > BIGBLOCKCT )
				blockCt = BIGBLOCKCT;
			if( b + blockCt == dvd.audioCells[c].last_sector + 1 )
			{
				context = _endIt;
				ptsBoundary = dvd.nextAudioCellPts( c );
			}

			if( DVDReadBlocks( vobs, b, blockCt, bigBlock ) < blockCt )
				FATAL( "[unauthor] Error reading DVD data." );

			uint64_t unsent;
			byteRange audio;
			PES_packet::header *PS1;
			PES_packet::LPCM_header *LPCM;
			uint8_t mode = ( job.format == lpcmf ? 0 : PES_packet::swap ) | PES_packet::adopt;

			for ( s=0; s < blockCt; s++ )
			{
				if(( PS1 = (PES_packet::header*) &bigBlock[ s * DVD_VIDEO_LB_LEN + 14 ] )
						->startCode != bEndian( (uint32_t) 0x000001BD) ||
						( LPCM = PES_packet::lpcmAddr( PS1 ) )->streamID != dvd.lpcm_id )
					continue;

				switch ( context )
				{
					case _isOpen:
						break;

					case _isNew:
						if( ! ( writer->state & lpcmEntity::specified ) )
						{
							if( writer->preset( LPCM ) )
								writer->open();
							else
							{
								finish = processErr = true;
								break;
							}
						}
						mode |= PES_packet::start;
						break;

					case _endIt:
						if( s == ( blockCt - 1 ) )
						{
							mode |= PES_packet::end;
							finish = true;
						}
						break;

					case _reopen:
						STAT( "Continuing " << writer->fName.stem() << endl );
						break;
				}

				if( context != _endIt && finish )
					break;

				mode = PES_packet::payload( PS1, &audio, mode, ptsBoundary );

				if( mode & PES_packet::end )
				{
					unfinishedBlock = ( mode & PES_packet::unfinished ?  b + s : 0 );
					finish = true;
				}

				if( total.max && ( unsent = total.max - total.now ) < audio.len )
				{
					audio.len = unsent;
					finish = true;
				}

				writer->process( &audio );

				total.now += audio.len;

				if( ( unsent = writer->unsent ) &&
						( writer = dvd.getWriter( writeIndex + 1, _isNew ) ) )
				{
					writeIndex++;
					audio.start += ( audio.len - unsent );
					audio.len = unsent;
					writer->process( &audio );
				}

				context = _isOpen;
				if( finish )
					break;
			}

			if( finish )
				break;
		}

		if( processErr )
		{
            SCRN( "\r" )
		}
		else if( ! ( writer->state & lpcmEntity::metered ) )
		{
			LOG( writer->ct.now << " audio bytes\n" );
		}

	}
    SCRN( "\n" )

	v = _verbose;
	_verbose = true;

	ECHO( "\n" );
	for( uint i=0; i < Lfiles.size(); ++i )
	{
		if( Lfiles[i].writer->isOpen() )
			Lfiles[i].writer->close();
		if( Lfiles[i].writer->state & lpcmEntity::specified )
			Lfiles[i].writer->md5Report();
	}
	ECHO( "\n" );

	_verbose = v;

	if( job.params & info && infofiles.size() )
	{
        SCRN( STAT_TAG  "Copying info files... \n" )
#ifdef dvdread_udflist
//      if( job.media & imagefile )
//         udfCopyInfoFiles( job.extractPath.string() );
		if( dvd.isImage )
			dvd.udfCopyInfoFiles();
		else
#endif
            copyInfoFiles( job.extractPath);
	}

#ifdef lgzip_support
	if( job.format >= lgzf )
		udfZip( dvd, false, job.extractPath.string() );
#endif

    SCRN( "\n" )
	stopWatch.pause();

	if( xlogExists )
	{
        logCopy( job.extractPath / fs::path("lplex.log") );
		logClose();
	}

    SCRN( "\n" )
	ECHO( "\n\n" );

	return 0;
}



// ----------------------------------------------------------------------------
//    copyInfoFiles :
// ----------------------------------------------------------------------------
//    Copies info files to folder <rootPath>.
// ----------------------------------------------------------------------------


void copyInfoFiles(const fs::path& rootPath )
{
	uint16_t i;
    fs::path subPath;

	if( ! infofiles.size() )
		return;

	for( i=0; i < infofiles.size(); i++)
	{
		if( infofiles[i].reject )
			continue;

        fs::path i_fName = fs::path(infofiles[i].fName);
        string comp =  i_fName.filename().string();

        if (toUpper(comp) ==  "LPLEX.LOG" )
			continue;

        subPath = fs::path(infofiles[i].fName.substr( infofiles[i].root ));

		INFO( "-copying \'" << subPath << "\'\n" );

        fs_MakeDirs( ( rootPath / subPath).parent_path() );

        fs::copy_file(i_fName, rootPath / subPath / i_fName);
	}
	ECHO( "\n" );
}




// ----------------------------------------------------------------------------
//    writeUserData :
// ----------------------------------------------------------------------------
//    Writes corresponding flac header and Lplex xml entity to given buffer.
//
//    Arguments:
//       <lFile>        - pointer to relevant lpcmEntity descriptor
//       <userData>     - pointer to destination buffer
//
//    Returns actual number of bytes written
// ----------------------------------------------------------------------------


uint16_t writeUserData( lpcmEntity *lFile, uint8_t *userData )
{
    fs::path noExt =  lFile->fName;
    noExt = noExt.parent_path() / noExt.stem();


	string tag = _f( "<Lplex ver=\"%s\" path=\"%s\" id=\"%d\" shift=\"%d\" md5prev=\"%s\"/>",
		VERSION,
        noExt.string().substr( lFile->root ).c_str(),
		lFile->index,
		-lFile->trim.offset,
        hexToStr( lFile->md5str, 16, 1 ).c_str() );

	flacHeader::write( userData, &lFile->fmeta );
	const char *buf = tag.c_str(); // also char_str()
#ifdef wxUTF8
	int len = tag.c_str().length();
#else
	int len = tag.length();
#endif
	memcpy( userData + 42, buf, len );

	return len + 42;
}




// ----------------------------------------------------------------------------
//    readUserData :
// ----------------------------------------------------------------------------
//    Attempts to read and transfer flac header and Lplex xml entity info from
//    given buffer into given lpcmEntity descriptor.
//
//    Arguments:
//       <lFile>        - pointer to relevant lpcmEntity descriptor
//       <userData>     - pointer to source buffer
//
//    Returns 0 on failure, 1 on success
// ----------------------------------------------------------------------------


uint16_t readUserData( lpcmEntity *lFile, uint8_t* userData )
{
	if( userData[0] == 'f' &&
			userData[1] == 'L' &&
			userData[2] == 'a' &&
			userData[3] == 'C' )
	{
		flacHeader::readStreamInfo( userData + 8, &lFile->fmeta );
		lFile->trim.len = flacHeader::bytesUncompressed( &lFile->fmeta );

		if( userData[42] == '<' &&
				userData[43] == 'L' &&
				userData[44] == 'p' &&
				userData[45] == 'l' &&
				userData[46] == 'e' &&
				userData[47] == 'x' )
		{
			xmlAttr attr( (char*) userData + 48 );

			while( attr.get() )
			{
				if( ! stricmp( attr.name, "ver" ) )
				{
				}

				else if( ! stricmp( attr.name, "path" ) )
				{
					fs_fixSeparators( attr.val );
					lFile->fName = attr.val;
					lFile->root = 0;
					lFile->state |= lpcmEntity::named;
				}

				else if( ! stricmp( attr.name, "id" ) )
					lFile->index = atoi( attr.val );

				else if( ! stricmp( attr.name, "shift" ) )
					lFile->trim.offset = atol( attr.val );

				else if( ! stricmp( attr.name, "md5prev" ) )
					strtomd5( (md5_byte_t*)&lFile->md5str, attr.val );

				else
					WARN( "[readUserData] Urecognized attribute \'" << attr.name
						<< "\'. Check if this (" << LPLEX_VERSION_STRING
						<< ") is the latest version of Lplex\n" );
			}
		}
		return lpcmEntity::soundCheck( lFile );
	}

	return 0;
}




// ----------------------------------------------------------------------------
//    tagEmbed :
// ----------------------------------------------------------------------------
//    Writes corresponding userdata into first Nav Pack of each multiplexed
//    track at PCI::RECI* field (byte offset 0x343 from start of first block).
//
//    * This field was originally intended for "Recording Information
//    (royalty management)", but there are reportedly no examples of it ever
//    having actually been used.
//
//    Returns 0 on success.
// ----------------------------------------------------------------------------


int tagEmbed()
{
	int c, v=0, L=-1, titleset=-1;
	uint64_t addr, _SOF = 0, _EOF = 0;
	uint16_t uDataLen;
	uint8_t userData[512];

	fstream vobFile;
    fs::path vob = job.dvdPath / "VIDEO_TS";

    lpcmPGtraverser dvd( vob.string().c_str(), false );

	while( dvd.getCell() )
	{
							//get the cell index...
		c = dvd.cellIndex( dvd.cell->vob_id, dvd.cell->cell_id );

							//check if in new titleset...
		if( titleset != dvd.titleset )
		{
			titleset = dvd.titleset;
			_SOF = _EOF = v = 0;
		}

#if 0
							//locate the matching lFile...
		L = -1;
		for( i=0; i < Lfiles.size(); i++ )
		{
			if( Lfiles[i].group == titleset - 1 )
				if( ++L == c )
				{
					L = i;
					break;
				}
		}
#endif

// note: see also DVDFileStat() in dvd_reader.h


							//get relevant start sector
		addr = (uint64_t)dvd.cells[c].start_sector * DVD_VIDEO_LB_LEN + 0x343;

							//if seeking beyond this file...
		while( addr > _EOF )
		{
			if( vobFile.is_open() )
				vobFile.close();

							//...open next vob file
			vobFile.open(
                (vob / fs::path(_f( "VTS_%02i_%i.VOB", titleset, ++v ))).string(),
				ios::binary | ios::in | ios::out );
			if( ! vobFile )
            {
                ERR( "Could not open ")
                ERR((vob / fs::path(_f( "VTS_%02i_%i.VOB\n", titleset, v ) )).string())
            }
							//...and update absolute seek limits
			_SOF = _EOF;
			vobFile.seekp( 0, ios::end );
			_EOF += vobFile.tellp();
		}
		addr -= _SOF;

							//go to relative file offset PCI::RECI & insert flac header

		vobFile.seekp( addr, ios::beg );

		uDataLen = writeUserData( &Lfiles[L++], userData );

		if( uDataLen <= 0xBD ) // 0x400 - 0x343
		{
			INFO( "(" <<  dvd.nameNow << ") Inserting Lplex tags at offset 0x343 of LB "
				<< dvd.cells[c].start_sector << "\n" );
			vobFile.write( (char *)userData, uDataLen );
		}
	}

	return 0;
}



counter<uint32_t> _progress;

// ----------------------------------------------------------------------------
//    mplexProgress :
// ----------------------------------------------------------------------------
//    Handler to trap mplex counter patch* messages of the form:
//       "  PATCH: Frame    192"
//
//    Arguments:
//       <msg>    - intercepted stderr message
//       <echo>   - whether to write to stderr
//
//    Returns whether message was trapped
// ----------------------------------------------------------------------------
// (*) see '../patch' folder.

int mplexProgress( const char *msg, bool echo )
{
	uint16_t perThousand;

	if(   msg[2] == 'P'
		&& msg[3] == 'A'
		&& msg[4] == 'T' )
	{
		if( echo )
		{
			perThousand = atol( msg + 15 ) * 1000 / _progress.max;
			if( _verbose )
                cerr << TINT_BLIP( _f( "  %2d.%d%% done", perThousand / 10, perThousand % 10 ).c_str() ) << flush;
			else
				blip( _f( "%2d.%d%%", perThousand / 10, perThousand % 10 ) );
		}
		return 1;
	}
	return 0;
}



// ----------------------------------------------------------------------------
//    mkisofsProgress :
// ----------------------------------------------------------------------------
//    Handler to trap mkisofs messages of the form
//       "98.58% done, estimate finish Wed Jun  7 13:54:43 2006"
//
//    Arguments:
//       <msg>    - intercepted stderr message
//       <echo>   - whether to write to stderr
//
//    Returns whether message was trapped
// ----------------------------------------------------------------------------


int mkisofsProgress( const char *msg, bool echo )
{
	char pct[8];

	if(   msg[6] == '\%'
		&& msg[7] == ' '
		&& msg[8] == 'd' )
	{
		if( echo )
		{
			memcpy( pct, msg + 1, 7 );
			pct[4] = '\%';
			pct[5] = '\0';
			if( ! _verbose )
				blip( pct );
		}
		return 1;
	}
	return 0;
}


// ----------------------------------------------------------------------------
//    mpeg2encProgress :
// ----------------------------------------------------------------------------
//    Handler to trap mpeg2enc messages of the form (mjpegtools >= 1.9.x)
//       "   INFO: [mpeg2enc] Enc1     40    40( 4) P q=9.00     [0% Intra]"
//    or  (mjpegtools < 1.9.x)
//       "INFO: [mpeg2enc] Frame end 25     12.00 1024.00 8.0 208.00"
//
//    Arguments:
//       <msg>    - intercepted stderr message
//       <echo>   - whether to write to stderr
//
//    Returns whether message was trapped
// ----------------------------------------------------------------------------

int _tag0 = 0;

int mpeg2encProgress( const char *msg, bool echo )
{
	uint16_t pct;
	char *pos = NULL;
	if(   msg[1] == '*'
		&& msg[2] == 'E'
		&& msg[3] == 'R' )
	{
		logReopen();
	}
											// do once: find ']' pos for this variant of mpeg2enc
	if( ! _tag0 )
	{
		for( int i=0; msg[i]; i++ )
			if( msg[i] == ']' )
				pos = (char*)msg + i - 2;

		if( ! pos )
			return 0;
	}
	else
		pos = (char*)msg + _tag0/*  - 2 */;

											// examine msg relative to ']' pos
	if(      pos[0] != 'n'
			|| pos[1] != 'c'
			|| pos[2] != ']' )
											// ...skip if it's a jpeg2yuv msg
		return 0;
											// do once: save ']' pos for future reference
	if( ! _tag0 )
			_tag0 = pos - msg;
											// mjpegtools >= 1.9.x
	if(      pos[4]  == 'E'
		&&    pos[5]  == 'n'
		&&    pos[6]  == 'c' ) pos += 9;
											// mjpegtools < 1.9.x
	else if( pos[10] == 'e'
		&&    pos[11] == 'n'
		&&    pos[12] == 'd' ) pos += 14;

	else
		return 0;
											// success
	if( echo )
	{
		pct = atoi( pos ) * 100 / _progress.max;
		if( _verbose )
			blip( _f( "%s%2d%% done", STAT_TAG, pct ) );
		else
			blip( _f( "%2d%%", pct ) );
	}

	return 1;
}



// ----------------------------------------------------------------------------
//    dvdauthorProgress :
// ----------------------------------------------------------------------------
//    Handler to trap dvdauthor messages of the form
//       "STAT: VOBU 91 at 31MB, 1 PGCS" and
//       "STAT: fixing VOBU at 26MB (81/91, 87%)"
//
//    Arguments:
//       <msg>    - intercepted stderr message
//       <echo>   - whether to write to stderr
//
//    Returns whether message was trapped
// ----------------------------------------------------------------------------


int dvdauthorProgress( const char *msg, bool echo )
{
	uint16_t pct = 0;
	static uint16_t pass = 1;

	if(   msg[4] == ':'
		&& msg[5] == ' '
		&& msg[6] == 'V'
		&& msg[7] == 'O'
		&& msg[8] == 'B'
		&& msg[9] == 'U' )
	{
		if( pass != 0)
		{
			_progress.start += _progress.now;
			_progress.now = pass = 0;
		}

		if( char *at = (char*)strstr( msg, "at" ) )
		{
			_progress.now = atoi( at + 3 ) * 16 / 10;
			pct = true;
		}
	}
	else
	if(   msg[4] == ':'
		&& msg[5] == ' '
		&& msg[6] == 'f'
		&& msg[7] == 'i'
		&& msg[8] == 'x'
		&& msg[9] == 'i' )
	{
		if( pass == 0)
		{
			_progress.start += _progress.now;
			_progress.now = 0;
			pass = 1;
		}
		_progress.now = atoi( msg + 21 ) * 4 / 10;
		pct = true;
	}

	if( pct )
	{
		if( echo && ! _verbose )
		{
			pct = ( _progress.start + _progress.now ) * 1000 / _progress.max;
//         blip( _f( "%2d%%", pct ) );
			blip( _f( "%2d.%d%%", pct / 10, pct % 10 ) );
		}
		return 1;
	}

	return 0;
}



// ----------------------------------------------------------------------------
//    addMenus :
// ----------------------------------------------------------------------------
//    Searches for tv-system-compatible user-generated menu resources (i.e.
//    VIDEO_TS.IFO/VOB, VTS_XX_0.IFO/VOB, etc) to include in the project
//    VIDEO_TS folder.
//
//    Arguments:
//       <filename>     - folder or filename to search
//       <tv>           - NTSC or PAL enumeration
//       <force>        - whether to add if incompatible
//
//    Returns 1 on success.
// ----------------------------------------------------------------------------


int addMenus( const char * filename, int tv, bool force )
{
	if( menufiles.size() )
		return 0;

    //wxDir video_ts;
    fs::path fName( filename );
	userMenus.verbose = false;

    if( fName.is_relative() )
         fName = fs::absolute(fName);

	STAT( _f( "Looking for%s menu resources in \'%s\'...\n",
		force ? "" : tv==NTSC ? " NTSC" : " PAL", fName.string().c_str() ) );

    if( ! fs::exists(fName))
	{
        if( fs::exists(fName))
            fName = fs::path( fName.parent_path() );
	}

    //string menuSpec( fName.string() );
#if 0
	class menuTraverser : public wxDirTraverser
	{
	public:
		enum { found=1, incompatible };

		lpcm_video_ts *menus;
		int state, tv;
		bool force;
		string dir;

		menuTraverser( lpcm_video_ts *v, int tvsys, const char *topdir, bool isforced )
				: menus(v), state(0), tv(tvsys), force(isforced)
			{ OnDir( topdir ); }

		virtual wxDirTraverseResult OnFile(const string& fName)
		{
			if( state & found && fName.Contains( dir ) )
			{
				menufiles.push_back( fName );
				LOG( _affirm << fName << "\n" );
			}
			return wxDIR_CONTINUE;
		}
		virtual wxDirTraverseResult OnDir(const string& dirname )
		{
			if( state & found )
				return wxDIR_IGNORE;
			if( menus->open( dirname, false ) )
			{
				if( menus->menuFmt == tv || force )
				{
					state |= found;
					if( ! ( job.params & customized ) )
                        SCRN( STAT_TAG  "Checking custom " )
                    SCRN( "menus... " )
					STAT( _f("Checking custom %s resources in '%s'\n",
						menus->menuFmt==NTSC ? "NTSC" : "PAL", dirname.c_str() ) );
				}
				else
				{
					state |= incompatible;
					LOG( _f("Incompatible (%s) menus in '%s'.\n",
						menus->menuFmt==NTSC ? "NTSC" : "PAL", dirname.c_str() ) );
				}
				dir = dirname;
			}
			return wxDIR_CONTINUE;
		}
	};

	int state = 0;

	if( video_ts.Open( fName.string() ) )
	{
		menuTraverser menuFinder( &userMenus, tv, fName.string(), force );
		video_ts.Traverse( menuFinder );
		state = menuFinder.state;
		if( state == menuTraverser::incompatible )
		{
            SCRN( "\n" )
			_verbose = true;

			ERR( _f( "Incompatible (%s) menus. You can either\n", menuFinder.menus->menuFmt == NTSC ? "NTSC" : "PAL"  ) );
			LOG( _f( "-omit the setting '--menu %s' to create a menuless dvd.\n", filename ) );
			LOG( _f( "-if you have a compatible dvd player, set '--video=%s' to allow\n",
				menuFinder.menus->menuFmt == NTSC ? "ntsc" : "pal" ) );
			LOG( _f( "   use of resources in '%s'.\n", menuFinder.dir.c_str() ) );
			LOG( _f( "-override this test using '--menuforce %s'\n\n", filename ) );

			exit( -1 );
		}
	}

	if( state == 0 )
	{
        SCRN( "\n" )
		_verbose = true;
		ERR( "No custom menu resources found. You can\n" );
		LOG( _f( "-omit the setting '--menu %s' to create a menuless dvd.\n\n", filename ) );
		exit( -1 );
	}

	if( menufiles.size() )
	{
		menuPath = menuSpec;
		job.params |= customized;
		return 1;
	}
#endif
	return 0;
}



// ----------------------------------------------------------------------------
//    copyMenufiles :
// ----------------------------------------------------------------------------
//    Copies user-generated menu resources into the Lplex VIDEO_TS folder and
//    changes Lplex's VTS_XX_ numbers if required according to the <map>.
// ----------------------------------------------------------------------------


void copyMenufiles( int *map )
{
	if( ! menufiles.size() )
		return;

	vector<string> xobs;
    fs::path VIDEO_TS = job.dvdPath / "VIDEO_TS";
    fs::path BUP = job.dvdPath / "XTRA" / "BUP" ;
    fs_MakeDirs( fs::path(BUP) );
    fs::path f;
    for(auto& p: fs::directory_iterator(VIDEO_TS))
    {
        f = p.path();
        if (f.empty()) break;
        string fName = Right(f.string(), 12);

        if( Right(fName, 4) == ".VOB" && fName.Left(4) == "VTS_" && fName[7] != '0' )
		{
            fName = fName.substr(0, 8);
            int titleset = atoi( Right(fName, 4).c_str() );

			if( map[titleset] != titleset )
			{
				LOG( _f( "-remapping title %d to VTS_%02d\n" , titleset, map[titleset] ) );
                fName.replace(4, 2, to_string(map[titleset]));
			}

            fs::rename(f, VIDEO_TS / (fName + ".XOB") );
			xobs.push_back( fName );
		}

        else if( Right(fName, 4) != ".XOB" )
            fs::rename(f, BUP / fName );

	}

	for( uint i=0; i < xobs.size(); ++i )
        fs::rename(VIDEO_TS / (xobs[i] + ".XOB"), VIDEO_TS / (xobs[i] + ".VOB") );

	for( uint i=0; i < menufiles.size(); ++i )
	{
		bool skip = false;
		for( uint j=0; j < xobs.size(); ++j )
		{
            if( Right(menufiles[i],12).Left(8) == xobs[j] )
			{
				WARN( "-filename conflict. Can't copy \'" << menufiles[i] << "\'\n" );
				skip = true;
				break;
			}
		}

		if( ! skip )
			INFO( "-copying \'" << menufiles[i] << "\'\n" );
            fs::copy_file(menufiles[i], VIDEO_TS / fs::path( menufiles[i] ).filename() );
	}
	ECHO( "\n" );
}



// ----------------------------------------------------------------------------
//    mapMenus :
// ----------------------------------------------------------------------------
//    Attempts to find the corresponding user-generated VTS_XX_0.IFO/VOB that
//    matches the native cell address table for each Lplex-generated titleset.
//
//    Returns a cross-index of lplex to user titleset numbers.
// ----------------------------------------------------------------------------


int* mapMenus()
{
	vector<string> xobs;
	if( ! menufiles.size() )
		return NULL;

	int err = 0;
    const char* VIDEO_TS = (job.dvdPath / "VIDEO_TS").string().c_str();
	lpcm_video_ts lplexMenus( VIDEO_TS, false );
	int *map = new int[lplexMenus.numTitlesets+1];


	do
	{
		bool found = false;
		do
		{
			if( lplexMenus.numCells == userMenus.numCells
					&& memcmp( lplexMenus.cells, userMenus.cells, userMenus.numCells * sizeof(cell_adr_t) ) == 0 )
			{
				INFO( _f( "Custom VTS_%02d matches title %d\n" ,
					userMenus.titleset, lplexMenus.titleset ) );
				map[lplexMenus.titleset] = userMenus.titleset;
				found = true;
				break;
			}
		} while( userMenus.getAudioStream() );

		if( ! found )
		{
			err = 1;
			map[lplexMenus.titleset] = lplexMenus.titleset;
			WARNv( _f( "No match for title %d in custom menu resources.\n",
				lplexMenus.titleset ) );
		}
		userMenus.titleset = 0;
		userMenus.numAudioStreams = userMenus.numCells = userMenus.audioStream = -1;
		userMenus.ifo = ifoOpen( userMenus.libdvdReader, 0 );
		userMenus.getAudioStream();
	} while( lplexMenus.getAudioStream() );

	if( err && ! menuForce )
	{
        SCRN( "\n" )
		_verbose = true;

		ERR( "Incompatible menu resources. You can either\n" );
		LOG( "-omit the setting '--menu <filepath>' to create a menuless dvd.\n" );
		LOG( "-override this test using '--menuforce <filepath>'\n\n" );

		exit( -1 );
	}

	return map;
}



