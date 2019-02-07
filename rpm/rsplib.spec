Name: rsplib
Version: 3.2.1~rc1.0
Release: 1
Summary: Reliable Server Pooling (RSerPool) implementation
License: GPL-3.0
Group: Applications/Internet
URL: https://www.uni-due.de/~be0001/rserpool/
Source: https://www.uni-due.de/~be0001/rserpool/download/%{name}-%{version}.tar.gz

AutoReqProv: on
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: lksctp-tools-devel
BuildRequires: bzip2-devel
BuildRequires: qt5-qtbase-devel
BuildRequires: qtchooser
BuildRoot: %{_tmppath}/%{name}-%{version}-build

# TEST ONLY:
# %define _unpackaged_files_terminate_build 0

%description
RSerPool client/server API library for session management Reliable Server Pooling (RSerPool) is the IETF's standard (RFC 5351 to RFC 5356) for a lightweight server pool and session management framework. It provides highly available pool management (that is registration handling and load distribution/balancing) by components called Registrar and a client-side/server-side API for accessing the service of a pool.

%prep
%setup -q

%build
%cmake -DCMAKE_INSTALL_PREFIX=/usr -DUSE_KERNEL_SCTP=1 -DENABLE_CSP=1 -DENABLE_QT=1 .
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install


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
/usr/lib*/librspcsp.so.*
/usr/lib*/librspdispatcher.so.*
/usr/lib*/librsphsmgt.so.*
/usr/lib*/librsplib.so.*
/usr/lib*/librspmessaging.so.*
/usr/lib*/libtdbreakdetector.so.*
/usr/lib*/libtdloglevel.so.*
/usr/lib*/libtdnetutilities.so.*
/usr/lib*/libtdrandomizer.so.*
/usr/lib*/libtdstorage.so.*
/usr/lib*/libtdstringutilities.so.*
/usr/lib*/libtdtagitem.so.*
/usr/lib*/libtdthreadsafety.so.*
/usr/lib*/libtdtimeutilities.so.*
/usr/lib*/libcpprspserver.so.*
/usr/lib*/libtdcppthread.so.*


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
/usr/include/rserpool/rserpool-internals.h
/usr/include/rserpool/rserpool-policytypes.h
/usr/include/rserpool/rserpool.h
/usr/include/rserpool/rserpool-csp.h
/usr/include/rserpool/tagitem.h
/usr/lib*/librspcsp*.a
/usr/lib*/librspcsp*.so
/usr/lib*/librspdispatcher*.a
/usr/lib*/librspdispatcher*.so
/usr/lib*/librsphsmgt*.a
/usr/lib*/librsphsmgt*.so
/usr/lib*/librsplib*.a
/usr/lib*/librsplib*.so
/usr/lib*/librspmessaging*.a
/usr/lib*/librspmessaging*.so
/usr/lib*/libtdbreakdetector*.a
/usr/lib*/libtdbreakdetector*.so
/usr/lib*/libtdloglevel*.a
/usr/lib*/libtdloglevel*.so
/usr/lib*/libtdnetutilities*.a
/usr/lib*/libtdnetutilities*.so
/usr/lib*/libtdrandomizer*.a
/usr/lib*/libtdrandomizer*.so
/usr/lib*/libtdstorage*.a
/usr/lib*/libtdstorage*.so
/usr/lib*/libtdstringutilities*.a
/usr/lib*/libtdstringutilities*.so
/usr/lib*/libtdtagitem*.a
/usr/lib*/libtdtagitem*.so
/usr/lib*/libtdthreadsafety*.a
/usr/lib*/libtdthreadsafety*.so
/usr/lib*/libtdtimeutilities*.a
/usr/lib*/libtdtimeutilities*.so
# NOTE: These files are library-internal files, not to be packaged in the RPM:
%ghost /usr/include/rserpool/asapinstance.h
%ghost /usr/include/rserpool/asapinterthreadmessage.h
%ghost /usr/include/rserpool/breakdetector.h
%ghost /usr/include/rserpool/componentstatuspackets.h
%ghost /usr/include/rserpool/componentstatusreporter.h
%ghost /usr/include/rserpool/debug.h
%ghost /usr/include/rserpool/dispatcher.h
%ghost /usr/include/rserpool/doublelinkedringlist.h
%ghost /usr/include/rserpool/ext_socket.h
%ghost /usr/include/rserpool/fdcallback.h
%ghost /usr/include/rserpool/identifierbitmap.h
%ghost /usr/include/rserpool/interthreadmessageport.h
%ghost /usr/include/rserpool/leaflinkedredblacktree.h
%ghost /usr/include/rserpool/loglevel.h
%ghost /usr/include/rserpool/messagebuffer.h
%ghost /usr/include/rserpool/netdouble.h
%ghost /usr/include/rserpool/netutilities.h
%ghost /usr/include/rserpool/notificationqueue.h
%ghost /usr/include/rserpool/peerlist-template.h
%ghost /usr/include/rserpool/peerlist-template_impl.h
%ghost /usr/include/rserpool/peerlistmanagement-template.h
%ghost /usr/include/rserpool/peerlistmanagement-template_impl.h
%ghost /usr/include/rserpool/peerlistnode-template.h
%ghost /usr/include/rserpool/peerlistnode-template_impl.h
%ghost /usr/include/rserpool/poolelementnode-template.h
%ghost /usr/include/rserpool/poolelementnode-template_impl.h
%ghost /usr/include/rserpool/poolhandle.h
%ghost /usr/include/rserpool/poolhandlespacechecksum.h
%ghost /usr/include/rserpool/poolhandlespacemanagement-basics.h
%ghost /usr/include/rserpool/poolhandlespacemanagement-template.h
%ghost /usr/include/rserpool/poolhandlespacemanagement-template_impl.h
%ghost /usr/include/rserpool/poolhandlespacemanagement.h
%ghost /usr/include/rserpool/poolhandlespacenode-template.h
%ghost /usr/include/rserpool/poolhandlespacenode-template_impl.h
%ghost /usr/include/rserpool/poolnode-template.h
%ghost /usr/include/rserpool/poolnode-template_impl.h
%ghost /usr/include/rserpool/poolpolicy-template.h
%ghost /usr/include/rserpool/poolpolicy-template_impl.h
%ghost /usr/include/rserpool/poolpolicysettings.h
%ghost /usr/include/rserpool/pooluserlist-template.h
%ghost /usr/include/rserpool/pooluserlist-template_impl.h
%ghost /usr/include/rserpool/poolusernode-template.h
%ghost /usr/include/rserpool/poolusernode-template_impl.h
%ghost /usr/include/rserpool/randomizer.h
%ghost /usr/include/rserpool/redblacktree.h
%ghost /usr/include/rserpool/redblacktree_impl.h
%ghost /usr/include/rserpool/registrartable.h
%ghost /usr/include/rserpool/rserpoolerror.h
%ghost /usr/include/rserpool/rserpoolmessage.h
%ghost /usr/include/rserpool/rserpoolmessagecreator.h
%ghost /usr/include/rserpool/rserpoolmessageparser.h
%ghost /usr/include/rserpool/rserpoolsocket.h
%ghost /usr/include/rserpool/session.h
%ghost /usr/include/rserpool/sessioncontrol.h
%ghost /usr/include/rserpool/sessionstorage.h
%ghost /usr/include/rserpool/simpleredblacktree.h
%ghost /usr/include/rserpool/sockaddrunion.h
%ghost /usr/include/rserpool/stringutilities.h
%ghost /usr/include/rserpool/tdtypes.h
%ghost /usr/include/rserpool/threadsafety.h
%ghost /usr/include/rserpool/threadsignal.h
%ghost /usr/include/rserpool/timer.h
%ghost /usr/include/rserpool/timestamphashtable.h
%ghost /usr/include/rserpool/timeutilities.h
%ghost /usr/include/rserpool/transportaddressblock.h


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
/usr/lib*/libcpprspserver.so.*
/usr/lib*/libtdcppthread.so.*

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
/usr/include/rserpool/cpprspserver.h
/usr/include/rserpool/mutex.h
/usr/include/rserpool/tcplikeserver.h
/usr/include/rserpool/thread.h
/usr/include/rserpool/udplikeserver.h
/usr/lib*/libcpprspserver*.a
/usr/lib*/libcpprspserver*.so
/usr/lib*/libtdcppthread*.a
/usr/lib*/libtdcppthread*.so


%package docs
Summary: Documentation files for RSPLIB
Group: System Environment/Libraries
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
/usr/share/rsplib/Handbook.pdf


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
/usr/bin/rspregistrar
/usr/share/man/man1/rspregistrar.1.gz


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
/usr/bin/cspmonitor
/usr/bin/hsdump
/usr/bin/rspserver
/usr/bin/rspterminal
/usr/share/man/man1/rspserver.1.gz
/usr/share/man/man1/rspterminal.1.gz
/usr/share/man/man1/cspmonitor.1.gz
/usr/share/man/man1/hsdump.1.gz


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


%package fgp-cfgfiles
Summary: RSerPool Fractal Generator Service example input files
Group: Applications/Internet
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
/usr/share/fgpconfig/*.fsf


%package all
Summary: RSerPool implementation RSPLIB
Group: Applications/Internet
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


%changelog
* Tue Nov 21 2017 Thomas Dreibholz <dreibh@simula.no> 3.1.8
- Initial RPM release
