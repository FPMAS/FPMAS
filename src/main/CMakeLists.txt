
# Main
set(SOURCES fpmas.cpp)

#utils
set(SOURCES
	${SOURCES}
	utils/config.cpp
	)

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

# Agent
set(SOURCES
	${SOURCES}
	agent/agent_base.cpp
	agent/agent_container.cpp
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
