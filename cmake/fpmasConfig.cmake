include(CMakeFindDependencyMacro)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR})
find_dependency(MPI)
find_dependency(Zoltan)
find_dependency(nlohmann_json)

include("${CMAKE_CURRENT_LIST_DIR}/fpmas_targets.cmake")
