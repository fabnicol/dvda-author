
# File dvda.m4

# General-purpose extended M4, M4sh and autoconf macros
# These macros do not depend on dvda-author specifics
# and are reusable in other projects.
# ====================================================

# All macros are copyright Fabrice Nicol, 2009
# These macros are part of the dvda-author package
# and are delivered under the same licensing terms.
# --------------------------------------------------



# Recap of symbols and shell variables defined by following macros (using examples)
# ----------------------------------------------------------------
#
# Naming conventions:
#          HAVE_APPNAME_(PATCH/BUILD): const for AC_DEFINE and AM_CONDITIONAL, whether to patch/build APPNAME
#          MAYBE_APPNAME             : shell variable for AC_SUBST, whether to recurse directory corresponding to APPNAME in build tree
#          APPNAME                   : shell variable for AC_SUBST, filepath to APPNAME
#          APPNAME_BUILD             : local (non-AC-SUBST'd) shell variable, whether to build APPNAME from source code (present in pakage or downloaded)
#          APPNAME_LIB               : shell variable for AC_SUBST, library path (user-defined or package-local lib)
#          APPNAME_LINK              : shell variable for AC_SUBST, library link (system-installed lib)
#          WITH_FUNCTION             : const for AM_CONDITIONAL, whether to build or link against FUNCTION=part of core source code or lib
#          WITHOUT_FUNCTION          : const for AC_DEFINE, whether to build or link against FUNCTION=part of core source code or lib
#

#  Use TAB=8 spaces

# DEF symbol				|	Shell variable		| AM conditional		|		value			|			macro
#----------------------------------------------------------------------------------------------------------------------------------------------
# HAVE_DVDAUTHOR_PATCH										   		    1 or not defined		DVDA_TEST_SOFTWARE_VERSION, DVDA_DOWNLAOD > DVDA_ARG_ENABLE_DOWNLOAD
# 									 HAVE_SOX_BUILD			   true/false				CONF_SUBDIRS
# HAVE_SOX_BUILD											   (0|1)				CONF_SUBDIRS
# 						DVDAUTHOR_BUILD						    yes/no				BUILD, DVDA_DOWNLOAD
#						SOX_BUILD						    yes/no				BUILD (>) DVDA_TEST_LIB, DVDA_ARG_WITH,DVDA_CONFIG_EXECUTABLE_INSTALL, DVDA_ARG_ENABLE > DVDA_ARG_ENABLE_DOWNLOAD
# 									 WITH_SOX			   true/false				DVDA_ARG_WITH
#									 HAVE_SOX			   true/false				DVDA_TEST_LIB
# WITHOUT_SOX		                           				                                 (0|1)      		        DVDA_ARG_WITH
#                                              				 HAVE_EXTERNAL_LIB                   true/false				DVDA_ARG_ENABLE
# HAVE_MOGRIFY          										   (0|1)				DVDA_TEST_AUX
# 						*MAYBE_SOX		 				   libsox/				DVDA_ARG_ENABLE > DVDA_ARG_ENABLE_DOWNLOAD, DVDA_TEST_LIB
# DVDAUTHOR												  /usr/bin/dvdauthor	  		DVDA_DOWNLOAD, DVDA_TEST_SOFTWARE_VERSION > DVDA_ARG_ENABLE_DOWNLOAD
# MOGRIFY                                       *MOGRIFY		 				  /usr/bin/mogrify			DVDA_TEST_AUX
#						*SOX_LIB						  /usr/lib/libsox.so			DVDA_TEST_LIB
#						*SOX_LINK		 				  -lsox					DVDA_TEST_LIB
#						enable_sox						   false				DVDA_TEST_LIB if failure to check installed shared lib against test functions (sanitizing)
# 						CURL							  /usr/bin/curl				DVDA_DOWNLOAD > DVDA_ARG_ENABLE_DOWNLOAD
# 						PATCH							  /usr/bin/patch			DVDA_DOWNLOAD > DVDA_ARG_ENABLE_DOWNLOAD
# 						TAR							  /usr/bin/tar				DVDA_DOWNLOAD > DVDA_ARG_ENABLE_DOWNLOAD
#						exitcode											DVDA_RUN
#						errorcode											DVDA_ERR


# starred shell variables are AC_SUBST'ed, others are local to top builddir configure.

m4_include([m4/auxiliary.m4])
m4_include([m4/oggflac-test.m4])


# LOOP_MIRRORS(VERSION,MAIN MIRROR,FILE TYPE [gz|bz2],MD5SUM)
# --------------------------------------------------------------------
# loops over SF_MIRRORLIST=kent,garr,voxel,free_fr, see dependencies.m4
# if all fails resort to autedetection by SF network.
# checks MD5SUMS of downloaded file.

AC_DEFUN([LOOP_MIRRORS],
    [

      filename=bn-$1.tar.$4
      MD5=$5
      exitcode=0
      filestring=m4_bpatsubst([$3],[tp:],[])

      while true
      do

        AS_IF([test [x]$filestring != x -a "$3" != ""],
         [  DVDA_CURL([$3/$filename], [$filename])],
         [
                        m4_foreach([mirror],[SF_MIRRORLIST],
                            [
                                MD5_BREAK([$filename],[$MD5])
                                AS_IF([test $exitcode != 0 -a "$3" != ""],
                                                  [
                                                        AC_MSG_NOTICE([Connecting to mirror:]mirror[...])
                                                        # This mirroring is Sourceforge-specific and should be twisted for other mirroring patterns.
                                                        DVDA_CURL([http://sourceforge.net/projects/root/files/$3/$filename/download?use_mirror=]mirror,[$filename])

                                                        # eg:
                                                        #     http://sourceforge.net/projects/mjpeg/files/mjpegtools/1.9.0/mjpegtools-1.9.0.tar.gz/download?use_mirror=kent
                                                 ],
                                                 [break])
                            ])
         ])

        MD5_BREAK([$filename],[$MD5])

        # last resort attempt, if everything has failed, use the Sourceforge network, except for cdrtools:

        AS_IF([test $exitcode != 0 -a "$3" != ""],
           [
              AC_MSG_NOTICE([MD5SUM: not equal to  $MD5, downloading however from network...])

              AS_IF([test bn != cdrtools], [DVDA_CURL([http://downloads.sourceforge.net/project/root/$3/$filename],[$filename])])
          ])

        break
      done
   ])

# DVDA_DOWNLOAD(BASENAME[-PATCH],VERSION,SITE, PATCHPATH,[POST-ACTION])
# --------------------------------------------------------------------
# Downloads BASENAME-VERSION.tar.gz/bz2 from SITE, untars and decompress it, then optionally patch it
# with BASENAME-PATCH-VERSION under PATCHPATH if -patch suffix used
# Resets variable BASENAME_PATCH to "no" if operation failed.

AC_DEFUN([DVDA_DOWNLOAD],
[
  m4_pushdef([bn], basename([$1]))
  m4_pushdef([upper], [upperbasename([$1])])
  m4_pushdef([root], [$4])
  errorcode=0


  # It is necessary to use a macro here, as there is an unfortunate hyphen in project name!

  m4_pushdef([site],[$2])
  AS_IF([test [x]m4_bpatsubst([$1],[patch],[]) = x$1],[patchbool=0],[patchbool=1])

  # not having tar may sometimes happen on lightweight windows-based platforms


  AC_PATH_PROG([TAR], [tar], [], [$bindir:/bin:/sbin:/usr/bin:/usr/local/bin])
  AS_IF([ test x$TAR = x],[
                           DVDA_ERR([tar is requested, please install it.])
                           AS_EXIT
                          ])
  AC_PATH_PROG([PATCH], [patch], [], [$bindir:/bin:/sbin:/usr/bin:/usr/local/bin])
  AS_IF([ test x$PATCH = x],[
                              DVDA_ERR([patch is requested, please install it.])
                              AS_EXIT
                            ])


  AC_PATH_PROG([CURL], [curl], [], [$bindir:/usr/bin:/usr/local/bin])



    AS_IF([test x$CURL = x],
          [
            DVDA_ERR([Install curl to download bn and rerun])
            AS_EXIT
          ],
          [
            DVDA_INF([Downloading bn. Make sure you have a functional internet connection.])

            version=m4_argn(1,$2)m4_argn(2,$2)
            upper[_VERSION]=$version
            DVDA_CLEAN([bn-$version.tar.gz])
            DVDA_CLEAN([bn-$version.tar.bz2])
            DVDA_CLEAN([bn-$version.tar.xz])

            type=gz
            LOOP_MIRRORS([$version],[$2],[$5],[$type],[$6])

            # outputs variable $filename and $exitcode

            AS_IF([ test  $exitcode != 0 -a "$5" != "" -a "$2" != "" -a "$6" != ""],
            [
             type=bz2
             LOOP_MIRRORS([$version],[$2],[$5],[$type],[$6])
            ])

            AS_IF([ test  $exitcode != 0 -a "$5" != "" -a "$2" != "" -a "$6" != ""],
            [
             type=xz
             LOOP_MIRRORS([$version],[$2],[$5],[$type],[$6])
            ])

            dir="bn[-]m4_argn(1,$2)"

            AS_IF([ test  $exitcode != 0 -a "$5" != "" -a "$2" != "" -a "$6" != ""],
              [
                DVDA_ERR([Download failure])
                AS_EXIT
              ],
              [

               AS_IF([test -d  $dir],
                [
                 DVDA_INF([Removing $dir])
                 rm -rf $dir
                ])
              ])

            AS_IF([test -f "$filename"],
             [
               AS_IF([test x$type = xgz],[mode=xzvf],[AS_IF([test x$type = xxz],[mode=xJvf],[AS_IF([test x$type = xbz2],[mode=xjvf])])])
               DVDA_TAR([$filename],[$mode])
               [MAYBE_]upper=$dir

               AS_IF([test "$exitcode" = "0"],
                 [
                    AS_IF([test "$patchbool" = "1"],
                       [
                            # cdrtools is Makefile-based whilst autotools-compliant packages are configure-based
                             AS_IF([test -f "$dir/Makefile" || test -f "$dir/configure"],
                               [
                                  DVDA_INF([cdrtools specific procedure...])
                                  m4_popdef([site])
                                  m4_pushdef([site],[$3])
                                  AS_IF([test "$patchbool" = "1"],
                                   [
                                    DVDA_CURL([site/$1-$version],[$1-$version])
                                    DVDA_PATCH([$1-$version])
                                   ],
                                   [
                                    DVDA_ERR([$1 needs patching])
                                    AS_EXIT
                                  ])
                              ])
                       ],
                       [
                             DVDA_INF([No patching was performed])
                       ])
                ],
                [  DVDA_INF([Proceeding to next package...]) ]
                )

             ],
             [DVDA_ERR([Extraction of bn failed])])
           ])


AS_IF([test "$errorcode" = "1"],[errorcode_boolean=0],[errorcode_boolean=1])
AS_IF([test "$errorcode" = "1"],[uppernormalisename([$1])[_BUILD]=no],[uppernormalisename([$1])[_BUILD]=yes])
AC_DEFINE_UNQUOTED(upper, ["${prefix}/bin/bn"], [Defining ]bn[ filepath.])
AC_DEFINE_UNQUOTED([HAVE_]upper, [$errorcode_boolean], [Whether ]bn[ source code will be downloaded for build.])
AC_SUBST(upper[_VERSION])
m4_popdef([site])
m4_popdef([bn])
m4_popdef([upper])
m4_popdef([root])
])#DVDA_DOWNLOAD

# DVDA_TEST_SOFTWARE_VERSION(SOFTWARE[-PATCH])
# --------------------------------------------
# find path to SOFTWARE
# test if software --version has "-patched" in its output
# define HAVE_NO_SOFTWARE_PATCH otherwise
# define  SOFTWARE as path to SOFTWARE both as shell variable and in config.h

AC_DEFUN([DVDA_TEST_SOFTWARE_VERSION],
[
software_path=
m4_pushdef([bn],[basename([$1])])
m4_pushdef([SOFTWARE],[upperbasename([$1])])
AC_MSG_NOTICE([Testing ]bn[ version...])
AC_PATH_PROG(SOFTWARE_PATH, [bn], [], [$bindir:/usr/bin:/usr/local/bin])

#Now variable 'SOFTWARE_PATH' is the filepath to the application
AS_IF([test x$SOFTWARE_PATH != x],
  [
    testchain=$($SOFTWARE_PATH --version 2>&1 | grep "patched")
    AC_MSG_NOTICE([tested: $SOFTWARE_PATH --version 2>&1 | grep "patched" --> $testchain])
    AS_IF([test x"$testchain" != x],
     [
       DVDA_INF([Patched version of bn is installed in path $SOFWARE_PATH])
       SOFTWARE="$SOFTWARE_PATH"
     ],
     [
       DVDA_INF([Installed version of bn is not patched])
       AC_DEFINE([HAVE_NO_]SOFTWARE[_PATCH], [1], [Does not use a configured patch of bn])
     ])
  ],
  [DVDA_INF([Installed version of bn is not patched])]
)
# do not forget C-language quotes here
AC_DEFINE_UNQUOTED(SOFTWARE, "$SOFTWARE_PATH", [Defining bn filepath])
m4_popdef([bn])
m4_popdef([SOFTWARE])
]) #DVDA_TEST_SOFTWARE_VERSION



# DVDA_ARG_ENABLE_DOWNLOAD(FEATURE[-PATCH],VERSION,SITE_OF_PACKAGE,SITE_OF_PATCH,[POST-ACTION])
# ---------------------------------------------------------------------------------------------
# Enables download of file named FEATURE-VERSION.tar.gz or .tar.bz2 (automatic detection)
# from SITE.
# Adds variables defined in DVDA_DOWNLOAD or DVDA_TEST_SOFTWARE_VERSION:

AC_DEFUN([DVDA_ARG_ENABLE_DOWNLOAD],
[
 DVDA_ARG_ENABLE([$1],
   [
    AS_IF([test x$1 = xall-deps -o x$1 = xall-all],
            [m4_map([DVDA_DOWNLOAD],[DOWNLOAD_OPTIONS])],
          [test x$1 != xno],
            [DVDA_DOWNLOAD($@)])

   ])

 AS_IF([test $1 = dvdauthor-patch && test x$DVDAUTHOR_PATCH = x],[DVDA_TEST_SOFTWARE_VERSION([$1])])

 AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1],msg)],
  [
   AS_IF([ test x$enableval != xno],
    [
     $2
     DVDA_INF([Will msg... ])
     upper=yes
    ],
    [
     DVDA_INF([Will not msg... ])
     m4_ifvaln([$3], [$3])dnl
     upper=no
    ])
  ])
 ])#DVDA_ARG_ENABLE_DOWNLOAD


# ===== redefine AC_ARG_ENABLE incorporating shreds of AC_HELP_STRING, see autoconf/general.m4 ========= #

# DVDA_ARG_ENABLE(feature,ACTION-IF-YES, ACTION-IF-NO)
# -------------------------------------
# Like AC_ARG_ENABLE yet with more concise syntax and optional ACTION-IF-YES/NO instead of GIVEN/NOT-GIVEN
# not given assimilated to NO
# Add definition for variable FEATURE=yes/no in both cases, yes if --enable-feature, no otherwise
# Compute HELMSG from FEATURE
# Add AC_DEFINE_UNQUOTED([HAVE_FEATURE], [0|1], [HELPMSG])
# Add AC_HELP_STRING([--enable-feature], [HELPMSG])
# Add verbosity to yes/no result
# Add AM_CONDITIONAL(HAVE_FEATURE,  YES/NO-TEST) with variable name (non-standard)

AC_DEFUN([DVDA_ARG_ENABLE],
[
m4_pushdef([dhms],[dehyphenate([$1])])
m4_pushdef([act],  suffix([$1]))
m4_pushdef([bn],   [basename([$1])])
m4_pushdef([norm], [normalise([$1])])
m4_pushdef([upper],[uppernormalisename([$1])])

m4_if(act,[build],
       [m4_pushdef([msg],[[configure, build and install ]bn[ from source code]])],
          act,[builds],
       [m4_pushdef([msg],[[configure, build and install all core dependencies <FLAC, Ogg, SoX, dvdauthor, cdrtools> from source code]])],
          act,[patch],
       [m4_pushdef([msg],[[download and patch ]bn[ from source code]])],
          act,[download],
       [m4_pushdef([msg],[[download ]bn[ from source code]])],
          act,[cvs],
       [m4_pushdef([msg],[[download ]bn[ (cvs code for windows builds)]])],
          act,[deps],
       [m4_pushdef([msg],[[download ]bn[ dependencies <FLAC, Ogg, SoX, dvdauthor, cdrtools> and patch the source code if necessary]])],

           [m4_pushdef([msg],[[enable ]dhms])])

# Check whether --enable-$1 or --disable-$1 was given.

AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1],msg)],
[
  if test x$enableval != xno; then
   $2
   DVDA_INF([Will msg... ])
   upper=yes
  else
   DVDA_INF([Will not msg... ])
   m4_ifvaln([$3], [$3])dnl
   upper=no
  fi
 ]
)


# We get AC_DEFINE out of the first yes test higher up because scripts passed along in arg3 may have result status that
# reset $enableval to "no"

AS_IF([test x$enableval = xyes],[enableval_boolean=1],[enableval_boolean=0])
AC_DEFINE_UNQUOTED([HAVE_]upper,[$enableval_boolean],msg)
AM_CONDITIONAL([HAVE_]upper,[test $enableval_boolean = 1 ])
m4_popdef([msg])
m4_popdef([norm])
m4_popdef([upper])
m4_popdef([act])
m4_popdef([dhms])
])# DVDA_ARG_ENABLE


# =====  DVDA_TEST_AUX ========= #
# --------------------------------
# DVDA_TEST_AUX(filename, Message)
# -------------------------------------
# Checks whether filename is a reachable file (AC_PATH_PROG) and defines HAVE_FILENAME and FILENAME as its filepath


AC_DEFUN([DVDA_TEST_AUX],
[
m4_pushdef([CAPNAME],[uppernormalisename([$1])])
#don't quote here
AC_PATH_PROG(CAPNAME, [$1],[], [$bindir:/usr/bin:/usr/local/bin])
AS_IF([test x$CAPNAME = x ],
  [
   DVDA_INF([No $2])
   auxbool=0
  ],
  [
   DVDA_INF([$2])
   auxbool=1
  ])
AC_DEFINE_UNQUOTED([HAVE_]CAPNAME, [$auxbool], [Found $1])
AC_DEFINE_UNQUOTED(CAPNAME, "$CAPNAME", [Pathname of $1])
AM_CONDITIONAL([HAVE_]CAPNAME, [test $auxbool = 1])
m4_popdef([CAPNAME])

])#DVDA_TEST_AUX


#unquote!


# BUILD(LIBBASENAME)
# -------------------------------------
# "build LIBBASENAME" message
# Add   LIBBASENAME_BUILD=yes

AC_DEFUN([BUILD],
     [
      DVDA_INF([Building $1 library from sources...
Triggering --enable-$1-build... ])
      upperbasename($1)[_BUILD]=yes
     ])#BUILD


# PROFILE_LD(LIBNAME,INPUT_LIB)
# -----------------------------
# set search paths for -I and -L before testing lib/header existence
# extract lib basename (e.g. flac) from libname (e.g. libflac) if input lib is given
# add variable TEST for AC_CHECK LIB first argument

AC_DEFUN([PROFILE_LD],[
  CAPNAME=upperbasename([$1])

  AC_MSG_NOTICE([Profiling $CAPNAME...])
  AS_IF([test x$2 = x],
     [LDFLAGS="$LDFLAGS -L/usr/lib -L/usr/local/lib -lm"
      TEST="$1"
      AS_IF([test -d /usr/lib/$1],
       [LDFLAGS="$LDFLAGS -L/usr/lib/$1"],
       [test -d /usr/local/lib],
       [LDFLAGS="$LDFLAGS -L/usr/local/lib/$1"])
      AS_IF([test -d /usr/local/include/$1],
       [CPPFLAGS="$CPPFLAGS -I/usr/local/include/$1"],
       [test -d /usr/include/$1],
       [CPPFLAGS="$CPPFLAGS -I/usr/include/$1"])
      AS_IF([test -d /usr/lib/$CAPNAME],
       [LDFLAGS="$LDFLAGS -L/usr/lib/$CAPNAME"],
       [test -d /usr/local/lib],
       [LDFLAGS="$LDFLAGS -L/usr/local/lib/$CAPNAME"])
      AS_IF([test -d /usr/local/include/$CAPNAME],
       [CPPFLAGS="$CPPFLAGS -I/usr/local/include/$CAPNAME"],
       [test -d /usr/include/CAPNAME],
       [CPPFLAGS="$CPPFLAGS -I/usr/include/$CAPNAME"])
     ],
     [LDADD="$LDADD -lm $2"
      TEST=`echo $2 | sed "s/.*lib\(.*\)\(\.\).*/\1/"`
      DVDA_INF([Looking for functions in installed $TEST...])
     ])
])

# This function tests for installed SoX libraries and triggers building of source files if not available
#
#NOTE: syntax change: --with-sox=sox --exec-prefix=/usr/local   not --with-sox=/usr/local/lib/libsox.so  as with prior versions (<09.07)
#

# DVDA_TEST_LIB(BASELIBNAME, [LIBINPUT], [[[LIBHEADER],[FUNCTIONS-TO-TEST]] ...],BUILDLIBPATH,[shared])
# --------------------------------------------------------------------------------
# Test library by basename (e.g. flac) or input library pathname (if given), against LIBHEADER and FUNCTIONS-TO-TEST
# FUNCTIONS-TO-TEST is a space-separated list of unquoted functions to be tested, possibly reduced to [function]
# Add UPPERBASENAME_LIB shell variable
# Add UPPERBASENAME_BUILD=yes/no shell variable
# Add HAVE_EXTERNAL_UPPERBASENAME as AM conditional if lib is given as input
# If shared is added, only link to shared .so library under exec_prefix/lib/lib(basename).so, unless LIBINPUT is given (default for sox)
# If not found, disable lib capability by setting  enable_basename=false as a shell variable.
# Invoke DVDA_CONFIG_EXECUTABLE_INSTALL before

AC_DEFUN([DVDA_TEST_LIB],
[
PROFILE_LD([$1],[$2])

# we cannot resort to shell variable CAPNAME as prefix here, defining a macro is necessary
# remember m4_car and m4_cdr should be used unquoted

m4_pushdef([UPPERBASENAME],[upperbasename([$1])])
m4_pushdef([CHECKLIST],[m4_normalize([$3])])
m4_foreach([LIST], [CHECKLIST],
[
  m4_pushdef([FUNCTIONLIST],m4_cdr(LIST))
  ## Four levels of brackets are needed: one is stripped, followed by the quotes of the list followed by quotes for pair groups, followed by header quotes or function list quotes
  ## FLAC_LIB etc must have been computed before

 AC_CHECK_HEADERS(m4_car(LIST),
  [
   AC_CHECK_LIB([$TEST], car_w(FUNCTIONLIST),
    [

       m4_foreach_w([VAR], cdr_w(FUNCTIONLIST), [AC_CHECK_LIB([$TEST], [VAR], [], [BUILD([$1])])])
       AS_IF([test x$2 = x],
        [
         UPPERBASENAME[_LINK]="-l$1"
         m4_ifvaln([$4],[
         # oddly AS_CASE did not work here
         AS_IF( [test $4 = shared],
                [
                 UPPERBASENAME[_LIB]="${prefix}/lib/lib$1.so"
                  AS_IF([test -f $UPPERBASENAME[_LIB]],
                      [ DVDA_INF([Using installed dynamic lib$1 library...])

                      ]
                      ,[ DVDA_ERR([Could not find UPPERBASENAME lib, retry with --libdir=DIR, root directory for $1 lib])
                         UPPERBASENEME[_LIB]=
                       ])
                ],
                [test $4 = static],
                [
                UPPERBASENAME[_LIB]="${prefix}/lib/lib$TEST.a"
                  AS_IF([test -f $UPPERBASENAME[_LIB]],
                      [ DVDA_INF([Using installed static lib$1 library...])]
                      ,[ DVDA_ERR([Could not find UPPERBASENAME lib, retry with --libdir=DIR, root directory for $1 lib])
                         UPPERBASENAME[_LIB]=
                       ])
                ])])

        DVDA_INF([Using installed [lib]$1 library...])
       ],
       [
        DVDA_INF([Using specified [lib]$1 library...])
        UPPERBASENAME[_LIB]="$2"
        UPPERBASENAME[_BUILD]=no
       ])
    ],
    [
      BUILD([$1])
      AC_MSG_NOTICE([$4 was not found in $TEST])
    ])
 ],
 [
   BUILD([$1])
   AC_MSG_NOTICE([No appropriate headers for $1])
 ])
 m4_popdef([FUNCTIONLIST])
])dnl

 # automake conditionals will depend on possible --without features so should be placed in configure.ac


 #m4_popdef([LIST])


 m4_popdef([UPPERBASENAME])
]) #DVDA_TEST_LIB



# DVDA_ARG_WITH invocation should always be inserted in configure.ac after DVDA_ARG_ENABLE
# because --enable-lib-build triggers LIB_BUILD=yes, which makes it useless to test system (or user's) lib against checks
# as is done by DVDA_TEST_LIB

AC_DEFUN([DVDA_ARG_WITH],
[
m4_pushdef([CAPNAME],[upperbasename([$1])])
m4_pushdef([lower],m4_tolower([$1]))
AS_IF([test x$[withval_]CAPNAME != xno],[
AC_ARG_WITH([lower], [AS_HELP_STRING([--with-]lower,[full pathname of library or --without-]lower)],
   [
    [withval_]CAPNAME=$withval
    AS_IF([test x$withval = xno],
          [
           AC_DEFINE([WITHOUT_]CAPNAME,[1],[Disables $lower support])
           CAPNAME[_BUILD]=no
          ],
          [test x$withval != xyes],
          [AC_MSG_NOTICE([Using specified ]lower[ lib: $withval])
           CAPNAME[_LIB_INPUT]=$withval])
   ],
   [
     [withval_]CAPNAME=
     CAPNAME[_LIB_INPUT]=
   ])
],
[
   AC_DEFINE([WITHOUT_]CAPNAME,[1],[Disables $lower support])
   CAPNAME[_BUILD]=no
])

# do not simply use the withval variable as --without-X options might interfere globally

AS_IF([test x$CAPNAME[_BUILD] != xyes && test x$[withval_]CAPNAME != xno],
 [DVDA_TEST_LIB([$1],[$CAPNAME[_LIB_INPUT]],$2,$3,$4,$5)])

# whether lib has not been deactivated by --without-lib
AM_CONDITIONAL([WITH_]CAPNAME, [test x$[withval_]CAPNAME != xno])

# whether linking to installed lib with --with-lib=/full/path/to/lib
AM_CONDITIONAL([HAVE_EXTERNAL_]CAPNAME, [test x$[withval_]CAPNAME != x])

# whether configure automatically found valid system link
AM_CONDITIONAL([HAVE_]CAPNAME, [test x$CAPNAME[_LINK] != x ])
])dnl

#CONF_SUBDIRS([X_BUILD names],[DIRNAMES])
#---------------------------------------
# Parse in first white space separated list all X_BUILD shell variables that indicate build/no build of code sections (whether configured or not).
# Associate dirname in corresponding rank of white space separated list of dirnames, copied in MAYBE_DIRNAME
# AC_SUBST MAYBE_DIRNAME
# constraint on labelling is: libs should be created under MAYBE_VAR/src/MAYBE_VAR.a


AC_DEFUN([DVDA_CONFIG],[

    m4_foreach([ALIST],[$1],[
    m4_pushdef([LIST],m4_car(ALIST))
    m4_pushdef([FL], m4_unquote(m4_cdr(ALIST)))
    m4_pushdef([VAR],m4_car(LIST))
    m4_pushdef([CDR],m4_unquote(m4_cdr(LIST)))


    AS_IF([test x$VAR[_BUILD] = xyes || test x$ALL_BUILDS = xyes -a x$[withval_]VAR != xno],
           [
              [MAYBE_]VAR=CDR
              VAR[_BUILD]=yes
              VAR[_CONFIGURE_FILE]="[$MAYBE_]VAR"/configure

              m4_ifvaln([$2],[$2],[VAR[_LIB]="\${ROOTDIR}[/local/lib/lib]m4_tolower(cut_lib_prefix(VAR))[.a]"]) #do not quote VAR. It is necessary to lower case as base names are uniform

              [CONFIGURE_]VAR[_FLAGS]="FL $VAR[_FLAGS]"
              AC_SUBST([CONFIGURE_]VAR[_FLAGS])
              AC_MSG_NOTICE([CONFIGURE_]VAR[_FLAGS]=$[CONFIGURE_]VAR[_FLAGS])
              AS_IF([test -d  $ROOTDIR/$[MAYBE_]VAR && ! test -d  $[MAYBE_]VAR ], [cp -r $ROOTDIR/$[MAYBE_]VAR  $PWD])
           ])

    AM_CONDITIONAL([HAVE_]VAR[_BUILD], [test x$VAR[_BUILD] = xyes || test x$ALL_BUILDS = xyes])
    AC_SUBST([MAYBE_]VAR)
    AC_SUBST(VAR[_CONFIGURE_FILE])
    AC_SUBST(VAR[_LIB])
    AC_SUBST(VAR[_LINK])
    m4_popdef([VAR])
    m4_popdef([CDR])
    m4_popdef([FL])
    m4_popdef([LIST])
    ])])dnl

AC_DEFUN([DVDA_CONFIG_EXECUTABLE_INSTALL],               [DVDA_CONFIG([$1],[#executable_install])])
AC_DEFUN([DVDA_CONFIG_LIBRARY_NO_INSTALL],     [DVDA_CONFIG([$1],[VAR[_LIB]="\${top_builddir}/[$MAYBE_]VAR/src/[$MAYBE_]VAR.a"])])
AC_DEFUN([DVDA_CONFIG_LIBRARY_LOCAL_INSTALL], [DVDA_CONFIG([$1],[]) ])


AC_DEFUN([DVDA_PREFIX_DEFAULT],
 [ AC_PREFIX_DEFAULT([$1])
   AS_IF([test $prefix = NONE],[prefix=$ac_default_prefix])])

# All above macros are copyright Fabrice Nicol, 2009
# These macros are part of the dvda-author package
# and are delivered under the same licensing terms.
# --------------------------------------------------

