cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(HunterGate)

message(STATUS "[project] CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

option(HUNTER_STATUS_DEBUG "Hunter debug info" OFF)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.214.tar.gz"
    SHA1 "e14bc153a7f16d6a5eeec845fb0283c8fad8c358"
    LOCAL
)

project(beast_machine_loader)

include(CTest)
include(${CMAKE_SOURCE_DIR}/cmake/clang_tools.cmake)

option(CODE_COVERAGE "Enable coverage reporting" OFF)
option(DO_CLANG_TIDY "Enable clang tidy" OFF)
option(DO_CLANG_FORMAT "Enable clang format" OFF)

hunter_add_package(Boost COMPONENTS 
    system
    thread)

find_package(Boost CONFIG REQUIRED 
    system
    thread)

hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

hunter_add_package(Catch2)
find_package(Catch2 CONFIG REQUIRED)

if(DO_CLANG_TIDY)
  enable_clang_tidy(
      CHECKS
          -*
          misc-non-private-member-variables-in-classes
    )
endif()

if(DO_CLANG_FORMAT)
  find_program(CLANG_FORMAT_EXE NAMES clang-format clang-format-6.0 PATHS /usr/local/clang_8.0.0/bin)
  if(NOT CLANG_FORMAT_EXE)
    message(STATUS "clang-format not found.")
  else()
      message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
  endif ()
endif()

message("code")
add_subdirectory(code)
message("test")
add_subdirectory(test)

enable_testing()
