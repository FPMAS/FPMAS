set(SOURCES
	${SOURCES}
	# API
	api/model/model.cpp
	# Utils
	utils/log.cpp
	# Communication
	communication/communication.cpp
	# Load balancing
	load_balancing/zoltan_load_balancing.cpp
	# Scheduler
	scheduler/scheduler.cpp
	# Runtime
	runtime/runtime.cpp
	# Model
	model/model.cpp
	)

add_library(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC zoltan)

target_include_directories(
	fpmas
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
