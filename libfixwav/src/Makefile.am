

SOURCES:=$(wildcard *.c)
OBJECTS:=$(SOURCES:.c=.o)
CPPFLAGS += -stdc99 -O3 -I@ROOTDIR@/libfixwav/src/include -I@ROOTDIR@/libutils/src/include -I@ROOTDIR@/src/include -I@ROOTDIR@
LDLIBS += @BUILDDIR@/libutils/src/libc_utils.a

ifeq "@HAVE_libiberty_BUILD@" "yes"
	LDADD    += @iberty_LIB@
	CPPFLAGS +=-I@ROOTDIR@/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  LDLIBS       += @iberty_LIB@
	  CPPFLAGS +=-I@ROOTDIR@/libiberty/src/include
	endif
	echo Linking to local libiberty.a...
endif
ifeq "@HAVE_iberty@" "yes"
     LDFLAGS  += @libiberty_LINK@
endif

libfixwav.a: libfixwav.a($(OBJECTS))

