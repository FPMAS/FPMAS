find_path(nlohmann_json_INCLUDE_DIR nlohmann/json.hpp)

if(nlohmann_json_INCLUDE_DIR)
	# Find nlohmann/json version
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
		${CMAKE_CURRENT_BINARY_DIR}/cmake/find_nlohmann_json_version)

	execute_process(COMMAND ${CMAKE_COMMAND}
		"-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
		-G "${CMAKE_GENERATOR}"
		${CMAKE_CURRENT_LIST_DIR}/find_nlohmann_json_version
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake/find_nlohmann_json_version)

	execute_process(COMMAND ${CMAKE_COMMAND} --build .
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake/find_nlohmann_json_version)

	execute_process(COMMAND
		${CMAKE_CURRENT_BINARY_DIR}/cmake/find_nlohmann_json_version/find_nlohmann_json_version
		OUTPUT_VARIABLE nlohmann_json_VERSION)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nlohmann_json
	FOUND_VAR nlohmann_json_FOUND
	REQUIRED_VARS
		nlohmann_json_INCLUDE_DIR
	VERSION_VAR nlohmann_json_VERSION)

if(nlohmann_json_FOUND)
	set(nlohmann_json_INCLUDE_DIRS ${Zoltan_INCLUDE_DIR})
endif()

if(nlohmann_json_FOUND AND NOT TARGET nlohmann_json::nlohmann_json)
	add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
	set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIR}"
		)
endif()

