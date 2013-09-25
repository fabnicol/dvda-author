SUBDIRS=include
noinst_LIBRARIES=libats2wav.a
libats2wav_a_SOURCES=libats2wav.c
AM_CPPFLAGS= -std=c99 -I${top_srcdir}/src/include -I${top_srcdir}/libutils/src/include -I${top_srcdir}/libfixwav/src/include -I${top_srcdir}/libats2wav/src/include
LDADD=${top_builddir}/libutils/src/libc_utils.a

if HAVE_IBERTY_BUILD
     LDADD       += ${IBERTY_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
else
if  HAVE_EXTERNAL_IBERTY
     LDADD       += ${IBERTY_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
endif
if HAVE_IBERTY
     AM_LDFLAGS  = ${IBERTY_LINK}
endif
endif
