Name: rsplib
Version: 3.2.0~rc1.4
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
BuildRequires: qt5-qtbase-devel
BuildRequires: qtchooser
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%description
RSerPool client/server API library for session management Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.

%define _unpackaged_files_terminate_build 0


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
/usr/lib/librspcsp.so.*
/usr/lib/librspdispatcher.so.*
/usr/lib/librsphsmgt.so.*
/usr/lib/librsplib.so.*
/usr/lib/librspmessaging.so.*
/usr/lib/libtdbreakdetector.so.*
/usr/lib/libtdloglevel.so.*
/usr/lib/libtdnetutilities.so.*
/usr/lib/libtdrandomizer.so.*
/usr/lib/libtdstorage.so.*
/usr/lib/libtdstringutilities.so.*
/usr/lib/libtdtagitem.so.*
/usr/lib/libtdthreadsafety.so.*
/usr/lib/libtdtimeutilities.so.*
/usr/lib/libcpprspserver.so.*
/usr/lib/libtdcppthread.so.*

%files devel
/usr/include/rserpool/rserpool-internals.h
/usr/include/rserpool/rserpool-policytypes.h
/usr/include/rserpool/rserpool.h
/usr/include/rserpool/rserpool-csp.h
/usr/include/rserpool/tagitem.h
/usr/include/rserpool/cpprspserver.h
/usr/include/rserpool/mutex.h
/usr/include/rserpool/tcplikeserver.h
/usr/include/rserpool/thread.h
/usr/include/rserpool/udplikeserver.h
/usr/lib/librspcsp*.a
/usr/lib/librspcsp*.so
/usr/lib/librspdispatcher*.a
/usr/lib/librspdispatcher*.so
/usr/lib/librsphsmgt*.a
/usr/lib/librsphsmgt*.so
/usr/lib/librsplib*.a
/usr/lib/librsplib*.so
/usr/lib/librspmessaging*.a
/usr/lib/librspmessaging*.so
/usr/lib/libtdbreakdetector*.a
/usr/lib/libtdbreakdetector*.so
/usr/lib/libtdloglevel*.a
/usr/lib/libtdloglevel*.so
/usr/lib/libtdnetutilities*.a
/usr/lib/libtdnetutilities*.so
/usr/lib/libtdrandomizer*.a
/usr/lib/libtdrandomizer*.so
/usr/lib/libtdstorage*.a
/usr/lib/libtdstorage*.so
/usr/lib/libtdstringutilities*.a
/usr/lib/libtdstringutilities*.so
/usr/lib/libtdtagitem*.a
/usr/lib/libtdtagitem*.so
/usr/lib/libtdthreadsafety*.a
/usr/lib/libtdthreadsafety*.so
/usr/lib/libtdtimeutilities*.a
/usr/lib/libtdtimeutilities*.so
/usr/lib/libcpprspserver.so.*
/usr/lib/libtdcppthread.so.*
/usr/lib/libcpprspserver*.a
/usr/lib/libcpprspserver*.so
/usr/lib/libtdcppthread*.a
/usr/lib/libtdcppthread*.so

%files docs
%doc docs/Handbook.pdf

%files registrar
/usr/bin/rspregistrar
/usr/share/man/man1/rspregistrar.1.gz

%files tools
/usr/bin/cspmonitor
/usr/bin/hsdump
/usr/bin/rspserver
/usr/bin/rspterminal
/usr/share/man/man1/rspserver.1.gz
/usr/share/man/man1/rspterminal.1.gz
/usr/share/man/man1/cspmonitor.1.gz
/usr/share/man/man1/hsdump.1.gz

%files services
/usr/bin/calcappclient
/usr/bin/fractalpooluser
/usr/bin/pingpongclient
/usr/bin/scriptingclient
/usr/bin/scriptingcontrol
/usr/bin/scriptingserviceexample
/usr/share/man/man1/calcappclient.1.gz
/usr/share/man/man1/fractalpooluser.1.gz
/usr/share/man/man1/pingpongclient.1.gz
/usr/share/man/man1/scriptingclient.1.gz
/usr/share/man/man1/scriptingcontrol.1.gz
/usr/share/man/man1/scriptingserviceexample.1.gz
/usr/share/fractalpooluser/*.qm
/usr/share/fgpconfig/*.fsf


%changelog
* Tue Nov 21 2017 Thomas Dreibholz <dreibh@simula.no> 3.1.8
- Initial RPM release
