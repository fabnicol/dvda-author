SUBDIRS=include
CAT=/bin/cat

## src/ output executable will be 'dvda'

## libc_utils is always rebuilt, libFLAC and libfixwav are shipped with package for i386*-*[cygwin linux] distributions
## to avoid conflicts, we take care to add either -l to LDFLAGS links of libs to LDADD, except for static build.
## if giving FLAC or Ogg as input, do also give the other one for compatibility reasons.
## Definitions: WITH_LIB: activates this capability
#               HAVE_EXTERNAL_LIB: use specified lib, e.g. --with-lib=/full/path/to/lib
##              HAVE_LIB: LIB has been tested to be system-installed, so linking should be OK (in principle)


#Interpret: HAVE_X: have X on system
#           HAVE_EXTERNAL_X: given on commandline --with-X=...
#           $(if $(X),...)   or:   ifeq "$(X)" "X": if build package with basename X (--enable-X-build), then do...
#           WITH_X: X-related code was not filtered out by conditional compilation (e.g. WITHOUT-SOX)

.ONESHELL:
SHELL=/bin/sh
SHELLFLAGS=-ec

include /home/fab/Dev/dvda-author-dev/mk/FLAC.global.mk
include /home/fab/Dev/dvda-author-dev/mk/sox.global.mk
include /home/fab/Dev/dvda-author-dev/mk/libogg.global.mk

DEBUG_FLAGS ?= 

CFLAGS += -D_GNU_SOURCE
LDFLAGS :=-L/usr/local/lib -L/usr/lib -Xlinker --allow-multiple-definition 

ifeq "$(DEBUG_FLAGS)" ""
   LDFLAGS += -s
endif

CPPFLAGS :=-I/home/fab/Dev/dvda-author-dev/libutils/src/include -I/home/fab/Dev/dvda-author-dev/src/include
# for config.h
CPPFLAGS += -I/home/fab/Dev/dvda-author-dev
LDLIBS := -lm /home/fab/Dev/dvda-author-dev/libutils/src/libc_utils.a /home/fab/Dev/dvda-author-dev/libfixwav/src/libfixwav.a

OBJECTS=amg2.o ats.o atsi2.o audio.o auxiliary.o dvda-author.o  \
	     file_input_parsing.o samg2.o launch_manager.o command_line_parsing.o  \
	     lexer.o ats2wav.o

ifneq "" "yes"
   OBJECTS += menu.o asvs.o xml.o sound.o videoimport.o
endif

ifneq "" "no"
    OBJECTS += libsoxconvert.o
else
    CPPFLAGS += -DWITHOUT_sox
endif

CPPFLAGS +=-I/home/fab/Dev/dvda-author-dev/libfixwav/src/include

ifneq "" ""
     LDLIBS+=
     CPPFLAGS +=-I/home/fab/Dev/dvda-author-dev//include
endif

ifeq "$(sox_BUILD)" "yes"
     LDLIBS  += 
endif

STATIC_LIBS=FLAC libogg sox

.PHONY: create_options all install clean

all:    dvda-author
	@mv -f dvda-author $(PROGRAM)
	echo [dvda-author] >> /home/fab/Dev/dvda-author-dev/depconf/BUILD.TRACE
	echo "     Build successful." >> /home/fab/Dev/dvda-author-dev/depconf/BUILD.TRACE
	echo
	echo ----------------------------------------------- 
	echo Finished building $(PROGRAM)
	echo Now run 'make install'
	echo Man page will be in directory /usr/share/man/man1
	echo Html page help and other files in $(DESTDIR)/usr/share/info/$(PROGRAM)
	echo -----------------------------------------------
	echo

	
CPPFLAGS += $(foreach prog,$(STATIC_LIBS),$(if $(MAYBE_$(prog)),-I/home/fab/Dev/dvda-author-dev/$(MAYBE_$(prog))/src))

CPPFLAGS += $(foreach prog,$(STATIC_LIBS),$$(if test "$($(prog)_LIB)" = "" -a "$($(prog)_LINK)" = ""; then echo -DWITHOUT_$(prog); fi))

$(OBJECTS): %.o: /home/fab/Dev/dvda-author-dev/src/%.c
	$(CC)  -c -DHAVE_CONFIG_H $(CFLAGS) $(CPPFLAGS)  $< -o $@ 

LDLIBS_append += $(foreach prog,$(STATIC_LIBS),$($(prog)_LIB) $($(prog)_LINK))  

LDLIBS += $(LDLIBS_append) 

dvda-author: $(OBJECTS) $(LDLIBS_append)

install: $(PROGRAM)
	$(INSTALL) $(PROGRAM) $(DESTDIR)/usr/bin

clean: 
	$(RM) $(wildcard *.o) dvda-author-dev dvda-author

# Symbol  will be replaced by sox dependencies after configure in libsox by target do_sox_lib_deps_subst
# left-align if conditions

