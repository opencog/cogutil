# ----------------------------------------------------------
# Python and Cython
#

# ----------------------------------------------------------
# secure_getenv
# Without this, one gets the common cryptic python error message:
#    ImportError: No module named 'opencog'

INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(secure_getenv "stdlib.h" HAVE_SECURE_GETENV)
IF (HAVE_SECURE_GETENV)
   ADD_DEFINITIONS(-DHAVE_SECURE_GETENV)
ENDIF (HAVE_SECURE_GETENV)

# ----------------------------------------------------------
# NOTE: Python interpreter is needed for runing python unit tests,
# and for running the FindCython module.
#
# Search for Python3 first, and use that, if found. Else use Python2.
# To use Python2 only, run the following in the build directory:
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
IF (PYTHONLIBS_FOUND)
	IF ((PYTHON3INTERP_FOUND AND 3.4.0 VERSION_LESS ${PYTHONLIBS_VERSION_STRING})
	     OR
	     (2.7.0 VERSION_LESS ${PYTHONLIBS_VERSION_STRING}))
		SET (HAVE_PY_LIBS 1)
		MESSAGE(STATUS "Python ${PYTHONLIBS_VERSION_STRING} libraries found.")
	ENDIF()
ELSE()
	MESSAGE(STATUS "Python libraries NOT found.")
ENDIF()

# Cython is used to generate python bindings.
IF(HAVE_PY_INTERP)
	# Version 0.24 or later needed for @property markup for python2
	FIND_PACKAGE(Cython 0.24.0)

	IF (CYTHON_FOUND AND HAVE_PY_LIBS)
		ADD_DEFINITIONS(-DHAVE_CYTHON)
		SET(HAVE_CYTHON 1)

		IF (NOT DEFINED PYTHON_INSTALL_PREFIX)
			FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/scripts)
			# Unpack Python install destination detection script into project
			# binary dir.
			#
			# This is a hack due to the distutils in debian/ubuntu's python3 being misconfigured
			# see discussion https://github.com/opencog/atomspace/issues/1782
			#
			# If the bug is fixed, most of this script could be replaced by:
			#
			# from distutils.sysconfig import get_python_lib; print(get_python_lib(plat_specific=True, prefix=prefix))
			#
			# However, using this would not respect a python virtual environments, so in a way this is better!
			FILE(WRITE ${PROJECT_BINARY_DIR}/scripts/get_python_lib.py
				"import sys\n"
				"import sysconfig\n"
				"import site\n"
				"\n"
				"if __name__ == '__main__':\n"
				"    prefix = sys.argv[1]\n"
				"\n"
				"    # use sites if the prefix is recognized and the sites module is available\n"
				"    # (virtualenv is missing getsitepackages())\n"
				"    if hasattr(site, 'getsitepackages'):\n"
				"        paths = [p for p in site.getsitepackages() if p.startswith(prefix)]\n"
				"        if len(paths) == 1:\n"
				"            print(paths[0])\n"
				"            exit(0)\n"
				"    \n"
				"    # use sysconfig platlib as the fall back\n"
				"    print(sysconfig.get_paths()['platlib'])\n"
				)

			# Find python destination dir for python bindings
			# because it may differ on each operating system.
			EXECUTE_PROCESS(
				COMMAND ${PYTHON_EXECUTABLE} "${PROJECT_BINARY_DIR}/scripts/get_python_lib.py" "${CMAKE_INSTALL_PREFIX}"
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
	IF (PYTHON3INTERP_FOUND AND PYTHONLIBS_FOUND)
		IF (3.4.0 VERSION_LESS ${PYTHONLIBS_VERSION_STRING})
			FIND_PROGRAM(NOSETESTS_EXECUTABLE nosetests3)
		ENDIF ()
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
