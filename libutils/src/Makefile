
CPPFLAGS +=-I/cygdrive/c/Users/fabrn/dvda-author/libutils/src/include -I/cygdrive/c/Users/fabrn/dvda-author/libutils/src/private -I/cygdrive/c/Users/fabrn/dvda-author/src/include -I/cygdrive/c/Users/fabrn/dvda-author/libfixwav/src/include
#for config.h
CPPFLAGS += -I/cygdrive/c/Users/fabrn/dvda-author

ifeq "" "yes"
     LDLIBS   += /cygdrive/c/Users/fabrn/dvda-author/libiberty/src/libiberty.a
     CPPFLAGS +=-I/cygdrive/c/Users/fabrn/dvda-author/libiberty/src/include
     ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
       LDLIBS   += /cygdrive/c/Users/fabrn/dvda-author/libiberty/src/libiberty.a
       CPPFLAGS +=-I/cygdrive/c/Users/fabrn/dvda-author/libiberty/src/include
     endif
endif

ifeq "@HAVE_libiberty@" "yes"
      LDLIBS  += -liberty
endif

SOURCES:=$(wildcard /cygdrive/c/Users/fabrn/dvda-author/libutils/src/*.c)
OBJECTS:=$(SOURCES:.c=.o)

libc_utils.a: libc_utils.a($(OBJECTS))

$(OBJECTS):%.o: %.c 
