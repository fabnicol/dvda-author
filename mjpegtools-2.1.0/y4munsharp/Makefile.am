# Makefile.am for y4munsharp

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I $(top_srcdir)/utils -I $(top_srcdir)

y4munharp_CFLAGS=@PROGRAM_NOPIC@

bin_PROGRAMS = y4munsharp

y4munsharp_SOURCES = y4munsharp.c

y4munsharp_LDADD = \
	$(top_builddir)/utils/libmjpegutils.la \
	@LIBM_LIBS@

if HAVE_ALTIVEC
y4munsharp_LDADD += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif
