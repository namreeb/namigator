set(LIBRARY_NAME pathfind)

set(SRC
    BVH.cpp
    Map.cpp
    TemporaryObstacle.cpp
    Tile.cpp
)

add_library(lib${LIBRARY_NAME} STATIC ${SRC})
target_include_directories(lib${LIBRARY_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(lib${LIBRARY_NAME} PRIVATE ${FILESYSTEM_LIBRARY} utility RecastNavigation::Recast RecastNavigation::Detour)

if (Python3_FOUND)
    Python3_add_library(${LIBRARY_NAME} MODULE ${SRC} python.cpp)
    target_compile_definitions(${LIBRARY_NAME} PUBLIC PYTHON)
    target_include_directories(${LIBRARY_NAME} PRIVATE ${Python3_INCLUDE_DIRS})
    target_link_libraries(${LIBRARY_NAME} PRIVATE ${FILESYSTEM_LIBRARY} utility RecastNavigation::Recast RecastNavigation::Detour storm)

    install(TARGETS ${LIBRARY_NAME} DESTINATION lib)
endif()
