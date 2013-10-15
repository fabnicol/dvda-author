help2man_MAKESPEC=manual

#Autoconf-substituted, do not modify#
help2man_LIB=
help2man_LINK=
MAYBE_help2man=
HAVE_help2man=no
HAVE_EXTERNAL_help2man=@HAVE_EXTERNAL_help2man@

help2man: Makefile
	if test -f $@; then rm -f $@; fi; touch $@
	if test "$(MAYBE_$@)" != "" ; then
	  if test -d $(MAYBE_$@) ; then
		cd $(MAYBE_$@) && $(SHELL) configure --prefix=${prefix} ; $(MAKE) ; cd -
	  fi
	fi
	$(call execfollow,$(MAYBE_$@),$@,--version|line)
	if test -f $(MAYBE_$@)/$@ ; then
		  $(MAYBE_$@)/$@ -s 1 -N -o dvda-author.1 src/dvda
		  $(call docfollow,dvda-author.1)
		  manhelp=dvda-author.1
	fi
