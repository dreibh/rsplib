Name: rsplib
Version: 3.4.3
Release: 1
Summary: Reliable Server Pooling (RSerPool) implementation
License: GPL-3+
Group: Applications/Internet
URL: https://www.uni-due.de/~be0001/rserpool/
Source: https://www.uni-due.de/~be0001/rserpool/download/%{name}-%{version}.tar.xz

AutoReqProv: on
BuildRequires: bzip2-devel
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: lksctp-tools-devel
BuildRequires: qt5-qtbase-devel
BuildRequires: qtchooser
BuildRoot: %{_tmppath}/%{name}-%{version}-build

# Meta-package rsplib: install rsplib-all => install all sub-packages!
Requires: %{name}-all


%description
RSerPool client/server API library for session management Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.

%prep
%setup -q

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr -DUSE_KERNEL_SCTP=1 -DENABLE_CSP=1 -DENABLE_QT=1 .
%cmake_build

%install
%cmake_install

%files


%package librsplib
Summary: RSerPool client/server API library for session management
Group: System Environment/Libraries

%description librsplib
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
The API library is provided by this package.

%files librsplib
%{_libdir}/librspcsp.so.*
%{_libdir}/librspdispatcher.so.*
%{_libdir}/librsphsmgt.so.*
%{_libdir}/librsplib.so.*
%{_libdir}/librspmessaging.so.*
%{_libdir}/libtdbreakdetector.so.*
%{_libdir}/libtdloglevel.so.*
%{_libdir}/libtdnetutilities.so.*
%{_libdir}/libtdrandomizer.so.*
%{_libdir}/libtdstorage.so.*
%{_libdir}/libtdstringutilities.so.*
%{_libdir}/libtdtagitem.so.*
%{_libdir}/libtdthreadsafety.so.*
%{_libdir}/libtdtimeutilities.so.*
%{_libdir}/libcpprspserver.so.*
%{_libdir}/libtdcppthread.so.*


%package librsplib-devel
Summary: headers of the RSerPool client/server API library rsplib
Group: Development/Libraries
Requires: %{name}-librsplib = %{version}-%{release}

%description librsplib-devel
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides header files for the rsplib library. You need them
to develop your own RSerPool-based clients and servers.

%files librsplib-devel
%{_includedir}/rserpool/rserpool-internals.h
%{_includedir}/rserpool/rserpool-policytypes.h
%{_includedir}/rserpool/rserpool.h
%{_includedir}/rserpool/rserpool-csp.h
%{_includedir}/rserpool/tagitem.h
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
# NOTE: These files are library-internal files, not to be packaged in the RPM:
%ghost %{_includedir}/rserpool/asapinstance.h
%ghost %{_includedir}/rserpool/asapinterthreadmessage.h
%ghost %{_includedir}/rserpool/breakdetector.h
%ghost %{_includedir}/rserpool/componentstatuspackets.h
%ghost %{_includedir}/rserpool/componentstatusreporter.h
%ghost %{_includedir}/rserpool/debug.h
%ghost %{_includedir}/rserpool/dispatcher.h
%ghost %{_includedir}/rserpool/doublelinkedringlist.h
%ghost %{_includedir}/rserpool/ext_socket.h
%ghost %{_includedir}/rserpool/fdcallback.h
%ghost %{_includedir}/rserpool/identifierbitmap.h
%ghost %{_includedir}/rserpool/interthreadmessageport.h
%ghost %{_includedir}/rserpool/leaflinkedredblacktree.h
%ghost %{_includedir}/rserpool/loglevel.h
%ghost %{_includedir}/rserpool/messagebuffer.h
%ghost %{_includedir}/rserpool/netdouble.h
%ghost %{_includedir}/rserpool/netutilities.h
%ghost %{_includedir}/rserpool/notificationqueue.h
%ghost %{_includedir}/rserpool/peerlist-template.h
%ghost %{_includedir}/rserpool/peerlist-template_impl.h
%ghost %{_includedir}/rserpool/peerlistmanagement-template.h
%ghost %{_includedir}/rserpool/peerlistmanagement-template_impl.h
%ghost %{_includedir}/rserpool/peerlistnode-template.h
%ghost %{_includedir}/rserpool/peerlistnode-template_impl.h
%ghost %{_includedir}/rserpool/poolelementnode-template.h
%ghost %{_includedir}/rserpool/poolelementnode-template_impl.h
%ghost %{_includedir}/rserpool/poolhandle.h
%ghost %{_includedir}/rserpool/poolhandlespacechecksum.h
%ghost %{_includedir}/rserpool/poolhandlespacemanagement-basics.h
%ghost %{_includedir}/rserpool/poolhandlespacemanagement-template.h
%ghost %{_includedir}/rserpool/poolhandlespacemanagement-template_impl.h
%ghost %{_includedir}/rserpool/poolhandlespacemanagement.h
%ghost %{_includedir}/rserpool/poolhandlespacenode-template.h
%ghost %{_includedir}/rserpool/poolhandlespacenode-template_impl.h
%ghost %{_includedir}/rserpool/poolnode-template.h
%ghost %{_includedir}/rserpool/poolnode-template_impl.h
%ghost %{_includedir}/rserpool/poolpolicy-template.h
%ghost %{_includedir}/rserpool/poolpolicy-template_impl.h
%ghost %{_includedir}/rserpool/poolpolicysettings.h
%ghost %{_includedir}/rserpool/pooluserlist-template.h
%ghost %{_includedir}/rserpool/pooluserlist-template_impl.h
%ghost %{_includedir}/rserpool/poolusernode-template.h
%ghost %{_includedir}/rserpool/poolusernode-template_impl.h
%ghost %{_includedir}/rserpool/randomizer.h
%ghost %{_includedir}/rserpool/redblacktree.h
%ghost %{_includedir}/rserpool/redblacktree_impl.h
%ghost %{_includedir}/rserpool/registrartable.h
%ghost %{_includedir}/rserpool/rserpoolerror.h
%ghost %{_includedir}/rserpool/rserpoolmessage.h
%ghost %{_includedir}/rserpool/rserpoolmessagecreator.h
%ghost %{_includedir}/rserpool/rserpoolmessageparser.h
%ghost %{_includedir}/rserpool/rserpoolsocket.h
%ghost %{_includedir}/rserpool/session.h
%ghost %{_includedir}/rserpool/sessioncontrol.h
%ghost %{_includedir}/rserpool/sessionstorage.h
%ghost %{_includedir}/rserpool/simpleredblacktree.h
%ghost %{_includedir}/rserpool/sockaddrunion.h
%ghost %{_includedir}/rserpool/stringutilities.h
%ghost %{_includedir}/rserpool/tdtypes.h
%ghost %{_includedir}/rserpool/threadsafety.h
%ghost %{_includedir}/rserpool/threadsignal.h
%ghost %{_includedir}/rserpool/timer.h
%ghost %{_includedir}/rserpool/timestamphashtable.h
%ghost %{_includedir}/rserpool/timeutilities.h
%ghost %{_includedir}/rserpool/transportaddressblock.h


%package libcpprspserver
Summary: C++ RSerPool client/server API library
Group: System Environment/Libraries
Requires: %{name}-librsplib = %{version}-%{release}

%description libcpprspserver
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides an object-oriented API for the rsplib library.

%files libcpprspserver
%{_libdir}/libcpprspserver.so.*
%{_libdir}/libtdcppthread.so.*

%package libcpprspserver-devel
Summary: Headers of the C++ RSerPool client/server API library
Group: Development/Libraries
Requires: %{name}-libcpprspserver = %{version}-%{release}
Requires: %{name}-librsplib-devel = %{version}-%{release}

%description libcpprspserver-devel
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides the header files for the rsplib C++ API. You need them to
develop your own RSerPool-based clients and servers based on the C++ API.

%files libcpprspserver-devel
%{_includedir}/rserpool/cpprspserver.h
%{_includedir}/rserpool/mutex.h
%{_includedir}/rserpool/tcplikeserver.h
%{_includedir}/rserpool/thread.h
%{_includedir}/rserpool/udplikeserver.h
%{_libdir}/libcpprspserver*.a
%{_libdir}/libcpprspserver*.so
%{_libdir}/libtdcppthread*.a
%{_libdir}/libtdcppthread*.so


%package docs
Summary: Documentation files for RSPLIB
Group: System Environment/Libraries
BuildArch: noarch
Requires: %{name}-tools = %{version}-%{release}

%description docs
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package contains the documentation for the RSerPool implementation
RSPLIB.

%files docs
%{_datadir}/doc/rsplib/Handbook.pdf


%package registrar
Summary: RSerPool Registrar service
Group: Applications/Internet
Requires: %{name}-librsplib = %{version}-%{release}
Recommends: %{name}-docs = %{version}-%{release}

%description registrar
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides the registrar, which is the management component
for RSerPool-based server pools. You need at least one registrar in a
setup, but for redundancy reasons, you should have at least two.

%files registrar
%{_bindir}/rspregistrar
%{_mandir}/man1/rspregistrar.1.gz


%package tools
Summary: RSerPool test tools
Group: Applications/Internet
Requires: %{name}-librsplib = %{version}-%{release}
Recommends: %{name}-docs = %{version}-%{release}
Recommends: %{name}-fgp-cfgfiles = %{version}-%{release}
Recommends: chrpath

%description tools
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides some test tools for RSerPool setups.

%files tools
%{_bindir}/cspmonitor
%{_bindir}/hsdump
%{_bindir}/rspserver
%{_bindir}/rspterminal
%{_mandir}/man1/rspserver.1.gz
%{_mandir}/man1/rspterminal.1.gz
%{_mandir}/man1/cspmonitor.1.gz
%{_mandir}/man1/hsdump.1.gz


%package services
Summary: RSerPool example services
Group: Applications/Internet
Requires: %{name}-libcpprspserver = %{version}-%{release}
Requires: %{name}-librsplib = %{version}-%{release}
Requires: %{name}-tools = %{version}-%{release}
Recommends: %{name}-docs = %{version}-%{release}

%description services
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides the rsplib RSerPool example services:
Echo, Discard, Daytime, CharGen, CalcApp, FractalGenerator and
ScriptingService.

%files services
%{_bindir}/calcappclient
%{_bindir}/fractalpooluser
%{_bindir}/pingpongclient
%{_bindir}/scriptingclient
%{_bindir}/scriptingcontrol
%{_bindir}/scriptingserviceexample
%{_mandir}/man1/calcappclient.1.gz
%{_mandir}/man1/fractalpooluser.1.gz
%{_mandir}/man1/pingpongclient.1.gz
%{_mandir}/man1/scriptingclient.1.gz
%{_mandir}/man1/scriptingcontrol.1.gz
%{_mandir}/man1/scriptingserviceexample.1.gz
%{_datadir}/fractalpooluser/*.qm


%package fgp-cfgfiles
Summary: RSerPool Fractal Generator Service example input files
Group: Applications/Internet
BuildArch: noarch
Recommends: fractgen
Recommends: %{name}-tools = %{version}-%{release}

%description fgp-cfgfiles
Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to
RFC 5356) for a lightweight server pool and session management framework.
It provides highly available pool management (that is registration
handling and load distribution/balancing) by components called Registrar
and a client-side/server-side API for accessing the service of a pool.
This package provides a set of input files for the Fractal Generator
service.

%files fgp-cfgfiles
%{_datadir}/fgpconfig/*.fsf


%package all
Summary: RSerPool implementation RSPLIB
Group: Applications/Internet
BuildArch: noarch
Obsoletes: %{name} < %{version}
Provides:  %{name} = %{version}
Requires: %{name}-docs = %{version}-%{release}
Requires: %{name}-fgp-cfgfiles = %{version}-%{release}
Requires: %{name}-libcpprspserver = %{version}-%{release}
Requires: %{name}-libcpprspserver-devel = %{version}-%{release}
Requires: %{name}-librsplib = %{version}-%{release}
Requires: %{name}-librsplib-devel = %{version}-%{release}
Requires: %{name}-registrar = %{version}-%{release}
Requires: %{name}-services = %{version}-%{release}
Requires: %{name}-tools = %{version}-%{release}

%description all
This is the installation metapackage for the RSerPool implementation RSPLIB.
It installs all RSPLIB components.

%files all


%changelog
* Sun Sep 11 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.4.3
- New upstream release.
* Mon Aug 22 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.4.2
- New upstream release.
* Mon Jan 10 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.4.1
- New upstream release.
* Mon Jan 03 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.4.0
- New upstream release.
* Mon Nov 08 2021 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.3.3
- New upstream release.
* Wed Nov 03 2021 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.3.3
- New upstream release.
* Mon Oct 25 2021 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.3.2
- New upstream release.
* Mon May 03 2021 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.3.1
- New upstream release.
* Sat Mar 06 2021 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.3.0
- New upstream release.
* Sun Feb 14 2021 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.7
- New upstream release.
* Fri Nov 13 2020 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.6
- New upstream release.
* Fri Feb 07 2020 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.5
- New upstream release.
* Wed Aug 07 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.4
- New upstream release.
* Tue Aug 06 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.3
- New upstream release.
* Fri Jun 14 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.2
- New upstream release.
* Tue May 21 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 3.2.1
- New upstream release.
* Tue Nov 21 2017 Thomas Dreibholz <dreibh@simula.no> 3.1.8
- Initial RPM release
