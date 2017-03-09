project(namigator)

cmake_minimum_required(VERSION 3.2.3)

# for all code which uses Detour, we want this definition present
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