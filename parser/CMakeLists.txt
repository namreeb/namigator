add_library(parser STATIC ${SRC_MAP} ${SRC_ADT} ${SRC_WMO} ${SRC_DOODAD}
    Adt/Adt.cpp
    Adt/AdtChunk.cpp
    Adt/Chunks/Subchunks/MCLQ.cpp
    Adt/Chunks/Subchunks/MCVT.cpp
    Adt/Chunks/MCNK.cpp
    Adt/Chunks/MDDF.cpp
    Adt/Chunks/MH2O.cpp
    Adt/Chunks/MHDR.cpp
    Adt/Chunks/MMDX.cpp
    Adt/Chunks/MODF.cpp
    Adt/Chunks/MWMO.cpp
    Doodad/Doodad.cpp
    Doodad/DoodadInstance.cpp
    Wmo/Wmo.cpp
    Wmo/WmoDoodad.cpp
    Wmo/WmoInstance.cpp
    Wmo/GroupFile/WmoGroupChunk.cpp
    Wmo/GroupFile/WmoGroupFile.cpp
    Wmo/GroupFile/Chunks/MLIQ.cpp
    Wmo/GroupFile/Chunks/MOPY.cpp
    Wmo/GroupFile/Chunks/MOVI.cpp
    Wmo/GroupFile/Chunks/MOVT.cpp
    Wmo/RootFile/Chunks/MODD.cpp
    Wmo/RootFile/Chunks/MODN.cpp
    Wmo/RootFile/Chunks/MODS.cpp
    Map/Map.cpp
    DBC.cpp
    MpqManager.cpp
)

target_link_libraries(parser utility storm ${FILESYSTEM_LIBRARY})
target_include_directories(parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
