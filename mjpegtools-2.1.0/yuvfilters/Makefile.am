EXTRA_DIST = README.2-3pulldown

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = yuvycsnoise yuvkineco

noinst_LTLIBRARIES = libyuvfilters.la

AM_CPPFLAGS = -I$(top_srcdir)/utils
MJPEGLIB = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
MJPEGLIB += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

libyuvfilters_la_SOURCES = \
	addtask.c \
	alloctask.c \
	initframe.c \
	putframe.c \
	yuvkineco.c \
	yuvstdin.c \
	yuvstdout.c \
	yuvycsnoise.c

noinst_HEADERS = \
	yuvfilters.h

yuvkineco_SOURCES = main.c
yuvkineco_CFLAGS = -DFILTER=yuvkineco
yuvkineco_LDADD = libyuvfilters.la $(MJPEGLIB)

yuvycsnoise_SOURCES = main.c
yuvycsnoise_CFLAGS = -DFILTER=yuvycsnoise
yuvycsnoise_LDADD = libyuvfilters.la $(MJPEGLIB)
