include /home/fab/Desktop/dvda-author-dev/mk/dvdauthor.global.mk

dvdauthor_MAKESPEC=manual
dvdauthor_TESTBINARY=dvdauthor

ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/home/fab/Desktop/dvda-author-dev/depconf/dvdauthor.depconf: Makefile
	@mkdir -p /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor)/srcm4
	 cp -f /home/fab/Desktop/dvda-author-dev/m4.extra.dvdauthor/*  /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor)/srcm4
	$(if $(MAYBE_dvdauthor),
	  if test -d /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor) ; then
	   cd /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor) && $(SHELL) -c "aclocal -Isrcm4 && autoheader && automake -acf && autoconf -Isrcm4" 
	    if test "$$?" = "0"; then 
	    cd /home/fab/Desktop/dvda-author-dev
	   else 
	    echo "autoreconf failed for dvdauthor"
	    exit -1
	   fi
	   mkdir -p /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor) && cd /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor)
	   $(SHELL) /home/fab/Desktop/dvda-author-dev/$(MAYBE_dvdauthor)/configure  $(CONFIGURE_dvdauthor_FLAGS) && $(MAKE) $(PARALLEL) && $(MAKE) install
	   if test "$$?" = "0"; then touch /home/fab/Desktop/dvda-author-dev/depconf/dvdauthor.depconf; fi
	   cd /home/fab/Desktop/dvda-author-dev
	  fi)
	$(call index,dvdauthor,$(EXEEXT),binary)
else
/home/fab/Desktop/dvda-author-dev/depconf/dvdauthor.depconf: 
	echo "Please install libtoolize"
	exit -1


endif
else
/home/fab/Desktop/dvda-author-dev/depconf/dvdauthor.depconf: 
	echo "Please install automake"
	exit -1

endif
else

/home/fab/Desktop/dvda-author-dev/depconf/dvdauthor.depconf: 
	echo "Please install autoconf"
	exit -1
endif
