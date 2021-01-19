cmake_minimum_required(VERSION 3.10)

project(fpmas-examples)

include_directories(../src)
add_executable(behavior_with_args
	fpmas/model/behavior_with_args.cpp)
target_link_libraries(behavior_with_args fpmas)
