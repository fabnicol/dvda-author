include /home/fab/Documents/dvda-author/mk/mjpegtools.global.mk

mjpegtools_MAKESPEC=auto
mjpegtools_CONFIGSPEC=exe
mjpegtools_TESTBINARY=mplex

/home/fab/Documents/dvda-author/depconf/mjpegtools.depconf: $(mjpegtools_DEPENDENCY)
	$(call depconf,mjpegtools)  
	 
