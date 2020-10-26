

SOURCES:=$(wildcard /cygdrive/c/Users/fabrn/dvda-author/libfixwav/src/*.c)
OBJECTS:=$(subst /cygdrive/c/Users/fabrn/dvda-author/libfixwav/src,/cygdrive/c/Users/fabrn/dvda-author/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/cygdrive/c/Users/fabrn/dvda-author/libfixwav/src/include -I/cygdrive/c/Users/fabrn/dvda-author/libutils/src/include -I/cygdrive/c/Users/fabrn/dvda-author/src/include 
#for config.h
CPPFLAGS += -I/cygdrive/c/Users/fabrn/dvda-author


ifeq "" "yes"
	CPPFLAGS +=-I/cygdrive/c/Users/fabrn/dvda-author/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/cygdrive/c/Users/fabrn/dvda-author/libiberty/src/include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/cygdrive/c/Users/fabrn/dvda-author/libfixwav/src/%.o: /cygdrive/c/Users/fabrn/dvda-author/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) -g $(CPPFLAGS) $< -o $@
