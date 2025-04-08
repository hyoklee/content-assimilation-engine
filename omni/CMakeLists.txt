cmake_minimum_required(VERSION 3.25)
project(cae)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

find_package(yaml-cpp)
include_directories(${yaml-cpp_INCLUDE_DIRS})
add_executable(wrp wrp.cc )
target_link_libraries(wrp yaml-cpp::yaml-cpp)

add_test(NAME parse_yaml COMMAND wrp put ../../data/posix.yml)
