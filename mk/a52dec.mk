include /home/fab/Dev/dvda-author/mk/a52dec.global.mk

a52dec_MAKESPEC=auto
a52dec_CONFIGSPEC=exe
a52dec_TESTBINARY=a52dec

/home/fab/Dev/dvda-author/depconf/a52dec.depconf: $(a52dec_DEPENDENCY)
	$(call depconf,a52dec)
