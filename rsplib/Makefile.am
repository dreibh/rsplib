lib_LTLIBRARIES = librsplib.la libtagitem.la libnetutilities.la libstringutilities.la libtimeutilities.la librandomizer.la libloglevel.la libthreadsafety.la

librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rsplib.h rsplib-tags.h
librsplib_la_SOURCES = \
asapcreator.c \
asapcreator.h \
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

libtagitemincludedir      = $(prefix)/include/rserpool
libtagiteminclude_HEADERS = tdtypes.h tagitem.h tagitem.h
libtagitem_la_SOURCES = tagitem.c
libtagitem_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

libstringutilitiesincludedir      = $(prefix)/include/rserpool
libstringutilitiesinclude_HEADERS = stringutilities.h
libstringutilities_la_SOURCES = stringutilities.c
libstringutilities_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) \
   -release $(RSPLIB_RELEASE)

libtimeutilitiesincludedir      = $(prefix)/include/rserpool
libtimeutilitiesinclude_HEADERS = timeutilities.h
libtimeutilities_la_SOURCES = timeutilities.c
libtimeutilities_la_LDFLAGS = \
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
nameserver_LDADD   = librsplib.la libnetutilities.la libstringutilities.la libtimeutilities.la libtagitem.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
nameserver_LDFLAGS = $(all_libraries) -L.

simpleexamplepe_SOURCES = simpleexamplepe.c
simpleexamplepe_LDADD   = librsplib.la libnetutilities.la libstringutilities.la libtimeutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
simpleexamplepe_LDFLAGS = $(all_libraries) -L.

simpleexamplepu_SOURCES = simpleexamplepu.c
simpleexamplepu_LDADD   = librsplib.la libnetutilities.la libstringutilities.la libtimeutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
simpleexamplepu_LDFLAGS = $(all_libraries) -L.

examplepe_SOURCES = examplepe.c
examplepe_LDADD   = librsplib.la libnetutilities.la libstringutilities.la libtimeutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
examplepe_LDFLAGS = $(all_libraries) -L.

examplepu_SOURCES = examplepu.c
examplepu_LDADD   = librsplib.la libnetutilities.la libstringutilities.la libtimeutilities.la libloglevel.la libthreadsafety.la $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS) $(GLIB_LIBS)
examplepu_LDFLAGS = $(all_libraries) -L.


fork_SOURCES =  fork.c
fork_LDADD   = libstringutilities.la libtimeutilities.la libloglevel.la librandomizer.la libthreadsafety.la $(thread_LIBS) $(GLIB_LIBS)
fork_LDFLAGS = $(all_libraries) -L.


#t1_SOURCES = t1.cc
#t1_LDADD   = librsplib.la -lsctpsocket -lstdc++ -lsctp $(thread_LIBS) $(GLIB_LIBS) -lsctp
#t1_LDFLAGS = $(all_libraries) -L.
