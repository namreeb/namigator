set(LIBRARY_NAME libpathfind)
set(PYTHON_NAME pathfind)

set(SRC
    BVH.cpp
    Map.cpp
    TemporaryObstacle.cpp
    Tile.cpp
)
if (NAMIGATOR_BUILD_C_API)
    set(SRC ${SRC} pathfind_c_bindings.cpp)
endif()

add_library(${LIBRARY_NAME} STATIC ${SRC})
target_include_directories(${LIBRARY_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${LIBRARY_NAME} PRIVATE ${FILESYSTEM_LIBRARY} utility RecastNavigation::Recast RecastNavigation::Detour)

if (NAMIGATOR_BUILD_C_API)
    install(TARGETS ${LIBRARY_NAME} ARCHIVE DESTINATION lib)
endif()

if (NAMIGATOR_BUILD_PYTHON)
    pybind11_add_module(${PYTHON_NAME} python.cpp )

    target_link_libraries(${PYTHON_NAME} PRIVATE ${LIBRARY_NAME} ${FILESYSTEM_LIBRARY} utility RecastNavigation::Recast RecastNavigation::Detour storm)

    install(TARGETS ${PYTHON_NAME} DESTINATION namigator)
endif()
