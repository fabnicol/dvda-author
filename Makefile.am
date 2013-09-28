if HAVE_SMAKE
MAKETOOL=${SMAKE}
else
MAKETOOL=make  -k
endif

define trace
	echo "$2:  $1" >> BUILD.TRACE
endef

define follow
	if test   "$1" != "" ; then $(call trace, $2, "done: $1") ; else $(call trace, $2, failed); fi
endef

define execfollow
	$(call follow, $3: `find $1 -name $2 -print0`, "executable: `find . -name $2 -print0  -exec   {}  $4  \;`")
endef

define docfollow
	$(call follow, $2: `find  ${ROOTDIR} -name $1 -print0`, "doc: `find . -name $1 -print0  `")
endef

# downloaded binaries will be built locally, to avoid possible versioning issues with already installed one
# passing right flags to make distcheck when configuring nested packages We do not configure libogg, because install is forced under _build/_inst instead of local/

DISTCHECK_CONFIGURE_FLAGS=--enable-all-all
ACLOCAL_AMFLAGS=-I m4 -I m4.extra -I m4.extra.dvdauthor --install

# delegating nested 'configure' to 'make' in gcc-fashion saves about 40% of computing time on a multicore with make -j4
# configure real processing times on Intel Q4 9450, 2GB DDR3 1600 MHz:

# libFLAC + libogg (prior versions)
# make     make -j4
# 9.5s --> 4.5s

# nested packages will be configured only if main configure has been launched freshly.
# stamprecord si a so-called cookie, used only for its time stamp

.PHONY: erase_build.trace do_sox_lib_deps_subst 

erase_build.trace:
	if test -f  BUILD.TRACE; then rm -f BUILD.TRACE; fi

all: erase_build.trace libogg libflac libsox dvdauthor cdrtools mjpegtools help2man imagemagick lplex libmpeg2 a52dec do_sox_lib_deps_subst


# directly patching the Makefile appears to be more efficient. You need a GNU-compliant make.
do_sox_lib_deps_subst: src/Makefile libsox
if HAVE_SOX_BUILD
	chmod +x ${MAYBE_SOX}/sox-libs
	$(SED) -e "s,SOX_LIB_DEPS,$(shell cat ${MAYBE_SOX}/sox-libs | $(SED) -e "s,[\.\.\/].*[a],,g"   -e  "s,@.*@,,g"),g" < src/Makefile > src/Makefile.temp
	mv src/Makefile.temp src/Makefile
	$(call follow, src/Makefile.temp, $@)
endif


# delegating configure to a script for FLAC and SOX is oddly necessary, for unclear reasons. The same script embedded here causes libtool issues,
# probably a libtool bug?

libogg: Makefile
if HAVE_LIBOGG_BUILD
	if test -f libogg; then rm -f libogg; fi; touch libogg
	cd ${MAYBE_LIBOGG} &&  ${SHELL} configure ${CONFIGURE_LIBOGG_FLAGS} && $(MAKETOOL) && $(MAKETOOL) install ; cd -
	$(call follow, $@: `find ${ROOTDIR} -name libogg.a -print0`, library)
endif


libflac: Makefile
	if test -f libflac; then rm -f libflac; fi; touch libflac
if HAVE_FLAC_BUILD
	chmod +x script && ${SHELL} script
	$(call follow, $@: `find ${ROOTDIR} -name libflac.a -print0` , library)
endif

# you need the autotools to be able to rebuild sox with the sox-libs patch (the alternative would be using pkg-config, which is no better)

libsox: Makefile
	if test -f libsox; then rm -f libsox; fi; touch libsox
if HAVE_AUTOMAKE
if HAVE_AUTOCONF
if HAVE_SOX_BUILD
	chmod +x script1 && ${SHELL} script1
	$(call follow, $@: ` find ${ROOTDIR} -name libsox.a -print0` , library)
endif
endif
endif

dvdauthor: Makefile
	if test -f dvdauthor; then rm -f dvdauthor; fi; touch dvdauthor
if HAVE_AUTOMAKE
if HAVE_AUTOCONF
if HAVE_DVDAUTHOR_BUILD
	cp -r ${ROOTDIR}/m4.extra.dvdauthor ${MAYBE_DVDAUTHOR}/srcm4 && cd ${MAYBE_DVDAUTHOR} && autoreconf -i -Isrcm4 && ${SHELL} configure  --prefix=${prefix} ; cd -
endif
endif
endif

#cdrtools must be compiled with special make install parameters hence should not be included in the SUBDIRS list. Building/cleaning will need prior configuration.

cdrtools: Makefile
	if test -f cdrtools; then rm -f cdrtools; fi; touch cdrtools
	cd ${MAYBE_CDRTOOLS} && $(MAKETOOL) ; cd -
	$(call execfollow, ${MAYBE_CDRTOOLS}, cdrecord, $@, -version 2>null)

mjpegtools: Makefile
	if test -f mjpegtools; then rm -f mjpegtools; fi; touch mjpegtools
if HAVE_MJPEGTOOLS_BUILD
	chmod +x script6 && ${SHELL} script6
	$(call execfollow, ${MAYBE_MJPEGTOOLS}, mplex, $@,  2>&1 |  line)
endif

help2man: Makefile
	if test -f help2man; then rm -f help2man; fi; touch help2man
if HAVE_HELP2MAN_BUILD
	cd ${MAYBE_HELP2MAN} && ${SHELL} configure --prefix=${prefix} ; cd -
	$(call execfollow, ${MAYBE_HELP2MAN}, help2man, $@,  --version | line)
endif

imagemagick: Makefile
	if test -f imagemagick; then rm -f imagemagick; fi; touch imagemagick
if HAVE_IMAGEMAGICK_BUILD
	chmod +x script2 && ${SHELL} script2
	$(call execfollow, ${MAYBE_IMAGEMAGICK}, convert, $@,  --version 2>&1 | line)
endif

lplex: Makefile
	if test -f lplex; then rm -f lplex; fi; touch lplex
if HAVE_LPLEX_BUILD
	chmod +x script3 && ${SHELL} script3
	$(call execfollow, ${MAYBE_LPLEX}, lplex, $@,  --version 2>&1 | line)
endif

libmpeg2: Makefile
	if test -f libmpeg2; then rm -f libmpeg2; fi; touch libmpeg2
if HAVE_LIBMPEG2_BUILD
	chmod +x script4 && ${SHELL} script4
	$(call execfollow, ${MAYBE_LIBMPEG2}, mpeg2dec, $@,  <null 2>&1 | line)
endif

a52dec: Makefile
	if test -f a52dec; then rm -f a52dec; fi; touch a52dec
if HAVE_A52DEC_BUILD
	chmod +x script5 && ${SHELL} script5
	$(call execfollow, ${MAYBE_A52DEC}, a52dec, $@,  2>&1 | line)
endif

dvda-author.1:
if HAVE_HELP2MAN
	${HELP2MAN} -s 1 -N -o dvda-author.1 src/dvda
	$(call docfollow,  dvda-author.1, $@)
endif

dochtml: dvda-author.1
#PATCH  09.12  We need to display in UTF-8 mode and take off an illicit file header created by man2html
if HAVE_MAN2HTML
	${MAN2HTML} < dvda-author.1 | $(SED) -e s/Content-type\.\*\$\/'<meta http-equiv="content-type" content="text\/html; charset=UTF-8">'/ > dochtml
	$(call docfollow, dochtml, $@)
endif


SUBDIRS=${MAYBE_IBERTY} libutils ${MAYBE_LIBOGG} ${MAYBE_FLAC} ${MAYBE_FIXWAV} ${MAYBE_SOX}  ${MAYBE_LPLEX} ${MAYBE_DVDAUTHOR} ${MAYBE_HELP2MAN}  ${MAYBE_IMAGEMAGICK} ${MAYBE_LIBMPEG2} ${MAYBE_A52DEC} src
DIST_SUBDIRS=libiberty libutils libfixwav src  images 

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

dist_doc_DATA=README  BUGS EXAMPLES LIMITATIONS BUILD.lplex BUILD.Windows BUILD.Ubuntu  DEPENDENCIES HOWTO.conf dvda-author.conf.example

if HAVE_MAN2HTML
  dist_doc_DATA += dochtml
  htmlhelp=dochtml
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

EXTRA_DIST=autogen dvda-author.ico  dvda-author-launch.bat  sox-libs  m4.extra m4.extra.dvdauthor menu  libs

install-data-local:  $(manhelp) $(htmlhelp)
install-exec-local:

if HAVE_CDRTOOLS_BUILD
	cd ${MAYBE_CDRTOOLS}; $(MAKETOOL) INS_BASE=${prefix} install; cd -
endif
if HAVE_DVDAUTHOR_BUILD
	cd ${MAYBE_DVDAUTHOR}; $(MAKETOOL)  install; cd - ;
	$(call follow, find ${MAYBE_DVDAUTHOR} -name dvdauthor -print0, $(`find ${MAYBE_DVDAUTHOR} -name dvdauthor`  --version 2>&1 |grep patched))
endif
if HAVE_HELP2MAN_BUILD
	cd ${MAYBE_HELP2MAN}; $(MAKETOOL) install; cd - ;
	$(call follow, find ${MAYBE_HELP2MAN} -name help2man -print0, $(`find ${MAYBE_HELP2MAN} -name help2man -print0` ))
endif
if HAVE_MJPEGTOOLS_BUILD
	cd ${MAYBE_MJPEGTOOLS}; $(MAKETOOL) install; cd - ;
endif
if HAVE_IMAGEMAGICK_BUILD
	cd ${MAYBE_IMAGEMAGICK}; $(MAKETOOL) install; cd - ;
endif
if HAVE_SOX_BUILD
	cd ${MAYBE_SOX}; $(MAKETOOL) install; cd -;
endif



# redefining install is necessary to rename nested package directories for later builds with --enable-all-builds, which request lib* labelling

install: install-exec install-data-local install-am
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
	rm -f dvda-author.1 dochtml
if HAVE_CDRTOOLS_BUILD
	cd ${MAYBE_CDRTOOLS}; $(MAKETOOL) INS_BASE=${prefix} clean; cd -
endif
if HAVE_DVDAUTHOR_BUILD
	cd ${MAYBE_DVDAUTHOR}; $(MAKETOOL)  clean; cd - ;
endif
if HAVE_HELP2MAN_BUILD
	cd ${MAYBE_HELP2MAN}; $(MAKETOOL) clean; cd - ;
endif
if HAVE_MJPEGTOOLS_BUILD
	cd ${MAYBE_MJPEGTOOLS}; $(MAKETOOL) clean; cd - ;
endif
if HAVE_IMAGEMAGICK_BUILD
	cd ${MAYBE_IMAGEMAGICK}; $(MAKETOOL) clean; cd - ;
endif
if HAVE_SOX_BUILD
	cd ${MAYBE_SOX}; $(MAKETOOL) clean; cd -;
endif

