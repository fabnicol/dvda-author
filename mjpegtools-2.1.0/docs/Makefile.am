# If the installation of man pages by you (e.g. you're a package builder)
# You may want to disable it
man_MANS = lavplay.1  lavrec.1  mjpegtools.1  mpeg2enc.1  mplex.1 \
	yuvplay.1 lav2wav.1 lav2yuv.1 lavtrans.1 mp2enc.1 \
	yuvscaler.1 lavpipe.1 yuv2lav.1 yuvdenoise.1 jpeg2yuv.1 \
	png2yuv.1 \
        pgmtoy4m.1 ppmtoy4m.1 y4mtoppm.1 y4mcolorbars.1 \
	y4mtopnm.1 pnmtoy4m.1 \
        yuvkineco.1 yuvycsnoise.1 yuvmedianfilter.1 \
	y4munsharp.1 y4mscaler.1 \
	lav2mpeg.1 yuv4mpeg.5 yuvfps.1 yuvinactive.1 y4mdenoise.1

info_INFO = mjpeg-howto.info
	
EXTRA_DIST = \
	$(man_MANS) \
	FAQ.txt \
	authors.sgml \
	lavtools.sgml

MAINTAINERCLEANFILES = Makefile.in

# (Ronald) this stuff comes from texinfos.am on my
# installation. We need it for proper in-auto-build
# installation of info pages.
# It's adapted to handle the debian install-info case
# too...

INSTALL_INFO = /sbin/install-info
INFO_ENTRY = "* mjpeg-howto: (mjpeg-howto).        How to use the mjpeg tools"
INFO_DESCRIPTION = "mjpeg tools documentation"

install-exec-hook: install-info
dist-hook: dist-info

## Look in both . and srcdir because the info pages might have been
## rebuilt in the build directory.  Can't cd to srcdir; that might
## break a possible install-sh reference.
## Funny name due to --cygnus influence; we want to reserve
## `install-info' for the user.
install-info-am: $(info_INFO)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(infodir)
	@list='$(info_INFO)'; \
	for file in $$list; do \
	  d=$(srcdir); \
## We use these strange circumlocutions because we want the "ifile" to
## be relative, for the install.
	  for ifile in `cd $$d && echo $$file $$file-[0-9] $$file-[0-9][0-9]`; do \
	    if test -f $$d/$$ifile; then \
	      echo " $(INSTALL_DATA) $$d/$$ifile $(DESTDIR)$(infodir)/$$ifile"; \
	      $(INSTALL_DATA) $$d/$$ifile $(DESTDIR)$(infodir)/$$ifile; \
	    else : ; fi; \
	  done; \
	done
	@$(POST_INSTALL)
	@if test x$(INSTALL_INFO) != x && test -f $(INSTALL_INFO); then \
	  list='$(info_INFO)'; \
	  for file in $$list; do \
## Run `:' after install-info in case install-info fails.  We really
## don't care about failures here, because they can be spurious.  For
## instance if you don't have a dir file, install-info will fail.  I
## think instead it should create a new dir file for you.  This bug
## causes the `make distcheck' target to fail reliably.
	    echo " install-info --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/$$file";\
## Use `|| :' here because Sun make passes -e to sh; if install-info
## fails then we'd fail if we used `;'.
	    if $(SHELL) -c '$(INSTALL_INFO) --version | sed 1q | fgrep -s -v -i debian' >/dev/null 2>&1; then \
	      $(INSTALL_INFO) --entry=$(INFO_ENTRY) --info-dir=$(DESTDIR)$(infodir) $(DESTDIR)$(infodir)/$$file || :;\
	    else \
## Special Debian case
	      $(INSTALL_INFO) --infodir=$(DESTDIR)$(infodir) --menuentry=$(INFO_ENTRY) --description=$(INFO_DESCRPITION) $(DESTDIR)$(infodir)/$$file || :; \
	    fi; \
	  done; \
	fi

uninstall-info-am:
	$(PRE_UNINSTALL)
## Run two loops here so that we can handle PRE_UNINSTALL and
## NORMAL_UNINSTALL correctly.
	if test x$(INSTALL_INFO) != x && test -f $(INSTALL_INFO); then \
	  list='$(info_INFO)'; \
	  for file in $$list; do \
	    if $(SHELL) -c '$(INSTALL_INFO) --version | sed 1q | fgrep -s -v -i debian' >/dev/null 2>&1; then \
## GNU case
	      $(INSTALL_INFO) --info-dir=$(DESTDIR)$(infodir) --remove $$file; \
	    else \
## Debian case
	      $(INSTALL_INFO) --infodir=$(DESTDIR)$(infodir) --delete $$file; \
	    fi; \
	  done; \
	fi
	@$(NORMAL_UNINSTALL)
	list='$(info_INFO)'; \
	for file in $$list; do \
	  (cd $(DESTDIR)$(infodir) && rm -f $$file $$file-[0-9] $$file-[0-9][0-9]); \
	done

dist-info: $(info_INFO)
	list='$(info_INFO)'; \
	for base in $$list; do \
	  d=$(srcdir); \
	  for file in `cd $$d && eval echo $$base*`; do \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file; \
	  done; \
	done
