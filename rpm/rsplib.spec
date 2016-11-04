Name: rsplib
Version: 3.1.2
Release: 1
Summary: Reliable Server Pooling (RSerPool) implementation
License: GPL-3.0
Group: Applications/Internet
URL: http://www.iem.uni-due.de/~dreibh/rserpool/
Source: http://www.iem.uni-due.de/~dreibh/rserpool/download/%{name}-%{version}.tar.gz

AutoReqProv: on
BuildRequires: libtool, automake, autoconf
BuildRequires: gcc-c++
BuildRequires: lksctp-tools-devel
BuildRequires: bzip2-devel
BuildRequires: glib2-devel
BuildRequires: qt-devel
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


%package legacy-wrappers
Summary: RSerPool legacy names
Group: Applications/Internet
Requires: %{name} = %{version}-%{release}
Requires: %{name}-tools

%description legacy-wrappers
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.
This package provides legacy wrappers for rsplib-2.x scripts. The programs registrar, server and terminal have been renamed to rspregistrar, rspserver and rspterminal, respectively, in order to avoid naming conflicts.


%prep
%setup -q

%build
autoreconf -if

%configure --disable-maintainer-mode --enable-kernel-sctp --enable-qt --enable-csp --prefix=/usr
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%clean
rm -rf "$RPM_BUILD_ROOT"

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
%{_libdir}/librspcsp*.la
%{_libdir}/librspcsp*.so
%{_libdir}/librspdispatcher*.a
%{_libdir}/librspdispatcher*.la
%{_libdir}/librspdispatcher*.so
%{_libdir}/librsphsmgt*.a
%{_libdir}/librsphsmgt*.la
%{_libdir}/librsphsmgt*.so
%{_libdir}/librsplib*.a
%{_libdir}/librsplib*.la
%{_libdir}/librsplib*.so
%{_libdir}/librspmessaging*.a
%{_libdir}/librspmessaging*.la
%{_libdir}/librspmessaging*.so
%{_libdir}/libtdbreakdetector*.a
%{_libdir}/libtdbreakdetector*.la
%{_libdir}/libtdbreakdetector*.so
%{_libdir}/libtdloglevel*.a
%{_libdir}/libtdloglevel*.la
%{_libdir}/libtdloglevel*.so
%{_libdir}/libtdnetutilities*.a
%{_libdir}/libtdnetutilities*.la
%{_libdir}/libtdnetutilities*.so
%{_libdir}/libtdrandomizer*.a
%{_libdir}/libtdrandomizer*.la
%{_libdir}/libtdrandomizer*.so
%{_libdir}/libtdstorage*.a
%{_libdir}/libtdstorage*.la
%{_libdir}/libtdstorage*.so
%{_libdir}/libtdstringutilities*.a
%{_libdir}/libtdstringutilities*.la
%{_libdir}/libtdstringutilities*.so
%{_libdir}/libtdtagitem*.a
%{_libdir}/libtdtagitem*.la
%{_libdir}/libtdtagitem*.so
%{_libdir}/libtdthreadsafety*.a
%{_libdir}/libtdthreadsafety*.la
%{_libdir}/libtdthreadsafety*.so
%{_libdir}/libtdtimeutilities*.a
%{_libdir}/libtdtimeutilities*.la
%{_libdir}/libtdtimeutilities*.so
%{_libdir}/libcpprspserver.so.*
%{_libdir}/libtdcppthread.so.*
%{_libdir}/libcpprspserver*.a
%{_libdir}/libcpprspserver*.la
%{_libdir}/libcpprspserver*.so
%{_libdir}/libtdcppthread*.a
%{_libdir}/libtdcppthread*.la
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

%files legacy-wrappers
%{_bindir}/registrar
%{_bindir}/server
%{_bindir}/terminal
%{_datadir}/man/man1/registrar.1.gz
%{_datadir}/man/man1/server.1.gz
%{_datadir}/man/man1/terminal.1.gz


%changelog
* Mon Nov 04 2016 Thomas Dreibholz <dreibh@simula.no> 3.0.2
- Initial RPM release
