include /home/fab/Dev/dvda-author/mk/libogg.global.mk

libogg_MAKESPEC=auto
libogg_CONFIGSPEC=lib
libogg_TARGETLIB=libogg.a

/home/fab/Dev/dvda-author/depconf/libogg.depconf: $(libogg_DEPENDENCY)
	$(call depconf,libogg)

