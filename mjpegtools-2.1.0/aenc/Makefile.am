# Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I $(top_srcdir) -I $(top_srcdir)/utils

bin_PROGRAMS = mp2enc

mp2enc_SOURCES = \
	common.c \
	encode.c \
	fft.c \
	musicin.c \
	psy.c \
	tables.c \
	tonal.c \
	wav_io.c

noinst_HEADERS = \
	common.h \
	encoder.h \
	table_absthr.h \
	table_alloc.h \
	table_cb.h \
	table_enwindow.h \
	table_th.h \
	wav_io.h

mp2enc_LDADD = $(top_builddir)/utils/libmjpegutils.la $(LIBM_LIBS)
if HAVE_ALTIVEC
mp2enc_LDADD += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif
