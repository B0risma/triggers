cmake_minimum_required(VERSION 3.28)
project(server_v2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Original V1 server (rule-based)
# add_executable(server ${CMAKE_SOURCE_DIR}/source/main.cpp
#     ${CMAKE_SOURCE_DIR}/source/trigger_cfg.cpp
#     source/rule_srv.cpp
#     source/virt_switch.cpp
#     source/v2.cpp)

# New V2 server (event-based)
add_executable(server_v2 ${CMAKE_SOURCE_DIR}/source/main_v2.cpp
    source/event_system/event_queue.cpp
    source/event_system/trigger.cpp
    source/event_system/api_handler.cpp
    source/event_system/rule.cpp)

include_directories(${CMAKE_SOURCE_DIR}/3rdparty)
include_directories(${CMAKE_SOURCE_DIR}/source)

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/httplib)

# target_link_libraries(server httplib)
target_link_libraries(server_v2 httplib)
