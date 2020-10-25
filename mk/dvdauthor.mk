include /cygdrive/c/Users/fabrn/dvda-author/mk/dvdauthor.global.mk

dvdauthor_MAKESPEC=manual
dvdauthor_TESTBINARY=dvdauthor.exe

ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/cygdrive/c/Users/fabrn/dvda-author/depconf/dvdauthor.depconf: Makefile
	@mkdir -p /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor)/srcm4
	 cp -f /cygdrive/c/Users/fabrn/dvda-author/m4.extra.dvdauthor/*  /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor)/srcm4
	$(if $(MAYBE_dvdauthor),
	  if test -d /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor) ; then
	   cd /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor) && $(SHELL) -c "aclocal -Isrcm4 && autoheader && automake -acf && autoconf -Isrcm4" 
	    if test "$$?" = "0"; then 
	    cd /cygdrive/c/Users/fabrn/dvda-author
	   else 
	    echo "autoreconf failed for dvdauthor"
	    exit -1
	   fi
	   mkdir -p /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor) && cd /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor)
	   $(SHELL) /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_dvdauthor)/configure  $(CONFIGURE_dvdauthor_FLAGS) && $(MAKE) $(PARALLEL) && $(MAKE) install
	   if test "$$?" = "0"; then touch /cygdrive/c/Users/fabrn/dvda-author/depconf/dvdauthor.depconf; fi
	   cd /cygdrive/c/Users/fabrn/dvda-author
	  fi)
	$(call index,dvdauthor,$(EXEEXT),binary)
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/dvdauthor.depconf: 
	echo "Please install libtoolize"
	exit -1


endif
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/dvdauthor.depconf: 
	echo "Please install automake"
	exit -1

endif
else

/cygdrive/c/Users/fabrn/dvda-author/depconf/dvdauthor.depconf: 
	echo "Please install autoconf"
	exit -1
endif
