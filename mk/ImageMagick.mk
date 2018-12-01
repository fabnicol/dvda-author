include /home/fab/Desktop/dvda-author-dev/mk/ImageMagick.global.mk

ImageMagick_MAKESPEC=auto
ImageMagick_CONFIGSPEC=exe
ImageMagick_TESTBINARY=convert

/home/fab/Desktop/dvda-author-dev/depconf/ImageMagick.depconf: $(ImageMagick_DEPENDENCY)
	$(call depconf,ImageMagick)
