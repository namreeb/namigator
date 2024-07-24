cmake_minimum_required(VERSION 3.12)

project(namigator)

# namigator depends on a compiler which supports C++17
set(CMAKE_CXX_STANDARD 17)

# generate position independent code for everything
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default to release build type if none is specified
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type")
endif()

# threading library is required
find_package(Threads REQUIRED)

# this is used a lot for serialization
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-multichar")
endif()

option(NAMIGATOR_BUILD_PYTHON "Build Python bindings if Python2/3 is present." TRUE)
option(NAMIGATOR_INSTALL_TESTS "Install tests." TRUE)
option(NAMIGATOR_BUILD_C_API "Build the C API." TRUE)
option(NAMIGATOR_BUILD_EXECUTABLES "Build the MapViewer executable. Windows only." TRUE)

if(NAMIGATOR_BUILD_PYTHON)
    add_subdirectory(pybind11)
else()
    message(WARNING "Python bindings for ${CMAKE_PROJECT_NAME} will not be compiled")
endif()

set(FILESYSTEM_LIBRARY "")
# GCC 7 requires an extra lib for this, but MSVC does not
if (UNIX AND NOT APPLE)
    set(FILESYSTEM_LIBRARY "stdc++fs")
endif()

# we want this definition present globally
add_definitions(-DDT_POLYREF64)

# third party dependencies
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "Build demo")
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "Build tests")
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "Build examples")
add_subdirectory(recastnavigation EXCLUDE_FROM_ALL)

if (NAMIGATOR_BUILD_C_API)
    install(TARGETS Detour Recast ARCHIVE DESTINATION lib)
endif()

# This is just easier than copying over the DLLs
if (MSVC)
    set(STORM_USE_BUNDLED_LIBRARIES ON CACHE BOOL "")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif()

add_subdirectory(stormlib EXCLUDE_FROM_ALL)
if (NAMIGATOR_BUILD_C_API)
    install(TARGETS storm ARCHIVE DESTINATION lib)
endif()

# namigator libraries
add_subdirectory(utility)

add_subdirectory(parser)
add_subdirectory(pathfind)

# namigator executables
add_subdirectory(MapBuilder)

if(NAMIGATOR_INSTALL_TESTS)
    add_subdirectory(test)
endif()

if (WIN32 AND NAMIGATOR_BUILD_EXECUTABLES)
    add_subdirectory(MapViewer)
endif()
