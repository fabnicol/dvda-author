
# File dependencies.m4

# Application-specific data for extended M4, M4sh and autoconf macros in dvda.m4
# ==============================================================================
# File is under copyright by Fabrice Nicol, 2009

# To add new tests, just add new data abiding by the data structure, especially M4 quotes


AC_DEFUN([DVDA_CHECK_DEPENDENCIES],
[

# ====== Auxiliary tools: man page and html doc, mjpegtools, ImageMagick ========= #

#mkisofs and cdrecord are tested by DVDA_DOWNLOAD

m4_map([DVDA_TEST_AUX],[
        [[help2man],  [man page will be generated after install: help2man]],
        [[man2html],  [man page will be generated after install: man2html]],
        [[curl],      [patched version of prerequisites will be downloaded: curl]],
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
        [[smake],     [using smake instead of GNU make]],
        [[lplex],     [using lplex to mux lpcm audio and video]],
        [[make],      [whether make is installed]],
        [[mpeg2dec],  [whether mpeg2dec is installed]],
        [[a52dec],   [whether a52dec is installed]]])


  m4_define([SOX_STATIC_MSG],
    [Using static library build only. You may need to install the following libraries:
      libasound libpng libz libltdl libmagic libsamplerate, using "-dev" packages.
    Warning: Availability will not be tested. An error message 'Cannot find -l...' at the
    end of compiling stage will indicate that you need to install the corresponding library.])

  m4_define([IMAGEMAGICK_MSG],
    [With GNU/Linux you will need to run sudo ldconfig after make install.])


  #===================== build features ==============================================#



    m4_map([DVDA_ARG_ENABLE],
           [
             [[iberty-build]],
             [[ogg-build]],
             [[flac-build]],
             [[sox-build]],
             [[help2man-build]],
             [[lplex-build]],
             [[mpeg2dec-build]],
             [[a52dec-build]],
             [[mjpegtools-build]],
             [[core-build],
              [withval_FIXWAV=no
               withval_FLAC=no
               withval_OGG=no
               withval_SOX=no]],
             [[ImageMagick-build],[DVDA_INF([IMAGEMAGICK_MSG])]],
             [[dvdauthor-build]],
             [[cdrtools-build]],
             [[static-sox],
              [DVDA_INF([SOX_STATIC_MSG])
               SOX_LINK="$SOX_LINK -lasound -lpng -lz -lltdl -lmagic -lsamplerate"
               SOX_LIB="/usr/lib/libsox.a $(find /usr/lib/sox/ -regex lib.*a)"]],
             [[all-all],
                [ALL_BUILDS=yes]],
             [[all-builds]]])

  #=================  platform-specific features =====================================#


    AS_CASE([${build}],
            [*-*-mingw32*],[
                    DVDA_INF([MinGW detected: libsox and libiberty will be built from source])
                    DVDA_ARG_ENABLE([sox-build])
                    DVDA_ARG_ENABLE([iberty-build])],

            [*-*-cygwin*],[
                    DVDA_ARG_ENABLE([sox-build])
                    DVDA_ARG_ENABLE([iberty-build])
                    DVDA_INF([Cygwin detected: libsoX and glibc will be built from source])])
          # on cygwin, libsox dependencies will not be resolved (platform limitation)

    # put feature specific parameters after global parameters to redefine X_LIBs.


    # downloading (patched) version of source code
    # from sourceforge mirror network except for cdrtools (berlios)
    # possible Sourceforge mirrors: kent, garr, voxel, free_fr ... Specify ./configure SF_MIRRORS=kent on command line otherwise autoselect is performed

    m4_define([SF_MIRRORLIST],[kent,garr,voxel,free_fr])

    # basename(-patch), version=[untarred directory version name,appended label], root download site, root site for patch download, mirror root characteristics, MD5SUM for main package
    # download site = for Sourceforge:
    #     http://sourceforge.net/projects/basename/basename/mirror root characteristics/basename-version.tar.[bz2|gz]/download?use_mirror=$SF_MIRROR
    #               = for cdrtools:
    #     mirror root characteristics/basename-version.tar.[bz2|gz]


    m4_define([DOWNLOAD_OPTIONS],[
            [[dvdauthor-patch],[0.6.14],       [http://dvd-audio.sourceforge.net/patches],        [dvdauthor], [dvdauthor/0.6.14],                        [bd646b47950c4091ffd781d43fd2c5e9]],
            [[cdrtools-patch], [3.00],         [http://dvd-audio.sourceforge.net/patches/mkisofs],[],          [ftp://ftp.berlios.de/pub/cdrecord],       [bb21cefefcfbb76cf249120e8978ffdd]],
            [[sox-patch],      [14.4.1],       [http://dvd-audio.sourceforge.net/patches],        [sox],       [sox/14.4.1],                              [ff9ca6aca972549de0e80e8e30ed379c]],
            [[flac-download],  [1.3.0],        [http://dvd-audio.sourceforge.net/patches],        [],          [http://downloads.xiph.org/releases/flac], [13b5c214cee8373464d3d65dee362cdd]],
            [[libogg-download],   [1.1.4],     [],                                                [ogg],       [http://downloads.xiph.org/releases/ogg],  [10200ec22543841d9d1c23e0aed4e5e9]],
            [[help2man-download],[1.43.3],     [],                                                [],          [http://mirror.ibcp.fr/pub/gnu/help2man],  [a84868db7c139238df8add5d86a0b54f]],
            [[ImageMagick-download], [6.8.7-0],[],                                                [],          [http://www.imagemagick.org/download],     [2f3854878735be72e66ac53a3146b63d]],
            [[lplex-download], [0.3],          [],                                                [],          [http://dvd-audio.sourceforge.net],                                        [23e52c149ccfa0169955a57ff783fd21]],
            [[mjpegtools-download], [2.1.0],   [],                                                [mjpeg],     [mjpegtools/2.1.0],                        [57bf5dd78976ca9bac972a6511b236f3]],
            [[libmpeg2-download], [0.5.1],     [],                                                [],          [libmpeg2.sourceforge.net/files],          [0f92c7454e58379b4a5a378485bbd8ef]],
            [[a52dec-download], [0.7.4],       [],                                                [],          [http://liba52.sourceforge.net/files],                                        [caa9f5bc44232dc8aeea773fea56be80]]])

    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[all-deps]])

    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], [[all-all]])

#    m4_map([DVDA_ARG_ENABLE_DOWNLOAD], DOWNLOAD_OPTIONS)

    # for sox libs, empirically it appears safer to link to dynamic libs under linux at least, due to linking issues with static libs: shared forces this,
    # unless explicit filepath input is given
    # owing to sox versioning issues (notably intervention of home-made getopt in versions <= 14.3), a wider array of function checks is justified.
    # functions should be white-space separated, as should header-function list double-quoted pairs. Headers are comma-separated from funtion lists in pair.


    m4_map([DVDA_ARG_WITH],[
      [[FLAC],  [[[FLAC/all.h],[FLAC__stream_decoder_init_file]]]],
      [[ogg],   [[[ogg/ogg.h], [ogg_stream_init]]]],
      [[fixwav],[[[fixwav_manager.h],[fixwav]]]],
      [[sox],   [[[sox.h],     [sox_format_init  sox_open_read
                                sox_open_write sox_create_effects_chain
                                sox_create_effect sox_find_effect
                                sox_add_effect sox_flow_effects
                                sox_delete_effects sox_close sox_format_quit]]],[shared]],
      [[iberty],[[[getopt.h],[getopt  getopt_long]],[[stdlib.h],[getsubopt]],
                [[string.h],[strchrnul]],           [[strndup.h],[strndup]]],[static]]])

    # to be invoked after ENABLE and WITH features
    # insert here application-specific macros that cannot be inserted in anothor file

    DISABLE_OGG_TEST

    # installing binaries, normally executables

    DVDA_CONFIG_EXECUTABLE_INSTALL([[[[DVDAUTHOR],[dvdauthor-0.6.14]]],
               [[[LPLEX], [lplex-0.3]], [--prefix=$prefix --disable-shared]],
               [[[MJPEGTOOLS], [mjpegtools-2.1.0]],[--enable-static-build --disable-fast-install --prefix=$ROOTDIR/local]],
               [[[CDRTOOLS],[cdrtools-3.00]]],
               [[[A52DEC],[a52dec-0.7.4]],[--prefix=$prefix]],
               [[[MPEG2DEC],[mpeg2dec-0.5.1]], [--prefix=$prefix]],
               [[[HELP2MAN], [help2man-1.43.3]]],
               [[[IMAGEMAGICK], [ImageMagick-6.8.7-0]]]])

    # auxiliary libs installed under local/ within package to avoid possible versioning issues with system-installed libs

    DVDA_CONFIG_LIBRARY_LOCAL_INSTALL([
     [[[FLAC],[flac-1.3.0]],[--disable-shared --disable-thorough-tests --disable-oggtest --disable-cpplibs --disable-doxygen-docs --disable-xmms-plugin --disable-doxygen-docs --prefix=$ROOTDIR/local CPPFLAGS="-I$ROOTDIR/local/include"]],
     [[[SOX],[sox-14.4.1]],  [--without-mad --without-flac --without-lame --prefix=$ROOTDIR/local CPPFLAGS="-I$ROOTDIR/local/include"]],
     [[[OGG],[ogg-1.1.4]],  [--prefix=$ROOTDIR/local CPPFLAGS="-I$ROOTDIR/local/include"]]])

    # auxiliary libs that remain within package, not installed

    DVDA_CONFIG_LIBRARY_NO_INSTALL([[[[IBERTY],[libiberty]]], [[[FIXWAV],[libfixwav]]]])


])

