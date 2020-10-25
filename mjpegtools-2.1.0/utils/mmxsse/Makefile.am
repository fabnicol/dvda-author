# Process this file with Automake to produce Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/utils

noinst_LTLIBRARIES = libmmxsse.la


libmmxsse_la_SOURCES = \
	build_sub22_mests.c \
	build_sub44_mests.c \
	find_best_one_pel.c \
	mblock_sad_mmx.c \
	mblock_sad_mmxe.c \
	mblock_sub44_sads_x86.c \
	mblock_sumsq_mmx.c \
	mblock_bsumsq_mmx.c \
	mblock_bsad_mmx.c \
	motion.c

noinst_HEADERS = \
	mblock_sub44_sads_x86.h \
	mmxsse_motion.h

EXTRA_DIST = \
	mblock_sub44_sads_x86_h.c 

MAINTAINERCLEANFILES = Makefile.in
