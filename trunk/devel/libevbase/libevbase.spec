# Authority: SounOS.org

Summary: Event Base sets with epoll/kqueue/select
Name: libevbase
Version: 0.0.18
Release: 1%{?dist}
License: BSD
Group: System Environment/Libraries
URL: http://code.google.com/p/sbase/

Source: http://code.google.com/p/sbase/download/%{name}-%{version}.tar.gz
Packager: SounOS <SounOS@gmail.com>
Vendor: SounOS
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root


%description
Server Base Library for TCP/UDP communication event base

%prep
%setup

%build
%configure
%{__make}

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR="%{buildroot}"

%clean
%{__rm} -rf %{buildroot}

%post 
	/sbin/ldconfig

%postun 
	/sbin/ldconfig

%files
%defattr(-, root, root, 0755)
%{_includedir}/*
%{_bindir}/*
%{_libdir}/*

%changelog
* Wed Jun 27 2007 14:19:04 CST SounOS <SounOS@gmail.com>
- Fixed terminate_session result program crash bug , replaced it with  MESSAGE_QUIT

* Wed Jun 20 2007 10:00:53 CST SounOS <SounOS@gmail.com>
- Updated source code version to 0.1.6
- Fixed recv OOB and Terminate Connection bug
- Update thread->clean service->clean session->clean
- Fixed ev_init() evbase_init() to clean memory leak bug
