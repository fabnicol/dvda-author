

SOURCES:=$(wildcard /home/fab/Desktop/dvda-author-dev/libfixwav/src/*.c)
OBJECTS:=$(subst /home/fab/Desktop/dvda-author-dev/libfixwav/src,/home/fab/Desktop/dvda-author-dev/libfixwav/src, $(SOURCES:.c=.o) )

CPPFLAGS += -I/home/fab/Desktop/dvda-author-dev/libfixwav/src/include -I/home/fab/Desktop/dvda-author-dev/libutils/src/include -I/home/fab/Desktop/dvda-author-dev/src/include 
#for config.h
CPPFLAGS += -I/home/fab/Desktop/dvda-author-dev


ifeq "" "yes"
	CPPFLAGS +=-I/home/fab/Desktop/dvda-author-dev/libiberty/src/include
	ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
	  CPPFLAGS +=-I/home/fab/Desktop/dvda-author-dev/libiberty/src/include
	endif
endif

libfixwav.a: libfixwav.a($(OBJECTS))

#implicit rules won't work here
/home/fab/Desktop/dvda-author-dev/libfixwav/src/%.o: /home/fab/Desktop/dvda-author-dev/libfixwav/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
