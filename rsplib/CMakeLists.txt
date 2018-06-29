include_directories(.)

# ###### Libraries ##########################################################

# ====== liblibtdtimeutilities ==============================================
LIST(APPEND libtdtimeutilities_headers
   timeutilities.h
)
LIST(APPEND libtdtimeutilities_sources
   timeutilities.c
)
ADD_LIBRARY(libtdtimeutilities SHARED ${libtdtimeutilities_sources})

set_target_properties(libtdtimeutilities PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdtimeutilities-static STATIC ${libtdtimeutilities_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdtimeutilities "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdtimeutilities ${libtdtimeutilities_libs} ${NEAT_SOCKETAPI_LIB} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdtimeutilities libtdtimeutilities-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdtimeutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== liblibtdrandomizer =================================================
LIST(APPEND libtdrandomizer_headers
   randomizer.h
)
LIST(APPEND libtdrandomizer_sources
   randomizer.c
)
ADD_LIBRARY(libtdrandomizer SHARED ${libtdrandomizer_sources})

set_target_properties(libtdrandomizer PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdrandomizer-static STATIC ${libtdrandomizer_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdrandomizer "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdrandomizer ${libtdrandomizer_libs} ${NEAT_SOCKETAPI_LIB} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdrandomizer libtdrandomizer-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdrandomizer_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== liblibtdstringutilities ============================================
LIST(APPEND libtdstringutilities_headers
   stringutilities.h
)
LIST(APPEND libtdstringutilities_sources
   stringutilities.c
)
ADD_LIBRARY(libtdstringutilities SHARED ${libtdstringutilities_sources})

set_target_properties(libtdstringutilities PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdstringutilities-static STATIC ${libtdstringutilities_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdstringutilities "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdstringutilities ${libtdstringutilities_libs} ${NEAT_SOCKETAPI_LIB} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdstringutilities libtdstringutilities-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdstringutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdloglevel ======================================================
LIST(APPEND libtdloglevel_headers
   loglevel.h
)
LIST(APPEND libtdloglevel_sources
   loglevel.c
)
ADD_LIBRARY(libtdloglevel SHARED ${libtdloglevel_sources})

set_target_properties(libtdloglevel PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdloglevel-static STATIC ${libtdloglevel_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdloglevel "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdloglevel libtdtimeutilities ${libtdloglevel_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdloglevel libtdloglevel-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdloglevel_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdtagitem =======================================================
LIST(APPEND libtdtagitem_headers
   tagitem.h
)
LIST(APPEND libtdtagitem_sources
   tagitem.c
)
ADD_LIBRARY(libtdtagitem SHARED ${libtdtagitem_sources})

set_target_properties(libtdtagitem PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdtagitem-static STATIC ${libtdtagitem_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdtagitem "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdtagitem libtdloglevel libtdtimeutilities ${libtdtagitem_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdtagitem libtdtagitem-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdtagitem_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdbreakdetector =================================================
LIST(APPEND libtdbreakdetector_headers
   breakdetector.h
)
LIST(APPEND libtdbreakdetector_sources
   breakdetector.c
)
ADD_LIBRARY(libtdbreakdetector SHARED ${libtdbreakdetector_sources})

set_target_properties(libtdbreakdetector PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdbreakdetector-static STATIC ${libtdbreakdetector_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdbreakdetector "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdbreakdetector libtdtimeutilities ${libtdbreakdetector_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdbreakdetector libtdbreakdetector-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdbreakdetector_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdthreadsafety ======================================================
LIST(APPEND libtdthreadsafety_headers
   threadsafety.h threadsignal.h
)
LIST(APPEND libtdthreadsafety_sources
   threadsafety.c threadsignal.c
)
ADD_LIBRARY(libtdthreadsafety SHARED ${libtdthreadsafety_sources})

set_target_properties(libtdthreadsafety PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdthreadsafety-static STATIC ${libtdthreadsafety_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdthreadsafety "-pthread")
ENDIF()

INSTALL(TARGETS libtdthreadsafety libtdthreadsafety-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdthreadsafety_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdcppthread =====================================================
LIST(APPEND libtdcppthread_headers
   mutex.h thread.h
)
LIST(APPEND libtdcppthread_sources
   mutex.cc thread.cc
)
ADD_LIBRARY(libtdcppthread SHARED ${libtdcppthread_sources})

set_target_properties(libtdcppthread PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdcppthread-static STATIC ${libtdcppthread_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdcppthread "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdcppthread libtdthreadsafety ${libtdcppthread_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdcppthread libtdcppthread-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdcppthread_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdstorage =====================================================
LIST(APPEND libtdstorage_headers
   doublelinkedringlist.h
   leaflinkedredblacktree.h
   redblacktree.h
   redblacktree_impl.h
   simpleredblacktree.h
)
LIST(APPEND libtdstorage_sources
   doublelinkedringlist.c
   leaflinkedredblacktree.c
   simpleredblacktree.c
)
ADD_LIBRARY(libtdstorage SHARED ${libtdstorage_sources})

set_target_properties(libtdstorage PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdstorage-static STATIC ${libtdstorage_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdstorage "-pthread")
ENDIF()

INSTALL(TARGETS libtdstorage libtdstorage-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdstorage_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== libtdnetutilities ==================================================
LIST(APPEND libtdnetutilities_headers
   ext_socket.h
   messagebuffer.h
   netutilities.h
   netdouble.h
   sockaddrunion.h
)
LIST(APPEND libtdnetutilities_sources
   messagebuffer.c
   netutilities.c
   netdouble.c
)
ADD_LIBRARY(libtdnetutilities SHARED ${libtdnetutilities_sources})

set_target_properties(libtdnetutilities PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdnetutilities-static STATIC ${libtdnetutilities_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdnetutilities "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdnetutilities libtdrandomizer libtdstringutilities libtdtimeutilities libtdloglevel ${libtdnetutilities_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdnetutilities libtdnetutilities-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdnetutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== librsphsmgt ========================================================
LIST(APPEND librsphsmgt_headers
   rserpoolerror.h
   poolhandlespacechecksum.h
   peerlistmanagement-template.h
   peerlistmanagement-template_impl.h
   peerlistnode-template.h
   peerlistnode-template_impl.h
   peerlist-template.h
   peerlist-template_impl.h
   poolelementnode-template.h
   poolelementnode-template_impl.h
   poolhandle.h
   poolhandlespacemanagement-basics.h
   poolhandlespacemanagement.h
   poolhandlespacemanagement-template.h
   poolhandlespacemanagement-template_impl.h
   poolhandlespacenode-template.h
   poolhandlespacenode-template_impl.h
   poolnode-template.h
   poolnode-template_impl.h
   poolpolicysettings.h
   poolpolicy-template.h
   poolpolicy-template_impl.h
   pooluserlist-template.h
   pooluserlist-template_impl.h
   poolusernode-template.h
   poolusernode-template_impl.h
   timestamphashtable.h
   transportaddressblock.h
)
LIST(APPEND librsphsmgt_sources
   rserpoolerror.c
   poolhandlespacechecksum.c
   poolhandle.c
   poolhandlespacemanagement-basics.c
   poolhandlespacemanagement.c
   poolpolicysettings.c
   timestamphashtable.c
   transportaddressblock.c
)
ADD_LIBRARY(librsphsmgt SHARED ${librsphsmgt_sources})

set_target_properties(librsphsmgt PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(librsphsmgt-static STATIC ${librsphsmgt_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC librsphsmgt "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (librsphsmgt libtdstorage libtdrandomizer libtdstringutilities libtdnetutilities ${librsphsmgt_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS librsphsmgt librsphsmgt-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${librsphsmgt_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== librspmessaging ========================================================
LIST(APPEND librspmessaging_headers
   rserpoolmessage.h
   rserpoolmessagecreator.h
   rserpoolmessageparser.h
)
LIST(APPEND librspmessaging_sources
   rserpoolmessage.c
   rserpoolmessagecreator.c
   rserpoolmessageparser.c
)
ADD_LIBRARY(librspmessaging SHARED ${librspmessaging_sources})

set_target_properties(librspmessaging PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(librspmessaging-static STATIC ${librspmessaging_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC librspmessaging "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (librspmessaging  libtdnetutilities libtdtimeutilities librsphsmgt libtdloglevel ${librspmessaging_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS librspmessaging librspmessaging-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${librspmessaging_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== librspdispatcher ===================================================
LIST(APPEND librspdispatcher_headers
   dispatcher.h
   fdcallback.h
   timer.h
)
LIST(APPEND librspdispatcher_sources
   dispatcher.c
   fdcallback.c
   timer.c
)
ADD_LIBRARY(librspdispatcher SHARED ${librspdispatcher_sources})

set_target_properties(librspdispatcher PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(librspdispatcher-static STATIC ${librspdispatcher_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC librspdispatcher "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (librspdispatcher libtdtimeutilities libtdloglevel libtdnetutilities libtdstorage ${librspdispatcher_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS librspdispatcher librspdispatcher-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${librspdispatcher_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== librspcsp ==========================================================
LIST(APPEND librspcsp_headers
   componentstatuspackets.h
   componentstatusreporter.h
)
LIST(APPEND librspcsp_sources
   componentstatusreporter.c
)
ADD_LIBRARY(librspcsp SHARED ${librspcsp_sources})

set_target_properties(librspcsp PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(librspcsp-static STATIC ${librspcsp_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC librspcsp "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (librspcsp libtdtimeutilities libtdstringutilities libtdloglevel librspdispatcher libtdnetutilities ${librspcsp_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS librspcsp librspcsp-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${librspcsp_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== librsplib ==========================================================
LIST(APPEND librsplib_headers
   asapinstance.h
   asapinterthreadmessage.h
   debug.h
   identifierbitmap.h
   interthreadmessageport.h
   notificationqueue.h
   registrartable.h
   rserpool.h
   rserpool-internals.h
   rserpool-policytypes.h
   rserpoolsocket.h
   session.h
   sessionstorage.h
   sessioncontrol.h
   tdtypes.h
)
LIST(APPEND librsplib_sources
   asapinstance.c
   asapinterthreadmessage.c
   identifierbitmap.c
   interthreadmessageport.c
   notificationqueue.c
   registrartable.c
   rserpoolsocket.c
   rspbasicmode.c
   rspenhancedmode.c
   rsputilities.c
   sessionstorage.c
   sessioncontrol.c

)
ADD_LIBRARY(librsplib SHARED ${librsplib_sources})

set_target_properties(librsplib PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(librsplib-static STATIC ${librsplib_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC librsplib "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (librsplib libtdrandomizer libtdtimeutilities libtdstringutilities libtdnetutilities libtdthreadsafety libtdloglevel libtdtagitem libtdstorage librsphsmgt librspdispatcher librspmessaging librspcsp ${librsplib_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS librsplib librsplib-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${librsplib_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ====== librspcpprspserver =================================================
LIST(APPEND librspcpprspserver_headers
   cpprspserver.h
   tcplikeserver.h
   udplikeserver.h
)
LIST(APPEND librspcpprspserver_sources
   tcplikeserver.cc
   udplikeserver.cc
)
ADD_LIBRARY(librspcpprspserver SHARED ${librspcpprspserver_sources})

set_target_properties(librspcpprspserver PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(librspcpprspserver-static STATIC ${librspcpprspserver_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC librspcpprspserver "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (librspcpprspserver libtdcppthread librsplib libtdthreadsafety libtdnetutilities libtdtimeutilities libtdloglevel libtdbreakdetector ${librspcpprspserver_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS librspcpprspserver librspcpprspserver-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${librspcpprspserver_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})



# ###### Programs ###########################################################
# ADD_EXECUTABLE(rtpa-server rtpa-server.cc)
# TARGET_LINK_LIBRARIES(rtpa-server qosmgr rtpaudioserver rtpserver libtdtimeutilities "${CMAKE_THREAD_LIBS_INIT}")
# INSTALL(TARGETS             rtpa-server
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#         BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
# INSTALL(FILES rtpa-server.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
# 
# 
# ADD_EXECUTABLE(rtpa-client rtpa-client.cc)
# TARGET_LINK_LIBRARIES(rtpa-client rtpaudioclient audiodecoder audiowriter libtdloglevel libtdtimeutilities "${CMAKE_THREAD_LIBS_INIT}")
# INSTALL(TARGETS             rtpa-client
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#         BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
# INSTALL(FILES rtpa-client.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
# 
# ADD_EXECUTABLE(rtpa-vclient rtpa-vclient.cc)
# TARGET_LINK_LIBRARIES(rtpa-vclient rtpaudioclient audiodecoder audiowriter libtdloglevel libtdtimeutilities "${CMAKE_THREAD_LIBS_INIT}")
# INSTALL(TARGETS             rtpa-vclient
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#         BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
# INSTALL(FILES rtpa-vclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
#       
# IF (ENABLE_QT)
#    ADD_EXECUTABLE(rtpa-qclient rtpa-qclient.cc qaudiomixer.cc qinfotabwidget.cc qspectrumanalyzer.cc)
#    qt5_use_modules(rtpa-qclient Core Widgets)
#    TARGET_LINK_LIBRARIES(rtpa-qclient rtpaudioclient audiodecoder audiowriter libtdloglevel libtdtimeutilities "${CMAKE_THREAD_LIBS_INIT}")
#    INSTALL(TARGETS             rtpa-qclient
#          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#          BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
#    INSTALL(FILES rtpa-qclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
# ENDIF()
