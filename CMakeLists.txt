project(config_parse)
cmake_minimum_required(VERSION 2.8.4)


add_definitions("-g -Wall")
add_subdirectory(src)


export(PACKAGE mylib)
export(TARGETS config_parse FILE config_parse-exports.cmake)
configure_file(config_parse-config.cmake.in ${CMAKE_BINARY_DIR}/config_parse-config.cmake)
