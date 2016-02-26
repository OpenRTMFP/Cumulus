CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(Cumulus)

LIST(APPEND CMAKE_CXX_FLAGS " -DCUMULUS_LOGS")

FIND_PACKAGE(PkgConfig)

FIND_LIBRARY(POCO_NET PocoNet)
FIND_LIBRARY(POCO_UTIL  PocoUtil)
FIND_LIBRARY(POCO_XML PocoXML)
FIND_LIBRARY(POCO_FOUNDATION PocoFoundation)
FIND_LIBRARY(OPENSSL_LIBRARY ssl)
PKG_CHECK_MODULES(LUAJIT REQUIRED luajit)

ADD_SUBDIRECTORY(CumulusLib)
ADD_SUBDIRECTORY(CumulusServer)

MESSAGE(STATUS "")
MESSAGE(STATUS "")
MESSAGE(STATUS "${PROJECT_NAME} configuration summary:")
MESSAGE(STATUS "")
MESSAGE(STATUS "  Install prefix .................. : " ${CMAKE_INSTALL_PREFIX})
MESSAGE(STATUS "  C compiler ...................... : " ${CMAKE_C_COMPILER})
MESSAGE(STATUS "  C++ compiler .................... : " ${CMAKE_CXX_COMPILER})
MESSAGE(STATUS "  C compiler flags ................ : " ${CMAKE_C_FLAGS})
MESSAGE(STATUS "  C++ compiler flags .............. : " ${CMAKE_CXX_FLAGS})
MESSAGE(STATUS "")
MESSAGE(STATUS "  Poco Net ........................ : " ${POCO_NET})
MESSAGE(STATUS "  Poco Utils ...................... : " ${POCO_UTIL})
MESSAGE(STATUS "  Poco XML ........................ : " ${POCO_XML})
MESSAGE(STATUS "  Poco Foundation ................. : " ${POCO_FOUNDATION})
MESSAGE(STATUS "  SLL ........ .................... : " ${OPENSSL_LIBRARY})
MESSAGE(STATUS "  LUAJIT (version) ................ : " ${LUAJIT_VERSION})
MESSAGE(STATUS "")

# vim:ft=cmake
