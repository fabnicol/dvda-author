# Makefile for lavtools

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/utils $(LIBQUICKTIME_CFLAGS)

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

# *********************************************************************
# Support lib

lib_LTLIBRARIES = liblavfile.la liblavjpeg.la

if HAVE_V4L
  lib_LTLIBRARIES += liblavrec.la
endif
if HAVE_V4L_OR_SDL
  lib_LTLIBRARIES += liblavplay.la
endif


liblavincludedir = $(pkgincludedir)
liblavinclude_HEADERS = \
	avilib.h \
	editlist.h \
	lav_io.h \
	jpegutils.h \
	frequencies.h \
	liblavrec.h \
	liblavplay.h \
	audiolib.h

LAV_ALL_LIB_OPTS = \
	$(LT_STATIC) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) $(EXTRA_LDFLAGS)

liblavfile_la_SOURCES = editlist.c lav_io.c avilib.c
liblavfile_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBDV_CFLAGS) $(JPEG_CFLAGS)
liblavfile_la_LDFLAGS = $(LAV_ALL_LIB_OPTS)
liblavfile_la_LIBADD = $(LIBQUICKTIME_LIBS) $(JPEG_LIBS) $(LIBDV_LIBS) $(LIBMJPEGUTILS)

liblavjpeg_la_SOURCES = jpegutils.c
liblavjpeg_la_CPPFLAGS = $(AM_CPPFLAGS) $(JPEG_CFLAGS)
liblavjpeg_la_LDFLAGS = $(LAV_ALL_LIB_OPTS)
liblavjpeg_la_LIBADD = $(JPEG_LIBS) $(LIBMJPEGUTILS)

liblavrec_la_SOURCES = liblavrec.c audiolib.c frequencies.c
liblavrec_la_CPPFLAGS = $(AM_CPPFLAGS) # so automake doesn't complain about audiolib & frequencies
liblavrec_la_LDFLAGS = $(LAV_ALL_LIB_OPTS)
liblavrec_la_LIBADD = liblavfile.la liblavjpeg.la $(LIBMJPEGUTILS)
liblavrec_la_DEPENDENCIES = liblavfile.la liblavjpeg.la

liblavplay_la_SOURCES = liblavplay.c audiolib.c
liblavplay_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBDV_CFLAGS) $(X_CFLAGS) $(SDL_CFLAGS)
liblavplay_la_LDFLAGS = $(LAV_ALL_LIB_OPTS)
liblavplay_la_LIBADD = liblavfile.la $(SDL_LIBS) liblavjpeg.la $(LIBDV_LIBS) $(LIBMJPEGUTILS)
if HAVE_SDL
   liblavplay_la_LIBADD += ${X_LIBS} -lX11
endif
liblavplay_la_DEPENDENCIES = liblavfile.la liblavjpeg.la

# *********************************************************************
# The tools themselves

bin_PROGRAMS = \
	mjpeg_simd_helper \
	lav2wav \
	lav2yuv \
	lavaddwav \
	lavtrans \
	ypipe \
        yuv2lav \
	transist.flt \
	matteblend.flt \
	multiblend.flt \
	lavpipe \
	jpeg2yuv \
	ppmtoy4m \
	pnmtoy4m \
	y4mtoppm \
	y4mtopnm \
	y4mcolorbars \
	lavinfo \
	y4mstabilizer \
	yuvfps \
	yuvinactive

if HAVE_GTK
  bin_PROGRAMS += glav
endif
if HAVE_SDL
  bin_PROGRAMS += yuvplay
endif
if HAVE_V4L
  bin_PROGRAMS += lavrec lavvideo
endif
if HAVE_V4L_OR_SDL
  bin_PROGRAMS += lavplay
endif
if HAVE_LIBPNG
  bin_PROGRAMS += png2yuv
endif

noinst_HEADERS = \
	colorspace.h \
	glav.h \
	pipelist.h \
	subsample.h \
	lav_common.h

mjpeg_simd_helper_SOURCES = mjpeg_simd_helper.c
mjpeg_simd_helper_LDADD = $(LIBMJPEGUTILS)

ypipe_SOURCES = ypipe.c
ypipe_LDADD = $(LIBMJPEGUTILS)

yuv2lav_SOURCES = yuv2lav.c
yuv2lav_LDADD = $(LIBMJPEGUTILS) liblavfile.la liblavjpeg.la $(LIBM_LIBS)

transist_flt_SOURCES = transist.flt.c
transist_flt_LDADD = $(LIBMJPEGUTILS)

matteblend_flt_SOURCES = matteblend.flt.c
matteblend_flt_LDADD = $(LIBMJPEGUTILS)

multiblend_flt_SOURCES = multiblend.flt.c
multiblend_flt_LDADD = $(LIBMJPEGUTILS)

lavplay_SOURCES = lavplay.c
lavplay_CPPFLAGS = $(AM_CPPFLAGS) $(SDL_CFLAGS)
lavplay_LDADD = $(LIBMJPEGUTILS) \
	liblavplay.la liblavjpeg.la liblavfile.la \
	@LIBGETOPT_LIB@ @PTHREAD_LIBS@ $(SDL_LIBS)

lavrec_SOURCES = lavrec.c
lavrec_LDADD = $(LIBMJPEGUTILS) \
	liblavrec.la liblavfile.la liblavjpeg.la \
	@LIBGETOPT_LIB@ @PTHREAD_LIBS@

lav2yuv_SOURCES = lav2yuv.c lav_common.c
lav2yuv_CPPFLAGS = $(AM_CPPFLAGS) $(LIBDV_CFLAGS)
lav2yuv_LDADD = $(LIBMJPEGUTILS) \
		liblavfile.la liblavjpeg.la

jpeg2yuv_SOURCES = jpeg2yuv.c
jpeg2yuv_CPPFLAGS = $(AM_CPPFLAGS) $(JPEG_CFLAGS)
jpeg2yuv_LDADD = $(LIBMJPEGUTILS) liblavjpeg.la $(JPEG_LIBS)

png2yuv_SOURCES = png2yuv.c colorspace.c subsample.c
png2yuv_CPPFLAGS = $(AM_CPPFLAGS) $(LIBPNG_CFLAGS)
png2yuv_LDADD = $(LIBMJPEGUTILS) $(LIBPNG_LIBS) -lz -lm

lavpipe_SOURCES = lavpipe.c pipelist.c
lavpipe_LDADD = $(LIBMJPEGUTILS)

lav2wav_SOURCES = lav2wav.c
lav2wav_LDADD = $(LIBMJPEGUTILS) liblavfile.la

lavtrans_SOURCES = lavtrans.c
lavtrans_LDADD = $(LIBMJPEGUTILS) liblavfile.la

lavaddwav_SOURCES = lavaddwav.c
lavaddwav_LDADD = $(LIBMJPEGUTILS) liblavfile.la

glav_SOURCES = glav.c glav_main.c
glav_CPPFLAGS = $(AM_CPPFLAGS) ${GTK_CFLAGS} -DG_LOG_DOMAIN=\"lavtools\"
glav_LDADD = $(LIBMJPEGUTILS) ${GTK_LIBS}

lavvideo_SOURCES = lavvideo.c frequencies.c

yuvplay_SOURCES = yuvplay.c
yuvplay_CPPFLAGS = $(AM_CPPFLAGS) $(SDL_CFLAGS)
yuvplay_LDADD = $(SDL_LIBS) $(LIBMJPEGUTILS)

ppmtoy4m_SOURCES = ppmtoy4m.c colorspace.c subsample.c
ppmtoy4m_LDADD = $(LIBMJPEGUTILS)

pnmtoy4m_SOURCES = pnmtoy4m.c colorspace.c
pnmtoy4m_LDADD = $(LIBMJPEGUTILS)

y4mtoppm_SOURCES = y4mtoppm.c colorspace.c subsample.c
y4mtoppm_LDADD = $(LIBMJPEGUTILS)

y4mtopnm_SOURCES = y4mtopnm.c colorspace.c
y4mtopnm_LDADD = $(LIBMJPEGUTILS)

y4mcolorbars_SOURCES = y4mcolorbars.c colorspace.c subsample.c
y4mcolorbars_LDADD = $(LIBMJPEGUTILS)

lavinfo_SOURCES = lavinfo.c
lavinfo_LDADD = $(LIBMJPEGUTILS) liblavfile.la

y4mstabilizer_SOURCES = y4mstabilizer.c subsample.c
y4mstabilizer_LDADD = $(LIBMJPEGUTILS)

yuvfps_SOURCES = yuvfps.c
yuvfps_LDADD = $(LIBMJPEGUTILS)

yuvinactive_SOURCES = yuvinactive.c
yuvinactive_LDADD = $(LIBMJPEGUTILS)
