#ifdef WIN32
// fix bug in visual studio headers
#define NOMINMAX
#endif

#include "MpqManager.hpp"
#include "Adt/AdtChunk.hpp"
#include "Adt/Chunks/MHDR.hpp"
#include "Adt/Chunks/MCNK.hpp"
#include "Adt/Chunks/MH2O.hpp"
#include "Adt/Chunks/MWMO.hpp"
#include "Adt/Chunks/MDDF.hpp"
#include "Adt/Chunks/MODF.hpp"
#include "Adt/Chunks/MMDX.hpp"
#include "Adt/Chunks/MDDF.hpp"
#include "Doodad/DoodadPlacement.hpp"

#include "Adt/Adt.hpp"
#include "Map/Map.hpp"

#include "utility/Vector.hpp"
#include "Common.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <set>
#include <memory>
#include <cassert>
#include <limits>
#include <algorithm>

#define ADT_VERSION 18

namespace parser
{
Adt::Adt(Map *map, int adtX, int adtY)
    : X(adtX), Y(adtY), m_map(map),
      Bounds({
            (32.f - static_cast<float>(adtY) - 1.f)*MeshSettings::AdtSize,
            (32.f - static_cast<float>(adtX) - 1.f)*MeshSettings::AdtSize,
            std::numeric_limits<float>::max()
        },
        {
            (32.f - static_cast<float>(adtY))*MeshSettings::AdtSize,
            (32.f - static_cast<float>(adtX))*MeshSettings::AdtSize,
            std::numeric_limits<float>::lowest()
        })
{
    std::stringstream ss;

    ss << "World\\maps\\" << map->Name << "\\" << map->Name << "_" << adtX << "_" << adtY << ".adt";

    std::unique_ptr<utility::BinaryStream> reader(sMpqManager.OpenFile(ss.str()));

    if (!reader)
        THROW("Failed to open ADT");

    if (reader->Read<std::uint32_t>() != input::AdtChunkType::MVER)
        THROW("MVER does not begin ADT file");

    reader->rpos(reader->rpos() + 4);

    if (reader->Read<std::uint32_t>() != ADT_VERSION)
        THROW("ADT version is incorrect");

    size_t mhdrLocation;
    if (!reader->GetChunkLocation("MHDR", mhdrLocation))
        THROW("MHDR not found");

    const input::MHDR header(mhdrLocation, reader.get());

    std::unique_ptr<input::MH2O> liquidChunk(header.Offsets.Mh2oOffset ? new input::MH2O(header.Offsets.Mh2oOffset + 0x14, reader.get()) : nullptr);

    size_t currMcnk;
    if (!reader->GetChunkLocation("MCNK", currMcnk))
        THROW("MCNK not found");

    std::unique_ptr<input::MCNK> chunks[16][16];

    bool hasMclq = false;
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            chunks[y][x] = std::make_unique<input::MCNK>(currMcnk, reader.get());
            currMcnk += 8 + chunks[y][x]->Size;

            if (chunks[y][x]->HasWater)
                hasMclq = true;
        }

    // MMDX
    std::vector<std::string> doodadNames;
    size_t mmdxLocation;
    if (reader->GetChunkLocation("MMDX", header.Offsets.MmdxOffset + 0x14, mmdxLocation))
    {
        input::MMDX doodadNamesChunk(mmdxLocation, reader.get());
        doodadNames = std::move(doodadNamesChunk.DoodadNames);
    }

    // MMID: XXX - other people read in the MMID chunk here.  do we actually need it?

    // MWMO
    std::vector<std::string> wmoNames;
    size_t mwmoLocation;
    if (reader->GetChunkLocation("MWMO", header.Offsets.MwmoOffset + 0x14, mwmoLocation))
    {
        input::MWMO wmoNamesChunk(mwmoLocation, reader.get());
        wmoNames = std::move(wmoNamesChunk.WmoNames);
    }

    // MWID: XXX - other people read in the MWID chunk here.  do we actually need it?

    // MDDF
    size_t mddfLocation;
    std::unique_ptr<input::MDDF> doodadChunk(reader->GetChunkLocation("MDDF", header.Offsets.MddfOffset + 0x14, mddfLocation) ? new input::MDDF(mddfLocation, reader.get()) : nullptr);

    // MODF
    size_t modfLocation;
    std::unique_ptr<input::MODF> wmoChunk(reader->GetChunkLocation("MODF", header.Offsets.ModfOffset + 0x14, modfLocation) ? new input::MODF(modfLocation, reader.get()) : nullptr);

    // Process all data into triangles/indices
    // Terrain

    for (int chunkY = 0; chunkY < 16; ++chunkY)
        for (int chunkX = 0; chunkX < 16; ++chunkX)
        {
            auto const mapChunk = chunks[chunkY][chunkX].get();

            AdtChunk *const chunk = new AdtChunk();

            memcpy(chunk->m_heights, mapChunk->Heights, sizeof(chunk->m_heights));

            chunk->m_terrainVertices = std::move(mapChunk->Positions);
            chunk->m_minZ = mapChunk->MinZ;
            chunk->m_maxZ = mapChunk->MaxZ;
            chunk->m_areaId = mapChunk->AreaId;
            chunk->m_zoneId = sMpqManager.GetZoneId(chunk->m_areaId);

            Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mapChunk->MaxZ);
            Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mapChunk->MinZ);

            memcpy(chunk->m_holeMap, mapChunk->HoleMap, sizeof(chunk->m_holeMap));

            // build index list to exclude holes (8 * 8 quads, 4 triangles per quad, 3 indices per triangle)
            chunk->m_terrainIndices.reserve(8 * 8 * 4 * 3);

            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x)
                {
                    // if this chunk has holes and this quad is one of them, skip it
                    if (mapChunk->HasHoles && mapChunk->HoleMap[y][x])
                        continue;

                    auto const currIndex = y * 17 + x;

                    // Upper triangle
                    chunk->m_terrainIndices.push_back(currIndex);
                    chunk->m_terrainIndices.push_back(currIndex + 9);
                    chunk->m_terrainIndices.push_back(currIndex + 1);

                    // Left triangle
                    chunk->m_terrainIndices.push_back(currIndex);
                    chunk->m_terrainIndices.push_back(currIndex + 17);
                    chunk->m_terrainIndices.push_back(currIndex + 9);

                    // Lower triangle
                    chunk->m_terrainIndices.push_back(currIndex + 9);
                    chunk->m_terrainIndices.push_back(currIndex + 17);
                    chunk->m_terrainIndices.push_back(currIndex + 18);

                    // Right triangle
                    chunk->m_terrainIndices.push_back(currIndex + 1);
                    chunk->m_terrainIndices.push_back(currIndex + 9);
                    chunk->m_terrainIndices.push_back(currIndex + 18);
                }

            m_chunks[chunkY][chunkX].reset(chunk);
        }

    // Water

    if (liquidChunk)
        for (size_t i = 0; i < liquidChunk->Layers.size(); ++i)
        {
            auto const layer = liquidChunk->Layers[i].get();
            auto const chunk = m_chunks[layer->Y][layer->X].get();

            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x)
                {
                    if (!layer->Render[y][x])
                        continue;

                    int terrainVert = y * 17 + x;
                    chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, layer->Heights[y + 0][x + 0] });

                    terrainVert = y * 17 + (x + 1);
                    chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, layer->Heights[y + 0][x + 1] });

                    terrainVert = (y + 1) * 17 + x;
                    chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, layer->Heights[y + 1][x + 0] });

                    terrainVert = (y + 1) * 17 + (x + 1);
                    chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, layer->Heights[y + 1][x + 1] });

                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 0][x + 0]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 0][x + 1]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 1][x + 0]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 1][x + 1]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 0][x + 0]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 0][x + 1]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 1][x + 0]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 1][x + 1]);

                    chunk->m_minZ = std::min(chunk->m_minZ, layer->Heights[y + 0][x + 0]);
                    chunk->m_minZ = std::min(chunk->m_minZ, layer->Heights[y + 0][x + 1]);
                    chunk->m_minZ = std::min(chunk->m_minZ, layer->Heights[y + 1][x + 0]);
                    chunk->m_minZ = std::min(chunk->m_minZ, layer->Heights[y + 1][x + 1]);
                    chunk->m_maxZ = std::max(chunk->m_maxZ, layer->Heights[y + 0][x + 0]);
                    chunk->m_maxZ = std::max(chunk->m_maxZ, layer->Heights[y + 0][x + 1]);
                    chunk->m_maxZ = std::max(chunk->m_maxZ, layer->Heights[y + 1][x + 0]);
                    chunk->m_maxZ = std::max(chunk->m_maxZ, layer->Heights[y + 1][x + 1]);

                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 4));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 2));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 3));

                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 2));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 1));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 3));
                }
        }

    if (hasMclq)
        for (int chunkY = 0; chunkY < 16; ++chunkY)
            for (int chunkX = 0; chunkX < 16; ++chunkX)
            {
                auto const mclqBlock = chunks[chunkY][chunkX]->LiquidChunk.get();

                if (!mclqBlock)
                    continue;

                auto const chunk = m_chunks[chunkY][chunkX].get();

                // four vertices per square, 8x8 squares (max)
                chunk->m_liquidVertices.reserve(chunk->m_liquidVertices.size() + 4 * 8 * 8);

                // six indices (two triangles) per square
                chunk->m_liquidIndices.reserve(chunk->m_liquidIndices.size() + 6 * 8 * 8);

                for (int y = 0; y < 8; ++y)
                    for (int x = 0; x < 8; ++x)
                    {
                        if (mclqBlock->RenderMap[y][x] == 0xF)
                            continue;

                        // XXX FIXME - this is here because it may be that this bit is what actually controls rendering.  trap with assert to debug
                        assert(mclqBlock->RenderMap[y][x] != 8);

                        int terrainVert = y * 17 + x;
                        chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, mclqBlock->Heights[y][x] });

                        terrainVert = y * 17 + (x + 1);
                        chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, mclqBlock->Heights[y][x + 1] });

                        terrainVert = (y + 1) * 17 + x;
                        chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, mclqBlock->Heights[y + 1][x] });

                        terrainVert = (y + 1) * 17 + (x + 1);
                        chunk->m_liquidVertices.push_back({ chunk->m_terrainVertices[terrainVert].X, chunk->m_terrainVertices[terrainVert].Y, mclqBlock->Heights[y + 1][x + 1] });

                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 0][x + 0]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 0][x + 1]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 1][x + 0]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 1][x + 1]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 0][x + 0]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 0][x + 1]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 1][x + 0]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 1][x + 1]);

                        chunk->m_minZ = std::min(chunk->m_minZ, mclqBlock->Heights[y + 0][x + 0]);
                        chunk->m_minZ = std::min(chunk->m_minZ, mclqBlock->Heights[y + 0][x + 1]);
                        chunk->m_minZ = std::min(chunk->m_minZ, mclqBlock->Heights[y + 1][x + 0]);
                        chunk->m_minZ = std::min(chunk->m_minZ, mclqBlock->Heights[y + 1][x + 1]);
                        chunk->m_maxZ = std::max(chunk->m_maxZ, mclqBlock->Heights[y + 0][x + 0]);
                        chunk->m_maxZ = std::max(chunk->m_maxZ, mclqBlock->Heights[y + 0][x + 1]);
                        chunk->m_maxZ = std::max(chunk->m_maxZ, mclqBlock->Heights[y + 1][x + 0]);
                        chunk->m_maxZ = std::max(chunk->m_maxZ, mclqBlock->Heights[y + 1][x + 1]);

                        chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 4));
                        chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 2));
                        chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 3));

                        chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 2));
                        chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 1));
                        chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(chunk->m_liquidVertices.size() - 3));
                    }
            }

    // WMOs

    if (wmoChunk)
        for (auto const &wmoDefinition : wmoChunk->Wmos)
        {
            auto const wmo = map->GetWmo(wmoNames[wmoDefinition.NameId]);
            const WmoInstance *wmoInstance;

            // ensure that the instance has been loaded
            if ((wmoInstance = map->GetWmoInstance(static_cast<unsigned int>(wmoDefinition.UniqueId))) == nullptr)
            {
                math::BoundingBox bounds;
                wmoDefinition.GetBoundingBox(bounds);

                math::Matrix transformMatrix;
                wmoDefinition.GetTransformMatrix(transformMatrix);

                wmoInstance = new WmoInstance(wmo, wmoDefinition.DoodadSet, wmoDefinition.NameSet, bounds, transformMatrix);
                map->InsertWmoInstance(wmoDefinition.UniqueId, wmoInstance);
            }

            assert(!!wmo && !!wmoInstance);

            for (auto const &chunk : wmoInstance->AdtChunks)
            {
                if (chunk.AdtX != X || chunk.AdtY != Y)
                    continue;

                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_wmoInstances.push_back(wmoDefinition.UniqueId);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ = std::min(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ, wmoInstance->Bounds.MinCorner.Z);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ = std::max(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ, wmoInstance->Bounds.MaxCorner.Z);
            }

            Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, wmoInstance->Bounds.MinCorner.Z);
            Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, wmoInstance->Bounds.MaxCorner.Z);
        }

    // Doodads

    if (doodadChunk)
        for (auto const &doodadDefinition : doodadChunk->Doodads)
        {
            auto const doodad = map->GetDoodad(doodadNames[doodadDefinition.NameId]);

            // skip those doodads which have no collision geometry
            if (!doodad->Vertices.size() || !doodad->Indices.size())
                continue;

            const DoodadInstance *doodadInstance;

            // ensure that the instance has been loaded
            if ((doodadInstance = map->GetDoodadInstance(static_cast<unsigned int>(doodadDefinition.UniqueId))) == nullptr)
            {
                math::Matrix transformMatrix;
                doodadDefinition.GetTransformMatrix(transformMatrix);

                doodadInstance = new DoodadInstance(doodad, transformMatrix);
                map->InsertDoodadInstance(static_cast<unsigned int>(doodadDefinition.UniqueId), doodadInstance);
            }

            assert(!!doodad && !!doodadInstance);

            for (auto const &chunk : doodadInstance->AdtChunks)
            {
                if (chunk.AdtX != X || chunk.AdtY != Y)
                    continue;

                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_doodadInstances.push_back(doodadDefinition.UniqueId);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ = std::min(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ, doodadInstance->Bounds.MinCorner.Z);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ = std::max(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ, doodadInstance->Bounds.MaxCorner.Z);
            }

            Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, doodadInstance->Bounds.MinCorner.Z);
            Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, doodadInstance->Bounds.MaxCorner.Z);
        }
}

const AdtChunk *Adt::GetChunk(const int chunkX, const int chunkY) const
{
    return m_chunks[chunkY][chunkX].get();
}
}