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

unset(Python3_FOUND CACHE)
unset(PYTHON_COMPONENT CACHE)

# github python setup action will set this environment variable
if(DEFINED ENV{pythonLocation})
    set(Python3_ROOT_DIR $ENV{pythonLocation})
    set(Python3_FIND_REGISTRY NEVER)
endif()

option(NAMIGATOR_BUILD_PYTHON "Build Python bindings if Python2/3 is present." TRUE)
option(NAMIGATOR_INSTALL_TESTS "Install tests." TRUE)

if(NAMIGATOR_BUILD_PYTHON)
    # First try finding python 3 development files
    find_package(Python3 COMPONENTS Development)

    if (Python3_FOUND)
        set(PYTHON_COMPONENT "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
        set(PYTHON_LIBRARIES "${Python3_LIBRARIES}")
        set(PYTHON_INCLUDE_DIRS "${Python3_INCLUDE_DIRS}")
    else()
        message(STATUS "Could not find python3.  Attemping to find python2.")
        find_package(Python2 COMPONENTS Development)
        if (Python2_FOUND)
            set(PYTHON_COMPONENT "python${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
            set(PYTHON_LIBRARIES "${Python2_LIBRARIES}")
            set(PYTHON_INCLUDE_DIRS "${Python2_INCLUDE_DIRS}")
        else()
            message(STATUS "Could not find python2 either.")
        endif()
    endif()
endif() # NAMIGATOR_BUILD_PYTHON

# Enable debug output from FindBoost
#set(Boost_DEBUG ON)
#set(Boost_DETAILED_FAILURE_MSG ON)

if (PYTHON_COMPONENT)
    # There currently seems to be a bug with the Linux distro of boost binaries
    # where some symbols are not built with position independent code (-fPIC).
    # So rather than requiring our users to build boost themselves, we can just
    # disable static linking of Boost libraries for now.
    if (MSVC)
        set(Boost_USE_STATIC_LIBS ON)
    else()
        set(Boost_USE_STATIC_LIBS OFF)
    endif()

    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    set(Boost_FIND_QUIETLY ON)

    find_package(Boost COMPONENTS "${PYTHON_COMPONENT}")

    # If we found python 3 and boost rejected it, try again with python 2
    if (NOT Boost_FOUND AND Python3_FOUND)
        message(STATUS "Boost rejected ${PYTHON_COMPONENT}.  Trying with python2.")
        find_package(Python2 COMPONENTS Development)

        if (Python2_FOUND)
            message(STATUS "Python2 found.  Checking for support from boost.")
            set(PYTHON_COMPONENT "python${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
            set(PYTHON_LIBRARIES "${Python2_LIBRARIES}")
            set(PYTHON_INCLUDE_DIRS "${Python2_INCLUDE_DIRS}")
            find_package(Boost COMPONENTS "${PYTHON_COMPONENT}")
        else()
            message(STATUS "Could not find python2 either.")
        endif()

        # Failed?  Give up
        if (NOT Boost_FOUND)
            message(STATUS "Giving up on finding boost and python match")
            unset(PYTHON_COMPONENT)
            unset(PYTHON_LIBRARIES)
            unset(PYTHON_INCLUDE_DIRS)
        endif()
    endif()
endif()

if (NAMIGATOR_BUILD_PYTHON AND NOT PYTHON_COMPONENT)
    message(WARNING "No boost-compatible python was found.  Python bindings for ${CMAKE_PROJECT_NAME} will not be compiled")
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
install(TARGETS Detour Recast ARCHIVE DESTINATION lib)

# This is just easier than copying over the DLLs
if (MSVC)
    set(STORM_USE_BUNDLED_LIBRARIES ON CACHE BOOL "")
endif()
add_subdirectory(stormlib EXCLUDE_FROM_ALL)
install(TARGETS storm ARCHIVE DESTINATION lib)

# namigator libraries
add_subdirectory(utility)

add_subdirectory(parser)
add_subdirectory(pathfind)

# namigator executables
add_subdirectory(MapBuilder)

if(NAMIGATOR_INSTALL_TESTS)
    add_subdirectory(test)
endif()

if (WIN32)
    add_subdirectory(MapViewer)
endif()
