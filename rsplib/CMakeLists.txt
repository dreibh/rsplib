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
endif()
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
endif()
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
endif()
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
endif()
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
endif()
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
endif()
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
endif()

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
endif()
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
endif()

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
endif()
TARGET_LINK_LIBRARIES (libtdnetutilities libtdrandomizer libtdstringutilities libtdtimeutilities libtdloglevel ${libtdnetutilities_libs} "${CMAKE_THREAD_LIBS_INIT}")

INSTALL(TARGETS libtdnetutilities libtdnetutilities-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${libtdnetutilities_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})







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
# IF (WITH_QT)
#    ADD_EXECUTABLE(rtpa-qclient rtpa-qclient.cc qaudiomixer.cc qinfotabwidget.cc qspectrumanalyzer.cc)
#    qt5_use_modules(rtpa-qclient Core Widgets)
#    TARGET_LINK_LIBRARIES(rtpa-qclient rtpaudioclient audiodecoder audiowriter libtdloglevel libtdtimeutilities "${CMAKE_THREAD_LIBS_INIT}")
#    INSTALL(TARGETS             rtpa-qclient
#          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#          BUNDLE DESTINATION  ${CMAKE_INSTALL_BINDIR})
#    INSTALL(FILES rtpa-qclient.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
# ENDIF()
