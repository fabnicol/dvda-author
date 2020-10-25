include /cygdrive/c/Users/fabrn/dvda-author/mk/help2man.global.mk

# Perl binary, no extension even under Windows

help2man_MAKESPEC=auto
help2man_CONFIGSPEC=
help2man_TESTBINARY=help2man

/cygdrive/c/Users/fabrn/dvda-author/depconf/help2man.depconf: 
	cd $(MAYBE_help2man)
	./configure --prefix=/cygdrive/c/Users/fabrn/dvda-author/local
	make && make install
	cd /cygdrive/c/Users/fabrn/dvda-author
