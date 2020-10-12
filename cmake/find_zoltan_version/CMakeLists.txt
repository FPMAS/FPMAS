cmake_minimum_required(VERSION 3.10)

project(find_zoltan_version)

find_package(MPI REQUIRED)

add_executable(find_zoltan_version main.cpp)
target_link_libraries(find_zoltan_version zoltan MPI::MPI_CXX)
