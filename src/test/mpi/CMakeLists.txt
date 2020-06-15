set(SOURCES
	communication/communication.cpp
	graph/parallel/distributed_id.cpp
	graph/parallel/distributed_graph.cpp
	graph/parallel/location_manager.cpp
	synchro/hard/termination.cpp
	synchro/hard/hard_sync_mode.cpp
	load_balancing/zoltan_load_balancing.cpp
	model/model.cpp
	)

add_executable(fpmas_mpi_test "mpi_main.cpp" ${SOURCES})
target_include_directories(fpmas_mpi_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fpmas_mpi_test fpmas)
target_link_libraries(fpmas_mpi_test zoltan)
target_link_libraries(fpmas_mpi_test gtest_main)
target_link_libraries(fpmas_mpi_test gmock_main)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
