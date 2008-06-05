# Authority: SounOS.org

Summary: Server Base Library for TCP/UDP communication
Name: libsbase
Version: 0.3.0
Release: 1%{?dist}
License: BSD
Group: System Environment/Libraries
URL: http://code.google.com/p/sbase/

Source: http://code.google.com/p/sbase/download/%{name}-%{version}.tar.gz
Packager: SounOS <SounOS@gmail.com>
Vendor: SounOS
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: gcc libevbase >= 0.0.14
Requires: glibc >= 2.3.4 libevbase >= 0.0.14

%description
Server Base Library for TCP/UDP communication

%prep
%setup

%build
%configure --enable-debug
%{__make}

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR="%{buildroot}"
perl -i -p -e "s@/usr/local@%{_prefix}@g" doc/rc.lechod
perl -i -p -e "s@%{_prefix}/etc@%{_sysconfdir}@g" doc/rc.lechod
mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/init.d
install -c -m755  doc/rc.lechod %{buildroot}/%{_sysconfdir}/rc.d/init.d/lechod
install -c -m644 doc/rc.lechod.ini %{buildroot}/%{_sysconfdir}/lechod.ini

%clean
%{__rm} -rf %{buildroot}

%post 
	/sbin/ldconfig
	/sbin/chkconfig --add lechod

%postun 
	if [ $1 = 0 ]; then 
		/sbin/service lechod stop > /dev/null 2>&1
		/sbin/chkconfig --del lechod
	fi
	/sbin/ldconfig

%files
%defattr(-, root, root, 0755)
%{_includedir}/*
%{_bindir}/*
%{_libdir}/*
%{_sysconfdir}/*

%changelog
* Wed Jun 27 2007 14:19:04 CST SounOS <SounOS@gmail.com>
- Fixed terminate_session result program crash bug , replaced it with  MESSAGE_QUIT

* Wed Jun 20 2007 10:00:53 CST SounOS <SounOS@gmail.com>
- Updated source code version to 0.1.6
- Fixed recv OOB and Terminate Connection bug
- Update thread->clean service->clean session->clean
- Fixed ev_init() evbase_init() to clean memory leak bug
