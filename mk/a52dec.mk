include /cygdrive/c/Users/fabrn/dvda-author/mk/a52dec.global.mk

a52dec_MAKESPEC=auto
a52dec_CONFIGSPEC=exe
a52dec_TESTBINARY=a52dec.exe

/cygdrive/c/Users/fabrn/dvda-author/depconf/a52dec.depconf: $(a52dec_DEPENDENCY)
	cd $(MAYBE_a52dec)  && WANT_AUTOMAKE=latest autoreconf -if && cd -
	$(call depconf,a52dec)