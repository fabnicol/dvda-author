SUBDIRS=include
noinst_LIBRARIES=libfixwav.a

libfixwav_a_SOURCES=auxiliary.c  checkData.c  checkParameters.c   readHeader.c  repair.c  manager.c
AM_CPPFLAGS=-Wall  -std=c99 -O3 -I${top_srcdir}/libfixwav/src/include -I${top_srcdir}/libutils/src/include -I${top_srcdir}/src/include -I${top_srcdir}
AM_LDFLAGS=-s  -lm
LDADD=${top_builddir}/libutils/src/libc_utils.a

$(if $(iberty),
	LDADD       += $(iberty_LIB)
	AM_CPPFLAGS +=-I$(top_srcdir)/libiberty/src/include,
	if  HAVE_EXTERNAL_iberty
	  LDADD       += $(iberty_LIB)
	  AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
	endif
	echo Linking to local libiberty.a...)

if HAVE_iberty
     AM_LDFLAGS  += $(iberty_LINK)
endif



