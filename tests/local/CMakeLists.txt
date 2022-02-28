set(SOURCES
	# API
	api/graph/basic_id.cpp
	api/communication/communication.cpp
	# Comm
	communication/communication.cpp
	# Graph
	## Base
	graph/node.cpp
	graph/graph.cpp
	## Parallel
	graph/distributed_graph.cpp
	graph/distributed_node.cpp
	graph/distributed_edge.cpp
	graph/location_manager.cpp
	graph/zoltan_load_balancing.cpp
	graph/random_load_balancing.cpp
	graph/graph_builder.cpp
	# Synchro
	synchro/data_update_pack.cpp
	synchro/ghost/data_sync.cpp
	synchro/ghost/sync_linker.cpp
	synchro/ghost/global_ghost_mode.cpp
	synchro/hard/hard_sync_mutex.cpp
	synchro/hard/mutex_client.cpp
	synchro/hard/mutex_server.cpp
	synchro/hard/termination.cpp
	synchro/hard/hard_data_sync.cpp
	synchro/hard/hard_sync_linker.cpp
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
	model/behavior.cpp
	model/spatial/spatial_model.cpp
	model/spatial/grid.cpp
	model/spatial/graph.cpp
	model/spatial/grid_load_balancing.cpp
	model/spatial/grid_agent_mapping.cpp
	model/spatial/test_agents.cpp
	# Output
	io/fake_data.cpp
	io/output.cpp
	io/csv_output.cpp
	io/json_output.cpp
	io/breakpoint.cpp
	io/json.cpp
	io/datapack.cpp
	io/json_datapack.cpp
	io/formatted_output.cpp
	# Utils
	utils/format.cpp
	utils/perf.cpp
	utils/functional.cpp
	utils/cast.cpp
	)

add_executable(fpmas_local_test "local_main.cpp" ${SOURCES})
target_include_directories(fpmas_local_test PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../mocks
	)
target_link_libraries(fpmas_local_test fpmas)
target_link_libraries(fpmas_local_test Zoltan::Zoltan)
target_link_libraries(fpmas_local_test gtest_main)
target_link_libraries(fpmas_local_test gmock_main)

target_compile_definitions(fpmas_local_test PUBLIC LOG_LEVEL=${LOG_LEVEL})
