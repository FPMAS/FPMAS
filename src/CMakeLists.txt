set(SOURCES
	${SOURCES}
	# API
	fpmas/api/communication/communication.cpp
	fpmas/api/graph/distributed_id.cpp
	fpmas/api/model/model.cpp
	fpmas/synchro/hard/api/client_server.cpp
	# Utils
	fpmas/utils/log.cpp
	# Communication
	fpmas/communication/communication.cpp
	# Load balancing
	fpmas/graph/zoltan_load_balancing.cpp
	# Scheduler
	fpmas/scheduler/scheduler.cpp
	# Runtime
	fpmas/runtime/runtime.cpp
	# Model
	fpmas/model/model.cpp
	fpmas/model/serializer.cpp
	# Synchro
	fpmas/synchro/hard/termination.cpp
	# Main
	fpmas.cpp
	)

add_library(fpmas ${SOURCES})

target_compile_definitions(fpmas PUBLIC LOG_LEVEL=${LOG_LEVEL})
target_link_libraries(fpmas PUBLIC MPI::MPI_CXX)
target_link_libraries(fpmas PUBLIC zoltan)

target_include_directories(
	fpmas PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

install(TARGETS fpmas DESTINATION lib)
install(DIRECTORY . DESTINATION include FILES_MATCHING PATTERN "*.h")

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
