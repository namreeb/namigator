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

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/Directory.hpp"
#include "utility/Include/MathHelper.hpp"

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
            (32.f - (float)adtY - 1.f)*utility::MathHelper::AdtSize,
            (32.f - (float)adtX - 1.f)*utility::MathHelper::AdtSize,
            std::numeric_limits<float>::max()
        },
        {
            (32.f - (float)adtY)*utility::MathHelper::AdtSize,
            (32.f - (float)adtX)*utility::MathHelper::AdtSize,
            std::numeric_limits<float>::lowest()
        })
{
    std::stringstream ss;

    ss << "World\\maps\\" << map->Name << "\\" << map->Name << "_" << adtX << "_" << adtY << ".adt";

    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(ss.str()));

    if (reader->Read<unsigned int>() != input::AdtChunkType::MVER)
        THROW("MVER does not begin ADT file");

    reader->Slide(4);

    if (reader->Read<unsigned int>() != ADT_VERSION)
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
            chunks[y][x].reset(new input::MCNK(currMcnk, reader.get()));
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

            chunk->m_terrainVertices = std::move(mapChunk->Positions);

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

                    const int currIndex = y * 17 + x;

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

                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 0][x + 0]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 0][x + 0]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 0][x + 1]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 0][x + 1]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 1][x + 0]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 1][x + 0]);
                    Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, layer->Heights[y + 1][x + 1]);
                    Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, layer->Heights[y + 1][x + 1]);

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

                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 0][x + 0]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 0][x + 0]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 0][x + 1]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 0][x + 1]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 1][x + 0]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 1][x + 0]);
                        Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, mclqBlock->Heights[y + 1][x + 1]);
                        Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, mclqBlock->Heights[y + 1][x + 1]);

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
            auto wmo = map->GetWmo(wmoNames[wmoDefinition.NameId]);
            const WmoInstance *wmoInstance;

            // ensure that the instance has been loaded
            if ((wmoInstance = map->GetWmoInstance(wmoDefinition.UniqueId)) == nullptr)
            {
                utility::BoundingBox bounds;
                wmoDefinition.GetBoundingBox(bounds);

                utility::Vertex origin;
                wmoDefinition.GetOrigin(origin);

                utility::Matrix transformMatrix;
                wmoDefinition.GetTransformMatrix(transformMatrix);

                wmoInstance = new WmoInstance(wmo, wmoDefinition.DoodadSet, bounds, origin, transformMatrix);
                map->InsertWmoInstance(wmoDefinition.UniqueId, wmoInstance);
            }

            assert(!!wmo && !!wmoInstance);

            std::vector<utility::Vertex> vertices;
            std::vector<int> indices;

            wmoInstance->BuildTriangles(vertices, indices);

            // iterate over all vertices to determine which chunks have the wmo
            // FIXME - this does not uses wmo liquid or doodad vertices, which perhaps it should?  or maybe we dont even need this?
            for (auto const &vertex : vertices)
            {
                // if this vertex doesn't even fall on the adt, there is no way it can land on a chunk
                // if it falls on the edge, always handle it the same way (<= in the min case on both axis)
                if (vertex.X > Bounds.MaxCorner.X || vertex.X <= Bounds.MinCorner.X || vertex.Y > Bounds.MaxCorner.Y || vertex.Y <= Bounds.MinCorner.Y)
                    continue;

                const int chunkX = static_cast<int>((Bounds.MaxCorner.Y - vertex.Y) / utility::MathHelper::AdtChunkSize);
                const int chunkY = static_cast<int>((Bounds.MaxCorner.X - vertex.X) / utility::MathHelper::AdtChunkSize);

                assert(chunkX < 16 && chunkY < 16);

                Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, vertex.Z);
                Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, vertex.Z);

                m_chunks[chunkY][chunkX]->m_wmos.insert(wmoDefinition.UniqueId);
            }
        }

    // Doodads

    if (doodadChunk)
        for (auto const &doodadDefinition : doodadChunk->Doodads)
        {
            auto doodad = map->GetDoodad(doodadNames[doodadDefinition.NameId]);

            // skip those doodads which have no collision geometry
            if (!doodad->Vertices.size() || !doodad->Indices.size())
                continue;

            const DoodadInstance *doodadInstance;

            // ensure that the instance has been loaded
            if ((doodadInstance = map->GetDoodadInstance(doodadDefinition.UniqueId)) == nullptr)
            {
                utility::Vertex origin;
                doodadDefinition.GetOrigin(origin);

                utility::Matrix transformMatrix;
                doodadDefinition.GetTransformMatrix(transformMatrix);

                doodadInstance = new DoodadInstance(doodad, origin, transformMatrix);
                map->InsertDoodadInstance(doodadDefinition.UniqueId, doodadInstance);
            }

            assert(!!doodad && !!doodadInstance);

            std::vector<utility::Vertex> vertices;
            std::vector<int> indices;

            doodadInstance->BuildTriangles(vertices, indices);

            // iterate over all vertices to determine which chunks have the doodad
            // FIXME - do we even need this?
            for (auto const &vertex : vertices)
            {
                // if this vertex doesn't even fall on the adt, there is no way it can land on a chunk
                if (vertex.X > Bounds.MaxCorner.X || vertex.X <= Bounds.MinCorner.X || vertex.Y > Bounds.MaxCorner.Y || vertex.Y <= Bounds.MinCorner.Y)
                    continue;

                const int chunkX = static_cast<int>((Bounds.MaxCorner.Y - vertex.Y) / utility::MathHelper::AdtChunkSize);
                const int chunkY = static_cast<int>((Bounds.MaxCorner.X - vertex.X) / utility::MathHelper::AdtChunkSize);

                assert(chunkX < 16 && chunkY < 16);

                Bounds.MaxCorner.Z = std::max(Bounds.MaxCorner.Z, vertex.Z);
                Bounds.MinCorner.Z = std::min(Bounds.MinCorner.Z, vertex.Z);

                m_chunks[chunkY][chunkX]->m_doodads.insert(doodadDefinition.UniqueId);
            }
        }
}

#ifdef _DEBUG
void Adt::WriteObjFile() const
{
    std::stringstream ss;

    ss << m_map->Name << "_" << X  << "_" << Y << ".obj";

    std::ofstream out(ss.str());

    size_t indexOffset = 1;
    std::set<unsigned int> dumpedWmos;
    std::set<unsigned int> dumpedDoodads;

    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            auto chunk = m_chunks[y][x].get();

            // terrain vertices
            out << "# terrain vertices (" << chunk->m_terrainVertices.size() << ")" << std::endl;
            for (unsigned int i = 0; i < chunk->m_terrainVertices.size(); ++i)
                out << "v " << -chunk->m_terrainVertices[i].Y
                    << " "  <<  chunk->m_terrainVertices[i].Z
                    << " "  << -chunk->m_terrainVertices[i].X << std::endl;

            // terrain indices
            out << "# terrain indices (" << chunk->m_terrainIndices.size() << ")" << std::endl;
            for (unsigned int i = 0; i < chunk->m_terrainIndices.size(); i += 3)
                out << "f " << indexOffset + chunk->m_terrainIndices[i]
                    << " "  << indexOffset + chunk->m_terrainIndices[i+1]
                    << " "  << indexOffset + chunk->m_terrainIndices[i+2] << std::endl;

            indexOffset += chunk->m_terrainVertices.size();

            // water vertices
            out << "# water vertices (" << chunk->m_liquidVertices.size() << ")" << std::endl;
            for (unsigned int i = 0; i < chunk->m_liquidVertices.size(); ++i)
                out << "v " << -chunk->m_liquidVertices[i].Y
                    << " "  <<  chunk->m_liquidVertices[i].Z
                    << " "  << -chunk->m_liquidVertices[i].X << std::endl;

            // water indices
            out << "# water indices (" << chunk->m_liquidIndices.size() << ")" << std::endl;
            for (unsigned int i = 0; i < chunk->m_liquidIndices.size(); i += 3)
                out << "f " << indexOffset + chunk->m_liquidIndices[i]   << " "
                            << indexOffset + chunk->m_liquidIndices[i+1] << " "
                            << indexOffset + chunk->m_liquidIndices[i+2] << std::endl;

            indexOffset += chunk->m_liquidVertices.size();

            // wmo vertices and indices
            out << "# wmo vertices / indices (including liquids and doodads)" << std::endl;
            for (auto id = chunk->m_wmos.cbegin(); id != chunk->m_wmos.cend(); ++id)
            {
                // if this wmo has already been dumped as part of another chunk, skip it
                if (dumpedWmos.find(*id) != dumpedWmos.end())
                    continue;

                dumpedWmos.insert(*id);

                auto wmoInstance = m_map->GetWmoInstance(*id);
                std::vector<utility::Vertex> vertices;
                std::vector<int> indices;

                wmoInstance->BuildTriangles(vertices, indices);

                for (auto const &vertex : vertices)
                    out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

                for (size_t i = 0; i < indices.size(); i+=3)
                    out << "f " << indexOffset + indices[i]   << " " 
                                << indexOffset + indices[i+1] << " "
                                << indexOffset + indices[i+2] << std::endl;

                indexOffset += vertices.size();

                wmoInstance->BuildLiquidTriangles(vertices, indices);

                for (auto const &vertex : vertices)
                    out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

                for (unsigned int i = 0; i < indices.size(); i+=3)
                    out << "f " << indexOffset + indices[i]   << " " 
                                << indexOffset + indices[i+1] << " "
                                << indexOffset + indices[i+2] << std::endl;

                indexOffset += vertices.size();

                wmoInstance->BuildDoodadTriangles(vertices, indices);

                for (auto const &vertex : vertices)
                    out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

                for (unsigned int i = 0; i < indices.size(); i+=3)
                    out << "f " << indexOffset + indices[i]   << " " 
                                << indexOffset + indices[i+1] << " "
                                << indexOffset + indices[i+2] << std::endl;

                indexOffset += vertices.size();
            }

            // doodad vertices
            out << "# doodad vertices" << std::endl;
            for (auto id = chunk->m_doodads.cbegin(); id != chunk->m_doodads.cend(); ++id)
            {
                // if this wmo has already been dumped as part of another chunk, skip it
                if (dumpedDoodads.find(*id) != dumpedDoodads.end())
                    continue;

                dumpedDoodads.insert(*id);

                const DoodadInstance *doodadInstance = m_map->GetDoodadInstance(*id);
                std::vector<utility::Vertex> vertices;
                std::vector<int> indices;

                doodadInstance->BuildTriangles(vertices, indices);

                for (auto const &vertex : vertices)
                    out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

                for (size_t i = 0; i < indices.size(); i+=3)
                    out << "f " << indexOffset + indices[i]   << " " 
                                << indexOffset + indices[i+1] << " "
                                << indexOffset + indices[i+2] << std::endl;

                indexOffset += vertices.size();
            }
        }

    out.close();
}

size_t Adt::GetWmoCount() const
{
    std::set<unsigned int> wmos;

    for (int chunkY = 0; chunkY < 16; ++chunkY)
        for (int chunkX = 0; chunkX < 16; ++chunkX)
            wmos.insert(m_chunks[chunkY][chunkX]->m_wmos.cbegin(), m_chunks[chunkY][chunkX]->m_wmos.cend());

    return wmos.size();
}
#endif

const AdtChunk *Adt::GetChunk(const int chunkX, const int chunkY) const
{
    return m_chunks[chunkY][chunkX].get();
}
}