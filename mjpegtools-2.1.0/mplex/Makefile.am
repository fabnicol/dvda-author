# Process this file with Automake to produce Makefile.in

EXTRA_DIST = \
	README \
	INSTRUCT \
	mpeg_systems_paper_0.99.ps.gz \
	Doxyfile \
	TODO

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I $(top_srcdir) -I $(top_srcdir)/utils

bin_PROGRAMS = mplex

lib_LTLIBRARIES = libmplex2.la

libmplex2_la_SOURCES = \
	ac3strm_in.cpp \
	audiostrm_out.cpp \
	bits.cpp \
	decodebufmodel.cpp \
	dtsstrm_in.cpp \
	inputstrm.cpp \
	interact.cpp \
	lpcmstrm_in.cpp \
	mpastrm_in.cpp \
	multiplexor.cpp \
	padstrm.cpp \
	stillsstream.cpp \
	stream_params.cpp \
	systems.cpp \
	videostrm_in.cpp \
	videostrm_out.cpp \
	subpstream.cpp

libmplex_includedir = $(pkgincludedir)/mplex

libmplex_include_HEADERS = \
	audiostrm.hpp \
	aunit.hpp \
	aunitbuffer.hpp \
	bits.hpp \
	decodebufmodel.hpp \
	inputstrm.hpp \
	interact.hpp \
	mplexconsts.hpp \
	multiplexor.hpp \
	outputstrm.hpp \
	padstrm.hpp \
	stillsstream.hpp \
	stream_params.hpp \
	systems.hpp \
	videostrm.hpp

libmplex2_la_LDFLAGS =  \
	$(LT_STATIC) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) $(EXTRA_LDFLAGS)

libmplex2_la_LIBADD = \
	$(top_builddir)/utils/libmjpegutils.la

# Need to do this because of the way utils/altivec/* was done - it makes a
# reference to a function (next_larger_quant)  in mpeg2enc's library.  OSX
# doesn't care but Fedora4/PPC's linker gives an undefined symbol error
if HAVE_ALTIVEC
libmplex2_la_LIBADD += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

libmplex2_la_CXXFLAGS =

mplex_SOURCES = main.cpp 

mplex_DEPENDENCIES = libmplex2.la

mplex_LDADD = libmplex2.la @LIBGETOPT_LIB@ $(LIBM_LIBS)

