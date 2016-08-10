include /mnt/fab/Dev/dvda-author-dev/BUILD/mk/FLAC.global.mk

FLAC_MAKESPEC=auto
FLAC_CONFIGSPEC=lib
FLAC_DEPENDENCY=/mnt/fab/Dev/dvda-author-dev/BUILD/depconf/libogg.depconf 
FLAC_TARGETLIB=libFLAC.a

/mnt/fab/Dev/dvda-author-dev/BUILD/depconf/FLAC.depconf: $(FLAC_DEPENDENCY)
	$(call depconf,FLAC,"","",CFLAGS=)
