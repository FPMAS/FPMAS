cmake_minimum_required(VERSION 3.1O)

project(fpmas)

add_subdirectory(googletest)

set(CMAKE_CXX_STANDARD 17)

# MPI setup
find_package(MPI REQUIRED)
set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
add_compile_options(-g -O0)

set(N_PROC ${MPIEXEC_MAX_NUMPROCS} CACHE STRING "Number of procs to use in the
MPI context")
set(LOG_LEVEL 0 CACHE STRING "Log level")
add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})

message("-- Number of used procs : ${N_PROC}")
if(N_PROC GREATER MPIEXEC_MAX_NUMPROCS)
	message("-- Enabling hardware threads cpus")
	set(MPIEXEC_PREFLAGS "--use-hwthread-cpus")
endif()

if(MPIEXEC_EXECUTABLE)
	message("-- Found mpiexec : ${MPIEXEC_EXECUTABLE}")
	add_custom_target(run
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${N_PROC}
		${MPIEXEC_PREFLAGS}
		${CMAKE_CURRENT_BINARY_DIR}/src/main/fpmas
		${MPIEXEC_POSTFLAGS}
		)

	add_custom_target(mpi_test
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${N_PROC}
		${MPIEXEC_PREFLAGS}
		${CMAKE_CURRENT_BINARY_DIR}/src/test/mpi/fpmas_mpi_test
		"--gtest_filter=Mpi_*"
		${MPIEXEC_POSTFLAGS}
		)

	add_custom_target(mpi_memcheck
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${N_PROC}
		${MPIEXEC_PREFLAGS}
		"valgrind"
		"--suppressions=../mpi_init.supp"
		"--leak-check=full"
		"--show-leak-kinds=all"
		${CMAKE_CURRENT_BINARY_DIR}/src/test/mpi/fpmas_mpi_test
		"--gtest_filter=Mpi_*"
		${MPIEXEC_POSTFLAGS}
		)

endif()

find_package(nlohmann_json REQUIRED) 

# Builds the Zoltan library
add_subdirectory(zoltan)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/zoltan
	${CMAKE_CURRENT_SOURCE_DIR}/zoltan/include
	)

add_subdirectory(src/main)
add_subdirectory(src/test)

add_subdirectory(examples)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_MACRO_EXPANSION "YES")
	set(DOXYGEN_HTML_OUTPUT ".")
	#set(DOXYGEN_HAVE_DOT "YES")
	#set(DOXYGEN_TEMPLATE_RELATIONS "YES")
	# set(DOXYGEN_EXCLUDE src/main/zoltan)
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/src/main
		)
endif()


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
