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
binarytree.c \
binarytree.h \
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
poolhandlespacechecksum.c \
poolhandlespacechecksum.h \
leaflinkedbinarytree.c \
leaflinkedbinarytree.h \
leaflinkedredblacktree.c \
leaflinkedredblacktree.h \
leaflinkedtreap.c \
leaflinkedtreap.h \
linearlist.c \
linearlist.h \
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
rserpoolerror.c \
rserpoolerror.h \
rsplib.c \
rsplib.h \
rsplib-tags.h \
rspsession.c \
servertable.c \
servertable.h \
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
timer.c \
timer.h \
transportaddressblock.c \
transportaddressblock.h
librsplib_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)


noinst_PROGRAMS = registrar examplepe examplepu simpleexamplepe simpleexamplepu fractalpoolelement testregistrator fork cspmonitor
# examplepe2 examplepu2 t3


# ???
# -static wird überall gesetzt. Im configure-Skript sollte besser
# eingestellt werden können ob statisch oder dynamisch gelinkt wird!

registrar_SOURCES = registrar.c
registrar_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

simpleexamplepe_SOURCES = simpleexamplepe.c
simpleexamplepe_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

simpleexamplepu_SOURCES = simpleexamplepu.c
simpleexamplepu_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

examplepe_SOURCES = examplepe.c
examplepe_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

testregistrator_SOURCES = testregistrator.c
testregistrator_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

examplepu_SOURCES = examplepu.c
examplepu_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

fractalpoolelement_SOURCES = fractalpoolelement.cc fractalgeneratorexample.h
fractalpoolelement_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

fork_SOURCES = fork.c
fork_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

cspmonitor_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
cspmonitor_LDFLAGS = -static $(all_libraries) -L.

EXTRA_DIST = registrar-example poolelement-example pooluser-example

#t3_SOURCES = t3.cc
#t3_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
#t3_LDFLAGS = -static $(all_libraries) -L.
