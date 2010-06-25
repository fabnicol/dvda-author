SUBDIRS=include
noinst_LIBRARIES=libglibc.a
libglibc_a_SOURCES=getsubopt.c strdup.c strndup.c strchrnul.c  strnlen.c malloc.c  
AM_CPPFLAGS=-I${top_srcdir}/libglibc/src/include
if WITH_SOX
libglibc_a_SOURCES += getopt.c getopt1.c
endif


