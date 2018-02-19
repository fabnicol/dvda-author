include /home/fab/dvd-audio/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/home/fab/dvd-audio/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/home/fab/dvd-audio/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
