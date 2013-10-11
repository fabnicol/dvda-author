
dvdauthor_MAKESPEC=manual

#Autoconf-substituted, do not modify#
dvdauthor_LIB=@dvdauthor_LIB@
dvdauthor_LINK=@dvdauthor_LINK@
MAYBE_dvdauthor=@MAYBE_dvdauthor@
HAVE_cdrtools=@HAVE_a52_dec@
HAVE_EXTERNAL_cdrtools=@HAVE_EXTERNAL_cdrtools@

if HAVE_AUTOMAKE
if HAVE_AUTOCONF
dvdauthor: Makefile
	if test -f dvdauthor; then rm -f dvdauthor; fi; touch dvdauthor
	cp -r @ROOTDIR@/m4.extra.dvdauthor $(MAYBE_$@)/srcm4
	$(if $(MAYBE_$@),
	  if test -d $(MAYBE_$@) ; then
	   cd $(MAYBE_$@) && $(SHELL) -c "aclocal -Isrcm4 && autoheader && automake -acf && autoconf -Isrcm4" \
	   && $(SHELL) configure  --prefix=$(prefix) ; $(MAKE); cd -
	  fi)
	$(call execfollow,$(MAYBE_$@),$@,2>&1|grep version)
endif
endif

