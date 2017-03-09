set(EXECUTABLE_NAME MapBuilder)

set(SRC Source/GameObjectBVHBuilder.cpp Source/main.cpp Source/MeshBuilder.cpp Source/RecastContext.cpp Source/Worker.cpp)

include_directories(Include ${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/recastnavigation/Detour/Include")

add_executable(${EXECUTABLE_NAME} ${SRC})

target_link_libraries(${EXECUTABLE_NAME} utility parser recastnavigation storm)
