# Makefile for help2man

# Copyright (C) 2012 Free Software Foundation, Inc.

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

# Written by Brendan O'Dea <bod@debian.org>

srcdir = .
prefix = /home/fab/Dev/dvda-author2/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
pkglibdir = $(libdir)/help2man
datarootdir = ${prefix}/share
mandir = ${datarootdir}/man
datadir = ${datarootdir}
infodir = ${datarootdir}/info
localedir = $(datadir)/locale

export SHELL = /bin/bash
export VPATH = .:$(srcdir)

DESTDIR =
LINGUAS = $(basename $(notdir $(wildcard $(srcdir)/po/*.po)))

CC = gcc
PERL = perl
LIBS = -ldl 
INSTALL = /bin/sh /home/fab/Dev/dvda-author2/config/install-sh 
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = /bin/sh /home/fab/Dev/dvda-author2/config/install-sh  -m 644
INSTALL_INFO = /usr/bin/install-info
MAKEINFO = /usr/bin/makeinfo
MSGFMT = /usr/bin/msgfmt
MKINSTALLDIRS = $(srcdir)/mkinstalldirs

target = help2man
preload = bindtextdomain

# find file in vpath
vpath_file = $$($(PERL) -e 'print +(grep -f, map "$$_/$$ARGV[0]", \
    map +(length) ? $$_ : ".", split ":", $$ENV{VPATH} || ".")[0]' $(1))

all: $(target) man info preload man_l10n

install: all install_base install_preload install_l10n
install_dirs:
	$(MKINSTALLDIRS) $(DESTDIR)$(bindir)
	$(MKINSTALLDIRS) $(DESTDIR)$(pkglibdir)
	$(MKINSTALLDIRS) $(DESTDIR)$(mandir)/man1
	$(MKINSTALLDIRS) $(DESTDIR)$(infodir)

install_base: install_dirs
	$(INSTALL_SCRIPT) $(target) $(DESTDIR)$(bindir)
	$(INSTALL_DATA) $(call vpath_file,$(target).1) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(call vpath_file,$(target).info) \
	    $(DESTDIR)$(infodir)/$(target).info

	if test -f $(DESTDIR)$(infodir)/dir; \
	then \
	    $(INSTALL_INFO) --info-dir=$(DESTDIR)$(infodir) \
		$(DESTDIR)$(infodir)/$(target).info; \
	fi

install_preload: install_dirs preload
	$(INSTALL_PROGRAM) $(preload).so $(DESTDIR)$(pkglibdir)

install_l10n: install_dirs msg_l10n man_l10n
	for lang in $(LINGUAS); \
	do \
	    $(MKINSTALLDIRS) $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	    $(INSTALL_DATA) $(call vpath_file,po/$$lang.gmo) \
		$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(target).mo; \
	    $(MKINSTALLDIRS) $(DESTDIR)$(mandir)/$$lang/man1; \
	    $(INSTALL_DATA) $(call vpath_file,$(target).$$lang.1) \
		$(DESTDIR)$(mandir)/$$lang/man1/$(target).1; \
	done

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(target) \
	    $(DESTDIR)$(pkglibdir)/$(preload).so \
	    $(DESTDIR)$(mandir)/man1/$(target).1 \
	    $(DESTDIR)$(mandir)/*/man1/$(target).1 \
	    $(DESTDIR)$(localedir)/*/LC_MESSAGES/$(target).mo \
	    $(DESTDIR)$(infodir)/$(target).info

	if test -f $(DESTDIR)$(infodir)/dir; \
	then \
	    $(INSTALL_INFO) --delete --info-dir=$(DESTDIR)$(infodir) \
		$(target).info; \
	fi

clean:
	-rm -f $(target) $(target).h2m $(target).*.h2m $(preload).so *.tmp[0-9]*
	-rm -rf localetmp

mostlyclean: clean
	rm -f $(target).dvi $(target).aux $(target).cp $(target).cps \
	    $(target).fn $(target).ky $(target).log $(target).pg $(target).toc \
	    $(target).tp $(target).vr po/*.po~

distclean: mostlyclean
	-rm -rf config.cache config.log config.status Makefile autom4te.cache

realclean: distclean
	rm -f $(target).info $(target).1 $(target).*.1 po/*.gmo

maintainer-clean: realclean
	rm -f $(srcdir)/configure

$(target): $(srcdir)/$(target).PL
	$(PERL) $? --with-gettext

$(target).h2m: $(srcdir)/$(target).h2m.PL
	$(PERL) $?

preload: $(preload).so
$(preload).so: $(srcdir)/$(preload).c
	$(CC) $(CFLAGS) -o $@ -fPIC -shared $? $(LIBS)

man: $(target).1
$(target).1: $(srcdir)/$(target).PL $(srcdir)/$(target).h2m.PL
	$(MAKE) $(target) $(target).h2m
	./$(target) --include=$(target).h2m --output=$@ ./$(target)

msg_l10n: $(addprefix po/,$(addsuffix .gmo,$(LINGUAS)))
po/%.gmo: $(srcdir)/po/%.po
	test -d po || mkdir po
	$(MSGFMT) -o $@ $?

localetmp/%/LC_MESSAGES/$(target).mo: po/%.gmo
	lang=$(patsubst localetmp/%/LC_MESSAGES/$(target).mo,%,$@); \
	$(MKINSTALLDIRS) localetmp/$$lang/LC_MESSAGES && \
	$(INSTALL_DATA) po/$$lang.gmo localetmp/$$lang/LC_MESSAGES/$(target).mo

man_l10n: $(addprefix $(target).,$(addsuffix .1,$(LINGUAS)))
$(target).%.1: $(srcdir)/$(target).PL $(srcdir)/$(target).h2m.PL \
    $(srcdir)/po/%.po
	lang=$(patsubst $(target).%.1,%,$@); \
	$(MAKE) $(target) $(preload).so \
	    localetmp/$$lang/LC_MESSAGES/$(target).mo $(target).$$lang.h2m; \
	LD_PRELOAD=./$(preload).so LOCALEDIR=localetmp TEXTDOMAIN=help2man \
	    ./$(target) --include=$(call vpath_file,$(target).$$lang.h2m) \
		--output=$@ ./$(target)

$(target).%.h2m: $(srcdir)/$(target).h2m.PL $(srcdir)/po/%.po
	lang=$(patsubst $(target).%.h2m,%,$@); \
	locale=$$(echo $$lang $$lang | \
	     sed -n "/^$$lang"'\s/{s/^\S\+\s\+//p; q}' $(srcdir)/locales -); \
	$(MAKE) localetmp/$$lang/LC_MESSAGES/$(target).mo; \
	$(PERL) $(srcdir)/$(target).h2m.PL --locale="$$locale" \
	    --message-dir=localetmp --output=$@

info: $(target).info
$(target).info: $(srcdir)/$(target).texi
	$(MAKEINFO) $? -o $@

dvi: $(target).dvi
$(target).dvi: $(srcdir)/$(target).texi
	$(TEXI2DVI) $?

Makefile: $(srcdir)/Makefile.in
	./config.status

update-po: $(srcdir)/po/$(target).pot
	for lang in $(LINGUAS); \
	do \
	    echo -n "Updating $$lang "; \
	    msgmerge -U $(srcdir)/po/$$lang.po $(srcdir)/po/$(target).pot; \
	done

$(srcdir)/po/$(target).pot: $(srcdir)/$(target).PL $(srcdir)/$(target).h2m.PL
	$(MAKE) $(target)
	xgettext -L Perl -k_ -kN_ -cTranslators: \
	    --msgid-bugs-address="Brendan O'Dea <bug-help2man@gnu.org>" -o $@ \
	    $(target) $(srcdir)/$(target).h2m.PL

.PHONY: all clean distclean preload man msg_l10n man_l10n info dvi install \
    install_base install_preload install_l10n mostlyclean realclean \
    uninstall

.SUFFIXES:
