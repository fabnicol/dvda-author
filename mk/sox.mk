include /home/fab/Dev/dvda-author/mk/sox.global.mk

sox_MAKESPEC=manual
ifeq "yes" "yes"
ifeq "yes" "yes"
ifeq "yes" "yes"

/home/fab/Dev/dvda-author/depconf/sox.depconf: $(sox_DEPENDENCY)
	@if test  "$(MAYBE_sox)" != ""; then
		if test -d "/home/fab/Dev/dvda-author/$(MAYBE_sox)" ; then
			cp "/home/fab/Dev/dvda-author/sox-libs" "/home/fab/Dev/dvda-author/$(MAYBE_sox)/sox-libs.in"
			else
			echo "/home/fab/Dev/dvda-author/$(MAYBE_sox) is not a directory"
			exit -1
		fi
	fi
	$(call configure_lib_package,sox,/usr/bin/autoreconf,-if,CFLAGS=)
else
/home/fab/Dev/dvda-author/depconf/sox.depconf: 
	echo "Please install libtoolize"
	exit -1

endif
else
/home/fab/Dev/dvda-author/depconf/sox.depconf:
	echo "Please install automake"
	exit -1

endif
else
/home/fab/Dev/dvda-author/depconf/sox.depconf:
	echo "Please install autoconf"
	exit -1
endif
