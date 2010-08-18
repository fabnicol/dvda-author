dvda-author  version 10.06 (June 2010)
===========================


1. DESCRIPTION
--------------


dvda-author creates high-definition DVD-Audio disc structures from either WAV,
FLAC/Ogg FLAC, or SoX-14.3.1-supported audio formats (.au, .aiff, .gsm, etc.)

This version supports multichannel audio up to 5.1.

Switch -x enables extraction of audio content from DVD-Audio structures into .wav files.

It can also create "hybrid" or "universal" DVD-Audio/Video disc structures,
which contain both DVD-Audio and DVD-Video zones.

A navigation feature makes it possible to start playing a video
title from within audio zone or an audio title from within video zone.

Wav header correction, 'gapless' track-to-track playback
and DVD-Audio menus are the latest features to date.

See man page, html page or website for details.


2. WEBSITE
-----------

The latest version of the software is available at the following internet
address:

  http://dvd-audio.sourceforge.net/


3. BUILD ENVIRONMENT
--------------------

A GNU-style make system is shipped with this package.
Successful builds were obtained on the following platforms:

  - Cygwin/Windows XP
  - FreeBSD 7.0
  - Linux 2.6.24 (Ubuntu 8.04)
  - MSYS 1-11

Details on building issues are given in INSTALL.


4. FILES
--------

Source code directories are:

  src
      Source code for dvda-author:
	amg2.c                 creates AUDIO_TS.IFO
	asvs.c                 creates AUDIO_SV.IFO
	ats.c                  creates AOB files
	atsi2.c                creates ATS_XX.0.IFO files
	audio.c                core audio processing
	auxiliary.c            help and auxiliary functions
	command_line_parsing.c command-line parser
	dvda-author2.c         launches configuration file then command-line parsing
	file_input_parsing.c   parses audio input directories or disc (when extracting)
	include                header files
	launch_manager.c       sequentially launches sub-processes
	lexer.c                simple lexer for configuration files
	libsoxconvert.c        SoX launcher
	menu.c                 creates DVD-Audio compliant menus for (patched) dvdauthor
	samg2.c                creates AUDIO_PP.IFO
	videoimport.c          imports VIDEO_TS directory and computes video links.
	xml.c                  creates dvdauthor Xml projects.

  libutils
      Source code for the libc_utils static library

  libats2wav
      Source code for ats2wav, a DVD-Audio titleset extracter that converts .AOB
      files into .wav audio files.

  libfixwav
      Source code for a special library that checks and fixes .wav file headers

  images
      NSIS compiler images for Windows installers.

  libiberty
      GNU libc code for function replacement (alternative to -liberty).

  m4, m4.extra, m4.extra.dvdauthor
      M4 macros, especially:

	dvda.m4                general-purpose macros that could be reused in other projects.
	auxiliary.m4           shorter general-purpose macros.
	dependencies.m4        dvda-author-specific parameters submitted to other M4 macros (using lists).
	sox-debug.m4           used to correct a bug in SoX code.
        oggflac-test.m4        tests for Ogg FLAC support
	iconv.m4, codeset.m4   third-party macros copied here as some platforms may not include it
			       and they are used by nested configure scripts.


Important DATA-type files are:

  dvda-author.1
      Man(1) page
  dvda-author.html
      HTML version of the man page
  dvda-author.desktop
      KDE/gnome desktop configuration file
  dvda-author.conf
      configuration file (default command-line options)
  dvda-author.png or .ico
      images for desktop.
  dvda-author.nsi
      NSIS script for generating Windows installer.
  CB_project
      Code::Blocks Xml project files
  BUGS
  LIMITATIONS
  HOWTO.conf
      Help file on how to generate a configuration file and use it
  dvda-author.conf.example
      Example of a complex configuration file project



5. INSTALLATION
---------------

See file INSTALL included in this distribution.
On *nix platforms, the GNU build system will automatically link to existing libFLAC
libraries, checking for the appropriate version (1.2.1 and later).
If your installed library is not compatible, or on Windows platforms, libFLAC will be
rebuilt from source code in directory libFLAC/

If you do not use the GNU build system, check that the configuration file dvda-author.conf
is in the same directory as the main executable (see INSTALL for other options).
By default, binaries will install under /usr/bin.

This version will use either libiberty.a if already installed on your platform
or source code for a few GNU libc functions located in libiberty/ The list of functions
used depends on platforms.


6. CREDITS
----------

Thanks to:

  Bahman Negahban for useful hints on FLAC 1.2.1 integration
  Tomasz Belinea  for testing version 08.07.
  Lee Feldkamp for testing Windows executables and adding audio options (-j,-s)
  Lee and Tim Feldkamp for authoring multichannel support.

and all other authors (see file AUTHORS for details).

7. NEWS
-------

See file NEWS

8. MAILING LIST
---------------

  dvd-audio-devel@lists.sourceforge.net


