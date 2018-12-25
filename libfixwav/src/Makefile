

SOURCES:=$(wildcard /home/fab/Dev/dvda-author/libfixwav/src/*.c)
OBJECTS:=$(subst /home/fab/Dev/dvda-author/libfixwav/src,/home/fab/Dev/dvda-author/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/home/fab/Dev/dvda-author/libfixwav/src/include -I/home/fab/Dev/dvda-author/libutils/src/include -I/home/fab/Dev/dvda-author/src/include 
#for config.h
CPPFLAGS += -I/home/fab/Dev/dvda-author


ifeq "" "yes"
	CPPFLAGS +=-I/home/fab/Dev/dvda-author/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/home/fab/Dev/dvda-author/libiberty/src/include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/home/fab/Dev/dvda-author/libfixwav/src/%.o: /home/fab/Dev/dvda-author/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
