cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
project(https_simple_proxy)

find_library(LIBURING uring)

add_library(http 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/connect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/response.cpp
)
target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)

add_library(recv_buffers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recv_buffers/recv_buffers.cpp
)

target_include_directories(recv_buffers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recv_buffers
)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED)

add_library(worker
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker/worker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker/tasks/resolve_task.cpp
)
target_include_directories(worker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/worker)
target_link_libraries(worker PUBLIC ${Boost_LIBRARIES} recv_buffers http)


add_library(indexed_list INTERFACE)
target_include_directories(indexed_list INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/indexed_list/)

add_library(event_loop 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/event_loop/event_loop.cpp
)

target_include_directories(event_loop PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/event_loop
)

target_link_libraries(event_loop PUBLIC ${LIBURING} recv_buffers indexed_list worker)

add_executable(${PROJECT_NAME} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(${PROJECT_NAME} http worker event_loop indexed_list)

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
target_link_libraries(indexed_list_test indexed_list GTest::gtest_main)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# add_executable(mock ${CMAKE_CURRENT_SOURCE_DIR}/mock/main.cpp)

# find_package(CURL REQUIRED)

# target_link_libraries(mock PRIVATE ${CURL_LIBRARIES})