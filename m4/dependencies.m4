
# File dependencies.m4

# Application-specific data for extended M4, M4sh and autoconf macros in dvda.m4
# ==============================================================================
# File is under copyright by Fabrice Nicol, 2009-2013

# To add new tests, just add new data abiding by the data structure, especially M4 quotes


AC_DEFUN([DVDA_CHECK_DEPENDENCIES],
[

# ====== Auxiliary tools: man page and html doc, mjpegtools, ImageMagick ========= #

#mkisofs and cdrecord are tested by DVDA_DOWNLOAD

m4_map([DVDA_TEST_AUX],[
	[[help2man],  [man page will be generated after install: help2man]],
	[[man2html],  [man page will be generated after install: man2html]],
	[[curl],      [patched version of prerequisites will be downloaded: curl]],
	[[patch],     [patch tool]],
	[[jpeg2yuv],  [background pictures can be converted to background videos: jpeg2yuv]],
	[[mplex],     [menus can be muxed: mplex]],
	[[mp2enc],    [mp2 soundtracks can be created: mp2enc]],
	[[mpeg2enc],  [mpeg2 streams can be encoded: mpeg2enc]],
	[[spumux],    [menu titles can be muxed: spumux]],
	[[mogrify],   [automatic menus can be generated: mogrify]],
	[[convert],   [automatic menus can be generated: convert]],
	[[mkisofs],   [ISO authoring software: mkisofs]],
	[[cdrecord],  [recording software: cdrecord]],
	[[md5sum],    [MD5 checksum utility]],
	[[autoconf],  [configure system build: autoconf]],
	[[automake],  [make system build: automake]],
	[[lplex],     [using lplex to mux lpcm audio and video]],
	[[make],      [whether make is installed]],
	[[mpeg2dec],  [whether mpeg2dec is installed]],
	[[git],       [whether git can be used for archiving purposes]],
	[[a52dec],    [whether a52dec is installed]]])


  m4_define([SOX_STATIC_MSG],
    [Using static library build only. You may need to install the following libraries:
      libasound libpng libz libltdl libmagic libsamplerate, using "-dev" packages.
    Warning: Availability will not be tested. An error message 'Cannot find -l...' at the
    end of compiling stage will indicate that you need to install the corresponding library.])

  m4_define([IMAGEMAGICK_MSG],
    [With GNU/Linux you will need to run sudo ldconfig after make install.])


  #===================== build features ==============================================#
  #=================  platform-specific features in AS_CASE =====================================#

#all _BUILD prepended items are lower case, including for FLAC-->flac

 AS_CASE([${build}],
   [*-*-mingw32*],
   [
    m4_map([DVDA_ARG_ENABLE],
	   [
	     [[libogg-build]],
	     [[libiberty-build]],
	     [[libfixwav-build]],
	     [[cdrtools-build]],
	     [[flac-build],[ libogg_BUILD=yes]],
	     [[sox-build],[
	       flac_BUILD=yes
	       libogg_BUILD=yes
	       ]],
	     [[help2man-build]],
	     [[man2html-build],[help2man_BUILD=yes]],
	     [[libmpeg2-build]],
    	     [[lplex-build],[ 
    	      libogg_BUILD=yes
    	      flac_BUILD=yes]],
	     [[a52dec-build]],
	     [[mjpegtools-build]],
	     [[core-build],
	      [withval_libfixwav=no
	       withval_FLAC=no
	       withval_libogg=no
	       withval_sox=no
	       withval_libiberty=yes
	       MAYBE_libiberty=$ROOTDIR/libiberty
	       libiberty_BUILD=yes
	       libiberty_BUILD=no
	       libogg_BUILD=no
	       libogg_DOWNLOAD=no
	       libiberty_BUILD=yes
	       MAYBE_libogg=""
	       MAYBE_libfixwav=""
	       libfixwav_BUILD=no
	       libogg_LINK=""]],
	     [[minimal-build],
	      [WITH_libfixwav=yes
	       WITH_FLAC=yes
	       WITH_libogg=yes
	       WITH_sox=yes
	       libogg_BUILD=yes
	       flac_BUILD=yes
	       libfixwav_BUILD=yes
	       sox_BUILD=yes]],
	     [[ImageMagick-build],[DVDA_INF([IMAGEMAGICK_MSG])]],
	     [[static-sox],
	      [DVDA_INF([SOX_STATIC_MSG])
	       sox_LINK="$sox_LINK -lasound -lpng -lz -lltdl -lmagic -lsamplerate"
	       sox_LIB="/usr/lib/libsox.a $(find /usr/lib/sox/ -regex lib.*a)"]],
	     [[all-all],
		[all_builds=yes]],
	     [[all-builds]]])
   ],
   [

     m4_map([DVDA_ARG_ENABLE],
	   [
	     [[libogg-build]],
	     [[libiberty-build]],
	     [[libfixwav-build]],
	     [[cdrtools-build]],
	     [[flac-build],[
	      libogg_BUILD=yes
	      ]],
	     [[sox-build],[
	       flac_BUILD=yes
	       libogg_BUILD=yes]],
	     [[help2man-build]],
	     [[man2html-build],[help2man_BUILD=yes]],
	     [[libmpeg2-build]],
    	     [[lplex-build],[ 
    	      libogg_BUILD=yes
    	      flac_BUILD=yes]],
	     [[dvdauthor-build]],
	     [[a52dec-build]],
	     [[mjpegtools-build]],
	     [[core-build],
	      [WITH_libfixwav=no
	       MAYBE_libfixwav=""
	       WITH_libiberty=no
	       MAYBE_libiberty=""
	       libiberty_BUILD=no
	       libfixwav_BUILD=no
	       withval_FLAC=no
	       withval_libogg=no
	       withval_sox=no
	       libogg_BUILD=no
	       libogg_DOWNLOAD=no
	       MAYBE_libogg=""
	       libogg_LINK=""]],
	     [[minimal-build],
	      [WITH_libfixwav=yes
	       WITH_FLAC=yes
	       WITH_libogg=yes
	       WITH_sox=yes
	       libogg_BUILD=yes
	       flac_BUILD=yes
	       libfixwav_BUILD=yes
	       sox_BUILD=yes]],
	     [[ImageMagick-build],[DVDA_INF([IMAGEMAGICK_MSG])]],
	     [[static-sox],
	      [DVDA_INF([SOX_STATIC_MSG])
	       sox_LINK="$sox_LINK -lasound -lpng -lz -lltdl -lmagic -lsamplerate"
	       sox_LIB="/usr/lib/libsox.a $(find /usr/lib/sox/ -regex lib.*a)"]],
	     [[all-all],
		[all_builds=yes]],
	     [[all-builds]]])

   ])

     m4_define([DOWNLOAD_OPTIONS],[
	    [
		[dvdauthor-patch],
		[0.7.1],
		[http://dvd-audio.sourceforge.net/patches/dvdauthor-patch-0.7.1],
		[http://downloads.sourceforge.net/project/dvdauthor/dvdauthor/0.7.1],
		[http://dvd-audio.sourceforge.net/utils],
		[2694a5a3ef460106ea3caf0f7f60ff80]
	    ],
	    [
		[cdrtools-patch],
		[3.00],
		[http://dvd-audio.sourceforge.net/patches/mkisofs/cdrtools-patch-3.00],
		[ftp://ftp.berlios.de/pub/cdrecord],
		[http://dvd-audio.sourceforge.net/utils],
		[bb21cefefcfbb76cf249120e8978ffdd]
	    ],
	    [
		[sox-patch],
		[14.4.1],
		[http://dvd-audio.sourceforge.net/patches/sox-patch-14.4.1],
		[http://downloads.sourceforge.net/project/sox/sox/14.4.1],
		[http://dvd-audio.sourceforge.net/utils],
		[ff9ca6aca972549de0e80e8e30ed379c]
	    ],
	    [
		[flac-download],
		[1.3.0],
		[],
		[http://downloads.xiph.org/releases/flac],
		[http://dvd-audio.sourceforge.net/utils],
		[13b5c214cee8373464d3d65dee362cdd]
	    ],
	    [
		[libogg-download],
		[1.3.1],
		[],
		[http://downloads.xiph.org/releases/ogg],
		[http://dvd-audio.sourceforge.net/utils],
		[ba526cd8f4403a5d351a9efaa8608fbc]
	    ],
	    [
		[help2man-download],
		[1.43.3],
		[],
		[http://mirror.ibcp.fr/pub/gnu/help2man],
		[http://dvd-audio.sourceforge.net/utils],
		[a84868db7c139238df8add5d86a0b54f]
	    ],
    	    [
		[man2html-download],
		[1.6],
		[],
		[],
		[http://dvd-audio.sourceforge.net/utils],
		[ad4f385addc87974de373d7057a2ea7b]
	    ],
	    [
		[ImageMagick-download],
		[6.8.7-0],
		[],
		[ftp://mirrors.linsrv.net/pub/ImageMagick],
		[ftp://ftp.sunet.se/pub/multimedia/graphics/ImageMagick],
		[2f3854878735be72e66ac53a3146b63d]
	    ],
	    [
		[lplex-download],
		[0.3],
		[],
		[],
		[https://downloads.sourceforge.net/project/dvd-audio/dvda-author/dvda-author-dev/external%20packages],
		[c28803dfe4b136a2c25bbb088b2eafce]
	    ],
	    [
		[mjpegtools-download],
		[2.1.0],
		[],
		[http://sourceforge.net/projects/mjpeg/files/mjpegtools/2.1.0],
		[http://dvd-audio.sourceforge.net/utils],
		[57bf5dd78976ca9bac972a6511b236f3]
	    ],
	    [
		[libmpeg2-download],
		[0.5.1],
		[],
		[http://libmpeg2.sourceforge.net/files],
		[http://dvd-audio.sourceforge.net/utils],
		[0f92c7454e58379b4a5a378485bbd8ef]
	    ],
	    [
		[a52dec-download],
		[0.7.4],
		[],
		[http://liba52.sourceforge.net/files],
		[http://dvd-audio.sourceforge.net/utils],
		[caa9f5bc44232dc8aeea773fea56be80]
	    ]])


m4_define([DOWNLOAD_MINIMAL_OPTIONS],[
	    [
		[flac-download],
		[1.3.0],
		[],
		[http://downloads.xiph.org/releases/flac],
		[http://dvd-audio.sourceforge.net/utils],
		[13b5c214cee8373464d3d65dee362cdd]
	    ],
	    [
		[libogg-download],
		[1.3.1],
		[],
		[http://downloads.xiph.org/releases/ogg],
		[http://dvd-audio.sourceforge.net/utils],
		[ba526cd8f4403a5d351a9efaa8608fbc]
	    ],
	    [
		[sox-patch],
		[14.4.1],
		[http://dvd-audio.sourceforge.net/patches/sox-patch-14.4.1],
		[http://downloads.sourceforge.net/project/sox/sox/14.4.1],
		[http://dvd-audio.sourceforge.net/utils],
		[ff9ca6aca972549de0e80e8e30ed379c]
	    ]])


    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[all-deps]])
    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[all-all]])
    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [DOWNLOAD_OPTIONS])
    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[minimal-deps]])

    # owing to sox versioning issues (notably intervention of home-made getopt in versions <= 14.3), a wider array of function checks is justified.
    # functions should be white-space separated, as should header-function list double-quoted pairs. Headers are comma-separated from funtion lists in pair.

    m4_map([DVDA_ARG_WITH],[
      [[flac],  [[[FLAC/all.h],[FLAC__stream_decoder_init_file]]]],
      [[libogg],   [[[ogg/ogg.h], [ogg_stream_init]]]],
      [[libfixwav],[[[fixwav_manager.h],[fixwav]]]],
      [[sox],   [[[sox.h],     [sox_format_init  sox_open_read
				sox_open_write sox_create_effects_chain
				sox_create_effect sox_find_effect
				sox_add_effect sox_flow_effectsb
				sox_delete_effects sox_close sox_format_quit]]],[shared]],
      [[libiberty],[[[getopt.h],[getopt  getopt_long]],[[stdlib.h],[getsubopt]],
		[[string.h],[strchrnul]],           [[strndup.h],[strndup]]],[static]]])

    # to be invoked after ENABLE and WITH features
    # insert here application-specific macros that cannot be inserted in another file

 # auxiliary libs installed under local/ within package to avoid possible versioning issues with system-installed libs

    DVDA_CONFIG_LIBRARY_LOCAL_INSTALL([
     [[[sox],[sox-14.4.1]],  [--without-libltdl --without-mad --with-pkgconfigdir=no --without-flac --without-ladspa --without-twolame --without-lame --without-ffmpeg --disable-fast-install --prefix="$BUILDDIR/local" CPPFLAGS="-I$BUILDDIR/local/include"]],
     [[[libogg],[libogg-1.3.1]],  [--prefix="$BUILDDIR/local" CPPFLAGS="-I$BUILDDIR/local/include"]],
     [[[FLAC],[flac-1.3.0]],[--enable-static --disable-shared --disable-fast-install --with-ogg-libraries="$BUILDDIR/local/lib" --with-ogg-includes="$BUILDDIR/local/include/ogg" \
       --disable-thorough-tests --disable-oggtest --disable-doxygen-docs --disable-xmms-plugin --disable-doxygen-docs --prefix="$BUILDDIR/local" CPPFLAGS="-I$BUILDDIR/local/include"]]])
       
     # installing binaries, normally executables

    AS_CASE([${build}],
	    [*-*-mingw32*],
	    [

	     DVDA_CONFIG_EXECUTABLE_INSTALL([[[[dvdauthor],[dvdauthor-0.7.1]],[--disable-xmltest --disable-dvdunauthor --prefix="$BUILDDIR/local"]],
	       [[[lplex], [lplex-0.3]], [--prefix="$BUILDDIR/local" --disable-shared --with-libFLAC-libraries="$BUILDDIR/local/lib" --with-libFLAC-includes="$BUILDDIR/local/include"]],
	       [[[mjpegtools], [mjpegtools-2.1.0]],
			       [ --prefix="$BUILDDIR/local" 
				 --disable-shared  --enable-static --enable-static-build --disable-fast-install --prefix="$BUILDDIR/local"
				 --without-gtk --without-libpng --without-libdv --without-dga --without-libsdl --without-libquicktime
				 --disable-simd-accel LIBDIR=/lib LDFLAGS=-L/lib CPPFLAGS=-I/include]],
	       [[[cdrtools],[cdrtools-3.00]],[--prefix="$BUILDDIR/local"]],
	       [[[a52dec],[a52dec-0.7.4]],[--prefix="$BUILDDIR/local"]],
	       [[[libmpeg2],[libmpeg2-0.5.1]],[--prefix="$BUILDDIR/local"]],
	       [[[help2man], [help2man-1.43.3]],[--prefix="$BUILDDIR/local"]],
	       [[[ImageMagick], [ImageMagick-6.8.7-0]],[--prefix="$BUILDDIR/local"]],
	       [[[man2html], [man2html-1.6]],[bindir="$BUILDDIR/local/bin"]]])
	    ],
	    [
	     DVDA_CONFIG_EXECUTABLE_INSTALL([[[[dvdauthor],[dvdauthor-0.7.1]],[--disable-xmltest --disable-dvdunauthor --prefix="$BUILDDIR/local"]],
	       [[[lplex], [lplex-0.3]], [--prefix="$BUILDDIR/local" --disable-shared --with-libFLAC-libraries="$BUILDDIR/local/lib" --with-libFLAC-includes="$BUILDDIR/local/include"]],
	       [[[mjpegtools], [mjpegtools-2.1.0]],[--without-gtk  --without-libdv --without-dga --without-libsdl --without-libquicktime  --without-pic --disable-shared --enable-static-build --enable-static  --disable-fast-install --prefix="$BUILDDIR/local"]],
	       [[[cdrtools],[cdrtools-3.00]],[--prefix="$BUILDDIR/local"]],
	       [[[a52dec],[a52dec-0.7.4]],[--prefix="$BUILDDIR/local"]],
	       [[[libmpeg2],[libmpeg2-0.5.1]], [--prefix="$BUILDDIR/local"]],
	       [[[help2man], [help2man-1.43.3]],[--prefix="$BUILDDIR/local"]],
	       [[[ImageMagick], [ImageMagick-6.8.7-0]],[--prefix="$BUILDDIR/local"]],
       	       [[[man2html], [man2html]],[bindir="$BUILDDIR/local/bin"]]])
	    ])
    
    # auxiliary libs that remain within package, not installed

    DVDA_CONFIG_LIBRARY_NO_INSTALL([[[[libiberty],[libiberty/src]]], [[[libfixwav],[libfixwav/src]]]])

])

