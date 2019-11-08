
add_executable(fpmas_test main.cpp)
target_link_libraries(fpmas_test gtest_main)

add_custom_target(test
	${CMAKE_CURRENT_BINARY_DIR}/fpmas_test
	)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
