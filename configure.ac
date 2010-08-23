#                     -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# dvda-author executable script
#
# All autoconf and automake scripts Copyright Fabrice Nicol 2008-2009 <fabnicol@users.sourceforge.net>
# Dependencies: all files in m4/
######################################################################################################



AC_INIT([dvda-author],[10.06-400.core],[fabnicol@users.sourceforge.net])
# requested to define _GNU_SOURCE and check the availability of GNU extensions
# This should be called before any macros that run the C   compiler.
AC_USE_SYSTEM_EXTENSIONS

AC_PREREQ(2.63)
AC_LANG([C])
# Path to auxiliary configuration files

# AC_CONFIG must be called before AM_INIT_AUTOMAKE
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

# standard init macros

# make dist produces both tar.bz2 and tar.gz packages
AM_INIT_AUTOMAKE([dist-lzma -Wall])
AC_CONFIG_SRCDIR([src/videoimport.c])
AC_CONFIG_HEADER([config.h])

# uses default prefix /usr unless otherwise specified by --prefix= on commandline
DVDA_PREFIX_DEFAULT([/usr])

AC_CANONICAL_BUILD

# do not use libtool in this version, for arcane and unclear reasons.
AC_PROG_RANLIB

# probably spurious on most platforms yet useful on MSYS
CPPFLAGS="$CPPFLAGS -I/usr/include -I/usr/local/include"
#default configuration
#by default, link, do not build, and use Ogg FLAC:
FLAC_BUILD=no
STATIC_SOX=no
SOX_BUILD=no


#preserving user CFLAGS input
cflags_save="$CFLAGS -std=c99"
#redefine CFLAGS to avoid systematic -g -O2 introduced by AC_PROG_CC
#This should come after AC_PROG macros
ROOTDIR=`cd "$srcdir" ; pwd`
#preparing local build of downloaded libs
DVDA_MKDIR([local])


# -----------------------------------------------------------------------------------------
# configuration file installation
#
# dvda-author.conf   will be installed in SHORTLINKDIR=INSTALL_CONF_DIR.
# ac_defining INSTALL_CONF_DIR is an alternative to configuring commonvars.h.in
# in AC_CONFIG_FILES. This requires:
# a) AC_DEFINE_UNQUOTED
# b) another identifier than SHORTLINKDIR
# c) including <config.h> in commonvars.h
# if not using configure, define SETTINGSFILE=full/path/to/dvda-author.conf at compile time
# ------------------------------------------------------------------------------------------

# checks installation dirs
# For BSD-type packaging, it is preferable not to use /usr/local as a default prefix

AC_PREFIX_DEFAULT([/usr])

AC_ARG_WITH(config, [AS_HELP_STRING([--with-config],[Path to directory of configuration file])], [SHORTLINKDIR=$withval], [])

datarootdir="${prefix}/share"
PIXDIR=$datarootdir/pixmaps
AS_IF([ test x$SHORTLINKDIR = x ], [SHORTLINKDIR=$datarootdir/applications/$PACKAGE_TARNAME])
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

AC_ARG_WITH([debug],
    [AS_HELP_STRING([--with-debug],[ =full for debugging, =no for Release build ])],
    [
       AS_IF([ test "x$withval" = "xno"],
        [
          AS_IF([ test x$ac_cv_c_compiler_gnu = xyes ],
           [
             CFLAGS="-g0 -O3"
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
            CFLAGS="-std=c99 -g3 -O2 -Wall -Wextra -pedantic"
	    cflags_save=`echo $cflags_save | $SED "s/-g \(.*\)-O2/ \1/g"` ])
           ],
           [
	    CFLAGS="-O2 $withval"
           ])

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
DVDA_CHECK_DEPENDENCIES

#=======================================
# Checks for programs/functions/compiler
#=======================================

#obsolescent function checks have been retrieved from autoscan output
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O
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
     [AC_DEFINE([ALWAYS_INLINE], [1], [Always inline when ALWAYS_INLINE is set])])
 ])


AS_IF([ test "x$ac_cv_prog_cc_stdc" = "xno" ],[AC_MSG_WARN([C compiler does not support ISO C99 standard])])


AC_C99_FLEXIBLE_ARRAY
AC_C99_SIZE_VARIABLE_STATIC_ARRAY
AS_IF([ test x$ac_cv_c99_flexible_array != xyes ], [AC_MSG_WARN([Compiler does not support feature known as the flexible array struct member])])

AS_IF([ test x$ac_cv_c99_size_variable_static_array != xyes ],  [AC_MSG_WARN([Compiler does not support size-variable static arrays])])

AC_C_VARARRAYS
AC_C_CONST
AC_C_INLINE

# Determine endian-ness of target processor.

AC_C_FIND_ENDIAN

AC_DEFINE_UNQUOTED(CPU_IS_BIG_ENDIAN, ${ac_cv_c_big_endian},
    [Target processor is big endian.])
AC_DEFINE_UNQUOTED(CPU_IS_LITTLE_ENDIAN, ${ac_cv_c_little_endian},
    [Target processor is little endian.])


AS_IF([ test x${ac_cv_c_big_endian} = xyes ],[ CFLAGS="$CFLAGS   -DCPU_IS_BIG_ENDIAN"],[ CFLAGS="$CFLAGS  -DCPU_IS_LITTLE_ENDIAN"])


#========================================
# Checks for libraries, headers and types
#========================================

AC_HEADER_DIRENT
AC_HEADER_ASSERT
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([errno.h sys/stat.h time.h inttypes.h locale.h stdint.h stdlib.h string.h sys/time.h unistd.h fcntl.h])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_C_CONST
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T

AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([fpos_t], [AC_MSG_RESULT([fpos_t exists on platform])],[AC_MSG_WARN([WARNING: fpos_t does not exists on platform])])

AC_SYS_LARGEFILE
AC_DEFINE([_FILE_OFFSET_BITS], [64], [Large file support enabled])


m4_map([AC_SUBST],[[[ROOTDIR]],[[LIBSOXCONVERT_CPPFLAGS]],[[DEBUG_FLAGS]],[[SHORTLINKDIR]], [[PIXDIR]],[[MENUDIR]]])


# correspondence between X_BUILD shell variables and subdirectory names. Could be avoided with lesser flexibility in naming conventions.


AC_MSG_NOTICE([Configuring ${PACKAGE_TARNAME}...])
AC_CONFIG_FILES([Makefile
         dvda-author.desktop
         dvda-author-shared.spec
         dvda-author.spec
         dvda-author.nsi
         src/Makefile
         src/include/Makefile
         libutils/Makefile
         libutils/src/Makefile
         libutils/src/include/Makefile
         libutils/src/private/Makefile
         libfixwav/Makefile
         libfixwav/src/Makefile
         libfixwav/src/include/Makefile
         CB_project/Makefile
         CB_project/dvda-author/Makefile
         CB_project/libc_utils/Makefile
         CB_project/libfixwav/Makefile
         CB_project/libats2wav/Makefile
         libiberty/Makefile
         libiberty/src/Makefile
         libiberty/src/include/Makefile
         libats2wav/Makefile
         libats2wav/src/Makefile
         libats2wav/src/include/Makefile
         images/Makefile
         script
         script1
         script2
         script3
         script4
         script5
         COREBUILD
         EXAMPLES
])



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
                    fixwav..........$(echo $FIXWAV_LIB)
                    flac............$(echo $FLAC_LIB) $FLAC_LINK
                    ogg.............$(echo $OGG_LIB) $LIBOGG_LINK
                    sox.............$(echo $SOX_LIB) $SOX_LINK

    Build libraries:
                    libFLAC ........$FLAC_BUILD
                    libsox  ........$SOX_BUILD
                    gnu libc .......$IBERTY_BUILD
                    libats2wav .....yes
                    libutils .......yes

    Ogg FLAC support:
                     ...............$OGG_BUILD

    Static sox build:
                     ...............$STATIC_SOX

                  
])


AS_IF([ test x$ac_cv_c_compiler_gnu = xyes ],[
  AC_MSG_RESULT([
  Tools :
         Compiler is GCC : ............. $ac_cv_c_compiler_gnu
])
])

AS_IF([ test x$IBIBERTY_LIB -o x$IBIBERTY_LINK != x ],[AC_MSG_RESULT([  libiberty.a will be linked to])])

AS_IF([ test x$IBERTY_BUILD = xyes ],
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

AS_IF([ test x$SMAKE != x ], [ AC_MSG_RESULT([smake will be used instead of GNU make])
                              make=$SMAKE
                             ],
                             [make="$MAKE"])

AS_IF([ test x$MP2ENC = x -o x$JPEG2YUV = x -o x$MPEG2ENC = x -o x$MPLEX = x],[
 AC_MSG_RESULT([  You will have to author top menus from available MPEG background files, as tools are missing to automate generation.])])


AC_MSG_RESULT([[

  Using $make as a system build tool.

  Auxiliary tools 
                                   installed (path)/build (yes/no)
    
    help2man: .....................$HELP2MAN $HELP2MAN_BUILD
    man2html: .....................$MAN2HTML $MAN2HTML_BUILD
    spumux: .......................$SPUMUX $DVDAUTHOR_BUILD
    dvdauthor: ....................$DVDAUTHOR $DVDAUTHOR_BUILD
    mkisofs: ......................$MKISOFS $CDRTOOLS_BUILD
    cdrecord: .....................$CDRECORD $CDRTOOLS_BUILD
    mp2enc: .......................$MP2ENC $MJPEGTOOLS_BUILD
    jpeg2yuv: .....................$JPEG2YUV $MJPEGTOOLS_BUILD
    mpeg2enc: .....................$MPEG2ENC $MJPEGTOOLS_BUILD
    mpeg2dec: .....................$MPEG2DEC $MPEG2DEC_BUILD
    a52dec: .......................$A52DEC $A52DEC_BUILD
    mplex: ........................$MPLEX $MJPEGTOOLS_BUILD
    mogrify:.......................$MOGRIFY $MAGICK_BUILD
    convert:.......................$CONVERT $MAGICK_BUILD
    lplex: ........................$LPLEX $LPLEX_BUILD
   
   Building tools:

    make:..........................$make
    curl: .........................$CURL
    patch:.........................$PATCH

  These tools should all be installed to enable advanced dvda-author features.

  Installation directories :
    prefix is: ....................$prefix
    Library directory: ........... $(echo $libdir)
    Program directory: ........... $(echo $bindir)
    Man pages: ................... $datarootdir/man/man1
    Documentation directory: ..... $datarootdir/doc/${PACKAGE_TARNAME}
    Images directory: ............ $(echo $PIXDIR)
    Configuration files: ......... $SHORTLINKDIR
    Images directory: ............ $MENUDIR

    You may now build by typing 'make' and install with 'make install'.

]])

AS_IF([ test x$MAGICK_BUILD = xyes ], [DVDA_INF([MAGICK_MSG])])

