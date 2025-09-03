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
# Search for Python3
FIND_PACKAGE(Python3 COMPONENTS Interpreter Development)
IF (Python3_Interpreter_FOUND)
	SET (HAVE_PY_INTERP 1)
	MESSAGE(STATUS "Python ${Python3_VERSION} interpreter found.")
ENDIF()

IF (Python3_Development_FOUND)
	SET (HAVE_PY_LIBS 1)
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

		# The CMake interfaces are completely different between
		# Cmake 3 and CMake 4. Try to be backwards compat, if possible.
		IF (NOT DEFINED PYTHON_INSTALL_PREFIX)
			IF (DEFINED Python3_SITEARCH)
				SET(PYTHON_INSTALL_PREFIX ${Python3_SITEARCH})
			ENDIF()
		ENDIF()

		IF (NOT DEFINED PYTHON_INSTALL_PREFIX)
			FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/scripts)
			# Unpack Python install destination detection script into project
			# binary dir.
			#
			# This is a hack due to the distutils in debian/ubuntu's
			# python3 being misconfigured.
			# See discussion https://github.com/opencog/atomspace/issues/1782
			#
			# If the bug is fixed, most of this script could be replaced by:
			#
			# from distutils.sysconfig import get_python_lib;
			# print(get_python_lib(plat_specific=True, prefix=prefix))
			#
			# However, using this would not respect python virtual
			# environments, so the below is still better!
			FILE(WRITE ${PROJECT_BINARY_DIR}/scripts/get_python_lib.py
				"import sys\n"
				"import sysconfig\n"
				"import site\n"
				"\n"
				"if __name__ == '__main__':\n"
				"    prefix = sys.argv[1]\n"
				"\n"
				"    # Use sites if the prefix is recognized and the sites module is available\n"
				"    # (virtualenv is missing getsitepackages())\n"
				"    if hasattr(site, 'getsitepackages'):\n"
				"        paths = [p for p in site.getsitepackages() if p.startswith(prefix)]\n"
				"        if len(paths) == 1:\n"
				"            print(paths[0])\n"
				"            exit(0)\n"
				"    \n"
				"    # Use sysconfig platlib as the fall back\n"
				"    print(sysconfig.get_paths()['platlib'])\n"
				)

			# Find python destination dir for python bindings
			# because it may differ on each operating system.
			EXECUTE_PROCESS(
				COMMAND ${Python3_EXECUTABLE}
				"${PROJECT_BINARY_DIR}/scripts/get_python_lib.py"
				"${CMAKE_INSTALL_PREFIX}"
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
	# IF (Python3_Interpreter_FOUND AND Python3_Development_FOUND)
	IF (Python3_Interpreter_FOUND)
		FIND_PROGRAM(NOSETESTS_EXECUTABLE nosetests3)
	ENDIF ()
	IF (NOT NOSETESTS_EXECUTABLE)
		MESSAGE(STATUS "nosetests not found: needed for python tests")
	ENDIF ()
	IF (NOSETESTS_EXECUTABLE AND CYTHON_FOUND AND HAVE_PY_LIBS)
		SET(HAVE_NOSETESTS 1)
		MESSAGE(STATUS "Using nosetests executable " ${NOSETESTS_EXECUTABLE})
	ENDIF (NOSETESTS_EXECUTABLE AND CYTHON_FOUND AND HAVE_PY_LIBS)
ENDIF(HAVE_PY_INTERP)
