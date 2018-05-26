include /home/fab/Dev/dvda-author-dev/mk/ImageMagick.global.mk

ImageMagick_MAKESPEC=auto
ImageMagick_CONFIGSPEC=exe
ImageMagick_TESTBINARY=convert

/home/fab/Dev/dvda-author-dev/depconf/ImageMagick.depconf: $(ImageMagick_DEPENDENCY)
	$(call depconf,ImageMagick)
