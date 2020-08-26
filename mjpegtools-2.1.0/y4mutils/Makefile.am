# Makefile for y4mutils

MAINTAINERCLEANFILES = Makefile.in

AM_CFLAGS   = @PROGRAM_NOPIC@
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/utils

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

bin_PROGRAMS = \
	yuvmedianfilter \
	pgmtoy4m \
	y4mshift \
	y4mspatialfilter \
	y4mhist \
	y4mblack \
	y4mtoyuv \
	y4minterlace \
	yuv4mpeg \
	y4mivtc \
	yuyvtoy4m

if HAVE_LIBQUICKTIME
bin_PROGRAMS += y4mtoqt qttoy4m
endif

y4mivtc_SOURCES = y4mivtc.c
y4mivtc_LDADD = $(LIBMJPEGUTILS)

yuvmedianfilter_SOURCES = yuvmedianfilter.c
yuvmedianfilter_LDADD = $(LIBMJPEGUTILS)

pgmtoy4m_SOURCES = pgmtoy4m.c
pgmtoy4m_LDADD = $(LIBMJPEGUTILS)

y4mshift_SOURCES = y4mshift.c
y4mshift_LDADD = $(LIBMJPEGUTILS)

y4mspatialfilter_SOURCES = y4mspatialfilter.c
y4mspatialfilter_LDADD = $(LIBMJPEGUTILS) $(LIBM_LIBS)

y4mhist_SOURCES = y4mhist.c
y4mhist_LDADD = $(SDL_LIBS) $(SDLgfx_LIBS) $(LIBMJPEGUTILS)

y4mblack_SOURCES = y4mblack.c
y4mblack_LDADD = $(LIBMJPEGUTILS)

y4minterlace_SOURCES = y4minterlace.c
y4minterlace_LDADD = $(LIBMJPEGUTILS)

if HAVE_LIBQUICKTIME
y4mtoqt_SOURCES = y4mtoqt.c
y4mtoqt_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/lavtools $(LIBQUICKTIME_CFLAGS)
y4mtoqt_LDADD = $(LIBMJPEGUTILS) $(top_builddir)/lavtools/liblavfile.la

qttoy4m_SOURCES = qttoy4m.c
qttoy4m_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/lavtools $(LIBQUICKTIME_CFLAGS)
qttoy4m_LDADD = $(LIBMJPEGUTILS) $(top_builddir)/lavtools/liblavfile.la
endif

y4mtoyuv_SOURCES = y4mtoyuv.c
y4mtoyuv_LDADD = $(LIBMJPEGUTILS)

yuv4mpeg_SOURCES = yuv4mpeg.c
yuv4mpeg_LDADD = $(LIBMJPEGUTILS)

yuyvtoy4m_SOURCES = yuyvtoy4m.c
yuyvtoy4m_LDADD = $(LIBMJPEGUTILS)
