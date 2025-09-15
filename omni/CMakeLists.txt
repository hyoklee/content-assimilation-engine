# Content Assimilation Engine (CAE) - OMNI Module
# Note: This CMakeLists.txt is designed to be included from the main project

cmake_minimum_required(VERSION 3.22)
project(OMNI)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

# Option to enable/disable MPI support
option(OMNI_ENABLE_MPI "Enable MPI support for OMNI module" OFF)

# Check if we have the necessary dependencies available from the parent project
if(OMNI_ENABLE_MPI)
    if(NOT TARGET MPI::MPI_CXX)
        message(STATUS "MPI not found in parent project, attempting to find it")
        find_package(MPI REQUIRED)
    endif()
    set(MPI_LIBS MPI::MPI_CXX)
    message(STATUS "MPI support enabled")
else()
    message(STATUS "MPI support disabled")
    set(MPI_LIBS "")
endif()

# Check for yaml-cpp
find_package(yaml-cpp REQUIRED)
if (${YAML_CPP_LIBRARIES})
    set(YAML_CPP_LIBS ${YAML_CPP_LIBRARIES})
elseif(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_LIBS yaml-cpp::yaml-cpp)
else()
    set(YAML_CPP_LIBS yaml-cpp)
endif()

message(STATUS "Building OMNI module with yaml-cpp")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Option to enable/disable HDF5 support
option(USE_HDF5 "Enable HDF5 support for OMNI module" OFF)

# Find HDF5 package if enabled
if(USE_HDF5)
    find_package(HDF5)
    include_directories(${HDF5_INCLUDE_DIRS})
    set(HDF5_LIBS ${HDF5_LIBRARIES})
    add_definitions(-DUSE_HDF5)
    message(STATUS "HDF5 support enabled")
else()
    set(HDF5_LIBS "")
    message(STATUS "HDF5 support disabled")
endif()

# Options for POCO and AWS support
option(USE_POCO "Enable POCO support for HTTP downloads and shared memory" OFF)
option(USE_AWS "Enable AWS SDK support for S3 operations" OFF)
option(USE_HERMES "Enable HERMES support for data staging" OFF)

if(USE_POCO)
    add_definitions(-DUSE_POCO)
    find_package(Poco REQUIRED COMPONENTS Crypto Foundation Net NetSSL)
    set(POCO_LIBS Poco::Crypto Poco::Foundation Poco::Net Poco::NetSSL)
    message(STATUS "POCO support enabled")
else()
    set(POCO_LIBS "")
    message(STATUS "POCO support disabled")
endif()

if(USE_AWS)
    add_definitions(-DUSE_AWS)
    find_package(AWSSDK REQUIRED COMPONENTS core s3)
    set(AWS_LIBS ${AWSSDK_LINK_LIBRARIES})
    message(STATUS "AWS SDK support enabled")
else()
    set(AWS_LIBS "")
    message(STATUS "AWS SDK support disabled")
endif()

if(USE_HERMES)
    add_definitions(-DUSE_HERMES)
    find_package(Hermes REQUIRED)
    # Add other Hermes dependencies as needed
    message(STATUS "HERMES support enabled")
else()
    message(STATUS "HERMES support disabled")
endif()

# HDF5 dependency with vcpkg support
if(USE_HDF5)
    find_package(HDF5 REQUIRED COMPONENTS C)
    include_directories(${HDF5_INCLUDE_DIRS})
    if (TARGET HDF5::HDF5)
        set(HDF5_LIBS HDF5::HDF5)
    elseif (TARGET hdf5)
        set(HDF5_LIBS hdf5)
    else()
        set(HDF5_LIBS ${HDF5_LIBRARIES})
    endif()

    message(STATUS "HDF5 found: ${HDF5_VERSION}")
else()
    message(STATUS "HDF5 support disabled")
    set(HDF5_LIBS "")
endif()

message(STATUS "yaml-cpp found: ${yaml-cpp_VERSION}")

# Find nlohmann-json for JSON parsing
find_package(nlohmann_json REQUIRED)
message(STATUS "nlohmann-json found")

# Source files for the factory and repository implementations
if(USE_HDF5)
    set(OMNI_FACTORY_SOURCES
        format/format_factory.cc
        format/hdf5_dataset_client.cc
        format/dataset_config.cc
        repo/repo_factory.cc
        par.cc
	pat.cc
        h5.cc
    )
else()
    set(OMNI_FACTORY_SOURCES
        format/format_factory.cc
        repo/repo_factory.cc
    )
endif()

# Create a static library for OMNI components
add_library(omni_lib STATIC ${OMNI_FACTORY_SOURCES})
target_link_libraries(omni_lib ${MPI_LIBS} ${YAML_CPP_LIBS})
target_include_directories(omni_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(USE_HDF5)
    target_include_directories(omni_lib PRIVATE ${HDF5_INCLUDE_DIRS})
endif()

# Main YAML parser and job orchestrator (wrp binary)
add_executable(wrp 
    wrp.cc
    format/globus_utils.cpp
    glo.cc
)
target_include_directories(wrp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Build library list for wrp
set(WRP_LIBS 
    omni_lib 
    ${YAML_CPP_LIBS} 
    ${POCO_LIBS} 
    ${AWS_LIBS}
    nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT}
)

if(USE_HDF5)
    list(APPEND WRP_LIBS ${HDF5_LIBS})
endif()

if(OMNI_ENABLE_MPI)
    list(APPEND WRP_LIBS ${MPI_LIBS})
    target_compile_definitions(wrp PRIVATE OMNI_ENABLE_MPI)
endif()

target_link_libraries(wrp ${WRP_LIBS})

# MPI binary format processor (wrp_binary_format_mpi binary)
if(OMNI_ENABLE_MPI)
    add_executable(wrp_binary_format_mpi wrp_binary_format_mpi.cc)
    target_link_libraries(wrp_binary_format_mpi ${MPI_LIBS})
    target_include_directories(wrp_binary_format_mpi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
else()
    message(STATUS "Skipping wrp_binary_format_mpi (MPI disabled)")
endif()




# Set output directory for binaries to match the main project
# set_target_properties(wrp rp_binary_format_mpi  PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
# )


# Copy config files to bin directory
if(USE_HDF5)
    configure_file(config/dataset_config.yaml ${CMAKE_BINARY_DIR}/omni/config/dataset_config.yaml COPYONLY)
endif()

# Install targets
if(OMNI_ENABLE_MPI)
    install(TARGETS wrp wrp_binary_format_mpi
        RUNTIME DESTINATION ${CAE_INSTALL_BIN_DIR}
    )
else()
    install(TARGETS wrp
        RUNTIME DESTINATION ${CAE_INSTALL_BIN_DIR}
    )
endif()

install(TARGETS omni_lib
    ARCHIVE DESTINATION ${CAE_INSTALL_LIB_DIR}
)

# Install header files with new structure
if(USE_HDF5)
    install(FILES 
        format/format_client.h
        format/format_factory.h
        format/binary_file_omni.h
        format/dataset_config.h
        format/hdf5_dataset_client.h
        omni_processing.h
        DESTINATION ${CAE_INSTALL_INCLUDE_DIR}/omni/format
    )
else()
    install(FILES 
        format/format_client.h
        format/format_factory.h
        format/binary_file_omni.h
        DESTINATION ${CAE_INSTALL_INCLUDE_DIR}/omni/format
    )
endif()

install(FILES 
    repo/repo_client.h
    repo/repo_factory.h
    repo/filesystem_repo_omni.h
    DESTINATION ${CAE_INSTALL_INCLUDE_DIR}/omni/repo
)

# Install configuration examples
install(DIRECTORY config/
    DESTINATION ${CAE_INSTALL_DATA_DIR}/omni/config
    FILES_MATCHING PATTERN "*.yaml"
)

# Install documentation
install(FILES README.md
    DESTINATION ${CAE_INSTALL_DATA_DIR}/omni
)

# Configure and install configuration files
configure_file(config/quick_test.yaml ${CMAKE_BINARY_DIR}/omni/config/quick_test.yaml COPYONLY)
configure_file(config/demo_job.yaml ${CMAKE_BINARY_DIR}/omni/config/demo_job.yaml COPYONLY)
configure_file(config/example_job.yaml ${CMAKE_BINARY_DIR}/omni/config/example_job.yaml COPYONLY)
configure_file(config/wildcard_test.yaml ${CMAKE_BINARY_DIR}/omni/config/wildcard_test.yaml COPYONLY)

# Install test script with executable permissions
install(PROGRAMS config/run_all_tests.sh
        DESTINATION ${CMAKE_BINARY_DIR}/omni/config/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                   GROUP_READ GROUP_EXECUTE
                   WORLD_READ WORLD_EXECUTE)

# Install wildcard test script with executable permissions
install(PROGRAMS config/test_wildcards.sh
        DESTINATION ${CMAKE_BINARY_DIR}/omni/config/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                   GROUP_READ GROUP_EXECUTE
                   WORLD_READ WORLD_EXECUTE)

# Add tests
add_test(NAME put COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/posix.yml)
add_test(NAME get COMMAND wrp get cae)
add_test(NAME ls COMMAND wrp ls)

# Tests that should fail
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

# Additional test files
add_test(NAME nested COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/nested.yml)
add_test(NAME empty COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/empty.yml)
add_test(NAME root COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/root.yml)
add_test(NAME scalar COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/scalar.yml)

add_test(NAME invalid_yml COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/invalid.yml)
set_tests_properties(invalid_yml
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME path COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/path.yml)
set_tests_properties(path
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME root_seq COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/root_seq.yml)

add_test(NAME lseek COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/lseek.yml)
set_tests_properties(lseek
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

# POCO-specific tests
if(USE_POCO)
    add_test(NAME hash COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/hash.yml)
    set_tests_properties(hash
        PROPERTIES
        WILL_FAIL "TRUE"
        RETURN_VALUE 1
    )

    add_test(NAME http COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/http.yml)
    add_test(NAME redi COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/redi.yml)
    add_test(NAME rget COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/rget.yml)
endif()

if(USE_HDF5)
    add_test(NAME hdf5 COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/hdf5.yml)
endif()

# Globus transfer test (requires POCO for HTTP/HTTPS operations)
if(USE_POCO)
    add_test(NAME globus COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/globus.yml)
    if("$ENV{GLOBUS_TRANSFER_TOKEN}" STREQUAL "")
        set_tests_properties(globus
            PROPERTIES
            WILL_FAIL TRUE
        )
    else()
        set_tests_properties(globus
            PROPERTIES
            ENVIRONMENT "GLOBUS_TRANSFER_TOKEN=$ENV{GLOBUS_TRANSFER_TOKEN}"
            WILL_FAIL FALSE
        )
    endif()
    message(STATUS "Added Globus transfer test (requires GLOBUS_TRANSFER_TOKEN environment variable)")
endif()
