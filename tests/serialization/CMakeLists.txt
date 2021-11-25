
macro(set_up_serial_test target source)
	add_executable(${target} ${source})
	target_include_directories(${target} PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../mocks
		)
	target_link_libraries(${target} fpmas)
	target_link_libraries(${target} Zoltan::Zoltan)
	target_link_libraries(${target} gtest_main)
	target_link_libraries(${target} gmock_main)
	target_compile_definitions(${target} PUBLIC LOG_LEVEL=${LOG_LEVEL})
endmacro()

set_up_serial_test(fpmas_agent_json_test json.cpp)
set_up_serial_test(fpmas_agent_datapack_test datapack.cpp)
set_up_serial_test(fpmas_agent_json_datapack_test json_datapack.cpp)

