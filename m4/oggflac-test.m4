
AC_DEFUN([DISABLE_LIBOGG_TEST],[ AS_IF([test x$LIBOGG_BUILD = xno -a  x$LIBOGG_LINK = x], [FLAC_FLAGS="$FLAC_FLAGS --disable-ogg"])])
