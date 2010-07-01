
if HAVE_SMAKE
MAKE=${SMAKE}
else
MAKE="${MAKE} -j4"
endif

# downloaded binaries will be built locally, to avoid possible versioning issues with already installed one
# passing right flags to make distcheck when configuring nested packages We do not configure libogg, because install is forced under _build/_inst instead of local/

DISTCHECK_CONFIGURE_FLAGS=--enable-all-deps --enable-all-builds
ACLOCAL_AMFLAGS=-I m4 --install

# delegating nested 'configure' to 'make' in gcc-fashion saves about 40% of computing time on a multicore with make -j4
# configure real processing times on Intel Q4 9450, 2GB DDR3 1600 MHz:
#
# libFLAC + libogg (prior versions)
# make     make -j4
# 9.5s --> 4.5s

# nested packages will be configured only if main configure has been launched freshly.
# stamprecord si a so-called cookie, used only for its time stamp

.PHONY: do_sox_lib_deps_subst


all: stamprecord0 stamprecord1 stamprecord2 stamprecord3 stamprecord4 do_sox_lib_deps_subst


# directly patching the Makefile appears to be more efficient. You nee a GNU-compliant make.
do_sox_lib_deps_subst: src/Makefile stamprecord2
if HAVE_SOX_BUILD
	chmod +x ${MAYBE_SOX}/sox-libs
	$(SED) -e "s,SOX_LIB_DEPS,$(shell cat ${MAYBE_SOX}/sox-libs | $(SED) "s,[@][^@]*[@],,g" ),g" < src/Makefile > src/Makefile.temp
	mv src/Makefile.temp src/Makefile
endif


# delegating configure to a script for FLAC and SOX is oddly necessary, for unclear reasons. The same script embedded here causes libtool issues,
# probably a libtool bug?

stamprecord0: Makefile
	if test -f stamprecord0; then rm -f stamprecord0; fi; touch stamprecord0
if HAVE_LIBOGG_BUILD
	cd ${MAYBE_LIBOGG} &&  ${SHELL} configure ${CONFIGURE_LIBOGG_FLAGS} && $(MAKE) && $(MAKE) install ; cd -
endif


stamprecord1: Makefile
	if test -f stamprecord1; then rm -f stamprecord1; fi; touch stamprecord1
if HAVE_FLAC_BUILD
	chmod +x script && ${SHELL} script
endif

# you need the autotools to be able to rebuild sox with the sox-libs patch (the alternative would be using pkg-config, which is no better)

stamprecord2: Makefile
	if test -f stamprecord2; then rm -f stamprecord2; fi; touch stamprecord2
if HAVE_AUTOMAKE
if HAVE_AUTOCONF
if HAVE_SOX_BUILD
	chmod +x script1 && ${SHELL} script1
endif
endif
endif

stamprecord3: Makefile
	if test -f stamprecord3; then rm -f stamprecord3; fi; touch stamprecord3
if HAVE_AUTOMAKE
if HAVE_AUTOCONF
if HAVE_DVDAUTHOR_BUILD
	cp -r ${ROOTDIR}/m4.extra.dvdauthor ${MAYBE_DVDAUTHOR}/srcm4 && cd ${MAYBE_DVDAUTHOR} && autoreconf -i -Isrcm4 && ${SHELL} configure  --prefix=${prefix} ; cd -
endif
endif
endif

stamprecord4: Makefile
	if test -f stamprecord4; then rm -f stamprecord4; fi; touch stamprecord4
if HAVE_CDRTOOLS_BUILD
	cd ${MAYBE_CDRTOOLS} && $(MAKE) ; cd - ; 
endif

dvda-author.1:
if HAVE_HELP2MAN
	${HELP2MAN} -s 1 -N -o ${top_srcdir}/dvda-author.1 ${top_srcdir}/src/dvda
endif

dvda-author.html: dvda-author.1
#PATCH  09.12  We need to display in UTF-8 mode and take off an illicit file header created by man2html
if HAVE_MAN2HTML
	${MAN2HTML} < ${top_srcdir}/dvda-author.1 | $(SED) -e s/Content-type\.\*\$\/'<meta http-equiv="content-type" content="text\/html; charset=UTF-8">'/ > ${top_srcdir}/dvda-author.html
endif


SUBDIRS=${MAYBE_IBERTY} libutils ${MAYBE_LIBOGG} ${MAYBE_FLAC} ${MAYBE_FIXWAV} libats2wav  ${MAYBE_SOX}  ${MAYBE_DVDAUTHOR} src
DIST_SUBDIRS=libglibc  libutils libfixwav src CB_project libats2wav images

#normally is ${prefix}/share/applications/dvda-author-${VERSION}
sysconfdir=${SHORTLINKDIR}
# distributed under $sysconfdir, normally
dist_sysconf_DATA=dvda-author.desktop dvda-author.conf
menudir=${MENUDIR}
# distributed under $menudir=$sysconfdir/menu
dist_menu_DATA=menu/black_PAL_720x576.jpg menu/black_PAL_720x576.png menu/black_NTSC_720x480.jpg menu/black_NTSC_720x480.png menu/silence.wav menu/activeheader
# normally $prefix/share/pixmaps/dvda-author
pixdir=${PIXDIR}
# distributed under $pixdir
dist_pix_DATA=dvda-author_48x48.png dvda-author_64x64.png
# distributed under $docdir, normally $prefix/doc/dvda-author
if HAVE_HELP2MAN

endif

dist_doc_DATA=README  BUGS EXAMPLES LIMITATIONS HOWTO.conf dvda-author.conf.example

if HAVE_MAN2HTML
  dist_doc_DATA += dvda-author.html
  htmlhelp=dvda-author.html
else
  htmlhelp=
endif

if HAVE_HELP2MAN
  dist_man_MANS=dvda-author.1
  manhelp=dvda-author.1
else
  manhelp=
endif

# GNU build system regeneration script and others
EXTRA_DIST=autogen dvda-author.ico  dvda-author.bat  sox-libs  m4.extra m4.extra.dvdauthor
install-data-local:  $(manhelp) $(htmlhelp)
install-exec-local:
	 cd ${MAYBE_CDRTOOLS}; $(MAKE) INS_BASE=${prefix} install; cd - ;

# redefining install is necessary to rename nested package directories for later builds with --enable-all-builds, which request lib* labelling

install: install-exec install-am
if HAVE_LIBOGG_BUILD
	if test -d libogg-${LIBOGG_VERSION}; then mv libogg-${LIBOGG_VERSION}/ libogg/; fi
endif
if HAVE_FLAC_BUILD
	if test -d flac-${FLAC_VERSION}; then mv flac-${FLAC_VERSION}/ libFLAC/; fi
endif
if HAVE_SOX_BUILD
	if test -d sox-${SOX_VERSION}; then mv sox-${SOX_VERSION}/ libsox/; fi
endif

clean-local:
	  rm -f dvda-author.1 dvda-author.html
