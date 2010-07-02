SUBDIRS=include
noinst_LIBRARIES=libfixwav.a

libfixwav_a_SOURCES=auxiliary.c  checkData.c  checkParameters.c   readHeader.c  repair.c  manager.c
AM_CPPFLAGS=-Wall  -std=c99 -O3 -I${top_srcdir}/libfixwav/src/include -I${top_srcdir}/libutils/src/include -I${top_srcdir}/src/include -I${top_srcdir}
AM_LDFLAGS=-s  -lm
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
     AM_LDFLAGS  += ${IBERTY_LINK}
endif
endif



