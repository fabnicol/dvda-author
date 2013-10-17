# File dvda.m4
# General-purpose extended M4, M4sh and autoconf macros
# These macros do not depend on dvda-author specifics
# and are reusable in other projects.
# ====================================================

# All macros are copyright Fabrice Nicol, 2009-2013
# These macros are part of the dvda-author package
# and are delivered under the same licensing terms.
# --------------------------------------------------


m4_include([m4/oggflac-test.m4])


# LOOP_MIRRORS(VERSION,MAIN MIRROR,FILE TYPE [gz|bz2],MD5SUM)
# --------------------------------------------------------------------

AC_DEFUN([LOOP_MIRRORS],
    [
      Version=$1
      Sourceforge_url=$2
      Other_url=$3
      Type=$4
      Md5=$5

      Filename=bn[-]$Version.tar.$Type

      echo Using 'filename' as $Filename

      exitcode=0

      m4_ifvaln([bn],
      [
	AS_IF([test "$Sourceforge_url" != "" ],
		       [DVDA_CURL([$Sourceforge_url/$Filename], [$Filename])],
		       [
			AS_IF([test "$Other_url" != ""],
			     [DVDA_CURL([$Other_url/$Filename], [$Filename])],
			     [
			       AC_MSG_NOTICE([No repository was provided for bn... Please download by other means.])
			       AS_EXIT
			     ])
		       ])
      ],
      [
	AC_MSG_NOTICE([Issue with downloading: no filename ... Please download by other means.])
	AS_EXIT
      ])

      MD5_BREAK([$Filename],[$Md5])

      AS_IF([test $exitcode != 0],
	   [
	      AC_MSG_NOTICE([[MD5SUM: not equal to  $md5, trying again fallback Url $Other_url]])
	      AS_IF([test "$Other_url" != ""],[DVDA_CURL([$Other_url/$Filename], [$Filename])])
	   ])




   ]) #LOOP_MIRRORS


# DVDA_DOWNLOAD(BASENAME[-PATCH],VERSION,SITE, PATCHPATH,[POST-ACTION])
# --------------------------------------------------------------------
# Downloads BASENAME-VERSION.tar.gz/bz2 from SITE, untars and decompress it, then optionally patch it
# with BASENAME-PATCH-VERSION under PATCHPATH if -patch suffix used
# Resets variable BASENAME_PATCH to "no" if operation failed.

AC_DEFUN([DVDA_DOWNLOAD],
[
  m4_define([bn], basename([$1]))
  m4_define([upper], [upperbasename([$1])])
  m4_define([version],[$2])
  m4_define([patch_url],[$3])
  m4_define([sourceforge_url],[$4])
  m4_define([other_url],[$5])
  m4_define([MD5],[$6])
  m4_define([filename], [bn[-]version])

  errorcode=0

  AS_IF([test [x]m4_bpatsubst([$1],[patch],[]) = x$1],[patchbool=0],[patchbool=1])

  # not having tar may sometimes happen on lightweight windows-based platforms

    AS_IF([test x$CURL = x],
	  [
	    DVDA_ERR([Install curl to download bn and rerun])
	    AS_EXIT
	  ],
	  [
	    DVDA_INF([Downloading bn. Make sure you have a functional internet connection.])
	    upper[_VERSION]=version
	    DVDA_CLEAN([filename.tar.gz])
	    DVDA_CLEAN([filename.tar.bz2])
	    DVDA_CLEAN([filename.tar.xz])

	    LOOP_MIRRORS([version],[sourceforge_url],[other_url],[gz],[MD5])
	    mode=xzf
	    type=gz

	    AS_IF([test $exitcode != 0],
		    [
		      exitcode=0
		      LOOP_MIRRORS([version],[sourceforge_url],[other_url],[bz2],[MD5])
		      mode=xjf
		      type=bz2
		    ])

	    AS_IF([test $exitcode != 0],
		    [
		      exitcode=0
		      LOOP_MIRRORS([version],[sourceforge_url],[other_url],[xz],[MD5])
		      mode=xJf
		      type=xz
		    ])

	    AS_IF([ test  $exitcode != 0 ],
	      [
		DVDA_ERR([Download failure])
		AS_EXIT
	      ],
	      [
	       AS_IF([test -d  filename],
		[
		 DVDA_INF([Removing directory filename])
		 rm -rf filename
		])

	       AS_IF([test -d  bn],
		[
		 DVDA_INF([Removing directory bn])
		 rm -rf bn
		])

	      ])

	    AS_IF([test -f filename.tar.$type],
	     [

	       DVDA_TAR([filename.tar.$type],[$mode])


	       [MAYBE_]bn=filename

	       AS_IF([test "$exitcode" = "0"],
		 [
		    AS_IF([test "$patchbool" = "1"],
		       [

			    AS_IF([test "$patchbool" = "1"],
			      [
				DVDA_CURL([patch_url], bn-patch-version)
				DVDA_PATCH([bn-patch-version])
			      ],
			      [
				DVDA_ERR([$1 needs patching])
				AS_EXIT
			      ])

			    AS_IF([test -d filename],
				    [AC_MSG_NOTICE([Extraction: OK])],
				    [
				      AS_IF([test -d bn],
					    [
					      mv -f bn bn-version
					      AS_IF([ test "$?" = "0" ],
						      [AC_MSG_NOTICE([Renamed directory to bn-version])],
						      [
							 AC_MSG_NOTICE([Extraction is not canonical: naming issue])
							 AS_EXIT
						      ])
					    ],
					    [
					      AC_MSG_NOTICE([Extraction is not canonical: naming issue])
					      AS_EXIT
					    ])
				    ])

		       ],
		       [
			     DVDA_INF([No patching was performed])
		       ])
		],
		[  DVDA_INF([Extraction of bn.tar.$type failed. Proceeding to next package...]) ]
		)

	     ],
	     [DVDA_ERR([Download of bn failed])])
	   ])


AS_IF([test "$errorcode" = "1"],[errorcode_boolean=0],[errorcode_boolean=1])
AS_IF([test "$errorcode" = "1"],[basename([$1])[_BUILD]=no],[basename([$1])[_BUILD]=yes])
AC_DEFINE_UNQUOTED(upper, ["${prefix}/bin/bn"], [Defining ]bn[ filepath.])
AC_DEFINE_UNQUOTED([HAVE_]bn, [$errorcode_boolean], [Whether ]bn[ source code will be downloaded for build.])
AC_SUBST(upper[_VERSION])

#m4_popdef([bn])
#m4_popdef([upper])
#m4_popdef([version])
#m4_popdef([patch_url])
#m4_popdef([sourceforge_url])
#m4_popdef([other_url])
#m4_popdef([MD5])
#m4_popdef([filename])

])  #DVDA_DOWNLOAD

# DVDA_TEST_SOFTWARE_VERSION(SOFTWARE[-PATCH])
# --------------------------------------------
# find path to SOFTWARE
# test if software --version has "-patched" in its output
# define HAVE_NO_SOFTWARE_PATCH otherwise
# define  SOFTWARE as path to SOFTWARE both as shell variable and in config.h

AC_DEFUN([DVDA_TEST_SOFTWARE_VERSION],
[
software_path=
m4_define([bn],[basename([$1])])
m4_define([SOFTWARE],[upperbasename([$1])])
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
#m4_popdef([bn])
#m4_popdef([SOFTWARE])
]) #DVDA_TEST_SOFTWARE_VERSION


# DVDA_TEST_MAKE_VERSION
# --------------------------------------------
# find path to MAKE in MAKE_PATH
# test if software --version has "3.82" in its output ore more

AC_DEFUN([DVDA_TEST_MAKE_VERSION],
[
AC_MSG_NOTICE([Testing make version...])
AC_PATH_PROG(MAKE_PATH, [make], [], [$bindir:/bin:/usr/bin:/usr/local/bin])
#caution: quote [...]  regep square brackets
AS_IF([test x$MAKE_PATH != x],
  [
    testchain=$($MAKE_PATH -v | grep -E [3\.[8-9]{1}[2-9]{1}])
    AC_MSG_NOTICE([tested: whether version of $MAKE_PATH is 3.82+ ])
    AS_IF([test x"$testchain" != x],
     [
       DVDA_INF([Version of make is 3.82+: $testchain])
       MAKE="$MAKE_PATH"
     ],
     [
       DVDA_INF([Installed version of make is not 3.82+: $testchain])
       AS_EXIT
     ])
  ],
  [
    DVDA_INF([Installed version of make is not upgraded])
    AS_EXIT
  ]
)

]) #DVDA_TEST_SOFTWARE_VERSION


# DVDA_ARG_ENABLE_DOWNLOAD
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
	  [test x$1 = xminimal-deps],
	    [m4_map([DVDA_DOWNLOAD],[DOWNLOAD_MINIMAL_OPTIONS])],
	  [test x$1 != xno],
	    [DVDA_DOWNLOAD($@)])

   ])

 AS_IF([test $1 = dvdauthor-patch && test x$DVDAUTHOR_PATCH = x],[DVDA_TEST_SOFTWARE_VERSION([$1])])

 AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1],msg)],
  [
   AS_IF([ test x$enableval != xno],
    [
     DVDA_INF([Will enable $1... ])
    ],
    [
     DVDA_INF([Will disable $1... ])
    ])
  ])
 ]) #DVDA_ARG_ENABLE_DOWNLOAD


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

AC_DEFUN([DVDA_ARG_ENABLE],
[
m4_define([dhms],[dehyphenate([$1])])
m4_define([act],  suffix([$1]))
m4_define([bn],   [basename([$1])])
m4_define([norm], [normalise([$1])])

m4_if(act,[build],
       [m4_define([msg],[[configure, build and install ]bn[ from source code]])],
	  act,[builds],
       [m4_define([msg],[[configure, build and install all core dependencies <FLAC, Ogg, SoX, dvdauthor, cdrtools> from source code]])],
	  act,[patch],
       [m4_define([msg],[[download and patch ]bn[ from source code]])],
	  act,[download],
       [m4_define([msg],[[download ]bn[ from source code]])],
	  act,[cvs],
       [m4_define([msg],[[download ]bn[ (cvs code for windows builds)]])],
	  act,[deps],
       [m4_define([msg],[[download ]bn[ dependencies <FLAC, Ogg, SoX, dvdauthor, cdrtools> and patch the source code if necessary]])],

	   [m4_define([msg],[[enable ]dhms])])

# Check whether --enable-$1 or --disable-$1 was given.
echo "++++++++" doing $1: bn with enableval=$enableval

AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1],msg)],
[
  AS_IF([test x$enableval != xno],
  [
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

   $2
   DVDA_INF([Will msg... ])
   BUILD(bn)
   [HAVE_]bn[_BUILD]=yes
   indic=yes
  ],
  [
   DVDA_INF([Will not msg... ])
   m4_ifvaln([$3], [$3])
   [HAVE_]bn[_BUILD]=no
   indic=no
  ])
],
[
 [HAVE_]bn[_BUILD]=""
 indic=""
]
)

AS_IF([test "$indic" = "yes"],[indic_boolean=1],[indic_boolean=0])

AC_DEFINE_UNQUOTED([HAVE_]bn[_BUILD],[$indic_boolean],msg)

AC_SUBST([HAVE_]bn[_BUILD])

#m4_popdef([msg])
#m4_popdef([norm])
#m4_popdef([upper])
#m4_popdef([act])
#m4_popdef([bn])
#m4_popdef([dhms])
])# DVDA_ARG_ENABLE


# =====  DVDA_TEST_AUX ========= #
# --------------------------------
# DVDA_TEST_AUX(filename, Message)
# -------------------------------------
# Checks whether filename is a reachable file (AC_PATH_PROG) and defines HAVE_FILENAME and FILENAME as its filepath


AC_DEFUN([DVDA_TEST_AUX],
[

m4_define([CAPNAME],[uppernormalisename([$1])])
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

AC_DEFINE_UNQUOTED([HAVE_]$1, [$auxbool], [Found $1])
AC_DEFINE_UNQUOTED(CAPNAME, "$CAPNAME", [Pathname of $1])
AS_IF([test $auxbool = 1],[[HAVE_]$1=yes],[[HAVE_]$1=no])
AC_SUBST([HAVE_]$1)
m4_undefine([CAPNAME])
]) #DVDA_TEST_AUX


#unquote!


# BUILD(LIBBASENAME)
# -------------------------------------
# "build LIBBASENAME" message
# Add   LIBBASENAME_BUILD=yes

AC_DEFUN([BUILD],
     [
     m4_define([lower], [m4_tolower($1)])

      DVDA_INF([Building $1 library from sources...
Triggering --enable-$1-build... ])
AS_IF([test "$1" != "" ],
       [
	  AC_MSG_WARN([[Using command line args: $command_line_args ...]])
	  AS_IF([test "$1" = "libfixwav" -o "$1" = "libiberty" -o "$1" = "all-all" -o "$1"="all-deps" -o "$1" = "all-builds" -o `echo "$command_line_args" | sed s/lower//g` != "$command_line_args"],
	      [
		basename($1)[_BUILD]=yes
	      ],
	      [
		AC_MSG_WARN([[Please download $1 or restart configure with --enable-]lower[-download or --enable-]lower[-patch <sox, cdrtools and dvdauthor>]])
		AS_EXIT
	      ])

      ],
      [
	echo Naming error: empty "enable" feature
	sleep 5s
	AS_EXIT
      ])

      #m4_popdef([lower])

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

m4_define([BASENAME],[basename([$1])])
m4_define([CHECKLIST],[m4_normalize([$3])])
m4_foreach([LIST], [CHECKLIST],
[
  m4_define([FUNCTIONLIST],m4_cdr(LIST))
  ## Four levels of brackets are needed: one is stripped, followed by the quotes of the list followed by quotes for pair groups, followed by header quotes or function list quotes
  ## FLAC_LIB etc must have been computed before

 AC_CHECK_HEADERS(m4_car(LIST),
  [
   AC_CHECK_LIB([$TEST], car_w(FUNCTIONLIST),
    [

       m4_foreach_w([VAR], cdr_w(FUNCTIONLIST), [AC_CHECK_LIB([$TEST], [VAR], [], [BUILD([$1])])])
       AS_IF([test x$2 = x],
	[
	 BASENAME[_LINK]="-l$1"
	 m4_ifvaln([$4],[
	 # oddly AS_CASE did not work here
	 AS_IF( [test $4 = shared],
		[
		 BASENAME[_LIB]="${prefix}/lib/lib$1.so"
		  AS_IF([test -f $BASENAME[_LIB]],
		      [ DVDA_INF([Using installed dynamic lib$1 library...])

		      ]
		      ,[ DVDA_ERR([Could not find BASENAME lib, retry with --libdir=DIR, root directory for $1 lib])
			 BASENAME[_LIB]=
			 BASENAME[_LINK]=
		       ])
		],
		[test $4 = static],
		[
		BASENAME[_LIB]="${prefix}/lib/lib$TEST.a"
		  AS_IF([test -f $BASENAME[_LIB]],
		      [ DVDA_INF([Using installed static lib$1 library...])]
		      ,[ DVDA_ERR([Could not find BASENAME lib, retry with --libdir=DIR, root directory for $1 lib])
			 BASENAME[_LIB]=
			 BASENAME[_LINK]=
		       ])
		])])

       ],
       [
	DVDA_INF([Using specified [lib]$1 library...])
	BASENAME[_LIB]="$2"
	BASENAME[_BUILD]=no
	BASENAME[_LINK]=
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
 #m4_popdef([FUNCTIONLIST])
])

#m4_popdef([BASENAME])
#m4_popdef([CHECKLIST])

]) #DVDA_TEST_LIB



# DVDA_ARG_WITH invocation should always be inserted in configure.ac after DVDA_ARG_ENABLE
# because --enable-lib-build triggers LIB_BUILD=yes, which makes it useless to test system (or user's) lib against checks
# as is done by DVDA_TEST_LIB

AC_DEFUN([DVDA_ARG_WITH],
[
m4_define([bn],[basename([$1])])
m4_define([lower],m4_tolower([$1]))
AS_IF([test x$[withval_]bn != xno],[
AC_ARG_WITH([lower], [AS_HELP_STRING([--with-]lower,[full pathname of library or --without-]lower)],
   [
    #given on command line
    [withval_]bn=$withval
    AS_IF([test x$withval = xno],
	  [
	   AC_DEFINE([WITHOUT_]bn,[1],[Disables $lower support])
	   bn[_BUILD]=no
	   [HAVE_EXTERNAL_]bn=no
	   [WITH_]bn=no
	  ],
	  [test x$withval != xyes],
	  [
	   AC_MSG_NOTICE([Using specified ]lower[ lib: $withval])
	   bn[_LIB_INPUT]=$withval
	   [HAVE_EXTERNAL_]bn=yes
	   [WITH_]bn=yes
	  ])
   ],
   [
     #not given on command line
     [withval_]bn=no
      #building libfixwav by default
      AS_IF([test bn != libfixwav ],[bn[_BUILD]=""],[bn[_BUILD]=yes])
      bn[_LIB_INPUT]=""
      [HAVE_EXTERNAL_]bn=no
      [WITH_]bn=""
   ])
],
[
   AC_DEFINE([WITHOUT_]bn,[1],[Disables $lower support])
   bn[_BUILD]=no
  [HAVE_EXTERNAL_]bn=no
  [WITH_]bn=no
])

# do not simply use the withval variable as --without-X options might interfere globally

AS_IF([test x$bn[_BUILD] != xyes && test x$[withval_]bn != xno],
 [DVDA_TEST_LIB([$1],[$bn[_LIB_INPUT]],$2,$3,$4,$5)])

# whether lib has not been deactivated by --without-lib

AC_SUBST([WITH_]bn)
AC_SUBST([HAVE_EXTERNAL_]bn)
AC_SUBST([HAVE_]bn[_LINK])
m4_undefine([bn])
#m4_popdef([bn])
#m4_popdef([lower])

])

#CONF_SUBDIRS([X_BUILD names],[DIRNAMES])
#---------------------------------------
# Parse in first white space separated list all X_BUILD shell variables that indicate build/no build of code sections (whether configured or not).
# Associate dirname in corresponding rank of white space separated list of dirnames, copied in MAYBE_DIRNAME
# AC_SUBST MAYBE_DIRNAME
# constraint on labelling is: libs should be created under MAYBE_VAR/src/MAYBE_VAR.a


AC_DEFUN([DVDA_CONFIG],[

    m4_foreach([ALIST],[$1],[
    m4_define([LIST],m4_car(ALIST))
    m4_define([FL], m4_unquote(m4_cdr(ALIST)))
    m4_define([VAR],m4_car(LIST))
    m4_define([UPPERVAR],m4_toupper(VAR))
    m4_define([CDR],m4_unquote(m4_cdr(LIST)))
    AC_MSG_RESULT([configure:  VAR[_BUILD]=$VAR[_BUILD]])
    AC_MSG_RESULT([configure:  [WITH_]VAR=$[WITH_]VAR])
    AS_IF([test \( x$VAR[_BUILD] = xyes -o x$ALL_BUILDS = xyes \) -a x$[WITH_]VAR != xno ],
	   [
	      AC_MSG_RESULT([configure:  Adding VAR to PROGRAM_TARGETS...])
	      PROGRAM_TARGETS="$PROGRAM_TARGETS VAR"

	      [MAYBE_]VAR=m4_unquote(CDR)
	      VAR[_BUILD]=yes
	      VAR[_CONFIGURE_FILE]="[$MAYBE_]VAR"/configure

	      m4_ifvaln([$2],[$2],
	      [
		VAR[_LIB]="$BUILDDIR[/local/lib/lib]cut_lib_prefix(VAR)[.a]"
		VAR[_LINK]=""
	      ]) #do not quote VAR. It is necessary to lower case as base names are uniform

	      [CONFIGURE_]VAR[_FLAGS]="FL $UPPERVAR[_FLAGS]"
	      AC_SUBST([CONFIGURE_]VAR[_FLAGS])
	      AC_MSG_NOTICE([CONFIGURE_]VAR[_FLAGS]=$[CONFIGURE_]VAR[_FLAGS])
	      AS_IF([test -d  $BUILDDIR/$[MAYBE_]VAR && ! test -d  $[MAYBE_]VAR ], [cp -r $BUILDDIR/$[MAYBE_]VAR  $PWD])
	   ],
	   [VAR=])

   #do not quote ac_subst'd VARS

    AC_SUBST([MAYBE_]VAR)
    AC_SUBST(VAR)
    AC_SUBST(VAR[_CONFIGURE_FILE])
    AC_SUBST(VAR[_LIB])
    AC_SUBST(VAR[_LINK])
    #m4_popdef([VAR])
    #m4_popdef([UPPERVAR])
    #m4_popdef([CDR])
    #m4_popdef([FL])
    #m4_popdef([LIST])
    ])])


AC_DEFUN([DVDA_CONFIG_EXECUTABLE_INSTALL],[DVDA_CONFIG([$1],[#executable_install])])
AC_DEFUN([DVDA_CONFIG_LIBRARY_NO_INSTALL],[DVDA_CONFIG([$1],[
       VAR[_LIB]="$BUILDDIR/[$MAYBE_]VAR/VAR.a"])
       VAR[_LINK]=""
    ])
AC_DEFUN([DVDA_CONFIG_LIBRARY_LOCAL_INSTALL],[DVDA_CONFIG([$1],[]) ])


AC_DEFUN([DVDA_PREFIX_DEFAULT],
 [ AC_PREFIX_DEFAULT([$1])
   AS_IF([test $prefix = NONE],[prefix=$ac_default_prefix])])

# All above macros are copyright Fabrice Nicol, 2009-2013
# These macros are part of the dvda-author package
# and are delivered under the same licensing terms.
# --------------------------------------------------

