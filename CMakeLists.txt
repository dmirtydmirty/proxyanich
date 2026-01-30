cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
project(https_simple_proxy)

set(3D_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3dparty)

if (NOT EXISTS ${3D_PARTY_DIR})
    message(FATAL_ERROR "no 3dparty directory found exit...")
endif()

option(SPDLOG_NO_EXCEPTIONS "Compile with -fno-exceptions. Call abort() on any spdlog exceptions" ON)
add_subdirectory(${3D_PARTY_DIR}/spdlog)
add_subdirectory(${3D_PARTY_DIR}/boost)


set(LIBURING_LIB "${3D_PARTY_DIR}/liburing/src/liburing.a")
set(LIBURING_INCLUDE "${3D_PARTY_DIR}/liburing/src/include")

add_custom_target(liburing_make
    COMMAND make -C ${3D_PARTY_DIR}/liburing -j6
    COMMAND echo "Liburing built"
)

add_library(liburing STATIC IMPORTED)
add_dependencies(liburing liburing_make)

set_target_properties(liburing PROPERTIES
    IMPORTED_LOCATION ${LIBURING_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LIBURING_INCLUDE}
)

add_library(http 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/connect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/response.cpp
)
target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)

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

target_link_libraries(utils PUBLIC spdlog::spdlog liburing)

add_library(config 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config/config.cpp
)

target_include_directories(config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
)

add_executable(${PROJECT_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dp_thread/dp_thread.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dp_thread
)

target_link_libraries(${PROJECT_NAME} http utils config boost_lockfree)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(http_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_http.cpp)
target_link_libraries(http_test http GTest::gtest_main)

add_executable(indexed_list_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_indexed_list.cpp)
target_link_libraries(indexed_list_test utils GTest::gtest_main)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)