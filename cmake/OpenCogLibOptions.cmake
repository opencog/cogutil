# -------------------------------------------------
# Library configuration

# Use CMake's standard GNUInstallDirs for proper lib/lib64 handling
INCLUDE(GNUInstallDirs)

# This fixes linker issue on OS X
# See https://cmake.org/cmake/help/v3.0/policy/CMP0042.html
IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(CMAKE_MACOSX_RPATH 1)
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Backwards compatibility: derive LIB_DIR_SUFFIX from CMAKE_INSTALL_LIBDIR
# Many downstream projects still use lib${LIB_DIR_SUFFIX}/opencog
IF (NOT DEFINED LIB_DIR_SUFFIX)
	# CMAKE_INSTALL_LIBDIR is typically "lib" or "lib64"
	# Extract suffix by removing "lib" prefix
	STRING(REGEX REPLACE "^lib" "" LIB_DIR_SUFFIX "${CMAKE_INSTALL_LIBDIR}")
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
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/opencog")
SET(CMAKE_BUILD_WITH_INSTALL_NAME_DIR FALSE)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
