# file: Makefile.in Copyright Fabrice Nicol 2013
# do not use automake
# Caution: you need GNU make, version 3.82 and higher, preferably 3.99+

# Do not use bash. It would cause subtle issues with libtool in the flac
# package, probably a libtool bug.

PROGRAM=dvda-author-dev
export PROGRAM
INSTALL=$(SHELL) /home/fab/dvda-author/config/install-sh
INSTALL_DATA=$(SHELL) /home/fab/dvda-author/config/install-sh  -m 644
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
$(shell if test -f /home/fab/dvda-author/mk/$(prog).mk; then \
echo /home/fab/dvda-author/mk/$(prog).mk; fi))
PROGRAM_TARGET_CONFIGS := $(foreach prog,$(PROGRAM_TARGETS), \
$(shell if test  -f /home/fab/dvda-author/mk/$(prog).mk ; then echo $(prog).config; fi))
PROGRAM_TARGET_DEPCONFS := $(PROGRAM_TARGET_CONFIGS:.config=.depconf)

ifneq "" "no"
  ifneq "libiberty/src" ""
    BUILD_SUBDIRS += /home/fab/dvda-author/libiberty/src
  endif
else
  CPPFLAGS += -DWITHOUT_libiberty
endif

BUILD_SUBDIRS += /home/fab/dvda-author/libfixwav/src

BUILD_SUBDIRS += /home/fab/dvda-author/src

.SUFFIXES: .config .depconf

ALL_TARGETS_EXTERNAL=erase_build.trace

ALL_TARGETS_EXTERNAL += $(PROGRAM_TARGET_CONFIGS)

ifeq "yes" "yes"
 ALL_TARGETS_EXTERNAL += do_sox_lib_deps_subst
endif

ALL_TARGETS += $(ALL_TARGETS_EXTERNAL) $(BUILD_SUBDIRS)

.PHONY: all    libutils

all:  $(ALL_TARGETS) $(PROGRAM).1 $(PROGRAM).html

include /home/fab/dvda-author/mk/functions.mk
include $(PROGRAM_TARGET_MAKEFILES)

help2man.config: /home/fab/dvda-author/depconf/help2man.depconf
man2html.config: /home/fab/dvda-author/depconf/man2html.depconf

export CPPFLAGS
export PARALLEL

libutils:
	$(MAKE) --directory=/home/fab/dvda-author/libutils/src

$(BUILD_SUBDIRS): $(ALL_TARGETS_EXTERNAL) libutils
	@echo Running make in directory $@...
	$(MAKE) $(PARALLEL) --directory=$@

$(PROGRAM_TARGET_CONFIGS): %.config: /home/fab/dvda-author/depconf/%.depconf
	@echo
	echo Finished building $*...

erase_build.trace:
	@echo PROGRAM_TARGETS=$(PROGRAM_TARGETS)
	if test -f /home/fab/dvda-author/depconf/BUILD.TRACE; then
	  mv /home/fab/dvda-author/depconf/BUILD.TRACE /home/fab/dvda-author/depconf/BUILD.TRACE~
	fi

# directly patching the Makefile appears to be more efficient. You need a
# GNU-compliant make.

#this must be lazy-evaluation otherwise will not work
pkgconfig_style_libs=$(if $(MAYBE_sox),$(shell cat						\
/home/fab/dvda-author/$(MAYBE_sox)/sox-libs | /usr/bin/sed -e "s/^.*\.a//g ; s/@.*@//g;	\
s,\/,\\\/,g"))

do_sox_lib_deps_subst: /home/fab/dvda-author/src/Makefile  /home/fab/dvda-author/$(MAYBE_sox)/sox-libs
	@echo Processing dependencies for sox...
	/usr/bin/sed -i -e 's/SOX_LIB_DEPS/$(pkgconfig_style_libs)/g' \
/home/fab/dvda-author/src/Makefile
	echo "[sox] done sox library substitution with libs: $(pkgconfig_style_libs)"\
>> /home/fab/dvda-author/depconf/BUILD.TRACE

manpage: $(PROGRAM).1

htmlpage: $(PROGRAM).html

pdf: $(PROGRAM).html
		[ -f "/usr/bin/wkhtmltopdf" ] && /usr/bin/wkhtmltopdf $(PROGRAM).html $(PROGRAM).pdf

$(PROGRAM).1:
	@if test "no" = "yes"; then
	  if test -f /home/fab/dvda-author/src/$(PROGRAM); then
		 -s 1 -N -o $(PROGRAM).1 /home/fab/dvda-author/src/$(PROGRAM)
	  fi
	else
	  if test -f src/$(PROGRAM) && test -d "help2man-1.47.9" \
&& test -f "help2man-1.47.9"/help2man; then
		"/home/fab/dvda-author/help2man-1.47.9"/help2man -s 1 -N \
-o $(PROGRAM).1 /home/fab/dvda-author/src/$(PROGRAM)
	  fi
	fi
	$(call docfollow,  $@)


$(PROGRAM).html: $(PROGRAM).1
	@if test "no" = "yes"; then
		 < $(PROGRAM).1 > $@
		$(call docfollow, $@)
	else
	  if test -f "/home/fab/dvda-author/local/bin/man2html" ; then
		"/home/fab/dvda-author/local/bin/man2html" < $(PROGRAM).1 > $@
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

dist_sysconf_DATA=/home/fab/dvda-author/dvda-author.desktop /home/fab/dvda-author/dvda-author.conf

# distributed under $menudir=$sysconfdir/menu
# normally $prefix/share/pixmaps/dvda-author
# distributed under $pixdir

dist_pic_DATA= /home/fab/dvda-author/dvda-author_48x48.png /home/fab/dvda-author/dvda-author_64x64.png
# distributed under $docdir, normally $prefix/doc/dvda-author

dist_doc_DATA=/home/fab/dvda-author/README  /home/fab/dvda-author/BUGS /home/fab/dvda-author/EXAMPLES \
/home/fab/dvda-author/LIMITATIONS /home/fab/dvda-author/BUILD.Ubuntu /home/fab/dvda-author/COREBUILD \
  /home/fab/dvda-author/DEPENDENCIES /home/fab/dvda-author/HOWTO.conf /home/fab/dvda-author/dvda-author.conf.example

# GNU build system regeneration script and others

install-sys: $(dist_sysconf_DATA)
	$(MKDIR_P) $(DESTDIR)$(sysconfdir)
	$(INSTALL_DATA) $^ $(DESTDIR)$(sysconfdir)

install-pic:  $(wildcard /home/fab/dvda-author/menu/*)
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

install:  /home/fab/dvda-author/src/$(PROGRAM) install-data-local install-pic install-sys \
install-man
	$(MKDIR_P) $(DESTDIR)/usr/local/bin
	@$(foreach dir,src $(subdirs), $(MAKE) --directory=$(dir) install)

clean: clean-local
	$(call clean_directory,/home/fab/dvda-author/libutils/src $(BUILD_SUBDIRS))
	$(RM) /home/fab/dvda-author/src/$(PROGRAM)

clean-local:
	$(RM) $(PROGRAM).1 $(PROGRAM).html .dvda-author
	[ -d /home/fab/dvda-author/depconf ] && $(RM) $(wildcard /home/fab/dvda-author/depconf/*.depconf)

distclean: clean
	$(RM) -rf $(wildcard /home/fab/dvda-author/autom4te*)
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
