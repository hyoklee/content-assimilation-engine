cmake_minimum_required(VERSION 3.28)
add_subdirectory(filesystem)
add_subdirectory(posix)

#-----------------------------------------------------------------------------
# Install adapter headers
#-----------------------------------------------------------------------------
file(GLOB_RECURSE HERMES_HEADERS "*.h")
install(
        FILES
        ${HERMES_HEADERS}
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/hermes_adapters
        COMPONENT
        headers
)
install(DIRECTORY mapper DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hermes_adapters)
