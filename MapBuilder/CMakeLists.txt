set(EXECUTABLE_NAME MapBuilder)
set(LIBRARY_NAME libmapbuild)
set(PYTHON_NAME mapbuild)

set(SRC BVHConstructor.cpp GameObjectBVHBuilder.cpp MeshBuilder.cpp RecastContext.cpp Worker.cpp FileExist.cpp)
if (NAMIGATOR_BUILD_C_API)
    set(SRC ${SRC} MapBuilder_c_bindings.cpp)
endif()

add_library(${LIBRARY_NAME} ${SRC})
target_include_directories(${LIBRARY_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(${LIBRARY_NAME} PRIVATE parser utility RecastNavigation::Recast RecastNavigation::Detour storm ${CMAKE_THREAD_LIBS_INIT})

if (NAMIGATOR_BUILD_C_API)
    install(TARGETS ${LIBRARY_NAME} ARCHIVE DESTINATION lib)
endif()

if (NAMIGATOR_BUILD_EXECUTABLES)
    add_executable(${EXECUTABLE_NAME} main.cpp)
    target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME})
    install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
endif()

if (NAMIGATOR_BUILD_PYTHON)
    pybind11_add_module(${PYTHON_NAME} python.cpp)

    target_link_libraries(${PYTHON_NAME} PRIVATE ${LIBRARY_NAME} parser utility RecastNavigation::Recast RecastNavigation::Detour storm)

    install(TARGETS ${PYTHON_NAME} DESTINATION namigator)
endif()
