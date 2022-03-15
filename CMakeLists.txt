project(namigator)

cmake_minimum_required(VERSION 3.12)

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

# First try finding python 3 development files
find_package(Python3 COMPONENTS Development.Module)

if (NOT Python3_FOUND)
    message(WARNING "No python3 was found.  Python bindings for ${CMAKE_PROJECT_NAME} will not be compiled")
endif()

# GCC 7 requires an extra lib for this, but MSVC does not
if (MSVC)
    set(FILESYSTEM_LIBRARY "")
else()
    set(FILESYSTEM_LIBRARY "stdc++fs")
endif()

# we want this definition present globally
add_definitions(-DDT_POLYREF64)

# third party dependencies
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "Build demo")
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "Build tests")
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "Build examples")
add_subdirectory(recastnavigation EXCLUDE_FROM_ALL)

add_definitions(-DSTORMLIB_NO_AUTO_LINK)
add_subdirectory(stormlib EXCLUDE_FROM_ALL)

# namigator libraries
add_subdirectory(utility)

add_subdirectory(parser)
add_subdirectory(pathfind)

# namigator executables
add_subdirectory(MapBuilder)

# tests
add_subdirectory(test)

if (WIN32)
    add_subdirectory(MapViewer)
endif()
