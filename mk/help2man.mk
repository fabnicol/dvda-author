include /home/fab/dvda-author/mk/help2man.global.mk

# Perl binary, no extension even under Windows

help2man_MAKESPEC=auto
help2man_CONFIGSPEC=
help2man_TESTBINARY=help2man

/home/fab/dvda-author/depconf/help2man.depconf: 
	cd $(MAYBE_help2man)
	./configure --prefix=/home/fab/dvda-author/local
	make && make install
	cd /home/fab/dvda-author
	touch depconf/help2man.depconf
