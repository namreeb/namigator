include_directories(Include ${CMAKE_SOURCE_DIR})

add_library(pathfind STATIC
    Map.cpp
    TemporaryObstacle.cpp
    Tile.cpp
)
add_dependencies(pathfind utility recastnavigation)
