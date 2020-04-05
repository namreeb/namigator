project(namigator)

cmake_minimum_required(VERSION 3.2.3)

# namigator depends on a compiler which supports C++14
set(CMAKE_CXX_STANDARD 14)

# threading library is required
find_package(Threads REQUIRED)

# this is used a lot for serialization
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-multichar")
endif()

# if we have python and boost.python, enable generation of python bindings

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(PYTHON_COMPONENT "python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}")

find_package(Boost REQUIRED COMPONENTS "${PYTHON_COMPONENT}")

# we want this definition present globally
add_definitions(-DDT_POLYREF64)

# third party dependencies
add_subdirectory(RecastDetourBuild)
add_subdirectory(stormlib)

# namigator libraries
add_subdirectory(utility)

add_subdirectory(parser)
add_subdirectory(pathfind)

# namigator executables
add_subdirectory(MapBuilder)

if (WIN32)
    add_subdirectory(MapViewer)
endif()
