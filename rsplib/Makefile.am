SUBDIRS = fgpconfig wireshark

lib_LTLIBRARIES = librsplib.la libcpprspserver.la

# ###### rsplib library #####################################################
librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rserpool.h rserpool-policytypes.h rserpool-internals.h tagitem.h
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
messagebuffer.h \
messagebuffer.c \
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
pooluserlist-template.h \
pooluserlist-template_impl.h \
poolusernode-template.h \
poolusernode-template_impl.h \
randomizer.c \
randomizer.h \
redblacktree.h \
redblacktree_impl.h \
registrartable.c \
registrartable.h \
rserpool.h \
rserpool-internals.h \
rserpool-policytypes.h \
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
rsputilities.c \
session.h \
sessionstorage.c \
sessionstorage.h \
sessioncontrol.c \
sessioncontrol.h \
simpleredblacktree.h \
simpleredblacktree.c \
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
timestamphashtable.c \
timestamphashtable.h \
transportaddressblock.c \
transportaddressblock.h \
netdouble.c \
netdouble.h
librsplib_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) -lm


# ###### cpprspserver library ###############################################
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
libcpprspserver_la_LIBADD  = librsplib.la
libcpprspserver_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) -lm


# ###### Main programs ######################################################
if BUILD_QT_PROGRAMS
bin_PROGRAMS = fractalpooluser registrar server terminal hsdump pingpongclient scriptingclient calcappclient cspmonitor
EXTRA_DIST = registrar-example poolelement-example pooluser-example add-dummy-interface
else
bin_PROGRAMS = registrar server terminal hsdump pingpongclient scriptingclient calcappclient cspmonitor
EXTRA_DIST = fractalpooluser.cc fractalpooluser.h registrar-example poolelement-example pooluser-example add-dummy-interface
endif

if BUILD_QT_PROGRAMS
%.moc.cc: %.h
	$(MOC) -o $@ $<

%.h: %.ui
	$(UIC) -o $@ $<

%.cc: %.ui
	$(UIC) -o $@ -impl $*.h $<
endif

registrar_SOURCES = registrar.c registrar.h registrar-global.c registrar-core.c registrar-asap.c registrar-enrp.c registrar-takeover.c takeoverprocess.c takeoverprocess.h
registrar_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

server_SOURCES = server.cc standardservices.cc standardservices.h fractalgeneratorservice.cc fractalgeneratorservice.h calcappservice.cc calcappservice.h pingpongpackets.h fractalgeneratorpackets.h calcapppackets.h scriptingservice.h scriptingservice.cc scriptingpackets.h
server_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

terminal_SOURCES = terminal.c
terminal_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

hsdump_SOURCES = hsdump.c
hsdump_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

pingpongclient_SOURCES = pingpongclient.c
pingpongclient_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

scriptingclient_SOURCES = scriptingclient.c
scriptingclient_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

calcappclient_SOURCES = calcappclient.cc calcapppackets.h statistics.cc statistics.h
calcappclient_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

cspmonitor_SOURCES = cspmonitor.c
cspmonitor_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

if BUILD_QT_PROGRAMS
fractalpooluser_MOC      = fractalpooluser.moc.cc
fractalpooluser_CXXFLAGS = $(QT_CXXFLAGS) "-DDEFAULT_FPU_CONFIGDIR=\"$(datadir)/fgpconfig\""
fractalpooluser_SOURCES  = fractalpooluser.cc fractalpooluser.h $(fractalpooluser_MOC)
fractalpooluser_LDADD    = $(QT_LDADD) librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm
endif


# ###### Test programs ######################################################
noinst_PROGRAMS = hrestest testregistrator combinesummaries fork gettimestamp exectime execalarm subnetcalc rootshell

hrestest_SOURCES = hrestest.c
hrestest_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

testregistrator_SOURCES = testregistrator.c
testregistrator_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

combinesummaries_SOURCES = combinesummaries.cc
combinesummaries_LDADD   = $(bz2_LIBS)

fork_SOURCES = fork.c
fork_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

exectime_SOURCES = exectime.cc
exectime_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

execalarm_SOURCES = execalarm.cc
execalarm_LDADD   = libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

gettimestamp_SOURCES = gettimestamp.c
gettimestamp_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm

subnetcalc_SOURCES = subnetcalc.cc
subnetcalc_LDADD   =

rootshell_SOURCES = rootshell.c
rootshell_LDADD   =

# t1_SOURCES = t1.cc
# t1_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(glib_LIBS) -lm


# ###### Scripts ############################################################
dist_bin_SCRIPTS = scriptingcontrol scriptingserviceexample


# ###### Manual pages #######################################################
dist_man_MANS = registrar.8 server.8 terminal.8 cspmonitor.8 calcappclient.8 pingpongclient.8 hsdump.8 scriptingcontrol.8 scriptingserviceexample.8 scriptingclient.8 fractalpooluser.8
