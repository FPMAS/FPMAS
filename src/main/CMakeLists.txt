#utils
set(SOURCES
	${SOURCES}
	utils/config.cpp
	)

set(SOURCES
	${SOURCES}
	utils/log.cpp
	)

# Communication
set(SOURCES
	${SOURCES}
	communication/communication.cpp
	)

set(SOURCES
	${SOURCES}
	communication/sync_communication.cpp
	)

set(SOURCES
	${SOURCES}
	communication/readers_writers.cpp
	)

# Graph
set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_utils.cpp
	)

# Proxy
set(SOURCES
	${SOURCES}
	graph/parallel/proxy/proxy.cpp)

add_library(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)
target_link_libraries(fpmas PUBLIC nlohmann_json::nlohmann_json)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
