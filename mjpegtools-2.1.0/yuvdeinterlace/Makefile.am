# Makefile.am for yuvdeinterlace

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/utils

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

bin_PROGRAMS = yuvdeinterlace

noinst_HEADERS = 
    
yuvdeinterlace_SOURCES = yuvdeinterlace.cc

yuvdeinterlace_LDADD = $(LIBMJPEGUTILS)
