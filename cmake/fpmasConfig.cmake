include(CMakeFindDependencyMacro)
message(${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR})
find_dependency(MPI)
find_dependency(Zoltan)

include("${CMAKE_CURRENT_LIST_DIR}/fpmas_targets.cmake")
