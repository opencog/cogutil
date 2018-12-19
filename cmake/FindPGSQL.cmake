# Copyright (c) 2008, OpenCog.org (http://opencog.org)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# - Try to find the PostgreSQL library; Once done this will define
#
# PGSQL_FOUND - system has the PostgreSQL library and required dependencies
# PGSQL_INCLUDE_DIRS - the PostgreSQL include directory
# PGSQL_LIBRARIES - The libraries needed to use PostgreSQL

# Look for the header file
FIND_PATH(PGSQL_INCLUDE_DIR libpq-fe.h
	/usr/include
	/usr/local/include
	/usr/include/postgresql
	/usr/local/include/postgresql
	DOC "Set the PGSQL_INCLUDE_DIR cmake cache entry to the directory containing libpq-fe.h."
)

# Look for the library
FIND_LIBRARY(PGSQL_LIBRARY NAMES pq
	DOC "Set the PGSQL_LIBRARY_DIR cmake cache entry to the directory containing libpq.so."
)

# Get the version number
IF (PGSQL_INCLUDE_DIR)
	FILE(STRINGS "${PGSQL_INCLUDE_DIR}/pg_config.h" PGVERSTR
		REGEX "^#define[\t ]+PG_VERSION[\t ]+\".*\"")
	IF(PGVERSTR)
		STRING(REGEX REPLACE "^#define[\t ]+PG_VERSION[\t ]+\"([^\"]*)\".*"
			"\\1" PGSQL_VERSION_STRING "${PGVERSTR}")
	ENDIF(PGVERSTR)
ENDIF (PGSQL_INCLUDE_DIR)

# Check for required minimum version number.
find_package_handle_standard_args(PGSQL
	REQUIRED_VARS PGSQL_LIBRARY PGSQL_INCLUDE_DIR
	VERSION_VAR PGSQL_VERSION_STRING
)

# Copy the results to the output variables.
IF (PGSQL_INCLUDE_DIR AND PGSQL_LIBRARY)

	SET(PGSQL_FOUND 1)
	SET(PGSQL_LIBRARIES ${PGSQL_LIBRARY} )
	SET(PGSQL_INCLUDE_DIRS ${PGSQL_INCLUDE_DIR})
ELSE (PGSQL_INCLUDE_DIR AND PGSQL_LIBRARY)
	SET(PGSQL_FOUND 0)
	SET(PGSQL_LIBRARIES)
	SET(PGSQL_INCLUDE_DIRS)
ENDIF (PGSQL_INCLUDE_DIR AND PGSQL_LIBRARY)

# Report the results.
IF (NOT PGSQL_FOUND)
	IF (NOT PGSQL_FIND_QUIETLY)
		MESSAGE(STATUS "${PGSQL_DIR_MESSAGE}")
	ELSE (NOT PGSQL_FIND_QUIETLY)
		IF (PGSQL_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "${PGSQL_DIR_MESSAGE}")
		ENDIF (PGSQL_FIND_REQUIRED)
	ENDIF (NOT PGSQL_FIND_QUIETLY)
ENDIF (NOT PGSQL_FOUND)

MARK_AS_ADVANCED(PGSQL_INCLUDE_DIRS)
MARK_AS_ADVANCED(PGSQL_LIBRARIES)
