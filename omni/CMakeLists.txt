cmake_minimum_required(VERSION 3.25)
project(OMNI)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

find_package(yaml-cpp)
include_directories(${yaml-cpp_INCLUDE_DIRS})

add_executable(wrp wrp.cc)

# compile with coverage options
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(wrp PRIVATE -coverage)
  target_link_options(wrp PRIVATE -coverage)
endif()  

target_link_libraries(wrp yaml-cpp::yaml-cpp)

add_test(NAME put COMMAND wrp put ../posix.yml)

add_test(NAME get COMMAND wrp get ../output.yml)

add_test(NAME ls COMMAND wrp ls)

add_test(NAME invalid COMMAND wrp invalid)
set_tests_properties(invalid
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME no_cmd COMMAND wrp)
set_tests_properties(no_cmd
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME put_only COMMAND wrp put)
set_tests_properties(put_only
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME get_only COMMAND wrp get)
set_tests_properties(get_only
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME not_found COMMAND wrp put not_found)
set_tests_properties(not_found
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME nested COMMAND wrp put ../nested.yml)

add_test(NAME empty COMMAND wrp put ../empty.yml)

add_test(NAME root COMMAND wrp put ../root.yml)

add_test(NAME scalar COMMAND wrp put ../scalar.yml)

add_test(NAME invalid_yml COMMAND wrp put ../invalid.yml)
set_tests_properties(invalid_yml
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)
