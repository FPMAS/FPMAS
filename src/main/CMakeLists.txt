
# MPI setup
find_package(MPI REQUIRED)
set(CMAKE_CXX_COMPILER mpicxx)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILER_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS, "${CMAKE_CXX_LINK_FLAGS} ${MPI_CXX_LINK_FLAGS}")

if(MPIEXEC_EXECUTABLE)
	message("-- Found mpiexec : ${MPIEXEC_EXECUTABLE}")
	add_custom_target(run
		${MPIEXEC_EXECUTABLE}
		${MPIEXEC_NUMPROC_FLAG}
		${MPIEXEC_MAX_NUMPROCS}
		${MPIEXEC_PREFLAGS}
		${CMAKE_CURRENT_BINARY_DIR}/fpmas
		${MPIEXEC_POSTFLAGS}
		)
endif()

# Builds the Zoltan library
add_subdirectory(zoltan)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/zoltan
	${CMAKE_CURRENT_SOURCE_DIR}/zoltan/include
	)

# Main
set(SOURCES fpmas.cpp)

# Communication
set(SOURCES
	${SOURCES}
	communication/communication.cpp
	)

add_executable(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

