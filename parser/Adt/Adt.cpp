#ifdef WIN32
// fix bug in visual studio headers
#    define NOMINMAX
#endif

#include "Adt/Adt.hpp"

#include "Adt/AdtChunk.hpp"
#include "Adt/Chunks/MCNK.hpp"
#include "Adt/Chunks/MDDF.hpp"
#include "Adt/Chunks/MH2O.hpp"
#include "Adt/Chunks/MHDR.hpp"
#include "Adt/Chunks/MMDX.hpp"
#include "Adt/Chunks/MODF.hpp"
#include "Adt/Chunks/MWMO.hpp"
#include "Common.hpp"
#include "Doodad/DoodadPlacement.hpp"
#include "Map/Map.hpp"
#include "MpqManager.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#define ADT_VERSION 18

namespace parser
{
Adt::Adt(Map* map, int adtX, int adtY)
    : X(adtX), Y(adtY), m_map(map),
      Bounds({(32.f - static_cast<float>(adtY) - 1.f) * MeshSettings::AdtSize,
              (32.f - static_cast<float>(adtX) - 1.f) * MeshSettings::AdtSize,
              std::numeric_limits<float>::max()},
             {(32.f - static_cast<float>(adtY)) * MeshSettings::AdtSize,
              (32.f - static_cast<float>(adtX)) * MeshSettings::AdtSize,
              std::numeric_limits<float>::lowest()})
{
    std::unique_ptr<utility::BinaryStream> reader;

    size_t mhdrLocation;

    // if we are working with files from the alpha, the data we need is already
    // in memory.  lets create a unique pointer to the shared memory so other
    // threads don't have to make copies of the same data.  it will never
    // change once loaded so this should be fine.
    if (map->m_isAlphaData)
    {
        reader = std::make_unique<utility::BinaryStream>(map->m_alphaData);

        auto const offset = map->m_adtOffsets[adtX][adtY];

        if (!reader->GetChunkLocation("MHDR", map->m_adtOffsets[adtX][adtY],
            mhdrLocation))
            THROW(Result::MHDR_NOT_FOUND);

        reader->rpos(mhdrLocation);
    }
    else
    {
        std::stringstream ss;

        ss << "World\\maps\\" << map->Name << "\\" << map->Name << "_" << adtX
           << "_" << adtY << ".adt";

        reader = sMpqManager.OpenFile(ss.str());

        if (!reader->GetChunkLocation("MHDR", mhdrLocation))
            THROW(Result::MHDR_NOT_FOUND);
    }

    if (!reader)
        THROW(Result::FAILED_TO_OPEN_ADT);

    auto const old = reader->rpos();
    reader->rpos(0);
    if (reader->Read<std::uint32_t>() != input::AdtChunkType::MVER)
        THROW(Result::MVER_DOES_NOT_BEGIN_ADT_FILE);

    reader->rpos(reader->rpos() + 4);

    if (reader->Read<std::uint32_t>() != ADT_VERSION)
        THROW(Result::ADT_VERSION_IS_INCORRECT);

    reader->rpos(old);

    const input::MHDR header(mhdrLocation, reader.get(), map->m_isAlphaData);

    std::unique_ptr<input::MH2O> liquidChunk(
        header.Mh2oOffset ?
            new input::MH2O(header.Mh2oOffset, reader.get()) :
            nullptr);

    size_t currMcnk;
    if (!reader->GetChunkLocation("MCNK", mhdrLocation, currMcnk))
        THROW(Result::NO_MCNK_CHUNK);

    std::unique_ptr<input::MCNK> chunks[16][16];

    bool hasMclq = false;
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            chunks[y][x] = std::make_unique<input::MCNK>(
                currMcnk, reader.get(), map->m_isAlphaData, adtX, adtY);
            currMcnk += 8 + chunks[y][x]->Size;

            if (chunks[y][x]->HasWater)
                hasMclq = true;
        }

    std::vector<std::string> doodadNames;
    std::vector<std::string> wmoNames;

    if (!map->m_isAlphaData)
    {
        // MMDX
        size_t mmdxLocation;
        if (reader->GetChunkLocation("MMDX", header.DoodadNamesOffset,
                                     mmdxLocation))
        {
            input::MMDX doodadNamesChunk(mmdxLocation, reader.get());
            doodadNames = std::move(doodadNamesChunk.DoodadNames);
        }

        // MWMO
        size_t mwmoLocation;
        if (reader->GetChunkLocation("MWMO", header.WmoNamesOffset,
                                     mwmoLocation))
        {
            input::MWMO wmoNamesChunk(mwmoLocation, reader.get());
            wmoNames = std::move(wmoNamesChunk.WmoNames);
        }
    }

    // MDDF
    size_t mddfLocation;
    std::unique_ptr<input::MDDF> doodadChunk(
        reader->GetChunkLocation("MDDF", header.DoodadPlacementOffset,
                                 mddfLocation) ?
            new input::MDDF(mddfLocation, reader.get()) :
            nullptr);

    // MODF
    size_t modfLocation;
    std::unique_ptr<input::MODF> wmoChunk(
        reader->GetChunkLocation("MODF", header.WmoPlacementOffset,
                                 modfLocation) ?
            new input::MODF(modfLocation, reader.get()) :
            nullptr);

    // Process all data into triangles/indices
    // Terrain

    for (int chunkY = 0; chunkY < 16; ++chunkY)
        for (int chunkX = 0; chunkX < 16; ++chunkX)
        {
            auto const mapChunk = chunks[chunkY][chunkX].get();

            AdtChunk* const chunk = new AdtChunk();

            memcpy(chunk->m_heights, mapChunk->Heights,
                   sizeof(chunk->m_heights));

            chunk->m_terrainVertices = std::move(mapChunk->Positions);
            chunk->m_minZ = mapChunk->MinZ;
            chunk->m_maxZ = mapChunk->MaxZ;
            chunk->m_areaId = mapChunk->AreaId;
            chunk->m_zoneId = sMpqManager.GetZoneId(chunk->m_areaId);

            Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mapChunk->MaxZ);
            Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mapChunk->MinZ);

            memcpy(chunk->m_holeMap, mapChunk->HoleMap,
                   sizeof(chunk->m_holeMap));

            // build index list to exclude holes (8 * 8 quads, 4 triangles per
            // quad, 3 indices per triangle)
            chunk->m_terrainIndices.reserve(8 * 8 * 4 * 3);

            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x)
                {
                    // if this chunk has holes and this quad is one of them,
                    // skip it
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
                    chunk->m_liquidVertices.push_back(
                        {chunk->m_terrainVertices[terrainVert].X,
                         chunk->m_terrainVertices[terrainVert].Y,
                         layer->Heights[y + 0][x + 0]});

                    terrainVert = y * 17 + (x + 1);
                    chunk->m_liquidVertices.push_back(
                        {chunk->m_terrainVertices[terrainVert].X,
                         chunk->m_terrainVertices[terrainVert].Y,
                         layer->Heights[y + 0][x + 1]});

                    terrainVert = (y + 1) * 17 + x;
                    chunk->m_liquidVertices.push_back(
                        {chunk->m_terrainVertices[terrainVert].X,
                         chunk->m_terrainVertices[terrainVert].Y,
                         layer->Heights[y + 1][x + 0]});

                    terrainVert = (y + 1) * 17 + (x + 1);
                    chunk->m_liquidVertices.push_back(
                        {chunk->m_terrainVertices[terrainVert].X,
                         chunk->m_terrainVertices[terrainVert].Y,
                         layer->Heights[y + 1][x + 1]});

                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z,
                                                  layer->Heights[y + 0][x + 0]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z,
                                                  layer->Heights[y + 0][x + 1]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z,
                                                  layer->Heights[y + 1][x + 0]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z,
                                                  layer->Heights[y + 1][x + 1]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z,
                                                  layer->Heights[y + 0][x + 0]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z,
                                                  layer->Heights[y + 0][x + 1]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z,
                                                  layer->Heights[y + 1][x + 0]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z,
                                                  layer->Heights[y + 1][x + 1]);

                    chunk->m_minZ =
                        std::min(chunk->m_minZ, layer->Heights[y + 0][x + 0]);
                    chunk->m_minZ =
                        std::min(chunk->m_minZ, layer->Heights[y + 0][x + 1]);
                    chunk->m_minZ =
                        std::min(chunk->m_minZ, layer->Heights[y + 1][x + 0]);
                    chunk->m_minZ =
                        std::min(chunk->m_minZ, layer->Heights[y + 1][x + 1]);
                    chunk->m_maxZ =
                        std::max(chunk->m_maxZ, layer->Heights[y + 0][x + 0]);
                    chunk->m_maxZ =
                        std::max(chunk->m_maxZ, layer->Heights[y + 0][x + 1]);
                    chunk->m_maxZ =
                        std::max(chunk->m_maxZ, layer->Heights[y + 1][x + 0]);
                    chunk->m_maxZ =
                        std::max(chunk->m_maxZ, layer->Heights[y + 1][x + 1]);

                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(
                        chunk->m_liquidVertices.size() - 4));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(
                        chunk->m_liquidVertices.size() - 2));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(
                        chunk->m_liquidVertices.size() - 3));

                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(
                        chunk->m_liquidVertices.size() - 2));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(
                        chunk->m_liquidVertices.size() - 1));
                    chunk->m_liquidIndices.push_back(static_cast<std::int32_t>(
                        chunk->m_liquidVertices.size() - 3));
                }
        }

    if (hasMclq)
        for (int chunkY = 0; chunkY < 16; ++chunkY)
            for (int chunkX = 0; chunkX < 16; ++chunkX)
            {
                auto const mclqBlock =
                    chunks[chunkY][chunkX]->LiquidChunk.get();

                if (!mclqBlock)
                    continue;

                auto const chunk = m_chunks[chunkY][chunkX].get();

                // four vertices per square, 8x8 squares (max)
                chunk->m_liquidVertices.reserve(chunk->m_liquidVertices.size() +
                                                4 * 8 * 8);

                // six indices (two triangles) per square
                chunk->m_liquidIndices.reserve(chunk->m_liquidIndices.size() +
                                               6 * 8 * 8);

                for (int y = 0; y < 8; ++y)
                    for (int x = 0; x < 8; ++x)
                    {
                        if (mclqBlock->RenderMap[y][x] == 0xF)
                            continue;

                        int terrainVert = y * 17 + x;
                        chunk->m_liquidVertices.push_back(
                            {chunk->m_terrainVertices[terrainVert].X,
                             chunk->m_terrainVertices[terrainVert].Y,
                             mclqBlock->Heights[y][x]});

                        terrainVert = y * 17 + (x + 1);
                        chunk->m_liquidVertices.push_back(
                            {chunk->m_terrainVertices[terrainVert].X,
                             chunk->m_terrainVertices[terrainVert].Y,
                             mclqBlock->Heights[y][x + 1]});

                        terrainVert = (y + 1) * 17 + x;
                        chunk->m_liquidVertices.push_back(
                            {chunk->m_terrainVertices[terrainVert].X,
                             chunk->m_terrainVertices[terrainVert].Y,
                             mclqBlock->Heights[y + 1][x]});

                        terrainVert = (y + 1) * 17 + (x + 1);
                        chunk->m_liquidVertices.push_back(
                            {chunk->m_terrainVertices[terrainVert].X,
                             chunk->m_terrainVertices[terrainVert].Y,
                             mclqBlock->Heights[y + 1][x + 1]});

                        Bounds.MinCorner.Z =
                            std::min(Bounds.MinCorner.Z,
                                     mclqBlock->Heights[y + 0][x + 0]);
                        Bounds.MinCorner.Z =
                            std::min(Bounds.MinCorner.Z,
                                     mclqBlock->Heights[y + 0][x + 1]);
                        Bounds.MinCorner.Z =
                            std::min(Bounds.MinCorner.Z,
                                     mclqBlock->Heights[y + 1][x + 0]);
                        Bounds.MinCorner.Z =
                            std::min(Bounds.MinCorner.Z,
                                     mclqBlock->Heights[y + 1][x + 1]);
                        Bounds.MaxCorner.Z =
                            std::max(Bounds.MaxCorner.Z,
                                     mclqBlock->Heights[y + 0][x + 0]);
                        Bounds.MaxCorner.Z =
                            std::max(Bounds.MaxCorner.Z,
                                     mclqBlock->Heights[y + 0][x + 1]);
                        Bounds.MaxCorner.Z =
                            std::max(Bounds.MaxCorner.Z,
                                     mclqBlock->Heights[y + 1][x + 0]);
                        Bounds.MaxCorner.Z =
                            std::max(Bounds.MaxCorner.Z,
                                     mclqBlock->Heights[y + 1][x + 1]);

                        chunk->m_minZ = std::min(
                            chunk->m_minZ, mclqBlock->Heights[y + 0][x + 0]);
                        chunk->m_minZ = std::min(
                            chunk->m_minZ, mclqBlock->Heights[y + 0][x + 1]);
                        chunk->m_minZ = std::min(
                            chunk->m_minZ, mclqBlock->Heights[y + 1][x + 0]);
                        chunk->m_minZ = std::min(
                            chunk->m_minZ, mclqBlock->Heights[y + 1][x + 1]);
                        chunk->m_maxZ = std::max(
                            chunk->m_maxZ, mclqBlock->Heights[y + 0][x + 0]);
                        chunk->m_maxZ = std::max(
                            chunk->m_maxZ, mclqBlock->Heights[y + 0][x + 1]);
                        chunk->m_maxZ = std::max(
                            chunk->m_maxZ, mclqBlock->Heights[y + 1][x + 0]);
                        chunk->m_maxZ = std::max(
                            chunk->m_maxZ, mclqBlock->Heights[y + 1][x + 1]);

                        chunk->m_liquidIndices.push_back(
                            static_cast<std::int32_t>(
                                chunk->m_liquidVertices.size() - 4));
                        chunk->m_liquidIndices.push_back(
                            static_cast<std::int32_t>(
                                chunk->m_liquidVertices.size() - 2));
                        chunk->m_liquidIndices.push_back(
                            static_cast<std::int32_t>(
                                chunk->m_liquidVertices.size() - 3));

                        chunk->m_liquidIndices.push_back(
                            static_cast<std::int32_t>(
                                chunk->m_liquidVertices.size() - 2));
                        chunk->m_liquidIndices.push_back(
                            static_cast<std::int32_t>(
                                chunk->m_liquidVertices.size() - 1));
                        chunk->m_liquidIndices.push_back(
                            static_cast<std::int32_t>(
                                chunk->m_liquidVertices.size() - 3));
                    }
            }

    // WMOs

    if (wmoChunk)
        for (auto const& wmoDefinition : wmoChunk->Wmos)
        {
            std::string wmoName;
            if (map->m_isAlphaData)
            {
                assert(wmoDefinition.NameId < map->m_wmoNames.size());
                wmoName = map->m_wmoNames[wmoDefinition.NameId];
            }
            else
            {
                assert(wmoDefinition.NameId < wmoNames.size());
                wmoName = wmoNames[wmoDefinition.NameId];
            }

            auto const wmo = map->GetWmo(wmoName);
            const WmoInstance* wmoInstance;

            // ensure that the instance has been loaded
            if ((wmoInstance = map->GetWmoInstance(static_cast<unsigned int>(
                     wmoDefinition.UniqueId))) == nullptr)
            {
                math::BoundingBox bounds;
                wmoDefinition.GetBoundingBox(bounds);

                math::Matrix transformMatrix;
                wmoDefinition.GetTransformMatrix(transformMatrix);

                wmoInstance = new WmoInstance(wmo, wmoDefinition.DoodadSet,
                                              wmoDefinition.NameSet, bounds,
                                              transformMatrix);
                map->InsertWmoInstance(wmoDefinition.UniqueId, wmoInstance);
            }

            assert(!!wmo && !!wmoInstance);

            for (auto const& chunk : wmoInstance->AdtChunks)
            {
                if (chunk.AdtX != X || chunk.AdtY != Y)
                    continue;

                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_wmoInstances.push_back(
                    wmoDefinition.UniqueId);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ =
                    std::min(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ,
                             wmoInstance->Bounds.MinCorner.Z);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ =
                    std::max(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ,
                             wmoInstance->Bounds.MaxCorner.Z);
            }

            Bounds.MinCorner.Z =
                std::min(Bounds.MinCorner.Z, wmoInstance->Bounds.MinCorner.Z);
            Bounds.MaxCorner.Z =
                std::max(Bounds.MaxCorner.Z, wmoInstance->Bounds.MaxCorner.Z);
        }

    // Doodads

    if (doodadChunk)
        for (auto const& doodadDefinition : doodadChunk->Doodads)
        {
            std::string doodadName;
            if (map->m_isAlphaData)
            {
                assert(doodadDefinition.NameId < m_map->m_doodadNames.size());
                doodadName = m_map->m_doodadNames[doodadDefinition.NameId];
            }
            else
            {
                assert(doodadDefinition.NameId < doodadNames.size());
                doodadName = doodadNames[doodadDefinition.NameId];
            }

            auto const doodad = map->GetDoodad(doodadName);

            // skip those doodads which have no collision geometry
            if (!doodad->Vertices.size() || !doodad->Indices.size())
                continue;

            const DoodadInstance* doodadInstance;

            // ensure that the instance has been loaded
            if ((doodadInstance = map->GetDoodadInstance(
                     static_cast<unsigned int>(doodadDefinition.UniqueId))) ==
                nullptr)
            {
                math::Matrix transformMatrix;
                doodadDefinition.GetTransformMatrix(transformMatrix);

                doodadInstance = new DoodadInstance(doodad, transformMatrix);
                map->InsertDoodadInstance(
                    static_cast<unsigned int>(doodadDefinition.UniqueId),
                    doodadInstance);
            }

            assert(!!doodad && !!doodadInstance);

            for (auto const& chunk : doodadInstance->AdtChunks)
            {
                if (chunk.AdtX != X || chunk.AdtY != Y)
                    continue;

                m_chunks[chunk.ChunkY][chunk.ChunkX]
                    ->m_doodadInstances.push_back(doodadDefinition.UniqueId);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ =
                    std::min(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_minZ,
                             doodadInstance->Bounds.MinCorner.Z);
                m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ =
                    std::max(m_chunks[chunk.ChunkY][chunk.ChunkX]->m_maxZ,
                             doodadInstance->Bounds.MaxCorner.Z);
            }

            Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z,
                                          doodadInstance->Bounds.MinCorner.Z);
            Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z,
                                          doodadInstance->Bounds.MaxCorner.Z);
        }
}

const AdtChunk* Adt::GetChunk(const int chunkX, const int chunkY) const
{
    return m_chunks[chunkY][chunkX].get();
}
} // namespace parser