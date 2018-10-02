INCLUDE_DIRECTORIES(.)

# ###### Libraries ##########################################################

# ====== liblibtdtimeutilities ==============================================
LIST(APPEND libtdtimeutilities_headers
   timeutilities.h
)
LIST(APPEND libtdtimeutilities_sources
   timeutilities.c
)

INSTALL(FILES ${libtdtimeutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdtimeutilities-${TYPE} SHARED ${libtdtimeutilities_sources})

   SET_TARGET_PROPERTIES(libtdtimeutilities-${TYPE} PROPERTIES OUTPUT_NAME tdtimeutilities CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdtimeutilities-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}" m)

   INSTALL(TARGETS libtdtimeutilities-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdtimeutilities-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== liblibtdrandomizer =================================================
LIST(APPEND libtdrandomizer_headers
   randomizer.h
)
LIST(APPEND libtdrandomizer_sources
   randomizer.c
)

INSTALL(FILES ${libtdrandomizer_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdrandomizer-${TYPE} SHARED ${libtdrandomizer_sources})

   SET_TARGET_PROPERTIES(libtdrandomizer-${TYPE} PROPERTIES OUTPUT_NAME tdrandomizer CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdrandomizer-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdrandomizer-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdrandomizer-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== liblibtdstringutilities ============================================
LIST(APPEND libtdstringutilities_headers
   stringutilities.h
)
LIST(APPEND libtdstringutilities_sources
   stringutilities.c
)

INSTALL(FILES ${libtdstringutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdstringutilities-${TYPE} SHARED ${libtdstringutilities_sources})

   SET_TARGET_PROPERTIES(libtdstringutilities-${TYPE} PROPERTIES OUTPUT_NAME tdstringutilities CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdstringutilities-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdstringutilities-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdstringutilities-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== libtdthreadsafety ======================================================
LIST(APPEND libtdthreadsafety_headers
   threadsafety.h threadsignal.h
)
LIST(APPEND libtdthreadsafety_sources
   threadsafety.c threadsignal.c
)

INSTALL(FILES ${libtdthreadsafety_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdthreadsafety-${TYPE} SHARED ${libtdthreadsafety_sources})

   SET_TARGET_PROPERTIES(libtdthreadsafety-${TYPE} PROPERTIES OUTPUT_NAME tdthreadsafety CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdthreadsafety-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdthreadsafety-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdthreadsafety-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== libtdloglevel ======================================================
LIST(APPEND libtdloglevel_headers
   loglevel.h
)
LIST(APPEND libtdloglevel_sources
   loglevel.c
)

INSTALL(FILES ${libtdloglevel_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdloglevel-${TYPE} SHARED ${libtdloglevel_sources})

   SET_TARGET_PROPERTIES(libtdloglevel-${TYPE} PROPERTIES OUTPUT_NAME tdloglevel CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdloglevel-${TYPE} libtdtimeutilities-${TYPE} libtdthreadsafety-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdloglevel-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdloglevel-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== libtdtagitem =======================================================
LIST(APPEND libtdtagitem_headers
   tagitem.h
)
LIST(APPEND libtdtagitem_sources
   tagitem.c
)

INSTALL(FILES ${libtdtagitem_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdtagitem-${TYPE} SHARED ${libtdtagitem_sources})

   SET_TARGET_PROPERTIES(libtdtagitem-${TYPE} PROPERTIES OUTPUT_NAME tdtagitem CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdtagitem-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdtagitem-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdtagitem-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== libtdbreakdetector =================================================
LIST(APPEND libtdbreakdetector_headers
   breakdetector.h
)
LIST(APPEND libtdbreakdetector_sources
   breakdetector.c
)

INSTALL(FILES ${libtdbreakdetector_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdbreakdetector-${TYPE} SHARED ${libtdbreakdetector_sources})

   SET_TARGET_PROPERTIES(libtdbreakdetector-${TYPE} PROPERTIES OUTPUT_NAME tdbreakdetector CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdbreakdetector-${TYPE} libtdtimeutilities-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdbreakdetector-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdbreakdetector-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== libtdcppthread =====================================================
LIST(APPEND libtdcppthread_headers
   mutex.h thread.h
)
LIST(APPEND libtdcppthread_sources
   mutex.cc thread.cc
)

INSTALL(FILES ${libtdcppthread_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdcppthread-${TYPE} SHARED ${libtdcppthread_sources})

   SET_TARGET_PROPERTIES(libtdcppthread-${TYPE} PROPERTIES OUTPUT_NAME tdcppthread CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdcppthread-${TYPE} libtdthreadsafety-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdcppthread-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdcppthread-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


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

INSTALL(FILES ${libtdstorage_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdstorage-${TYPE} SHARED ${libtdstorage_sources})

   SET_TARGET_PROPERTIES(libtdstorage-${TYPE} PROPERTIES OUTPUT_NAME tdstorage CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdstorage-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdstorage-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdstorage-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


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

INSTALL(FILES ${libtdnetutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libtdnetutilities-${TYPE} SHARED ${libtdnetutilities_sources})

   SET_TARGET_PROPERTIES(libtdnetutilities-${TYPE} PROPERTIES OUTPUT_NAME tdnetutilities CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libtdnetutilities-${TYPE} libtdrandomizer-${TYPE} libtdstringutilities-${TYPE} libtdtimeutilities-${TYPE} libtdloglevel-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libtdnetutilities-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libtdnetutilities-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


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

INSTALL(FILES ${librsphsmgt_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(librsphsmgt-${TYPE} SHARED ${librsphsmgt_sources})

   SET_TARGET_PROPERTIES(librsphsmgt-${TYPE} PROPERTIES OUTPUT_NAME rsphsmgt CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (librsphsmgt-${TYPE} libtdstorage-${TYPE} libtdrandomizer-${TYPE} libtdstringutilities-${TYPE} libtdnetutilities-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS librsphsmgt-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(librsphsmgt-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


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

INSTALL(FILES ${librspmessaging_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(librspmessaging-${TYPE} SHARED ${librspmessaging_sources})

   SET_TARGET_PROPERTIES(librspmessaging-${TYPE} PROPERTIES OUTPUT_NAME rspmessaging CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (librspmessaging-${TYPE} libtdnetutilities-${TYPE} libtdtimeutilities-${TYPE} librsphsmgt-${TYPE} libtdloglevel-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS librspmessaging-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(librspmessaging-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


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

INSTALL(FILES ${librspdispatcher_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(librspdispatcher-${TYPE} SHARED ${librspdispatcher_sources})
   SET_TARGET_PROPERTIES(librspdispatcher-${TYPE} PROPERTIES
      VERSION   ${BUILD_VERSION}
      SOVERSION ${BUILD_MAJOR}
   )

   SET_TARGET_PROPERTIES(librspdispatcher-shared PROPERTIES OUTPUT_NAME rspdispatcher CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (librspdispatcher-${TYPE} libtdtimeutilities-${TYPE} libtdloglevel-${TYPE} libtdnetutilities-${TYPE} libtdstorage-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS librspdispatcher-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()


# ====== librspcsp ==========================================================
IF (ENABLE_CSP)
   LIST(APPEND librspcsp_headers
      componentstatuspackets.h
      componentstatusreporter.h
   )
   LIST(APPEND librspcsp_sources
      componentstatusreporter.c
   )

   INSTALL(FILES ${librspcsp_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

   FOREACH(TYPE shared;static)
      ADD_LIBRARY(librspcsp-${TYPE} SHARED ${librspcsp_sources})

      SET_TARGET_PROPERTIES(librspcsp-${TYPE} PROPERTIES OUTPUT_NAME rspcsp CLEAN_DIRECT_OUTPUT 1)
      TARGET_LINK_LIBRARIES (librspcsp-${TYPE} libtdtimeutilities-${TYPE} libtdstringutilities-${TYPE} libtdloglevel-${TYPE} librspdispatcher-${TYPE} libtdnetutilities-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

      INSTALL(TARGETS librspcsp-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
   ENDFOREACH()
ENDIF()

SET_TARGET_PROPERTIES(librspcsp-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


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

INSTALL(FILES ${librsplib_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(librsplib-${TYPE} SHARED ${librsplib_sources})

   SET_TARGET_PROPERTIES(librsplib-${TYPE} PROPERTIES OUTPUT_NAME rsplib CLEAN_DIRECT_OUTPUT 1)
   IF (ENABLE_CSP)
      TARGET_LINK_LIBRARIES (librsplib-${TYPE} libtdrandomizer-${TYPE} libtdtimeutilities-${TYPE} libtdstringutilities-${TYPE} libtdnetutilities-${TYPE} libtdthreadsafety-${TYPE} libtdloglevel-${TYPE} libtdtagitem-${TYPE} libtdstorage-${TYPE} librsphsmgt-${TYPE} librspdispatcher-${TYPE} librspmessaging-${TYPE} librspcsp-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")
   ELSE()
      TARGET_LINK_LIBRARIES (librsplib-${TYPE} libtdrandomizer-${TYPE} libtdtimeutilities-${TYPE} libtdstringutilities-${TYPE} libtdnetutilities-${TYPE} libtdthreadsafety-${TYPE} libtdloglevel-${TYPE} libtdtagitem-${TYPE} libtdstorage-${TYPE} librsphsmgt-${TYPE} librspdispatcher-${TYPE} librspmessaging-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")
   ENDIF()

   INSTALL(TARGETS librsplib-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(librsplib-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ====== libcpprspserver =================================================
LIST(APPEND libcpprspserver_headers
   cpprspserver.h
   tcplikeserver.h
   udplikeserver.h
)
LIST(APPEND libcpprspserver_sources
   tcplikeserver.cc
   udplikeserver.cc
)

INSTALL(FILES ${libcpprspserver_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

FOREACH(TYPE shared;static)
   ADD_LIBRARY(libcpprspserver-${TYPE} SHARED ${libcpprspserver_sources})

   SET_TARGET_PROPERTIES(libcpprspserver-${TYPE} PROPERTIES OUTPUT_NAME cpprspserver CLEAN_DIRECT_OUTPUT 1)
   TARGET_LINK_LIBRARIES (libcpprspserver-${TYPE} libtdcppthread-${TYPE} librsplib-${TYPE} libtdthreadsafety-${TYPE} libtdnetutilities-${TYPE} libtdtimeutilities-${TYPE} libtdloglevel-${TYPE} libtdbreakdetector-${TYPE} ${SCTP_LIB} "${CMAKE_THREAD_LIBS_INIT}")

   INSTALL(TARGETS libcpprspserver-${TYPE} DESTINATION ${CMAKE_INSTALL_LIBDIR})
ENDFOREACH()

SET_TARGET_PROPERTIES(libcpprspserver-shared PROPERTIES
   VERSION   ${BUILD_VERSION}
   SOVERSION ${BUILD_MAJOR}
)


# ###### Programs ###########################################################
ADD_EXECUTABLE(rspregistrar rspregistrar.c rspregistrar-global.c rspregistrar-core.c rspregistrar-asap.c rspregistrar-enrp.c rspregistrar-takeover.c rspregistrar-security.c rspregistrar-misc.c takeoverprocess.c)
IF (ENABLE_CSP)
    TARGET_LINK_LIBRARIES(rspregistrar libtdbreakdetector-shared librspdispatcher-shared librspcsp-shared librsphsmgt-shared librspmessaging-shared libtdstorage-shared libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared "${BZIP2_LIBRARIES}" "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
ELSE()
    TARGET_LINK_LIBRARIES(rspregistrar libtdbreakdetector-shared librspdispatcher-shared librsphsmgt-shared librspmessaging-shared libtdstorage-shared libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared "${BZIP2_LIBRARIES}" "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
ENDIF()
INSTALL(TARGETS             rspregistrar
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES rspregistrar.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(rspserver rspserver.cc standardservices.cc fractalgeneratorservice.cc calcappservice.cc scriptingservice.cc environmentcache.cc memfile.cc sha1.c)
TARGET_LINK_LIBRARIES(rspserver libtdcppthread-shared libtdthreadsafety-shared libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared libtdbreakdetector-shared libcpprspserver-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             rspserver
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES rspserver.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(FILES scriptingcontrol   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES scriptingcontrol.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(rspterminal rspterminal.c)
TARGET_LINK_LIBRARIES(rspterminal libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             rspterminal
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES rspterminal.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(hsdump hsdump.c)
TARGET_LINK_LIBRARIES(hsdump libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared libtdbreakdetector-shared librsphsmgt-shared librspmessaging-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             hsdump
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES hsdump.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(pingpongclient pingpongclient.c)
TARGET_LINK_LIBRARIES(pingpongclient libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             pingpongclient
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES pingpongclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(scriptingclient scriptingclient.cc sha1.c sha1.h memfile.cc memfile.h)
TARGET_LINK_LIBRARIES(scriptingclient libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             scriptingclient
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES scriptingclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

INSTALL(FILES scriptingserviceexample   DESTINATION ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES scriptingserviceexample.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

ADD_EXECUTABLE(calcappclient calcappclient.cc calcapppackets.h statistics.cc statistics.h)
TARGET_LINK_LIBRARIES(calcappclient libtdrandomizer-shared libtdstringutilities-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
INSTALL(TARGETS             calcappclient
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
INSTALL(FILES calcappclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)


IF (ENABLE_CSP)
    ADD_EXECUTABLE(cspmonitor cspmonitor.c)
    TARGET_LINK_LIBRARIES(cspmonitor librspcsp-shared libtdbreakdetector-shared libtdtimeutilities-shared libtdstringutilities-shared libtdnetutilities-shared libtdstorage-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
    INSTALL(TARGETS             cspmonitor
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
    INSTALL(FILES cspmonitor.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
ENDIF()


IF (ENABLE_QT)
   SET(CMAKE_INCLUDE_CURRENT_DIR ON)
   ADD_EXECUTABLE(fractalpooluser fractalpooluser.cc)
   SET_TARGET_PROPERTIES(fractalpooluser PROPERTIES COMPILE_FLAGS "-DDEFAULT_FPU_CONFIGDIR=\\\"\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/fgpconfig\\\"\"")
   TARGET_LINK_LIBRARIES(fractalpooluser libtdrandomizer-shared libtdtimeutilities-shared libtdnetutilities-shared libtdloglevel-shared librsplib-shared Qt5::Widgets Qt5::Xml Qt5::Core "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
   INSTALL(TARGETS             fractalpooluser
           RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
           BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
   INSTALL(FILES fractalpooluser.1                                 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
   INSTALL(FILES fractalpooluser_de_DE.qm fractalpooluser_nb_NO.qm DESTINATION ${CMAKE_INSTALL_DATADIR}/fractalpooluser)
ENDIF()


IF (BUILD_TEST_PROGRAMS)

   ADD_EXECUTABLE(hrestest hrestest.c)
   TARGET_LINK_LIBRARIES(hrestest libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(testregistrator testregistrator.c)
   TARGET_LINK_LIBRARIES(testregistrator libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(fork fork.c)
   TARGET_LINK_LIBRARIES(fork libtdtimeutilities-shared libtdrandomizer-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(exectime exectime.cc)
   TARGET_LINK_LIBRARIES(exectime libtdcppthread-shared libtdtimeutilities-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(execalarm execalarm.cc)
   TARGET_LINK_LIBRARIES(execalarm libtdcppthread-shared libtdtimeutilities-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(gettimestamp gettimestamp.c)
   TARGET_LINK_LIBRARIES(gettimestamp libtdtimeutilities-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ADD_EXECUTABLE(rootshell rootshell.c)
   TARGET_LINK_LIBRARIES(rootshell)

   ADD_EXECUTABLE(attacker attacker.cc)
   TARGET_LINK_LIBRARIES(attacker libtdbreakdetector-shared librsplib-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

#    ADD_EXECUTABLE(t1 t1.c)
#    TARGET_LINK_LIBRARIES(t1 librsplib "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")
#
#    ADD_EXECUTABLE(t2 t2.cc environmentcache.cc environmentcache.h sha1.c sha1.h)
#    TARGET_LINK_LIBRARIES(t1 librsplib libcpprspserver-shared libtdthreadsafety-shared "${SCTP_LIB}" "${CMAKE_THREAD_LIBS_INIT}")

   ENDIF()
