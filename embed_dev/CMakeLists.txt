cmake_minimum_required(VERSION 3.23)
project(embed_dev)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost REQUIRED QUIET)
find_package(Boost COMPONENTS program_options QUIET)
if (Boost_FOUND)
    find_package(Boost COMPONENTS program_options )
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()
find_package(RdKafka QUIET CONFIG)
#find_package(RdKafka REQUIRED QUIET MODULE )
#find_package(CppKafka REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cppkafka/include)

add_executable(embed_dev main.cpp)

target_link_libraries(embed_dev Boost::boost Boost::program_options) # RdKafka::rdkafka
target_link_libraries(embed_dev /home/serj/projects/task/cppkafka/build/src/lib/libcppkafka.so)
target_link_libraries(embed_dev /usr/lib/x86_64-linux-gnu/librdkafka.so)