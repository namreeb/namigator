include_directories(Include ${CMAKE_SOURCE_DIR})

add_library(pathfind STATIC
    Source/Map.cpp
    Source/TemporaryObstacle.cpp
    Source/Tile.cpp
)
add_dependencies(pathfind utility recastnavigation)
