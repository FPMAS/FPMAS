find_library(Zoltan_LIBRARY zoltan)
find_path(Zoltan_INCLUDE_DIRS zoltan.h)

if(Zoltan_LIBRARY AND Zoltan_INCLUDE_DIRS)
	# Find zoltan version
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
		${CMAKE_CURRENT_BINARY_DIR}/cmake/find_zoltan_version)

	execute_process(COMMAND ${CMAKE_COMMAND}
		"-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
		-G "${CMAKE_GENERATOR}"
		${CMAKE_CURRENT_LIST_DIR}/find_zoltan_version
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake/find_zoltan_version)

	execute_process(COMMAND ${CMAKE_COMMAND} --build .
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake/find_zoltan_version)

	execute_process(COMMAND
		${CMAKE_CURRENT_BINARY_DIR}/cmake/find_zoltan_version/find_zoltan_version
		OUTPUT_VARIABLE Zoltan_VERSION)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zoltan
	FOUND_VAR Zoltan_FOUND
	REQUIRED_VARS
		Zoltan_LIBRARY
		Zoltan_INCLUDE_DIRS
	VERSION_VAR Zoltan_VERSION)

if(Zoltan_FOUND)
	set(Zoltan_LIBRARIES ${Zoltan_LIBRARY})
	set(Zoltan_INCLUDE_DIRS ${Zoltan_INCLUDE_DIR})
endif()

if(Zoltan_FOUND AND NOT TARGET Zoltan::Zoltan)
	add_library(Zoltan::Zoltan UNKNOWN IMPORTED)
	set_target_properties(Zoltan::Zoltan PROPERTIES
		IMPORTED_LOCATION "${Zoltan_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${Zoltan_INCLUDE_DIR}"
		)
endif()

