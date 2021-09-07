
# ----------------------------------------------------------
# This is required for Guile
FIND_LIBRARY(GMP_LIBRARY gmp)
FIND_PATH(GMP_INCLUDE_DIR gmp.h)

# Gnu Guile scheme interpreter. Mandatory
# Version 2.2.2 is needed for mrs io ports, compilation, atomics, nlp
FIND_PACKAGE(Guile 2.2.2)
IF (GUILE_FOUND AND GMP_LIBRARY AND GMP_INCLUDE_DIR)
	ADD_DEFINITIONS(-DHAVE_GUILE)
	SET(HAVE_GUILE 1)
	INCLUDE_DIRECTORIES(${GUILE_INCLUDE_DIR})
ELSE (GUILE_FOUND AND GMP_LIBRARY AND GMP_INCLUDE_DIR)
	SET(GUILE_DIR_MESSAGE "Guile was not found; the scheme bindings will not be built.\nTo over-ride, make sure GUILE_LIBRARIES and GUILE_INCLUDE_DIRS are set.")
	MESSAGE(FATAL_ERROR "${GUILE_DIR_MESSAGE}")
ENDIF (GUILE_FOUND AND GMP_LIBRARY AND GMP_INCLUDE_DIR)
