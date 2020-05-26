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

# Graph
set(SOURCES
	${SOURCES}
	graph/parallel/zoltan/zoltan_utils.cpp
	)

add_library(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
