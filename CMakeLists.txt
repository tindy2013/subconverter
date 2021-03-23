project(subconverter LANGUAGES CXX)
cmake_minimum_required(VERSION 3.4)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include/")
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()
SET(CMAKE_CXX_STANDARD 17)


IF (NOT MSVC)
    ADD_COMPILE_OPTIONS(-Wall -Wextra -Wno-unused-parameter -Wno-unused-result)
ELSE()
    ADD_COMPILE_OPTIONS(/W4)
ENDIF()

#remove std::regex support since it is not compatible with group modifiers and slow
#OPTION(USING_STD_REGEX "Use std::regex from C++ library instead of PCRE2." OFF)
OPTION(USING_MALLOC_TRIM "Call malloc_trim after processing request to lower memory usage (Your system must support malloc_trim)." OFF)
#now using internal MD5 calculation
#OPTION(USING_MBEDTLS "Use mbedTLS instead of OpenSSL for MD5 calculation." OFF)

INCLUDE(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES(
"
#include<string>
void main(){std::to_string(0);}
" HAVE_TO_STRING)

IF(HAVE_TO_STRING)
    ADD_DEFINITIONS(-DHAVE_TO_STRING)
ENDIF()

IF(USING_MALLOC_TRIM)
    ADD_DEFINITIONS(-DMALLOC_TRIM)
ENDIF()

ADD_EXECUTABLE(subconverter 
    src/generator/config/nodemanip.cpp
    src/generator/config/ruleconvert.cpp
    src/generator/config/subexport.cpp
    src/generator/template/templates.cpp
    src/handler/interfaces.cpp
    src/handler/multithread.cpp
    src/handler/upload.cpp
    src/handler/webget.cpp
    src/main.cpp
    src/parser/infoparser.cpp
    src/parser/subparser.cpp
    src/script/cron.cpp
    src/script/script_quickjs.cpp
    src/server/webserver_libevent.cpp
    src/utils/base64/base64.cpp
    src/utils/codepage.cpp
    src/utils/file.cpp
    src/utils/logger.cpp
    src/utils/md5/md5.cpp
    src/utils/network.cpp
    src/utils/regexp.cpp
    src/utils/string.cpp
    src/utils/system.cpp
    src/utils/urlencode.cpp)
INCLUDE_DIRECTORIES(src)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR})

FIND_PACKAGE(PkgConfig REQUIRED)

SET(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)
TARGET_LINK_LIBRARIES(subconverter ${CMAKE_THREAD_LIBS_INIT})

PKG_CHECK_MODULES(LIBEVENT libevent>=2.1.10 REQUIRED)
FIND_PATH(LIBEVENT_INCLUDE_DIR NAMES event.h PATHS ${LIBEVENT_INCLUDE_DIRS})
FIND_LIBRARY(LIBEVENT_LIBRARY NAMES event PATHS ${LIBEVENT_LIBRARY_DIRS})
LINK_DIRECTORIES(${LIBEVENT_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${LIBEVENT_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(subconverter ${LIBEVENT_LIBRARY})

FIND_PACKAGE(CURL 7.54.0 REQUIRED)
LINK_DIRECTORIES(${CURL_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(subconverter CURL::libcurl)
ADD_DEFINITIONS(-DCURL_STATICLIB)

FIND_PACKAGE(Rapidjson REQUIRED)
INCLUDE_DIRECTORIES(${RAPIDJSON_INCLUDE_DIRS})

PKG_CHECK_MODULES(YAML_CPP yaml-cpp>=0.6.3 REQUIRED)
FIND_LIBRARY(YAML_CPP_LIBRARY NAMES yaml-cpp yaml-cppd PATHS ${YAML_CPP_LIBRARY_DIRS})
LINK_DIRECTORIES(${YAML_CPP_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${YAML_CPP_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(subconverter ${YAML_CPP_LIBRARY})

#IF(USING_STD_REGEX STREQUAL "ON")
#    ADD_DEFINITIONS(-DUSE_STD_REGEX)
#ELSE()
    FIND_PACKAGE(PCRE2 REQUIRED)
    INCLUDE_DIRECTORIES(${PCRE2_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(subconverter ${PCRE2_LIBRARY})
    ADD_DEFINITIONS(-DPCRE2_STATIC)
#ENDIF()

FIND_PACKAGE(QuickJS REQUIRED)
INCLUDE_DIRECTORIES(${QUICKJS_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(subconverter ${QUICKJS_LIBRARIES})

FIND_PACKAGE(LibCron REQUIRED)
INCLUDE_DIRECTORIES(${LIBCRON_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(subconverter ${LIBCRON_LIBRARIES})

IF(WIN32)
    TARGET_LINK_LIBRARIES(subconverter wsock32 ws2_32)
ELSE()
    INCLUDE(GNUInstallDirs)
    INSTALL(TARGETS subconverter DESTINATION ${CMAKE_INSTALL_BINDIR}/subconverter)
    INSTALL(DIRECTORY base/ DESTINATION ${CMAKE_INSTALL_BINDIR}/subconverter FILES_MATCHING PATTERN "*")
ENDIF()
