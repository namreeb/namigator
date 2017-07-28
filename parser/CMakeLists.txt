include_directories(Include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/stormlib/src)

set(SRC_ADT_CHUNKS
    Source/Adt/Chunks/Subchunks/MCLQ.cpp
    Source/Adt/Chunks/Subchunks/MCVT.cpp
    Source/Adt/Chunks/MCNK.cpp
    Source/Adt/Chunks/MDDF.cpp
    Source/Adt/Chunks/MH2O.cpp
    Source/Adt/Chunks/MHDR.cpp
    Source/Adt/Chunks/MMDX.cpp
    Source/Adt/Chunks/MODF.cpp
    Source/Adt/Chunks/MWMO.cpp
)

set(SRC_ADT ${SRC_ADT_CHUNKS}
    Source/Adt/Adt.cpp
    Source/Adt/AdtChunk.cpp
)

set(SRC_DOODAD
    Source/Doodad/Doodad.cpp
    Source/Doodad/DoodadInstance.cpp
)

set(SRC_MAP
    Source/Map/Map.cpp
)

set(SRC_WMO_GROUP_CHUNKS
    "Source/Wmo/GroupFile/Chunks/MLIQ.cpp"
    "Source/Wmo/GroupFile/Chunks/MOPY.cpp"
    "Source/Wmo/GroupFile/Chunks/MOVI.cpp"
    "Source/Wmo/GroupFile/Chunks/MOVT.cpp"
)

set(SRC_WMO_GROUP ${SRC_WMO_GROUP_CHUNKS}
    "Source/Wmo/GroupFile/WmoGroupChunk.cpp"
    "Source/Wmo/GroupFile/WmoGroupFile.cpp"
)

set(SRC_WMO_ROOT_CHUNKS
    "Source/Wmo/RootFile/Chunks/MODD.cpp"
    "Source/Wmo/RootFile/Chunks/MODN.cpp"
    "Source/Wmo/RootFile/Chunks/MODS.cpp"
)

set(SRC_WMO_ROOT ${SRC_WMO_ROOT_CHUNKS}
)

set(SRC_WMO ${SRC_WMO_GROUP} ${SRC_WMO_ROOT}
    "Source/Wmo/Wmo.cpp"
    "Source/Wmo/WmoDoodad.cpp"
    "Source/Wmo/WmoInstance.cpp"
)

add_library(parser STATIC ${SRC_MAP} ${SRC_ADT} ${SRC_WMO} ${SRC_DOODAD}
    Source/DBC.cpp
    Source/MpqManager.cpp
    Source/parser.cpp
)

add_dependencies(parser utility)