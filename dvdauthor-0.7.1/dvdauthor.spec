%define DISTRO_RH %([ -f /etc/redhat-release ] && sed -e "s/\\(.\\+ release \\)\\(.\\+\\)\\( .\\+\\)/\\2/" /etc/redhat-release)
%define DISTRO_SUSE %([ -f /etc/SuSE-release ] && grep VERSION /etc/SuSE-release| sed -e "s/VERSION = /SuSE/"|tr -d ".")
 

%define	dvdauthor_version	0.7.1-patched
%define	dvdauthor_release	1

Summary: 	Latest DVDAuthor package
Name: 		dvdauthor
Version: 	%(echo %{dvdauthor_version}|tr -s "-" "_")
Release: 	%{dvdauthor_release}.%{DISTRO_RH}%{DISTRO_SUSE}
License:        GNU GPL
Vendor:         Unknown
Group: 		Applications/Multimedia
Source0: 	http://prdownloads.sourceforge.net/dvdauthor/dvdauthor-%{dvdauthor_version}.tar.gz
BuildRoot: 	/var/tmp/%{name}-%{version}-root
Requires: 	libxml2 >= 2.6.0 libdvdread
BuildRequires:	libxml2-devel >= 2.6.0 libdvdread-devel
URL:            http://dvdauthor.sourceforge.net/

%description
dvdauthor is a program that will generate a DVD movie from a valid
mpeg2 stream that should play when you put it in a DVD player.


%prep 
%setup -n %{name}-%{dvdauthor_version}

%build
%configure
make

%install
[ "x${RPM_BUILD_ROOT}" != "x/" ] && rm -rf "${RPM_BUILD_ROOT}"
%makeinstall


%clean
[ "x${RPM_BUILD_ROOT}" != "x/" ] && rm -rf "${RPM_BUILD_ROOT}"


%pre


%post


%preun


%files
%defattr(-,root,root)
%doc {COPYING,ChangeLog,INSTALL,README,TODO}
%{_bindir}/*
%{_mandir}/man1/*
%{_datadir}/dvdauthor/*

%changelog
* Fri Feb 21 2003 Scott Smith 0.4.4
- adapted into source tree
* Fri Feb 21 2003 Dr. Peter Bieringer <pb at bieringer dot de> 0.4.3
- initial
