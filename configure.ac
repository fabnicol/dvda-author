#                     -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# dvda-author executable script
#
# All autoconf and make scripts Copyright Fabrice Nicol 2008-2014 <fabnicol@users.sourceforge.net>
# Dependencies: all files in m4/
######################################################################################################


AC_INIT([dvda-author],[dev],[fabnicol@users.sourceforge.net])

command_line_args="$*"
# requested to define _GNU_SOURCE and check the availability of GNU extensions
# This should be called before any macros that run the C   compiler.
AC_USE_SYSTEM_EXTENSIONS

AC_PREREQ(2.63)
AC_LANG([C])
# Path to auxiliary configuration files

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

# standard init macros
AC_PROG_LEX
AC_PROG_EGREP

AC_CONFIG_SRCDIR([src/videoimport.c])
AC_CONFIG_HEADERS([config.h])

# uses default prefix /usr unless otherwise specified by --prefix= on commandline

AC_CANONICAL_BUILD

LT_INIT

AS_IF([test -f DOWNLOADS], [rm -f DOWNLOADS])
AS_IF([test -f PATCHED.DOWNLOADS], [rm -f PATCHED.DOWNLOADS])

AS_CASE([${build}],
          [*-*-mingw32*],
		  [export EXEEXTENSION=".exe"],
		  [export EXEEXTENSION=""])
		  		  
# probably spurious on most platforms yet useful on MSYS
CPPFLAGS="$CPPFLAGS -I/usr/include -I/usr/local/include"
#default configuration
#by default, link, do not build, and use Ogg FLAC:
FLAC_BUILD=no
STATIC_SOX=no
sox_BUILD=no

#preserving user CFLAGS input
cflags_save="$CFLAGS -std=c99"
#redefine CFLAGS to avoid systematic -g -O2 introduced by AC_PROG_CC
#This should come after AC_PROG macros
AS_IF([test "$BUILDDIR" = ""],[export BUILDDIR=$PWD])
AS_IF([test "$ROOTDIR" = ""], [export  ROOTDIR=$PWD])

#preparing local build of downloaded libs

m4_include([m4/dependencies.m4])
m4_include([m4/auxiliary.m4])

DVDA_MKDIR([local])

# -----------------------------------------------------------------------------------------
# configuration file installation
#
# dvda-author.conf   will be installed in SHORTLINKDIR (in Makefiles) or INSTALL_CONF_DIR (in C code)
# ac_defining INSTALL_CONF_DIR is an alternative to configuring commonvars.h.in
# in AC_CONFIG_FILES. This requires:
# a) AC_DEFINE_UNQUOTED
# b) another identifier than SHORTLINKDIR
# c) including <config.h> in commonvars.h
# if not using configure, define SETTINGSFILE=full/path/to/dvda-author.conf at compile time
# ------------------------------------------------------------------------------------------

# checks installation dirs
# For BSD-type packaging, it is preferable not to use /usr/local as a default prefix
m4_include([m4/dvda.m4])
DVDA_PREFIX_DEFAULT([/usr/local])
AC_ARG_WITH(config, [AS_HELP_STRING([--with-config],[Path to directory of configuration file])], [SHORTLINKDIR=$withval], [])

datarootdir="${prefix}/share"
PIXDIR=$datarootdir/pixmaps
AS_IF([ test x$SHORTLINKDIR = x ], [SHORTLINKDIR=$datarootdir/applications/$PACKAGE_NAME-$PACKAGE_VERSION])
MENUDIR=$SHORTLINKDIR/menu
AS_IF([ test $exec_prefix = "NONE" ],
  [
   AC_MSG_RESULT([Using libdir=${prefix}/lib])
   libdir="${prefix}/lib"
    bindir="${prefix}/bin"
  ])

AC_DEFINE_UNQUOTED([BINDIR], "${bindir}", [Installation directory for dvda-author and tools])
AC_DEFINE_UNQUOTED(INSTALL_CONF_DIR, "$SHORTLINKDIR", [Defines directory of .desktop and .conf files])

AC_PROG_SED

#after sed test
DVDA_TEST_MAKE_VERSION

AC_ARG_WITH([debug],
    [AS_HELP_STRING([--with-debug],[ =full for debugging, =no for Release build ])],
    [
       AS_IF([ test "x$withval" = "xno"],
	[
	  AS_IF([ test x$ac_cv_c_compiler_gnu = xyes ],
	   [
             CFLAGS="-g0 -O3 -Wno-unused-result"
		#exceptionally erasing -g and -O2 from user CFLAGS input
		cflags_save=`echo $cflags_save | $SED "s/-g \(.*\)-O2/ \1/g"`
	     LIBS="$LIBS -s"
	   ])
	],
	[
	  AS_IF([ test "x$withval" = "xfull"],
	   [
	    AS_IF([ test x$ac_cv_c_compiler_gnu = xyes ],[
		# -O2 is necessary for inlining, do not take it off
	    CFLAGS="-std=c99 -g3  -Wall -Wextra -pedantic"
	    cflags_save=`echo $cflags_save | $SED "s/-g \(.*\)-O2/ \1/g"` ])
	   ],
	   [
	    CFLAGS="-O2 $withval"
	   ])

	   LIBS=`echo $LIBS | $SED "s/-s//g"`
	   CFLAGS=`echo $CFLAGS | $SED "s/-O.//g"`
           DEBUG_FLAGS="-g"

	])
    ],
    [
      LIBS="$LIBS -s"
    ])

#adding user input
CFLAGS="$CFLAGS $cflags_save"

AC_ARG_WITH([debug-flags],
    [AS_HELP_STRING([--with-debug-flags],[Debug build flags])],
    [
	 DEBUG_FLAGS=$withval
    ])

#======================================
# Core feature macros, see m4 directory
#======================================

# insert lib-specific tests here
# invoking main M4sh subroutine
#definition of empty commandline

AC_ARG_ENABLE([cont],[AS_HELP_STRING([--enable-cont],[continue download even if package not available])],[CONTINUE=yes],[CONTINUE=no])

DVDA_CHECK_DEPENDENCIES

#=======================================
# Checks for programs/functions/compiler
#=======================================

#obsolescent function checks have been retrieved from autoscan output
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_CC_STDC
AC_FUNC_MALLOC
AC_FUNC_STRNLEN
AC_FUNC_FSEEKO
AC_FUNC_REALLOC

#checks only
AC_CHECK_FUNCS([strcasecmp select getcwd floor memmove memset mkdir rmdir setlocale strerror strtol strtoul snprintf fseek setlocale strchr strdup strndup strstr strnlen getsubopt memchr dup2 strncasecmp strrchr])

# using -liberty to bootstrap GNU libc usage

AS_IF([ test x$ac_cv_c_compiler_gnu = xyes ],
 [
  GCC_MAJOR_VERSION=`$CC -dumpversion |$SED "s/\..*//"`
  AS_IF([ test `echo $CC | $SED "s/mingw32//"` = `echo $CC` ],
     [AC_DEFINE([COMPILER_IS_GCC], [1], [Whether compiler is GNU GCC])])
 ])

AC_PROG_CC_C99

AS_IF([ test "x$ac_cv_prog_cc_stdc" = "xno" ], [AC_MSG_WARN([Compiler does not support size-variable static arrays.
Please install C99-compliant compiuler. Aborting...])
						AS_EXIT])

AC_C_VARARRAYS
AC_C_CONST
AC_C_INLINE
# Determine endian-ness of target processor.

AC_C_BIGENDIAN

AC_DEFINE_UNQUOTED(CPU_IS_BIG_ENDIAN, ${ac_cv_c_big_endian},
    [Target processor is big endian.])
AC_DEFINE_UNQUOTED(CPU_IS_LITTLE_ENDIAN, ${ac_cv_c_little_endian},
    [Target processor is little endian.])


#========================================
# Checks for libraries, headers and types
#========================================

AC_HEADER_DIRENT
AC_HEADER_ASSERT
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([errno.h sys/stat.h time.h inttypes.h locale.h stdint.h stdlib.h string.h sys/time.h unistd.h fcntl.h])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_REALLOC
AC_C_CONST
AC_STRUCT_TM
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([fpos_t], [AC_MSG_RESULT([fpos_t exists on platform])],[AC_MSG_WARN([WARNING: fpos_t does not exists on platform])])

AC_SYS_LARGEFILE
AC_DEFINE([_FILE_OFFSET_BITS], [64], [Large file support enabled])

m4_map([AC_SUBST],[[[ROOTDIR]],[[CFLAGS]],[[BUILDDIR]],[[SHORTLINKDIR]],[[PIXDIR]],[[MENUDIR]],[[LIBSOXCONVERT_CPPFLAGS]],[[DEBUG_FLAGS]],[[VERSION]],[[PROGRAM_TARGETS]],[[EXEEXTENSION]]])

# correspondence between X_BUILD shell variables and subdirectory names. Could be avoided with lesser flexibility in naming conventions.

AC_MSG_NOTICE([Configuring ${PACKAGE_TARNAME}...])

AS_MKDIR_P([$BUILDDIR/depconf])

AC_CONFIG_FILES([COREBUILD images/Makefile libutils/src/Makefile libfixwav/src/Makefile libiberty/src/Makefile Makefile])
AC_CONFIG_FILES([$ROOTDIR/mk/a52dec.mk $ROOTDIR/mk/cdrtools.mk $ROOTDIR/mk/dvdauthor.mk $ROOTDIR/mk/FLAC.mk $ROOTDIR/mk/functions.mk])
AC_CONFIG_FILES([$ROOTDIR/mk/help2man.mk $ROOTDIR/mk/ImageMagick.mk $ROOTDIR/mk/libogg.mk $ROOTDIR/mk/libmpeg2.mk $ROOTDIR/mk/lplex.mk])
AC_CONFIG_FILES([$ROOTDIR/mk/man2html.mk $ROOTDIR/mk/mjpegtools.mk $ROOTDIR/mk/sox.mk sox-libs src/Makefile EXAMPLES])

GENERATE_GLOBAL_MKS
AC_OUTPUT

AC_MSG_RESULT([

*************************************************
* ${PACKAGE_TARNAME} was configured
*************************************************

You can now build ${PACKAGE_TARNAME} by issuing 'make'.
When the build is complete, become root and install
it by issuing 'make install'.
A ${PACKAGE_TARNAME} man page will be installed on Unix system
 
  Configuration summary :
 
    Version : ..................... $VERSION
 
    Dependencies:
		    fixwav..........$BUILDDIR/libfixwav/src/libfixwav.a
		    flac............$FLAC_LIB $FLAC_LINK
		    ogg.............$libogg_LIB $libogg_LINK
		    sox.............$sox_LIB $sox_LINK
 
    Build libraries:
		    libFLAC ........$FLAC_BUILD
		    libogg .........$libogg_BUILD
		    libsox  ........$sox_BUILD

		    gnu libc .......$libiberty_BUILD
		    libutils .......yes
 
    Ogg FLAC support:
		     ...............$libogg_BUILD
 
    Static sox build:
		     ...............$STATIC_SOX
 
 
])


AS_IF([ test x$ac_cv_c_compiler_gnu = xyes ],[
  AC_MSG_RESULT([
  Tools :
    Compiler is GCC : ............. $ac_cv_c_compiler_gnu
   
  Build options:
    CFLAGS=$CFLAGS
    CPPFLAGS=$CPPFLAGS
    LDFLAGS=$LDFLAGS
])
])

AS_IF([ test x$libiberty_LIB -o x$libiberty_LINK != x ],[AC_MSG_RESULT([  libiberty.a will be linked to])])

AS_IF([ test x$libiberty_BUILD = xyes ],
 [
  AC_MSG_RESULT([  GNU lib sources in libiberty/ will be recompiled for enhanced portability.])
 ])




AS_IF([test x$ac_cv_c_big_endian = xyes], [AC_MSG_RESULT([  Machine is big- endian])],[AC_MSG_RESULT([  Machine is little-endian])])

AS_IF([ test x$HELP2MAN != x ],
[
  AC_MSG_RESULT([  Man page will be generated (man 1)])
  AS_IF([ test x$MAN2HTML != x ], [AC_MSG_RESULT([  Html man page will be generated ])],[AC_MSG_RESULT([  Html man page will not be generated ])])
],
[
  AC_MSG_RESULT([  Man page will not be generated])
])


AS_IF([ test x$MP2ENC = x -o x$JPEG2YUV = x -o x$MPEG2ENC = x -o x$MPLEX = x],[
 AC_MSG_RESULT([  You will have to author top menus from available MPEG background files, as tools are missing to automate generation.])])

AC_MSG_RESULT([[
 
  Using $make as a system build tool.

  Auxiliary tools		   build [default no/yes]    install path [default system path/build path]
 
    help2man: .....................$help2man_BUILD $HELP2MAN
    man2html: .....................$man2html_BUILD $MAN2HTML
    spumux: .......................$dvdauthor_BUILD $SPUMUX
    dvdauthor: ....................$dvdauthor_BUILD $DVDAUTHOR
    mkisofs: ......................$cdrtools_BUILD $MKISOFS
    cdrecord: .....................$cdrtools_BUILD $CDRECORD
    mp2enc: .......................$mjpegtools_BUILD $MP2ENC
    jpeg2yuv: .....................$mjpegtools_BUILD $JPEG2YUV
    mpeg2enc: .....................$mjpegtools_BUILD $MPEG2ENC
    mpeg2dec: .....................$libmpeg2_BUILD $MPEG2DEC
    a52dec: .......................$a52dec_BUILD $A52DEC
    mplex: ........................$mjpegtools_BUILD $MPLEX
    mogrify:.......................$ImageMagick_BUILD $MOGRIFY
    convert:.......................$ImageMagick_BUILD $CONVERT
    lplex: ........................$lplex_BUILD $LPLEX
 
]])

AS_IF([test "$PROGRAM_TARGETS" != ""],[AC_MSG_RESULT([
   Make will target the following packages:
 
       $PROGRAM_TARGETS
 
   and the following .mk files:
])],
[AC_MSG_RESULT([
No external package will be built.
])])

for i in $PROGRAM_TARGETS
do
 AS_IF([test -f "$ROOTDIR/mk/$i.mk" ],[echo -e \\t$i.mk])
done

 AS_IF([test "$GIT" != ""],
    AC_MSG_RESULT([
  Distribution of files:  using git in $GIT
  ]))


AC_MSG_RESULT([[
   Building tools:

    make:..........................$MAKE
    make version:..................$MAKE_VERSION
    curl: .........................$CURL
    patch:.........................$PATCH
    autoconf:......................$AUTOCONF
    automake:......................$AUTOMAKE
    libtoolize:....................$LIBTOOLIZE
 
  These tools should all be installed to enable advanced dvda-author features or 
  auxiliary packages.
 
  Installation directories :
    prefix is: ....................$prefix
    Library directory: ........... $(echo $libdir)
    Program directory: ........... $(echo $bindir)
    Man pages: ................... $datarootdir/man/man1
    Documentation directory: ..... $datarootdir/doc/${PACKAGE_TARNAME}
    Images directory: ............ $(echo $PIXDIR)
    Configuration files: ......... $SHORTLINKDIR
    Images directory: ............ $MENUDIR
 
]])


AS_IF([ test -f DOWNLOADS ],[
  AC_MSG_RESULT([[  The following file(s) have been downloaded:

$(cat DOWNLOADS| grep Downloaded | $SED s/Downloaded:// ) ]])],
  [AC_MSG_RESULT([  No packages were downloaded.])])

AS_IF([ test -f PATCHED.DOWNLOADS ],
  [
     patches=$(cat PATCHED.DOWNLOADS | grep Patched | $SED -r 's/Patched://g ; s/\n/\n\ \ \ \ \ \ /g')
     AC_MSG_RESULT([[
  The following  patch(es) were applied:
 
  $patches]])


   AS_IF([test x$sox_BUILD = xyes -a "$(echo $patches | grep sox)" = ""],
	  [
	     AC_MSG_RESULT([Patching issue: you need to download and apply the sox patch first before building.])
	     patching_issue=yes
	  ])

   AS_IF([test x$dvdauthor_BUILD = xyes -a "$(echo $patches | grep dvdauthor)" = ""],
	  [
	     AC_MSG_RESULT([Patching issue: you need to download and apply the dvdauthor patch first before building.])
	     patching_issue=yes
	  ])

  ],
  [
     AC_MSG_RESULT([
  No packages were patched after download.])

     AS_IF([test x$cdrtools_BUILD = xyes -o x$dvdauthor_BUILD = xyes -o x$sox_BUILD = xyes],[patching_issue=yes])

  ])

AS_IF([test x$patching_issue = xyes],
       [
	  AC_MSG_RESULT([[
  Please try downloading the patches manually. (See file README for http links to patches)]])
       ]
       ,
       [
	 AC_MSG_RESULT([
  You may now build by typing 'make' and install with 'make install'.
       ])])

AS_IF([ test x$ImageMagick_BUILD = xyes ], [DVDA_INF([IMAGEMAGICK_MSG])])


