              
AC_DEFUN([DISABLE_OGG_TEST],[ AS_IF([test x$OGG_BUILD = xno || test x$OGG_LINK = x], [FLAC_FLAGS="$FLAC_FLAGS --disable-ogg"])])
           