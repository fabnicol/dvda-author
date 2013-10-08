SUBDIRS=private include
noinst_LIBRARIES=libc_utils.a
AM_LDFLAGS=-s
libc_utils_a_SOURCES=c_utils.c  winport.c
AM_CPPFLAGS=-I${top_srcdir}/libutils/src/include -I${top_srcdir}/libutils/src/private -I${top_srcdir} -I${top_srcdir}/src/include

virtual:
if HAVE_iberty_BUILD 
     LDADD       = ${iberty_LIB}
     AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include,
     if  HAVE_EXTERNAL_iberty
       LDADD       = ${iberty_LIB}
       AM_CPPFLAGS +=-I${top_srcdir}/libiberty/src/include
     endif
     if HAVE_iberty
      AM_LDFLAGS  += ${iberty_LINK}
     endif
     echo Linking to local libiberty.a
endif




