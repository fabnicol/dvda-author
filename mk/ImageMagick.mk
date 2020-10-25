include /cygdrive/c/Users/fabrn/dvda-author/mk/ImageMagick.global.mk

ImageMagick_MAKESPEC=auto
ImageMagick_CONFIGSPEC=exe
ImageMagick_TESTBINARY=magick.exe

/cygdrive/c/Users/fabrn/dvda-author/depconf/ImageMagick.depconf: $(ImageMagick_DEPENDENCY)
	$(call depconf,ImageMagick)
