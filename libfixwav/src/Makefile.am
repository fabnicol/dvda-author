SUBDIRS=include
noinst_LIBRARIES=libfixwav.a

libfixwav_a_SOURCES=auxiliary.c  checkData.c  checkParameters.c   readHeader.c  repair.c  manager.c 
libfixwav_a_CPPFLAGS=-Wall  -std=c99 -O3 -I${top_srcdir}/libfixwav/src/include -I${top_srcdir}/libutils/src/include -I${top_srcdir}/src/include -I${top_srcdir}
libfixwav_a_LDFLAGS=-s  -lm
libfixwav_a_LIBADD=${top_builddir}/libutils/src/libc_utils.a 


