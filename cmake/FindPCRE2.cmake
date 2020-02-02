#
#
# Locate pcre2
#
# This module accepts the following environment variables:
#
#    PCRE2_DIR or PCRE2_ROOT - Specify the location of PCRE2
#
# This module defines the following CMake variables:
#
#    PCRE2_FOUND - True if libpcre2 is found
#    PCRE2_LIBRARY - A variable pointing to the PCRE2 library
#    PCRE2_INCLUDE_DIR - Where to find the headers

#=============================================================================
# Inspired by FindGDAL
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See COPYING-CMAKE-SCRIPTS for more information.
#=============================================================================

# This makes the presumption that you are include pcre2.h like
#
#include "pcre2.h"

if (DEFINED PCRE2_ROOT AND NOT PCRE2_ROOT)
	set (PCRE2_LIBRARY "" CACHE INTERNAL "")
	set (PCRE2_INCLUDE_DIR "" CACHE INTERNAL "")
	return ()
endif (DEFINED PCRE2_ROOT AND NOT PCRE2_ROOT)

if (UNIX AND NOT PCRE2_FOUND)
	# Use pcre2-config to obtain the library location and name, something like
	# -L/sw/lib -lpcre2-8)
	find_program (PCRE2_CONFIG pcre2-config
		HINTS
		${PCRE2_DIR}
		${PCRE2_ROOT}
		$ENV{PCRE2_DIR}
		$ENV{PCRE2_ROOT}
		PATH_SUFFIXES bin
		PATHS
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
		/usr/local
	)

	if (PCRE2_CONFIG)
		execute_process (COMMAND ${PCRE2_CONFIG} --cflags
			ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
			OUTPUT_VARIABLE PCRE2_CONFIG_CFLAGS)
		if (PCRE2_CONFIG_CFLAGS)
			string (REGEX MATCHALL "-I[^ ]+" _pcre2_dashI ${PCRE2_CONFIG_CFLAGS})
			string (REGEX REPLACE "-I" "" _pcre2_includepath "${_pcre2_dashI}")
			string (REGEX REPLACE "-I[^ ]+" "" _pcre2_cflags_other ${PCRE2_CONFIG_CFLAGS})
		endif (PCRE2_CONFIG_CFLAGS)
		execute_process (COMMAND ${PCRE2_CONFIG} --libs8
			ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
			OUTPUT_VARIABLE PCRE2_CONFIG_LIBS)
		if (PCRE2_CONFIG_LIBS)
			string (REGEX MATCHALL "-l[^ ]+" _pcre2_dashl ${PCRE2_CONFIG_LIBS})
			string (REGEX REPLACE "-l" "" _pcre2_lib "${_pcre2_dashl}")
			string (REGEX MATCHALL "-L[^ ]+" _pcre2_dashL ${PCRE2_CONFIG_LIBS})
			string (REGEX REPLACE "-L" "" _pcre2_libpath "${_pcre2_dashL}")
		endif (PCRE2_CONFIG_LIBS)
	endif (PCRE2_CONFIG)
endif (UNIX AND NOT PCRE2_FOUND)

find_path (PCRE2_INCLUDE_DIR pcre2.h
	HINTS
	${_pcre2_includepath}
	${PCRE2_DIR}
	${PCRE2_ROOT}
	$ENV{PCRE2_DIR}
	$ENV{PCRE2_ROOT}
	PATH_SUFFIXES
	include/pcre2
	include/PCRE2
	include
	PATHS
	~/Library/Frameworks/pcre2.framework/Headers
	/Library/Frameworks/pcre2.framework/Headers
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	/usr/local
)

find_library (PCRE2_LIBRARY
	NAMES ${_pcre2_lib} pcre2-8 PCRE2
	HINTS
	${PCRE2_DIR}
	${PCRE2_ROOT}
	$ENV{PCRE2_DIR}
	$ENV{PCRE2_ROOT}
	${_pcre2_libpath}
	PATH_SUFFIXES lib
	PATHS
	~/Library/Frameworks/pcre2.framework
	/Library/Frameworks/pcre2.framework
	/sw
	/opt/local
	/opt/csw
	/opt
	/usr/local
)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (PCRE2 DEFAULT_MSG PCRE2_LIBRARY PCRE2_INCLUDE_DIR)

set (PCRE2_LIBRARIES ${PCRE2_LIBRARY})
set (PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
