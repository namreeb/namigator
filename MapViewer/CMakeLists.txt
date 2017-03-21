set(EXECUTABLE_NAME MapViewer)

set(SRC
    Source/Camera.cpp
    Source/CommonControl.cpp
    Source/DetourDebugDraw.cpp
    Source/main.cpp
    Source/Renderer.cpp
)

set(SHADERS
    Source/pixelShader.hlsl
    Source/vertexShader.hlsl
)

set_source_files_properties(Source/pixelShader.hlsl PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 4.0
    VS_SHADER_ENTRYPOINT PShader
    VS_SHADER_FLAGS "/Fh \"${CMAKE_CURRENT_SOURCE_DIR}/Include/pixelShader.hpp\""
)

set_source_files_properties(Source/vertexShader.hlsl PROPERTIES
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 4.0
    VS_SHADER_ENTRYPOINT VShader
    VS_SHADER_FLAGS "/Fh \"${CMAKE_CURRENT_SOURCE_DIR}/Include/vertexShader.hpp\""
)

include_directories(Include
    ${CMAKE_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/recastnavigation/Detour/Include"
    "${CMAKE_SOURCE_DIR}/recastnavigation/DetourTileCache/Include"
)

add_executable(${EXECUTABLE_NAME} WIN32 ${SRC} ${SHADERS})

target_link_libraries(${EXECUTABLE_NAME} utility parser pathfind storm recastnavigation)
add_dependencies(MapViewer utility parser pathfind recastnavigation)