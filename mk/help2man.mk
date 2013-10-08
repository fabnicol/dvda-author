help2man_MAKESPEC=manual

help2man: Makefile
	if test -f help2man; then rm -f help2man; fi; touch help2man
	if test "$(MAYBE_$@)" != "" ; then
	  if test -d $(MAYBE_$@) ; then
		cd $(MAYBE_$@) && $(SHELL) configure --prefix=${prefix} ; $(MAKETOOL) ; cd -
	  fi
	fi
	$(call execfollow,$(MAYBE_$@),$@,--version|line)
	if test -f $(MAYBE_$@)/help2man ; then
		  $(MAYBE_$@)/help2man -s 1 -N -o dvda-author.1 src/dvda
		  $(call docfollow,dvda-author.1)
		  manhelp=dvda-author.1
	fi
