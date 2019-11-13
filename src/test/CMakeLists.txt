
set(SOURCES "")

set(SOURCES
	${SOURCES}
	communication/communication.cpp)

set(SOURCES
	${SOURCES}
	environment/graph.cpp)

set(SOURCES
	${SOURCES}
	environment/serializers.cpp)

set(SOURCES
	${SOURCES}
	environment/nested_graph.cpp)


add_executable(fpmas_mpi_test "mpi_main.cpp" ${SOURCES})
target_link_libraries(fpmas_mpi_test fpmas)
target_link_libraries(fpmas_mpi_test gtest_main)

add_executable(fpmas_local_test "local_main.cpp" ${SOURCES})
target_link_libraries(fpmas_local_test fpmas)
target_link_libraries(fpmas_local_test gtest_main)

add_custom_target(local_test
	${CMAKE_CURRENT_BINARY_DIR}/fpmas_local_test
	"--gtest_filter=-Mpi_*"
	)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
