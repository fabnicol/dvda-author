# Process this file with Automake to produce Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/utils

LTCOMPILE = $(LIBTOOL) --mode=compile \
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)

noinst_LTLIBRARIES = libaltivec.la

libaltivec_la_SOURCES = \
	add_pred.c \
	amber.c \
	benchmark.c \
	bsad.c \
	bsumsq.c \
	bsumsq_sub22.c \
	build_sub22_mests.c \
	build_sub44_mests.c \
	detect.c \
	fdct.c \
	fdct_idct.c \
	field_dct_best.c \
	find_best_one_pel.c \
	idct.c \
	iquant_intra.c \
	iquant_non_intra.c \
	motion.c \
	pred_comp.c \
	quant_non_intra.c \
	quant_weight_coeff_sum.c \
	quantize.c \
	sad_00.c \
	sad_01.c \
	sad_10.c \
	sad_11.c \
	sub_mean_reduction.c \
	sub_mean_reduction_ppc.S \
	sub_pred.c \
	subsample_image.c \
	sumsq.c \
	sumsq_sub22.c \
	variance.c

noinst_HEADERS = \
	altivec_conf.h \
	altivec_motion.h \
	altivec_predict.h \
	altivec_quantize.h \
	altivec_transform.h \
	amber.h \
	benchmark.h \
	vectorize.h \
	verify.h

MAINTAINERCLEANFILES = Makefile.in
