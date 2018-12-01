include /home/fab/Desktop/dvda-author-dev/mk/cdrtools.global.mk

cdrtools_MAKESPEC=auto
cdrtools_CONFIGSPEC=exe
cdrtools_TESTBINARY=cdrecord
INS_BASE="/home/fab/Desktop/dvda-author-dev/local"


/home/fab/Desktop/dvda-author-dev/depconf/cdrtools.depconf: $(cdrtools_DEPENDENCY)
	$(call depconf,cdrtools,noconfigure,"-k")


# symlink issues with cdrtools under gmake make it almost necessary to use -k to avoid blocking the whole build chain
# the alternative smake system written by cdrtools'author would avoid this yet does not support (08-2016) parallel builds.
