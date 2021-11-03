configure_file(fpmas.h.in ${CMAKE_CURRENT_SOURCE_DIR}/fpmas.h)

set(SOURCES
	${SOURCES}
	# API
	fpmas/api/communication/communication.cpp
	fpmas/api/graph/distributed_id.cpp
	fpmas/api/model/model.cpp
	fpmas/api/model/spatial/grid.cpp
	fpmas/synchro/hard/api/client_server.cpp
	fpmas/api/scheduler/scheduler.cpp
	# Utils
	fpmas/utils/log.cpp
	fpmas/utils/format.cpp
	fpmas/utils/perf.cpp
	# Communication
	fpmas/communication/communication.cpp
	# IO
	fpmas/io/datapack_base_io.cpp
	# Graph 
	fpmas/graph/zoltan_load_balancing.cpp
	fpmas/graph/graph_builder.cpp
	# Scheduler
	fpmas/scheduler/scheduler.cpp
	# Runtime
	fpmas/runtime/runtime.cpp
	# Model
	fpmas/model/detail/model.cpp
	fpmas/model/model.cpp
	fpmas/model/serializer.cpp
	fpmas/model/json_serializer.cpp
	fpmas/model/spatial/spatial_model.cpp
	fpmas/model/spatial/grid.cpp
	fpmas/model/spatial/spatial_agent_mapping.cpp
	fpmas/model/spatial/grid_agent_mapping.cpp
	fpmas/model/spatial/graph.cpp
	fpmas/model/spatial/grid_load_balancing.cpp
	fpmas/model/spatial/cell_load_balancing.cpp
	# Random
	fpmas/random/random.cpp
	# Synchro
	fpmas/synchro/hard/termination.cpp
	# Output
	fpmas/io/output.cpp
	# Main
	fpmas.cpp
	)

add_library(fpmas ${SOURCES})

target_compile_definitions(fpmas PUBLIC LOG_LEVEL=${LOG_LEVEL})
target_compile_definitions(fpmas PUBLIC FPMAS_ID_TYPE=${FPMAS_ID_TYPE})
target_link_libraries(fpmas PUBLIC MPI::MPI_CXX Zoltan::Zoltan
	nlohmann_json::nlohmann_json)

target_include_directories(fpmas PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

install(TARGETS fpmas EXPORT fpmas_targets
	DESTINATION lib
	INCLUDES DESTINATION include)

install(DIRECTORY . DESTINATION include FILES_MATCHING PATTERN "*.h")

export(EXPORT fpmas_targets
	NAMESPACE fpmas::)
install(EXPORT fpmas_targets
	DESTINATION lib/cmake/fpmas
	NAMESPACE fpmas::)
