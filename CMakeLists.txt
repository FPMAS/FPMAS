cmake_minimum_required(VERSION 3.1O)

project(fpmas)

add_subdirectory(src/main)

# Setup googletest
add_subdirectory(src/test)
add_executable(fpmas_test src/test/main.cpp)
target_link_libraries(fpmas_test gtest_main)
add_test(NAME fpmas_test COMMAND fpmas_test)


# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_EXCLUDE src/main/zoltan)
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/src/main
		)
endif()


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
