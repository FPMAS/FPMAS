
set(SOURCES "")

# Comm
set(SOURCES
	${SOURCES}
	communication/communication.cpp)

# Graph
set(SOURCES
	${SOURCES}
	environment/graph/base/graph.cpp)

# Graph serialization
set(SOURCES
	${SOURCES}
	environment/graph/base/serializers.cpp)

set(SOURCES
	${SOURCES}
	environment/graph/nested_graph.cpp)

# Parallel graph
## Zoltan
set(SOURCES
	${SOURCES}
	environment/graph/parallel/zoltan/zoltan_lb.cpp)

set(SOURCES
	${SOURCES}
	environment/graph/parallel/zoltan/zoltan_node_migrate.cpp)

set(SOURCES
	${SOURCES}
	environment/graph/parallel/zoltan/zoltan_arc_migrate.cpp)

set(SOURCES
	${SOURCES}
	environment/graph/parallel/zoltan/zoltan_ghost_node_migrate.cpp)

## Graph
set(SOURCES
	${SOURCES}
	environment/graph/parallel/distributed_graph.cpp)

set(SOURCES
	${SOURCES}
	environment/graph/parallel/olz.cpp)

set(SOURCES
	${SOURCES}
	environment/graph/parallel/olz_graph.cpp)

## Proxy
set(SOURCES
	${SOURCES}
	environment/graph/parallel/proxy.cpp)

add_executable(fpmas_mpi_test "mpi_main.cpp" ${SOURCES})

target_include_directories(fpmas_mpi_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fpmas_mpi_test fpmas)
target_link_libraries(fpmas_mpi_test zoltan)
target_link_libraries(fpmas_mpi_test gtest_main)

add_executable(fpmas_local_test "local_main.cpp" ${SOURCES})
target_include_directories(fpmas_local_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fpmas_local_test fpmas)
target_link_libraries(fpmas_local_test zoltan)
target_link_libraries(fpmas_local_test gtest_main)

add_custom_target(local_test
	${CMAKE_CURRENT_BINARY_DIR}/fpmas_local_test
	"--gtest_filter=-Mpi_*"
	)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
