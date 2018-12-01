include /home/fab/Desktop/dvda-author-dev/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/home/fab/Desktop/dvda-author-dev/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/home/fab/Desktop/dvda-author-dev/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
