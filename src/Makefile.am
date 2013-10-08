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

bin_PROGRAMS=dvda

dvda_SOURCES=amg2.c ats.c atsi2.c audio.c auxiliary.c dvda-author2.c  \
	     file_input_parsing.c samg2.c launch_manager.c command_line_parsing.c  \
	     lexer.c ats2wav.c

if HAVE_core_BUILD
   dvda_SOURCES += menu.c asvs.c xml.c sound.c videoimport.c
endif

AM_LDFLAGS=-L/usr/local/lib -L/usr/lib -Xlinker --allow-multiple-definition -L${top_builddir}/libs
AM_CPPFLAGS=-I${top_srcdir}/libutils/src/include -I${top_srcdir}/src/include -I${top_srcdir}   ${DEBUG_FLAGS}

dvda_LDADD= -lm ${top_builddir}/libutils/src/libc_utils.a

if WITH_sox
    dvda_SOURCES += libsoxconvert.c
endif

if WITH_fixwav
     dvda_LDADD+=$(fixwav_LIB)
     AM_CPPFLAGS +=-I${top_srcdir}/libfixwav/src/include
endif


# Symbol SOX_LIB_DEPS will be replaced by sox dependencies after configure in libsox by target do_sox_lib_deps_subst
# left-align if conditions

$(foreach prog,$(PROGRAM_TARGETS),
	$(if $(prog),
	    $(if $($(prog)_LIB),
	      dvda_LDADD += $($(prog)_LIB)
	      AM_CPPFLAGS  += -I$(top_srcdir)/$(MAYBE_$(prog))/src ),
	    if HAVE_EXTERNAL_$(prog)
	      $(if $($(prog)_LIB),
		dvda_LDADD   += $($(prog)_LIB)
		echo Linking to system lib $($(prog)_LIB))
	    else
	      if HAVE_$(prog)
		$(if $($(prog)_LINK),
		LIBS   += $($(prog)_LINK)
		echo Linking to user lib $(($prog)_LINK)a)
	      else
		echo No linking to $(prog)
	      endif
	    endif
	 ))

$(if $(sox), LIBS   += SOX_LIB_DEPS)

## canonical names cannot contain hyphens, so using program_transform_name

program_transform_name='s/dvda/dvda-author/g'
