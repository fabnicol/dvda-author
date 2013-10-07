#cdrecord must be compiled with special make install parameters hence should not be included in the SUBDIRS list. Building/cleaning will need prior configuration.

cdrtools: Makefile
	$(call execfollow,$(MAYBE_@),cdrecord,-version 2>null)

