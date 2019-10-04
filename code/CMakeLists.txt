cmake_minimum_required(VERSION 3.13)

project(beast_machine)
message(STATUS "doing ${PROJECT_NAME}")

file(GLOB_RECURSE _HDRS "*.hpp")
file(GLOB_RECURSE _SRCS "*.cpp")

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include/)
