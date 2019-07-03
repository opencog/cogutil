# ADD_CXXTEST macro from http://www.cmake.org/Wiki/CMakeMacroAddCxxTest
FIND_PACKAGE(Cxxtest)

IF (CXXTEST_FOUND)

	#ADD_CONFIGURABLE_CXXTEST declares a configurable cxxtest. If the executable
	#is to be compiled then it needs to be added as a dependency to the target
	#'tests' by declaring 'ADD_DEPENDENCIES(tests NAME)'. To run the executable
	#by ctest using a configuration script, declare as follows
	#ADD_TEST(NAME COMMAND configuration_and_run_script.sh)

	MACRO(ADD_CONFIGURABLE_CXXTEST NAME)
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cpp
			COMMAND
				${CXXTEST_GEN}
				--runner=ErrorPrinter
				--have-eh
				-o ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.cxxtest
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.cxxtest ${ARGN}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)

		ADD_EXECUTABLE(${NAME} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cpp ${ARGN})
	ENDMACRO(ADD_CONFIGURABLE_CXXTEST)

	MACRO(ADD_CXXTEST NAME)
		ADD_CONFIGURABLE_CXXTEST(${NAME} ${ARGN})
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CXXTEST_INCLUDE_DIRS})

        IF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
            ADD_TEST(${NAME} ${PROJECT_SOURCE_DIR}/scripts/run_lcov.sh
                ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${NAME})
        ELSE (CMAKE_BUILD_TYPE STREQUAL "Coverage")
            ADD_TEST(${NAME} ${NAME})
		ENDIF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		IF (GUILE_LOAD_PATH)
			SET_PROPERTY(TEST ${NAME} PROPERTY ENVIRONMENT "GUILE_LOAD_PATH=${GUILE_LOAD_PATH}")
		ENDIF(GUILE_LOAD_PATH)
		ADD_DEPENDENCIES(tests ${NAME})
	ENDMACRO(ADD_CXXTEST)

	#The above macro generates a single source file for all input test headers.
	#If by some reason you prefer separate compilation of each part, you may use
	#the variation. This doesn't compile the target one have to call 'make NAME'
	#before 'make test', or add it as a dependency to 'tests' target by
	#declaring 'ADD_DEPENDENCIES(tests NAME)'.

	MACRO(ADD_CXXTEST_SEP NAME)
		# generate the parts
		FOREACH(_PART ${ARGN})
			GET_FILENAME_COMPONENT(_NAME ${_PART} NAME)
			GET_FILENAME_COMPONENT(_NAME_WE ${_PART} NAME_WE)
			ADD_CUSTOM_COMMAND(
				OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_NAME_WE}.cpp
				COMMAND
					${CXXTEST_GEN}
					--part
					--have-eh
					-o ${CMAKE_CURRENT_BINARY_DIR}/${_NAME_WE}.cpp
					${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.cxxtest
				DEPENDS ${_PART}
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			)
		ENDFOREACH(_PART)

		# generate the runner
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_runner.cpp
			COMMAND
				${CXXTEST_GEN}
				--runner=ErrorPrinter --root
				--have-eh
				-o ${NAME}_runner.cpp
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)

		# enumerate all generated files
		SET(PARTS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_runner.cpp)
		FOREACH(_PART ${ARGN})
			GET_FILENAME_COMPONENT(_PART_WE ${_PART} NAME_WE)
			SET(PARTS ${PARTS} ${_PART_WE}.cpp)
		ENDFOREACH(_PART)

		ADD_EXECUTABLE(${NAME} ${PARTS})

		ADD_TEST(${NAME} ${NAME})
		IF (GUILE_LOAD_PATH)
			SET_PROPERTY(TEST ${NAME} PROPERTY ENVIRONMENT "GUILE_LOAD_PATH=${GUILE_LOAD_PATH}")
		ENDIF(GUILE_LOAD_PATH)
	ENDMACRO(ADD_CXXTEST_SEP)

ENDIF(CXXTEST_FOUND)
