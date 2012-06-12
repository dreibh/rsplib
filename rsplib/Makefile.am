# $Id$
# --------------------------------------------------------------------------
#
#              //===//   //=====   //===//   //       //   //===//
#             //    //  //        //    //  //       //   //    //
#            //===//   //=====   //===//   //       //   //===<<
#           //   \\         //  //        //       //   //    //
#          //     \\  =====//  //        //=====  //   //===//    Version II
#
# ------------- An Efficient RSerPool Prototype Implementation -------------
#
# Copyright (C) 2002-2012 by Thomas Dreibholz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: dreibh@iem.uni-due.de

SUBDIRS = fgpconfig wireshark

# NOTE: The order of the libraries is important!
lib_LTLIBRARIES = \
   libtdtimeutilities.la \
   libtdrandomizer.la  \
   libtdstringutilities.la \
   libtdthreadsafety.la \
   libtdloglevel.la \
   libtdstorage.la \
   libtdnetutilities.la \
   librsphsmgt.la \
   librspmessaging.la \
   libtdtagitem.la  \
   librspdispatcher.la  \
   librspcsp.la  \
   librsplib.la  \
   libtdbreakdetector.la \
   libtdcppthread.la \
   libcpprspserver.la


# ###### tdtimeutilities library ############################################
# libtdtimeutilitiesincludedir      = $(prefix)/include/tdutilities
# libtdtimeutilitiesinclude_HEADERS = timeutilities.h
libtdtimeutilities_la_SOURCES     = timeutilities.c timeutilities.h
libtdtimeutilities_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdrandomizer library ############################################
# libtdrandomizerincludedir      = $(prefix)/include/tdutilities
# libtdrandomizerinclude_HEADERS = randomizer.h
libtdrandomizer_la_SOURCES       = randomizer.c randomizer.h
libtdrandomizer_la_LIBADD        = libtdtimeutilities.la -lm
libtdrandomizer_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdstringutilities library ##########################################
# libtdstringutilitiesincludedir      = $(prefix)/include/tdutilities
# libtdstringutilitiesinclude_HEADERS = stringutilities.h
libtdstringutilities_la_SOURCES     = stringutilities.c stringutilities.h
libtdstringutilities_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdloglevel library #################################################
# libtdloglevelincludedir      = $(prefix)/include/tdutilities
# libtdloglevelinclude_HEADERS = loglevel.h
libtdloglevel_la_SOURCES     = loglevel.c loglevel.h
libtdloglevel_la_LIBADD      = libtdtimeutilities.la libtdthreadsafety.la
libtdloglevel_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdtagitem library ##################################################
# libtdtagitemincludedir      = $(prefix)/include/tdutilities
# libtdtagiteminclude_HEADERS = tagitem.h
libtdtagitem_la_SOURCES     = tagitem.c tagitem.h
libtdtagitem_la_LIBADD      = libtdtimeutilities.la libtdloglevel.la
libtdtagitem_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdbreakdetector library ############################################
# libtdbreakdetectorincludedir      = $(prefix)/include/tdutilities
# libtdbreakdetectorinclude_HEADERS = breakdetector.h
libtdbreakdetector_la_SOURCES     = breakdetector.c breakdetector.h
libtdbreakdetector_la_LIBADD      = libtdtimeutilities.la
libtdbreakdetector_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdthreadsafety library #############################################
# libtdthreadsafetyincludedir      = $(prefix)/include/tdutilities
# libtdthreadsafetyinclude_HEADERS = threadsafety.h threadsignal.h
libtdthreadsafety_la_SOURCES = \
threadsafety.c \
threadsafety.h \
threadsignal.c \
threadsignal.h
libtdthreadsafety_la_LIBADD  = $(thread_LIBS)
libtdthreadsafety_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdcppthread library #############################################
# libtdcppthreadincludedir      = $(prefix)/include/tdutilities
# libtdcppthreadinclude_HEADERS = cppthread.h threadsignal.h
libtdcppthread_la_SOURCES = \
mutex.cc \
mutex.h \
thread.cc \
thread.h
libtdcppthread_la_LIBADD  = libtdthreadsafety.la
libtdcppthread_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) $(thread_LIBS)

# ###### tdstorage library ##################################################
# libtdstorageincludedir      = $(prefix)/include/tdutilities
# libtdstorageinclude_HEADERS = doublelinkedringlist.h leaflinkedredblacktree.h simpleredblacktree.h
libtdstorage_la_SOURCES     = \
doublelinkedringlist.h \
doublelinkedringlist.c \
leaflinkedredblacktree.c \
leaflinkedredblacktree.h \
redblacktree.h \
redblacktree_impl.h \
simpleredblacktree.h \
simpleredblacktree.c
libtdstorage_la_LIBADD  =
libtdstorage_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)

# ###### tdnetutilities library #############################################
# libtdnetutilitiesincludedir      = $(prefix)/include/tdutilities
# libtdnetutilitiesinclude_HEADERS = ext_socket.h messagebuffer.h netutilities.h netdouble.h sockaddrunion.h
libtdnetutilities_la_SOURCES     = \
ext_socket.h \
messagebuffer.h \
messagebuffer.c \
netutilities.c \
netutilities.h \
netdouble.c \
netdouble.h \
sockaddrunion.h
libtdnetutilities_la_LIBADD  = libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdloglevel.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)
libtdnetutilities_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) -lm


# ###### rsphsmgt library ###################################################
# librsphsmgtincludedir      = $(prefix)/include/rserpool
# librsphsmgtinclude_HEADERS =
librsphsmgt_la_SOURCES = \
rserpoolerror.c \
rserpoolerror.h \
poolhandlespacechecksum.c \
poolhandlespacechecksum.h \
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
timestamphashtable.c \
timestamphashtable.h \
transportaddressblock.c \
transportaddressblock.h
librsphsmgt_la_LIBADD  = libtdstorage.la libtdrandomizer.la libtdstringutilities.la libtdnetutilities.la -lm
librsphsmgt_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)


# ###### rspmessaging library ###############################################
# librspmessagingincludedir      = $(prefix)/include/tdutilities
# librspmessaginginclude_HEADERS = rserpoolmessage.h rserpoolmessagecreator.h rserpoolmessageparser.h
librspmessaging_la_SOURCES     = \
rserpoolmessage.c \
rserpoolmessage.h \
rserpoolmessagecreator.c \
rserpoolmessagecreator.h \
rserpoolmessageparser.c \
rserpoolmessageparser.h
librspmessaging_la_LIBADD  = libtdnetutilities.la libtdtimeutilities.la librsphsmgt.la libtdloglevel.la
librspmessaging_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)


# ###### rspdispatcher library #############################################
# librspdispatcherincludedir      = $(prefix)/include/tdutilities
# librspdispatcherinclude_HEADERS = ext_socket.h netutilities.h netdouble.h
librspdispatcher_la_SOURCES     = \
dispatcher.c \
dispatcher.h \
fdcallback.c \
fdcallback.h \
timer.c \
timer.h
librspdispatcher_la_LIBADD  = libtdtimeutilities.la libtdloglevel.la libtdnetutilities.la libtdstorage.la
librspdispatcher_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) -lm


# ###### rspcsp library #####################################################
# librspcspincludedir      = $(prefix)/include/tdutilities
# librspcspinclude_HEADERS = ext_socket.h netutilities.h netdouble.h
librspcsp_la_SOURCES     = \
componentstatuspackets.h \
componentstatusreporter.h \
componentstatusreporter.c
librspcsp_la_LIBADD  = libtdtimeutilities.la libtdstringutilities.la libtdloglevel.la librspdispatcher.la libtdnetutilities.la
librspcsp_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) -lm


# ###### rsplib library #####################################################
librsplibincludedir      = $(prefix)/include/rserpool
librsplibinclude_HEADERS = rserpool.h rserpool-policytypes.h rserpool-csp.h rserpool-internals.h tagitem.h
librsplib_la_SOURCES = \
asapinstance.c \
asapinstance.h \
asapinterthreadmessage.c \
asapinterthreadmessage.h \
debug.h \
identifierbitmap.c \
identifierbitmap.h \
interthreadmessageport.c \
interthreadmessageport.h \
notificationqueue.c \
notificationqueue.h \
registrartable.c \
registrartable.h \
rserpool.h \
rserpool-internals.h \
rserpool-policytypes.h \
rserpoolsocket.c \
rserpoolsocket.h \
rspbasicmode.c \
rspenhancedmode.c \
rsputilities.c \
session.h \
sessionstorage.c \
sessionstorage.h \
sessioncontrol.c \
sessioncontrol.h \
tdtypes.h
librsplib_la_LIBADD  = libtdrandomizer.la libtdtimeutilities.la libtdstringutilities.la libtdnetutilities.la libtdthreadsafety.la libtdloglevel.la libtdtagitem.la libtdstorage.la librsphsmgt.la librspdispatcher.la librspmessaging.la librspcsp.la
librsplib_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE) -lm $(socketapi_LIBS) $(sctplib_LIBS) $(thread_LIBS)


# ###### cpprspserver library ###############################################
libcpprspserverincludedir      = $(prefix)/include/rserpool
libcpprspserverinclude_HEADERS = mutex.h thread.h cpprspserver.h udplikeserver.h tcplikeserver.h
libcpprspserver_la_SOURCES     = \
cpprspserver.h \
tcplikeserver.cc \
tcplikeserver.h \
udplikeserver.cc \
udplikeserver.h
libcpprspserver_la_LIBADD  = libtdcppthread.la librsplib.la libtdthreadsafety.la libtdnetutilities.la libtdtimeutilities.la libtdloglevel.la libtdbreakdetector.la
libcpprspserver_la_LDFLAGS = \
   -version-info $(RSPLIB_CURRENT):$(RSPLIB_REVISION):$(RSPLIB_AGE)


# ###### Main programs ######################################################
if BUILD_QT_PROGRAMS
bin_PROGRAMS = fractalpooluser rspregistrar rspserver rspterminal hsdump pingpongclient scriptingclient calcappclient cspmonitor
EXTRA_DIST = add-dummy-interface
else
bin_PROGRAMS = rspregistrar rspserver rspterminal hsdump pingpongclient scriptingclient calcappclient cspmonitor
EXTRA_DIST = fractalpooluser.cc fractalpooluser.h fractalpooluser_de_DE.ts add-dummy-interface
endif

if ENABLE_REGISTRAR_STATISTICS
rspregistrar_SOURCES = rspregistrar.c rspregistrar.h rspregistrar-global.c rspregistrar-core.c rspregistrar-asap.c rspregistrar-enrp.c rspregistrar-takeover.c rspregistrar-security.c rspregistrar-misc.c takeoverprocess.c takeoverprocess.h
rspregistrar_LDADD   = libtdbreakdetector.la librspdispatcher.la librspcsp.la librsphsmgt.la librspmessaging.la libtdstorage.la libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la -lm $(bz2_LIBS) $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)
else
rspregistrar_SOURCES = rspregistrar.c rspregistrar.h rspregistrar-global.c rspregistrar-core.c rspregistrar-asap.c rspregistrar-enrp.c rspregistrar-takeover.c rspregistrar-security.c rspregistrar-misc.c takeoverprocess.c takeoverprocess.h
rspregistrar_LDADD   = libtdbreakdetector.la librspdispatcher.la librspcsp.la librsphsmgt.la librspmessaging.la libtdstorage.la libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la -lm $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)
endif

rspserver_SOURCES = rspserver.cc standardservices.cc standardservices.h fractalgeneratorservice.cc fractalgeneratorservice.h calcappservice.cc calcappservice.h pingpongpackets.h fractalgeneratorpackets.h calcapppackets.h scriptingservice.h scriptingservice.cc scriptingpackets.h environmentcache.cc environmentcache.h sha1.c sha1.h
rspserver_LDADD   = libtdcppthread.la libtdthreadsafety.la libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la libtdbreakdetector.la libcpprspserver.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

rspterminal_SOURCES = rspterminal.c
rspterminal_LDADD   = libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la libtdbreakdetector.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

hsdump_SOURCES = hsdump.c
hsdump_LDADD   = libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la libtdbreakdetector.la librsphsmgt.la librspmessaging.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

pingpongclient_SOURCES = pingpongclient.c
pingpongclient_LDADD   = libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la libtdbreakdetector.la librsplib.la -lm $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

scriptingclient_SOURCES = scriptingclient.c sha1.c sha1.h
scriptingclient_LDADD   = libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la libtdbreakdetector.la librsplib.la -lm $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

calcappclient_SOURCES = calcappclient.cc calcapppackets.h statistics.cc statistics.h
calcappclient_LDADD   = libtdrandomizer.la libtdstringutilities.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la libtdbreakdetector.la librsplib.la -lm $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

cspmonitor_SOURCES = cspmonitor.c
cspmonitor_LDADD   = librspcsp.la libtdbreakdetector.la libtdtimeutilities.la libtdstringutilities.la libtdnetutilities.la libtdstorage.la -lm $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)


# ###### FractalPoolUser program ############################################
if BUILD_QT_PROGRAMS
FRACTALPOOLUSERMOCHEADER = fractalpooluser.h
FRACTALPOOLUSERMOCEDFILES = $(FRACTALPOOLUSERMOCHEADER:%.h=%_moc.cc)
CLEANFILES = $(FRACTALPOOLUSERMOCEDFILES)

%_moc.cc: %.h
	$(MOC) -o $@ $<

# To compile (i.e. generate object files), first create MOC file.
# This MOC file is included by fractalpooluser.cc, so no compile step is necessary.
$(fractalpooluser_OBJECTS): $(FRACTALPOOLUSERMOCEDFILES)

fractalpooluserdir = $(datadir)/fractalpooluser
dist_fractalpooluser_DATA = fractalpooluser_de_DE.qm

fractalpooluser_CXXFLAGS = $(QT_CXXFLAGS) "-DDEFAULT_FPU_CONFIGDIR=\"$(datadir)/fgpconfig\""
fractalpooluser_SOURCES  = fractalpooluser.cc fractalpooluser.h
fractalpooluser_LDADD    = $(QT_LDADD) libtdrandomizer.la libtdtimeutilities.la libtdnetutilities.la libtdloglevel.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)
endif


# ###### Test programs ######################################################
if BUILD_TEST_PROGRAMS
noinst_PROGRAMS = hrestest testregistrator fork gettimestamp exectime execalarm rootshell attacker
else
noinst_PROGRAMS =
endif

if BUILD_TEST_PROGRAMS
hrestest_SOURCES = hrestest.c
hrestest_LDADD   = libtdbreakdetector.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

testregistrator_SOURCES = testregistrator.c
testregistrator_LDADD   = libtdbreakdetector.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

fork_SOURCES = fork.c
fork_LDADD   = libtdtimeutilities.la libtdrandomizer.la -lm

exectime_SOURCES = exectime.cc
exectime_LDADD   = libtdcppthread.la libtdtimeutilities.la

execalarm_SOURCES = execalarm.cc
execalarm_LDADD   = libtdtimeutilities.la

gettimestamp_SOURCES = gettimestamp.c
gettimestamp_LDADD   = libtdtimeutilities.la

rootshell_SOURCES = rootshell.c
rootshell_LDADD   =

attacker_SOURCES = attacker.cc
attacker_LDADD   = libtdbreakdetector.la librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

#t1_SOURCES = t1.c
#t1_LDADD   = librsplib.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)

#t2_SOURCES = t2.cc environmentcache.cc environmentcache.h sha1.c sha1.h
#t2_LDADD   = librsplib.la libcpprspserver.la $(socketapi_LIBS) $(sctplib_LIBS) $(glib_LIBS)
endif


# ###### Scripts ############################################################
dist_bin_SCRIPTS = scriptingcontrol scriptingserviceexample server terminal registrar


# ###### Manual pages #######################################################
dist_man_MANS = rspregistrar.1 rspserver.1 rspterminal.1 cspmonitor.1 calcappclient.1 pingpongclient.1 hsdump.1 scriptingcontrol.1 scriptingserviceexample.1 scriptingclient.1 fractalpooluser.1 server.1 terminal.1 registrar.1
