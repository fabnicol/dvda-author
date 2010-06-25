
 ## dealing with the unfortunate bug of SoX versions < 14.3
 ## SoX redefines getopt_log in its source code and this causes runtime stack overflows owing to dvda-author code calling the SoX intervening function instead of the right one
 ## bug corrected by SoX developers as of end of May 2009 by replacing getopt_long with lsx_getopt_long
 ## does not arise if in-tree code is built however.
 ## glibc will be compiled with a name replacement for getopt_long (getopt_long_surrogate)

# do not invoke DVDA_ARG_ENABLE in main dependencies.m4.

AC_DEFUN([SOX_DEBUG_TEST],[
 
  AS_IF([ test x$SOX_LIB_INPUT != x ],
# looking into --with-sox specified lib
  [
   AC_CHECK_LIB([$SOX_LIB_INPUT], [getopt_long],
                [
                 AC_MSG_NOTICE([getopt_long detected in SoX code...
 circumventing SoX bug by using glibc.])
                 IBERTY_BUILD=yes
                 circumvent_sox_bug=yes
                ],
	        [ AC_MSG_NOTICE([getopt_long not detected in SoX code... OK]) ]
               )
  ],
  [

# looking into installed lib
   AC_SEARCH_LIBS(getopt_long, [], [ AC_MSG_NOTICE([getopt_long detected in SoX code...
   circumventing SoX bug by using glibc.])
			             IBERTY_BUILD=yes
			             circumvent_sox_bug=yes
                                     AC_DEFINE([HAVE_SOX_BUG],[1],[SoX has buggy getopt_long])
				   ],
				   [ AC_MSG_NOTICE([getopt_long not detected in SoX code... OK]) ])
  ])

  AM_CONDITIONAL([HAVE_IBERTY_BUILD], [test $IBERTY[_BUILD] = xyes || test x$ALL_BUILDS = xyes])
  AC_DEFINE([HAVE_IBERTY_BUILD],[1],[configure, build and install (chunks of) (l)iberty from source code])
  ])
