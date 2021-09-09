cmake_minimum_required(VERSION 3.10)

project(fpmas VERSION 1.1.1)

# Custom modules directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## Dependecies
# MPI package
find_package(MPI 3 REQUIRED)

# Custom Zoltan module
find_package(Zoltan 3.81 REQUIRED)

# Since only the single include json.hpp is required, we defined a custom
# nlohmann_json module so that a "complete" installation of nlohmann_json is
# not required, while still allowing version management. If the "complete"
# installation is used, find_package() will automatically load the
# nlohmann_json Config.cmake.
find_package(nlohmann_json 3.7 REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG
	"${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -O3\
	-Wall -Wextra -Wpedantic -Werror -Wno-error=pedantic\
	-Woverloaded-virtual -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

if(CMAKE_BUILD_TYPE STREQUAL Debug)
	message("-- Build type: Debug (${CMAKE_CXX_FLAGS_DEBUG})")
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
	message("-- Build type: Release (${CMAKE_CXX_FLAGS_RELEASE})")
elseif(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
	message("-- Build type: RelWithDebInfo (${CMAKE_CXX_FLAGS_RELWITHDEBINFO})")
endif()


set(LOG_LEVEL FPMAS_LOG_WARN CACHE STRING "Log level")
set(FPMAS_ID_TYPE "unsigned long" CACHE STRING "FPMAS ID type (unsigned)")
message("-- LOG_LEVEL : ${LOG_LEVEL}")
message("-- FPMAS_ID_TYPE : ${FPMAS_ID_TYPE}")

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples EXCLUDE_FROM_ALL)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/fpmasConfigVersion.cmake
	COMPATIBILITY SameMajorVersion)

# CMake packaging configuration
install(FILES
	cmake/fpmasConfig.cmake
	cmake/FindZoltan.cmake
	cmake/Findnlohmann_json.cmake
	${CMAKE_CURRENT_BINARY_DIR}/fpmasConfigVersion.cmake
	DESTINATION lib/cmake/fpmas)

# "find_version" utilities need to be installed with custom modules
install(DIRECTORY
	cmake/find_zoltan_version
	DESTINATION lib/cmake/fpmas)
install(DIRECTORY
	cmake/find_nlohmann_json_version
	DESTINATION lib/cmake/fpmas)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	#set(DOXYGEN_MACRO_EXPANSION "YES")
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/docs/img)
	#set(DOXYGEN_CLANG_ASSISTED_PARSING "YES")
	#set(DOXYGEN_CLANG_DATABASE_PATH "build/compile_commands.json")
	set(DOXYGEN_SORT_MEMBER_DOCS "NO")
	set(DOXYGEN_PREDEFINED "DOXYGEN_BUILD")
	set(DOXYGEN_ALIASES
		[[FPMAS_ID_TYPE="\ref FPMAS_ID_TYPE"]]
		[[LOCAL="\ref fpmas::api::graph::LocationState \"LOCAL\""]]
		[[DISTANT="\ref fpmas::api::graph::LocationState \"DISTANT\""]]
		[[light_json="\ref fpmas::io::json::light_json \"light_json\""]]
		[[DistributedId="\ref fpmas::api::graph::DistributedId \"DistributedId\""]]
		[[DistributedGraph="\ref fpmas::api::graph::DistributedGraph \"DistributedGraph\""]]
		[[DistributedNode="\ref fpmas::api::graph::DistributedNode \"DistributedNode\""]]
		[[DistributedNodes="\ref fpmas::api::graph::DistributedNode \"DistributedNodes\""]]
		[[DistributedEdge="\ref fpmas::api::graph::DistributedEdge \"DistributedEdge\""]]
		[[DistributedEdges="\ref fpmas::api::graph::DistributedEdge \"DistributedEdges\""]]
		[[LoadBalancing="\ref fpmas::api::graph::LoadBalancing \"LoadBalancing\""]]
		[[SyncMode="\ref fpmas::api::synchro::SyncMode \"SyncMode\""]]
		[[Agent="\ref fpmas::api::model::Agent \"Agent\""]]
		[[Agents="\ref fpmas::api::model::Agent \"Agents\""]]
		[[SpatialAgent="\ref fpmas::api::model::SpatialAgent \"SpatialAgent\""]]
		[[SpatialAgents="\ref fpmas::api::model::SpatialAgent \"SpatialAgents\""]]
		[[SpatialModel="\ref fpmas::api::model::SpatialModel \"SpatialModel\""]]
		[[SpatialModels="\ref fpmas::api::model::SpatialModel \"SpatialModels\""]]
		[[GridAgent="\ref fpmas::api::model::GridAgent \"GridAgent\""]]
		[[GridAgents="\ref fpmas::api::model::GridAgent \"GridAgents\""]]
		[[Cell="\ref fpmas::api::model::Cell \"Cell\""]]
		[[Cells="\ref fpmas::api::model::Cell \"Cells\""]]
		[[GridCell="\ref fpmas::api::model::GridCell \"GridCell\""]]
		[[GridCells="\ref fpmas::api::model::GridCell \"GridCells\""]]
		[[AgentGroup="\ref fpmas::api::model::AgentGroup \"AgentGroup\""]]
		[[AgentGroups="\ref fpmas::api::model::AgentGroup \"AgentGroups\""]]
		[[Behavior="\ref fpmas::api::model::Behavior \"Behavior\""]]
		[[Behaviors="\ref fpmas::api::model::Behavior \"Behaviors\""]]
		[[AgentNode="\ref fpmas::api::model::AgentNode \"AgentNode\""]]
		[[AgentGraph="\ref fpmas::api::model::AgentGraph \"AgentGraph\""]]
		[[AgentTask="\ref fpmas::api::model::AgentTask \"AgentTask\""]]
		[[AgentTasks="\ref fpmas::api::model::AgentTask \"AgentTasks\""]]
		[[AgentNode="\ref fpmas::api::model::AgentNode \"AgentNode\""]]
		[[Model="\ref fpmas::api::model::Model \"Model\""]]
		[[Job="\ref fpmas::api::scheduler::Job \"Job\""]]
		[[Jobs="\ref fpmas::api::scheduler::Job \"Jobs\""]]
		[[JobList="\ref fpmas::api::scheduler::JobList \"JobList\""]]
		[[DistributedMoveAlgorithm="\ref fpmas::api::model::DistributedMoveAlgorithm \"DistributedMoveAlgorithm\""]]
		[[Scheduler="\ref fpmas::api::scheduler::Scheduler \"Scheduler\""]]
		[[Runtime="\ref fpmas::api::runtime::Runtime \"Runtime\""]]
		[[Probes="\ref fpmas::api::utils::perf::Probe \"Probes\""]]
		[[implem="\par Implementation details"]]
		)
	set(DOXYGEN_HAVE_DOT "YES")
	#set(DOXYGEN_TEMPLATE_RELATIONS "YES")
	set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)
	set(DOXYGEN_EXCLUDE_SYMBOLS
		[[Node< DistributedId, api::graph::DistributedEdge< T > >]]
		[[Edge< DistributedId, api::graph::DistributedNode< T > >]]
		[[Graph< api::graph::DistributedNode< T >, api::graph::DistributedEdge< T > >]]
		[[bool fpmas::io::json::light_serializer< AgentType >::warn_print]]

		# Hacky specialization to remove in a next release
		[[nlohmann::adl_serializer< std::unordered_map< DistributedId, Value, Hash, Equal, Alloc > >]]
		[[DIST_GRAPH_PARAMS]] [[DIST_GRAPH_PARAMS_SPEC]]
		[[ExampleAgent]]
		)
	set(DOXYGEN_EXCLUDE "src/fpmas/utils/log.h")
	set(DOXYGEN_HIDE_FRIEND_COMPOUNDS "YES")
	set(DOXYGEN_EXAMPLE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/examples)
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/examples
		)
endif()
