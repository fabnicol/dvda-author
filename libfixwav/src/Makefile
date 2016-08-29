

SOURCES:=$(wildcard /mnt/fab/Dev/dvd-audio-dev/libfixwav/src/*.c)
OBJECTS:=$(subst /mnt/fab/Dev/dvd-audio-dev/libfixwav/src,/mnt/fab/Dev/dvd-audio-dev/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/mnt/fab/Dev/dvd-audio-dev/libfixwav/src/include -I/mnt/fab/Dev/dvd-audio-dev/libutils/src/include -I/mnt/fab/Dev/dvd-audio-dev/src/include 
#for config.h
CPPFLAGS += -I/mnt/fab/Dev/dvd-audio-dev


ifeq "" "yes"
	CPPFLAGS +=-I/mnt/fab/Dev/dvd-audio-dev/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/mnt/fab/Dev/dvd-audio-dev/libiberty/src/include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/mnt/fab/Dev/dvd-audio-dev/libfixwav/src/%.o: /mnt/fab/Dev/dvd-audio-dev/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
