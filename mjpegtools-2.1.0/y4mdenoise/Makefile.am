# Makefile for y4mdenoise

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = implementation.html

AM_CFLAGS = -DNDEBUG -finline-functions @PROGRAM_NOPIC@
AM_CXXFLAGS = -DNDEBUG -finline-functions @PROGRAM_NOPIC@

INCLUDES =  -I$(top_srcdir) -I$(top_srcdir)/utils

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la
if HAVE_ALTIVEC
LIBMJPEGUTILS += $(top_builddir)/mpeg2enc/libmpeg2encpp.la
endif

bin_PROGRAMS = y4mdenoise

noinst_HEADERS = \
	Allocator.hh \
	BitmapRegion2D.hh \
	DoublyLinkedList.hh \
	Limits.hh \
	MotionSearcher.hh \
	newdenoise.hh \
	PlacementAllocator.hh\
	ReferenceFrame.hh \
	Region2D.hh \
	SearchBorder.hh \
	SearchWindow.hh \
	Set.hh \
	SetRegion2D.hh \
	SkipList.hh \
	Status_t.h \
	TemplateLib.hh \
	VariableSizeAllocator.hh \
	Vector.hh

noinst_PROGRAMS = regiontest

regiontest_SOURCES = regiontest.cc

y4mdenoise_SOURCES = main.c newdenoise.cc
y4mdenoise_LDADD = $(LIBMJPEGUTILS)
