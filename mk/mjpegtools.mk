include /cygdrive/c/Users/fabrn/dvda-author/mk/mjpegtools.global.mk

mjpegtools_MAKESPEC=auto
mjpegtools_CONFIGSPEC=exe
mjpegtools_TESTBINARY=mplex.exe

/cygdrive/c/Users/fabrn/dvda-author/depconf/mjpegtools.depconf: $(mjpegtools_DEPENDENCY)
	$(call depconf,mjpegtools)  
	 
