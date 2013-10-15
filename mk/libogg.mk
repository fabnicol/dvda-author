include /home/fab/Dev/dvda-author-dev/mk/libogg.global.mk

libogg_MAKESPEC=auto
libogg_CONFIGSPEC=lib
libogg_DEPENDENCY=$(MAYBE_libogg)
libogg_TARGETLIB=libogg.a

#Start of autoconf-substituted, do not modify#

libogg.depconf: $(libogg_DEPENDENCY)
	$(call depconf,$*)

#end of autoconf-substituted

