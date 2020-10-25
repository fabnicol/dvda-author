
CPPFLAGS +=-I/home/fab/dvda-author/libutils/src/include -I/home/fab/dvda-author/libutils/src/private -I/home/fab/dvda-author/src/include -I/home/fab/dvda-author/libfixwav/src/include
#for config.h
CPPFLAGS += -I/home/fab/dvda-author

ifeq "" "yes"
     LDLIBS   += /home/fab/dvda-author/libiberty/src/libiberty.a
     CPPFLAGS +=-I/home/fab/dvda-author/libiberty/src/include
     ifeq  "@HAVE_EXTERNAL_iberty@" "yes"
       LDLIBS   += /home/fab/dvda-author/libiberty/src/libiberty.a
       CPPFLAGS +=-I/home/fab/dvda-author/libiberty/src/include
     endif
endif

ifeq "@HAVE_libiberty@" "yes"
      LDLIBS  += -liberty
endif

SOURCES:=$(wildcard /home/fab/dvda-author/libutils/src/*.c)
OBJECTS:=$(SOURCES:.c=.o)

libc_utils.a: libc_utils.a($(OBJECTS))

$(OBJECTS):%.o: %.c 
