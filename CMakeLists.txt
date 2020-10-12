cmake_minimum_required(VERSION 3.1O)

project(fpmas VERSION 0.1)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# MPI setup
find_package(MPI 3 REQUIRED)
find_package(Zoltan 3.81 REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -std=c++11 -O0 -Wall -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set(LOG_LEVEL 0 CACHE STRING "Log level")
message("-- LOG_LEVEL : ${LOG_LEVEL}")

# Nlohmann JSON library
find_file(JSON nlohmann/json.hpp) 
if(NOT JSON)
	message(FATAL_ERROR "-- nlohman/json library not found.")
else()
	message("-- Found nlohman/json library : ${JSON}")
endif()

add_subdirectory(src)
add_subdirectory(tests)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/fpmasConfigVersion.cmake
	COMPATIBILITY SameMajorVersion)

install(FILES
	cmake/fpmasConfig.cmake
	cmake/FindZoltan.cmake
	${CMAKE_CURRENT_BINARY_DIR}/fpmasConfigVersion.cmake
	DESTINATION lib/cmake/fpmas)
install(DIRECTORY
	cmake/find_zoltan_version
	DESTINATION lib/cmake/fpmas)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
	set(DOXYGEN_MACRO_EXPANSION "YES")
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/docs/img)
	#set(DOXYGEN_CLANG_ASSISTED_PARSING "YES")
	#set(DOXYGEN_CLANG_DATABASE_PATH "build/compile_commands.json")
	set(DOXYGEN_SORT_MEMBER_DOCS "NO")
	set(DOXYGEN_PREDEFINED "DOXYGEN_BUILD")
	set(DOXYGEN_ALIASES
		[[LOCAL="\ref fpmas::api::graph::LocationState \"LOCAL\""]]
		[[DISTANT="\ref fpmas::api::graph::LocationState \"DISTANT\""]]
		[[Agent="\ref fpmas::api::model::Agent \"Agent\""]]
		[[Agents="\ref fpmas::api::model::Agent \"Agents\""]]
		[[AgentGroup="\ref fpmas::api::model::AgentGroup \"AgentGroup\""]]
		[[AgentGroups="\ref fpmas::api::model::AgentGroup \"AgentGroups\""]]
		[[AgentNode="\ref fpmas::api::model::AgentNode \"AgentNode\""]]
		[[AgentGraph="\ref fpmas::api::model::AgentGraph \"AgentGraph\""]]
		[[AgentTask="\ref fpmas::api::model::AgentTask \"AgentTask\""]]
		[[AgentTasks="\ref fpmas::api::model::AgentTask \"AgentTasks\""]]
		[[AgentNode="\ref fpmas::api::model::AgentNode \"AgentNode\""]]
		[[Job="\ref fpmas::api::scheduler::Job \"Job\""]]
		[[Scheduler="\ref fpmas::api::scheduler::Scheduler \"Scheduler\""]]
		[[Runtime="\ref fpmas::api::runtime::Runtime \"Runtime\""]]
		[[implem="\par Implementation details"]]
		)
	set(DOXYGEN_HAVE_DOT "YES")
	#set(DOXYGEN_TEMPLATE_RELATIONS "YES")
	set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)
	set(DOXYGEN_EXCLUDE_SYMBOLS
		[[Node< DistributedId, api::graph::DistributedEdge< T > >]]
		[[Edge< DistributedId, api::graph::DistributedNode< T > >]]
		[[Graph< api::graph::DistributedNode< T >, api::graph::DistributedEdge< T > >]]
		[[DIST_GRAPH_PARAMS]] [[DIST_GRAPH_PARAMS_SPEC]]
		)
	set(DOXYGEN_EXCLUDE "src/fpmas/utils/log.h")
	set(DOXYGEN_HIDE_FRIEND_COMPOUNDS "YES")
	doxygen_add_docs(doc
		${CMAKE_CURRENT_SOURCE_DIR}/src
		)
endif()

# generates compile_commands.json, useful for CCLS completion
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
