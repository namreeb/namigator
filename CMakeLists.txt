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

# currently there is a bug in the cmake included with visual studio where the wrong
# version number is resolved for the compiler.  this should work around it.
# FIXME: this should be removed once visual studio begins using cmake 3.8
if (MSVC)
	if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.10)
        set(Boost_COMPILER -vc141)
    endif()
endif()

find_package(PythonInterp)
find_package(PythonLibs)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if (${PYTHON_VERSION_MAJOR} EQUAL 3)
    # not sure why this is necessary, but it seems that it is
    set(Boost_FIND_COMPONENTS python3)

    find_package(boost REQUIRED COMPONENTS python3)
else()
    # not sure why this is necessary, but it seems that it is
    set(Boost_FIND_COMPONENTS python)

    find_package(boost REQUIRED COMPONENTS python)
endif()

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