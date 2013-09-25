SUBDIRS=private include
noinst_LIBRARIES=libc_utils.a
AM_LDFLAGS=-s
libc_utils_a_SOURCES=c_utils.c  winport.c
AM_CPPFLAGS=-I${top_srcdir}/libutils/src/include -I${top_srcdir}/libutils/src/private -I${top_srcdir} -I${top_srcdir}/src/include

if HAVE_IBERTY_BUILD
     LDADD       = ${IBERTY_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
else
if  HAVE_EXTERNAL_IBERTY
     LDADD       = ${IBERTY_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
endif
if HAVE_IBERTY
     AM_LDFLAGS  += ${IBERTY_LINK}
endif
endif



