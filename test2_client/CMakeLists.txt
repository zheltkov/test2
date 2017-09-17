cmake_minimum_required(VERSION 3.8)
project(test2_client)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost COMPONENTS system filesystem program_options REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

add_executable(test2_client main.cpp)

target_link_libraries(test2_client
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        )