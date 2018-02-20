

SOURCES:=$(wildcard /home/fab/Desktop/dvd-audio/libfixwav/src/*.c)
OBJECTS:=$(subst /home/fab/Desktop/dvd-audio/libfixwav/src,/home/fab/Desktop/dvd-audio/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/home/fab/Desktop/dvd-audio/libfixwav/src/include -I/home/fab/Desktop/dvd-audio/libutils/src/include -I/home/fab/Desktop/dvd-audio/src/include 
#for config.h
CPPFLAGS += -I/home/fab/Desktop/dvd-audio


ifeq "" "yes"
	CPPFLAGS +=-I/home/fab/Desktop/dvd-audio/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/home/fab/Desktop/dvd-audio/libiberty/src/include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/home/fab/Desktop/dvd-audio/libfixwav/src/%.o: /home/fab/Desktop/dvd-audio/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
