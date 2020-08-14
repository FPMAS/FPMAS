cmake_minimum_required(VERSION 3.1O)

project(fpmas)

# MPI setup
find_package(MPI REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

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
	
	add_custom_target(mpi_test
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${N_PROC}
		${MPIEXEC_PREFLAGS}
		${CMAKE_CURRENT_BINARY_DIR}/tests/mpi/fpmas_mpi_test
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
		${CMAKE_CURRENT_BINARY_DIR}/tests/mpi/fpmas_mpi_test
		${MPIEXEC_POSTFLAGS}
		)

endif()

# Nlohmann JSON library
find_file(JSON nlohmann/json.hpp) 
if(NOT JSON)
	message(FATAL_ERROR "-- nlohman/json library not found.")
else()
	message("-- Found nlohman/json library : ${JSON}")
endif()

# Zoltan library
find_library(ZOLTAN zoltan)
if(NOT ZOLTAN)
	message(FATAL_ERROR "-- Zoltan library not found.")
else()
	message("-- Found zoltan library : ${ZOLTAN}")
endif()

add_subdirectory(src)
add_subdirectory(tests)

#add_subdirectory(examples)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_MACRO_EXPANSION "YES")
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/img)
	set(DOXYGEN_ALIASES
		[[Agents="\ref fpmas::api::model::Agent \"Agents\""]]
		[[AgentGroups="\ref fpmas::api::model::AgentGroup \"AgentGroups\""]]
		[[AgentNode="\ref fpmas::api::model::AgentNode \"AgentNode\""]]
		[[AgentGraph="\ref fpmas::api::model::AgentGraph \"AgentGraph\""]]
		[[AgentTasks="\ref fpmas::api::model::AgentTask \"AgentTasks\""]]
		[[Job="\ref fpmas::api::scheduler::Job \"Job\""]]
		)
	#set(DOXYGEN_HAVE_DOT "YES")
	#set(DOXYGEN_TEMPLATE_RELATIONS "YES")
	set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/src
		)
endif()

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
