lib_LTLIBRARIES = librsplib.la libcpprspserver.la

librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rserpool.h rserpool-internals.h
librsplib_la_SOURCES = \
asapinstance.c \
asapinstance.h \
asapinterthreadmessage.c \
asapinterthreadmessage.h \
breakdetector.c \
breakdetector.h \
componentstatuspackets.h \
componentstatusreporter.h \
componentstatusreporter.c \
debug.h \
dispatcher.c \
dispatcher.h \
doublelinkedringlist.h \
doublelinkedringlist.c \
ext_socket.h \
fdcallback.c \
fdcallback.h \
identifierbitmap.c \
identifierbitmap.h \
interthreadmessageport.c \
interthreadmessageport.h \
poolhandlespacechecksum.c \
poolhandlespacechecksum.h \
leaflinkedredblacktree.c \
leaflinkedredblacktree.h \
loglevel.c \
loglevel.h \
netutilities.c \
netutilities.h \
notificationqueue.c \
notificationqueue.h \
peerlistmanagement-template.h \
peerlistmanagement-template_impl.h \
peerlistnode-template.h \
peerlistnode-template_impl.h \
peerlist-template.h \
peerlist-template_impl.h \
poolelementnode-template.h \
poolelementnode-template_impl.h \
poolhandle.h \
poolhandle.c \
poolhandlespacemanagement-basics.c \
poolhandlespacemanagement-basics.h \
poolhandlespacemanagement.c \
poolhandlespacemanagement.h \
poolhandlespacemanagement-template.h \
poolhandlespacemanagement-template_impl.h \
poolhandlespacenode-template.h \
poolhandlespacenode-template_impl.h \
poolnode-template.h \
poolnode-template_impl.h \
poolpolicysettings.c \
poolpolicysettings.h \
poolpolicy-template.h \
poolpolicy-template_impl.h \
randomizer.c \
randomizer.h \
redblacktree.c \
redblacktree.h \
registrartable.c \
registrartable.h \
rserpool.h \
rserpoolsocket.c \
rserpoolsocket.h \
rserpoolerror.c \
rserpoolerror.h \
rserpoolmessage.c \
rserpoolmessage.h \
rserpoolmessagecreator.c \
rserpoolmessagecreator.h \
rserpoolmessageparser.c \
rserpoolmessageparser.h \
rspbasicmode.c \
rspenhancedmode.c \
session.h \
sessionstorage.c \
sessionstorage.h \
sessioncontrol.c \
sessioncontrol.h \
stringutilities.c \
stringutilities.h \
sockaddrunion.h \
timeutilities.c \
timeutilities.h \
tagitem.c \
tagitem.h \
tdtypes.h \
threadsafety.c \
threadsafety.h \
threadsignal.c \
threadsignal.h \
timer.c \
timer.h \
transportaddressblock.c \
transportaddressblock.h \
netdouble.c \
netdouble.h
librsplib_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)


libcpprspserverincludedir      = $(prefix)/include/rserpool
libcpprspserverinclude_HEADERS = mutex.h thread.h cpprspserver.h udplikeserver.h tcplikeserver.h
libcpprspserver_la_SOURCES     = \
cpprspserver.h \
mutex.cc \
mutex.h \
tcplikeserver.cc \
tcplikeserver.h \
thread.cc \
thread.h \
udplikeserver.cc \
udplikeserver.h
libcpprspserver_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)


if BUILD_QT_PROGRAMS
%.moc.cc: %.h
	$(MOC) -o $@ $<

%.h: %.ui
	$(UIC) -o $@ $<

%.cc: %.ui
	$(UIC) -o $@ -impl $*.h $<
endif

if BUILD_QT_PROGRAMS
noinst_PROGRAMS = fractalpooluser registrar terminal pingpong server fork gettimestamp cspmonitor testregistrator fractalpooluser calcapppoolelement calcapppooluser hsdump
else
noinst_PROGRAMS = registrar terminal pingpong server fork gettimestamp cspmonitor testregistrator calcapppoolelement calcapppooluser hsdump
endif


if BUILD_QT_PROGRAMS
fractalpooluser_MOC      = fractalpooluser.moc.cc
fractalpooluser_CXXFLAGS = $(QT_CXXFLAGS)
fractalpooluser_CXXADD   = $(QT_CXXFLAGS)
fractalpooluser_SOURCES  = fractalpooluser.cc fractalpooluser.h $(fractalpooluser_MOC) rsputilities.c rsputilities.h
fractalpooluser_LDADD    = $(QT_LDADD) librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm
endif


registrar_SOURCES = registrar.c
registrar_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

hsdump_SOURCES = hsdump.c
hsdump_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#simpleexamplepe_SOURCES = simpleexamplepe.c
#simpleexamplepe_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#simpleexamplepu_SOURCES = simpleexamplepu.c
#simpleexamplepu_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#examplepe_SOURCES = examplepe.c
#examplepe_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

testregistrator_SOURCES = testregistrator.c rsputilities.c rsputilities.h
testregistrator_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

terminal_SOURCES = terminal.c rsputilities.c rsputilities.h
terminal_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

pingpong_SOURCES = pingpong.c rsputilities.c rsputilities.h
pingpong_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

server_SOURCES = server.cc rsputilities.c rsputilities.h standardservices.cc standardservices.h pingpongpackets.h fractalgeneratorpackets.h
server_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#fractalpoolelement_SOURCES = fractalpoolelement.cc netdouble.c netdouble.h fractalgeneratorpackets.h
#fractalpoolelement_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

calcapppooluser_SOURCES = calcapppooluser.cc calcapppackets.h statistics.cc statistics.h rsputilities.c rsputilities.h
calcapppooluser_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

calcapppoolelement_SOURCES = calcapppoolelement.cc calcapppackets.h statistics.cc rsputilities.c rsputilities.h
calcapppoolelement_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

fork_SOURCES = fork.c
fork_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

gettimestamp_SOURCES = gettimestamp.c
gettimestamp_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

cspmonitor_SOURCES = cspmonitor.c
cspmonitor_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm


EXTRA_DIST = registrar-example poolelement-example pooluser-example examplens examplepe examplepu daytimeserver discardserver echoserver fractalserver nameserver pingpongserver pingpongclient client


#t3_SOURCES = t3.cc
#t3_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

#t4_SOURCES = t4.cc
#t4_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

#t5_SOURCES = t5.cc
#t5_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t6_SOURCES = t6.cc statistics.cc
#t6_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t7_SOURCES = t7.cc statistics.cc fractalgeneratorpackets.h
#t7_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t8_SOURCES = t8.cc statistics.cc statistics.h
#t8_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t9_SOURCES = t9.cc statistics.cc statistics.h
#t9_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t11_SOURCES = t11.cc statistics.cc
#t11_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t12_SOURCES = t12.cc statistics.cc statistics.h rsputilities.c rsputilities.h
#t12_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm
