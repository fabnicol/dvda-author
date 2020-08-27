## Process this file with automake to produce Makefile.in

SUBDIRS = util

#TODO: add changelog
msw_files = \
	gpl.txt \
	lplex.ini.lnk \
	lplex.log.lnk \
	project.lplex.lnk

if MSW_RELEASE

msw_releasedir = $(top_builddir)/release/$(PACKAGE)-$(VERSION)
msw_release_DATA = $(msw_files)

install-data-hook:
	test -z "$(msw_releasedir)/bin" || mkdir -p "$(msw_releasedir)/bin"
	test -z "$(msw_releasedir)/data" || mkdir -p "$(msw_releasedir)/data"
	cp -f $(bindir)/lplex.exe $(msw_releasedir)/
	cp -fr $(datadir)/$(PACKAGE)/* $(msw_releasedir)/data/
	@( \
	pending=$(msw_releasedir)/lplex.exe ; \
	while test "x$$pending" != "x" ; do \
		objs=$$pending ;\
		pending="" ;\
		for obj in $$objs ; do \
			dlls=$$(objdump -x $$obj | grep "DLL Name: " | \
				sed -e 's/DLL Name: //g' | sort -u) ;\
			for dll in $$dlls ; do \
				waschecked=$$(echo "$$checked" | grep -i $$dll) ;\
				if test "x$$waschecked" = "x" ; then \
					dllpath=$$(find $(prefix)/ -iname $$dll) ;\
					if test "x$$dllpath" != "x" ; then \
						echo "copying $$dll" ;\
						cp -f $$dllpath $(msw_releasedir) ;\
						pending="$$pending $$dllpath" ;\
					fi ;\
					checked="$$checked $$dll" ;\
				fi ;\
			done ;\
		done ;\
	done \
	)
	
#	@( \
#	dlls=$$(objdump -x $(msw_releasedir)/lplex.exe | \
#		grep "DLL Name: " | sed -e 's/DLL Name: //g') ;\
#	for dll in $$dlls ; do \
#		dllpath=$$(find $(prefix)/ -iname $$dll) ;\
#		if test "x$$dllpath" != "x" ; then \
#			echo "copying $$dll" ;\
#			cp -f $$dllpath $(msw_releasedir) ;\
#		fi ;\
#	done \
#	)
	
	@echo -e "\n\n"\
	"      A windows binary package has been created in 'release/$(PACKAGE)-$(VERSION)'.\n\n"\
	"      Before running lplex, don't forget to add the runtime tools\n"\
	"            mplex, jpeg2yuv, mpeg2enc, dvdauthor, mkisofs\n"\
	"      into the 'bin' directory, for instance by copying these from\n"\
	"      an official release of lplex.\n\n"

uninstall-hook:
	rm -fr $(msw_releasedir)

endif

EXTRA_DIST = $(msw_files)

