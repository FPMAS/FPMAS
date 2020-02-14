
set(SOURCES "")

# Comm
set(SOURCES
	${SOURCES}
	communication/communication.cpp)

set(SOURCES
	${SOURCES}
	communication/sync_communication.cpp)

set(SOURCES
	${SOURCES}
	communication/readers_writers.cpp)

# Graph
set(SOURCES
	${SOURCES}
	graph/base/graph.cpp)

set(SOURCES
	${SOURCES}
	graph/base/multi_graph.cpp)

# Graph serialization
set(SOURCES
	${SOURCES}
	graph/base/serializers.cpp)

# Parallel graph
## Zoltan
set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_lb.cpp)

set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_node_migrate.cpp)

set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_arc_migrate.cpp)

set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_ghost_node_migrate.cpp)

## Graph
set(SOURCES
	${SOURCES}
	graph/parallel/distributed_graph.cpp
	graph/parallel/distributed_graph_edge_cases.cpp
	)

set(SOURCES
	${SOURCES}
	graph/parallel/olz.cpp)

set(SOURCES
	${SOURCES}
	graph/parallel/olz_graph.cpp)

### synchro
set(SOURCES
	${SOURCES}
	graph/parallel/synchro/none.cpp)

set(SOURCES
	${SOURCES}
	graph/parallel/synchro/ghost_data.cpp)

set(SOURCES
	${SOURCES}
	graph/parallel/synchro/hard_sync_data.cpp)

## Proxy
set(SOURCES
	${SOURCES}
	graph/parallel/proxy/proxy.cpp)

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
