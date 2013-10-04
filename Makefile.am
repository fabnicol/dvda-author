if HAVE_SMAKE
MAKETOOL=$(SMAKE)
else
MAKETOOL=make  -k
endif

SHELL=/bin/bash
.SHELLFLAGS=-ec
.ONESHELL:

define trace
	echo "$2:  $1" >> BUILD.TRACE
endef

define follow
	if test   "$(strip $1)" != "" ; then $(call trace, $2, "done: $1") ; else $(call trace, $2, failed); fi
endef

define execfollow
	$(if "$2",
	    program=$$(eval find $2 -name $1 -print0)
	    $(call follow,$$program,executable: $$($(SHELL) $$program $3)),
	    $(call trace, $1, failed))
endef

define docfollow
	findstring=$(strip $(eval "find ${BUILDDIR} -name $1 -print0"))
	$(call follow, $1: $findstring, "doc: `find $findstring  `")
endef


define configure_sub_package
	target_subdir="$(strip $1)"
	configure_flags="$2"
	autotools_command_line="$3"
	$(if $$target_subdir,
	   if test -d  $$target_subdir; then
	      cd $$target_subdir && $$autotools_command_line && $(SHELL) configure $$configure_flags && $(MAKETOOL) MKDIR_P="mkdir.exe -p"
	      if test "$$?" = "0"; then
		 echo Installing from $$target_subdir ...
		 if test -f INSTALL; then mv -f INSTALL INSTALL.txt ; fi
		 $(MAKETOOL) install
	      fi
	   if test -f INSTALL.txt; then mv -f INSTALL.txt INSTALL; fi
	   cd -
	   fi)
endef

define config_lib_package
	$(call configure_sub_package,$2,$3,$4)
	testvar=$$(find $(BUILDDIR)/$2 -maxdepth 2 -type f -name $(strip $1).a -print0)
	$(if $$testvar,
			echo "found library: $$testvar for $1" >> BUILD.TRACE
			testvar=$$(find $(BUILDDIR)/local/lib -maxdepth 1 -name $(strip $1).a -print0)
			$(if $$testvar,
				echo "locally installed library: $$testvar from $1" >> BUILD.TRACE,
				echo "did not install library $1" >> BUILD.TRACE),
			echo "no library $1" >> BUILD.TRACE)
endef

define config_exec_package
	$(call configure_sub_package, $2, $3)
	$(call execfollow,$$(find $(BUILDDIR) -name $(strip $1) -print0),$2,$4)
endef


DISTCHECK_CONFIGURE_FLAGS=--enable-all-all
ACLOCAL_AMFLAGS=-I m4 -I m4.extra -I m4.extra.dvdauthor --install

# nested packages will be configured only if main configure has been launched freshly.
# stamprecord si a so-called cookie, used only for its time stamp


all: erase_build.trace libogg libFLAC libsox dvdauthor cdrecord mplex help2man  convert lplex mpeg2dec a52dec do_sox_lib_deps_subst


.PHONY: erase_build.trace do_sox_lib_deps_subst

erase_build.trace:
	if test -f  BUILD.TRACE; then mv BUILD.TRACE BUILD.TRACE~ ; fi

# directly patching the Makefile appears to be more efficient. You need a GNU-compliant make.
do_sox_lib_deps_subst: src/Makefile libsox
if HAVE_SOX_BUILD
	chmod +x ${MAYBE_SOX}/sox-libs
	$(SED) -e "s,SOX_LIB_DEPS,$(shell cat ${MAYBE_SOX}/sox-libs | $(SED) -e "s,[\.\.\/].*[a],,g"   ;  "s,@.*@,,g"),g" < src/Makefile > src/Makefile.temp
	mv src/Makefile.temp src/Makefile
	$(call follow,src/Makefile.temp,$@)
endif


libogg: Makefile
if HAVE_LIBOGG_BUILD
	$(call config_lib_package,libogg, $(MAYBE_LIBOGG),$(CONFIGURE_LIBOGG_FLAGS))
endif

#build and install libogg before libFLAC
libFLAC: Makefile libogg
if HAVE_FLAC_BUILD
	$(call config_lib_package,libFLAC, $(MAYBE_FLAC),$(CONFIGURE_FLAC_FLAGS))
endif

# you need the autotools to be able to rebuild sox with the sox-libs patch (the alternative would be using pkg-config, which is no better)

libsox: Makefile
if HAVE_SOX_BUILD
	if test  "$(MAYBE_SOX)" != ""; then
		if test -d "$(BUILDDIR)/$(MAYBE_SOX)" ; then
			cp "$(BUILDDIR)/sox-libs" "$(BUILDDIR)/$(MAYBE_SOX)/sox-libs.in"
			else
			echo "$(BUILDDIR)/$(MAYBE_SOX) is not a directory"
			exit -1
		fi
	fi
	$(call config_lib_package,libsox,$(MAYBE_SOX),$(CONFIGURE_SOX_FLAGS),autoreconf -if)
endif

dvdauthor: Makefile
	if test -f dvdauthor; then rm -f dvdauthor; fi; touch dvdauthor
if HAVE_AUTOMAKE
if HAVE_AUTOCONF
if HAVE_DVDAUTHOR_BUILD
	cp -r ${ROOTDIR}/m4.extra.dvdauthor ${MAYBE_DVDAUTHOR}/srcm4
	$(if $(MAYBE_DVDAUTHOR),
	  if test -d $(MAYBE_DVDAUTHOR) ; then
	   cd $(MAYBE_DVDAUTHOR) && $(SHELL) autoreconf -i -Isrcm4 && $(SHELL) configure  --prefix=$(prefix) ; $(MAKETOOL); cd -
	  fi)
endif
endif
endif

#cdrecord must be compiled with special make install parameters hence should not be included in the SUBDIRS list. Building/cleaning will need prior configuration.

cdrecord: Makefile
if HAVE_CDRTOOLS_BUILD
	$(call execfollow,$@,$(MAYBE_CDRTOOLS),-version 2>null))
endif

mplex: Makefile
if HAVE_MJPEGTOOLS_BUILD
	$(call config_exec_package,$@,$(MAYBE_MJPEGTOOLS),$(CONFIGURE_MJPEGTOOLS_FLAGS),2>&1|line)
endif

help2man: Makefile
	if test -f help2man; then rm -f help2man; fi; touch help2man
if HAVE_HELP2MAN_BUILD
	if test "${MAYBE_HELP2MAN}" != "" ; then \
	  if test -d ${MAYBE_HELP2MAN} ; then \
	  cd ${MAYBE_HELP2MAN} && ${SHELL} configure --prefix=${prefix} ; $(MAKETOOL) ; cd - ;\
	fi; fi
	$(call execfollow, ${MAYBE_HELP2MAN}, $@,  --version | line)
	if test -f ${MAYBE_HELP2MAN}/help2man ; then \
		  ${MAYBE_HELP2MAN}/help2man -s 1 -N -o dvda-author.1 src/dvda ;\
		  $(call docfollow,  $@) ;\
		  manhelp=dvda-author.1 ;\
	fi
endif

convert: Makefile
if HAVE_IMAGEMAGICK_BUILD
	$(call config_exec_package,$@,$(MAYBE_IMAGEMAGICK),$(CONFIGURE_IMAGEMAGICK_FLAGS),--version 2>&1|line)
endif

lplex: Makefile libFLAC
if HAVE_LPLEX_BUILD
	$(call config_exec_package,$@,$(MAYBE_LPLEX),$(CONFIGURE_LPLEX_FLAGS),--version 2>&1|line)
endif

mpeg2dec: Makefile
if HAVE_LIBMPEG2_BUILD
	$(call config_exec_package,$@,$(MAYBE_LIBMPEG2),$(CONFIGURE_IMAGEMAGICK_FLAGS),<null 2>&1|line)
endif

a52dec: Makefile
if HAVE_A52DEC_BUILD
	$(call config_exec_package,$@,$(MAYBE_A52DEC),$(CONFIGURE_A52DEC_FLAGS),2>&1|line)
endif

dvda-author.1:
if HAVE_HELP2MAN
	${HELP2MAN} -s 1 -N -o dvda-author.1 src/dvda
	$(call docfollow,  $@)
endif

dochtml: dvda-author.1
#PATCH  09.12  We need to display in UTF-8 mode and take off an illicit file header created by man2html
if HAVE_MAN2HTML
	${MAN2HTML} < dvda-author.1 | $(SED) -e s/Content-type\.\*\$\/'<meta http-equiv="content-type" content="text\/html; charset=UTF-8">'/ > dvda-author-@VERSION@.html
	$(call docfollow, $@)
endif


SUBDIRS=libiberty libutils libfixwav src  images

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

dist_doc_DATA=README  BUGS EXAMPLES LIMITATIONS BUILD.Ubuntu  DEPENDENCIES HOWTO.conf dvda-author.conf.example

if HAVE_MAN2HTML
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

EXTRA_DIST=autogen dvda-author.ico  dvda-author-launch.bat  sox-libs  m4.extra m4.extra.dvdauthor menu  libs patches

install-data-local:  $(manhelp) $(htmlhelp)

install-exec-local:

if HAVE_CDRTOOLS_BUILD
	cd ${MAYBE_CDRTOOLS}; $(MAKETOOL) INS_BASE=${prefix} install; cd -
endif
if HAVE_DVDAUTHOR_BUILD
	cd ${MAYBE_DVDAUTHOR}; $(MAKETOOL)  install; cd - ;
	$(call follow, find ${MAYBE_DVDAUTHOR} -name dvdauthor -print0, $(`find ${MAYBE_DVDAUTHOR} -name dvdauthor`  --version 2>&1 |grep patched))
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
if HAVE_HELP2MAN_BUILD
	cd ${MAYBE_HELP2MAN}; $(MAKETOOL) install; cd - ;
	$(call follow, find ${MAYBE_HELP2MAN} -name help2man -print0, $(`find ${MAYBE_HELP2MAN} -name help2man -print0` ))
endif

clean-local:
	rm -f dvda-author.1 dvda-author-@VERSION@.html
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

distclean: clean-local
	rm -rf autom4te* && rm -f config.log && rm -f config.status
	find . -name Makefile -delete

maintainer-clean: distclean
	find . -name configure -delete
	rm -rf config





