add_subdirectory(local)
add_subdirectory(mpi)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
