lib_LTLIBRARIES = librsplib.la libcppthread.la

libcppthreadincludedir      = $(prefix)/include/rserpool
libcppthreadinclude_HEADERS = mutex.h thread.h
libcppthread_la_SOURCES     = mutex.cc thread.cc

librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rsplib.h rsplib-tags.h
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
fdcallback.c \
fdcallback.h \
leaflinkedbinarytree.c \
leaflinkedbinarytree.h \
leaflinkedredblacktree.c \
leaflinkedredblacktree.h \
leaflinkedtreap.c \
leaflinkedtreap.h \
linearlist.c \
linearlist.h \
localaddresses.c \
localaddresses.h \
loglevel.c \
loglevel.h \
messagebuffer.c \
messagebuffer.h \
netutilities.c \
netutilities.h \
peerlistmanagement-template.h \
peerlistnode-template.h \
peerlist-template.h \
poolelementnode-template.h \
poolhandle.h \
poolhandle.c \
poolnamespacemanagement-basics.c \
poolnamespacemanagement-basics.h \
poolnamespacemanagement.c \
poolnamespacemanagement.h \
poolnamespacemanagement-template.h \
poolnamespacenode-template.h \
poolnode-template.h \
poolpolicysettings.c \
poolpolicysettings.h \
poolpolicy-template.h \
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


noinst_PROGRAMS = nameserver examplepe examplepu simpleexamplepe simpleexamplepu fractalpoolelement testregistrator fork cspmonitor
# examplepe2 examplepu2


# ???
# -static wird überall gesetzt. Im configure-Skript sollte besser
# eingestellt werden können ob statisch oder dynamisch gelinkt wird!

nameserver_SOURCES = nameserver.c
nameserver_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

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

fractalpoolelement_SOURCES = fractalpoolelement.cc
fractalpoolelement_LDADD   = libcppthread.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

fork_SOURCES = fork.c
fork_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)

cspmonitor_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
cspmonitor_LDFLAGS = -static $(all_libraries) -L.


#t1_SOURCES = t1.cc
#t1_LDADD   = librsplib.la -lsctpsocket -lstdc++ -lsctp $(thread_LIBS) $(GLIB_LIBS) -lsctp
#t1_LDFLAGS = -static $(all_libraries) -L.

#t2_SOURCES = t2.cc
#t2_LDADD   = libcppthread.la dispatcher.o fdcallback.o timer.o timeutilities.o loglevel.o leaflinkedredblacktree.o threadsafety.o -lsctpsocket -lstdc++ -lsctp $(thread_LIBS) -lsctp $(GLIB_LIBS)
#t2_LDFLAGS = -static $(all_libraries) -L.
