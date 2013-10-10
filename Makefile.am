#to be used with automake 1.14 and higher
#file: Makefile.am Copyright Fabrice Nicol 2013
#Caution: you need GNU make, version 3.82 and higher, preferably 3.99+

#Do not use bash. It would cause subtle issues with libtool in the flac package, probably a libtool bug.
SHELL=/bin/sh

.SHELLFLAGS=-ec

#Using ONESHELL, version make 3.82+, possibly higher with windows ports
.ONESHELL:

include $(foreach prog,@PROGRAM_TARGETS@,mk/$(prog).mk)
include mk/functions.mk

# nested packages will be configured only if main configure has been launched freshly.
# stamprecord si a so-called cookie, used only for its time stamp

.PHONY: all erase_build.trace do_sox_lib_deps_subst @PROGRAM_TARGETS@ create_autotargets $(AUTOTARGETS)

ALL_TARGETS=erase_build.trace @PROGRAM_TARGETS@

ifeq "@HAVE_sox_BUILD@" "yes"
 ALL_TARGETS += do_sox_lib_deps_subst
endif

all: $(ALL_TARGETS) src
	$(MAKE) --directory=$^

src: libutils $(iberty) $(fixwav) $(sox)

erase_build.trace:
	echo PROGRAM_TARGETS=@PROGRAM_TARGETS@
	if test -f BUILD.TRACE; then
	  mv BUILD.TRACE BUILD.TRACE~
	fi

# directly patching the Makefile appears to be more efficient. You need a GNU-compliant make.

do_sox_lib_deps_subst: src/Makefile sox
	echo Processing dependencies for sox=@sox@...
	$(if @sox@,
	chmod +x @MAYBE_sox@/sox-libs
	pkgconfig_style_libs=$(shell cat @MAYBE_sox@/sox-libs | @SED@ -e "s,[\.\/].*[a],,g   ;  s,@.*@,,g")
	@SED@ -e "s/SOX_LIB_DEPS/$$pkgconfig_style_libs/g" < src/Makefile > src/Makefile.temp
	mv src/Makefile.temp src/Makefile
	$(call follow,src/Makefile.temp, done sox library substitution \(pkgconfig-style\)))

dvda-author.1:
ifeq "@HAVE_help2man@" "yes"
	if test -f src/dvda; then
		@HELP2MAN@ -s 1 -N -o dvda-author.1 src/dvda
	fi
else
	if test -f src/dvda -a test -d @MAYBE_HELP2MAN@ -a test -f @MAYBE_HELP2MAN@/@HELP2MAN@; then
		@MAYBE_HELP2MAN@/@HELP2MAN@ -s 1 -N -o dvda-author.1 src/dvda
	fi
	$(call docfollow,  $@)
endif

dvda-author-@VERSION@.html: dvda-author.1
#PATCH  09.12  We need to display in UTF-8 mode and take off an illicit file header created by man2html
ifeq "@HAVE_man2html@" "yes"
		@MAN2HTML@ < dvda-author.1 | @SED@ -e s/Content-type\.\*\$\/'<meta http-equiv="content-type" content="text\/html; charset=UTF-8">'/ > $@
		$(call docfollow, $@)
endif

.PHONY: sysconfdir dist_sysconf_DATA menudir dist_menu_DATA dist_doc_DATA DISFORMATS $(FORMATS) \
	install-data-local install clean clean-local distclean

#normally is ${prefix}/share/applications/dvda-author-${VERSION}
sysconfdir=@SHORTLINKDIR@

# distributed under $sysconfdir, normally
dist_sysconf_DATA=dvda-author.desktop dvda-author.conf

# distributed under $menudir=$sysconfdir/menu
# normally $prefix/share/pixmaps/dvda-author
# distributed under $pixdir

dist_pic_DATA= dvda-author_48x48.png dvda-author_64x64.png
# distributed under $docdir, normally $prefix/doc/dvda-author

dist_doc_DATA=README  BUGS EXAMPLES LIMITATIONS BUILD.Ubuntu  DEPENDENCIES HOWTO.conf dvda-author.conf.example

ifeq "@HAVE_man2html@" "yes"
  htmlhelp=dvda-author-@VERSION@.html
else
  htmlhelp=
endif

ifeq "@HAVE_help2man@" "yes"
  manhelp=dvda-author.1
else
  manhelp=
endif

# GNU build system regeneration script and others

install-sys: dist_sysconf _DATA
	$(INSTALL_DATA) $^ $(DESTDIR)$(sysconfdir)

intall-pic:  @MENUDIR@
	$(INSTALL_DATA) $^ $(DESTDIR)$(picdir)

install-data-local:  $(manhelp) $(htmlhelp) $(dist_doc_DATA) $(dist_pic_DATA)
	$(INSTALL_DATA) $^ $(DESTDIR)$(infodir)

# redefining install is necessary to rename nested package directories for later builds with --enable-all-builds, which request lib* labelling

install: all install-data-local install-pic install-sys
	$(foreach dir,src $(subdirs), $(MAKE) --directory=$(dir) install)

clean: clean-local
	rm -f $(wildcard *.o *.po *.a)

clean-local:
	rm -f dvda-author.1 dvda-author-@VERSION@.html
	$(foreach prog,@PROGRAM_TARGETS@,$(call clean_package($(prog))))

distclean: clean-local
	rm -rf autom4te* && rm -f config.log && rm -f config.status
	find . -name Makefile -delete

maintainer-clean: distclean
	find . -name configure -delete
	rm -rf config

FORMATS=tar.xz tar.gz tar.bz2 zip
DISTFORMATS=$(FORMATS:%=dist-%)

$(FORMATS): ;

#master by default, can be overridden on commandline e.g. make dist-xz BRANCH=win32

$(DISTFORMATS): $(FORMATS)
ifeq "@HAVE_git@" "yes"
   BRANCH=master
   CURRRENTBRANCH=`@GIT@ status -bs -u no -z | sed -e 's/[# ]*//g'`
   ifneq "$(BRANCH)" "$(CURRENTBRANCH)"
	@GIT@ checkout $(BRANCH))
   endif
	if test $? = 0 ; then @GIT@ archive -o dvda-author-@VERSION@.$< HEAD $(BRANCH); fi
   ifneq "$(BRANCH)" "$(CURRENTBRANCH)"
	@GIT@ checkout $(CURRENTBRANCH)
   endif
endif
