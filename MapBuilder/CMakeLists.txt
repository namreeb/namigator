set(EXECUTABLE_NAME MapBuilder)
set(PYTHON_NAME mapbuild)

set(SRC GameObjectBVHBuilder.cpp MeshBuilder.cpp RecastContext.cpp Worker.cpp)

include_directories(Include "${CMAKE_CURRENT_SOURCE_DIR}/../" "${CMAKE_CURRENT_SOURCE_DIR}/../recastnavigation/Detour/Include")

add_executable(${EXECUTABLE_NAME} ${SRC} main.cpp)
target_link_libraries(${EXECUTABLE_NAME} parser utility recastnavigation storm ${CMAKE_THREAD_LIBS_INIT} ${PYTHON_LIBRARIES} ${Boost_PYTHON_LIBRARY})
add_dependencies(${EXECUTABLE_NAME} parser utility recastnavigation storm)

if (Boost_FOUND)
    find_package(PythonLibs)
    # Boost.Python must have static linking enabled explicitly per https://cmake.org/cmake/help/latest/module/FindBoost.html
    PYTHON_ADD_MODULE(${PYTHON_NAME} ${SRC} python.cpp)
    target_compile_definitions(${PYTHON_NAME} PUBLIC PYTHON BOOST_PYTHON_STATIC_LIB)
    include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(${PYTHON_NAME} parser utility recastnavigation storm ${CMAKE_THREAD_LIBS_INIT} ${PYTHON_LIBRARIES} ${Boost_PYTHON_LIBRARY})
    add_dependencies(${PYTHON_NAME} parser utility recastnavigation storm)
    message(STATUS "Boost::Python found.  Building Python bindings")
else()
    message(STATUS "Boost::Python not found.  Python bindings are disabled.")
endif()
