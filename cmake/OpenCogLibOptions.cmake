# -------------------------------------------------
# Library configuration

# This fixes linker issue on OS X
# See https://cmake.org/cmake/help/v3.0/policy/CMP0042.html
IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(CMAKE_MACOSX_RPATH 1)
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Small hack to handle unixes that use "/usr/lib64" instead of
# "/usr/lib" as the default lib path on 64 bit archs.
IF (NOT DEFINED LIB_DIR_SUFFIX)
	IF (WIN32)
		# On Windows with MSVC, we don't need this check
	ELSE (WIN32)
		EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -print-search-dirs OUTPUT_VARIABLE PRINT_SEARCH_DIRS_OUTPUT)
		STRING(REGEX MATCH "\r?\nlibraries:.*\r?\n" COMPILER_LIB_SEARCH_DIRS ${PRINT_SEARCH_DIRS_OUTPUT})
		IF (NOT ${COMPILER_LIB_SEARCH_DIRS} STREQUAL "")
			STRING(REGEX MATCH "/lib64/:|/lib64:|/lib64\n" HAS_LIB64 ${COMPILER_LIB_SEARCH_DIRS})
			IF (NOT ${HAS_LIB64} STREQUAL "")
				SET(LIB_DIR_SUFFIX "64")
			ENDIF (NOT ${HAS_LIB64} STREQUAL "")
		ENDIF (NOT ${COMPILER_LIB_SEARCH_DIRS} STREQUAL "")
	ENDIF (WIN32)
ENDIF (NOT DEFINED LIB_DIR_SUFFIX)

# RPATH handling (see https://cmake.org/Wiki/CMake_RPATH_handling)
# Note: RPATH only supported under Linux!
# The idea here is that during unit tests, we use the libraries in
# the build directories, and NOT the system directories. But that
# means that, during install, the paths have to be tweaked.
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
IF (APPLE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
ELSE (APPLE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
ENDIF (APPLE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_DIR_SUFFIX}/opencog")
SET(CMAKE_BUILD_WITH_INSTALL_NAME_DIR FALSE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
