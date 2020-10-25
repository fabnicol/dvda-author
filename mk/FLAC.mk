include /home/fab/dvda-author/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/home/fab/dvda-author/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/home/fab/dvda-author/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
