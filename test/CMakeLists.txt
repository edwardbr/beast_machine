cmake_minimum_required(VERSION 3.13)

project(test_beast_machine)

add_executable(${PROJECT_NAME} test.cpp)

target_compile_features(${PROJECT_NAME} 
  PRIVATE
    cxx_std_17
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Catch2::Catch2 
        beast_machine
        Threads::Threads
        Boost::system
        OpenSSL::SSL OpenSSL::Crypto
)

if(DO_CLANG_TIDY)
  clang_tidy_check(${PROJECT_NAME} )
endif()

if(CLANG_FORMAT_EXE)
    add_custom_target("clang_format_${PROJECT_NAME}" COMMAND "${CLANG_FORMAT_EXE} -i test.cpp")
endif()

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})