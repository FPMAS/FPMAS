set(SOURCES
	${SOURCES}
	# Utils
	utils/config.cpp
	utils/log.cpp
	# Communication
	communication/communication.cpp
	# Graph
	graph/parallel/zoltan/zoltan_utils.cpp
	# Scheduler
	scheduler/scheduler.cpp
	)

add_library(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
