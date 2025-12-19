# -------------------------------------------------
# OpenCog Testing Configuration
#
# This module provides common CxxTest setup for all OpenCog projects.
# It finds CxxTest, sets up the testing infrastructure, and provides
# helper macros for creating test targets.
#
# Usage in downstream projects:
#   INCLUDE(OpenCogTestOptions)
#   ... add subdirectories for main code ...
#   OPENCOG_SETUP_TESTING()   # Sets up tests target and check target
#   # Optionally add project-specific test targets:
#   OPENCOG_ADD_TEST_TARGET(test_foo tests/foo "Running foo tests...")

# -------------------------------------------------
# Find CxxTest

FIND_PACKAGE(Cxxtest)
IF (CXXTEST_FOUND)
	MESSAGE(STATUS "CxxTest found.")
ELSE (CXXTEST_FOUND)
	MESSAGE(STATUS "CxxTest missing: needed for unit tests.")
ENDIF (CXXTEST_FOUND)

# -------------------------------------------------
# Main setup macro - call this after ADD_SUBDIRECTORY for main code

MACRO(OPENCOG_SETUP_TESTING)
	IF (CXXTEST_FOUND)
		# Enable CTest support so that 'ctest' command works
		ENABLE_TESTING()

		# Create the 'tests' target that all test executables depend on
		ADD_CUSTOM_TARGET(tests)

		# Add tests subdirectory (excluded from default 'all' target)
		ADD_SUBDIRECTORY(tests EXCLUDE_FROM_ALL)

		# Create the 'check' target with coverage support
		IF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
			# Coverage build: run tests with coverage collection
			ADD_CUSTOM_TARGET(check
				WORKING_DIRECTORY tests
				COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure $(ARGS)
				COMMENT "Running tests with coverage..."
			)
		ELSE (CMAKE_BUILD_TYPE STREQUAL "Coverage")
			# Normal build: just run tests
			ADD_CUSTOM_TARGET(check
				DEPENDS tests
				WORKING_DIRECTORY tests
				COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure $(ARGS)
				COMMENT "Running tests..."
			)
		ENDIF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
	ENDIF (CXXTEST_FOUND)
ENDMACRO(OPENCOG_SETUP_TESTING)

# -------------------------------------------------
# Helper macro to add project-specific test targets
#
# Usage: OPENCOG_ADD_TEST_TARGET(target_name working_directory comment)
# Example: OPENCOG_ADD_TEST_TARGET(test_query tests/query "Running query tests...")

MACRO(OPENCOG_ADD_TEST_TARGET _target_name _working_dir _comment)
	IF (CXXTEST_FOUND)
		ADD_CUSTOM_TARGET(${_target_name}
			DEPENDS tests
			WORKING_DIRECTORY ${_working_dir}
			COMMAND ${CMAKE_CTEST_COMMAND} $(ARGS)
			COMMENT ${_comment}
		)
	ENDIF (CXXTEST_FOUND)
ENDMACRO(OPENCOG_ADD_TEST_TARGET)
