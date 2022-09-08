cmake_minimum_required(VERSION 3.23)
project(server_app)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost REQUIRED QUIET)
find_package(Boost COMPONENTS program_options QUIET)
if (Boost_FOUND)
    find_package(Boost COMPONENTS program_options )
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()
find_package(RdKafka QUIET CONFIG)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cppkafka/include)

add_executable(server_app main.cpp)

target_link_libraries(server_app Boost::boost Boost::program_options)
target_link_libraries(server_app /home/serj/projects/task/cppkafka/build/src/lib/libcppkafka.so)