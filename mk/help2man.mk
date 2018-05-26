include /home/fab/Dev/dvda-author-dev/mk/help2man.global.mk

help2man_MAKESPEC=auto
help2man_CONFIGSPEC=exe
help2man_TESTBINARY=help2man

/home/fab/Dev/dvda-author-dev/depconf/help2man.depconf: 
	$(call depconf,help2man)
