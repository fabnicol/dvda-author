sox_MAKESPEC=manual

#Autoconf-substituted, do not modify#
sox_LIB=@sox_LIB@
sox_LINK=@sox_LINK@
MAYBE_sox=@MAYBE_sox@
HAVE_a52dec=@HAVE_a52_dec@
HAVE_EXTERNAL_a52dec=@HAVE_EXTERNAL_a52dec@


sox: Makefile
	if test  "$(MAYBE_$@)" != ""; then
		if test -d "@BUILDDIR@/$(MAYBE_$@)" ; then
			cp "@BUILDDIR@/sox-libs" "@BUILDDIR@/$(MAYBE_$@)/sox-libs.in"
			else
			echo "@BUILDDIR@/$(MAYBE_$@) is not a directory"
			exit -1
		fi
	fi
	$(call config_lib_package,$@,autoreconf -if)

