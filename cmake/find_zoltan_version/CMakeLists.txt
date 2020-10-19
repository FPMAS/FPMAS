cmake_minimum_required(VERSION 3.10)

project(find_zoltan_version)

find_package(MPI REQUIRED)

find_path(Zoltan_include zoltan.h)
find_library(Zoltan_lib zoltan)

include_directories(${Zoltan_include})
add_executable(find_zoltan_version main.cpp)
target_link_libraries(find_zoltan_version ${Zoltan_lib} MPI::MPI_CXX)
