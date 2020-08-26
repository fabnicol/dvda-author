dnl Process this file with autoconf to produce a configure script.

# Copyright (C) 2012 Free Software Foundation, Inc.

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

dnl Written by Brendan O'Dea <bod@debian.org>

AC_INIT([GNU help2man], m4_esyscmd_s([./help2man.PL --version]),
	[bug-help2man@gnu.org])

AC_CONFIG_SRCDIR([help2man.PL])

AC_PROG_PERL(5.008)
test -z "$PERL" && AC_MSG_ERROR([perl 5.8 required])

AC_PERL_MODULE(Locale::gettext)
AC_PATH_PROG(MSGFMT, msgfmt)

AC_PROG_CC
AC_SEARCH_LIBS(dlsym, dl)
AC_SEARCH_LIBS(bindtextdomain, intl)

nls_default=yes
test "x$ac_cv_module_Locale__gettext" = xno && nls_default=no
test -z "$MSGFMT" && nls_default=no
test "x$ac_cv_search_dlsym" = xno && nls_default=no
test "x$ac_cv_search_bindtextdomain" = xno && nls_default=no

AC_ARG_ENABLE([nls], AC_HELP_STRING([--enable-nls],
	[enable support for generating localised pages]),
	[ac_cv_enable_nls=$enableval], [ac_cv_enable_nls=$nls_default])

AC_SUBST(extra_make_all, '')
AC_SUBST(extra_make_install, '')
AC_SUBST(extra_extract_args, '')
if test "$ac_cv_enable_nls" = yes
then
    # note: the following tests should match what is used to set nls_default above
    test "x$ac_cv_module_Locale__gettext" = xno &&
	AC_MSG_ERROR([perl module Locale::gettext required])

    test -z "$MSGFMT" && AC_MSG_ERROR([gettext required])

    test "x$ac_cv_search_dlsym" = xno &&
	AC_MSG_ERROR([dlsym() required])

    test "x$ac_cv_search_bindtextdomain" = xno &&
	AC_MSG_ERROR([bindtextdomain() required])

    extra_make_all='preload man_l10n'
    extra_make_install='install_preload install_l10n'
    extra_extract_args='--with-gettext'
fi

AC_PROG_INSTALL
AC_PATH_PROG(MAKEINFO, makeinfo, :)
AC_PATH_PROG(INSTALL_INFO, install-info, :)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
