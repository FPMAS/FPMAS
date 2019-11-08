
set(SOURCES "main.cpp")

set(SOURCES
	${SOURCES}
	communication/communication.cpp)

add_executable(fpmas_test ${SOURCES})

target_link_libraries(fpmas_test fpmas)
target_link_libraries(fpmas_test gtest_main)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
