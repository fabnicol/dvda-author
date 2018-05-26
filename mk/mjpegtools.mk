include /home/fab/Dev/dvda-author-dev/mk/mjpegtools.global.mk

mjpegtools_MAKESPEC=auto
mjpegtools_CONFIGSPEC=exe
mjpegtools_TESTBINARY=mplex

/home/fab/Dev/dvda-author-dev/depconf/mjpegtools.depconf: $(mjpegtools_DEPENDENCY)
	$(call depconf,mjpegtools)  
	 
