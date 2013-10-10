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

bin_PROGRAM=dvda_author

OBJECTS=amg2.o ats.o atsi2.o audio.o auxiliary.o dvda-author2.o  \
	     file_input_parsing.o samg2.o launch_manager.o command_line_parsing.o  \
	     lexer.o ats2wav.o

if @HAVE_core_BUILD@
   dvda_OBJECTS += menu.o asvs.o xml.o sound.o videoimport.o
endif

if @WITH_sox@
    dvda_OBJECTS += libsoxconvert.o
endif

if @WITH_fixwav@
     LDADD+=$(fixwav_LIB)
     CPPFLAGS +=-I${top_srcdir}/libfixwav/src/include
endif

LDFLAGS=-L/usr/local/lib -L/usr/lib -Xlinker --allow-multiple-definition -L${top_builddir}/libs
CPPFLAGS=-I${top_srcdir}/libutils/src/include -I${top_srcdir}/src/include -I${top_srcdir}   ${DEBUG_FLAGS}
LDADD= -lm ${top_builddir}/libutils/src/libc_utils.a

$(if $(sox), LIBS   += SOX_LIB_DEPS)

create_options:
	$(foreach prog,@PROGRAM_TARGETS@,
	   $(if $($(prog)_LIB),
	      LDADD += $($(prog)_LIB)
	      CPPFLAGS += -I$(top_srcdir)/$(MAYBE_$(prog))/src,
		 if test "$(HAVE_EXTERNAL_$(prog))" = "yes"
		    $(if $($(prog)_LIB),
		       LDADD   += $($(prog)_LIB)
		       echo Linking to system lib $($(prog)_LIB))
		 else
		     if test $(HAVE_$(prog))
			$(if $($(prog)_LINK),
			LDFLAGS   += $($(prog)_LINK)
			echo Linking to user lib $(($prog)_LINK))
		     else
			echo No linking to $(prog)
		     fi
		fi)


.PHONY: program_transform_name create_options

all:  create_options $(bin_PROGRAM) program_transform_name

$(bin_PROGRAM): $(OBJECTS)

program_transform_name: $(bin_PROGRAM)
	mv -f $(bin_PROGRAM) @PACKAGE_NAME@-@VERSION@

# Symbol SOX_LIB_DEPS will be replaced by sox dependencies after configure in libsox by target do_sox_lib_deps_subst
# left-align if conditions

