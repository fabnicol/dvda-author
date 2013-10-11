SUBDIRS=include
noinst_LIBRARIES=libiberty.a
libiberty_a_SOURCES=getsubopt.c strdup.c strndup.c strchrnul.c  strnlen.c malloc.c
AM_CPPFLAGS=-I${top_srcdir}/libiberty/src/include
if WITH_sox
libiberty_a_SOURCES += getopt.c getopt1.c
endif


