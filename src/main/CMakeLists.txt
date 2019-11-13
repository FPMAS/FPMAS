
# Builds the Zoltan library
add_subdirectory(zoltan)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/zoltan
	${CMAKE_CURRENT_SOURCE_DIR}/zoltan/include
	)

# Main
set(SOURCES fpmas.cpp)

# Communication
set(SOURCES
	${SOURCES}
	communication/communication.cpp
	)

# Graph
set(SOURCES
	${SOURCES}
	environment/graph.cpp
	)

add_library(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)
target_link_libraries(fpmas PUBLIC nlohmann_json::nlohmann_json)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
