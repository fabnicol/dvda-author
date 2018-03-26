# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([lplex],[0.3],[audioplex-lpcm@lists.sourceforge.net])
AC_CONFIG_SRCDIR([src/lplex.cpp])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT
#AC_SYS_LARGEFILE
#AC_DISABLE_SHARED

AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([atexit memset putenv strrchr strstr])

# --------------------------------------------------------------------------
# --------------------------------------------------------------------------

AC_ARG_ENABLE(tool-check,      [  --disable-tool-check         Don't test for presence of required
                               runtime tools],tool_check=no,tool_check=yes)
AC_ARG_ENABLE(colors,          [  --disable-colors             Don't color console output]
                               ,colors=no,colors=yes)
#AC_ARG_ENABLE(build-win32,     [  --enable-build-win32         Configure for msys/mingw32 win32 build]
#                               ,build_win32=yes,)
AC_ARG_ENABLE(pkg-win32,       [  --disable-pkg-win32          Don't install win32 files in a standalone
                               directory structure],pkg_win32=no,pkg_win32=yes)
AC_ARG_ENABLE(force-endian,    [  --enable-force-endian=x      Use x-endian byte-order, where x is
                               'big', 'little', or 'opp' (opposite of detected)], force_endian=$enableval, force_endian=no)
#AC_ARG_ENABLE(wxsegfault-test, [  --disable-wxsegfault-test    Don't try to compile & run the wxWidgets 2.6.3
#                               segfault test program],,wxsegfault_test=yes)

AM_OPTIONS_WXCONFIG

case "$host" in
	(*-*-cygwin|*mingw*) 
		build_win32=yes
#		LPLEX_DEFS="-Dlplex_win32"
		AC_CHECK_TOOL(WINDRES, windres)
		#	tool_check=no;
		;;
	(*) 
#		LPLEX_DEFS="-Dlplex_linux"
		AC_CHECK_PROG(XDGUTILS, xdg-mime, yes)
		pkg_win32=no
		;;
esac

AM_CONDITIONAL([MSW_RELEASE], [test "x$pkg_win32" = "xyes"])
AM_CONDITIONAL([HAVE_WINDRES], [test "x$WINDRES" != "x"])
AM_CONDITIONAL([HAVE_FREEDESKTOP], [test "x$XDGUTILS" = "xyes"])

if test "x$colors" = "xno" ; then
	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_nocolor"
fi

#if test "x$build-console" = "xyes" ; then
#	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_console"
#else
#	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_gui"
#fi

#if test "x$force_async" = "xyes" ; then
#	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_async"
#	echo "forcing asynchronous external process execution per your request"
#fi


# Checks for libraries.

# ensure '-ldl' is attached to link if system uses libdl,
# otherwise AC_CHECK_LIB tests will fail to load other shared objects
AC_CHECK_LIB(dl, dlopen, LIBDL="-ldl", LIBDL="")

# --------------------------------------------------------------------------
# check for libjpeg
# --------------------------------------------------------------------------
#AC_CHECK_LIB(jpeg, jpeg_read_header,, 
#  [AC_MSG_ERROR("Can't find libjpeg: is libjpeg-devel installed and on your path?")],
#)

# --------------------------------------------------------------------------
# check for libdvdread
# --------------------------------------------------------------------------
AC_CHECK_HEADER(dvdread/dvd_reader.h,dvdrd_h=yes,
  [AC_MSG_ERROR("Can't find libdvdread headers: are they on your path?")]
)
AC_CHECK_LIB(dvdread, ifoOpen,, 
  [AC_MSG_ERROR("Can't find libdvdread: is it installed and on your path?")], $LIBDL
)
AC_CHECK_LIB(dvdread, udflist,
	LPLEX_DEFS="$LPLEX_DEFS -Ddvdread_udflist",, $LIBDL
)

# --------------------------------------------------------------------------
# configure libFLAC
# --------------------------------------------------------------------------
AM_PATH_LIBFLAC(,AC_MSG_ERROR([Can't find libFLAC]))
AC_LANG(C++)
AM_PATH_LIBFLACPP(,AC_MSG_ERROR([Can't find libFLAC++]))
if test "x$build_win32" = "xyes" ; then
	# to prevent ntoh() undefined reference
	LIBFLACPP_LIBS="$LIBFLACPP_LIBS -lwsock32"
	AC_SUBST(LIBFLACPP_LIBS)
fi

# remove any empty '-L ' flags in LIBS vars, which cause link failure
LIBFLAC_LIBS=$(echo $LIBFLAC_LIBS|sed -e 's/-L //g')
LIBFLACPP_LIBS=$(echo $LIBFLACPP_LIBS|sed -e 's/-L //g')
AC_SUBST(LIBFLAC_LIBS)
AC_SUBST(LIBFLACPP_LIBS)

# --------------------------------------------------------------------------
# configure wxWidgets
# --------------------------------------------------------------------------

wxUTF8=1
AM_PATH_WXCONFIG(2.9.2, ,wxUTF8=0,,)
if test "$wxUTF8" = 0; then
	wxFail=0
	AM_PATH_WXCONFIG(2.7.2, ,wxFail=1,,--unicode=no)
	if test "$wxFail" = 1; then
		AC_MSG_ERROR([
	*   Can't find an appropriate version of wxWidgets. Please check if
	*      - wx-config is in path.
	*      - wxWidgets version is either:
	*          - 2.9.2 or above, any build.
	*          - 2.7.2 or above and build is ANSI (not unicode).

		])
	fi
fi
# wx-config defaults to gui link flags (-mwindows, etc) which block console io,
# so we need to redo WX_LIBS
if test "x$build_win32" != "x" ; then
   WX_LIBS="$(wx-config --libs core,base | sed -e 's/ -Wl,--subsystem,windows -mwindows//')"
#   WX_LIBS="$(echo $WX_LIBS | sed -e 's/ -mthreads//')"
   WX_LIBS="-Wl,--subsystem,console $WX_LIBS -s"
   WX_LIBS_STATIC=$WX_LIBS
	AC_SUBST(WX_LIBS)
	AC_SUBST(WX_LIBS_STATIC)
fi

# we assume that if wx-config is found, wxWindows is successfully installed.

#AC_PATH_PROG(WX_CONFIG_PATH, wx-config, no, $PATH:/usr/local/bin )
#if [[ "$WX_CONFIG_PATH" = "no" ]] ; then
#AC_MSG_ERROR("Couldn't find wx-config: is wx-config in your path?")
#fi

#AC_MSG_CHECKING([wxWidgets version])
#wx_version=`${WX_CONFIG_PATH}  --version`
#AC_MSG_RESULT($wx_version)

#if(wxFail==0 && test "x$build_win32" = "x" && test "x$wxsegfault_test" != "xno") then
#	AC_MSG_CHECKING([whether unix console segfaults in wxStandardPaths::GetInstallPrefix()])
#	workaround=-D_workaround_wx_2.6.3_unix_GetInstallPrefix_segfault
#	
##if test "x$build-console" = "xyes" ; then
##	WX_LIBS=`$WX_CONFIG_PATH --unicode=no --libs base`
##	WX_LIBS=$WX_LIBS -lwxjpeg
##else
#	WX_LIBS=`$WX_CONFIG_PATH --unicode=no --libs core,base`
##fi
#	WX_CPPFLAGS=`$WX_CONFIG_PATH --unicode=no --cxxflags`

#	ac_save_CXXFLAGS="$CXXFLAGS"
#	ac_save_LIBS="$LIBS"
#	CXXFLAGS="$CXXFLAGS $WX_CPPFLAGS"
#	LIBS="$LIBS $WX_LIBS"
#	
#	#rm -f conf.libxxxtest
#	AC_LANG(C++)
#	AC_TRY_RUN(
#	[
#		#include <wx/app.h>
#		#include <wx/stdpaths.h>
#		int main()
#		{
#			if ( ! wxInitialize() )
#				return -1;
#			wxStandardPaths().GetDataDir();
#			return 0;
#		}
#	],
#	[	workaround=
#		AC_MSG_RESULT(no)
#		AC_SUBST(WX_LIBS)
#		AC_SUBST(WX_CPPFLAGS)
#	],
#	[	wxSegf=`grep -i 'Segmentation fault' config.log`
#		wxSegf="$wxSegf`grep -i 'SIGSEGV' config.log`"
#		if [[ "${wxSegf}x" = "x" ]] ; then 
#			AC_MSG_RESULT(inconclusive)
#			echo "*   Test program failed, though apparently not because of the
#*   segfault bug. Activating the segfault workaround nevertheless
#*   (just in case) and continuing with configure. Please see
#*   config.log for specifics on the failure, you may need to resolve
#*   a runtime link issue.
#"
#			wxFail=3
#		else
#			AC_MSG_RESULT(yes)
#			echo "*     Activating the segfault workaround and continuing with
#*     configure."
##			WX_LIBS=
##			WX_CPPFLAGS=
#			wxFail=2
#		fi
#	])
#	
#	CXXFLAGS="$ac_save_CFLAGS"
#	LIBS="$ac_save_LIBS"
#fi
#if [[ "${wxFail}" = "1" ]] ; then 
#	echo "*     Please Install wxWidgets 2.6.3 or newer."
#fi
#if [[ "${wxFail}" = "1" ]||[ "${wxFail}" = "2" ]||[ "${wxFail}" = "3" ]] ; then 
#	echo "*   [About the segfault bug in wxWidgets 2.6.3]:
#*   This release predates a required fix to prevent a segfault in
#*   the unix console.  lplex will work around this, but if you'd
#*   like to eliminate the bug (and workaround) out of an
#*   abundance of paranoia, then please first update the following
#*   4 files
#*      - include/wx/stdpaths.h      - include/wx/unix/stdpaths.h
#*      - src/unix/stdpaths.cpp      - src/common/stdpbase.cpp
#*   to the latest cvs versions before building and installing
#*   wxWidgets.  You may need to run
#*        './configure --disable-wxsegfault-test'
#*   if you see the test still reporting 'inconclusive' after the fix.
#"
##	AC_MSG_ERROR([wxWidgets deficiencies.]);	
#fi

#if test "x$workaround" != "x" ; then
#   LPLEX_DEFS="$LPLEX_DEFS $workaround"
#fi


# Checks for library functions.
# --------------------------------------------------------------------------
# test byte-order
# --------------------------------------------------------------------------

_endian=unknown

# ---------------------------------------------------------------------------
# (slightly modified from:)
# Macros for configure.ac for wxWindows by Robert Roebling, Phil Blecker,
# Vadim Zeitlin and Ron Lee
# This macro (WX_C_BIGENDIAN) is under the wxWindows licence.
# Version: $Id: acinclude.m4,v 1.27 2005/07/08 20:44:14 MW Exp $
# ---------------------------------------------------------------------------
# a slightly better AC_C_BIGENDIAN macro which allows cross-compiling
# ---------------------------------------------------------------------------

AC_DEFUN([WX_C_BIGENDIAN],
[AC_CACHE_CHECK([whether byte ordering is bigendian], ac_cv_c_bigendian,
[ac_cv_c_bigendian=unknown
# See if sys/param.h defines the BYTE_ORDER macro.
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/param.h>], [
#if !BYTE_ORDER || !BIG_ENDIAN || !LITTLE_ENDIAN
 bogus endian macros
#endif], [# It does; now see whether it defined to BIG_ENDIAN or not.
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/param.h>], [
#if BYTE_ORDER != BIG_ENDIAN
 not big endian
#endif], ac_cv_c_bigendian=yes, ac_cv_c_bigendian=no)])
if test $ac_cv_c_bigendian = unknown; then
AC_TRY_RUN([main () {
  /* Are we little or big endian?  From Harbison&Steele.  */
  union
  {
    long l;
    char c[sizeof (long)];
  } u;
  u.l = 1;
  exit (u.c[sizeof (long) - 1] == 1);
}], [ac_cv_c_bigendian=no], [ac_cv_c_bigendian=yes], [ac_cv_c_bigendian=unknown])
fi])
if test $ac_cv_c_bigendian = unknown; then
  AC_MSG_WARN([Assuming little-endian target machine - this may be overriden by adding the line "ac_cv_c_bigendian=${ac_cv_c_bigendian='yes'}" to config.cache file])
fi
if test $ac_cv_c_bigendian = yes; then
	ifelse([$1], , :, [$1])
else 
	ifelse([$2], , :, [$2])     

#	_endian=big else _endian=lit
#  AC_DEFINE(WORDS_BIGENDIAN)
	
fi
])

# --------------------------------------------------------------------------
# configure byte-order
# --------------------------------------------------------------------------

if test "x$force_endian" = "xbig" ; then
	_endian=big
	echo "forcing big-endian byte-order per your request"
elif test "x$force_endian" = "xlittle" ; then
	_endian=little
	echo "forcing little-endian byte-order per your request"
else
	WX_C_BIGENDIAN(_endian=big,_endian=little)
	if test "x$force_endian" = "xopp" ; then
		if test "x$_endian" = "xlittle" ; then
			_endian=big
		else
			_endian=little
		fi
		echo "forcing opposite byte-order per your request: ${_endian}-endian"
	fi
fi


if test "x$_endian" = "xbig" ; then
	_endian="-Dlplex_bigendian"
else
	_endian=;
fi

if test "x$_endian" != "x" ; then
   LPLEX_DEFS="$LPLEX_DEFS $_endian"
fi



if test "x$build_win32" = "x" ; then
# --------------------------------------------------------------------------
# check how to run external tools
# --------------------------------------------------------------------------
   AC_CHECK_FUNCS(popen,havepopen=1,)
   if test "x$havepopen" = "x" ; then
      AC_PATH_PROG(SCRIPT_PATH, script)
      if test "x$SCRIPT_PATH" = "x" ; then
      	AC_MSG_WARN([	lplex requires either (preferably) the libc function popen()
      	at compile time, or as a fallback the bsd command 'script' at runtime.
      	Please check that at an absolute minimum 'script' is available.])
      else
      	AC_MSG_WARN([	Using 'script' as a fallback to popen(). Some
      	console output from the runtime tools will be delayed.])
      fi
   	LPLEX_DEFS="$LPLEX_DEFS -Dno_popen"
   fi
fi # !build_win32

if test "x$tool_check" = "xno" ; then
   echo "skipping runtime tools check..."
else

   # 'read tok1 tok2 tok3 tok4 tok5 tok6 <<< $txt' isn't portable since
   # the '<<<' operator isn't supported in the default msys shell and
   # generates a syntax error causing configure to fail
   readtoks()
   {
      tok1=$1; tok2=$2; tok3=$3; tok4=$4; tok5=$5; tok6=$6
   }

# --------------------------------------------------------------------------
# check for runtime tools
# --------------------------------------------------------------------------
   echo "checking for presence of required runtime tools"

   AC_PATH_PROG(DVDAUTHOR_PATH, dvdauthor)
   if test "x$DVDAUTHOR_PATH" != "x" ; then
#   	AC_MSG_WARN([lplex requires dvdauthor >= 0.6.14 at runtime.])
#   else
   	AC_MSG_CHECKING([dvdauthor version])
   	readtoks $(dvdauthor 2>&1)
   	dvdauthorVer=$tok3
   	AC_MSG_RESULT([$dvdauthorVer])

   	readtoks $(echo "${dvdauthorVer}"|tr '[(.)]' ' ')
   	x=`echo $(($tok1*10000+$tok2*100+$tok3))`
   	if(( x<611 )) ; then
         AC_MSG_WARN(dvdauthor version $dvdauthorVer is too old.)
   	fi
   	if(( x<614 )) ; then
         echo "*   Please upgrade dvdauthor to version 0.6.14 or greater.
*   Although lplex will work correctly with dvdauthor 0.6.11
*   in most cases, dvd navigation may fail if timestamps
*   exceed approximately 2 hours for any given stream.  Always 
*   ensure that the 'dvdauthor' section of lplex.log is free of
*   warnings prior to burning if using dvdauthor 0.6.11.
"
   	fi
   fi

   AC_PATH_PROG(MPLEX_PATH, mplex)
   if test "x$MPLEX_PATH" != "x" ; then
#    	AC_MSG_WARN([lplex requires (mjpegtools) mplex >= 2.2.5 at runtime.])
##	echo "*   You'll need to either apply a patch or get a current cvs update. Please see the
##*   readme in the 'patch' folder (included with this program)
##*   for more on how to do this. 
##"
#   else
   	AC_MSG_CHECKING([mplex version])
   	readtoks $(mplex 2>&1)
   	mjpegVer=$tok4
   	mplexVer=$tok5
   	AC_MSG_RESULT([$mjpegVer $mplexVer])

   	readtoks $(echo "${mplexVer}"|tr '[(.)]' ' ')
   	x=`echo $(($tok1*10000+$tok2*100+$tok3))`
   	if(( x<20205 )) ; then
         AC_MSG_WARN(mplex version number is too old.)
   	echo "*   The reported version of mplex $mplexVer predates a critical
*   fix for lpcm (in version 2.2.5), without which white noise and/or
*   unplayable dvds will result at some audio resolutions.  Please either
*   upgrade your mjpegtools package to >= 1.9.0, or see the readme in the
*   'patch' subfolder (included with this program) for more on how to fix
*   this version of mplex. 
"
   	fi
   fi

   AC_PATH_PROG(JPEG2YUV_PATH, jpeg2yuv)
   AC_PATH_PROG(MPEG2ENC_PATH, mpeg2enc)
   AC_PATH_PROG(MKISOFS_PATH, mkisofs)
   
   missing=""

   if test "x$DVDAUTHOR_PATH" = "x" ; then
   	AC_MSG_WARN([lplex requires dvdauthor 0.6.14+ at runtime])
      missing="$missing dvdauthor"
   fi
   if test "x$MPLEX_PATH" = "x" ; then
    	AC_MSG_WARN([lplex requires mplex 2.2.5+ at runtime (pkg: mjpegtools 1.9.0+)])
      missing="$missing mplex"
   fi
   if test "x$JPEG2YUV_PATH" = "x" ; then
    	AC_MSG_WARN([lplex requires jpeg2yuv at runtime (pkg: mjpegtools)])
      missing="$missing jpeg2yuv"
   fi
   if test "x$MPEG2ENC_PATH" = "x" ; then
   	AC_MSG_WARN([lplex requires mpeg2enc at runtime (pkg: mjpegtools)])
      missing="$missing mpeg2enc"
   fi
   if test "x$MKISOFS_PATH" = "x" ; then
   	AC_MSG_WARN([lplex requires mkisofs at runtime (pkg: cdrtools)])
      missing="$missing mkisofs"
   fi

fi # tool_check

LPLEX_CPPFLAGS="$LPLEX_DEFS -I../redist -I../../redist"
case "$host" in
	(*-*-cygwin) LPLEX_CPPFLAGS="-mno-cygwin $LPLEX_CPPFLAGS";;
	(*) ;;
esac
AC_SUBST(LPLEX_CPPFLAGS)

#AC_CONFIG_AUX_DIR(directory)
#AC_PROG_INSTALL

AC_CONFIG_FILES([Makefile
                 data/Makefile
                 patch/Makefile
                 redist/Makefile
                 redist/md5/Makefile
                 src/Makefile
                 src/desktop/Makefile
                 doc/Makefile
                 msw/Makefile
                 msw/util/Makefile])
AC_OUTPUT


x64=$(echo $host|grep 'x86_64')
if test "x$x64" != "x" ; then x64="-x64" ; fi

echo "// build info:
#undef  build_ver
#undef  build_host
#undef  build_defs
#define build_ver  \"$x64\"
#define build_host \"$host\"
#define build_defs \"$LPLEX_DEFS\"
"> lplex.def

if test "$wxUTF8" = 1; then echo "#define wxUTF8" >> lplex.def ; fi


echo "
configured for '$host'"

if test "x$LPLEX_DEFS" != "x" ; then
echo "   using $LPLEX_DEFS"
fi
if test "x$pkg_win32" = "xyes" ; then
#	DESTDIR=@top_builddir@/release/${PACKAGE}-${VERSION}
#	AC_SUBST(DESTDIR)
#	echo $DESTDIR
	echo "   installing as standalone release"
	missing="dvdauthor mplex jpeg2yuv mpeg2enc mkisofs"
fi
echo ""
if test "x$missing" != "x" ; then
   echo "Note: please install required runtime tools ($missing - see above) before running lplex."
#   echo "press any key to continue..."; read -n 1
fi
