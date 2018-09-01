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

SET_TARGET_PROPERTIES(libtdtimeutilities PROPERTIES
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

SET_TARGET_PROPERTIES(libtdrandomizer PROPERTIES
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

SET_TARGET_PROPERTIES(libtdstringutilities PROPERTIES
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

SET_TARGET_PROPERTIES(libtdloglevel PROPERTIES
   VERSION ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)

ADD_LIBRARY(libtdloglevel-static STATIC ${libtdloglevel_sources})

if(THREADS_HAVE_PTHREAD_ARG)
   TARGET_COMPILE_OPTIONS(PUBLIC libtdloglevel "-pthread")
ENDIF()
TARGET_LINK_LIBRARIES (libtdloglevel libtdtimeutilities libtdthreadsafety ${libtdloglevel_libs} "${CMAKE_THREAD_LIBS_INIT}")

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

SET_TARGET_PROPERTIES(libtdtagitem PROPERTIES
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

SET_TARGET_PROPERTIES(libtdbreakdetector PROPERTIES
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

SET_TARGET_PROPERTIES(libtdthreadsafety PROPERTIES
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

SET_TARGET_PROPERTIES(libtdcppthread PROPERTIES
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

SET_TARGET_PROPERTIES(libtdstorage PROPERTIES
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

SET_TARGET_PROPERTIES(libtdnetutilities PROPERTIES
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

SET_TARGET_PROPERTIES(librsphsmgt PROPERTIES
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

SET_TARGET_PROPERTIES(librspmessaging PROPERTIES
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

SET_TARGET_PROPERTIES(librspdispatcher PROPERTIES
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

SET_TARGET_PROPERTIES(librspcsp PROPERTIES
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

SET_TARGET_PROPERTIES(librsplib PROPERTIES
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

SET_TARGET_PROPERTIES(librspcpprspserver PROPERTIES
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
ADD_EXECUTABLE(rspregistrar rspregistrar.c rspregistrar-global.c rspregistrar-core.c rspregistrar-asap.c rspregistrar-enrp.c rspregistrar-takeover.c rspregistrar-security.c rspregistrar-misc.c takeoverprocess.c)
TARGET_LINK_LIBRARIES(rspregistrar libtdbreakdetector librspdispatcher librspcsp librsphsmgt librspmessaging libtdstorage libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel "${BZIP2_LIBRARIES}" "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             rspregistrar
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES rspregistrar.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(rspserver rspserver.cc standardservices.cc fractalgeneratorservice.cc calcappservice.cc scriptingservice.cc environmentcache.cc memfile.cc sha1.c)
TARGET_LINK_LIBRARIES(rspserver libtdcppthread libtdthreadsafety libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel libtdbreakdetector librspcpprspserver librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             rspserver
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES rspserver.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(FILES scriptingcontrol   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES scriptingcontrol.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(rspterminal rspterminal.c)
TARGET_LINK_LIBRARIES(rspterminal libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             rspterminal
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES rspterminal.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(hsdump hsdump.c)
TARGET_LINK_LIBRARIES(hsdump libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel libtdbreakdetector librsphsmgt librspmessaging "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             hsdump
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES hsdump.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(pingpongclient pingpongclient.c)
TARGET_LINK_LIBRARIES(pingpongclient libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             pingpongclient
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES pingpongclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(scriptingclient scriptingclient.cc sha1.c sha1.h memfile.cc memfile.h)
TARGET_LINK_LIBRARIES(scriptingclient libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             scriptingclient
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES scriptingclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(FILES scriptingserviceexample   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES scriptingserviceexample.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(calcappclient calcappclient.cc calcapppackets.h statistics.cc statistics.h)
TARGET_LINK_LIBRARIES(calcappclient libtdrandomizer libtdstringutilities libtdtimeutilities libtdnetutilities libtdloglevel libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             calcappclient
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES calcappclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(cspmonitor cspmonitor.c)
TARGET_LINK_LIBRARIES(cspmonitor librspcsp libtdbreakdetector libtdtimeutilities libtdstringutilities libtdnetutilities libtdstorage "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             cspmonitor
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES cspmonitor.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)


IF (ENABLE_QT)
   SET(CMAKE_INCLUDE_CURRENT_DIR ON)
   ADD_EXECUTABLE(fractalpooluser fractalpooluser.cc)
   SET_TARGET_PROPERTIES(fractalpooluser PROPERTIES COMPILE_FLAGS "-DDEFAULT_FPU_CONFIGDIR=\\\"\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/fgpconfig\\\"\"")
   TARGET_LINK_LIBRARIES(fractalpooluser libtdrandomizer libtdtimeutilities libtdnetutilities libtdloglevel librsplib Qt5::Widgets Qt5::Xml Qt5::Core "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
   INSTALL(TARGETS             fractalpooluser
           RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
           BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
   INSTALL(FILES fractalpooluser.1                                 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES fractalpooluser_de_DE.qm fractalpooluser_nb_NO.qm DESTINATION ${CMAKE_INSTALL_DATADIR}/fractalpooluser)
ENDIF()


IF (BUILD_TEST_PROGRAMS)

   ADD_EXECUTABLE(hrestest hrestest.c)
   TARGET_LINK_LIBRARIES(hrestest libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(testregistrator testregistrator.c)
   TARGET_LINK_LIBRARIES(testregistrator libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(fork fork.c)
   TARGET_LINK_LIBRARIES(fork libtdtimeutilities libtdrandomizer "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(exectime exectime.cc)
   TARGET_LINK_LIBRARIES(exectime libtdcppthread libtdtimeutilities "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(execalarm execalarm.cc)
   TARGET_LINK_LIBRARIES(execalarm libtdcppthread libtdtimeutilities "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(gettimestamp gettimestamp.c)
   TARGET_LINK_LIBRARIES(gettimestamp libtdtimeutilities "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(rootshell rootshell.c)
   TARGET_LINK_LIBRARIES(rootshell)

   ADD_EXECUTABLE(attacker attacker.cc)
   TARGET_LINK_LIBRARIES(attacker libtdbreakdetector librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

#    ADD_EXECUTABLE(t1 t1.c)
#    TARGET_LINK_LIBRARIES(t1 librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
#
#    ADD_EXECUTABLE(t2 t2.cc environmentcache.cc environmentcache.h sha1.c sha1.h)
#    TARGET_LINK_LIBRARIES(t1 librsplib libcpprspserver libtdthreadsafety "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ENDIF()
