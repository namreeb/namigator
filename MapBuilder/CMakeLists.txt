set(EXECUTABLE_NAME MapBuilder)
set(PYTHON_NAME mapbuild)

set(SRC GameObjectBVHBuilder.cpp MeshBuilder.cpp RecastContext.cpp Worker.cpp)

include_directories(Include "${CMAKE_CURRENT_SOURCE_DIR}/../" "${CMAKE_CURRENT_SOURCE_DIR}/../recastnavigation/Detour/Include")

add_executable(${EXECUTABLE_NAME} ${SRC} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE parser utility recastnavigation storm ${CMAKE_THREAD_LIBS_INIT} ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})
add_dependencies(${EXECUTABLE_NAME} parser utility recastnavigation storm)

if (PYTHON_COMPONENT)
    if (Python3_FOUND)
        Python3_add_library(${PYTHON_NAME} ${SRC} python.cpp)
    elseif (Python2_FOUND)
        Python2_add_library(${PYTHON_NAME} ${SRC} python.cpp)
    else()
        message(FATAL_ERROR "Python component defined but neither python2 nor python3 were found")
    endif()

    target_compile_definitions(${PYTHON_NAME} PUBLIC PYTHON BOOST_PYTHON_STATIC_LIB)
    include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    add_dependencies(${PYTHON_NAME} parser utility recastnavigation storm)
    target_link_libraries(${PYTHON_NAME} PRIVATE parser utility recastnavigation storm ${Boost_LIBRARIES})
endif()
