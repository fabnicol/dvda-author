

SOURCES:=$(wildcard /c/Users/Public/Dev/dvd-audio-dev/libfixwav/src/*.c)
OBJECTS:=$(subst /c/Users/Public/Dev/dvd-audio-dev/libfixwav/src,/c/Users/Public/Dev/dvd-audio-dev/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/c/Users/Public/Dev/dvd-audio-dev/libfixwav/src/include -I/c/Users/Public/Dev/dvd-audio-dev/libutils/src/include -I/c/Users/Public/Dev/dvd-audio-dev/src/include 
#for config.h
CPPFLAGS += -I/c/Users/Public/Dev/dvd-audio-dev


ifeq "" "yes"
	CPPFLAGS +=-I/c/Users/Public/Dev/dvd-audio-dev//include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/c/Users/Public/Dev/dvd-audio-dev//include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/c/Users/Public/Dev/dvd-audio-dev/libfixwav/src/%.o: /c/Users/Public/Dev/dvd-audio-dev/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
