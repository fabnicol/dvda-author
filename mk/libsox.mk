sox: Makefile
	if test  "$(MAYBE_$@)" != ""; then
		if test -d "$(BUILDDIR)/$(MAYBE_$@)" ; then
			cp "$(BUILDDIR)/sox-libs" "$(BUILDDIR)/$(MAYBE_$@)/sox-libs.in"
			else
			echo "$(BUILDDIR)/$(MAYBE_$@) is not a directory"
			exit -1
		fi
	fi
	$(call config_lib_package,$@,autoreconf -if)

