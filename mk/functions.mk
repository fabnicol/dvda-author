#Do not use bash. It would cause subtle issues with libtool in the flac package, probably a libtool bug.

define trace
	echo "$1:  $2" >> BUILD.TRACE
endef

define follow
	$(if "$1", $(call trace,done,"$1 $2"), $(call trace,failed,$1))
endef

define execfollow
	$(if "$2",
	    program=$$(find $(BUILDDIR)/$1 -maxdepth 2 -type f -name $(strip $2)$(EXEEXT) -print0)
	    $(call follow,$$program,executable: `$$program $3` ),
	    $(call trace,failed,$2))
endef

define docfollow
	findstring=$$(find $(BUILDDIR) -maxdepth 1 -name $(strip $1) -print0)
	$(call follow, $1, $$findstring)
endef

#the complex autotools invocation is rendered necessary by the missing/obsolete status of the dvdauthor autotools chain
#notice autoconf twice and aclocal -I... to retrieve missing macros
#Theres is an odd MKDIR_P bug with MIGW32, which is circumvented here for generality but could be taken out in later versions

define configure_sub_package
	target_subdir="$(strip $1)"
	configure_flags="$2"
	autotools_command_line="$3"
	$(if $$target_subdir,
	   if test -d  $$target_subdir; then
	      cd $$target_subdir
	      $(if $$autotools_command_line,$(SHELL) $$autotools_command_line)
	      $(SHELL) configure $$configure_flags --prefix="$(BUILDDIR)/local" CPPFLAGS="-I$(BUILDDIR)/local/include" \
		 && $(MAKETOOL) MKDIR_P="mkdir$(EXEEXT) -p"
	      if test "$$?" = "0"; then
		 echo Installing from $$target_subdir ...
		 if test -f INSTALL; then mv -f INSTALL INSTALL.txt ; fi
		 $(MAKETOOL) $(INS_BASE) install
	      fi
	   if test -f INSTALL.txt; then mv -f INSTALL.txt INSTALL; fi
	   cd -
	   fi)
endef

define config_lib_package
	flags=$(CONFIGURE_$1_FLAGS)
	directory=$(MAYBE_$1)
	autotools_command_line=$2
	$(call configure_sub_package,$$directory,$$flags,$$autotools_command_line)
	testvar=$$(find $(BUILDDIR)/$$directory -maxdepth 2 -type f -name $(strip $1).a -print0)
	$(if $$testvar,
			echo "found library: $$testvar for $1" >> BUILD.TRACE
			testvar=$$(find $(BUILDDIR)/local/lib -maxdepth 1 -name $(strip $1).a -print0)
			$(if $$testvar,
				echo "locally installed library: $$testvar from $1" >> BUILD.TRACE,
				echo "did not install library $1" >> BUILD.TRACE),
			echo "no library $1" >> BUILD.TRACE)
endef

define config_exec_package
    flags=$(CONFIGURE_$1_FLAGS)
    directory=$(MAYBE_$1)
    ifeq $(build_os) mingw32
	patchfile=$$(find patches -type f -regex .*$1-patch.* -print0)
	echo Found mingw32 OS...patching with local patch $$patchfile
	$(if $$patchfile, patch -p0 < $$patchfile && echo "locally patched: $1, using $$patchfile in patches/" >> PATCHED.DOWNLOADS)
    endif
	$(call configure_sub_package,$$directory,$$flags)
	$(call execfollow,$$directory,$2,$3)
endef

define clean_package
	$(if $1,$(if $2, (if test -d  $2; then cd $2; $(MAKETOOL)  clean ; cd - ; fi)))

