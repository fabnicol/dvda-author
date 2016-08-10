
include /mnt/fab/Dev/dvda-author-dev/BUILD/mk/man2html.global.mk

man2html_MAKESPEC=auto
man2html_CONFIGSPEC=exe
man2html_TESTBINARY=man2html

/mnt/fab/Dev/dvda-author-dev/BUILD/depconf/man2html.depconf: $(man2html_DEPENDENCY)
	$(call depconf,man2html,noconfigure,$(CONFIGURE_man2html_FLAGS) CFLAGS=)

