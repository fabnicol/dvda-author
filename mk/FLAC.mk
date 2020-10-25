include /cygdrive/c/Users/fabrn/dvda-author/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/cygdrive/c/Users/fabrn/dvda-author/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/cygdrive/c/Users/fabrn/dvda-author/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
