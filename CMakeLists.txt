cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
project(https_simple_proxy)

find_library(LIBURING uring)

add_library(http 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/connect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http/response.cpp
)
target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED)

add_library(worker
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker/worker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/worker/tasks/resolve_task.cpp
)
target_include_directories(worker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/worker)
target_link_libraries(worker PUBLIC ${Boost_LIBRARIES})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/http/connect.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBURING} http worker)

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