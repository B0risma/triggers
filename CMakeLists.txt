cmake_minimum_required(VERSION 3.28)
project(server LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(server ${CMAKE_SOURCE_DIR}/source/main.cpp
    ${CMAKE_SOURCE_DIR}/source/trigger_cfg.cpp
    source/rule_srv.cpp)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty)

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/httplib)

target_link_libraries(server httplib)
