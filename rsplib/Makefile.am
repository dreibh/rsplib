lib_LTLIBRARIES = librsplib.la libnetutilities.la libutilities.la librandomizer.la libloglevel.la libthreadsafety.la

librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rsplib.h rsplib-tags.h
librsplib_la_SOURCES = \
asapcache.c \
asapcache.h \
asapcreator.c \
asapcreator.h \
asaperror.c \
asaperror.h \
asapinstance.c \
asapinstance.h \
asapmessage.c \
asapmessage.h \
asapparser.c \
asapparser.h \
binarytree.c \
binarytree.h \
breakdetector.c \
breakdetector.h \
debug.h \
dispatcher.c \
dispatcher.h \
doublelinkedringlist.h \
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
peerlistmanagement-template.c \
peerlistmanagement-template.h \
peerlistnode-template.c \
peerlistnode-template.h \
peerlist-template.c \
peerlist-template.h \
poolelementnode-template.c \
poolelementnode-template.h \
poolnamespacemanagement-basics.c \
poolnamespacemanagement-basics.h \
poolnamespacemanagement.c \
poolnamespacemanagement.h \
poolnamespacemanagement-template.c \
poolnamespacemanagement-template.h \
poolnamespacenode-template.c \
poolnamespacenode-template.h \
poolnode-template.c \
poolnode-template.h \
poolpolicysettings.c \
poolpolicysettings.h \
poolpolicy-template.c \
poolpolicy-template.h \
randomizer.c \
randomizer.h \
rsplib.c \
rsplib.h \
rsplib-tags.h \
rspsession.c \
serverannounce.c \
serverannounce.h \
servertable.c \
servertable.h \
stringutilities.c \
stringutilities.h \
tagitem.c \
tagitem.h \
tdtypes.h \
threadsafety.c \
threadsafety.h \
timer.c \
timer.h \
transportaddressblock.c \
transportaddressblock.h \
utilities.c \
utilities.h

librsplib_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

libutilitiesincludedir      = $(prefix)/include/rserpool
libutilitiesinclude_HEADERS = tdtypes.h utilities.h tagitem.h
libutilities_la_SOURCES = utilities.c tagitem.c
libutilities_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

libnetutilitiesincludedir      = $(prefix)/include/rserpool
libnetutilitiesinclude_HEADERS = netutilities.h
libnetutilities_la_SOURCES = netutilities.c
libnetutilities_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

librandomizerincludedir      = $(prefix)/include/rserpool
librandomizerinclude_HEADERS = randomizer.h
librandomizer_la_SOURCES = randomizer.c
librandomizer_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

libloglevelincludedir      = $(prefix)/include/rserpool
libloglevelinclude_HEADERS = loglevel.h
libloglevel_la_SOURCES = loglevel.c
libloglevel_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

libthreadsafetyincludedir      = $(prefix)/include/rserpool
libthreadsafetyinclude_HEADERS = threadsafety.h
libthreadsafety_la_SOURCES = threadsafety.c
libthreadsafety_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)


noinst_PROGRAMS = nameserver examplepe examplepu simpleexamplepe simpleexamplepu fork
# examplepe2 examplepu2


nameserver_SOURCES = nameserver.c
nameserver_LDADD   = librsplib.la libnetutilities.la libutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
nameserver_LDFLAGS = $(all_libraries) -L.

simpleexamplepe_SOURCES = simpleexamplepe.c
simpleexamplepe_LDADD   = librsplib.la libnetutilities.la libutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
simpleexamplepe_LDFLAGS = $(all_libraries) -L.

simpleexamplepu_SOURCES = simpleexamplepu.c
simpleexamplepu_LDADD   = librsplib.la libnetutilities.la libutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
simpleexamplepu_LDFLAGS = $(all_libraries) -L.

examplepe_SOURCES = examplepe.c
examplepe_LDADD   = librsplib.la libnetutilities.la libutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
examplepe_LDFLAGS = $(all_libraries) -L.

examplepu_SOURCES = examplepu.c
examplepu_LDADD   = librsplib.la libnetutilities.la libutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
examplepu_LDFLAGS = $(all_libraries) -L.


fork_SOURCES =  fork.c
fork_LDADD   = libutilities.la libloglevel.la librandomizer.la libthreadsafety.la $(thread_LIBS) $(GLIB_LIBS)
fork_LDFLAGS = $(all_libraries) -L.


#t1_SOURCES = t1.cc
#t1_LDADD   = librsplib.la -lsctpsocket -lstdc++ -lsctp $(thread_LIBS) $(GLIB_LIBS) -lsctp
#t1_LDFLAGS = $(all_libraries) -L.
