cmake_minimum_required(VERSION 2.8)
project(config_parse-test)

include_directories("/usr/include/google")
find_package(config_parse REQUIRED)
add_definitions("-g -Wall")

add_executable(test test.c)
target_link_libraries(test cmockery pthread config_parse)
