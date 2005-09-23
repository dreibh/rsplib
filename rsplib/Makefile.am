lib_LTLIBRARIES = librsplib.la libcppthread.la

libcppthreadincludedir      = $(prefix)/include/rserpool
libcppthreadinclude_HEADERS = mutex.h thread.h
libcppthread_la_SOURCES     = mutex.cc thread.cc

librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rsplib.h rsplib-tags.h rsplib-internals.h
librsplib_la_SOURCES = \
rserpoolmessagecreator.c \
rserpoolmessagecreator.h \
asapinstance.c \
asapinstance.h \
rserpoolmessage.c \
rserpoolmessage.h \
rserpoolmessageparser.c \
rserpoolmessageparser.h \
breakdetector.c \
breakdetector.h \
componentstatusprotocol.h \
componentstatusprotocol.c \
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
messagebuffer.c \
messagebuffer.h \
netutilities.c \
netutilities.h \
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
registrartable.c \
registrartable.h \
rserpoolerror.c \
rserpoolerror.h \
rsplib.c \
rsplib.h \
rsplib-tags.h \
rspsession.c \
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
transportaddressblock.h
librsplib_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)


noinst_PROGRAMS = registrar examplepe examplepu simpleexamplepe simpleexamplepu fractalpoolelement testregistrator fork cspmonitor calcapppoolelement calcapppooluser t7 t8 t9 t11 t12


registrar_SOURCES = registrar.c
registrar_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

simpleexamplepe_SOURCES = simpleexamplepe.c
simpleexamplepe_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

simpleexamplepu_SOURCES = simpleexamplepu.c
simpleexamplepu_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

examplepe_SOURCES = examplepe.c
examplepe_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

testregistrator_SOURCES = testregistrator.c
testregistrator_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

examplepu_SOURCES = examplepu.c
examplepu_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

fractalpoolelement_SOURCES = fractalpoolelement.cc netdouble.c netdouble.h fractalgeneratorpackets.h
fractalpoolelement_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

calcapppooluser_SOURCES = calcapppooluser.cc calcapppackets.h statistics.cc statistics.h
calcapppooluser_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

calcapppoolelement_SOURCES = calcapppoolelement.cc calcapppackets.h statistics.cc
calcapppoolelement_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t5_SOURCES = t5.cc
#t5_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

#t6_SOURCES = t6.cc statistics.cc
#t6_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

t7_SOURCES = t7.cc statistics.cc netdouble.c netdouble.h fractalgeneratorpackets.h
t7_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

t8_SOURCES = t8.cc statistics.cc
t8_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

t9_SOURCES = t9.cc statistics.cc
t9_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

t11_SOURCES = t11.cc statistics.cc
t11_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

t12_SOURCES = t12.cc statistics.cc
t12_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

fork_SOURCES = fork.c
fork_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm

cspmonitor_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS) -lm
cspmonitor_LDFLAGS = -static $(all_libraries) -L.

EXTRA_DIST = registrar-example poolelement-example pooluser-example

#t3_SOURCES = t3.cc
#t3_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
#t3_LDFLAGS = -static $(all_libraries) -L.

#t4_SOURCES = t4.cc
#t4_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
#t4_LDFLAGS = -static $(all_libraries) -L.
