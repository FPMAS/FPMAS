
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
	graph/base/graph_item.cpp
	)

set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_utils.cpp
	)

# Proxy
set(SOURCES
	${SOURCES}
	graph/parallel/proxy/proxy.cpp)

# Agent
set(SOURCES
	${SOURCES}
	agent/agent_base.cpp
	agent/agent_container.cpp
	)

add_library(fpmas ${SOURCES})
add_executable(fpmas_main ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)
target_link_libraries(fpmas_main PUBLIC zoltan)
target_link_libraries(fpmas PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(fpmas_main PUBLIC nlohmann_json::nlohmann_json)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

target_include_directories(
	fpmas_main
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)
# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
