Name: dvda-author
Summary: dvda-author creates high-definition DVD-AUDIO discs
Version: 
Release: 1
License: GPL v3
Group: devel
Source: %{name}-%{version}.tar.gz


BuildRoot: %{_tmppath}/build-root-%{name}
Packager: Fab Nicol
Distribution: linux
Prefix: /usr
Url: http://dvd-audio.sourceforge.net




%description
%{name} creates high-definition DVD-Audio discs with navigable DVD-Video zone from DVD-Audio zone
Supported input audio types: .wav, .flac, .oga, SoX-supported formats
EXAMPLES
-creates a 3-group DVD-Audio disc (legacy syntax):

    %{name} -g file1.wav file2.flac -g file3.flac -g file4.wav 

-creates a hybrid DVD disc with both AUDIO_TS mirroring audio_input_directory and VIDEO_TS imported from directory VID, outputs disc structure to directory DVD_HYBRID and links video titleset #2 of VIDEO_TS to AUDIO_TS:

    %{name} -i ~/audio/audio_input_directory -o DVD_HYBRID -V ~/Video/VID -T 2 

Both types of constructions can be combined.   

%prep
rm -rf $RPM_BUILD_ROOT 
mkdir $RPM_BUILD_ROOT

%setup -q

%build
./configure --prefix=%{prefix} --without-debug 
make -j 2

%install
make DESTDIR=$RPM_BUILD_ROOT install-strip

cd $RPM_BUILD_ROOT
rm -rf  .$RPM_BUILD_DIR/%{name}-%{version}/local
find . -type d -fprint $RPM_BUILD_DIR/file.list.%{name}.dirs
find . -type f -fprint $RPM_BUILD_DIR/file.list.%{name}.files.tmp

echo   .%{prefix}/share/man/man1/%{name}.1 > $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/bin/%{name} >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/lib/libc_utils.a >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/applications/%{name}.desktop >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/share/pixmaps/%{name}_64x64.png >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/share/pixmaps/%{name}_48x48.png >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/share/applications/%{name}.conf >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/share/doc/%{name}/BUGS >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/share/doc/%{name}/README >> $RPM_BUILD_DIR/file.list.%{name}.files
echo   .%{prefix}/share/doc/%{name}/%{name}-%{version}.html >> $RPM_BUILD_DIR/file.list.%{name}.files
sed '1,2d;s,^\.,\%attr(-\,root\,root) \%dir ,' $RPM_BUILD_DIR/file.list.%{name}.dirs > $RPM_BUILD_DIR/file.list.%{name}
sed 's,^\.,\%attr(-\,root\,root) ,' $RPM_BUILD_DIR/file.list.%{name}.files >> $RPM_BUILD_DIR/file.list.%{name}


%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/file.list.%{name}.files
rm -rf $RPM_BUILD_DIR/file.list.%{name}.files.tmp
rm -rf $RPM_BUILD_DIR/file.list.%{name}.dirs


#%files -f ../file.list.%{name}
%files 
%defattr(-,root,root,0755)
%{prefix}/share/applications/*
%{prefix}/share/pixmaps/*
%{prefix}/bin/*                                                                                                                                                    
%{prefix}/share/doc/*                                                                                                                                              
%{prefix}/share/man/man?/*   

