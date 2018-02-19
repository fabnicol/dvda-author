

SOURCES:=$(wildcard /home/fab/dvd-audio/libfixwav/src/*.c)
OBJECTS:=$(subst /home/fab/dvd-audio/libfixwav/src,/home/fab/dvd-audio/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/home/fab/dvd-audio/libfixwav/src/include -I/home/fab/dvd-audio/libutils/src/include -I/home/fab/dvd-audio/src/include 
#for config.h
CPPFLAGS += -I/home/fab/dvd-audio


ifeq "" "yes"
	CPPFLAGS +=-I/home/fab/dvd-audio/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/home/fab/dvd-audio/libiberty/src/include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/home/fab/dvd-audio/libfixwav/src/%.o: /home/fab/dvd-audio/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
