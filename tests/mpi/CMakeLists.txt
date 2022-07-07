set(SOURCES
	communication/communication.cpp
	graph/distributed_id.cpp
	graph/distributed_graph.cpp
	graph/location_manager.cpp
	graph/zoltan_load_balancing.cpp
	graph/graph_builder.cpp
	graph/analysis.cpp
	graph/ring_graph_builder.cpp
	graph/small_world_graph_builder.cpp
	synchro/ghost/ghost_mode.cpp
	synchro/hard/termination.cpp
	synchro/hard/hard_sync_mode.cpp
	model/model.cpp
	model/test_agents.cpp
	model/spatial/spatial_model.cpp
	model/spatial/spatial_agent_mapping.cpp
	model/spatial/graph_builder.cpp
	model/spatial/grid_agent_mapping.cpp
	model/spatial/grid.cpp
	model/spatial/graph_range.cpp
	model/spatial/grid_load_balancing.cpp
	model/spatial/cell_load_balancing.cpp
	random/generator.cpp
	random/random.cpp
	io/csv_output.cpp
	io/breakpoint.cpp
	io/output.cpp
	)


macro(set_up_mpi_test target serial_set_up)
	add_executable(${target} "mpi_main.cpp" ${SOURCES} ${serial_set_up})
	target_include_directories(${target} PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../mocks
		)
	target_link_libraries(${target} fpmas)
	target_link_libraries(${target} Zoltan::Zoltan)
	target_link_libraries(${target} gtest_main)
	target_link_libraries(${target} gmock_main)

	target_compile_definitions(${target} PUBLIC LOG_LEVEL=${LOG_LEVEL})
endmacro()

set_up_mpi_test(fpmas_mpi_test model/json_setup.cpp)
set_up_mpi_test(fpmas_mpi_test_datapack model/datapack_setup.cpp)
set_property(TARGET fpmas_mpi_test_datapack PROPERTY EXCLUDE_FROM_ALL YES)
