include /cygdrive/c/Users/fabrn/dvda-author/mk/lplex.global.mk

lplex_MAKESPEC=auto
lplex_CONFIGSPEC=exe
lplex_DEPENDENCY += /cygdrive/c/Users/fabrn/dvda-author/depconf/FLAC.depconf Makefile
lplex_TESTBINARY=lplex.exe

ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/cygdrive/c/Users/fabrn/dvda-author/depconf/lplex.depconf: $(lplex_DEPENDENCY)
	 @$(if $(MAYBE_lplex),
	  if test -d /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_lplex) ; then
	   cd /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_lplex) && $(SHELL) -c "autoreconf -if -Im4 -Iredist" 
	    if test "$$?" = "0"; then 
	    cd /cygdrive/c/Users/fabrn/dvda-author
	   else 
	    echo "autoreconf failed for lplex"
	    exit -1
	   fi
	   mkdir -p /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_lplex) && cd /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_lplex)
	   $(SHELL) /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_lplex)/configure  $(CONFIGURE_lplex_FLAGS) && $(MAKE) $(PARALLEL) && $(MAKE) install
	   if test "$$?" = "0"; then touch /cygdrive/c/Users/fabrn/dvda-author/depconf/lplex.depconf; fi
	   cd /cygdrive/c/Users/fabrn/dvda-author
	  fi)
	@$(call index,lplex,$(EXEEXT),binary)
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/lplex.depconf: 
	echo "Please install libtoolize"
	exit -1


endif
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/lplex.depconf: 
	echo "Please install automake"
	exit -1

endif
else

/cygdrive/c/Users/fabrn/dvda-author/depconf/lplex.depconf: 
	echo "Please install autoconf"
	exit -1
endif
