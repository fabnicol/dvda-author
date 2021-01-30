include /home/fab/Documents/dvda-author/mk/dvdauthor.global.mk

dvdauthor_MAKESPEC=manual
dvdauthor_TESTBINARY=dvdauthor

ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/home/fab/Documents/dvda-author/depconf/dvdauthor.depconf: Makefile
	@mkdir -p /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor)/srcm4
	 cp -f /home/fab/Documents/dvda-author/m4.extra.dvdauthor/*  /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor)/srcm4
	$(if $(MAYBE_dvdauthor),
	  if test -d /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor) ; then
	   cd /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor) && $(SHELL) -c "aclocal -Isrcm4 && autoheader && automake -acf && autoconf -Isrcm4" 
	    if test "$$?" = "0"; then 
	    cd /home/fab/Documents/dvda-author
	   else 
	    echo "autoreconf failed for dvdauthor"
	    exit -1
	   fi
	   mkdir -p /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor) && cd /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor)
	   $(SHELL) /home/fab/Documents/dvda-author/$(MAYBE_dvdauthor)/configure  $(CONFIGURE_dvdauthor_FLAGS) && $(MAKE) $(PARALLEL) && $(MAKE) install
	   if test "$$?" = "0"; then touch /home/fab/Documents/dvda-author/depconf/dvdauthor.depconf; fi
	   cd /home/fab/Documents/dvda-author
	  fi)
	$(call index,dvdauthor,$(EXEEXT),binary)
else
/home/fab/Documents/dvda-author/depconf/dvdauthor.depconf: 
	echo "Please install libtoolize"
	exit -1


endif
else
/home/fab/Documents/dvda-author/depconf/dvdauthor.depconf: 
	echo "Please install automake"
	exit -1

endif
else

/home/fab/Documents/dvda-author/depconf/dvdauthor.depconf: 
	echo "Please install autoconf"
	exit -1
endif
