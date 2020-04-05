include_directories(Include ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/../" ${CMAKE_SOURCE_DIR}/stormlib/src)

set(SRC_ADT_CHUNKS
    Adt/Chunks/Subchunks/MCLQ.cpp
    Adt/Chunks/Subchunks/MCVT.cpp
    Adt/Chunks/MCNK.cpp
    Adt/Chunks/MDDF.cpp
    Adt/Chunks/MH2O.cpp
    Adt/Chunks/MHDR.cpp
    Adt/Chunks/MMDX.cpp
    Adt/Chunks/MODF.cpp
    Adt/Chunks/MWMO.cpp
)

set(SRC_ADT ${SRC_ADT_CHUNKS}
    Adt/Adt.cpp
    Adt/AdtChunk.cpp
)

set(SRC_DOODAD
    Doodad/Doodad.cpp
    Doodad/DoodadInstance.cpp
)

set(SRC_MAP
    Map/Map.cpp
)

set(SRC_WMO_GROUP_CHUNKS
    Wmo/GroupFile/Chunks/MLIQ.cpp
    Wmo/GroupFile/Chunks/MOPY.cpp
    Wmo/GroupFile/Chunks/MOVI.cpp
    Wmo/GroupFile/Chunks/MOVT.cpp
)

set(SRC_WMO_GROUP ${SRC_WMO_GROUP_CHUNKS}
    Wmo/GroupFile/WmoGroupChunk.cpp
    Wmo/GroupFile/WmoGroupFile.cpp
)

set(SRC_WMO_ROOT_CHUNKS
    Wmo/RootFile/Chunks/MODD.cpp
    Wmo/RootFile/Chunks/MODN.cpp
    Wmo/RootFile/Chunks/MODS.cpp
)

set(SRC_WMO_ROOT ${SRC_WMO_ROOT_CHUNKS}
)

set(SRC_WMO ${SRC_WMO_GROUP} ${SRC_WMO_ROOT}
    Wmo/Wmo.cpp
    Wmo/WmoDoodad.cpp
    Wmo/WmoInstance.cpp
)

add_library(parser STATIC ${SRC_MAP} ${SRC_ADT} ${SRC_WMO} ${SRC_DOODAD}
    DBC.cpp
    MpqManager.cpp
    parser.cpp
)

add_dependencies(parser utility)

if ("x_${CMAKE_CXX_COMPILER_ID}" STREQUAL "x_GNU")
    target_link_libraries(parser stdc++fs)
endif()
