
AC_DEFUN([DISABLE_LIBOGG_TEST],[ AS_IF([test x$LIBOGG_BUILD = xno || test x$LIBOGG_LINK = x], [FLAC_FLAGS="$FLAC_FLAGS --disable-libogg"])])
