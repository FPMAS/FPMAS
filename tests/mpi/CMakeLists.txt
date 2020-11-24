set(SOURCES
	communication/communication.cpp
	graph/distributed_id.cpp
	graph/distributed_graph.cpp
	graph/location_manager.cpp
	graph/zoltan_load_balancing.cpp
	synchro/ghost/ghost_mode.cpp
	synchro/hard/termination.cpp
	synchro/hard/hard_sync_mode.cpp
	model/model.cpp
	model/spatial_model.cpp
	model/test_agents.cpp
	model/grid.cpp
	)

add_executable(fpmas_mpi_test "mpi_main.cpp" ${SOURCES})
target_include_directories(fpmas_mpi_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fpmas_mpi_test fpmas)
target_link_libraries(fpmas_mpi_test Zoltan::Zoltan)
target_link_libraries(fpmas_mpi_test gtest_main)
target_link_libraries(fpmas_mpi_test gmock_main)

target_compile_definitions(fpmas_mpi_test PUBLIC LOG_LEVEL=${LOG_LEVEL})

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
