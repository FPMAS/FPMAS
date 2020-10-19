cmake_minimum_required(VERSION 3.10)

project(find_nlohmann_json_version)

find_path(nlohmann_json_include nlohmann/json.hpp)

include_directories(${nlohmann_json_include})
add_executable(find_nlohmann_json_version main.cpp)
