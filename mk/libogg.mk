include /home/fab/Desktop/dvda-author-dev/mk/libogg.global.mk

libogg_MAKESPEC=auto
libogg_CONFIGSPEC=lib
libogg_TARGETLIB=libogg.a

/home/fab/Desktop/dvda-author-dev/depconf/libogg.depconf: $(libogg_DEPENDENCY)
	$(call depconf,libogg)

