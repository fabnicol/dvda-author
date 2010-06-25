SUBDIRS=include 
noinst_LIBRARIES=libats2wav.a
libats2wav_a_SOURCES=libats2wav.c
AM_CPPFLAGS= -std=c99 -I${top_srcdir}/src/include -I${top_srcdir}/libutils/src/include -I${top_srcdir}/libfixwav/src/include -I${top_srcdir}/libats2wav/src/include
AM_LDADD=${top_builddir}/libutils/src/libc_utils.a