include /cygdrive/c/Users/fabrn/dvda-author/mk/sox.global.mk

sox_MAKESPEC=manual
ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/cygdrive/c/Users/fabrn/dvda-author/depconf/sox.depconf: $(sox_DEPENDENCY)
	@if test  "$(MAYBE_sox)" != ""; then
		if test -d "/cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_sox)" ; then
			cp "/cygdrive/c/Users/fabrn/dvda-author/sox-libs" "/cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_sox)/sox-libs.in"
			else
			echo "/cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_sox) is not a directory"
			exit -1
		fi
	fi
	$(call configure_lib_package,sox,"/usr/bin/autoreconf",-if,CFLAGS=)
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/sox.depconf: 
	echo "Please install libtoolize"
	exit -1

endif
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/sox.depconf:
	echo "Please install automake"
	exit -1

endif
else
/cygdrive/c/Users/fabrn/dvda-author/depconf/sox.depconf:
	echo "Please install autoconf"
	exit -1
endif
