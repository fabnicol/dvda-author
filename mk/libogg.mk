include /home/fab/Documents/dvda-author/mk/libogg.global.mk

libogg_MAKESPEC=auto
libogg_CONFIGSPEC=lib
libogg_TARGETLIB=libogg.a

/home/fab/Documents/dvda-author/depconf/libogg.depconf: $(libogg_DEPENDENCY)
	$(call depconf,libogg)

