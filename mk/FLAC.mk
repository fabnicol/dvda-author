include /home/fab/Documents/dvda-author/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/home/fab/Documents/dvda-author/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/home/fab/Documents/dvda-author/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
