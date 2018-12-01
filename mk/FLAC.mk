include /c/Users/Public/Dev/dvd-audio-dev/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/c/Users/Public/Dev/dvd-audio-dev/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/c/Users/Public/Dev/dvd-audio-dev/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
