#Do not use bash. It would cause subtle issues with libtool in the flac package, probably a libtool bug.


define docfollow
	@findstring=$$(find /cygdrive/c/Users/fabrn/dvda-author -maxdepth 1 -name $(strip $1) -print0)
	@echo "[doc]" >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
	@echo "    $1 $$findstring" >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE 
endef

define index
	@echo [$1] >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
	directory=$(MAYBE_$1)
	@if test "$3" = "library" ; then
	   @testvar=$$(find  /cygdrive/c/Users/fabrn/dvda-author/$$directory -maxdepth 4 -type f  -wholename "*$1$2" -print0)
	else
	   @testvar=$$(find  /cygdrive/c/Users/fabrn/dvda-author/$$directory -maxdepth 4 -type f  -wholename "*$($1_TESTBINARY)$2" -print0)
	fi
	@if test "$$testvar" != ""; then
		@echo "     built $3: $$testvar for $1" >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
		@if test "$3" = "library" ; then
		  @testvar2=$$(find /cygdrive/c/Users/fabrn/dvda-author/local/lib -wholename $($1_LIB) -print0)
		else
		  @testvar2=$$(find /cygdrive/c/Users/fabrn/dvda-author/local/bin -name "$($1_TESTBINARY)$(EXEEXT)" -print0)
		fi
		@if test "$$testvar2" != ""; then
			@echo "     local $3: $$testvar2 from $(MAYBE_$1)" >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
		else
			@echo "     did not install $3 $1" >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
		fi
	else
	        @echo "     no $3 $1" >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
	fi
endef


#the complex autotools invocation is rendered necessary by the missing/obsolete status of the dvdauthor autotools chain
#notice autoconf twice and aclocal -I... to retrieve missing macros
#Theres is an odd MKDIR_P bug with MIGW32, which is circumvented here for generality but could be taken out in later versions

define configure_sub_package
	@target_subdir=$(strip $1)
	echo Building $1...
	configure_flags="$2"
	@if test "$$target_subdir" != ""; then
	   @if test -d  "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir"; then
	      mkdir -p "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir/"
	      cd "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir/"
	      @if test "$(findstring noconfigure,$5)" = "" ; then 
       	       cd "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir"
	       if test "$3" != "" ; then echo Running shell command...; $(SHELL) "$3" "$4"; fi
	       cd -
	       echo Running configure in /cygdrive/c/Users/fabrn/dvda-author/$$target_subdir ...
	       export CPPFLAGS="-I/cygdrive/c/Users/fabrn/dvda-author/local/include" 
		   export has_m4="."
		   if test -d "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir/m4"; then has_m4=m4; fi
		   if test ! -f "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir/configure"; then autoreconf -I$$has_m4 -if; fi
	       $(SHELL) "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir/configure" $$configure_flags --prefix="/cygdrive/c/Users/fabrn/dvda-author/local" $6
	      else
	       if test "/cygdrive/c/Users/fabrn/dvda-author" != "/cygdrive/c/Users/fabrn/dvda-author"; then cp -rf "/cygdrive/c/Users/fabrn/dvda-author/$$target_subdir" /cygdrive/c/Users/fabrn/dvda-author; fi
	      fi
	      if test "$$?" = "0"; then  
	      echo -- *****
	      echo -- * Now building $1
	      echo -- * See file /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE --
	      echo -- *****
		  echo Now building $1 with command line $(MAKE) $(PARALLEL) MKDIR_P="\"$(MKDIR_P)\""  $(if $6,$6)...  >> /cygdrive/c/Users/fabrn/dvda-author/depconf/BUILD.TRACE
		  $(MAKE)  $(if $6,$6) $(PARALLEL) MKDIR_P="$(MKDIR_P)" 
	      fi
	      if test "$$?" = "0" -o "$6" != ""; then
            echo Installing from $$target_subdir ...
            @if test -f INSTALL; then mv -f INSTALL INSTALL.txt ; fi
            $(MAKE) INS_BASE=$(INS_BASE) $6 install
	      else 
            echo
            echo [ERR] Make failed while processing $1.
            echo
            sleep 1s
	      fi
	      if test -f INSTALL.txt; then mv -f INSTALL.txt INSTALL; fi
	      cd /cygdrive/c/Users/fabrn/dvda-author
	   fi
	fi
endef


define configure_lib_package
	@$(call configure_sub_package,$(MAYBE_$1),$(CONFIGURE_$1_FLAGS),$2,$3,,$4)
	if test "$$?" = "0"; then
	  touch /cygdrive/c/Users/fabrn/dvda-author/depconf/$1.depconf
	fi
	@$(call index,$1,.a,library)
endef

define configure_exec_package
	$(call configure_sub_package,$(MAYBE_$1),$(CONFIGURE_$1_FLAGS),,,$2,$3)
	if test "$$?" = "0" -o "$6" != ""; then 
        touch /cygdrive/c/Users/fabrn/dvda-author/depconf/$1.depconf
    fi
	@$(call index,$1,$(EXEEXT),binary)
endef


define configure_script_package
	$(call configure_sub_package,$(MAYBE_$1),$(CONFIGURE_$1_FLAGS),,,$2,$3)
	if test "$$?" = "0" -o "$6" != ""; then 
        touch /cygdrive/c/Users/fabrn/dvda-author/depconf/$1.depconf
    fi
	@$(call index,$1,"",script)
endef

define clean_package
	@$(if $1,if test -d  /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_$1); then cd /cygdrive/c/Users/fabrn/dvda-author/$(MAYBE_$1); $(MAKE)  clean ; cd - ; fi)
endef

define depconf
	@if test "$($1_MAKESPEC)" = "auto" ; then
	  if test "$($1_CONFIGSPEC)" = "lib"; then
		@$(call configure_lib_package,$1,$2,$3,$4)
	  else
	    if test "$($1_CONFIGSPEC)" = "exe"; then
		  @$(call configure_exec_package,$1,$2,$3)
	    else
		 if test "$($1_CONFIGSPEC)" = "script"; then
		  @$(call configure_script_package,$1,$2,$3)
         fi
		fi
	  fi
	fi
endef

define clean_directory
	for dir in $1; do
	   if test -d "$$dir" ; then
	     cd "$$dir"; $(RM) *.a *.po *.o *.1 *.html; cd /cygdrive/c/Users/fabrn/dvda-author
	   fi
	done
endef
