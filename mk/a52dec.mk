a52dec_MAKESPEC=auto
a52dec_CONFIGSPEC=exe
a52dec_DEPENDENCY=Makefile
a52dec_COMMANDLINE=2>&1|line
WITH_a52dec=@WITH_a52dec@

#Autoconf-substituted, do not modify#
a52dec_LIB=
a52dec_LINK=
MAYBE_a52dec=
HAVE_a52dec=@HAVE_a52_dec@
HAVE_EXTERNAL_a52dec=@HAVE_EXTERNAL_a52dec@


#Start of autoconf-substituted, do not modify#

a52dec.depconf: $(a52dec_DEPENDENCY)
	$(call depconf,$*)

#end of autoconf-substituted
