project(namigator)

cmake_minimum_required(VERSION 3.2.3)

# namigator depends on a compiler which supports C++14 
set(CMAKE_CXX_STANDARD 14)

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