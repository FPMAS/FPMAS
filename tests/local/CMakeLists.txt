set(SOURCES
	# API
	api/graph/base/node.cpp
	# Comm
	communication/communication.cpp
	# Graph
	## Base
	graph/base/node.cpp
	graph/base/graph.cpp
	## Parallel
	graph/parallel/distributed_graph.cpp
	graph/parallel/distributed_arc.cpp
	graph/parallel/location_manager.cpp
	# Synchro
	synchro/ghost/data_sync.cpp
	synchro/ghost/sync_linker.cpp
	synchro/hard/hard_sync_mutex.cpp
	synchro/hard/mutex_client.cpp
	synchro/hard/mutex_server.cpp
	synchro/hard/termination.cpp
	synchro/hard/hard_sync_mode.cpp
	synchro/hard/link_client.cpp
	synchro/hard/link_server.cpp
	# Load balancing
	load_balancing/zoltan_load_balancing.cpp
	# Scheduler
	scheduler/job.cpp
	scheduler/epoch.cpp
	scheduler/scheduler.cpp
	# Runtime
	runtime/runtime.cpp
	# Model
	model/model.cpp
	model/serializer.cpp
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
