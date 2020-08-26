# Makefile for yuvcorrect

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/utils

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

bin_PROGRAMS = yuvcorrect yuvcorrect_tune

noinst_HEADERS = yuvcorrect.h

yuvcorrect_SOURCES = yuvcorrect.c yuvcorrect_functions.c
yuvcorrect_LDADD = $(LIBMJPEGUTILS) $(LIBM_LIBS)

yuvcorrect_tune_SOURCES = yuvcorrect_tune.c yuvcorrect_functions.c
yuvcorrect_tune_LDADD = $(LIBMJPEGUTILS) $(LIBM_LIBS)
