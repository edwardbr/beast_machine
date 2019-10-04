cmake_minimum_required(VERSION 3.13)

project(websocket_test)


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

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})