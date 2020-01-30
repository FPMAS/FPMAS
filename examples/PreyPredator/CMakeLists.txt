# Prey predator
add_executable(prey_predator prey_predator.cpp main.cpp)
target_include_directories(
	prey_predator
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

target_link_libraries(prey_predator PUBLIC fpmas)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
