cmake_minimum_required(VERSION 3.10)

project(fpmas-examples)

include_directories(../src)

add_executable(behavior-with-args
	fpmas/model/behavior_with_args.cpp)
target_link_libraries(behavior-with-args fpmas)

add_executable(csv-output
	fpmas/output/csv_output.cpp)
target_link_libraries(csv-output fpmas)

add_executable(distributed-csv-output
	fpmas/output/distributed_csv_output.cpp)
target_link_libraries(distributed-csv-output fpmas)
