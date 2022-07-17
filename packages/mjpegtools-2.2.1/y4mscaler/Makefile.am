# Makefile for y4mscaler

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = ChangeLog README TODO

AM_CFLAGS = -DNDEBUG -DYS_VERSION_MAJOR=9 -DYS_VERSION_MINOR=0
AM_CXXFLAGS = -DNDEBUG -DYS_VERSION_MAJOR=9 -DYS_VERSION_MINOR=0

AM_CPPFLAGS =  -I$(top_srcdir) -I$(top_srcdir)/utils

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

bin_PROGRAMS = y4mscaler

noinst_HEADERS = \
	debug.h y4m-config.h \
	graphics.H kernels.H scaler-matto.H scaler.H ysScaling.H \
	ysSource.H ysStreamInfo.H ysTarget.H

y4mscaler_SOURCES = graphics.C kernels.C scaler-matto.C y4mscaler.C \
	ysScaling.C ysSource.C ysStreamInfo.C ysTarget.C
y4mscaler_LDADD = $(LIBMJPEGUTILS)
