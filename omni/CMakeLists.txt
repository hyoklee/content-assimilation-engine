# Content Assimilation Engine (CAE) - OMNI Module
# Note: This CMakeLists.txt is designed to be included from the main project

cmake_minimum_required(VERSION 3.22)
project(OMNI)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

# Option to enable/disable MPI support
option(USE_MPI "Enable MPI support for OMNI module" OFF)

# Option to enable code coverage
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

# Add coverage compiler flags if enabled
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled")
        set(COVERAGE_FLAGS "--coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    else()
        message(WARNING "Code coverage is only supported with GCC or Clang")
    endif()
endif()

# Check if we have the necessary dependencies available from the parent project
if(USE_MPI)
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
option(USE_REDIS "Enable Redis support for data storage" OFF)
option(USE_MEMCACHED "Enable Memcached support for data storage" OFF)

if(USE_POCO)
    add_definitions(-DUSE_POCO)
    find_package(Poco REQUIRED COMPONENTS Crypto Foundation Net NetSSL)
    set(POCO_LIBS Poco::Crypto Poco::Foundation Poco::Net Poco::NetSSL)
    message(STATUS "POCO support enabled")
else()
    set(POCO_LIBS "")
    message(STATUS "POCO support disabled")
endif()

# Redis support (requires POCO for Redis operations)
if(USE_REDIS)
    if(NOT USE_POCO)
        message(FATAL_ERROR "USE_REDIS requires USE_POCO to be enabled")
    endif()
    add_definitions(-DUSE_REDIS)
    find_package(Poco REQUIRED COMPONENTS Redis)
    list(APPEND POCO_LIBS Poco::Redis)
    message(STATUS "Redis support enabled")
else()
    message(STATUS "Redis support disabled")
endif()

# Memcached support (standalone, no POCO dependency)
if(USE_MEMCACHED)
    add_definitions(-DUSE_MEMCACHED)
    message(STATUS "Memcached support enabled")
else()
    message(STATUS "Memcached support disabled")
endif()

# DataHub support (requires POCO for HTTP operations)
option(USE_DATAHUB "Enable DataHub metastore integration" OFF)
if(USE_DATAHUB)
    if(NOT USE_POCO)
        message(FATAL_ERROR "USE_DATAHUB requires USE_POCO to be enabled")
    endif()
    add_definitions(-DUSE_DATAHUB)
    message(STATUS "DataHub support enabled")
else()
    message(STATUS "DataHub support disabled")
endif()

# Globus support (requires POCO for HTTP operations)
option(USE_GLOBUS "Enable Globus transfer support" OFF)
if(USE_GLOBUS)
    if(NOT USE_POCO)
        message(FATAL_ERROR "USE_GLOBUS requires USE_POCO to be enabled")
    endif()
    add_definitions(-DUSE_GLOBUS)
    message(STATUS "Globus support enabled")
else()
    message(STATUS "Globus support disabled")
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
    # Find thallium package for include directories
    find_package(thallium REQUIRED)
    # Link required Hermes/IOWarp libraries
    set(HERMES_LIBS
        hermes_hermes_core_client
        hermes_shm_host
        chimaera_client_host
        chimaera_runtime
        margo
        thallium
    )
    message(STATUS "HERMES support enabled with libraries: ${HERMES_LIBS}")
else()
    message(STATUS "HERMES support disabled")
    set(HERMES_LIBS "")
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

# Find nlohmann-json for JSON parsing (only needed when POCO is enabled)
if(USE_POCO)
    find_package(nlohmann_json REQUIRED)
    message(STATUS "nlohmann-json found")
else()
    message(STATUS "nlohmann-json not required (POCO disabled)")
endif()

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
if(USE_POCO)
    add_executable(wrp
        wrp.cc
        OMNI.cc
        format/globus_utils.cpp
        glo.cc
    )
else()
    add_executable(wrp
        wrp.cc
        OMNI.cc
    )
endif()
target_include_directories(wrp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Build library list for wrp
set(WRP_LIBS
    omni_lib
    ${YAML_CPP_LIBS}
    ${POCO_LIBS}
    ${AWS_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
)

if(USE_POCO)
    list(APPEND WRP_LIBS nlohmann_json::nlohmann_json)
endif()

if(USE_HDF5)
    list(APPEND WRP_LIBS ${HDF5_LIBS})
endif()

if(USE_MPI)
    list(APPEND WRP_LIBS ${MPI_LIBS})
    target_compile_definitions(wrp PRIVATE USE_MPI)
endif()

if(USE_HERMES)
    list(APPEND WRP_LIBS ${HERMES_LIBS})
endif()

target_link_libraries(wrp ${WRP_LIBS})

# DataHub unit test binary (requires POCO and DataHub support)
if(USE_POCO AND USE_DATAHUB)
    add_executable(test_datahub test_datahub.cpp OMNI.cc format/globus_utils.cpp glo.cc)
    target_include_directories(test_datahub PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_datahub omni_lib ${YAML_CPP_LIBS} ${POCO_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    if(USE_POCO)
        target_link_libraries(test_datahub nlohmann_json::nlohmann_json)
    endif()
    if(USE_HDF5)
        target_link_libraries(test_datahub ${HDF5_LIBS})
    endif()
    if(USE_AWS)
        target_link_libraries(test_datahub ${AWS_LIBS})
    endif()
    if(USE_HERMES)
        target_link_libraries(test_datahub ${HERMES_LIBS})
    endif()
    message(STATUS "Building DataHub unit test (test_datahub)")
else()
    if(USE_POCO AND NOT USE_DATAHUB)
        message(STATUS "Skipping DataHub unit test (USE_DATAHUB disabled)")
    endif()
endif()

# Globus Utils unit test binary (requires Globus support)
if(USE_GLOBUS)
    add_executable(test_globus_utils test_globus_utils.cpp format/globus_utils.cpp glo.cc)
    target_include_directories(test_globus_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_globus_utils ${POCO_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    if(USE_POCO)
        target_link_libraries(test_globus_utils nlohmann_json::nlohmann_json)
    endif()
    message(STATUS "Building Globus Utils unit test (test_globus_utils)")
else()
    message(STATUS "Skipping Globus Utils unit test (USE_GLOBUS disabled)")
endif()

# glo.cc comprehensive unit test (requires POCO support)
if(USE_POCO)
    add_executable(test_glo test_glo.cpp format/globus_utils.cpp glo.cc OMNI.cc)
    target_include_directories(test_glo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_glo omni_lib ${YAML_CPP_LIBS} ${POCO_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    if(USE_POCO)
        target_link_libraries(test_glo nlohmann_json::nlohmann_json)
    endif()
    if(USE_HDF5)
        target_link_libraries(test_glo ${HDF5_LIBS})
    endif()
    if(USE_AWS)
        target_link_libraries(test_glo ${AWS_LIBS})
    endif()
    if(USE_HERMES)
        target_link_libraries(test_glo ${HERMES_LIBS})
    endif()
    message(STATUS "Building glo.cc comprehensive unit test (test_glo)")
else()
    message(STATUS "Skipping glo.cc unit test (USE_POCO disabled)")
endif()

# OMNI unit test binary (requires POCO support)
if(USE_POCO)
    add_executable(test_omni test_omni.cpp OMNI.cc format/globus_utils.cpp glo.cc)
    target_include_directories(test_omni PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_omni omni_lib ${YAML_CPP_LIBS} ${POCO_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    if(USE_POCO)
        target_link_libraries(test_omni nlohmann_json::nlohmann_json)
    endif()
    if(USE_HDF5)
        target_link_libraries(test_omni ${HDF5_LIBS})
    endif()
    if(USE_AWS)
        target_link_libraries(test_omni ${AWS_LIBS})
    endif()
    if(USE_HERMES)
        target_link_libraries(test_omni ${HERMES_LIBS})
    endif()
    message(STATUS "Building OMNI unit test (test_omni)")

    # Extended OMNI unit test binary
    add_executable(test_omni_extended test_omni_extended.cpp OMNI.cc format/globus_utils.cpp glo.cc)
    target_include_directories(test_omni_extended PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_omni_extended omni_lib ${YAML_CPP_LIBS} ${POCO_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    if(USE_POCO)
        target_link_libraries(test_omni_extended nlohmann_json::nlohmann_json)
    endif()
    if(USE_HDF5)
        target_link_libraries(test_omni_extended ${HDF5_LIBS})
    endif()
    if(USE_AWS)
        target_link_libraries(test_omni_extended ${AWS_LIBS})
    endif()
    if(USE_HERMES)
        target_link_libraries(test_omni_extended ${HERMES_LIBS})
    endif()
    message(STATUS "Building OMNI extended unit test (test_omni_extended)")
else()
    message(STATUS "Skipping OMNI unit tests (USE_POCO disabled)")
endif()

# HDF5 dataset client unit test (requires HDF5 support)
if(USE_HDF5)
    add_executable(test_hdf5_dataset_client test_hdf5_dataset_client.cpp)
    target_include_directories(test_hdf5_dataset_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_hdf5_dataset_client omni_lib ${YAML_CPP_LIBS} ${HDF5_LIBS})
    message(STATUS "Building HDF5 dataset client unit test (test_hdf5_dataset_client)")
else()
    message(STATUS "Skipping HDF5 dataset client unit test (USE_HDF5 disabled)")
endif()

# MPI binary format processor (wrp_binary_format_mpi binary)
if(USE_MPI)
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
if(USE_MPI)
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

install(FILES
    OMNI.h
    DESTINATION ${CAE_INSTALL_INCLUDE_DIR}/omni
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

# Install man pages
install(FILES omni.5
    DESTINATION ${CAE_INSTALL_MAN_DIR}/man5
    OPTIONAL
)

install(FILES wrp.1
    DESTINATION ${CAE_INSTALL_MAN_DIR}/man1
    OPTIONAL
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

# Tests for -q (quiet) option
add_test(NAME quiet_put COMMAND wrp -q put ${CMAKE_CURRENT_SOURCE_DIR}/test/quiet_test.yml)
set_tests_properties(quiet_put
    PROPERTIES
    FAIL_REGULAR_EXPRESSION "checking IOWarp runtime|creating a new buffer|putting.*bytes into"
)

add_test(NAME quiet_get COMMAND wrp -q get quiet_test)
set_tests_properties(quiet_get
    PROPERTIES
    DEPENDS quiet_put
    FAIL_REGULAR_EXPRESSION "writing output|done"
)

add_test(NAME quiet_ls COMMAND wrp -q ls)
set_tests_properties(quiet_ls
    PROPERTIES
    DEPENDS quiet_put
    FAIL_REGULAR_EXPRESSION "connecting runtime"
)

# Test that -q option still fails properly with invalid commands
add_test(NAME quiet_invalid COMMAND wrp -q invalid)
set_tests_properties(quiet_invalid
    PROPERTIES
    WILL_FAIL TRUE
)

# Test that -q option fails when no command is provided
add_test(NAME quiet_no_cmd COMMAND wrp -q)
set_tests_properties(quiet_no_cmd
    PROPERTIES
    WILL_FAIL TRUE
    RETURN_VALUE 1
)

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

# These tests use YAML structures without required 'name' and 'tags' fields
add_test(NAME root COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/root.yml)
set_tests_properties(root
    PROPERTIES
    WILL_FAIL "TRUE"
)

add_test(NAME scalar COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/scalar.yml)
set_tests_properties(scalar
    PROPERTIES
    WILL_FAIL "TRUE"
)

add_test(NAME invalid_yml COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/invalid.yml)
set_tests_properties(invalid_yml
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

# Validation tests for required YAML fields
add_test(NAME missing_name COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/missing_name.yml)
set_tests_properties(missing_name
    PROPERTIES
    PASS_REGULAR_EXPRESSION "Error: 'name' field is required in OMNI YAML file"
)

add_test(NAME missing_tags COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/missing_tags.yml)
set_tests_properties(missing_tags
    PROPERTIES
    PASS_REGULAR_EXPRESSION "Error: 'tags' field is required in OMNI YAML file"
)

add_test(NAME missing_both COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/missing_both.yml)
set_tests_properties(missing_both
    PROPERTIES
    PASS_REGULAR_EXPRESSION "Error: 'name' field is required in OMNI YAML file"
)

add_test(NAME path COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/path.yml)
set_tests_properties(path
    PROPERTIES
    WILL_FAIL "TRUE"
    RETURN_VALUE 1
)

add_test(NAME root_seq COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/root_seq.yml)
set_tests_properties(root_seq
    PROPERTIES
    WILL_FAIL "TRUE"
)

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

# Globus transfer test (requires USE_GLOBUS)
if(USE_GLOBUS)
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

# Globus Utils unit test (requires Globus support)
if(USE_GLOBUS)
    add_test(NAME globus_utils_unit COMMAND $<TARGET_FILE:test_globus_utils>)
    set_tests_properties(globus_utils_unit
        PROPERTIES
        PASS_REGULAR_EXPRESSION "All tests passed!"
    )
    message(STATUS "Added Globus Utils unit test (tests is_globus_uri and parse_globus_uri)")
endif()

# glo.cc comprehensive unit test (requires POCO support)
if(USE_POCO)
    add_test(NAME glo_unit COMMAND $<TARGET_FILE:test_glo>)
    set_tests_properties(glo_unit
        PROPERTIES
        PASS_REGULAR_EXPRESSION "All glo.cc tests passed!"
    )
    message(STATUS "Added glo.cc comprehensive unit test (tests all glo.cc functions)")
endif()

# OMNI unit test (requires POCO support)
if(USE_POCO)
    add_test(NAME omni_unit COMMAND $<TARGET_FILE:test_omni>)
    set_tests_properties(omni_unit
        PROPERTIES
        PASS_REGULAR_EXPRESSION "All tests passed!"
    )
    message(STATUS "Added OMNI unit test (tests OMNI class methods)")

    add_test(NAME omni_extended_unit COMMAND $<TARGET_FILE:test_omni_extended>)
    set_tests_properties(omni_extended_unit
        PROPERTIES
        PASS_REGULAR_EXPRESSION "All tests passed!"
    )
    message(STATUS "Added OMNI extended unit test (tests YAML parsing and workflows)")
endif()

# HDF5 dataset client unit test (requires HDF5 support)
if(USE_HDF5)
    add_test(NAME hdf5_dataset_client_unit COMMAND $<TARGET_FILE:test_hdf5_dataset_client>)
    set_tests_properties(hdf5_dataset_client_unit
        PROPERTIES
        PASS_REGULAR_EXPRESSION "All HDF5 Dataset Client tests passed!"
    )
    message(STATUS "Added HDF5 dataset client unit test (tests all HDF5 dataset client functions)")
endif()

# DataHub unit tests (requires POCO and DataHub support)
if(USE_POCO AND USE_DATAHUB)
    # Unit test for DataHub functions (CheckDataHubConfig, ReadConfigFile, etc.)
    add_test(NAME datahub_unit COMMAND $<TARGET_FILE:test_datahub>)
    set_tests_properties(datahub_unit
        PROPERTIES
        PASS_REGULAR_EXPRESSION "ALL TESTS PASSED"
    )
    message(STATUS "Added DataHub unit test (tests CheckDataHubConfig and ReadConfigFile)")

    # Integration test that creates config and tests full workflow
    if(WIN32)
        add_test(NAME datahub_integration
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/test_datahub.bat
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    else()
        add_test(NAME datahub_integration
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/test_datahub.sh
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
    set_tests_properties(datahub_integration
        PROPERTIES
        PASS_REGULAR_EXPRESSION "DataHub metastore detected in config"
        TIMEOUT 30
    )
    message(STATUS "Added DataHub integration test (creates ~/.wrp/config and tests registration)")

    # Test that DataHub config is NOT detected when file doesn't exist or doesn't contain the flag
    # This test removes the config file to ensure clean state
    if(WIN32)
        add_test(NAME datahub_disabled_cleanup COMMAND powershell -Command "Remove-Item -Path $env:USERPROFILE\\.wrp\\config -Force -ErrorAction SilentlyContinue; exit 0")
    else()
        add_test(NAME datahub_disabled_cleanup COMMAND sh -c "rm -f ~/.wrp/config || true")
    endif()

    add_test(NAME datahub_disabled COMMAND wrp put ${CMAKE_CURRENT_SOURCE_DIR}/test/posix.yml)
    set_tests_properties(datahub_disabled
        PROPERTIES
        DEPENDS datahub_disabled_cleanup
        FAIL_REGULAR_EXPRESSION "DataHub metastore detected in config"
    )
endif()

# Add coverage target
if(ENABLE_COVERAGE)
    # Find gcov and lcov tools
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(LCOV_PATH AND GENHTML_PATH)
        # Add custom target for coverage report
        add_custom_target(coverage
            # Clean previous coverage data
            COMMAND ${LCOV_PATH} --directory . --zerocounters

            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure

            # Capture coverage data
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info

            # Remove system and external libraries from coverage
            COMMAND ${LCOV_PATH} --ignore-errors unused --remove coverage.info '/usr/*' '*/test/*' --output-file coverage.info.cleaned

            # Generate HTML report
            COMMAND ${GENHTML_PATH} -o coverage_html coverage.info.cleaned

            # Summary
            COMMAND ${LCOV_PATH} --list coverage.info.cleaned

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report..."
        )

        message(STATUS "Coverage target added. Run 'make coverage' or 'cmake --build . --target coverage' to generate report")
    elseif(GCOV_PATH)
        # Fallback to basic gcov if lcov is not available
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${GCOV_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests with coverage (gcov)..."
        )
        message(STATUS "Coverage target added (gcov only). Run 'make coverage' or 'cmake --build . --target coverage'")
    else()
        message(WARNING "gcov/lcov tools not found. Coverage target not available.")
    endif()
endif()
