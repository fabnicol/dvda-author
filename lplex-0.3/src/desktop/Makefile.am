## Process this file with automake to produce Makefile.in

if HAVE_FREEDESKTOP

lplex_appdatadir = $(datadir)/applications
lplex_appdata_DATA = lplex.desktop

lplex_pixmapsdir = $(datadir)/pixmaps
lplex_pixmaps_DATA = lplex.svg

desktop=$(top_srcdir)/src/desktop
Desktop=$(shell xdg-user-dir DESKTOP)

install-data-hook:
	test -e "$(HOME)/.lplex" || mkdir -p "$(HOME)/.lplex"
	chown --reference="$(HOME)" "$(HOME)/.lplex"
	cp -f "$(desktop)/lplex.desktop" "$(HOME)/.lplex/"
	chmod 755 "$(HOME)/.lplex/lplex.desktop"
	chown --reference="$(HOME)" "$(HOME)/.lplex/lplex.desktop"
	cp -f "$(HOME)/.lplex/lplex.desktop" "$(Desktop)/lplex.desktop"
	chmod 755 "$(Desktop)/lplex.desktop"
	chown --reference="$(HOME)" "$(Desktop)/lplex.desktop"
#   (running xdg-utils as root seems to be problematic)
#   desktop-file-install
#   xdg-desktop-icon install $(desktop)/lplex.desktop --novendor
	xdg-mime install $(desktop)/lplex-mime.xml
#   xdg-mime default lplex.desktop application/x-lplexscript

uninstall-hook:
	test -e "$(Desktop)/lplex.desktop" && rm -f "$(Desktop)/lplex.desktop"
	xdg-mime uninstall $(desktop)/lplex-mime.xml
#   xdg-desktop-icon uninstall $(desktop)/lplex.desktop --novendor

endif

EXTRA_DIST = \
	lplex.desktop \
	lplex.svg \
	lplex-mime.xml \
	lplex.rc \
	lplex.ico

