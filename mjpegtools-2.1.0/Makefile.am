# Process with automake to produce Makefile.in

# NOTE:  utils and mpeg2enc MUST be built first so that libmjpegutils AND
#        libmpeg2encpp exist.  Then in the following Makefile.am files if
#        HAVE_ALTIVEC is defined libmpeg2encpp will be added to the list of
#        libraries dependencies.  All this because utils/altivec/libaltivec.la
#        contains routines which  which should have gone into the encoder
#        library libmpeg2encpp.  Sigh.  Feel free to do it differently if you
#        can come up with a better method.

SUBDIRS = \
	utils \
	mpeg2enc \
	lavtools \
	aenc \
	mplex \
	scripts \
	docs \
	yuvcorrect \
	yuvscaler \
	yuvdenoise \
	yuvfilters \
	yuvdeinterlace \
	y4mdenoise \
	y4mscaler \
	y4munsharp \
        y4mutils \
	debian

#
# Add any non autoconf'd files here, extra readmes and other misc 
# info to be copied into the dist
#
EXTRA_DIST = \
	BUGS \
	CHANGES \
	HINTS \
	PLANS \
	README.AltiVec \
	README.DV \
	README.avilib \
	README.glav \
	README.lavpipe \
	README.transist \
	INSTALL \
	INSTALL.real \
	TODO \
	cpuinfo.sh \
	mjpeg_howto.txt \
	mjpegtools.spec \
	mjpegtools.spec.in \
	mjpegtools.pc \
	mjpegtools.pc.in \
	autogen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mjpegtools.pc

DISTCLEANFILES = \
	mjpegtools-config.h \
	mjpegtools.pc \
	confdefs.h \
	config.cache \
	config.status \
	config.log

MAINTAINERCLEANFILES = \
	compile \
	depcomp \
	install-sh \
	missing \
	mkinstalldirs \
	Makefile.in \
	aclocal.m4 \
	config.guess \
	config.h.in \
	config.sub \
	configure \
	ltmain.sh \
	stamp-h.in

## make rpms
rpm: Makefile
	$(MAKE) dist
	rpmbuild -ta --clean $(PACKAGE)-$(VERSION).tar.gz

## make debs
deb: Makefile dist
	-chmod -R +w $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)
	tar xzf $(PACKAGE)-$(VERSION).tar.gz
	cd $(PACKAGE)-$(VERSION); dpkg-buildpackage -rfakeroot
