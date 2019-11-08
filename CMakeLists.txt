cmake_minimum_required(VERSION 3.1O)

project(fpmas)

add_subdirectory(googletest)

# MPI setup
find_package(MPI REQUIRED)
set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})

if(MPIEXEC_EXECUTABLE)
	message("-- Found mpiexec : ${MPIEXEC_EXECUTABLE}")
	add_custom_target(run
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${MPIEXEC_MAX_NUMPROCS}
		${MPIEXEC_PREFLAGS}
		${CMAKE_CURRENT_BINARY_DIR}/src/main/fpmas
		${MPIEXEC_POSTFLAGS}
		)

	add_custom_target(test
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${MPIEXEC_MAX_NUMPROCS}
		${MPIEXEC_PREFLAGS}
		${CMAKE_CURRENT_BINARY_DIR}/src/test/fpmas_test
		${MPIEXEC_POSTFLAGS}
		)

endif()

add_subdirectory(src/main)

add_subdirectory(src/test)


# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_EXCLUDE src/main/zoltan)
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/src/main
		)
endif()


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
