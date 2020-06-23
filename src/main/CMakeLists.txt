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
	fpmas PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)

install(TARGETS fpmas DESTINATION lib)
install(FILES
	# API
	api/communication/communication.h
	api/graph/base/arc.h
	api/graph/base/graph.h
	api/graph/base/id.h
	api/graph/base/node.h
	api/graph/parallel/distributed_arc.h
	api/graph/parallel/distributed_graph.h
	api/graph/parallel/distributed_node.h
	api/graph/parallel/location_manager.h
	api/graph/parallel/location_state.h
	api/load_balancing/load_balancing.h
	api/model/model.h
	api/runtime/runtime.h
	api/scheduler/scheduler.h
	api/synchro/mutex.h
	api/synchro/sync_mode.h
	api/synchro/hard/client_server.h
	api/synchro/hard/enums.h
	api/synchro/hard/hard_sync_mutex.h
	api/utils/callback.h
	api/utils/ptr_wrapper.h

	# Implementation
	communication/communication.h

	graph/base/arc.h
	graph/base/graph.h
	graph/base/node.h

	graph/parallel/distributed_arc.h
	graph/parallel/distributed_graph.h
	graph/parallel/distributed_id.h
	graph/parallel/distributed_node.h
	graph/parallel/location_manager.h

	load_balancing/zoltan_load_balancing.h
	load_balancing/scheduled_load_balancing.h

	model/model.h
	model/serializer.h

	runtime/runtime.h

	scheduler/scheduler.h

	synchro/ghost/ghost_mode.h

	synchro/hard/data_update_pack.h
	synchro/hard/hard_data_sync.h
	synchro/hard/hard_sync_linker.h
	synchro/hard/hard_sync_mode.h
	synchro/hard/hard_sync_mutex.h
	synchro/hard/mutex_client.h
	synchro/hard/mutex_server.h
	synchro/hard/server_pack.h
	synchro/hard/termination.h

	utils/log.h
	utils/macros.h
	utils/callback.h

	DESTINATION include/fpmas)


# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
