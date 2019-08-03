DEFINES += VERSION=\\\"dev-9999\\\"

TEMPLATE = app
CONFIG += console ordered
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CC = /usr/bin/gcc
SRCDIR="/home/fab/Dev/dvda-author/"
TARGET = dvda-author

QMAKE_CFLAGS=-std=c99 -march=core2 -fPIC -ftrack-macro-expansion=0

DEFINES +=   _GNU_SOURCE __CB__ GUI_BEHAVIOR HAVE_lplex=1 COMPILER_IS_GCC HAVE_curl=1 HAVE_fixwav=1 HAVE_libogg=1 HAVE_iberty=1 HAVE_mpeg2enc=1 HAVE_mplex=1 HAVE_OGG_FLAC=1 HAVE_FLAC=1 HAVE_libogg=1 HAVE_sox=1  FLAC_API_SUPPORTS_OGG_FLAC=1 USE_SET

DEFINES += STARTSECTOR=272 # Schily magic value

#libsox.a compiled using: ./configure --disable-symlinks --disable-fast-install --without-libltdl  --without-magic --without-png --without-ladspa --without-mad --without-lame --without-twolame
linux: QMAKE_LFLAGS += -L/usr/lib64
linux:LIBS +=	 "/usr/lib64/libgsm.a"  "/usr/lib64/libMagick++-7.Q16.a" "/usr/lib64/libMagickCore-7.Q16.a" "/usr/lib64/libMagickWand-7.Q16.a" \
                        $$SRCDIR"local/lib/liblavfile.a" \
                        $$SRCDIR"local/lib/liblavjpeg.a" \
                        $$SRCDIR"local/lib/libsox.a" \
                        $$SRCDIR"local/lib/libffmpeg.a" \
                        $$SRCDIR"local/lib/libavfilter.a" \
                        $$SRCDIR"local/lib/libavformat.a" \
                        $$SRCDIR"local/lib/libavcodec.a" \
                        $$SRCDIR"local/lib/libavutil.a" \
                        "/usr/lib64/libsndfile.a" \
                        "/usr/lib64/libvorbisenc.a" \
                        "/usr/lib64/libvorbisfile.a"  \
                        "/usr/lib64/libvorbis.a" \
                        "/usr/lib64/libwavpack.a" \
                        "/usr/lib64/libFLAC.a" \
                        "/usr/lib64/libogg.a" \
                        "/usr/lib64/libopusfile.a" \
                        "/usr/lib64/libopus.a" \
                        "/usr/lib64/libopusurl.a" \
                        "/usr/lib64/libpng.a" \
                        "/usr/lib64/libz.a" \
                        "/opt/lib/libao.a" \                # Xiph libao
                        "/opt/lib/libasound.a" \            # Alsa project alsa-libs package
                        "/usr/lib64/libdvdread.a" \
                        "/usr/lib64/libjpeg.a" \
                        "/usr/lib64/libwebp.a" \
                        "/usr/lib/gcc/x86_64-pc-linux-gnu/9.1.0/libgomp.a" \
                        "/usr/local/lib/libpulse-simple.a" \                       # pulseaudio package
                        "/usr/local/lib/libpulse.a" \
                        "/usr/local/lib/pulseaudio/libpulsecommon-12.2.a" \
                        "/usr/lib64/libxcb.a" \
                        "/usr/lib64/libXdmcp.a" \
                        "/usr/lib64/libXau.a" \
                        "/usr/lib64/libdbus-1.a" \
                        "/usr/lib64/libSDL.a" \
                        "/usr/lib/gcc/x86_64-pc-linux-gnu/9.1.0/libgcc_eh.a" \
                        -static -static-libgcc \
                        "/usr/lib64/librt.a" \
                        "/usr/lib64/libresolv.a" \
                        -pthread \
                        -ldl



CONFIG(release, debug|release) {
LIBS +=   -s

QMAKE_CFLAGS=-march=core2 -O3 -Wno-unused-result
}

INCLUDEPATH = src/include libutils/src/include libutils/src/include libutils/src/private libfixwav/src/include libs/include/FLAC libs/include/libsoxconvert libiberty/src/include ffmpeg

SOURCES += \
    src/amg2.c \
    src/asvs.c \
    src/ats.c \
    src/ats2wav.c \
    src/atsi2.c \
    src/audio.c \
    src/auxiliary.c \
    src/command_line_parsing.c \
    src/file_input_parsing.c \
    src/launch_manager.c \
    src/lexer.c \
    src/libsoxconvert.c \
    src/menu.c \
    src/samg2.c \
    src/sound.c \
    src/videoimport.c \
    src/xml.c \
    libutils/src/winport.c \
    libutils/src/libc_utils.c \
    libfixwav/src/checkData.c \
    libfixwav/src/checkParameters.c \
    libfixwav/src/manager.c \
    libfixwav/src/readHeader.c \
    libfixwav/src/repair.c \
    src/dvda-author.c \
    libfixwav/src/fixwav_auxiliary.c \
    libiberty/src/getopt.c \
    libiberty/src/getsubopt.c \
    libiberty/src/strchrnul.c \
    libiberty/src/strnlen.c

OTHER_FILES += \
    src/dvda-author.conf \
    libfixwav/AUTHORS \
    libfixwav/COPYING \
    libfixwav/INSTALL \
    libfixwav/NEWS \
    libfixwav/README \
    libfixwav/ChangeLog \
    libfixwav/Makefile.am \
    libfixwav/src/include/Makefile.am \
    libs/include/FLAC/Makefile.am \
    m4/dependencies.m4 \
    m4/dvda.m4 \
    m4/auxiliary.m4 \
    configure.ac \
    AUTHORS \
    COPYING \
    COREBUILD \
    DEPENDENCIES \
    EXAMPLES.in \
    LIMITATIONS \
    INSTALL \
    HOWTO.conf \
    README \
    NEWS \
    TODO \
    ChangeLog \
    libiberty/src/Makefile.am \
    libiberty/src/include/Makefile.am \
    libutils/Makefile.am \
    libfixwav/src/Makefile.am \
    Makefile.in \
    src/Makefile.in \
    libutils/src/Makefile.in \
    m4/m4.variables \
    mk/functions.mk.in \
    mk/FLAC.mk.in \
    mk/libogg.mk.in \
    mk/mjpegtools.mk.in \
    mk/dvdauthor.mk.in \
    mk/ImageMagick.mk.in \
    mk/lplex.mk.in \
    mk/help2man.mk.in \
    mk/a52dec.mk.in \
    mk/cdrtools.mk.in \
    libfixwav/src/Makefile.in \
    libfixwav/src/Makefile.am.user \
    libfixwav/src/Makefile.inMakefile.am \
    mk/sox.mk.in \
    libiberty/src/Makefile.in \
    autogen \
    mk/libmpeg2.mk.in \
    script.mjpegtools \
    mk/man2html.mk.in \
    libiberty/src/include/Makefile.in

HEADERS += \
    src/include/amg.h \
    src/include/asvs.h \
    src/include/ats.h \
    src/include/ats2wav.h \
    src/include/atsi.h \
    src/include/audio2.h \
    src/include/auxiliary.h \
    src/include/command_line_parsing.h \
    src/include/commonvars.h \
    src/include/dvda-author.h \
    src/include/file_input_parsing.h \
    src/include/launch_manager.h \
    src/include/lexer.h \
    src/include/libsoxconvert.h \
    src/include/menu.h \
    src/include/multichannel.h \
    src/include/ports.h \
    src/include/samg.h \
    src/include/sound.h \
    src/include/structures.h \
    src/include/version.h \
    src/include/videoimport.h \
    src/include/xml.h \
    libutils/src/include/c_utils.h \
    libutils/src/include/decode.h \
    libutils/src/include/export.h \
    libutils/src/include/format.h \
    libutils/src/include/libiberty.h \
    libutils/src/include/ordinals.h \
    libutils/src/include/stream_decoder.h \
    libutils/src/include/stream_encoder.h \
    libutils/src/include/winport.h \
    libfixwav/src/include/audio.h \
    libfixwav/src/include/checkData.h \
    libfixwav/src/include/checkParameters.h \
    libfixwav/src/include/fixwav_auxiliary.h \
    libfixwav/src/include/fixwav_manager.h \
    libfixwav/src/include/fixwav.h \
    libfixwav/src/include/launch_fixwav.h \
    libfixwav/src/include/readHeader.h \
    libfixwav/src/include/repair.h \
    libs/include/FLAC/all.h \
    libs/include/FLAC/assert.h \
    libs/include/FLAC/callback.h \
    libs/include/FLAC/export.h \
    libs/include/FLAC/format.h \
    libs/include/FLAC/ordinals.h \
    libs/include/FLAC/stream_decoder.h \
    libs/include/FLAC/stream_encoder.h \
    libs/include/libsoxconvert/effects.h \
    libs/include/libsoxconvert/adpcm.h \
    libs/include/libsoxconvert/adpcms.h \
    libs/include/libsoxconvert/aiff.h \
    libs/include/libsoxconvert/aliases.h \
    libs/include/libsoxconvert/amr.h \
    libs/include/libsoxconvert/amr1.h \
    libs/include/libsoxconvert/amr2.h \
    libs/include/libsoxconvert/band.h \
    libs/include/libsoxconvert/biquad.h \
    libs/include/libsoxconvert/compandt.h \
    libs/include/libsoxconvert/cvsd.h \
    libs/include/libsoxconvert/cvsdfilt.h \
    libs/include/libsoxconvert/dft_filter.h \
    libs/include/libsoxconvert/dither.h \
    libs/include/libsoxconvert/f2c.h \
    libs/include/libsoxconvert/ffmpeg.h \
    libs/include/libsoxconvert/fft4g.h \
    libs/include/libsoxconvert/fifo.h \
    libs/include/libsoxconvert/formats.h \
    libs/include/libsoxconvert/g72x.h \
    libs/include/libsoxconvert/g711.h \
    libs/include/libsoxconvert/getopt.h \
    libs/include/libsoxconvert/gsm.h \
    libs/include/libsoxconvert/ignore-warning.h \
    libs/include/libsoxconvert/ima_rw.h \
    libs/include/libsoxconvert/lpc10.h \
    libs/include/libsoxconvert/mcompand_xover.h \
    libs/include/libsoxconvert/mp3-util.h \
    libs/include/libsoxconvert/noisered.h \
    libs/include/libsoxconvert/private.h \
    libs/include/libsoxconvert/rate_filters.h \
    libs/include/libsoxconvert/rate_half_fir.h \
    libs/include/libsoxconvert/rate_poly_fir.h \
    libs/include/libsoxconvert/rate_poly_fir0.h \
    libs/include/libsoxconvert/raw.h \
    libs/include/libsoxconvert/sgetopt.h \
    libs/include/libsoxconvert/sox_i.h \
    libs/include/libsoxconvert/sox_sample_test.h \
    libs/include/libsoxconvert/sox.h \
    libs/include/libsoxconvert/soxconfig.h \
    libs/include/libsoxconvert/soxomp.h \
    libs/include/libsoxconvert/soxstdint.h \
    libs/include/libsoxconvert/util.h \
    libs/include/libsoxconvert/vox.h \
    libs/include/libsoxconvert/win32-glob.h \
    libs/include/libsoxconvert/win32-ltdl.h \
    libs/include/libsoxconvert/xmalloc.h \
    libutils/src/private/private_c_utils.h \
    libiberty/src/include/getopt_int.h \
    libiberty/src/include/getopt.h \
    libiberty/src/include/getsubopt.h \
    libiberty/src/include/strchrnul.h \
    libiberty/src/include/strdup.h \
    libiberty/src/include/strndup.h \
    libiberty/src/include/strnlen.h

DISTFILES += \
    libfixwav/fixwav.autorepair.md \
    libfixwav/fixwav.autorepair.pdf

