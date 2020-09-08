set(SOURCES
	# Comm
	communication/communication.cpp
	# Graph
	## Base
	graph/node.cpp
	graph/graph.cpp
	## Parallel
	graph/distributed_graph.cpp
	graph/distributed_edge.cpp
	graph/location_manager.cpp
	graph/zoltan_load_balancing.cpp
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

target_compile_definitions(fpmas_local_test PUBLIC LOG_LEVEL=${LOG_LEVEL})

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
