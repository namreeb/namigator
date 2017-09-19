set(EXECUTABLE_NAME MapBuilder)
set(PYTHON_NAME mapbuild)

set(SRC Source/GameObjectBVHBuilder.cpp Source/MeshBuilder.cpp Source/RecastContext.cpp Source/Worker.cpp)

include_directories(Include "${CMAKE_CURRENT_SOURCE_DIR}/../" "${CMAKE_CURRENT_SOURCE_DIR}/../recastnavigation/Detour/Include")

add_executable(${EXECUTABLE_NAME} ${SRC} Source/main.cpp)
target_link_libraries(${EXECUTABLE_NAME} utility parser recastnavigation storm ${CMAKE_THREAD_LIBS_INIT} ${PYTHON_LIBRARIES} ${Boost_PYTHON_LIBRARY})
add_dependencies(${EXECUTABLE_NAME} utility parser recastnavigation storm)

if (Boost_FOUND)
    PYTHON_ADD_MODULE(${PYTHON_NAME} ${SRC} Source/python.cpp)
    target_compile_definitions(${PYTHON_NAME} PUBLIC PYTHON)
    include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(${PYTHON_NAME} utility parser recastnavigation storm ${CMAKE_THREAD_LIBS_INIT} ${PYTHON_LIBRARIES} ${Boost_PYTHON_LIBRARY})
    add_dependencies(${PYTHON_NAME} utility parser recastnavigation storm)
endif()
