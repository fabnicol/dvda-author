

# File auxiliary.m4
# All macros below are copyright Fabrice Nicol, 2009
# These macros are part of the dvda-author package
# and are delivered under the same licensing terms.
# --------------------------------------------------


# === Auxiliary tools === #

# normalise: remove hyphens, translitted into underscores        sox-build            --> sox_build
# basename : remove suffixes starting in hyphen or underscore    dvdauthor-patch      --> dvdauthor
# suffix   : remove body and preserves suffix starting in hyphen --enable-sox-build   --> build
# dehyphenate: translits hyphens into white space                sox-build            --> sox build
# upperbasename: capitalise basename                             dvdauthor-patch      --> DVDAUTHOR
# uppernormalisename: capitalise normalised name                 dvdauthor-patch      --> DVDAUTHOR_PATCH
# cdr_w    : takes off first word in white-space separated string "abc defh ikl"      --> "defh ikl"



m4_map([m4_define],[
[[normalise],         [m4_translit([$1], [-],     [_])]],
[[basename],          [m4_bpatsubst([$1],[[-_].*],[])]],
[[suffix],            [m4_bpatsubst([$1],[^.*-],  [])]],
[[dehyphenate],       [m4_translit([$1], [-],     [ ])]],
[[upperbasename],     [m4_toupper(basename([$1]))]],
[[uppernormalisename],[m4_toupper(normalise([$1]))]],
[[cdr_w],             [m4_bpatsubst([$1],[^[^ ]*[ ]*],[])]],
[[car_w],             [m4_bpatsubst([$1],[ .*],[])]],
[[DVDA_INF],          [ AC_MSG_NOTICE([===INF===  $1]) ]],
[[DVDA_ERR],          [
                        AC_MSG_WARN([===ERR===  $1])
                        errorcode=1
                      ]],
[[DVDA_RUN],
                                          [
                                            DVDA_INF([Running $1 $2 $3])
                                            $1 $2 $3
                                            exitcode=$?
                                            AS_IF([test $exitcode = 0],
                                               [DVDA_INF([...OK])],
                                               [
                                                 DVDA_ERR([...$1 $2 $3 failed])
                                                 sleep 2s
                                                 AS_IF([test x$1 != x"$CURL"], [exit $exitcode])
                                               ])
                                          ]],

[[DVDA_CLEAN],
                                          [
                                            AS_IF([test  -f "$1"],
                                             [
                                                   DVDA_INF([Cleaning up $1...])
                                                   rm -f "$1"
                                             ])
                                          ]],
[[DVDA_CURL],         [
                        AS_IF([test "$2" != ""],
                                    [
                                      AS_IF([test "$1" != ""],
                                          [DVDA_RUN(["$CURL"],[ -f --location -o $2],[$1])],
                                          [DVDA_INF([Cannot curl empty Url...])])
                                    ],
                                    [DVDA_INF([Cannot curl empty Url...])])
                       ]],
                                    
[[DVDA_PATCH],        [DVDA_RUN(["$PATCH"],[ -p4 -f --verbose < ],[$1])]],
[[MD5_CHECK],         [$($MD5SUM -b $1 | $SED "s/ .*//g")]],
[[MD5_BREAK],         [m4_ifval([$1],
                                  [
                                    md5=MD5_CHECK([$1])
                                    AS_IF([ test -f $1 && test x$md5 = x$2 ],
                                    [
                                      AC_MSG_NOTICE([Found right MD5 checksum: $md5])
                                      exitcode=0
                                      break
                                    ],
                                    [
                                      AS_IF([test "$md5" != ""],
                                      [ AC_MSG_NOTICE([Did not find right MD5 checksum: *$md5* instead of *$2*, skipping...])],
                                      [ AC_MSG_NOTICE([MD5 sum unavailable, skipping...])])
                                      exitcode=-1

                                    ])
                                  ],
                                  [
                                    sleep 2s
                                    exitcode=-1
                                  ])
                      ]],

[[DVDA_MKDIR],        [AS_IF([test -d "$1"],[rm -rf "$1" && mkdir "$1"],[mkdir "$1"])]],
[[DVDA_TAR],          [DVDA_RUN(["$TAR"],[ $2],[$1])]]])

