dnl Process this file with autoconf to produce a configure script.
AC_INIT([mjpegtools],[2.1.0],[mjpeg-developer@lists.sourceforge.net])
AC_PREREQ(2.57)
AC_CONFIG_SRCDIR([lavtools/lav_io.c])

MJPEG_MAJOR_VERSION=2
MJPEG_MINOR_VERSION=1
MJPEG_MICRO_VERSION=0
MJPEG_VERSION=$MJPEG_MAJOR_VERSION.$MJPEG_MINOR_VERSION.$MJPEG_MICRO_VERSION

AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([1.7])
AM_MAINTAINER_MODE


# libtool versioning
LT_RELEASE=$MJPEG_MAJOR_VERSION.$MJPEG_MINOR_VERSION
LT_CURRENT=$MJPEG_MICRO_VERSION
LT_REVISION=$MJPEG_MICRO_VERSION
LT_AGE=$MJPEG_MICRO_VERSION
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_STATIC)

dnl **********************************************************************
dnl Options

AC_ARG_ENABLE(compile-warnings,     
              AC_HELP_STRING([--enable-compile-warnings],
                             [Turn on compiler warnings.]))
AC_ARG_ENABLE(warnings_as_errors,   
              AC_HELP_STRING([--enable-warnings_as_errors],
                             [Compiler warnings are errors.]))
AC_ARG_ENABLE(static-build,     
              AC_HELP_STRING([--enable-static-build],
                             [Force static linkage instead of shared libraries (useful for profiling etc).]))
AC_ARG_ENABLE(simd-accel,
              AC_HELP_STRING([--enable-simd-accel],
                             [use SIMD multimedia instructions
                              (MMX, AltiVec, etc.) if possible]))
AC_ARG_WITH(extra-cflags,
            AC_HELP_STRING([--with-extra-cflags=flags],
                           [Options to be added to CFLAGS (optional)]))
AC_ARG_WITH(libquicktime,
            AC_HELP_STRING([--without-libquicktime],
                           [Do not use libquicktime.]),
            [], [with_libquicktime=yes])
AC_ARG_WITH(libdv,
            AC_HELP_STRING([--without-libdv], [Do not use libdv.]),
            [], [with_libdv=yes])
AC_ARG_WITH(libpng,
            AC_HELP_STRING([--without-libpng], [Do not use libpng.]),
            [], [with_libpng=yes])
AC_ARG_WITH(dga,
	    AC_HELP_STRING([--without-dga], [Do not use dga.]),
	    [], [with_dga=yes])
AC_ARG_WITH(gtk,
	    AC_HELP_STRING([--without-gtk], [Do not use gtk.]),
	    [], [with_gtk=yes])
AC_ARG_WITH(libsdl,
	    AC_HELP_STRING([--without-libsdl], [Do not use libsdl.]),
	    [], [with_libsdl=yes])
AC_ARG_WITH(sdlgfx,
	    AC_HELP_STRING([--without-sdlgfx], [Do not use sdlgfx.]),
	    [], [with_sdlgfx=yes])
AC_GNU_SOURCE

dnl Initialize libtool
AC_PROG_LIBTOOL

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC

# automake 1.9 could use AM_PROG_STDC but 1.10 became picky and needs CC_C_0
AM_PROG_CC_C_O
# AM_PROG_CC_STDC

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_SYS_LARGEFILE

dnl configure AS and ASFLAGS...
AM_PROG_AS

if test x"$with_extra_cflags" != "x"; then
   CFLAGS="$CFLAGS $with_extra_cflags"
   CXXFLAGS="$CXXFLAGS $with_extra_cflags"
   CCASFLAGS="$CCASFLAGS $with_extra_cflags"
fi

AC_CHECK_HEADERS([stdint.h inttypes.h sys/types.h getopt.h malloc.h sys/soundcard.h])

#maddog:  check for math lib, and add it to LIBS (needed for fmax, lround...)
#maddog:  (How does this interact with cpml stuff below???????)
AC_CHECK_LIB([m],[sin])
AC_CHECK_FUNCS([posix_memalign memalign fmax fmin lround])

AC_CHECK_FUNC(getopt_long,
              [AC_DEFINE(HAVE_GETOPT_LONG, 1, [long getopt support])],
              [ # FreeBSD and BSD/OS have a gnugetopt library for this:
               AC_CHECK_LIB([gnugetopt], [getopt_long],
                            [AC_DEFINE(HAVE_GETOPT_LONG, 1,
                                       [long getopt support])
                             LIBGETOPT_LIB="-lgnugetopt"])
              ])
AC_SUBST(LIBGETOPT_LIB)

AC_TYPE_SIGNAL

AC_MSG_NOTICE([The host is $host  $host_cpu  $host_vendor  $host_os .])

dnl test for alpha architecture, and Compaq Math Library (cpml) -- much faster
dnl than GNU libm on this arch.
LIBM_LIBS="-lm "
AC_SUBST(LIBM_LIBS)
AC_MSG_CHECKING([for alpha architecture])
case $host in
	alpha*-*-linux-*)
	    AC_MSG_RESULT([yes])
	    AC_CHECK_LIB(cpml, asin,
                         [ LIBM_LIBS="-lcpml"
                           have_cpml=true ],,)
            ;;
        *)
            AC_MSG_RESULT([no])
            ;;
esac

dnl *********************************************************************
dnl Some host-specific tweaks:
dnl  o (e.g) Cygwin shared libs cannot have undefined symbols, libtool only
dnl   builds them if really given flag to force this (see ltmain.sh)
dnl   This also requires inter-library dependency to be indicated in _LIBADD;
dnl   (probably good libtool practice in any case) to have all symbols satisfied
dnl  o Linux handles with or without this flag, but at least on OS-X/Darwin
dnl   altivec (utils) can cause circular library dependency (with mpeg2enc),
dnl   so better limit this flag
dnl *********************************************************************
case $host in
  *-apple-darwin*)
   CFLAGS="$CFLAGS"
   ;;
  *-unknown-netbsdelf2.*)
   LIBS="-lossaudio"
   ;;
  *-*-cygwin* | *-*-mingw* | *-*-pw32* | *-*-os2*) EXTRA_LDFLAGS="-no-undefined"
   ;;
esac
AC_SUBST(EXTRA_LDFLAGS)

dnl *********************************************************************
dnl Test whether we're linux or not. If so, define HAVE_V4L and build the
dnl video4linux/hardware dependent tools. Else, undefine HAVE_V4L

have_video4linux=false
AC_ARG_WITH([v4l], AC_HELP_STRING([--without-v4l], [Disable v4l1 API]))
AS_IF([test "x$with_v4l" != "xno"], [
case $host in
  *-*-linux*)
  AC_CHECK_HEADER(linux/videodev.h,
                  [have_video4linux=true
                   AC_DEFINE(HAVE_V4L, 1,
		             [Building for Linux - using the video4linux API])], [])
  ;; 
esac])

AS_IF([test "x$have_video4linux" != "xtrue" && test "x$with_v4l" != "xno"],
        [have_video4linux=false
	 AC_MSG_WARN([videodev.h not found - please install the linux kernel headers programms needing v4l disabled])])
AM_CONDITIONAL(HAVE_V4L, test x$have_video4linux = xtrue)

dnl *********************************************************************
dnl Figure out how to build with POSIX threads.
dnl (Creates PTHREAD_LIBS, PTHREAD_CFLAGS, PTHREAD_CC,
dnl  and defines HAVE_PTHREAD.)
dnl *********************************************************************
ACX_PTHREAD
dnl maddog: The following matches the original pthread-checking behavior
dnl         in this script, i.e. "If a CFLAG is necessary, use it for 
dnl         *everything*."  Hopefully, any CFLAGs will eventually go away.
dnl         The other option is to stick PTHREAD_CFLAGS/CC in Makefile.am....
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

oldLIBS="$LIBS"
LIBS="$LIBS $PTHREAD_LIBS"
AC_CHECK_FUNC(pthread_attr_getstacksize,
              [AC_DEFINE(HAVE_PTHREADSTACKSIZE, 1,
                         [pthread stack sizes accesible])])
LIBS="$oldLIBS"

AC_CHECK_LIB(jpeg, jpeg_start_compress,
	    [ JPEG_LIBS="-ljpeg"
	      JPEG_CFLAGS=""
	    have_libjpeg=true ],,)

if test x$have_libjpeg != "xtrue"; then
   AC_MSG_ERROR([JPEG 6b library missing - Go to http://www.ijg.org/])
fi

AC_SUBST(JPEG_LIBS)
AC_SUBST(JPEG_CFLAGS)

PKG_PROG_PKG_CONFIG

have_libquicktime=false
if test x$with_libquicktime != xno ; then
  PKG_CHECK_MODULES(LIBQUICKTIME, [libquicktime > 0.9.7],
                    [have_libquicktime=true
                     AC_DEFINE(HAVE_LIBQUICKTIME, 1, [libquicktime > 0.9.7 present])],
                    [have_libquicktime=false])
fi
AM_CONDITIONAL(HAVE_LIBQUICKTIME, test x$have_libquicktime = xtrue)

dnl *********************************************************************
dnl Check for PNG library
dnl   (creates LIBPNG_CFLAGS, LIBPNG_LIBS)
dnl *********************************************************************
have_libpng=false
if test x$with_libpng != xno ; then
  PKG_CHECK_MODULES(LIBPNG, [libpng], [have_libpng=true], [have_libpng=false])
  if test x$have_libpng = xfalse ; then
    PKG_CHECK_MODULES(LIBPNG, [libpng12], [have_libpng=true], [have_libpng=false])
  fi
fi
AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

dnl *********************************************************************
dnl Check for libdv
dnl   (creates LIBDV_CFLAGS, LIBDV_LIBS; defines HAVE_LIBDV)
dnl *********************************************************************
have_libdv=false
if test x$with_libdv != xno ; then
  PKG_CHECK_MODULES(LIBDV, [libdv >= 0.9],
                    [have_libdv=true
                     AC_DEFINE(HAVE_LIBDV, 1, [libdv is present])],
                    [have_libdv=false])
fi

dnl *********************************************************************
dnl  Look for X Window System.
dnl   (creates X_CFLAGS, X_PRE_LIBS, X_EXTRA_LIBS, X_LIBS;
dnl    defines X_DISPLAY_MISSING if no X)
dnl *********************************************************************
AC_PATH_XTRA

dnl ********************************************************************
dnl Check to see if __progname is provided by the system
dnl ********************************************************************
AC_CACHE_CHECK([for __progname],
               [mjt_cv_extern___progname],
               [AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
                                                [[extern char *__progname;
                                                  puts(__progname);]])],
                               [mjt_cv_extern___progname=yes],
			       [mjt_cv_extern___progname=no])])
if test x$mjt_cv_extern___progname = xyes ; then
  AC_DEFINE(HAVE___PROGNAME, 1, [Is __progname defined by system?])
fi

dnl ********************************************************************
dnl Check for DGA (for v4l-conf)
dnl ********************************************************************
have_dga=false
if test x$with_dga != xno ; then
	AC_SUBST(V4LCONF_LIBS)
	AC_CHECK_LIB(Xxf86dga, XF86DGAQueryExtension,
		     V4LCONF_LIBS="$X_LIBS -lXxf86dga"
		     have_dga="true"
		     AC_DEFINE(HAVE_LIBXXF86DGA,1, [libxf86dga is present]),
		     have_dga="false",
		     $X_LIBS $X_PRE_LIBS -lXext $X_EXTRA_LIBS)
fi
AM_CONDITIONAL(HAVE_LIBXXF86DGA, test x$have_dga = xtrue)

dnl ********************************************************************
dnl Check for Gtk+/glib (for glav).  2.4.0 or greater required (should
dnl not be a problem on recent distributions).
dnl   (defines GTK_CFLAGS and GTK_LIBS)
dnl ********************************************************************
have_gtk=false
if test x$with_gtk != xno ; then
	PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.4.0],
			  have_gtk="true", have_gtk="false")
fi
AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xtrue)

dnl ********************************************************************
dnl Check for the SDL library (for software playback)
dnl   (defines SDL_CFLAGS and SDL_LIBS)
dnl ********************************************************************
have_sdl=false
if test x$with_libsdl != xno ; then
  PKG_CHECK_MODULES(SDL, [sdl > 1.1.3],
		    [have_sdl=true
		     AC_DEFINE(HAVE_SDL, 1, [libsdl > 1.1.3 present])],
		    [have_sdl=false])
fi
AM_CONDITIONAL(HAVE_SDL, test x$have_sdl = xtrue)			  

dnl ********************************************************************
dnl Check for SDL_gfx which is used by y4mhist to display a graphical 
dnl histogram.
dnl ********************************************************************
have_sdlgfx=false
if test x$with_sdlgfx != xno ; then
	AC_SUBST(SDLgfx_LIBS)
	if test x$have_sdl = xtrue; then
		OLD_LIBS="$LIBS"
		LIBS="$LIBS -lSDL -lSDL_gfx"
		AC_CHECK_LIB(SDL_gfx, vlineColor,
			     [SDLgfx_LIBS="-lSDL_gfx"
			      AC_DEFINE(HAVE_SDLgfx, 1, [SDL_gfx library present])
			      have_sdlgfx=true ],[have_sdlgfx=false])
		LIBS="$OLD_LIBS"
	fi

fi
AM_CONDITIONAL(HAVE_SDLgfx, test x$have_sdlgfx = xtrue)

dnl ********************************************************************
dnl Test for MMX support if an IA32 platform.  If on a PPC then look for
dnl Altivec support.  For the IA32 platform see if gcc understands inline 
dnl MMX instructions.
dnl
dnl TODO: host = non intel, target = intel <blah> do the checks also?
dnl       Remember we are checking that the toolchain can generate the
dnl       code, not whether the host can execute the code, thats done
dnl       at run time with the exception of the SSE code.

have_asm_mmx=false
have_x86cpu=false
have_altivec=false
AC_MSG_CHECKING([Architecture])
case $host_cpu in
  i[[3-7]]86)
    AC_MSG_RESULT(IA32)
    AC_DEFINE(HAVE_X86CPU,1, [Compiling for x86 architecture CPU])
    have_x86cpu=true
    PROGRAM_NOPIC="-fno-PIC"
    ;;
  x86_64*)
    AC_MSG_RESULT(x86_64)
    AC_DEFINE(HAVE_X86CPU,1, [Compiling for x86-64 architecture CPU])
    have_x86cpu=true
    have_x86_64cpu=true
    PROGRAM_NOPIC="-fno-PIC"
    ;;
  powerpc | powerpc64)
    AC_MSG_RESULT(PowerPC)
    AC_DEFINE(HAVE_PPCCPU,1, [Compiling for PowerPC CPU])
    have_ppccpu=true
    PROGRAM_NOPIC=""
    ;;
  *)
    AC_MSG_RESULT(unknown)
    PROGRAM_NOPIC=""
    ;;
esac

dnl Apple's darwin works on multiple hosts now, so add a catchall override
dnl after processing normal hosts
case $host in
  *-apple-darwin*)
   PROGRAM_NOPIC="-mdynamic-no-pic" # dynamic-no-pic is an Apple gcc option
   ;;
esac

dnl This flag is used for PROGRAMS not SHARED LIBRARIES.  PIC code is required
dnl for shared libraries but is slower than non-pic code.  Compute bound 
dnl programs such as yuvdenoise and y4mspatialfilter can use this flag by
dnl simply adding programname_CFLAGS=@PROGRAM_NOPIC@ to the Makefile.am

AC_SUBST(PROGRAM_NOPIC)

if test "$enable_simd_accel" != "false" -a "$enable_simd_accel" != "no"
then
  if test x$have_x86cpu = xtrue
  then
      AC_MSG_CHECKING([if C compiler accepts inline MMX assembly])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]],
                                         [[__asm__ __volatile__("emms":::"memory");]])],
                        [have_asm_mmx=true],
			[have_asm_mmx=false])
      if test $have_asm_mmx = true; then
	 AC_MSG_RESULT(yes)
      else
	 AC_MSG_RESULT(no)
      fi

      if test $have_asm_mmx = true; then
	 AC_DEFINE(HAVE_ASM_MMX,1,[Inline MMX assembly accepted by C compiler])
      fi
  fi

  if test x$have_ppccpu = xtrue
  then
    AC_MSG_CHECKING([compiler support for AltiVec])
    cat > conftest.c <<EOF
#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif
int main() {
    union { vector signed int v;
            signed int i;
          } vi;
    vi.v = vec_splat_s32(1);
    return vi.i;
}
EOF
    ALTIVEC_CFLAGS=""
    if $CC -o conftest conftest.c >/dev/null 2>&1; then
      have_altivec=true
    elif $CC $CFLAGS -faltivec -maltivec -o conftest conftest.c >/dev/null 2>&1; then
      # Mac OS X style with -maltivec
      have_altivec=true
      ALTIVEC_CFLAGS="-faltivec -maltivec"
    elif $CC $CFLAGS -faltivec -o conftest conftest.c >/dev/null 2>&1; then
      # Mac OS X style without -maltivec
      have_altivec=true
      ALTIVEC_CFLAGS="-faltivec"
    elif $CC $CFLAGS -fvec -o conftest conftest.c >/dev/null 2>&1; then
      # Motorola style
      have_altivec=true
      ALTIVEC_CFLAGS="-fvec"
    elif $CC $CFLAGS -DHAVE_ALTIVEC_H=1 -maltivec -mabi=altivec -o conftest conftest.c >/dev/null 2>&1; then
      # gcc 3.1 style
      have_altivec=true
      ALTIVEC_CFLAGS="-DHAVE_ALTIVEC_H=1 -maltivec -mabi=altivec"
    fi
    rm -f conftest*;
  
    if test x$have_altivec = xtrue
    then
      CFLAGS="$CFLAGS ${ALTIVEC_CFLAGS}"
      CXXFLAGS="$CXXFLAGS ${ALTIVEC_CFLAGS}"
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_ALTIVEC,1, [Inline PPC Altivec primitives available])
    else
      AC_MSG_RESULT(no)
    fi
  fi
fi

dnl *********************************************************************
dnl For x86 and ppc machines check which code model we should use.  This is
dnl done with the helper script cpuinfo.sh because the logic to do the job
dnl correctly is complex and lengthy (about as long as the entire configure.ac
dnl file).

if test x$have_x86cpu = xtrue -o x$have_ppccpu = xtrue; then
  if test x$host_alias != x; then
    dnl This is to allow cross compiling
    AC_MSG_CHECKING(sub-architecture settings)
    if test x$have_x86cpu = xtrue; then
       host_mod_cpu=`echo $host_cpu | tr _ -`
       ARCHFLAGS="-march=$host_mod_cpu -mcpu=$host_mod_cpu"
    fi
  else
    if test ! -r $srcdir/cpuinfo.sh; then
       AC_MSG_ERROR([cpuinfo.sh script not found - do a cvs update])
    fi
    AC_MSG_CHECKING(sub-architecture settings)
    chmod +x $srcdir/cpuinfo.sh
    if test x$have_x86cpu = xtrue; then
       ARCHFLAGS=`$srcdir/cpuinfo.sh x86`
    elif test x$have_ppccpu = xtrue; then
       ARCHFLAGS=`$srcdir/cpuinfo.sh ppc`
    fi
  fi
  AC_MSG_RESULT($ARCHFLAGS)
fi

dnl **********************************************************************
dnl Handle forcing static linkage in key places
if test "x$enable_static_build" = "xyes" ; then
  echo "Forcing static linkage..."
  LT_STATIC=-all-static
else
  echo enable_static_build="\'${enable_static_build}\'"
fi



dnl **********************************************************************
dnl All the conditional stuff for the Makefiles

AM_CONDITIONAL(HAVE_ASM_MMX, test x$have_asm_mmx = xtrue)
AM_CONDITIONAL(HAVE_X86CPU, test x$have_x86cpu = xtrue)
AM_CONDITIONAL(HAVE_PPCCPU, test x$have_ppccpu = xtrue)
AM_CONDITIONAL(HAVE_ALTIVEC, test x$have_altivec = xtrue)

# This is a redhat ppc hack - OSX (which also uses gcc4) does NOT have any
# of the problems with undefined symbols, etc caused by a misplaced reference
# to an encoder routine (next_larger_quant) in the utils/altivec routines.
ALTIVEC_EXTRA_LIBS=
if test x$have_altivec = xtrue; then
   ALTIVEC_EXTRA_LIBS=-lmpeg2encpp
fi
AC_SUBST(ALTIVEC_EXTRA_LIBS)

have_v4l_or_sdl=false
if test x$have_video4linux = xtrue; then
  have_v4l_or_sdl=true
else
  if test x$have_sdl = xtrue; then
    have_v4l_or_sdl=true
  fi
fi
AM_CONDITIONAL(HAVE_V4L_OR_SDL, test x$have_v4l_or_sdl = xtrue)

dnl *********************************************************************
dnl Check for what warnings we want gcc to use and adjust the CFLAGS
dnl as needed. This only works for GCC.
dnl We do this last as it can screw up library detection etc...

CFLAGS="$ARCHFLAGS $CFLAGS"
CXXFLAGS="$ARCHFLAGS $CXXFLAGS"

if test "x${GCC}" != "xyes" ; then
  enable_compile_warnings=no
fi
AC_MSG_CHECKING(what warning flags to pass to the C compiler)
warnCFLAGS=
warnCXXFLAGS=
if test "x$enable_compile_warnings" != "xno" ; then
 if test "x$GCC" = "xyes" ; then
    case "$CFLAGS" in
     *-Wall*) ;;
     *) warnCFLAGS="-Wall -Wunused" ;;
    esac
  if test "x$enable_compile_warnings" = "xyes" ; then
    warnCFLAGS="$warnCFLAGS -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wcast-align -Wwrite-strings -Wcast-qual"
    warnCXXFLAGS="$warnCXXFLAGS -Wmissing-prototypes  -Wpointer-arith -Wcast-align -Wwrite-strings -Wcast-qual"
  fi
  if test "x$enable_warnings_as_errors" = "xyes" ; then
    warnCFLAGS="$warnCFLAGS -Werror"
    warnCXXFLAGS="$warnCXXFLAGS -Werror"
  fi
 fi
fi
AC_MSG_RESULT($warnCFLAGS)
AC_MSG_RESULT($warnCXXFLAGS)
if test "x$cflags_set" != "xyes" ; then
  CFLAGS="$CFLAGS $warnCFLAGS"
  CXXFLAGS="$CXXFLAGS $warnCXXFLAGS"
  cflags_set=yes
fi

AC_ARG_ENABLE(assert,
              AC_HELP_STRING([--disable-assert],
                             [Disable assertions (compile with NDEBUG).]),
 [ case "$enableval" in yes|no) : ;; *) AC_MSG_ERROR(bad value for --enable-assert)                ;; esac ],
 [ enable_assert=yes ])

if test x"$enable_assert"                 = x"no"; then NDEBUG=1;                fi

AC_DEFINE(MJPEGTOOLS,1, [Building mjpegtools])

case x"$NDEBUG"                in x"1") 
	AC_DEFINE(NDEBUG,1, [Optimised no DEBUG build, assertions disabled])       
	;; 
esac

dnl **********************************************************************
dnl Output a Makefile or two and the lib/header descriptor script
dnl **********************************************************************
AC_CONFIG_FILES([
     Makefile
     debian/Makefile
     debian/control
     docs/Makefile
     lavtools/Makefile
     yuvcorrect/Makefile
     yuvdenoise/Makefile
     yuvdeinterlace/Makefile
     yuvscaler/Makefile
     y4munsharp/Makefile
     y4mutils/Makefile
     yuvfilters/Makefile
     mpeg2enc/Makefile
     aenc/Makefile
     mplex/Makefile
     scripts/Makefile
     utils/Makefile
     utils/altivec/Makefile
     utils/mmxsse/Makefile
     y4mdenoise/Makefile
     y4mscaler/Makefile
     mjpegtools.pc
     mjpegtools.spec
     ])
AC_OUTPUT

dnl ************************************************************************
dnl Summarize the config for the user.
dnl ************************************************************************
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ MJPEG tools ${VERSION} build configuration :])
AC_MSG_NOTICE([])
if test "$have_x86cpu" = "true" ; then
  AC_MSG_NOTICE([  - X86 Optimizations:])
  AC_MSG_NOTICE([    - MMX/3DNow!/SSE enabled      : ${have_asm_mmx}])
fi
if test "$have_ppccpu" = "true" ; then
  AC_MSG_NOTICE([  - PowerPC Optimizations:])
  AC_MSG_NOTICE([    - AltiVec enabled             : ${have_altivec}])
fi
if test x"$ARCHFLAGS" != x; then
  AC_MSG_NOTICE([  - arch/cpu compiler flags       : ${ARCHFLAGS}])
fi
if test x"$with_extra_cflags" != "x"; then
  AC_MSG_NOTICE([  - extra_cflags added to CFLAGS: ${with_extra_cflags}])
fi
AC_MSG_NOTICE([  - video4linux recording/playback: ${have_video4linux}])
AC_MSG_NOTICE([  - software MJPEG playback       : ${have_sdl}])
AC_MSG_NOTICE([  - Quicktime playback/recording  : ${have_libquicktime}])
AC_MSG_NOTICE([  - PNG input support             : ${have_libpng}])
AC_MSG_NOTICE([  - AVI MJPEG playback/recording  : true (always)])
AC_MSG_NOTICE([  - libDV (digital video) support : ${have_libdv} ])
AC_MSG_NOTICE([  - Gtk+ support for glav         : ${have_gtk}])
AC_MSG_NOTICE([])
