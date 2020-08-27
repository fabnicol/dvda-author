## Process this file with automake to produce Makefile.in

msw_utils = \
	lplex_discrete.bat \
	lplex_padded.bat \
	lplex_seamless.bat \
	mkisofs.bat \
	sysinfo.bat

if MSW_RELEASE

msw_utilsdir = $(top_builddir)/release/$(PACKAGE)-$(VERSION)/util
msw_utils_DATA = $(msw_utils)

#uninstall-hook:

endif

EXTRA_DIST = $(msw_utils)

