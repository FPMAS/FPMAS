set(SOURCES
	${SOURCES}
	# API
	fpmas/api/graph/parallel/distributed_id.cpp
	fpmas/api/model/model.cpp
	fpmas/api/synchro/hard/client_server.cpp
	# Utils
	fpmas/utils/log.cpp
	# Communication
	fpmas/communication/communication.cpp
	# Load balancing
	fpmas/load_balancing/zoltan_load_balancing.cpp
	# Scheduler
	fpmas/scheduler/scheduler.cpp
	# Runtime
	fpmas/runtime/runtime.cpp
	# Model
	fpmas/model/model.cpp
	)

add_library(fpmas ${SOURCES})

target_link_libraries(fpmas PUBLIC MPI::MPI_CXX)
target_link_libraries(fpmas PUBLIC zoltan)

target_include_directories(
	fpmas PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

install(TARGETS fpmas DESTINATION lib)
install(DIRECTORY . DESTINATION include FILES_MATCHING PATTERN "*.h")

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
