cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
project(vpnserver)

option(SPDLOG_NO_EXCEPTIONS "Compile with -fno-exceptions. Call abort() on any spdlog exceptions" ON)
find_package(spdlog CONFIG REQUIRED)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

add_library(utils 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/thread/thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/recv_buffers/recv_buffers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/event_loop/event_loop.cpp
)
    
target_include_directories(utils PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/
)
target_link_libraries(utils PUBLIC spdlog::spdlog uring)

add_library(config 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config/config.cpp
)

target_include_directories(config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
)

add_executable(${PROJECT_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(${PROJECT_NAME} utils config)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(indexed_list_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_indexed_list.cpp)
target_link_libraries(indexed_list_test utils GTest::gtest_main)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)