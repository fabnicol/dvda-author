## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS=-Im4

SUBDIRS = data redist redist/md5 patch src src/desktop doc msw data

lplexdocdir = ${prefix}/doc/lplex
lplexdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	NEWS

EXTRA_DIST = $(lplexdoc_DATA) doc/lplex.1 autogen.sh

# not 100% sure about this since it causes a double recursion, but libs don't build without it
$(top_builddir)/redist/md5/libmd5.a:
	cd $(top_builddir)/redist/md5 && $(MAKE) $(AM_MAKEFLAGS) libmd5.a
.PHONY: $(top_builddir)/redist/md5/libmd5.a

#$(top_builddir)/redist/wx/libtreelistctrl.a:
#	cd $(top_builddir)/redist/wx && $(MAKE) $(AM_MAKEFLAGS) wx/libtreelistctrl.a
#.PHONY: $(top_builddir)/redist/wx/libtreelistctrl.a


# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done


