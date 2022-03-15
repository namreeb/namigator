set(EXECUTABLE_NAME MapBuilder)
set(PYTHON_NAME mapbuild)

set(SRC BVHConstructor.cpp GameObjectBVHBuilder.cpp MeshBuilder.cpp RecastContext.cpp Worker.cpp)

add_executable(${EXECUTABLE_NAME} ${SRC} main.cpp)
target_include_directories(${EXECUTABLE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(${EXECUTABLE_NAME} PRIVATE parser utility RecastNavigation::Recast RecastNavigation::Detour storm ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)

if (Python3_FOUND)
    Python3_add_library(${PYTHON_NAME} MODULE ${SRC} python.cpp)
    target_compile_definitions(${PYTHON_NAME} PUBLIC PYTHON)
    target_include_directories(${PYTHON_NAME} PUBLIC ${Python3_INCLUDE_DIRS})
    target_link_libraries(${PYTHON_NAME} PRIVATE parser utility RecastNavigation::Recast RecastNavigation::Detour storm)

    install(TARGETS ${PYTHON_NAME} DESTINATION lib)
endif()
