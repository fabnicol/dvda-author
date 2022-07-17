
# File dependencies.m4

# Application-specific data for extended M4, M4sh and autoconf macros in dvda.m4
# ==============================================================================
# File is under copyright by Fabrice Nicol, 2009-2020

# To add new tests, just add new data abiding by the data structure, especially M4 quotes


AC_DEFUN([DVDA_CHECK_DEPENDENCIES],
[

AC_DEFINE([WEBSITE],["https://dvd-audio.sourceforge.io"],["Project website"])

# sftp path is /home/frs/project/dvd-audio after logging to frs.sourceforge.net

m4_define([DEPS],[https://sourceforge.net/projects/dvd-audio/files/dvda-author/Dependencies])

# ====== Auxiliary tools: man page and html doc, mjpegtools, ImageMagick ========= #

#mkisofs and cdrecord are tested by DVDA_DOWNLOAD

m4_map([DVDA_TEST_AUX],[
	[[help2man],  [man page will be generated after install: help2man]],
	[[man2html],  [man page will be generated after install: man2html]],
    [[wkhtmltopdf], [pdf manpage will be generated from html page]],
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
	[[libtoolize],[configure libraries with libtool]],
	[[automake],  [make system build: automake]],
	[[lplex],     [using lplex to mux lpcm audio and video]],
	[[make],      [whether make is installed]],
	[[mpeg2dec],  [whether mpeg2dec is installed]],
	[[git],       [whether git can be used for archiving purposes]],
	[[a52dec],    [whether a52dec is installed]],
    [[ffmpeg],    [whether ffmpeg is installed]],
	[[vlc],       [whether vlc player can be used]]])


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
	     [[ffmpeg-build]],
	     [[libogg-build]],
	     [[libiberty-build]],
	     [[cdrtools-build]],
	     [[flac-build],[libogg_BUILD=yes]],
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
	      [
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
	       libogg_LINK=""
	       withval_lplex=no
	       ]],
	     [[minimal-build],
	      [
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
   	     [[ffmpeg-build]],
	     [[libogg-build]],
	     [[libiberty-build]],
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
	      [
	       WITH_libiberty=no
	       MAYBE_libiberty=""
	       libiberty_BUILD=no
	       withval_FLAC=no
	       withval_libogg=no
	       withval_sox=no
	       libogg_BUILD=no
	       libogg_DOWNLOAD=no
	       MAYBE_libogg=""
	       libogg_LINK=""
	       withval_lplex=no
	       ]],
	     [[minimal-build],
	      [
	       WITH_FLAC=yes
	       WITH_libogg=yes
	       WITH_sox=yes
	       ffmpeg_BUILD=yes
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
		[DEPS/patches/dvdauthor-patch-0.7.1],
		[],
		[DEPS],
		[fafb6291c423e6db6b7d0b219a3b6641]
	    ],
	    [
		[cdrtools-download],
		[3.02a09],
		[],
		[],
		[DEPS],
		[ae85b5330f1f92d909e332ed595bddb7]
	    ],
	    [
		[sox-patch],
		[14.4.2],
		[DEPS/patches/sox-patch-14.4.2],
		[],
		[DEPS],
		[a44b293232d8068feeb07f10fe7b7574]
	    ],
	    [
		[flac-download],
		[1.3.4],
		[],
		[],
		[DEPS],
		[9422d35889d7e33f7966c6c2ee5c6417]
	    ],
	    [
		[libogg-download],
		[1.3.4],
		[],
		[],
		[DEPS],
		[c163bc12bc300c401b6aa35907ac682671ea376f13ae0969a220f7ddf71893fe]
	    ],
	    [
		[help2man-patch],
		[1.47.16],
		[DEPS/patches/help2man-patch-1.47.16],
		[],
		[DEPS],
		[6a67db27ea0b478d49add546a7ad5c68]
        ],
        [
		[man2html-download],
		[1.6],
		[],
		[],
		[DEPS],
		[74b3156fb5ba68a48b07b568d181e771]
	    ],
	    [
		[imagemagick-download],
		[7.0.10-35],
		[],
		[],
		[DEPS],
		[656de45e34f04d7fb431b242932d5440]
	    ],
	    [
		[lplex-download],
		[0.3],
		[],
		[],
		[DEPS],
		[c1fd3438afedf1ab6fcdc646ccdc1b77]
	    ],
	    [
		[mjpegtools-download],
		[2.2.1],
		[],
		[],
		[DEPS],
		[875fffd2dcdd6913e121340fa7424ba4]
	    ],
	    [
		[libmpeg2-download],
		[0.5.1],
		[],
		[],
		[DEPS],
		[99311a5f3a393560db096a41dcf84204]
	    ],
	    [
		[a52dec-download],
		[0.7.4],
		[],
		[],
		[DEPS],
		[9b4249efde7cf6c10107f41497df30ef]
	    ],
		[
			[ffmpeg-download],
			[4.2.4],
			[],
			[],
			[DEPS],
			[9d4cdecb811bb5f14b53498568481e7f]
		]])


m4_define([DOWNLOAD_MINIMAL_OPTIONS],[
	    [
		[flac-download],
		[1.3.4],
		[],
		[],
		[DEPS],
		[9422d35889d7e33f7966c6c2ee5c6417]
	    ],
	    [
		[libogg-download],
		[1.3.4],
		[],
		[],
		[DEPS],
		[c163bc12bc300c401b6aa35907ac682671ea376f13ae0969a220f7ddf71893fe]
	    ],
	    [
		[sox-patch],
		[14.4.2],
		[DEPS/patches/sox-patch-14.4.2],
		[],
		[DEPS],
		[a44b293232d8068feeb07f10fe7b7574]
	    ]])


    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[all-deps]])
    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[all-all]])
    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [DOWNLOAD_OPTIONS])
    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[minimal-deps]])

    # owing to sox versioning issues (notably intervention of home-made getopt in versions <= 14.3), a wider array of function checks is justified.
    # functions should be white-space separated, as should header-function list double-quoted pairs. Headers are comma-separated from funtion lists in pair.

    m4_map([DVDA_ARG_WITH],[
      [[lplex],  [[[lplex.h],[author]]]],
      [[flac],  [[[FLAC/all.h],[FLAC__stream_decoder_init_file]]]],
      [[libogg],   [[[ogg/ogg.h], [ogg_stream_init]]]],
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
     [[[sox],[sox-14.4.2]],  [--without-libltdl --without-sndfile --without-mad --with-pkgconfigdir=no --without-flac --without-ladspa --without-twolame --without-lame --without-magic --disable-fast-install --enable-static --with-pic --disable-shared --prefix="$BUILDDIR/local" CFLAGS=-fPIE CXXFLAGS=-fPIE CPPFLAGS="-I$BUILDDIR/local/include"]],
     [[[libogg],[libogg-1.3.4]],  [--enable-static --disable-shared --prefix="$BUILDDIR/local" CFLAGS=-fPIC CXXFLAGS=-fPIC CPPFLAGS="-I$BUILDDIR/local/include"]],
     [[[FLAC],[flac-1.3.4]],[--enable-static --disable-shared --disable-fast-install --with-ogg-libraries="$BUILDDIR/local/lib" --with-ogg-includes="$BUILDDIR/local/include/ogg" \
       --disable-thorough-tests --disable-oggtest --disable-doxygen-docs --disable-xmms-plugin --disable-doxygen-docs --prefix="$BUILDDIR/local" CFLAGS=-fPIC CXXFLAGS=-fPIC CPPFLAGS="-I$BUILDDIR/local/include"]],
     [[[ffmpeg], [ffmpeg-4.2.4]], [--prefix="$BUILDDIR/local" --disable-demuxers --disable-decoders --disable-muxers --disable-parsers --disable-encoders --disable-devices --disable-protocols --enable-protocol=file --enable-protocol=data --disable-bsfs --disable-hwaccels  --disable-filters   --enable-decoder=mlp --enable-encoder=mlp --enable-encoder=pcm_s16le --enable-encoder=pcm_s24le --enable-encoder=pcm_s32le --enable-decoder=pcm_s16le --enable-decoder=pcm_s24le --enable-decoder=pcm_s24le  --enable-parser=mlp --enable-muxer=wav --enable-muxer=null --enable-muxer=truehd --enable-muxer=mlp --enable-demuxer=mlp --enable-muxer=pcm_s16le --enable-muxer=pcm_s24le --enable-muxer=pcm_s32le --enable-demuxer=pcm_s16le --enable-demuxer=pcm_s24le --enable-demuxer=pcm_s32le  --enable-filter=aresample --disable-bzlib --disable-iconv --disable-libxcb --disable-libxcb-shm --disable-libxcb-xfixes --disable-libxcb-shape --disable-sndio --disable-sdl2 --disable-zlib --disable-xlib --disable-libdrm --disable-vaapi --disable-vdpau --disable-videotoolbox  --enable-static --disable-shared   --enable-demuxer=wav --enable-demuxer=truehd  --disable-swscale  --disable-network --disable-postproc --disable-pixelutils --disable-avdevice --disable-alsa --disable-lzma --disable-doc --disable-d3d11va --disable-amd3dnow --disable-amd3dnowext  --disable-dxva2 --enable-pic]]])
       
     # installing binaries, normally executables

    AS_IF([test "$VIDEO_FORMAT" = ""],[VIDEO_FORMAT=PAL])

# avoid newlines within [ ... ]
 
    AS_CASE([${build}],
	    [*-*-mingw32*],
	    [
	     DVDA_CONFIG_EXECUTABLE_INSTALL([[[[dvdauthor],[dvdauthor-0.7.1]],[--disable-xmltest --disable-dvdunauthor --enable-default-video-format=$VIDEO_FORMAT --prefix="$BUILDDIR/local" CPPFLAGS=-I$ROOTDIR/lplex-0.3/redist]],
	       [[[lplex], [lplex-0.3]], [--prefix="$BUILDDIR/local" --disable-shared ROOTDIR=$ROOTDIR/lplex-0.3 --with-libFLAC-libraries="$BUILDDIR/local/lib" --with-libFLAC-includes="$BUILDDIR/local/include"]],
	       [[[mjpegtools], [mjpegtools-2.2.1]],
			       [ --prefix="$BUILDDIR/local" --disable-shared  --enable-static --enable-static-build --disable-fast-install --prefix="$BUILDDIR/local" --without-gtk  --without-libdv --without-dga --without-libsdl --without-libquicktime --disable-simd-accel LIBDIR=/lib LDFLAGS=-L/lib CXXFLAGS=-fPIC  CFLAGS=-fPIC CPPFLAGS=-I/include ]],
	       [[[cdrtools],[cdrtools-3.02]],[--prefix="$BUILDDIR/local"]],
	       [[[a52dec],[a52dec-0.7.4]],[--prefix="$BUILDDIR/local"]],
	       [[[libmpeg2],[libmpeg2-0.5.1]],[--prefix="$BUILDDIR/local" --disable-directx LDFLAGS="-Wl,--allow-multiple-definition"]],
	       [[[help2man], [help2man-1.47.16]],[--prefix="$BUILDDIR/local"]],
	       [[[ImageMagick], [ImageMagick-7.0.10-35]],[--prefix="$BUILDDIR/local" --without-magick-plus-plus --without-pango --without-tiff --without-lzma --without-xml]],
	       [[[man2html], [man2html-1.6]],[bindir="$BUILDDIR/local/bin"]]])
	    ],
	    [
	     DVDA_CONFIG_EXECUTABLE_INSTALL([[[[dvdauthor],[dvdauthor-0.7.1]],[--disable-xmltest --disable-dvdunauthor --enable-default-video-format=$VIDEO_FORMAT --prefix="$BUILDDIR/local"]],
	       [[[lplex], [lplex-0.3]], [--prefix="$BUILDDIR/local" --enable-static --disable-shared ROOTDIR=$ROOTDIR/lplex-0.3 --with-libFLAC-libraries="$BUILDDIR/local/lib" --with-libFLAC-includes="$BUILDDIR/local/include" CPPFLAGS=-I$ROOTDIR/lplex-0.3/redist]],
	       [[[mjpegtools], [mjpegtools-2.2.1]],[--without-gtk  --without-libdv --without-dga --without-libsdl --without-libquicktime --disable-shared --enable-static-build --enable-static  --disable-fast-install  --prefix="$BUILDDIR/local"  CXXFLAGS=-fPIC  CFLAGS=-fPIC]],
	       [[[cdrtools],[cdrtools-3.02]],[--prefix="$BUILDDIR/local"]],
	       [[[a52dec],[a52dec-0.7.4]],[--prefix="$BUILDDIR/local"]],
	       [[[libmpeg2],[libmpeg2-0.5.1]], [--enable-static --disable-shared --prefix="$BUILDDIR/local" --disable-directx]],
	       [[[help2man], [help2man-1.47.16]],[--prefix="$BUILDDIR/local"]],
	       [[[ImageMagick], [ImageMagick-7.0.10-35]],[--prefix="$BUILDDIR/local" --without-magick-plus-plus --without-pango --without-tiff --without-lzma --without-xml --enable-static --disable-shared JPEG_LIBS="$ROOTDIR/build/linux/dvda-author-full.build/lib/libjpeg.a"]],
   	       [[[man2html], [man2html]],[bindir="$BUILDDIR/local/bin" mandir="$BUILDDIR/local/share/man"]]])
	    ])

    # libjpeg.a version 9 is provided as full-build as version 8. causes a crash and is often installed on many Unix systems.
    # libjpeg package could as well be added to downloaded source...
    # auxiliary libs that remain within package, not installed

    DVDA_CONFIG_LIBRARY_NO_INSTALL([[[[libfixwav],[libfixwav/src]]]])
])
