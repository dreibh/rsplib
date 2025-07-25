# --------------------------------------------------------------------------
#
#              //===//   //=====   //===//   //       //   //===//
#             //    //  //        //    //  //       //   //    //
#            //===//   //=====   //===//   //       //   //===<<
#           //   \\         //  //        //       //   //    //
#          //     \\  =====//  //        //=====  //   //===//   Version III
#
# ------------- An Efficient RSerPool Prototype Implementation -------------
#
# Copyright (C) 2002-2025 by Thomas Dreibholz
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
# Contact: thomas.dreibholz@gmail.com

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(rsplib LANGUAGES C CXX)

SET(BUILD_MAJOR "3")
SET(BUILD_MINOR "5")
SET(BUILD_PATCH "3")
SET(BUILD_VERSION ${BUILD_MAJOR}.${BUILD_MINOR}.${BUILD_PATCH})


#############################################################################
#### INSTALLATION_DIRECTORIES                                            ####
#############################################################################

# See: https://cmake.org/cmake/help/v3.0/module/GNUInstallDirs.html
INCLUDE(GNUInstallDirs)


#############################################################################
#### PACKAGING                                                           ####
#############################################################################

SET(CPACK_SOURCE_GENERATOR "TXZ")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${BUILD_MAJOR}.${BUILD_MINOR}.${BUILD_PATCH}")
SET(CPACK_SOURCE_IGNORE_FILES "\\\\.git;\\\\.swp$;~$;\\\\.\\\\#;/\\\\#")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "^${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}[_-]")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "\\\\.cmake$|\\\\.make$|\\\\.log$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/CMakeCache\\\\.txt$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/(CMakeFiles|CMakeScripts|_CPack_Packages)/")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/package-version\\\\.h$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "/packaging\\\\.conf$")
LIST(APPEND CPACK_SOURCE_IGNORE_FILES "^${PROJECT_SOURCE_DIR}/(po.*/|src.*/|)Makefile$")
INCLUDE(CPack)

ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} clean package_source)


#############################################################################
#### UNINSTALL                                                           ####
#############################################################################

IF(NOT TARGET uninstall)
   CONFIGURE_FILE(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
      IMMEDIATE @ONLY)

   ADD_CUSTOM_TARGET(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
ENDIF()


#############################################################################
#### OS-DEPENDENT                                                        ####
#############################################################################

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   ADD_DEFINITIONS("-D_DEFAULT_SOURCE")

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   SET(CMAKE_REQUIRED_INCLUDES "/usr/local/include" "/usr/include")
   SET(CMAKE_LIBRARY_PATH "/usr/local/lib")
   INCLUDE_DIRECTORIES("/usr/local/include")
   SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   SET(CMAKE_REQUIRED_INCLUDES "/usr/local/include" "/usr/include" "/usr/local/opt/openssl/include")
   SET(CMAKE_LIBRARY_PATH "/usr/local/lib")
   INCLUDE_DIRECTORIES("/usr/local/include" "/usr/local/opt/openssl/include")

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
   MESSAGE(STATUS ${CMAKE_SYSTEM_NAME} " supported")
   SET(CMAKE_REQUIRED_INCLUDES "/usr/pkg/include" "/usr/include" "/usr/local/include")
   SET(CMAKE_LIBRARY_PATH "/usr/local/lib")
   INCLUDE_DIRECTORIES("/usr/pkg/include" "/usr/local/include")

ELSE()
   MESSAGE(FATAL_ERROR ${CMAKE_SYSTEM_NAME} " not supported (yet?)")

ENDIF()


#############################################################################
#### CHECK STRUCT MEMBERS                                                ####
#############################################################################

INCLUDE(CheckIncludeFile)
INCLUDE(CheckStructHasMember)

CHECK_STRUCT_HAS_MEMBER("struct sockaddr" "sa_len" "sys/types.h;sys/socket.h" HAVE_SA_LEN)
IF (HAVE_SA_LEN)
   MESSAGE(STATUS "HAVE_SA_LEN")
   ADD_DEFINITIONS(-DHAVE_SA_LEN)
ENDIF()

CHECK_STRUCT_HAS_MEMBER("struct sockaddr_in" "sin_len" "sys/types.h;netinet/in.h" HAVE_SIN_LEN)
IF (HAVE_SIN_LEN)
   MESSAGE(STATUS "HAVE_SIN_LEN")
   ADD_DEFINITIONS(-DHAVE_SIN_LEN)
ENDIF()

CHECK_STRUCT_HAS_MEMBER("struct sockaddr_in6" "sin6_len" "sys/types.h;netinet/in.h" HAVE_SIN6_LEN)
IF (HAVE_SIN6_LEN)
   MESSAGE(STATUS "HAVE_SIN6_LEN")
   ADD_DEFINITIONS(-DHAVE_SIN6_LEN)
ENDIF()

CHECK_STRUCT_HAS_MEMBER("struct sockaddr_storage" "ss_len" "sys/types.h;sys/socket.h" HAVE_SS_LEN)
IF (HAVE_SS_LEN)
   MESSAGE(STATUS "HAVE_SS_LEN")
   ADD_DEFINITIONS(-DHAVE_SS_LEN)
ENDIF()


#############################################################################
#### CHECK OPTIONS                                                       ####
#############################################################################

# Maximum logging level
SET(MAX_LOGLEVEL "9" CACHE STRING "Maximum logging level")
ADD_DEFINITIONS(-DMAX_LOGLEVEL=${MAX_LOGLEVEL})

# Registrar statistics
OPTION(ENABLE_REGISTRAR_STATISTICS "Enable Registrar statistics" 1)
IF (ENABLE_REGISTRAR_STATISTICS)
   ADD_DEFINITIONS(-DENABLE_REGISTRAR_STATISTICS)
ENDIF()

# Handlespace Management verification
OPTION(ENABLE_HSMGTVERIFY   "Enable handlespace management verification" 0)
IF(ENABLE_HSMGTVERIFY)
   ADD_DEFINITIONS(-DVERIFY)
ENDIF()

# Registrar statistics
OPTION(ENABLE_CSP           "Enable CSP" 1)
IF (ENABLE_CSP)
   ADD_DEFINITIONS(-DENABLE_CSP)
ENDIF()

# Qt programs
OPTION(ENABLE_QT            "Include Qt programs" 1)

# # KDE programs
# OPTION(ENABLE_KDE          "Include KDE programs" 0)

# Kernel SCTP
OPTION(USE_KERNEL_SCTP      "Use Kernel SCTP" 1)

# Test programs
OPTION(ENABLE_TEST_PROGRAMS "Build test programs" 0)

# Icons and logo files
OPTION(ENABLE_ICONS         "Build RSPLIB icons and logo files" 1)


# Handlespace management storage implementation
ADD_DEFINITIONS(-DINCLUDE_SIMPLEREDBLACKTREE -DUSE_SIMPLEREDBLACKTREE)


#############################################################################
#### REQUIREMENTS                                                        ####
#############################################################################

# ====== Valgrind ===========================================================
CHECK_INCLUDE_FILE("valgrind/memcheck.h" HAVE_VALGRIND_MEMCHECK_H)
IF(HAVE_VALGRIND_MEMCHECK_H)
   ADD_DEFINITIONS(-DHAVE_VALGRIND_MEMCHECK_H)
ENDIF()


# ====== Threads ============================================================
FIND_PACKAGE(Threads REQUIRED)


# ====== Qt =================================================================
IF (ENABLE_QT OR ENABLE_KDE)
   SET(CMAKE_AUTOMOC ON)
   FIND_PACKAGE(QT NAMES Qt6 COMPONENTS Core Network Widgets Xml)
   IF (NOT QT_FOUND)
      FIND_PACKAGE(QT NAMES Qt5 REQUIRED COMPONENTS Core Network Widgets Xml)
   ENDIF()
   MESSAGE("Using Qt version ${QT_VERSION_MAJOR}!")
   FIND_PACKAGE(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Widgets Xml)
ENDIF()


# # ====== KDE ================================================================
# IF (ENABLE_KDE)
#    # Needed:
#    # KDE5: libkf5coreaddons-dev libkf5xmlgui-dev
#    # KDE6: libkf6coreaddons-dev libkf6xmlgui-dev
#
#    FIND_PACKAGE(ECM "1.0.0" NO_MODULE REQUIRED)
#    SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
#
#    INCLUDE(KDEInstallDirs)
#    INCLUDE(KDECMakeSettings)
#    INCLUDE(KDECompilerSettings NO_POLICY_SCOPE)
#    INCLUDE(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
#
#    FIND_PACKAGE(KF${QT_VERSION_MAJOR} "${QT_VERSION_MAJOR}.0.0" REQUIRED COMPONENTS CoreAddons WidgetsAddons)
# ENDIF()


# ====== SCTP ==============================================================#
IF (USE_KERNEL_SCTP)
   # ====== Kernel SCTP =====================================================
   CHECK_INCLUDE_FILE(netinet/sctp.h HAVE_NETINET_SCTP_H)
   IF (HAVE_NETINET_SCTP_H)
      SET(SCTP_INCLUDE "netinet/sctp.h")
      ADD_DEFINITIONS(-DHAVE_KERNEL_SCTP -DHAVE_NETINET_SCTP_H)
      IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
         FIND_LIBRARY(SCTP_LIB sctp REQUIRED)
      ELSE()
         FIND_LIBRARY(SCTP_LIB sctp)
      ENDIF()
      IF (SCTP_LIB)
         SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${SCTP_LIB})
      ELSE()
         SET(SCTP_LIB "")
      ENDIF()
   ENDIF()

ELSE()
   # ====== SCTPLIB User-Space SCTP =========================================
   CHECK_INCLUDE_FILE(sctp.h HAVE_SCTPLIB_SCTP_H)
   FIND_LIBRARY(SCTPLIB_LIB sctplib)
   IF (NOT HAVE_SCTPLIB_SCTP_H OR NOT SCTPLIB_LIB)
      MESSAGE(FATAL_ERROR "sctplib not found - libsctplib installed?")
   ENDIF()
   MESSAGE(STATUS "sctplib found: ${SCTPLIB_LIB}")

   CHECK_INCLUDE_FILE(sctp.h HAVE_SCTPSOCKET_SCTP_H)
   FIND_LIBRARY(SCTPSOCKET_LIB sctpsocket)
   IF (NOT HAVE_SCTPSOCKET_SCTP_H OR NOT SCTPSOCKET_LIB)
      MESSAGE(FATAL_ERROR "sctpsocket not found - libsctpsocket installed?")
   ENDIF()
   MESSAGE(STATUS "sctpsocket found: ${SCTPSOCKET_LIB}")

   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(GLIB glib-2.0)
   IF (NOT ${GLIB_LIBS})
      MESSAGE(FATAL_ERROR "glib not found - glib-2.0 installed?")
   ENDIF()
   MESSAGE(STATUS "glib found: ${GLIB_LIBRARIES}")

   SET(SCTP_LIB ${SCTPSOCKET_LIB} ${SCTPLIB_LIB} ${GLIB_LIBRARIES})
   SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${SCTPSOCKET_LIB} ${SCTPLIB_LIB} ${GLIB_LIBRARIES})
ENDIF()


# ====== Graphics tools for icons and logo ==================================
IF (ENABLE_ICONS)
   FIND_PROGRAM(GS gs PATHS /bin /usr/bin /usr/local/bin)
   IF(NOT GS)
      MESSAGE(FATAL_ERROR
            " Cannot find GhostScript! Try:\n"
            " * Ubuntu/Debian: sudo apt install -y ghostscript\n"
            " * Fedora:        sudo dnf install -y ghostscript\n"
            " * FreeBSD:       sudo pkg install -y ghostscript10")
   ENDIF()

   FIND_PROGRAM(PDF2SVG pdf2svg PATHS /bin /usr/bin /usr/local/bin)
   IF(NOT PDF2SVG)
      MESSAGE(FATAL_ERROR
            " Cannot find PDF2SVG! Try:\n"
            " * Ubuntu/Debian: sudo apt install -y pdf2svg\n"
            " * Fedora:        sudo dnf install -y pdf2svg\n"
            " * FreeBSD:       sudo pkg install -y pdf2svg")
   ENDIF()

   FIND_PROGRAM(GM gm PATHS /bin /usr/bin /usr/local/bin)
   IF(NOT GM)
      MESSAGE(FATAL_ERROR
            " Cannot find GraphicsMagick! Try:\n"
            " * Ubuntu/Debian: sudo apt install -y graphicsmagick\n"
            " * Fedora:        sudo dnf install -y GraphicsMagick\n"
            " * FreeBSD:       sudo pkg install -y GraphicsMagick")
   ENDIF()
ENDIF()


# ====== Functions ==========================================================
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)

CHECK_FUNCTION_EXISTS(sctp_send HAVE_SCTP_SEND)
IF (HAVE_SCTP_SEND)
   ADD_DEFINITIONS(-DHAVE_SCTP_SEND)
ENDIF()

CHECK_FUNCTION_EXISTS(sctp_sendx HAVE_SCTP_SENDX)
IF (HAVE_SCTP_SENDX)
   ADD_DEFINITIONS(-DHAVE_SCTP_SENDX)
ENDIF()

CHECK_FUNCTION_EXISTS(sctp_sendmsg HAVE_SCTP_SENDMSG)
IF (HAVE_SCTP_SENDMSG)
   ADD_DEFINITIONS(-DHAVE_SCTP_SENDMSG)
ENDIF()

CHECK_FUNCTION_EXISTS(sctp_connectx HAVE_SCTP_CONNECTX)
IF (HAVE_SCTP_CONNECTX)
   ADD_DEFINITIONS(-DHAVE_SCTP_CONNECTX)
ENDIF()

IF (USE_KERNEL_SCTP)
   CHECK_SYMBOL_EXISTS(SCTP_DELAYED_SACK "netinet/sctp.h" HAVE_SCTP_DELAYED_SACK)
ELSE()
   CHECK_SYMBOL_EXISTS(SCTP_DELAYED_SACK "sctp.h" HAVE_SCTP_DELAYED_SACK)
ENDIF()
IF (HAVE_SCTP_DELAYED_SACK)
   ADD_DEFINITIONS(-DHAVE_SCTP_DELAYED_SACK)
ENDIF()

ADD_DEFINITIONS(-D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64)


# ====== BZip2 ==============================================================
FIND_PACKAGE(BZip2 REQUIRED)


# ====== IEEE Floating-Point ================================================
INCLUDE(CheckCSourceRuns)
CHECK_C_SOURCE_RUNS("
#include <stdio.h>

union DoubleIntUnion
{
   double             Double;
   unsigned long long Integer;
};

int main (int argc, char** argv)
{
   union DoubleIntUnion value;

   value.Double = 0.0;
   if(value.Integer != 0ULL) {
      return(1);
   }
   value.Double = 1000.5;
   if(value.Integer != 0x408f440000000000ULL) {
      return(1);
   }
   value.Double = -1000.5;
   if(value.Integer != 0xc08f440000000000ULL) {
      return(1);
   }

   return(0);
}"
USES_IEEEFP_FP)

IF (USES_IEEEFP_FP)
   ADD_DEFINITIONS(-DHAVE_IEEE_FP)
ENDIF()


# ====== Atomic =============================================================
# Based on: https://stackoverflow.com/questions/69281559/cmake-library-order-in-cmake-required-libraries-to-test-a-minimal-program-while
INCLUDE(CheckCSourceCompiles)
SET(ATOMIC_TEST_CODE "
#include <stdatomic.h>
#include <stdint.h>

void main()
{
   volatile _Atomic uint32_t a;
   volatile _Atomic uint64_t b;
   a = 1;
   b = 2;
}
")
CHECK_C_SOURCE_COMPILES("${ATOMIC_TEST_CODE}" atomic_test)
IF (atomic_test)
   MESSAGE("No need for libatomic!")
   SET(ATOMIC_LIB "")
ELSE()
   MESSAGE("Searching for libatomic!")
   FIND_LIBRARY(ATOMIC_LIB
                REQUIRED
                NAMES libatomic.so.1
                      libatomic.so
                HINTS /usr/local/lib64
                      /usr/local/lib
                      /usr/lib64
                      /usr/lib
                      /opt/local/lib64
                      /opt/local/lib
                      /lib64
                      /lib)
   IF (ATOMIC_LIB)
      GET_FILENAME_COMPONENT(atomic_lib_dir ${ATOMIC_LIB} DIRECTORY)
      CMAKE_PUSH_CHECK_STATE()
      SET(CMAKE_REQUIRED_LIBRARIES "-L${atomic_lib_dir} -latomic")
      CHECK_C_SOURCE_COMPILES("${ATOMIC_TEST_CODE}" atomic_test_with_library)
      CMAKE_POP_CHECK_STATE()
      IF(NOT atomic_test_with_library)
         MESSAGE(FATAL_ERROR "stdatomic compilation failed, even with libatomic!")
      ENDIF()
      MESSAGE("Using ${ATOMIC_LIB}!")
   ENDIF()
ENDIF()


#############################################################################
#### SOURCE DIRECTORIES                                                  ####
#############################################################################

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(docs)
