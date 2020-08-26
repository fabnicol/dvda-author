#
# Automake Makefile template for mpeg2enc++ library 
# and mpeg2enc command line wrapper.
#

bin_PROGRAMS = mpeg2enc 

mpeg2encpp_MMXSSE_INLINE = \
	fdct_x86.c \
	fdct_mmx.c \
	idct_mmx.c \
	quant_mmx.c \
	predict_mmx.c \
	predcomp_mmx.c \
	predcomp_mmxe.c \
	predict_x86.c \
	quantize_x86.c \
	transfrm_x86.c

EXTRA_DIST = NOTES README TODO INSTALL ChangeLog seqstats.cc seqstats.hh

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/utils

LIBMJPEGUTILS = $(top_builddir)/utils/libmjpegutils.la

lib_LTLIBRARIES = libmpeg2encpp.la

if HAVE_ASM_MMX
SIMD_INLINE = $(mpeg2encpp_MMXSSE_INLINE)
endif # HAVE_ASM_MMX

# Reference implementations of lowlevel routines that may have Architecture
# dependent (usually SIMD) implementations

mpeg2enc_REF = \
	fdct.c \
	idct.c \
	predict_ref.c \
	quantize_ref.c \
	transfrm_ref.c

mpeg2enc_noinst_header_REF = \
	transfrm_ref.h

mpeg2enc_inst_header_REF = \
	predict_ref.h

mpeg2enc_SOURCES = mpeg2enc.cc

libmpeg2encpp_la_SOURCES = conform.cc elemstrmwriter.cc encoderparams.cc \
		macroblock.cc motionest.cc mpeg2coder.cc mpeg2encoptions.cc \
		imageplanes.cc mpeg2encoder.cc \
		picture.cc picturereader.cc predict.cc putpic.cc \
		streamstate.cc seqencoder.cc \
		quantize.cc ratectl.cc stats.cc synchrolib.cc tables.c \
		transfrm.cc $(mpeg2enc_REF) \
		$(SIMD_INLINE) ontheflyratectlpass1.cc ontheflyratectlpass2.cc \
	rate_complexity_model.cc

noinst_HEADERS = channel.hh quantize_precomp.h simd.h \
	tables.h $(mpeg2enc_noinst_header_REF) rate_complexity_model.hh

libmpeg2encpp_includedir = $(pkgincludedir)/mpeg2enc

libmpeg2encpp_include_HEADERS = elemstrmwriter.hh encoderparams.hh \
	encodertypes.h macroblock.hh mpeg2coder.hh mpeg2encoder.hh mpeg2encoptions.hh \
	mpeg2encparams.h picture.hh picturereader.hh quantize.hh quantize_ref.h ratectl.hh \
	streamstate.h seqencoder.hh synchrolib.h syntaxconsts.h $(mpeg2enc_inst_header_REF) \
	ontheflyratectlpass1.hh ontheflyratectlpass2.hh \
	mpeg2syntaxcodes.h imageplanes.hh

libmpeg2encpp_la_LDFLAGS = \
	${LT_STATIC} \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) ${EXTRA_LDFLAGS}

libmpeg2encpp_la_DEPENDENCIES = $(LIBMJPEGUTILS)

libmpeg2encpp_la_LIBADD = $(LIBMJPEGUTILS) @PTHREAD_LIBS@

mpeg2enc_DEPENDENCIES = \
	$(LIBMJPEGUTILS) \
	libmpeg2encpp.la

mpeg2enc_LDADD = \
	libmpeg2encpp.la \
	$(LIBMJPEGUTILS) \
	@PTHREAD_LIBS@ @LIBGETOPT_LIB@ $(LIBM_LIBS)
