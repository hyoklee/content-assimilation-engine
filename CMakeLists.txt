cmake_minimum_required(VERSION 3.22)
project(cae VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

# -----------------------------------------------------------------------------
# Set CAE Version Variables
# -----------------------------------------------------------------------------
set(CAE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CAE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CAE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# -----------------------------------------------------------------------------
# Define Options
# -----------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(CAE_ENABLE_CMAKE_DOTENV "Load environment variables from .env.cmake" OFF)

# Main Hermes option - controls whether Hermes is built at all
option(CAE_ENABLE_HERMES "Enable Hermes support (required for adapters and VFD)" ON)

# Hermes adapter options (only available if Hermes is enabled)
option(CAE_ENABLE_NVIDIA_GDS_ADAPTER "Build the Hermes NVIDIA GDS adapter." OFF)
option(CAE_ENABLE_POSIX_ADAPTER "Build the Hermes POSIX adapter." ON)
option(CAE_ENABLE_STDIO_ADAPTER "Build the Hermes stdio adapter." ON)
option(CAE_ENABLE_MPIIO_ADAPTER "Build the Hermes MPI-IO adapter." OFF)
option(CAE_ENABLE_VFD "Build the Hermes HDF5 Virtual File Driver" OFF)

# Validate that adapter options require Hermes to be enabled
if(NOT CAE_ENABLE_HERMES)
    if(CAE_ENABLE_NVIDIA_GDS_ADAPTER OR CAE_ENABLE_POSIX_ADAPTER OR CAE_ENABLE_STDIO_ADAPTER OR CAE_ENABLE_MPIIO_ADAPTER OR CAE_ENABLE_VFD)
        message(WARNING "Hermes adapters and VFD require CAE_ENABLE_HERMES=ON. Disabling all Hermes-dependent features.")
        set(CAE_ENABLE_NVIDIA_GDS_ADAPTER OFF CACHE BOOL "Disabled due to Hermes being disabled" FORCE)
        set(CAE_ENABLE_POSIX_ADAPTER OFF CACHE BOOL "Disabled due to Hermes being disabled" FORCE)
        set(CAE_ENABLE_STDIO_ADAPTER OFF CACHE BOOL "Disabled due to Hermes being disabled" FORCE)
        set(CAE_ENABLE_MPIIO_ADAPTER OFF CACHE BOOL "Disabled due to Hermes being disabled" FORCE)
        set(CAE_ENABLE_VFD OFF CACHE BOOL "Disabled due to Hermes being disabled" FORCE)
    endif()
endif()

option(CAE_MPICH "Specify that this a MPICH build" OFF)
option(CAE_OPENMPI "Specify that this a OpenMPI build" OFF)

option(CAE_ENABLE_CUDA "Enable CUDA support" OFF)
option(CAE_ENABLE_ROCM "Enable ROCm support" OFF)

# A hack for spack to get dependencies
option(CAE_NO_COMPILE "Don't compile the code" OFF)

if(CAE_NO_COMPILE)
    install(
        FILES
        ${CMAKE_SOURCE_DIR}/.gitignore
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/.gitignore
    )
    return()
endif()

# ------------------------------------------------------------------------------
# DOTENV
# ------------------------------------------------------------------------------ 
if(CAE_ENABLE_CMAKE_DOTENV)
    if(EXISTS ${CMAKE_SOURCE_DIR}/.env.cmake)
        include(.env.cmake)
    endif() 
endif()

# -----------------------------------------------------------------------------
# Compiler Optimization
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_compile_definitions(CAE_LOG_VERBOSITY=10)
else()
    message("IN RELEASE MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")
    add_compile_definitions(CAE_LOG_VERBOSITY=1)
endif()

# Enable Position Independent Code globally
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Targets built within this project are exported at Install time for use
# by other projects.
# -----------------------------------------------------------------------------
if(CAE_ENABLE_HERMES)
    if(NOT CHIMAERA_EXPORTED_TARGETS)
        set(CHIMAERA_EXPORTED_TARGETS "HermesCore")
    endif()
else()
    if(NOT CHIMAERA_EXPORTED_TARGETS)
        set(CHIMAERA_EXPORTED_TARGETS "")
    endif()
endif()

# -----------------------------------------------------------------------------
# Code Coverage
# -----------------------------------------------------------------------------
if(CAE_ENABLE_COVERAGE)
    set(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage --coverage" CACHE STRING
        "Flags to the coverage program to perform coverage inspection"
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_FLAGS}")

    mark_as_advanced(COVERAGE_FLAGS)

    macro(set_coverage_flags target)
        target_link_libraries(${target} gcov)

        # set_target_properties(${target}
        # PROPERTIES
        # COMPILE_FLAGS ${COVERAGE_FLAGS}
        # LINK_FLAGS ${COVERAGE_FLAGS}
        # )
    endmacro()
endif()

add_custom_target(coverage COMMAND bash ${CMAKE_SOURCE_DIR}/ci/coverage.sh
    ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR})

# ------------------------------------------------------------------------------
# Setup install and output Directories
# ------------------------------------------------------------------------------
if(NOT CAE_INSTALL_BIN_DIR)
    set(CAE_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()

if(NOT CAE_INSTALL_LIB_DIR)
    set(CAE_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if(NOT CAE_INSTALL_INCLUDE_DIR)
    set(CAE_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()

if(NOT CAE_INSTALL_DATA_DIR)
    set(CAE_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

if(NOT CAE_INSTALL_MAN_DIR)
    set(CAE_INSTALL_MAN_DIR ${CMAKE_INSTALL_PREFIX}/share/man)
endif()

# -----------------------------------------------------------------------------
# Find Packages
# -----------------------------------------------------------------------------

# This is for compatibility with CLANG + Spack
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_INSTALL_RPATH "${CAE_INSTALL_LIB_DIR}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Include most packages needed by both client and host
if(CAE_ENABLE_HERMES)
    find_package(Hermes REQUIRED)
    message("Found hermes at ${Hermes_LIB_DIR}")
else()
    message(STATUS "Hermes support disabled - skipping Hermes package finding")
endif()

# ENABLE GPU SUPPORT
if(CAE_ENABLE_HERMES)
    if(CHIMAERA_ENABLE_CUDA)
        hshm_enable_cuda(17)
    endif()

    if(CHIMAERA_ENABLE_ROCM)
        hshm_enable_rocm("HIP" 17)
    endif()
else()
    message(STATUS "GPU support disabled (Hermes disabled)")
endif()

# Zeromq
# pkg_check_modules(ZMQ REQUIRED libzmq)
# include_directories(${ZMQ_INCLUDE_DIRS})
# message("Found libzmq at: ${ZMQ_INCLUDE_DIRS}")

# HDF5 (only for VFD, not the basic client)
if(CAE_ENABLE_HERMES AND CAE_ENABLE_VFD)
    set(CAE_REQUIRED_HDF5_VERSION 1.14.0)
    set(CAE_REQUIRED_HDF5_COMPONENTS C)
    find_package(HDF5 ${CAE_REQUIRED_HDF5_VERSION} CONFIG NAMES hdf5
        COMPONENTS ${CAE_REQUIRED_HDF5_COMPONENTS} shared)

    if(HDF5_FOUND)
        message(STATUS "found HDF5 ${HDF5_VERSION} at ${HDF5_INCLUDE_DIR}")
        set(HDF5_CAE_VFD_EXT_INCLUDE_DEPENDENCIES
            ${HDF5_CAE_VFD_EXT_INCLUDE_DEPENDENCIES}
            ${HDF5_INCLUDE_DIR})
        set(HDF5_CAE_VFD_EXT_LIB_DEPENDENCIES
            ${HDF5_CAE_VFD_EXT_LIB_DEPENDENCIES}
            ${HDF5_C_SHARED_LIBRARY})
    else()
        # Allow for HDF5 autotools builds
        find_package(HDF5 ${CAE_REQUIRED_HDF5_VERSION} MODULE REQUIRED
            COMPONENTS ${CAE_REQUIRED_HDF5_COMPONENTS})

        if(HDF5_FOUND)
            set(HDF5_CAE_VFD_EXT_INCLUDE_DEPENDENCIES
                ${HDF5_CAE_VFD_EXT_INCLUDE_DEPENDENCIES}
                ${HDF5_INCLUDE_DIRS})
            set(HDF5_CAE_VFD_EXT_LIB_DEPENDENCIES
                ${HDF5_CAE_VFD_EXT_LIB_DEPENDENCIES}
                ${HDF5_LIBRARIES})
        else()
            message(FATAL_ERROR "Could not find HDF5, please set HDF5_DIR (1.13.0) or HDF5_ROOT (1.13.1).")
        endif()
    endif()
endif()

# ------------------------------------------------------------------------------
# Setup CMake Environment
# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries.")

# -----------------------------------------------------------------------------
# Build Hermes Main Packages
# -----------------------------------------------------------------------------
# Set CAE directory variables for adapters
set(CAE_SRC_DIR ${CMAKE_SOURCE_DIR})
set(CAE_ADAPTER_DIR ${CMAKE_SOURCE_DIR}/adapters)
set(CAE_IO_CLIENT_DIR ${CMAKE_SOURCE_DIR}/adapters)
set(CAE_VFD_DIR_NAME "vfd")

# Main includes
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
if(CAE_ENABLE_HERMES)
    link_directories(${HermesShm_LIBRARY_DIRS})
endif()

# Task includes
include_directories(${CMAKE_SOURCE_DIR}/tasks)
if(CAE_ENABLE_HERMES)
    include_directories(${CMAKE_SOURCE_DIR}/tasks/hermes_core/include)
endif()

# Test includes
include_directories(${CMAKE_SOURCE_DIR}/test/unit)

set(TEST_MAIN ${CMAKE_SOURCE_DIR}/test/unit) 
if(CAE_ENABLE_HERMES)
    add_subdirectory(adapters)
else()
    message(STATUS "Skipping adapters directory (Hermes disabled)")
endif()
add_subdirectory(omni)

add_custom_target(lint COMMAND bash ${CMAKE_SOURCE_DIR}/ci/lint.sh ${CMAKE_SOURCE_DIR})

# -----------------------------------------------------------------------------
# Build + Enable Testing
# -----------------------------------------------------------------------------
# General function used to hook ctest to python test tool lib
function(pytest test_type test_name)
    if(CAE_ENABLE_HERMES)
        set(script ${CMAKE_SOURCE_DIR}/scripts/ci/py_hermes_ci/bin/run_test)

        # add_test(NAME ${test_name}
        # COMMAND ${script} ${test_type} ${test_name} ${CMAKE_BINARY_DIR} ${CAE_USE_ADDRESS_SANITIZER})
    else()
        message(STATUS "Skipping Hermes test setup (Hermes disabled)")
    endif()
endfunction()

function(jarvis_test test_type test_name)
    add_test(NAME ${test_name}
        COMMAND jarvis pipeline run yaml "${CMAKE_SOURCE_DIR}/test/jarvis_wrp_cae/pipelines/${test_type}/${test_name}.yaml")
endfunction()

enable_testing()
if(CAE_ENABLE_HERMES)
    add_subdirectory(test/unit)
else()
    message(STATUS "Skipping Hermes unit tests (Hermes disabled)")
endif()

# Install headers if Hermes is enabled
if(CAE_ENABLE_HERMES)
    # install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if(CAE_ENABLE_HERMES)
    jarvis_repo_add("${CMAKE_SOURCE_DIR}/test/jarvis_wrp_cae" "${CMAKE_SOURCE_DIR}/test/jarvis_wrp_cae/pipelines")
else()
    message(STATUS "Skipping Hermes jarvis repo setup (Hermes disabled)")
endif()

