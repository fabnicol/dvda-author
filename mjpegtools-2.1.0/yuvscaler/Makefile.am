# Makefile for yuvscaler

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/utils


LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

bin_PROGRAMS = \
	yuvscaler

noinst_HEADERS = \
	yuvscaler.h

EXTRA_DIST = yuvscaler_implementation.txt

yuvscaler_CFLAGS=@PROGRAM_NOPIC@
yuvscaler_SOURCES = yuvscaler.c yuvscaler_resample.c yuvscaler_bicubic.c
yuvscaler_LDADD = $(LIBMJPEGUTILS) $(LIBM_LIBS)
