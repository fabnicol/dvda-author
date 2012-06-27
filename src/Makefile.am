SUBDIRS=include
CAT=/bin/cat

## src/ output executable will be 'dvda'

## libc_utils is always rebuilt, libFLAC and libfixwav are shipped with package for i386*-*[cygwin linux] distributions
## to avoid conflicts, we take care to add either -l to LDFLAGS links of libs to LDADD, except for static build.
## if giving FLAC or Ogg as input, do also give the other one for compatibility reasons.
## Definitions: WITH_LIB: activates this capability
##              HAVE_LIB_BUILD: recurses subdirs to build capability locally
#               HAVE_EXTERNAL_LIB: use specified lib, e.g. --with-lib=/full/path/to/lib
##              HAVE_LIB: LIB has been tested to be system-installed, so linking should be OK (in principle)



bin_PROGRAMS=dvda

dvda_SOURCES=amg2.c ats.c atsi2.c audio.c auxiliary.c dvda-author2.c  \
             file_input_parsing.c samg2.c launch_manager.c command_line_parsing.c  \
             lexer.c 

if !HAVE_CORE_BUILD
dvda_SOURCES += menu.c asvs.c xml.c sound.c videoimport.c
endif


AM_LDFLAGS=-L/usr/local/lib -L/usr/lib -Xlinker --allow-multiple-definition
AM_CPPFLAGS=-I${top_srcdir}/libutils/src/include -I${top_srcdir}/src/include -I${top_srcdir}/libats2wav/src/include -I${top_srcdir}   ${DEBUG_FLAGS}

dvda_LDADD= -lm ${top_builddir}/libutils/src/libc_utils.a  ${top_builddir}/libats2wav/src/libats2wav.a


# Symbol SOX_LIB_DEPS will be replaced by sox dependencies after configure in libsox by target do_sox_lib_deps_subst
# left-align if conditions


if WITH_FLAC
if HAVE_EXTERNAL_FLAC
    dvda_LDADD  += ${FLAC_LIB}
endif
if HAVE_EXTERNAL_OGG
    dvda_LDADD  +=  ${OGG_LIB}
endif
if HAVE_FLAC_BUILD
    dvda_LDADD   +=  ${FLAC_LIB}
endif
if HAVE_OGG_BUILD
    dvda_LDADD   +=  ${OGG_LIB}
endif
if HAVE_FLAC
    dvda_LDADD  += ${FLAC_LINK} ${OGG_LINK}
endif
endif

if WITH_SOX
    dvda_SOURCES += libsoxconvert.c

if HAVE_SOX_BUILD
    AM_CPPFLAGS  += -I${top_srcdir}/${MAYBE_SOX}/src
    LIBS   += SOX_LIB_DEPS
    dvda_LDADD   += ${SOX_LIB}

else

if HAVE_EXTERNAL_SOX
    dvda_LDADD   += -ldl
    dvda_LDADD   += ${SOX_LIB}
endif
if HAVE_SOX
    LIBS   += ${SOX_LINK}
endif
endif

endif

if HAVE_IBERTY_BUILD
     dvda_LDADD+= ${IBERTY_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
else
if  HAVE_EXTERNAL_IBERTY
     dvda_LDADD+= ${IBERTY_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
endif
if HAVE_IBERTY
     LIBS+= ${IBERTY_LINK}
endif
endif

if WITH_FIXWAV
     dvda_LDADD+=${FIXWAV_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libfixwav/src/include
endif

## canonical names cannot contain hyphens, so using program_transform_name
## remark: AC_ARG_PROGRAM was tested useless here

program_transform_name='s/dvda/dvda-author/g'
