set(SOURCES
	# API
	api/graph/base/node.cpp
	# Comm
	communication/communication.cpp
	# Graph
	## Base
	graph/base/basic_node.cpp
	graph/base/basic_graph.cpp
	## Parallel
	graph/parallel/basic_distributed_graph.cpp
	graph/parallel/location_manager.cpp
	### Synchro
	graph/parallel/synchro/ghost/data_sync.cpp
	graph/parallel/synchro/ghost/sync_linker.cpp
	graph/parallel/synchro/hard/hard_sync_mutex.cpp
	graph/parallel/synchro/hard/mutex_client.cpp
	graph/parallel/synchro/hard/mutex_server.cpp
	graph/parallel/synchro/hard/termination.cpp
	graph/parallel/synchro/hard/hard_sync_mode.cpp
	)

add_executable(fpmas_local_test "local_main.cpp" ${SOURCES})
target_include_directories(fpmas_local_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fpmas_local_test fpmas)
target_link_libraries(fpmas_local_test zoltan)
target_link_libraries(fpmas_local_test gtest_main)
target_link_libraries(fpmas_local_test gmock_main)

add_custom_target(local_test
	${CMAKE_CURRENT_BINARY_DIR}/fpmas_local_test
	"--gtest_filter=-Mpi_*"
	)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
