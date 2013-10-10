#cdrecord must be compiled with special make install parameters hence should not be included in the SUBDIRS list. Building/cleaning will need prior configuration.

cdrtools_MAKESPEC=auto
cdrtools_CONFIGSPEC=exe
cdrtools_DEPENDENCY=Makefile
cdrtools_TESTBINARY=cdrecord
cdrtools_COMMANDLINE=-version 2>null


#Autoconf-substituted, do not modify#
cdrtools_LIB=@cdrtools_LIB@
cdrtools_LINK=@cdrtools_LINK@
MAYBE_cdrtools=@MAYBE_cdrtools@
HAVE_cdrtools=@HAVE_a52_dec@
HAVE_EXTERNAL_cdrtools=@HAVE_EXTERNAL_cdrtools@
