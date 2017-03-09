# these paths are relative to the recastnavigation directory
set(SRC_RECAST
    Recast/Source/Recast.cpp
    Recast/Source/RecastAlloc.cpp
    Recast/Source/RecastArea.cpp
    Recast/Source/RecastContour.cpp
    Recast/Source/RecastFilter.cpp
    Recast/Source/RecastMesh.cpp
    Recast/Source/RecastMeshDetail.cpp
    Recast/Source/RecastRasterization.cpp
    Recast/Source/RecastRegion.cpp
)

set(INCLUDE_RECAST
    Recast/Include
)

set(SRC_DETOUR
    Detour/Source/DetourCommon.cpp
    Detour/Source/DetourAlloc.cpp
    Detour/Source/DetourNavMesh.cpp
    Detour/Source/DetourNavMeshBuilder.cpp
    Detour/Source/DetourNavMeshQuery.cpp
    Detour/Source/DetourNode.cpp
    DebugUtils/Source/DebugDraw.cpp
    DebugUtils/Source/DetourDebugDraw.cpp
)

set(INCLUDE_DETOUR
    Detour/Include
    DebugUtils/Include
    DetourTileCache/Include
)

# path translation is done here:
set(SRC_RECAST_ACTUAL )
foreach(SRC ${SRC_RECAST})
    list(APPEND SRC_RECAST_ACTUAL "${CMAKE_SOURCE_DIR}/recastnavigation/${SRC}")
endforeach(SRC)

set(INCLUDE_RECAST_ACTUAL )
foreach(INCLUDE ${INCLUDE_RECAST})
    list(APPEND INCLUDE_RECAST_ACTUAL "${CMAKE_SOURCE_DIR}/recastnavigation/${INCLUDE}")
endforeach(INCLUDE)

set(SRC_DETOUR_ACTUAL )
foreach(SRC ${SRC_DETOUR})
    list(APPEND SRC_DETOUR_ACTUAL "${CMAKE_SOURCE_DIR}/recastnavigation/${SRC}")
endforeach(SRC)

set(INCLUDE_DETOUR_ACTUAL )
foreach(INCLUDE ${INCLUDE_DETOUR})
    list(APPEND INCLUDE_DETOUR_ACTUAL "${CMAKE_SOURCE_DIR}/recastnavigation/${INCLUDE}")
endforeach(INCLUDE)

include_directories(${INCLUDE_RECAST_ACTUAL} ${INCLUDE_DETOUR_ACTUAL})

add_library(recastnavigation STATIC ${SRC_RECAST_ACTUAL} ${SRC_DETOUR_ACTUAL})