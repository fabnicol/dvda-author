EXTRA_DIST = README.2-3pulldown

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = yuvycsnoise yuvkineco

noinst_LTLIBRARIES = libyuvfilters.la

AM_CPPFLAGS = -I$(top_srcdir)/utils
# needed for yuvkineco.c - removing the abs() calls breaks the program
AM_CFLAGS = -Wno-absolute-value
MJPEGLIB = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
MJPEGLIB += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

libyuvfilters_la_SOURCES = \
	addtask.c \
	alloctask.c \
	initframe.c \
	putframe.c \
	yuvstdin.c \
	yuvstdout.c

noinst_HEADERS = \
	yuvfilters.h

yuvkineco_SOURCES = yuvkineco.c
yuvkineco_LDADD = libyuvfilters.la $(MJPEGLIB)

yuvycsnoise_SOURCES = yuvycsnoise.c
yuvycsnoise_LDADD = libyuvfilters.la $(MJPEGLIB)
