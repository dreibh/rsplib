Name: rsplib
Version: 3.0.2
Release: 12.1
Summary: Reliable Server Pooling (RSerPool) implementation
License: GPL-3.0
Group: Productivity/Networking/Other
Url: http://www.iem.uni-due.de/~dreibh/rserpool/
Source0: http://www.iem.uni-due.de/~dreibh/rserpool/download/rsplib-3.0.2.tar.gz

AutoReqProv: on
BuildRequires: gcc-c++
BuildRequires: lksctp-tools-devel
BuildRequires: bzip2-devel
BuildRequires: glib2-devel
BuildRequires: qt-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%description
RSerPool client/server API library for session management Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.

%prep
%setup -q

%build
%configure
make

%install
make install DESTDIR=%{buildroot}

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root,-)
%{_bindir}/combinesummaries
%{_datadir}/man/man1/combinesummaries.1.gz

%changelog
* Mon Nov 04 2013 Thomas Dreibholz <dreibh@simula.no> 3.0.2
- Initial RPM Release
