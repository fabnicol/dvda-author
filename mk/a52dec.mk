include /home/fab/Desktop/dvda-author-dev/mk/a52dec.global.mk

a52dec_MAKESPEC=auto
a52dec_CONFIGSPEC=exe
a52dec_TESTBINARY=a52dec

/home/fab/Desktop/dvda-author-dev/depconf/a52dec.depconf: $(a52dec_DEPENDENCY)
	$(call depconf,a52dec)
