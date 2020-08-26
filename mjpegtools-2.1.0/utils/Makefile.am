# Process this file with Automake to produce Makefile.in

if HAVE_ALTIVEC
SUBDIRS = altivec
endif
if HAVE_ASM_MMX
SUBDIRS = mmxsse
endif

DIST_SUBDIRS = altivec mmxsse

EXTRA_DIST = cpuinfo.c

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/utils

lib_LTLIBRARIES = libmjpegutils.la

if HAVE_ALTIVEC
altivec_lib = $(top_builddir)/utils/altivec/libaltivec.la
endif
if HAVE_ASM_MMX
mmxsse_lib = $(top_builddir)/utils/mmxsse/libmmxsse.la
endif

libmjpegutils_la_LIBADD = $(mmxsse_lib) $(altivec_lib)

libmjpegutils_la_LDFLAGS = \
	$(LT_STATIC) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) $(EXTRA_LDFLAGS)

libmjpegutils_la_SOURCES = \
	mjpeg_logging.c \
	mpegconsts.c \
	mpegtimecode.c \
	yuv4mpeg.c \
	yuv4mpeg_ratio.c \
	motionsearch.c \
	cpu_accel.c

noinst_HEADERS = \
	cpu_accel.h \
	fastintfns.h \
	mmx.h \
	videodev_mjpeg.h \
	yuv4mpeg_intern.h

pkginclude_HEADERS = \
	format_codes.h \
	mjpeg_logging.h \
	mjpeg_types.h \
	mpegconsts.h \
	mpegtimecode.h \
	motionsearch.h \
	yuv4mpeg.h

MAINTAINERCLEANFILES = Makefile.in
