include /cygdrive/c/Users/fabrn/dvda-author/mk/help2man.global.mk

# Perl binary, no extension even under Windows

help2man_MAKESPEC=auto
help2man_CONFIGSPEC=script
help2man_TESTBINARY=help2man

/cygdrive/c/Users/fabrn/dvda-author/depconf/help2man.depconf: $(help2man_DEPENDENCY)
	$(call depconf,help2man)
