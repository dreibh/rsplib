Name: rsplib
Version: 3.2.0~rc1.1
Release: 1
Summary: Reliable Server Pooling (RSerPool) implementation
License: GPL-3.0
Group: Applications/Internet
URL: https://www.uni-due.de/~be0001/rserpool/
Source: https://www.uni-due.de/~be0001/rserpool/download/%{name}-%{version}.tar.gz

AutoReqProv: on
BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: lksctp-tools-devel
BuildRequires: bzip2-devel
BuildRequires: glib2-devel
BuildRequires: qt5-qtbase-devel
BuildRequires: qtchooser
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%description
RSerPool client/server API library for session management Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.



%package devel
Summary: Development files for rsplib
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.
This package provides header files for the rsplib library. You need them to develop your own RSerPool-based clients and servers.


%package docs
Summary: Documentation files for rsplib
Group: System Environment/Libraries
Requires: %{name} = %{version}-%{release}

%description docs
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.
This package contains the documentation for the RSerPool implementation rsplib.


%package registrar
Summary: RSerPool Registrar service
Group: Applications/Internet
Requires: %{name} = %{version}-%{release}
Requires: %{name}-docs

%description registrar
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.
This package provides the registrar, which is the management component for RSerPool-based server pools. You need at least one registrar in a setup, but for redundancy reasons, you should have at least two.


%package tools
Summary: RSerPool test tools
Group: Applications/Internet
Requires: %{name} = %{version}-%{release}
Requires: %{name}-docs
Requires: chrpath

%description tools
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.
This package provides some test tools for RSerPool setups.


%package services
Summary: RSerPool example services
Group: Applications/Internet
Requires: %{name} = %{version}-%{release}
Requires: %{name}-tools

%description services
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.
This package provides a set of input files for the Fractal Generator service.


%prep
%setup -q

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr -DUSE_KERNEL_SCTP=1 -DENABLE_CSP=1 -DENABLE_QT=1 .
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install


%files
%defattr(-,root,root,-)
%{_libdir}/librspcsp.so*
%{_libdir}/librspdispatcher.so*
%{_libdir}/librsphsmgt.so*
%{_libdir}/librsplib.so*
%{_libdir}/librspmessaging.so*
%{_libdir}/libtdbreakdetector.so*
%{_libdir}/libtdloglevel.so*
%{_libdir}/libtdnetutilities.so*
%{_libdir}/libtdrandomizer.so*
%{_libdir}/libtdstorage.so*
%{_libdir}/libtdstringutilities.so*
%{_libdir}/libtdtagitem.so*
%{_libdir}/libtdthreadsafety.so*
%{_libdir}/libtdtimeutilities.so*
%{_libdir}/libcpprspserver.so*
%{_libdir}/libtdcppthread.so*

%files devel
%{_includedir}/rserpool/rserpool-internals.h
%{_includedir}/rserpool/rserpool-policytypes.h
%{_includedir}/rserpool/rserpool.h
%{_includedir}/rserpool/rserpool-csp.h
%{_includedir}/rserpool/tagitem.h
%{_includedir}/rserpool/cpprspserver.h
%{_includedir}/rserpool/mutex.h
%{_includedir}/rserpool/tcplikeserver.h
%{_includedir}/rserpool/thread.h
%{_includedir}/rserpool/udplikeserver.h
%{_libdir}/librspcsp*.a
%{_libdir}/librspcsp*.so
%{_libdir}/librspdispatcher*.a
%{_libdir}/librspdispatcher*.so
%{_libdir}/librsphsmgt*.a
%{_libdir}/librsphsmgt*.so
%{_libdir}/librsplib*.a
%{_libdir}/librsplib*.so
%{_libdir}/librspmessaging*.a
%{_libdir}/librspmessaging*.so
%{_libdir}/libtdbreakdetector*.a
%{_libdir}/libtdbreakdetector*.so
%{_libdir}/libtdloglevel*.a
%{_libdir}/libtdloglevel*.so
%{_libdir}/libtdnetutilities*.a
%{_libdir}/libtdnetutilities*.so
%{_libdir}/libtdrandomizer*.a
%{_libdir}/libtdrandomizer*.so
%{_libdir}/libtdstorage*.a
%{_libdir}/libtdstorage*.so
%{_libdir}/libtdstringutilities*.a
%{_libdir}/libtdstringutilities*.so
%{_libdir}/libtdtagitem*.a
%{_libdir}/libtdtagitem*.so
%{_libdir}/libtdthreadsafety*.a
%{_libdir}/libtdthreadsafety*.so
%{_libdir}/libtdtimeutilities*.a
%{_libdir}/libtdtimeutilities*.so
%{_libdir}/libcpprspserver.so.*
%{_libdir}/libtdcppthread.so.*
%{_libdir}/libcpprspserver*.a
%{_libdir}/libcpprspserver*.so
%{_libdir}/libtdcppthread*.a
%{_libdir}/libtdcppthread*.so

%files docs
%doc docs/Handbook.pdf

%files registrar
%{_bindir}/rspregistrar
%{_datadir}/man/man1/rspregistrar.1.gz

%files tools
%{_bindir}/cspmonitor
%{_bindir}/hsdump
%{_bindir}/rspserver
%{_bindir}/rspterminal
%{_datadir}/man/man1/rspserver.1.gz
%{_datadir}/man/man1/rspterminal.1.gz
%{_datadir}/man/man1/cspmonitor.1.gz
%{_datadir}/man/man1/hsdump.1.gz

%files services
%{_bindir}/calcappclient
%{_bindir}/fractalpooluser
%{_bindir}/pingpongclient
%{_bindir}/scriptingclient
%{_bindir}/scriptingcontrol
%{_bindir}/scriptingserviceexample
%{_datadir}/man/man1/calcappclient.1.gz
%{_datadir}/man/man1/fractalpooluser.1.gz
%{_datadir}/man/man1/pingpongclient.1.gz
%{_datadir}/man/man1/scriptingclient.1.gz
%{_datadir}/man/man1/scriptingcontrol.1.gz
%{_datadir}/man/man1/scriptingserviceexample.1.gz
%{_datadir}/fractalpooluser/*.qm
%{_datadir}/fgpconfig/*.fsf


%changelog
* Tue Nov 21 2017 Thomas Dreibholz <dreibh@simula.no> 3.1.8
- Initial RPM release
