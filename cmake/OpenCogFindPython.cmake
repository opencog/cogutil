# ----------------------------------------------------------
# Python and Cython
#
# NOTE: Python interpreter is needed for runing python unit tests,
# and for running the FindCython module.
#
# Search for Python3 first, and use that, if found. Else use Python2.
# To use Python2 only from the build directory run the following
# rm CMakeCache.txt && cmake -DCMAKE_DISABLE_FIND_PACKAGE_Python3Interp=TRUE ..

FIND_PACKAGE(Python3Interp)
IF (3.4.0 VERSION_LESS "${PYTHON3_VERSION_STRING}")
	SET (HAVE_PY_INTERP 1)
	MESSAGE(STATUS "Python ${PYTHON3_VERSION_STRING} interpreter found.")
ENDIF()

IF (NOT HAVE_PY_INTERP)
	FIND_PACKAGE(PythonInterp)
	IF (2.7.0 VERSION_LESS ${PYTHON_VERSION_STRING})
		SET (HAVE_PY_INTERP 1)
		MESSAGE(STATUS "Python ${PYTHON_VERSION_STRING} interpreter found.")
	ENDIF()
ENDIF()

FIND_PACKAGE(PythonLibs)
IF (PYTHONLIBS_FOUND AND
     ((PYTHON3INTERP_FOUND AND 3.4.0 VERSION_LESS ${PYTHONLIBS_VERSION_STRING})
     OR
     (2.7.0 VERSION_LESS ${PYTHONLIBS_VERSION_STRING})))
	SET (HAVE_PY_LIBS 1)
	MESSAGE(STATUS "Python ${PYTHONLIBS_VERSION_STRING} libraries found.")
ELSE()
	MESSAGE(STATUS "Python libraries NOT found.")
ENDIF()

# Cython is used to generate python bindings.
IF(HAVE_PY_INTERP)
	FIND_PACKAGE(Cython 0.23.0)

	IF (CYTHON_FOUND AND HAVE_PY_LIBS)
		ADD_DEFINITIONS(-DHAVE_CYTHON)
		SET(HAVE_CYTHON 1)

		IF (NOT DEFINED PYTHON_INSTALL_PREFIX)
			# Find python destination dir for python bindings
			# because it may differ on each operating system.
			EXECUTE_PROCESS(
				COMMAND ${PYTHON_EXECUTABLE} "${PROJECT_SOURCE_DIR}/scripts/get_python_lib.py" "${CMAKE_INSTALL_PREFIX}"
				OUTPUT_VARIABLE PYTHON_DEST
				)

			# Replace new line at end
			STRING(REPLACE "\n" "" PYTHON_DEST "${PYTHON_DEST}")
			IF ("${PYTHON_DEST}" STREQUAL "")
				MESSAGE(FATAL_ERROR "Python destination dir not found")
			ELSE ("${PYTHON_DEST}" STREQUAL "")
				MESSAGE(STATUS "Python destination dir found: ${PYTHON_DEST}" )
			ENDIF ("${PYTHON_DEST}" STREQUAL "")

			# thunk
			SET(PYTHON_ROOT "${PYTHON_DEST}")
			SET(PYTHON_DEST "${PYTHON_DEST}/opencog")
		ELSE()
			SET(PYTHON_ROOT "${PYTHON_INSTALL_PREFIX}")
			SET(PYTHON_DEST "${PYTHON_INSTALL_PREFIX}/opencog")
		ENDIF()

		MESSAGE(STATUS
			"Python install dir: ${PYTHON_DEST}" )

	ELSE (CYTHON_FOUND AND HAVE_PY_LIBS)
		IF(NOT CYTHON_FOUND)
			MESSAGE(STATUS "Cython executable not found.")
		ENDIF(NOT CYTHON_FOUND)
	ENDIF (CYTHON_FOUND AND HAVE_PY_LIBS)

	# Nosetests will find and automatically run python tests.
	IF (PYTHON3INTERP_FOUND AND 3.4.0 VERSION_LESS ${PYTHONLIBS_VERSION_STRING})
		FIND_PROGRAM(NOSETESTS_EXECUTABLE nosetests3)
	ELSE ()
		FIND_PROGRAM(NOSETESTS_EXECUTABLE nosetests-2.7)
	ENDIF ()
	IF (NOT NOSETESTS_EXECUTABLE)
		MESSAGE(STATUS "nosetests not found: needed for python tests")
	ENDIF ()
	IF (NOSETESTS_EXECUTABLE AND CYTHON_FOUND AND HAVE_PY_LIBS)
		SET(HAVE_NOSETESTS 1)
		MESSAGE(STATUS "Using nosetests executable " ${NOSETESTS_EXECUTABLE})
	ENDIF (NOSETESTS_EXECUTABLE AND CYTHON_FOUND AND HAVE_PY_LIBS)
ENDIF(HAVE_PY_INTERP)
