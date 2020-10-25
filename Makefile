# file: Makefile.in Copyright Fabrice Nicol 2013
# do not use automake
# Caution: you need GNU make, version 3.82 and higher, preferably 3.99+

# Do not use bash. It would cause subtle issues with libtool in the flac
# package, probably a libtool bug.

PROGRAM=dvda-author-dev
export PROGRAM
INSTALL=$(SHELL) /cygdrive/c/Users/fabrn/dvda-author/config/install-sh
INSTALL_DATA=$(SHELL) /cygdrive/c/Users/fabrn/dvda-author/config/install-sh  -m 644
export INSTALL
export INSTALL_DATA
MKDIR_P=mkdir$(EXEEXT) -p

SHELL=/bin/sh

.SHELLFLAGS=-ec

#Using ONESHELL, version make 3.82+, possibly higher with windows ports
.ONESHELL:

CFLAGS ?= -g -O2 -g -O2 -std=c99
export CFLAGS
CPPFLAGS = -DHAVE_CONFIG_H

PROGRAM_TARGETS :=  sox libogg FLAC ffmpeg dvdauthor lplex mjpegtools cdrtools a52dec libmpeg2 help2man ImageMagick man2html libiberty libfixwav 
PROGRAM_TARGET_MAKEFILES := $(foreach prog,$(PROGRAM_TARGETS), \
$(shell if test -f /cygdrive/c/Users/fabrn/dvda-author/mk/$(prog).mk; then \
echo /cygdrive/c/Users/fabrn/dvda-author/mk/$(prog).mk; fi))
PROGRAM_TARGET_CONFIGS := $(foreach prog,$(PROGRAM_TARGETS), \
$(shell if test  -f /cygdrive/c/Users/fabrn/dvda-author/mk/$(prog).mk ; then echo $(prog).config; fi))
PROGRAM_TARGET_DEPCONFS := $(PROGRAM_TARGET_CONFIGS:.config=.depconf)

ifneq "" "no"
  ifneq "libiberty/src" ""
    BUILD_SUBDIRS += /cygdrive/c/Users/fabrn/dvda-author/libiberty/src
  endif
else
  CPPFLAGS += -DWITHOUT_libiberty
endif

BUILD_SUBDIRS += /cygdrive/c/Users/fabrn/dvda-author/libfixwav/src

BUILD_SUBDIRS += /cygdrive/c/Users/fabrn/dvda-author/src

.SUFFIXES: .config .depconf

ALL_TARGETS_EXTERNAL=erase_build.trace

ALL_TARGETS_EXTERNAL += $(PROGRAM_TARGET_CONFIGS)

ifeq "yes" "yes"
 ALL_TARGETS_EXTERNAL += do_sox_lib_deps_subst
endif

ALL_TARGETS += $(ALL_TARGETS_EXTERNAL) $(BUILD_SUBDIRS)

.PHONY: all    libutils

all:  $(ALL_TARGETS) $(PROGRAM).1 $(PROGRAM).html

include /cygdrive/c/Users/fabrn/dvda-author/mk/functions.mk
include $(PROGRAM_TARGET_MAKEFILES)

help2man.config: /cygdrive/c/Users/fabrn/dvda-author/depconf/help2man.depconf
man2html.config: /cygdrive/c/Users/fabrn/dvda-author/depconf/man2html.depconf

export CPPFLAGS
export PARALLEL

libutils:
	$(MAKE) --directory=/cygdrive/c/Users/fabrn/dvda-author/libutils/src

$(BUILD_SUBDIRS): $(ALL_TARGETS_EXTERNAL) libutils
	@echo Running make in directory $@...
	$(MAKE) $(PARALLEL) --directory=$@

$(PROGRAM_TARGET_CONFIGS): %.config: /cygdrive/c/Users/fabrn/dvda-author/depconf/%.depconf
	@echo
	echo Finished building $*...

erase_build.trace:
	@echo PROGRAM_TARGETS=$(PROGRAM_TARGETS)
	if test -f /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE; then
	  mv /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE~
	fi

# directly patching the Makefile appears to be more efficient. You need a
# GNU-compliant make.

#this must be lazy-evaluation otherwise will not work
pkgconfig_style_libs=$(if $(MAYBE_sox),$(shell cat						\
/cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_sox)/sox-libs | /usr/bin/sed -e "s/^.*\.a//g ; s/@.*@//g;	\
s,\/,\\\/,g"))

do_sox_lib_deps_subst: /cygdrive/c/Users/fabrn/dvda-author/src/Makefile  /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_sox)/sox-libs
	@echo Processing dependencies for sox...
	/usr/bin/sed -i -e 's/SOX_LIB_DEPS/$(pkgconfig_style_libs)/g' \
/cygdrive/c/Users/fabrn/dvda-author/src/Makefile
	echo "[sox] done sox library substitution with libs: $(pkgconfig_style_libs)"\
>> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE

manpage: $(PROGRAM).1

htmlpage: $(PROGRAM).html

pdf: $(PROGRAM).html
		[ -f "" ] &&  $(PROGRAM).html $(PROGRAM).pdf

$(PROGRAM).1:
	@if test "yes" = "yes"; then
	  if test -f /cygdrive/c/Users/fabrn/dvda-author/src/$(PROGRAM); then
		/usr/bin/help2man -s 1 -N -o $(PROGRAM).1 /cygdrive/c/Users/fabrn/dvda-author/src/$(PROGRAM)
	  fi
	else
	  if test -f src/$(PROGRAM) && test -d "help2man-1.47.9" \
&& test -f "help2man-1.47.9"/help2man; then
		"/cygdrive/c/Users/fabrn/dvda-author/help2man-1.47.9"/help2man -s 1 -N \
-o $(PROGRAM).1 /cygdrive/c/Users/fabrn/dvda-author/src/$(PROGRAM)
	  fi
	fi
	$(call docfollow,  $@)


$(PROGRAM).html: $(PROGRAM).1
	@if test "no" = "yes"; then
		 < $(PROGRAM).1 > $@
		$(call docfollow, $@)
	else
	  if test -f "/cygdrive/c/Users/fabrn/dvda-author/local/bin/man2html" ; then
		"/cygdrive/c/Users/fabrn/dvda-author/local/bin/man2html" < $(PROGRAM).1 > $@
		$(call docfollow, $@)
	  fi
	fi

.PHONY: DISFORMATS $(FORMATS) \
	install-data-local install clean clean-local distclean infodir

infodir:
	$(MKDIR_P)  $(DESTDIR)/usr/local/share/info/$(PROGRAM)


#normally is ${prefix}/share/applications/dvda-author-${VERSION}
sysconfdir=/usr/local/share/applications/dvda-author-dev

# distributed under $sysconfdir, normally

dist_sysconf_DATA=/cygdrive/c/Users/fabrn/dvda-author/dvda-author.desktop /cygdrive/c/Users/fabrn/dvda-author/dvda-author.conf

# distributed under $menudir=$sysconfdir/menu
# normally $prefix/share/pixmaps/dvda-author
# distributed under $pixdir

dist_pic_DATA= /cygdrive/c/Users/fabrn/dvda-author/dvda-author_48x48.png /cygdrive/c/Users/fabrn/dvda-author/dvda-author_64x64.png
# distributed under $docdir, normally $prefix/doc/dvda-author

dist_doc_DATA=/cygdrive/c/Users/fabrn/dvda-author/README  /cygdrive/c/Users/fabrn/dvda-author/BUGS /cygdrive/c/Users/fabrn/dvda-author/EXAMPLES \
/cygdrive/c/Users/fabrn/dvda-author/LIMITATIONS /cygdrive/c/Users/fabrn/dvda-author/BUILD.Ubuntu /cygdrive/c/Users/fabrn/dvda-author/COREBUILD \
  /cygdrive/c/Users/fabrn/dvda-author/DEPENDENCIES /cygdrive/c/Users/fabrn/dvda-author/HOWTO.conf /cygdrive/c/Users/fabrn/dvda-author/dvda-author.conf.example

# GNU build system regeneration script and others

install-sys: $(dist_sysconf_DATA)
	$(MKDIR_P) $(DESTDIR)$(sysconfdir)
	$(INSTALL_DATA) $^ $(DESTDIR)$(sysconfdir)

install-pic:  $(wildcard /cygdrive/c/Users/fabrn/dvda-author/menu/*)
	$(MKDIR_P) $(DESTDIR)/usr/local/share/applications/dvda-author-dev/menu
	$(INSTALL_DATA) $^ $(DESTDIR)/usr/local/share/applications/dvda-author-dev/menu

install-man:
	@ [ -f $(PROGRAM).1 ] && $(INSTALL_DATA) $(PROGRAM).1 /usr/local/share/man/man1

install-data-local:  $(dist_doc_DATA) $(dist_pic_DATA)
	$(MKDIR_P)  $(DESTDIR)/usr/local/share/info/$(PROGRAM)
	$(INSTALL_DATA) $^ $(DESTDIR)/usr/local/share/info/$(PROGRAM)
	@ [ -f $(PROGRAM).html ] && $(INSTALL_DATA) \
                           		$(PROGRAM).html \
					$(DESTDIR)/usr/local/share/info/$(PROGRAM)

# redefining install is necessary to rename nested package directories for later
# builds with --enable-all-builds, which request lib* labelling

install-strip: install

install:  /cygdrive/c/Users/fabrn/dvda-author/src/$(PROGRAM) install-data-local install-pic install-sys \
install-man
	$(MKDIR_P) $(DESTDIR)/usr/local/bin
	@$(foreach dir,src $(subdirs), $(MAKE) --directory=$(dir) install)

clean: clean-local
	$(call clean_directory,/cygdrive/c/Users/fabrn/dvda-author/libutils/src $(BUILD_SUBDIRS))
	$(RM) /cygdrive/c/Users/fabrn/dvda-author/src/$(PROGRAM)

clean-local:
	$(RM) $(PROGRAM).1 $(PROGRAM).html .dvda-author
	[ -d /cygdrive/c/Users/fabrn/dvda-author/depconf ] && $(RM) $(wildcard /cygdrive/c/Users/fabrn/dvda-author/depconf/*.depconf)

distclean: clean
	$(RM) -rf $(wildcard /cygdrive/c/Users/fabrn/dvda-author/autom4te*)
	$(RM) -rf config.*
	$(RM) Makefile src/Makefile libfixwav/src/Makefile libutils/src/Makefile \
libiberty/src/Makefile

maintainer-clean: distclean
	$(RM) configure  sox-libs *.tar.xz *-patch-* 
	$(RM) -rf config/  depconf/
	$(foreach prog,  sox libogg FLAC ffmpeg dvdauthor lplex mjpegtools cdrtools a52dec libmpeg2 help2man ImageMagick man2html libiberty libfixwav, $(shell $(RM) -rf $(MAYBE_$(prog)))) 

FORMATS=xz gz bz2 zip
DISTFORMATS=$(FORMATS:%=dist-%)

$(FORMATS): ;

#master by default, can be overridden on commandline e.g. make dist-xz
#BRANCH=win32

$(DISTFORMATS):dist-%: %
	@if test "yes" = "yes"; then
		BRANCH=master
		CURRRENTBRANCH=`/usr/bin/git status -bs -u no -z | sed -e 's/[# ]*//g'`
		if test "$(BRANCH)" != "$(CURRENTBRANCH)"; then
		  /usr/bin/git checkout $(BRANCH)
		fi
		/usr/bin/git archive --prefix=$(PROGRAM)/  -o $(PROGRAM).tar HEAD $(BRANCH)
		tar cJvf $(PROGRAM).tar.xz $(PROGRAM).tar && rm -f $(PROGRAM).tar
		if test "$(BRANCH)" != "$(CURRENTBRANCH)"; then
		  /usr/bin/git checkout $(CURRENTBRANCH)
		fi
	else
		echo Please install git to make a distribution package.
	fi

dist: dist-xz

force: ;
