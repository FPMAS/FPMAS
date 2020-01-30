# Sync modes comparison
add_executable(sync_modes sync_modes.cpp)
target_link_libraries(sync_modes PUBLIC fpmas)


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
