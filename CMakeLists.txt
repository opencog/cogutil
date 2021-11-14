#
# Master Opencog CMake file.
#
# General organization:
# -- check for different compilers, OS'es
# -- search for various required & optional libraries/tools
# -- decide what to build based on above results.
# -- configure various config files.
# -- print pretty summary
#
# To specify which compiler to use, the environment variables CC and CXX
# can override the path to the C and CPP compilers, respectively.
# CMake will not update these settings internally until both the ./CMakeFiles
# folder and ./CMakeCache.txt are deleted.

include(cmake/Summary.cmake)

# add_compile_definitions shows up in CMake version 3.12
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
IF (COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF (COMMAND CMAKE_POLICY)

IF (CMAKE_VERSION VERSION_GREATER 3.0.2)
	CMAKE_POLICY(SET CMP0037 OLD)
	CMAKE_POLICY(SET CMP0042 NEW)
ENDIF (CMAKE_VERSION VERSION_GREATER 3.0.2)
IF (CMAKE_VERSION VERSION_GREATER 3.9.0)
	CMAKE_POLICY(SET CMP0068 NEW)
ENDIF (CMAKE_VERSION VERSION_GREATER 3.9.0)

PROJECT(cogutil)

# Uncomment to be in Release mode [default].
# SET(CMAKE_BUILD_TYPE Release)

# Uncomment to build in debug mode.
# SET(CMAKE_BUILD_TYPE Debug)

# Uncomment to be in coverage testing mode.
# SET(CMAKE_BUILD_TYPE Coverage)

# Uncomment to build in profile mode.
# SET(CMAKE_BUILD_TYPE Profile)

# Uncomment to build in release mode with debug information.
# SET(CMAKE_BUILD_TYPE RelWithDebInfo)

# Default build type.
IF (CMAKE_BUILD_TYPE STREQUAL "")
	SET(CMAKE_BUILD_TYPE Release)
ENDIF (CMAKE_BUILD_TYPE STREQUAL "")

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

ADD_DEFINITIONS(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
	            -DPROJECT_BINARY_DIR="${CMAKE_BINARY_DIR}")

# Add the 'cmake' dir to cmake's module search path.
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Get cogutil version.
# NOTE: This is the official semantic-version, as it is derived from
# a version-control independent means of declaring versioning.
#
# TODO: Once CMAKE_MINIMUM_REQUIRED >= 3.0.2 check use of policy CMP0048.
#
FILE(READ "${CMAKE_SOURCE_DIR}/opencog/util/cogutil.h" _CU_H_CONTENTS)
STRING(REGEX MATCH
	"#define COGUTIL_VERSION_STRING \"([0-9]+[.0-9]+[.0-9]+)\""
	_ "${_CU_H_CONTENTS}")
SET(SEMANTIC_VERSION ${CMAKE_MATCH_1})

include(cmake/OpenCogGccOptions.cmake)

# ------------------------------------------------------
IF (WIN32) # Windows host but not cygwin
	MESSAGE(FATAL_ERROR "You must use Cygwin if compiling under Windows.")

	# Previous definitions used for win32 (non cygwin) compilation.
	# For reference only

	# ADD_DEFINITIONS(-DWIN32_NOT_UNIX)
	# FIND_PACKAGE(PThreads REQUIRED)
	# FIND_PACKAGE(STLPort REQUIRED)
	# INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include/win32" ${PTHREADS_INCLUDE_DIR} ${STLPORT_INCLUDE_DIR})
	# LINK_LIBRARIES(${PTHREADS_LIBRARY} ${STLPORT_LIBRARIES})
	# ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
ENDIF (WIN32)

# Do the cygwin build
IF (CYGWIN)
	ADD_DEFINITIONS(-DCYGWIN)
	LINK_LIBRARIES(pthread)
ENDIF (CYGWIN)

# ===================================================================
# Check for existance of various required, optional packages.

# Check for boost. We need dynamic-linked, threaded libs by default.
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREADED ON)
# The minimum boost version required for building.
SET(MIN_BOOST 1.60)

# Required boost packages
# 1.60 is minimum for required filesystem support
FIND_PACKAGE(Boost ${MIN_BOOST} COMPONENTS filesystem program_options system thread REQUIRED)

IF (Boost_FOUND)
	SET(Boost_FOUND_SAVE 1)
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Boost ${MIN_BOOST} or newer (package libboost-all-dev if using Ubuntu) is needed to build!")
ENDIF (Boost_FOUND)

# Optional boost packages; can build without these. (?? what needs this ??)
FIND_PACKAGE(Boost ${MIN_BOOST} COMPONENTS math_c99 QUIET)

# Arghhh. Except cmake is treating above as required, not optional. #$%**&
IF (Boost_FOUND_SAVE)
	SET(Boost_FOUND 1)
ENDIF (Boost_FOUND_SAVE)

# -------------------------------------------------------------
FIND_PACKAGE(Cxxtest)
IF (NOT CXXTEST_FOUND)
	MESSAGE(STATUS "CxxTest missing: needed for unit tests.")
ENDIF (NOT CXXTEST_FOUND)

# -------------------------------------------------------------
# Look for GNU C++ backtrace. Needed for basic stack printing.
FIND_PACKAGE(GNUBacktrace)
IF (GNU_BACKTRACE_FOUND)
	ADD_DEFINITIONS(-DHAVE_GNU_BACKTRACE)
ELSE (GNU_BACKTRACE_FOUND)
	MESSAGE(STATUS "GNU backtrace missing: No basic stack-trace printing.")
ENDIF (GNU_BACKTRACE_FOUND)

# -------------------------------------------------------------
# Look for binutils-dev. Needed for pretty stack printing.
FIND_PACKAGE(BFD)
IF (BFD_FOUND)
	MESSAGE(STATUS "Binutils found.")
	ADD_DEFINITIONS(-DHAVE_BFD)
	SET(HAVE_BFD 1)
ELSE (BFD_FOUND)
	MESSAGE(STATUS "Binutils-dev missing: No pretty stack-trace printing.")
ENDIF (BFD_FOUND)

# Look for libiberty-dev. Needed for pretty stack printing.
FIND_PACKAGE(Iberty)
IF (IBERTY_FOUND)
	MESSAGE(STATUS "Libiberty found.")
	ADD_DEFINITIONS(-DHAVE_IBERTY)
	SET(HAVE_IBERTY 1)
ELSE (IBERTY_FOUND)
	MESSAGE(STATUS "Libiberty-dev missing: No pretty stack-trace printing.")
ENDIF (IBERTY_FOUND)

# Look for standardized C++ parallelism
FIND_PACKAGE(ParallelSTL)
IF (PARALLEL_STL_FOUND)
	ADD_DEFINITIONS(-DHAVE_PARALLEL_STL)
ENDIF (PARALLEL_STL_FOUND)

# ===================================================================
# Global includes

# -------------------------------------------------
# Include configuration.

# Set default include paths.
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

# -------------------------------------------------
# Library configuration
# Small hack to handle unixes that use "/usr/lib64" instead of
# "/usr/lib" as the default lib path on 64 bit archs.
IF (NOT DEFINED LIB_DIR_SUFFIX)
	EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -print-search-dirs OUTPUT_VARIABLE PRINT_SEARCH_DIRS_OUTPUT)
	STRING(REGEX MATCH "\r?\nlibraries:.*\r?\n" COMPILER_LIB_SEARCH_DIRS ${PRINT_SEARCH_DIRS_OUTPUT})
	IF (NOT ${COMPILER_LIB_SEARCH_DIRS} STREQUAL "")
		STRING(REGEX MATCH "/lib64/:|/lib64:|/lib64\n" HAS_LIB64 ${COMPILER_LIB_SEARCH_DIRS})
		IF (NOT ${HAS_LIB64} STREQUAL "")
			SET(LIB_DIR_SUFFIX "64")
		ENDIF (NOT ${HAS_LIB64} STREQUAL "")
	ENDIF (NOT ${COMPILER_LIB_SEARCH_DIRS} STREQUAL "")
ENDIF (NOT DEFINED LIB_DIR_SUFFIX)

# RPATH handling (see https://cmake.org/Wiki/CMake_RPATH_handling)
# Note: RPATH only supported under Linux!
SET(CMAKE_SKIP_BUILD_RPATH	FALSE)
IF (APPLE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
ELSE (APPLE)
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
ENDIF (APPLE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/opencog")
SET(CMAKE_BUILD_WITH_INSTALL_NAME_DIR FALSE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# -------------------------------------------------
# Install configuration

# Only list install files that have actually changed.
SET(CMAKE_INSTALL_MESSAGE "LAZY")

# set confdir and datadir
IF (NOT DEFINED CONFDIR)
	SET (CONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
ENDIF (NOT DEFINED CONFDIR)
IF (NOT DEFINED DATADIR)
	SET (DATADIR "${CMAKE_INSTALL_PREFIX}/share/opencog")
ENDIF (NOT DEFINED DATADIR)
ADD_DEFINITIONS(-DCONFDIR="${CONFDIR}")
ADD_DEFINITIONS(-DDATADIR="${DATADIR}")

# (re?)define MAN_INSTALL_DIR
SET (MAN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/man")

# ===================================================================
# Decide what to build, based on the packages found.

IF (Boost_FOUND)
	SET(HAVE_UTIL 1)
ENDIF (Boost_FOUND)

ADD_SUBDIRECTORY(cmake)
ADD_SUBDIRECTORY(etc)
ADD_SUBDIRECTORY(opencog)

IF (CXXTEST_FOUND)
	ADD_CUSTOM_TARGET(tests)
	ADD_SUBDIRECTORY(tests EXCLUDE_FROM_ALL)
	IF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		# doing coverage stuff while running tests if this is the Coverage build
		ADD_CUSTOM_TARGET(test
			# TODO lcov should be found by cmake first
			# TODO set it up so that we can pick to run coverage per test, or
			# combined across all tests (the latter is MUCH faster). Use a define?
			# There is coverage specific stuff in AddCxxTest.cmake now...
			# -
			WORKING_DIRECTORY tests
			COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process $(ARGS)
			# This script combines the coverage analysis of each test,
			# then creates html in tests/lcov
			# Note: this should now be run separately...
			#COMMAND ${PROJECT_SOURCE_DIR}/scripts/combine_lcov.sh
			COMMENT "Running tests with coverage..."
		)
	ELSE (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		# If this is a build with coverage enabled then test normally
		ADD_CUSTOM_TARGET(test
			DEPENDS tests
			WORKING_DIRECTORY tests
			COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process $(ARGS)
			COMMENT "Running tests..."
		)
	ENDIF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
ENDIF (CXXTEST_FOUND)

ADD_CUSTOM_TARGET(cscope
	COMMAND find opencog examples tests -name '*.cc' -o -name '*.h' -o -name '*.cxxtest' -o -name '*.scm' > ${CMAKE_SOURCE_DIR}/cscope.files
	COMMAND cscope -b
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Generating CScope database"
)

# ===================================================================
# Packaging
# For what architecture the package is for
# TODO: Will give error on non debian distros, fix it.
EXECUTE_PROCESS(COMMAND  dpkg --print-architecture
	OUTPUT_VARIABLE PACKAGE_ARCHITECTURE
	OUTPUT_STRIP_TRAILING_WHITESPACE)
STRING(TIMESTAMP UTC_DATE %Y%m%d UTC)
# If 'sudo make install' is run before 'make package', then install_manifest.txt
# will be owned by root. Creating the file during configuration stage ensures
# that is owned by the builder thus avoiding 'Permission denied' error when
# packaging.
FILE(WRITE "${PROJECT_BINARY_DIR}/install_manifest.txt")

## Cpack configuration
SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_CONTACT "opencog@googlegroups.com")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The OpenCog Basic C++ Utilities")
SET(CPACK_PACKAGE_NAME "libcogutil-dev")
SET(CPACK_PACKAGE_VENDOR "opencog.org")
SET(CPACK_PACKAGE_VERSION "${SEMANTIC_VERSION}-${UTC_DATE}")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_FILE_NAME
	"${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${PACKAGE_ARCHITECTURE}")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")

## Debian specific configurations
SET(DEPENDENCY_LIST
	"libboost-filesystem-dev (>= ${MIN_BOOST})"
	"libboost-math-dev (>= ${MIN_BOOST})"
	"libboost-program-options-dev (>= ${MIN_BOOST})"
	"libboost-thread-dev (>= ${MIN_BOOST})"
	"libboost-system-dev (>= ${MIN_BOOST})"
	"libstdc++6 (>= 4.7)"
	"binutils-dev"
	"libiberty-dev"
)

STRING(REPLACE ";" ", " MAIN_DEPENDENCIES "${DEPENDENCY_LIST}")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${MAIN_DEPENDENCIES}")
SET(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://opencog.org")
INCLUDE(CPack)

# ===================================================================
# Documentation.
FIND_PACKAGE(Doxygen)
ADD_SUBDIRECTORY(doc EXCLUDE_FROM_ALL)

# ===================================================================
# Show a summary of what we found, what we will do.

SUMMARY_ADD("Doxygen" "Code documentation" DOXYGEN_FOUND)
SUMMARY_ADD("StackPrint" "Pretty printing of stack traces" HAVE_BFD AND HAVE_IBERTY)
SUMMARY_ADD("Unit tests" "Unit tests" CXXTEST_FOUND)
SUMMARY_ADD("Util" "General utility library" HAVE_UTIL)
SUMMARY_SHOW()
