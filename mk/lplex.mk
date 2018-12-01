include /home/fab/Desktop/dvda-author-dev/mk/lplex.global.mk

lplex_MAKESPEC=auto
lplex_CONFIGSPEC=exe
lplex_DEPENDENCY += /home/fab/Desktop/dvda-author-dev/depconf/FLAC.depconf Makefile
lplex_TESTBINARY=lplex

ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/home/fab/Desktop/dvda-author-dev/depconf/lplex.depconf: $(lplex_DEPENDENCY)
	 $(if $(MAYBE_lplex),
	  if test -d /home/fab/Desktop/dvda-author-dev/$(MAYBE_lplex) ; then
	   cd /home/fab/Desktop/dvda-author-dev/$(MAYBE_lplex) && $(SHELL) -c "autoreconf -if -Im4 -Iredist" 
	    if test "$$?" = "0"; then 
	    cd /home/fab/Desktop/dvda-author-dev
	   else 
	    echo "autoreconf failed for lplex"
	    exit -1
	   fi
	   mkdir -p /home/fab/Desktop/dvda-author-dev/$(MAYBE_lplex) && cd /home/fab/Desktop/dvda-author-dev/$(MAYBE_lplex)
	   $(SHELL) /home/fab/Desktop/dvda-author-dev/$(MAYBE_lplex)/configure  $(CONFIGURE_lplex_FLAGS) && $(MAKE) $(PARALLEL) && $(MAKE) install
	   if test "$$?" = "0"; then touch /home/fab/Desktop/dvda-author-dev/depconf/lplex.depconf; fi
	   cd /home/fab/Desktop/dvda-author-dev
	  fi)
	$(call index,lplex,$(EXEEXT),binary)
else
/home/fab/Desktop/dvda-author-dev/depconf/lplex.depconf: 
	echo "Please install libtoolize"
	exit -1


endif
else
/home/fab/Desktop/dvda-author-dev/depconf/lplex.depconf: 
	echo "Please install automake"
	exit -1

endif
else

/home/fab/Desktop/dvda-author-dev/depconf/lplex.depconf: 
	echo "Please install autoconf"
	exit -1
endif
