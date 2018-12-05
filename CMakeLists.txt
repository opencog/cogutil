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

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12.2)
IF (COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0003 NEW)
	CMAKE_POLICY(SET CMP0005 OLD)
ENDIF (COMMAND CMAKE_POLICY)

IF (CMAKE_VERSION VERSION_GREATER 3.0.2)
	CMAKE_POLICY(SET CMP0037 OLD)
ENDIF (CMAKE_VERSION VERSION_GREATER 3.0.2)

# Only list install files that have actually changed
SET(CMAKE_INSTALL_MESSAGE "LAZY")

PROJECT(cogutil)

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

IF (NOT CMAKE_COMPILER_IS_GNUCXX) # Compiler is not gcc
	MESSAGE(FATAL_ERROR "Only the gcc compiler is supported at this time")
ENDIF (NOT CMAKE_COMPILER_IS_GNUCXX)

# uncomment to be in Release mode [default]
# SET(CMAKE_BUILD_TYPE Release)

# uncomment to build in debug mode
# SET(CMAKE_BUILD_TYPE Debug)

# uncomment to be in coverage testing mode
# SET(CMAKE_BUILD_TYPE Coverage)

# uncomment to build in profile mode
# SET(CMAKE_BUILD_TYPE Profile)

# uncomment to build in release mode with debug information
# SET(CMAKE_BUILD_TYPE RelWithDebInfo)

# default build type
IF (CMAKE_BUILD_TYPE STREQUAL "")
	SET(CMAKE_BUILD_TYPE Release)
ENDIF (CMAKE_BUILD_TYPE STREQUAL "")

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

ADD_DEFINITIONS(-DPROJECT_SOURCE_DIR=\\"${CMAKE_SOURCE_DIR}\\"
	            -DPROJECT_BINARY_DIR=\\"${CMAKE_BINARY_DIR}\\")

# add the 'cmake' dir to cmake's module search path
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# determine version
SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The OpenCog Basic C++ Utilities")
SET(CPACK_PACKAGE_NAME "libcogutil-dev")
SET(CPACK_PACKAGE_VENDOR "opencog.org")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
FILE(READ "${CMAKE_SOURCE_DIR}/opencog/util/cogutil.h" _CU_H_CONTENTS)
STRING(REGEX MATCH
	"#define COGUTIL_MAJOR_VERSION ([0-9]+)"
	_ "${_CU_H_CONTENTS}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
STRING(REGEX MATCH
	"#define COGUTIL_MINOR_VERSION ([0-9]+)"
	_ "${_CU_H_CONTENTS}")
SET(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_1})
STRING(REGEX MATCH
	"#define COGUTIL_MICRO_VERSION ([0-9]+)"
	_ "${_CU_H_CONTENTS}")
SET(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_1})
# NOTE: This is the official semantic-version, as it is derived from a
# version-control independent means of declaring versioning.
SET(SEMANTIC_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

# NOTE: `git describe` returns an output with count of commits after the last
# annotated-tags. This makes relasing multiple daily builds easy as the
# installed packgaes can easily be sorted. If that doesn't work then for
# debian packages 'Breaks' and 'Replaces' fields might have to be used.
EXECUTE_PROCESS(COMMAND  git describe --long "--match=*${SEMANTIC_VERSION}*"
	OUTPUT_VARIABLE COGUTIL_GIT_VERSION
	RESULT_VARIABLE RV
	OUTPUT_STRIP_TRAILING_WHITESPACE)
IF(${RV})
	MESSAGE(WARNING "\nThere is no git annotated-tag with semantic-version = "
	${SEMANTIC_VERSION})
ENDIF()


# ===============================================================
# Detect different compilers and OS'es, tweak flags as necessary.

IF (CMAKE_COMPILER_IS_GNUCXX)  # Compiler is GCC
	# aiiee cmake version 2.8.8 or newer is needed for below to
	# work...
	# IF (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6.4)
	#	MESSAGE(FATAL_ERROR "GCC version must be at least 4.6.4!")
	# ENDIF (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6.4)

	IF (APPLE)
		SET(CMAKE_C_FLAGS "-Wall -Wno-long-long -Wno-conversion")
		SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
		SET(CMAKE_C_FLAGS_PROFILE "-O0 -pg")
		SET(CMAKE_C_FLAGS_RELEASE "-O2 -g0")
		# Vital to do this otherwise unresolved symbols everywhere:
		SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-flat_namespace,-undefined,dynamic_lookup")
		SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-flat_namespace,-undefined,dynamic_lookup")
	ELSE (APPLE)
		SET(CMAKE_C_FLAGS "-Wall -fPIC -Wl,--copy-dt-needed-entries")
		SET(CMAKE_C_FLAGS_DEBUG "-ggdb3 -fstack-protector")
		SET(CMAKE_C_FLAGS_PROFILE "-O2 -g3 -fstack-protector -pg")
		SET(CMAKE_C_FLAGS_RELEASE "-O2 -g -fstack-protector")
	ENDIF (APPLE)

	# 1) -Wno-variadic-macros is to avoid warnings regarding using
	# variadic in macro OC_ASSERT (the warning warns that this is only
	# available from C99, lol!)
	#
	# 2) -fopenmp for multithreading support
	#
	# 3) -std=gnu++0x for C++0x and GNU extensions support
	SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wno-variadic-macros -fopenmp -std=gnu++11")

	SET(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	SET(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})
	SET(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

	# Options for generating gcov code coverage output
	SET(CMAKE_C_FLAGS_COVERAGE "-O0 -g -fprofile-arcs -ftest-coverage -fno-inline")
	SET(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_C_FLAGS_COVERAGE} -fno-default-inline")
	# Might be needed for some combinations of ln and gcc
	IF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		LINK_LIBRARIES(gcov)
	ENDIF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
ENDIF (CMAKE_COMPILER_IS_GNUCXX)


# Do the cygwin build
IF (CYGWIN)
	ADD_DEFINITIONS(-DCYGWIN)
	LINK_LIBRARIES(pthread)
ENDIF (CYGWIN)

# ===================================================================
# Check for existance of various required, optional packages.

# Look for binutils-dev. Needed for pretty stack printing.
FIND_PACKAGE(BFD)
IF (BFD_FOUND)
	MESSAGE(STATUS "Binutils found.")
	ADD_DEFINITIONS(-DHAVE_BFD)
	SET(HAVE_BFD 1)
ELSE (BFD_FOUND)
	MESSAGE(STATUS "Binutils-dev missing: No pretty stack-trace printing.")
ENDIF (BFD_FOUND)

# Check for boost. We need dynamic-linked, threaded libs by default.
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREADED ON)
# The minimum boost version required for building.
SET(MIN_BOOST 1.46)

# Required boost packages
# hypertable requires boost iostreams
# boost-1.49 no longer has a libboost_iostreams
# 1.46 is minimum for required filesystem support
# program_options needed by some combo utilities
FIND_PACKAGE(Boost ${MIN_BOOST} COMPONENTS date_time filesystem program_options regex serialization system thread REQUIRED)

IF (Boost_FOUND)
	SET(Boost_FOUND_SAVE 1)

	# Boost.Signals2 requires this in signal_type.hpp
	SET(BOOST_PARAMETER_MAX_ARITY 7)
	ADD_DEFINITIONS(-DBOOST_PARAMETER_MAX_ARITY=${BOOST_PARAMETER_MAX_ARITY})
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Boost ${MIN_BOOST} or newer (package libboost-all-dev if using Ubuntu) is needed to build OpenCog!")
ENDIF (Boost_FOUND)

# Opencog won't compile with Boost 1.51, some kind of conflict with
# hash functions, see github bugs 1 and 36
IF (105100 EQUAL ${Boost_VERSION})
	MESSAGE(FATAL_ERROR "Boost version 1.51 will not work with OpenCog.  Please use a different version.")
ENDIF (105100 EQUAL ${Boost_VERSION})
MESSAGE(STATUS "Boost version ${Boost_VERSION} found.")

# Optional boost packages; can build without these.
FIND_PACKAGE(Boost ${MIN_BOOST} COMPONENTS python program_options QUIET)
FIND_PACKAGE(Boost ${MIN_BOOST} COMPONENTS math_c99 QUIET)

# Arghhh. Except cmake is treating above as required, not optional. #$%**&
IF (Boost_FOUND_SAVE)
	SET(Boost_FOUND 1)
ENDIF (Boost_FOUND_SAVE)

FIND_PACKAGE(Cxxtest)
IF (NOT CXXTEST_FOUND)
	MESSAGE(STATUS "CxxTest missing: needed for unit tests.")
ENDIF (NOT CXXTEST_FOUND)


# Look for libiberty-dev. Needed for pretty stack printing.
FIND_PACKAGE(Iberty)
IF (IBERTY_FOUND)
	MESSAGE(STATUS "Libiberty found.")
	ADD_DEFINITIONS(-DHAVE_IBERTY)
	SET(HAVE_IBERTY 1)
ELSE (IBERTY_FOUND)
	MESSAGE(STATUS "Libiberty-dev missing: No pretty stack-trace printing.")
ENDIF (IBERTY_FOUND)


# ==========================================================
# Decide what to build, based on the packages found.

IF (Boost_FOUND)
	SET(HAVE_UTIL 1)
ENDIF (Boost_FOUND)

# ===================================================================
# global includes

# set confdir and datadir
IF (NOT DEFINED CONFDIR)
	SET (CONFDIR "${CMAKE_INSTALL_PREFIX}/etc")
ENDIF (NOT DEFINED CONFDIR)
IF (NOT DEFINED DATADIR)
	SET (DATADIR "${CMAKE_INSTALL_PREFIX}/share/opencog")
ENDIF (NOT DEFINED DATADIR)
ADD_DEFINITIONS(-DCONFDIR=\\"${CONFDIR}\\")
ADD_DEFINITIONS(-DDATADIR=\\"${DATADIR}\\")

# (re?)define MAN_INSTALL_DIR
SET (MAN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/man")

# small hack to handle unixes that use "/usr/lib64" instead of "/usr/lib" as the
# default lib path on 64 bit archs
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

# set default include paths
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

# RPATH handling (see https://cmake.org/Wiki/CMake_RPATH_handling)
# Note: RPATH only supported under Linux!
SET(CMAKE_SKIP_BUILD_RPATH	FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/opencog")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

ADD_SUBDIRECTORY(cmake)
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

# Packaging
# For what architecture the package is for
# TODO: Will give error on non debian distros.
EXECUTE_PROCESS(COMMAND  dpkg --print-architecture
	OUTPUT_VARIABLE PACKAGE_ARCHITECTURE
	OUTPUT_STRIP_TRAILING_WHITESPACE)
# Cleanup because debain version require that it starts with a number.
STRING(REGEX MATCH "([0-9]+[0-9.]+)-([a-zA-Z0-9-]+)$"
	_ "${COGUTIL_GIT_VERSION}")
SET(CPACK_PACKAGE_VERSION "${CMAKE_MATCH_0}")

SET(CPACK_PACKAGE_CONTACT "opencog@googlegroups.com")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_FILE_NAME
	"${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${PACKAGE_ARCHITECTURE}")
SET(CPACK_SOURCE_IGNORE_FILES
	"~$"
	".a$"
	".so$"
	".log$"
	".tar.z$"
	".tar.gz$"
	".tar.bz2$"
	"/Makefile$"
	"/CMakeFiles/"
	"/CMakeFiles/"
	"/CMakeCache.txt$"
	"/DartTestfile.txt$"
	"/cmake_install.cmake$"
	"/*.cmake$"
	"/*.make$"
	"UTest$"
	"UTest.cpp$"
	"/_CPack_Packages/"
	"/sniff$"
	"/install_manifest.txt$"
	"${CPACK_PACKAGE_FILE_NAME}"
	"${CPACK_PACKAGE_DIRECTORY}"
	"^${PROJECT_SOURCE_DIR}/debian/"
	"^${PROJECT_SOURCE_DIR}/.*spec$"
	"^${PROJECT_SOURCE_DIR}/vendor/"
	"^${PROJECT_SOURCE_DIR}/.git/"
)

SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")

## Debian specific configurations
SET(DEPENDENCY_LIST
	"libboost-date-time-dev (>= ${MIN_BOOST})"
	"libboost-filesystem-dev (>= ${MIN_BOOST})"
	"libboost-math-dev (>= ${MIN_BOOST})"
	"libboost-program-options-dev (>= ${MIN_BOOST})"
	"libboost-regex-dev (>= ${MIN_BOOST})"
	"libboost-serialization-dev (>= ${MIN_BOOST})"
	"libboost-thread-dev (>= ${MIN_BOOST})"
	"libboost-system-dev (>= ${MIN_BOOST})"
	"libboost-random-dev (>= ${MIN_BOOST})"
	"libstdc++6 (>= 4.7)"
	"binutils-dev"
	"libiberty-dev"
)
STRING(REPLACE ";" ", " MAIN_DEPENDENCIES "${DEPENDENCY_LIST}")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${MAIN_DEPENDENCIES}")
SET(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://opencog.org")
INCLUDE(CPack)

# documentation
FIND_PACKAGE(Doxygen)
ADD_SUBDIRECTORY(doc EXCLUDE_FROM_ALL)

# Show a summary of what we got
SUMMARY_ADD("Doxygen" "Code documentation" DOXYGEN_FOUND)
SUMMARY_ADD("StackPrint" "Pretty printing of stack traces" HAVE_BFD AND HAVE_IBERTY)
SUMMARY_ADD("Unit tests" "Unit tests" CXXTEST_FOUND)
SUMMARY_ADD("Util" "General utility library" HAVE_UTIL)
SUMMARY_SHOW()
