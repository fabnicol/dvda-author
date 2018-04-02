#Do not use bash. It would cause subtle issues with libtool in the flac package, probably a libtool bug.


define docfollow
	@findstring=$$(find /home/fab/Dev/dvda-author -maxdepth 1 -name $(strip $1) -print0)
	echo "[doc]" >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
	echo "    $1 $$findstring" >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE 
endef

define index
	@echo [$1] >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
	directory=$(MAYBE_$1)
	if test "$3" = "library" ; then
	   testvar=$$(find /home/fab/Dev/dvda-author/$$directory -maxdepth 4 -type f -wholename "*$1$2" -print0)
	else
	   testvar=$$(find /home/fab/Dev/dvda-author/$$directory -maxdepth 4 -type f -wholename "*$($1_TESTBINARY)$2" -print0)
	fi
	if test "$$testvar" != ""; then
		echo "     built $3: $$testvar for $1" >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
		if test "$3" = "library" ; then
		  testvar2=$$(find /home/fab/Dev/dvda-author/local/lib -wholename $($1_LIB) -print0)
		else
		  testvar2=$$(find /home/fab/Dev/dvda-author/local/bin -name "$($1_TESTBINARY)$(EXEEXT)" -print0)
		fi
		if test "$$testvar2" != ""; then
			echo "     local $3: $$testvar2 from $(MAYBE_$1)" >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
		else
			echo "     did not install $3 $1" >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
		fi
	else
	        echo "     no $3 $1" >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
	fi
endef


#the complex autotools invocation is rendered necessary by the missing/obsolete status of the dvdauthor autotools chain
#notice autoconf twice and aclocal -I... to retrieve missing macros
#Theres is an odd MKDIR_P bug with MIGW32, which is circumvented here for generality but could be taken out in later versions

define configure_sub_package
	@target_subdir=$(strip $1)
	echo Building $1...
	configure_flags="$2"
	if test "$$target_subdir" != ""; then
	   if test -d  "/home/fab/Dev/dvda-author/$$target_subdir"; then
	      mkdir -p "/home/fab/Dev/dvda-author/$$target_subdir/"
	      cd "/home/fab/Dev/dvda-author/$$target_subdir/"
	      if test "$(findstring noconfigure,$5)" = "" ; then 
       	       cd "/home/fab/Dev/dvda-author/$$target_subdir"
	       if test "$3" != "" ; then echo Running shell command...; $(SHELL) "$3" "$4"; fi
	       cd -
	       echo Running configure in /home/fab/Dev/dvda-author/$$target_subdir ...
	       $(SHELL) "/home/fab/Dev/dvda-author/$$target_subdir/configure" $$configure_flags --prefix="/home/fab/Dev/dvda-author/local" CPPFLAGS="-I/home/fab/Dev/dvda-author/local/include"  $6
	      else
	       if test "/home/fab/Dev/dvda-author" != "/home/fab/Dev/dvda-author"; then cp -rf "/home/fab/Dev/dvda-author/$$target_subdir" /home/fab/Dev/dvda-author; fi
	      fi
	      if test "$$?" = "0"; then  
	      echo -- *****
	      echo -- * Now building $1
	      echo -- * See file /home/fab/Dev/dvda-author/depconf/BUILD.TRACE --
	      echo -- *****
		  echo Now building $1 with command line $(MAKE) $(PARALLEL) MKDIR_P="\"$(MKDIR_P)\""  $(if $6,$6)...  >> /home/fab/Dev/dvda-author/depconf/BUILD.TRACE
		  $(MAKE)  $(if $6,$6) $(PARALLEL) MKDIR_P="$(MKDIR_P)" 
	      fi
	      if test "$$?" = "0" -o "$6" != ""; then
            echo Installing from $$target_subdir ...
            if test -f INSTALL; then mv -f INSTALL INSTALL.txt ; fi
            $(MAKE) INS_BASE=$(INS_BASE) $6 install
	      else 
            echo
            echo [ERR] Make failed while processing $1.
            echo
            sleep 1s
	      fi
	      if test -f INSTALL.txt; then mv -f INSTALL.txt INSTALL; fi
	      cd /home/fab/Dev/dvda-author
	   fi
	fi
endef


define configure_lib_package
	@$(call configure_sub_package,$(MAYBE_$1),$(CONFIGURE_$1_FLAGS),$2,$3,,$4)
	if test "$$?" = "0"; then
	  touch /home/fab/Dev/dvda-author/depconf/$1.depconf
	fi
	$(call index,$1,.a,library)
endef

define configure_exec_package
	@if test "$(build_os)" = "mingw32"; then
	  patchfile=$$(find patches -type f -regex .*$1-patch.* -print0)
	  echo Found mingw32 OS...patching with local patch $$patchfile
	  $(if $$patchfile, patch -p0 < $$patchfile && echo "locally patched: $1: using $$patchfile in patches/" >> PATCHED.DOWNLOADS)
	fi
	$(call configure_sub_package,$(MAYBE_$1),$(CONFIGURE_$1_FLAGS),,,$2,$3)
	if test "$$?" = "0" -o "$6" != ""; then 
        touch /home/fab/Dev/dvda-author/depconf/$1.depconf
    fi
	$(call index,$1,$(EXEEXT),binary)
endef

define clean_package
	@$(if $1,if test -d  /home/fab/Dev/dvda-author/$(MAYBE_$1); then cd /home/fab/Dev/dvda-author/$(MAYBE_$1); $(MAKE)  clean ; cd - ; fi)
endef

define depconf
	@if test "$($1_MAKESPEC)" = "auto" ; then
	  if test "$($1_CONFIGSPEC)" = "lib"; then
		$(call configure_lib_package,$1,$2,$3,$4)
	  else
	    if test "$($1_CONFIGSPEC)" = "exe"; then
		  $(call configure_exec_package,$1,$2,$3)
	    fi
	  fi
	fi
endef

define clean_directory
	for dir in $1; do
	   if test -d "$$dir" ; then
	     cd "$$dir"; $(RM) *.a *.po *.o *.1 *.html; cd /home/fab/Dev/dvda-author
	   fi
	done
endef
