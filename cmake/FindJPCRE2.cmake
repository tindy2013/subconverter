# - Find JPCRE2
# Find the JPCRE2 headers.
#
#  JPCRE2_INCLUDE_DIRS - where to find jpcre2.h.
#  JPCRE2_FOUND        - True if JPCRE2 found.

# Look for the header file.
FIND_PATH(JPCRE2_INCLUDE_DIR NAMES jpcre2.hpp)
MARK_AS_ADVANCED(JPCRE2_INCLUDE_DIR)

if(JPCRE2_INCLUDE_DIR)
  set(JPCRE2_FOUND TRUE)
endif()

IF(JPCRE2_FOUND)
  SET(JPCRE2_INCLUDE_DIRS ${JPCRE2_INCLUDE_DIR})
ENDIF(JPCRE2_FOUND)

if(JPCRE2_FOUND)
  if(NOT JPCRE2_FIND_QUIETLY)
    message(STATUS "Found JPCRE2 header files in ${JPCRE2_INCLUDE_DIRS}")
  endif()
elseif(JPCRE2_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find JPCRE2")
else()
  message(STATUS "Optional package JPCRE2 was not found")
endif()
