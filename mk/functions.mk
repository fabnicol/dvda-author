#Do not use bash. It would cause subtle issues with libtool in the flac package, probably a libtool bug.


define docfollow
	@findstring=$$(find /mnt/fab/Dev/dvd-audio-dev -maxdepth 1 -name $(strip $1) -print0)
	echo "[doc]" >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
	echo "    $1 $$findstring" >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE 
endef

define index
	@echo [$1] >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
	directory=$(MAYBE_$1)
	if test "$3" = "library" ; then
	   testvar=$$(find /mnt/fab/Dev/dvd-audio-dev/$$directory -maxdepth 4 -type f -wholename "*$1$2" -print0)
	else
	   testvar=$$(find /mnt/fab/Dev/dvd-audio-dev/$$directory -maxdepth 4 -type f -wholename "*$($1_TESTBINARY)$2" -print0)
	fi
	if test "$$testvar" != ""; then
		echo "     built $3: $$testvar for $1" >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
		if test "$3" = "library" ; then
		  testvar2=$$(find /mnt/fab/Dev/dvd-audio-dev/local/lib -wholename $($1_LIB) -print0)
		else
		  testvar2=$$(find /mnt/fab/Dev/dvd-audio-dev/local/bin -name "$($1_TESTBINARY)$(EXEEXT)" -print0)
		fi
		if test "$$testvar2" != ""; then
			echo "     local $3: $$testvar2 from $(MAYBE_$1)" >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
		else
			echo "     did not install $3 $1" >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
		fi
	else
	        echo "     no $3 $1" >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
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
	   if test -d  "/mnt/fab/Dev/dvd-audio-dev/$$target_subdir"; then
	      mkdir -p "/mnt/fab/Dev/dvd-audio-dev/$$target_subdir/"
	      cd "/mnt/fab/Dev/dvd-audio-dev/$$target_subdir/"
	      if test "$(findstring noconfigure,$5)" = "" ; then 
       	       cd "/mnt/fab/Dev/dvd-audio-dev/$$target_subdir"
	       if test "$3" != "" ; then echo Running shell command...; $(SHELL) "$3" "$4"; fi
	       cd -
	       echo Running configure in /mnt/fab/Dev/dvd-audio-dev/$$target_subdir ...
	       $(SHELL) "/mnt/fab/Dev/dvd-audio-dev/$$target_subdir/configure" $$configure_flags --prefix="/mnt/fab/Dev/dvd-audio-dev/local" CPPFLAGS="-I/mnt/fab/Dev/dvd-audio-dev/local/include"  $6
	      else
	       if test "/mnt/fab/Dev/dvd-audio-dev" != "/mnt/fab/Dev/dvd-audio-dev"; then cp -rf "/mnt/fab/Dev/dvd-audio-dev/$$target_subdir" /mnt/fab/Dev/dvd-audio-dev; fi
	      fi
	      if test "$$?" = "0"; then  
	      echo -- *****
	      echo -- * Now building $1
	      echo -- * See file /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE --
	      echo -- *****
		  echo Now building $1 with command line $(MAKE) $(PARALLEL) MKDIR_P="\"$(MKDIR_P)\""  $(if $6,$6)...  >> /mnt/fab/Dev/dvd-audio-dev/depconf/BUILD.TRACE
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
	      cd /mnt/fab/Dev/dvd-audio-dev
	   fi
	fi
endef


define configure_lib_package
	@$(call configure_sub_package,$(MAYBE_$1),$(CONFIGURE_$1_FLAGS),$2,$3,,$4)
	if test "$$?" = "0"; then
	  touch /mnt/fab/Dev/dvd-audio-dev/depconf/$1.depconf
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
        touch /mnt/fab/Dev/dvd-audio-dev/depconf/$1.depconf
    fi
	$(call index,$1,$(EXEEXT),binary)
endef

define clean_package
	@$(if $1,if test -d  /mnt/fab/Dev/dvd-audio-dev/$(MAYBE_$1); then cd /mnt/fab/Dev/dvd-audio-dev/$(MAYBE_$1); $(MAKE)  clean ; cd - ; fi)
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
	     cd "$$dir"; $(RM) *.a *.po *.o *.1 *.html; cd /mnt/fab/Dev/dvd-audio-dev
	   fi
	done
endef
