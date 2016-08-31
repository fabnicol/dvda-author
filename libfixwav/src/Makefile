

SOURCES:=$(wildcard /home/fab/Dev/dvda-author-dev/libfixwav/src/*.c)
OBJECTS:=$(subst /home/fab/Dev/dvda-author-dev/libfixwav/src,/home/fab/Dev/dvda-author-dev/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/home/fab/Dev/dvda-author-dev/libfixwav/src/include -I/home/fab/Dev/dvda-author-dev/libutils/src/include -I/home/fab/Dev/dvda-author-dev/src/include 
#for config.h
CPPFLAGS += -I/home/fab/Dev/dvda-author-dev


ifeq "" "yes"
	CPPFLAGS +=-I/home/fab/Dev/dvda-author-dev//include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/home/fab/Dev/dvda-author-dev//include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/home/fab/Dev/dvda-author-dev/libfixwav/src/%.o: /home/fab/Dev/dvda-author-dev/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
