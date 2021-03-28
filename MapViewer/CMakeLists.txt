set(EXECUTABLE_NAME MapViewer)

set(SRC
    Camera.cpp
    CommonControl.cpp
    DetourDebugDraw.cpp
    main.cpp
    Renderer.cpp
)

set(SHADERS
    pixelShader.hlsl
    VertexShader.hlsl
)

set_source_files_properties(pixelShader.hlsl PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 4.0
    VS_SHADER_ENTRYPOINT PShader
    VS_SHADER_FLAGS "/Fh \"${CMAKE_CURRENT_SOURCE_DIR}/pixelShader.hpp\""
)

set_source_files_properties(VertexShader.hlsl PROPERTIES
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 4.0
    VS_SHADER_ENTRYPOINT VShader
    VS_SHADER_FLAGS "/Fh \"${CMAKE_CURRENT_SOURCE_DIR}/VertexShader.hpp\""
)

add_executable(${EXECUTABLE_NAME} WIN32 ${SHADERS} ${SRC})

target_link_libraries(${EXECUTABLE_NAME} PRIVATE utility parser libpathfind storm RecastNavigation::Detour RecastNavigation::DebugUtils)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)