set(SOURCES
	communication/communication.cpp
	graph/distributed_id.cpp
	graph/distributed_graph.cpp
	graph/location_manager.cpp
	graph/zoltan_load_balancing.cpp
	graph/graph_builder.cpp
	synchro/ghost/ghost_mode.cpp
	synchro/hard/termination.cpp
	synchro/hard/hard_sync_mode.cpp
	model/model.cpp
	model/test_agents.cpp
	model/spatial/spatial_model.cpp
	model/spatial/grid.cpp
	model/spatial/graph_range.cpp
	random/generator.cpp
	io/csv_output.cpp
	io/breakpoint.cpp
	io/output.cpp
	)

add_executable(fpmas_mpi_test "mpi_main.cpp" ${SOURCES})
target_include_directories(fpmas_mpi_test PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../mocks
	)
target_link_libraries(fpmas_mpi_test fpmas)
target_link_libraries(fpmas_mpi_test Zoltan::Zoltan)
target_link_libraries(fpmas_mpi_test gtest_main)
target_link_libraries(fpmas_mpi_test gmock_main)

target_compile_definitions(fpmas_mpi_test PUBLIC LOG_LEVEL=${LOG_LEVEL})
